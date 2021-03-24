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

#pragma once

#include "AppEvent.h"

#include <platform/CHIPDeviceLayer.h>

struct k_timer;

class AppTask
{
public:
    int StartApp();

    void PostEvent(const AppEvent * event);
    void PostEvent(AppEvent::Type type, AppEvent::Handler handler);

private:
    friend AppTask & GetAppTask();

    int Init();

    void DispatchEvent(AppEvent * event);
    static void ButtonPushHandler(AppEvent * event);
    static void ButtonReleaseHandler(AppEvent * event);
    static void FunctionTimerHandler(AppEvent * event);

    static void ButtonStateHandler(uint32_t buttonState, uint32_t hasChanged);

    static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);

    static AppTask sAppTask;
};

inline AppTask & GetAppTask()
{
    return AppTask::sAppTask;
}
