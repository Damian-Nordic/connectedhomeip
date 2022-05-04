/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include "OTAImageProcessorImpl.h"

#include <app/clusters/ota-requestor/OTADownloader.h>
#include <lib/support/CodeUtils.h>
#include <platform/CHIPDeviceLayer.h>
#include <system/SystemError.h>

#include <dfu/dfu_multi_image.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_mcuboot.h>
#include <dfu/mcuboot.h>
#include <logging/log.h>
#include <pm/device.h>
#include <sys/reboot.h>

namespace chip {
namespace DeviceLayer {

CHIP_ERROR OTAImageProcessorImpl::PrepareDownload()
{
    VerifyOrReturnError(mDownloader != nullptr, CHIP_ERROR_INCORRECT_STATE);

    return DeviceLayer::SystemLayer().ScheduleLambda([this] { mDownloader->OnPreparedForDownload(PrepareDownloadImpl()); });
}

CHIP_ERROR OTAImageProcessorImpl::PrepareDownloadImpl()
{
    mHeaderParser.Init();
    ReturnErrorOnFailure(System::MapErrorZephyr(dfu_multi_image_init(mBuffer, sizeof(mBuffer), nullptr)));

    for (int image_id = 0; image_id < 2; ++image_id)
    {
        dfu_image_writer writer = { .image_id = image_id,
                                    .open =
                                        [](int id) {
                                            ChipLogError(SoftwareUpdate, "OPEN");
                                            return dfu_target_init(DFU_TARGET_IMAGE_TYPE_MCUBOOT, id, /* size */ 0, nullptr);
                                        },
                                    .write =
                                        [](const uint8_t * chunk, size_t chunk_size) {
                                            ChipLogError(SoftwareUpdate, "WRITE");
                                            return dfu_target_write(chunk, chunk_size);
                                        },
                                    .close =
                                        [](bool success) {
                                            ChipLogError(SoftwareUpdate, "CLOSE");
                                            return dfu_target_done(success);
                                        } };

        ReturnErrorOnFailure(System::MapErrorZephyr(dfu_multi_image_register_writer(&writer)));
    }

    ReturnErrorOnFailure(System::MapErrorZephyr(dfu_target_mcuboot_set_buf(mBuffer, sizeof(mBuffer))));

    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Finalize()
{
    return System::MapErrorZephyr(dfu_multi_image_done(true));
}

CHIP_ERROR OTAImageProcessorImpl::Abort()
{
    return System::MapErrorZephyr(dfu_multi_image_done(false));
}

CHIP_ERROR OTAImageProcessorImpl::Apply()
{
    // schedule update of all possible targets by caling this function with argument -1
    int err = dfu_target_schedule_update(-1);

#ifdef CONFIG_CHIP_OTA_REQUESTOR_REBOOT_ON_APPLY
    if (!err)
    {
        return SystemLayer().StartTimer(
            System::Clock::Milliseconds32(CHIP_DEVICE_CONFIG_OTA_REQUESTOR_REBOOT_DELAY_MS),
            [](System::Layer *, void * /* context */) {
                PlatformMgr().HandleServerShuttingDown();
                k_msleep(CHIP_DEVICE_CONFIG_SERVER_SHUTDOWN_ACTIONS_SLEEP_MS);
                sys_reboot(SYS_REBOOT_WARM);
            },
            nullptr /* context */);
    }
    else
    {
        return System::MapErrorZephyr(err);
    }
#else
    return System::MapErrorZephyr(err);
#endif
}

CHIP_ERROR OTAImageProcessorImpl::ProcessBlock(ByteSpan & aBlock)
{
    VerifyOrReturnError(mDownloader != nullptr, CHIP_ERROR_INCORRECT_STATE);

    CHIP_ERROR error = ProcessHeader(aBlock);
    if (error == CHIP_NO_ERROR)
    {
        // DFU target library buffers data internally, so do not clone the block data.
        error = System::MapErrorZephyr(dfu_multi_image_write(mParams.downloadedBytes, aBlock.data(), aBlock.size()));
        mParams.downloadedBytes += aBlock.size();
    }

    // Report the result back to the downloader asynchronously.
    return DeviceLayer::SystemLayer().ScheduleLambda([this, error, aBlock] {
        if (error == CHIP_NO_ERROR)
        {
            mDownloader->FetchNextData();
        }
        else
        {
            mDownloader->EndDownload(error);
        }
    });
}

bool OTAImageProcessorImpl::IsFirstImageRun()
{
    return mcuboot_swap_type() == BOOT_SWAP_TYPE_REVERT;
}

CHIP_ERROR OTAImageProcessorImpl::ConfirmCurrentImage()
{
    return System::MapErrorZephyr(boot_write_img_confirmed());
}

CHIP_ERROR OTAImageProcessorImpl::ProcessHeader(ByteSpan & aBlock)
{
    if (mHeaderParser.IsInitialized())
    {
        OTAImageHeader header;
        CHIP_ERROR error = mHeaderParser.AccumulateAndDecode(aBlock, header);

        // Needs more data to decode the header
        ReturnErrorCodeIf(error == CHIP_ERROR_BUFFER_TOO_SMALL, CHIP_NO_ERROR);
        ReturnErrorOnFailure(error);

        mParams.totalFileBytes = header.mPayloadSize;
        mHeaderParser.Clear();
    }

    return CHIP_NO_ERROR;
}

// external flash power consumption optimization
void ExtFlashHandler::DoAction(Action aAction)
{
#if CONFIG_PM_DEVICE && CONFIG_NORDIC_QSPI_NOR && !CONFIG_SOC_NRF52840 // nRF52 is optimized per default
    // utilize the QSPI driver sleep power mode
    const auto * qspi_dev = device_get_binding(DT_LABEL(DT_INST(0, nordic_qspi_nor)));
    if (qspi_dev)
    {
        const auto requestedAction = Action::WAKE_UP == aAction ? PM_DEVICE_ACTION_RESUME : PM_DEVICE_ACTION_SUSPEND;
        (void) pm_device_action_run(qspi_dev, requestedAction); // not much can be done in case of a failure
    }
#endif
}

OTAImageProcessorImplPMDevice::OTAImageProcessorImplPMDevice(ExtFlashHandler & aHandler) : mHandler(aHandler)
{
    mHandler.DoAction(ExtFlashHandler::Action::SLEEP);
}

CHIP_ERROR OTAImageProcessorImplPMDevice::PrepareDownload()
{
    mHandler.DoAction(ExtFlashHandler::Action::WAKE_UP);
    return OTAImageProcessorImpl::PrepareDownload();
}

CHIP_ERROR OTAImageProcessorImplPMDevice::Abort()
{
    auto status = OTAImageProcessorImpl::Abort();
    mHandler.DoAction(ExtFlashHandler::Action::SLEEP);
    return status;
}

CHIP_ERROR OTAImageProcessorImplPMDevice::Apply()
{
    auto status = OTAImageProcessorImpl::Apply();
    mHandler.DoAction(ExtFlashHandler::Action::SLEEP);
    return status;
}

} // namespace DeviceLayer
} // namespace chip
