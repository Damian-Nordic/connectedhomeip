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

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_DEVICE_LAYER
#include <platform/CHIPDeviceLayer.h>
#endif

#include <app-common/zap-generated/enums.h>
#include <app/server/Server.h>
#include <controller-clusters/zap-generated/CHIPClientCallbacks.h>
#include <controller-clusters/zap-generated/CHIPClusters.h>
#include <controller/CHIPDevice.h>
#include <controller/CHIPDeviceController.h>
#include <controller/ExampleOperationalCredentialsIssuer.h>
#include <lib/core/CHIPCallback.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/CHIPError.h>
#include <lib/shell/Commands.h>
#include <lib/shell/Engine.h>
#include <lib/shell/commands/Help.h>
#include <lib/support/CHIPArgParser.hpp>
#include <lib/support/CHIPMem.h>
#include <lib/support/RandUtils.h>
#include <lib/support/Span.h>
#include <lib/support/logging/CHIPLogging.h>
#include <messaging/ExchangeContext.h>
#include <messaging/ExchangeDelegate.h>
#include <messaging/ExchangeMgr.h>
#include <messaging/Flags.h>
#include <protocols/bdx/BdxMessages.h>
#include <protocols/bdx/BdxTransferSession.h>
#include <protocols/bdx/TransferFacilitator.h>
#include <utility>

using chip::DeviceLayer::ConnectivityMgr;
using namespace chip;
using namespace chip::Controller;
using namespace chip::Callback;

namespace chip {
namespace Shell {
namespace {

Shell::Engine sSubShell;
Controller::DeviceController sController;
constexpr uint64_t kOtaProviderDeviceId = 841375;

using namespace chip::bdx;

class BdxDownloader : public chip::bdx::Initiator
{
public:
    void SetInitialExchange(chip::Messaging::ExchangeContext * ec) { mExchangeCtx = ec; }

private:
    // inherited from bdx::Endpoint
    void HandleTransferSessionOutput(chip::bdx::TransferSession::OutputEvent & event)
    {
        CHIP_ERROR err = CHIP_NO_ERROR;

        if (event.EventType != TransferSession::OutputEventType::kNone)
        {
            ChipLogProgress(BDX, "OutputEvent type: %u", static_cast<uint32_t>(event.EventType));
        }

        switch (event.EventType)
        {
        case TransferSession::OutputEventType::kNone:
            if (mIsTransferComplete)
            {
                ChipLogProgress(BDX, "Transfer complete!");
                mTransfer.Reset();
                mIsTransferComplete = false;
            }
            break;
        case TransferSession::OutputEventType::kMsgToSend: {
            chip::Messaging::SendFlags sendFlags;
            VerifyOrReturn(mExchangeCtx != nullptr, ChipLogError(BDX, "mExchangeContext is null"));
            if (event.msgTypeData.MessageType == static_cast<uint8_t>(MessageType::ReceiveInit))
            {
                sendFlags.Set(chip::Messaging::SendMessageFlags::kFromInitiator);
            }
            if (event.msgTypeData.MessageType != static_cast<uint8_t>(MessageType::BlockAckEOF))
            {
                sendFlags.Set(chip::Messaging::SendMessageFlags::kExpectResponse);
            }
            err = mExchangeCtx->SendMessage(event.msgTypeData.ProtocolId, event.msgTypeData.MessageType, std::move(event.MsgData),
                                            sendFlags);
            VerifyOrReturn(err == CHIP_NO_ERROR, ChipLogError(BDX, "SendMessage failed: %s", chip::ErrorStr(err)));
            break;
        }
        case TransferSession::OutputEventType::kAcceptReceived:
            VerifyOrReturn(CHIP_NO_ERROR == mTransfer.PrepareBlockQuery(), ChipLogError(BDX, "PrepareBlockQuery failed"));
            break;
        case TransferSession::OutputEventType::kBlockReceived: {
            ChipLogProgress(BDX, "Got block length %zu", event.blockdata.Length);

            if (event.blockdata.IsEof)
            {
                err = mTransfer.PrepareBlockAck();
                VerifyOrReturn(err == CHIP_NO_ERROR, ChipLogError(BDX, "PrepareBlockAck failed: %s", chip::ErrorStr(err)));
                mIsTransferComplete = true;
            }
            else
            {
                err = mTransfer.PrepareBlockQuery();
                VerifyOrReturn(err == CHIP_NO_ERROR, ChipLogError(BDX, "PrepareBlockQuery failed: %s", chip::ErrorStr(err)));
            }
            break;
        }
        case TransferSession::OutputEventType::kStatusReceived:
            ChipLogError(BDX, "Got StatusReport %u", static_cast<uint32_t>(event.statusData.statusCode));
            mTransfer.Reset();
            break;
        case TransferSession::OutputEventType::kInternalError:
            ChipLogError(BDX, "InternalError");
            mTransfer.Reset();
            break;
        case TransferSession::OutputEventType::kTransferTimeout:
            ChipLogError(BDX, "Transfer timed out");
            mTransfer.Reset();
            break;
        case TransferSession::OutputEventType::kInitReceived:
        case TransferSession::OutputEventType::kAckReceived:
        case TransferSession::OutputEventType::kQueryReceived:
        case TransferSession::OutputEventType::kAckEOFReceived:
        default:
            ChipLogError(BDX, "Unexpected event type");
        }
    }

