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

#include <mdns/Advertiser.h>
#include <platform/CHIPDeviceLayer.h>
#include <support/UnitTestRegistration.h>

#include <nlunit-test.h>
#include <openthread/srp_client.h>

#include <string>

using namespace chip;
using namespace chip::Mdns;
using namespace chip::DeviceLayer;

namespace {

void TestSimpleAdvertise(nlTestSuite * inSuite, void * inContext)
{
    CHIP_ERROR error = ThreadStackMgr().InitThreadStack();
    NL_TEST_ASSERT(inSuite, error == CHIP_NO_ERROR);

    error = ServiceAdvertiser::Instance().Start(nullptr, 0);
    NL_TEST_ASSERT(inSuite, error == CHIP_NO_ERROR);

    error = ServiceAdvertiser::Instance().Advertise(
        OperationalAdvertisingParameters().SetPeerId(PeerId().SetFabricId(1).SetNodeId(2)).SetMRPRetryIntervals(300, 300));
    NL_TEST_ASSERT(inSuite, error == CHIP_NO_ERROR);

    const otSrpClientService * service = otSrpClientGetServices(ThreadStackMgrImpl().OTInstance());
    NL_TEST_ASSERT(inSuite, service != nullptr);
    NL_TEST_ASSERT(inSuite, std::string("0000000000000001-0000000000000002") == service->mInstanceName);
}

const nlTest sTests[] = {
    NL_TEST_DEF("TestSimpleAdvertise", TestSimpleAdvertise), //
    NL_TEST_SENTINEL()                                       //
};

} // namespace

int TestAdvertiser_OpenThread(void)
{
    nlTestSuite theSuite = { "TestAdvertiser_OpenThread", &sTests[0], nullptr, nullptr };
    nlTestRunner(&theSuite, nullptr);
    return nlTestRunnerStats(&theSuite);
}

CHIP_REGISTER_TEST_SUITE(TestAdvertiser_OpenThread);
