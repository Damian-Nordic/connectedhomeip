/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#include "QRCodeUtil.h"

#include <platform/CHIPDeviceLayer.h>

#include <setup_payload/QRCodeSetupPayloadGenerator.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(app);

void PrintQRCode()
{
    using namespace ::chip::DeviceLayer;

    CHIP_ERROR err = CHIP_NO_ERROR;

    chip::SetupPayload payload;
    payload.version               = 1;
    payload.rendezvousInformation = chip::RendezvousInformationFlags::kBLE;

    err = ConfigurationMgr().GetSetupPinCode(payload.setUpPINCode);
    if (err != CHIP_NO_ERROR)
    {
        LOG_INF("ConfigurationMgr().GetSetupPinCode() failed: %s", log_strdup(chip::ErrorStr(err)));
    }

    err = ConfigurationMgr().GetSetupDiscriminator(payload.discriminator);
    if (err != CHIP_NO_ERROR)
    {
        LOG_INF("ConfigurationMgr().GetSetupDiscriminator() failed: %s", log_strdup(chip::ErrorStr(err)));
    }

    err = ConfigurationMgr().GetVendorId(payload.vendorID);
    if (err != CHIP_NO_ERROR)
    {
        LOG_INF("ConfigurationMgr().GetVendorId() failed: %s", log_strdup(chip::ErrorStr(err)));
    }

    err = ConfigurationMgr().GetProductId(payload.productID);
    if (err != CHIP_NO_ERROR)
    {
        LOG_INF("ConfigurationMgr().GetProductId() failed: %s", log_strdup(chip::ErrorStr(err)));
    }

    chip::QRCodeSetupPayloadGenerator generator(payload);

    // TODO: Usage of STL will significantly increase the image size, this should be changed to more efficient method for
    // generating payload
    std::string result;
    err = generator.payloadBase41Representation(result);
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("Failed to generate QR Code");
    }

    LOG_INF("SetupPINCode: [%" PRIu32 "]", payload.setUpPINCode);
    // There might be whitespace in setup QRCode, add brackets to make it clearer.
    LOG_INF("SetupQRCode:  [%s]", log_strdup(result.c_str()));
}