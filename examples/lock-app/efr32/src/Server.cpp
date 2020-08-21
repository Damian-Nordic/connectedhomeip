/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#include "AppConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <sys/param.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <app/chip-zcl-zpro-codec.h>
#include <inet/IPAddress.h>
#include <inet/InetError.h>
#include <inet/InetLayer.h>
#include <platform/CHIPDeviceLayer.h>
#include <support/CodeUtils.h>
#include <support/ErrorStr.h>
#include <system/SystemPacketBuffer.h>
#include <transport/SecureSessionMgr.h>
#include <transport/UDP.h>

#include "Server.h"

#include "attribute-storage.h"
#include "gen/znet-bookkeeping.h"
#include "util.h"

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::Transport;

// Transport Callbacks
namespace {

#ifndef EXAMPLE_SERVER_NODEID
// Use this hardcoded NODEID for the lock app example if none was provided
#define EXAMPLE_SERVER_NODEID 0x3546526e
#endif // EXAMPLE_SERVER_NODEID

class ServerCallback : public SecureSessionMgrCallback
{
public:
    virtual void OnMessageReceived(const MessageHeader & header, Transport::PeerConnectionState * state,
                                   System::PacketBuffer * buffer, SecureSessionMgrBase * mgr)
    {
        const size_t data_len = buffer->DataLength();
        char src_addr[PeerAddress::kMaxToStringSize];

        // as soon as a client connects, assume it is connected
        VerifyOrExit(buffer != NULL, EFR32_LOG("Received data but couldn't process it..."));
        VerifyOrExit(header.GetSourceNodeId().HasValue(), EFR32_LOG("Unknown source for received message"));

        VerifyOrExit(state->GetPeerNodeId() != kUndefinedNodeId, EFR32_LOG("Unknown source for received message"));

        state->GetPeerAddress().ToString(src_addr, sizeof(src_addr));

        EFR32_LOG("Packet received from %s: %zu bytes", src_addr, static_cast<size_t>(data_len));

        HandleDataModelMessage(header, buffer, mgr);
        buffer = NULL;

    exit:
        // SendTo calls Free on the buffer without an AddRef, if SendTo was not called, free the buffer.
        if (buffer != NULL)
        {
            System::PacketBuffer::Free(buffer);
        }
    }

    virtual void OnNewConnection(Transport::PeerConnectionState * state, SecureSessionMgrBase * mgr)
    {
        EFR32_LOG("Received a new connection.");
    }

private:
    /**
     * Handle a message that should be processed via our data model processing
     * codepath.
     *
     * @param [in] buffer The buffer holding the message.  This function guarantees
     *                    that it will free the buffer before returning.
     */
    void HandleDataModelMessage(const MessageHeader & header, System::PacketBuffer * buffer, SecureSessionMgrBase * mgr)
    {
        EmberApsFrame frame;
        bool ret = extractApsFrame(buffer->Start(), buffer->DataLength(), &frame);
        if (ret)
        {
            EFR32_LOG("APS framprocessing success!");
        }
        else
        {
            EFR32_LOG("APS processing failure!");
            System::PacketBuffer::Free(buffer);
            return;
        }
        ChipResponseDestination responseDest(header.GetSourceNodeId().Value(), mgr);
        uint8_t * message;
        uint16_t messageLen = extractMessage(buffer->Start(), buffer->DataLength(), &message);
        ret                 = emberAfProcessMessage(&frame,
                                    0, // type
                                    message, messageLen,
                                    &responseDest, // source identifier
                                    NULL);

        System::PacketBuffer::Free(buffer);

        if (ret)
        {
            EFR32_LOG("Data model processing success!");
        }
        else
        {
            EFR32_LOG("Data model processing failure!");
        }
    }
};

static ServerCallback gCallbacks;
static SecurePairingUsingTestSecret gTestPairing;

} // namespace

void InitDataModelHandler()
{
    emberAfEndpointConfigure();
    emAfInit();
}

// The echo server assumes the platform's networking has been setup already
void StartServer(DemoSessionManager * sessions)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    Optional<Transport::PeerAddress> peer(Transport::Type::kUndefined);

    err = sessions->Init(EXAMPLE_SERVER_NODEID, &DeviceLayer::SystemLayer,
                         UdpListenParameters(&DeviceLayer::InetLayer).SetAddressType(kIPAddressType_IPv6));
    SuccessOrExit(err);

    err = sessions->NewPairing(Optional<NodeId>::Value(kUndefinedNodeId), peer, 0, 0, &gTestPairing);
    SuccessOrExit(err);

    sessions->SetDelegate(&gCallbacks);

exit:
    if (err != CHIP_NO_ERROR)
    {
        EFR32_LOG("ERROR setting up transport: %s", ErrorStr(err));
    }
    else
    {
        EFR32_LOG("Lock Server Listening...");
    }
}