    bool mIsTransferComplete = false;
};

class StorageDelegate : public PersistentStorageDelegate
{
    CHIP_ERROR SyncGetKeyValue(const char * key, void * buffer, uint16_t & size) override
    {
        ChipLogProgress(AppServer, "Retrieved value from server storage.");
        return DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(key, buffer, size);
    }

    CHIP_ERROR SyncSetKeyValue(const char * key, const void * value, uint16_t size) override
    {
        ChipLogProgress(AppServer, "Stored value in server storage");
        return DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(key, value, size);
    }

    CHIP_ERROR SyncDeleteKeyValue(const char * key) override
    {
        ChipLogProgress(AppServer, "Delete value in server storage");
        return DeviceLayer::PersistedStorage::KeyValueStoreMgr().Delete(key);
    }
} sStorage;

BdxDownloader bdxDownloader;

void OnQueryImageResponse(void * context, uint8_t status, uint32_t delayedActionTime, uint8_t * imageURI, uint32_t softwareVersion,
                          chip::ByteSpan updateToken, bool userConsentNeeded, chip::ByteSpan metadataForRequestor)
{
    ChipLogDetail(SoftwareUpdate, "OnQueryImageResponse");

    TransferSession::TransferInitData initOptions;
    initOptions.TransferCtlFlags = chip::bdx::TransferControlFlags::kReceiverDrive;
    initOptions.MaxBlockSize     = 1024;
    initOptions.FileDesLength    = static_cast<uint16_t>(strlen("test.txt"));
    initOptions.FileDesignator   = reinterpret_cast<const uint8_t *>("test.txt");

    Device * device = nullptr;
    sController.GetDevice(kOtaProviderDeviceId, &device);
    if (device == nullptr)
    {
        ChipLogError(BDX, "Failed to get device");
        return;
    }

    Messaging::ExchangeContext * exchangeCtx =
        device->GetExchangeManager()->NewContext(device->GetSecureSession().Value(), &bdxDownloader);
    if (exchangeCtx == nullptr)
    {
        ChipLogError(BDX, "unable to allocate ec");
        return;
    }

    bdxDownloader.SetInitialExchange(exchangeCtx);
    // This will kick of a timer which will regularly check for updates to the bdx::TransferSession state machine.
    bdxDownloader.InitiateTransfer(&chip::DeviceLayer::SystemLayer(), chip::bdx::TransferRole::kReceiver, initOptions, 20000);
}

void OnQueryFailure(void * context, uint8_t status)
{
    ChipLogDetail(SoftwareUpdate, "QueryImage failure response %" PRIu8, status);
}

chip::Callback::Callback<OtaSoftwareUpdateProviderClusterQueryImageResponseCallback>
    mQueryImageResponseCallback(OnQueryImageResponse, nullptr);
chip::Callback::Callback<DefaultFailureCallback> mOnQueryFailureCallback(OnQueryFailure, nullptr);
void OnConnection(void * context, Device * device)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    chip::Controller::OtaSoftwareUpdateProviderCluster cluster;
    constexpr EndpointId kOtaProviderEndpoint = 0;

    chip::Callback::Cancelable * successCallback = mQueryImageResponseCallback.Cancel();
    chip::Callback::Cancelable * failureCallback = mOnQueryFailureCallback.Cancel();

