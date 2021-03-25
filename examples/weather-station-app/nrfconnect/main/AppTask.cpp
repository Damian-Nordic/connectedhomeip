/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AppTask.h"
#include "OnboardingCodesUtil.h"
#include "Server.h"
#include "Service.h"

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
#include "NFCWidget.h"
#endif

#include <platform/CHIPDeviceLayer.h>

#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr.h>

#include <algorithm>

using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app);

namespace {

enum class FunctionTimerMode
{
    kDisabled,
    kFactoryResetTrigger,
    kFactoryResetComplete
};

constexpr size_t kAppEventQueueSize             = 10;
constexpr size_t kFactoryResetTriggerTimeoutMs  = 3000;
constexpr size_t kFactoryResetCompleteTimeoutMs = 3000;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
k_timer sFunctionTimer;
FunctionTimerMode sFunctionTimerMode = FunctionTimerMode::kDisabled;

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
NFCWidget sNFC;

void StartNFCTag()
{
    VerifyOrReturn(!sNFC.IsTagEmulationStarted());

    LOG_INF("Starting NFC Tag emulation");

    // Get QR Code and emulate its content using NFC tag
    std::string QRCode;

    if (GetQRCode(QRCode, chip::RendezvousInformationFlags::kBLE) != CHIP_NO_ERROR)
    {
        LOG_ERR("Getting QR code payload failed");
        return;
    }

    // TODO: Issue #4504 - Remove replacing spaces with _ after problem described in #415 will be fixed.
    std::replace(QRCode.begin(), QRCode.end(), ' ', '_');

    if (sNFC.StartTagEmulation(QRCode.c_str(), QRCode.size()) < 0)
    {
        LOG_ERR("Starting NFC Tag emulation failed");
        return;
    }
}
#endif

} // namespace

AppTask AppTask::sAppTask;

int AppTask::Init()
{
    // Initialize buttons
    int ret = dk_buttons_init(ButtonStateHandler);
    if (ret)
    {
        LOG_ERR("dk_buttons_init() failed");
        return ret;
    }

    // Initialize timer
    k_timer_init(
        &sFunctionTimer, [](k_timer *) { sAppTask.PostEvent(AppEvent::Type::kTimer, FunctionTimerHandler); }, nullptr);

    // Init ZCL Data Model and start server
    InitServer();
    ConfigurationMgr().LogDeviceConfig();
    PrintOnboardingCodes(chip::RendezvousInformationFlags::kBLE);

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
    ret = sNFC.Init(ConnectivityMgr());
    if (ret)
    {
        LOG_ERR("NFC initialization failed");
        return ret;
    }

    PlatformMgr().AddEventHandler(AppTask::ChipEventHandler, 0);
#endif

    return 0;
}

int AppTask::StartApp()
{
    int ret = Init();

    if (ret)
    {
        LOG_ERR("AppTask.Init() failed");
        return ret;
    }

    AppEvent event = {};

    while (true)
    {
        ret = k_msgq_get(&sAppEventQueue, &event, K_SECONDS(10));

        while (!ret)
        {
            DispatchEvent(&event);
            ret = k_msgq_get(&sAppEventQueue, &event, K_NO_WAIT);
        }

        // TODO: fetch sensor data and update clusters
    }
}

void AppTask::PostEvent(const AppEvent * event)
{
    if (k_msgq_put(&sAppEventQueue, event, K_NO_WAIT))
    {
        LOG_ERR("Failed to post event to app task event queue");
    }
}

void AppTask::PostEvent(AppEvent::Type type, AppEvent::Handler handler)
{
    AppEvent event;
    event.mType    = type;
    event.mHandler = handler;
    PostEvent(&event);
}

void AppTask::DispatchEvent(AppEvent * event)
{
    assert(event->mHandler);
    event->mHandler(event);
}

void AppTask::ButtonPushHandler(AppEvent *)
{
    sFunctionTimerMode = FunctionTimerMode::kFactoryResetTrigger;
    k_timer_start(&sFunctionTimer, K_MSEC(kFactoryResetTriggerTimeoutMs), K_NO_WAIT);
}

void AppTask::ButtonReleaseHandler(AppEvent *)
{
    // If the button was released within the first kFactoryResetTriggerTimeoutMs, open the BLE pairing window.
    if (sFunctionTimerMode == FunctionTimerMode::kFactoryResetTrigger)
    {
        const auto error = OpenDefaultPairingWindow(chip::ResetAdmins::kNo);

        if (error == CHIP_NO_ERROR)
            LOG_INF("Enabled BLE advertising");
        else
            LOG_ERR("OpenDefaultPairingWindow() failed: %s", chip::ErrorStr(error));

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
        StartNFCTag();
#endif
    }

    sFunctionTimerMode = FunctionTimerMode::kDisabled;
    k_timer_stop(&sFunctionTimer);
}

void AppTask::FunctionTimerHandler(AppEvent *)
{
    switch (sFunctionTimerMode)
    {
    case FunctionTimerMode::kFactoryResetTrigger:
        LOG_INF("Factory Reset triggered. Release button within %ums to cancel.", kFactoryResetCompleteTimeoutMs);
        sFunctionTimerMode = FunctionTimerMode::kFactoryResetComplete;
        k_timer_start(&sFunctionTimer, K_MSEC(kFactoryResetCompleteTimeoutMs), K_NO_WAIT);
        break;
    case FunctionTimerMode::kFactoryResetComplete:
        ConfigurationMgr().InitiateFactoryReset();
        break;
    default:
        break;
    }
}

void AppTask::ButtonStateHandler(uint32_t buttonState, uint32_t hasChanged)
{
    if (hasChanged & DK_BTN1_MSK)
    {
        if (buttonState & DK_BTN1_MSK)
            sAppTask.PostEvent(AppEvent::Type::kButtonPush, ButtonPushHandler);
        else
            sAppTask.PostEvent(AppEvent::Type::kButtonRelease, ButtonReleaseHandler);
    }
}

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
void AppTask::ChipEventHandler(const ChipDeviceEvent * event, intptr_t /* arg */)
{
    if (event->Type == DeviceEventType::kCHIPoBLEAdvertisingChange &&
        event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped && sNFC.IsTagEmulationStarted())
    {
        LOG_INF("Stopping NFC Tag emulation");
        sNFC.StopTagEmulation();
    }
}
#endif