    // These QueryImage params have been chosen arbitrarily
    constexpr VendorId kExampleVendorId      = VendorId::Common;
    constexpr uint16_t kExampleProductId     = 77;
    constexpr uint16_t kExampleImageType     = 0;
    constexpr uint16_t kExampleHWVersion     = 3;
    constexpr uint16_t kExampleCurentVersion = 0;
    constexpr uint8_t kExampleProtocolsSupported =
        EMBER_ZCL_OTA_DOWNLOAD_PROTOCOL_BDX_SYNCHRONOUS; // TODO: support this as a list once ember adds list support
    const uint8_t locationBuf[] = { 'U', 'S' };
    ByteSpan exampleLocation(locationBuf);
    constexpr bool kExampleClientCanConsent = false;
    ByteSpan metadata;

    err = cluster.Associate(device, kOtaProviderEndpoint);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Associate() failed: %s", chip::ErrorStr(err));
        return;
    }
    err = cluster.QueryImage(successCallback, failureCallback, kExampleVendorId, kExampleProductId, kExampleImageType,
                             kExampleHWVersion, kExampleCurentVersion, kExampleProtocolsSupported, exampleLocation,
                             kExampleClientCanConsent, metadata);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "QueryImage() failed: %s", chip::ErrorStr(err));
    }
}

void OnConnectFail(void * context, chip::NodeId deviceId, CHIP_ERROR error)
{
    ChipLogError(SoftwareUpdate, "failed to connect to 0x%" PRIX64 ": %s", deviceId, chip::ErrorStr(error));
}

chip::Callback::Callback<OnDeviceConnected> mConnectionCallback(OnConnection, nullptr);
chip::Callback::Callback<OnDeviceConnectionFailure> mConnectFailCallback(OnConnectFail, nullptr);

class AddressUpdateDelegate : public DeviceAddressUpdateDelegate
{
public:
    void OnAddressUpdateComplete(NodeId nodeId, CHIP_ERROR error)
    {
        streamer_printf(streamer_get(), "Address updated: %u \n", error.AsInteger());

        error = sController.GetConnectedDevice(kOtaProviderDeviceId, &mConnectionCallback, &mConnectFailCallback);

        if (error != CHIP_NO_ERROR)
        {
            streamer_printf(streamer_get(), "Failed to connected to the device: %u \n", error.AsInteger());
        }
    }

} sAddressUpdateDelegate;

CHIP_ERROR Do(int argc, char ** argv)
{
    streamer_printf(streamer_get(), "Performing OTA... \n");

    Controller::ControllerInitParams params;
    params.storageDelegate = &sStorage;
    params.fabricTable     = &Server::GetInstance().GetFabricTable();
    params.sessionManager  = &Server::GetInstance().GetSecureSessionManager();
    params.exchangeManager = &Server::GetInstance().GetExchangManager();

    CHIP_ERROR error = sController.Init(params);
    if (error != CHIP_NO_ERROR && error != CHIP_ERROR_INCORRECT_STATE)
    {
        streamer_printf(streamer_get(), "Controller init failed: %s \n", error.AsString());
        return error;
    }

    Device * device;
    error = sController.GetDevice(kOtaProviderDeviceId, &device);
    if (error != CHIP_NO_ERROR)
    {
        streamer_printf(streamer_get(), "Controller GetDevice failed: %s \n", error.AsString());
        return error;
    }

    sController.RegisterDeviceAddressUpdateDelegate(&sAddressUpdateDelegate);
    error = sController.UpdateDevice(kOtaProviderDeviceId);

    return CHIP_NO_ERROR;
}

CHIP_ERROR OtaHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return CHIP_NO_ERROR;
    }
    return sSubShell.ExecCommand(argc, argv);
}
} // namespace

void RegisterOtaCommands()
{
    static const shell_command_t sSubCommands[] = {
        { &Do, "do", "Perform OTA" },
    };

    static const shell_command_t sOtaCommand = { &OtaHandler, "ota", "OTA commands" };

    // Register `device` subcommands with the local shell dispatcher.
    sSubShell.RegisterCommands(sSubCommands, ArraySize(sSubCommands));

    // Register the root `device` command with the top-level shell.
    Engine::Root().RegisterCommands(&sOtaCommand, 1);
}

} // namespace Shell
} // namespace chip
