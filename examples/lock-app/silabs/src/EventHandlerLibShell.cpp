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

#include "EventHandlerLibShell.h"
#include "AppTask.h"
#include "lib/shell/Engine.h"

#include "app/server/Server.h"
#include "platform/CHIPDeviceLayer.h"
#include <lib/support/CodeUtils.h>

constexpr uint8_t lockEndpoint = 1;

using namespace chip;
using namespace chip::app;
using namespace Clusters::DoorLock;
using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

Engine sShellDoorlockSubCommands;
Engine sShellDoorlockEventSubCommands;

/********************************************************
 * Doorlock shell functions
 *********************************************************/

CHIP_ERROR DoorlockCommandHandler(int argc, char ** argv)
{
    return sShellDoorlockSubCommands.ExecCommand(argc, argv);
}

/********************************************************
 * Event shell functions
 *********************************************************/

CHIP_ERROR EventDoorlockCommandHandler(int argc, char ** argv)
{
    return sShellDoorlockEventSubCommands.ExecCommand(argc, argv);
}

/********************************************************
 * Alarm shell functions
 *********************************************************/

CHIP_ERROR AlarmEventHandler(int argc, char ** argv)
{
    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }

    AlarmEventData * data = Platform::New<AlarmEventData>();
    data->eventId         = Events::DoorLockAlarm::Id;
    data->alarmCode       = static_cast<AlarmCodeEnum>(atoi(argv[0]));

    DeviceLayer::PlatformMgr().ScheduleWork(EventWorkerFunction, reinterpret_cast<intptr_t>(data));

    return CHIP_NO_ERROR;
}

/********************************************************
 * Door state shell functions
 *********************************************************/

CHIP_ERROR DoorStateEventHandler(int argc, char ** argv)
{
    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }

    DoorStateEventData * data = Platform::New<DoorStateEventData>();
    data->eventId             = Events::DoorStateChange::Id;
    data->doorState           = static_cast<DoorStateEnum>(atoi(argv[0]));

    DeviceLayer::PlatformMgr().ScheduleWork(EventWorkerFunction, reinterpret_cast<intptr_t>(data));

    return CHIP_NO_ERROR;
}

/**
 * @brief configures lock matter shell
 *
 */

CHIP_ERROR RegisterLockEvents()
{
    static const shell_command_t sDoorlockSubCommands[] = { { &EventDoorlockCommandHandler, "event",
                                                              " Usage: doorlock event <subcommand>" } };

    static const shell_command_t sDoorlockEventSubCommands[] = {
        { &AlarmEventHandler, "lock-alarm", "Sends lock alarm event to lock app" },
        { &DoorStateEventHandler, "door-state-change", "Sends door state change event to lock app" }
    };

    static const shell_command_t sDoorLockCommand = { &DoorlockCommandHandler, "doorlock",
                                                      "doorlock commands. Usage: doorlock <subcommand>" };

    sShellDoorlockEventSubCommands.RegisterCommands(sDoorlockEventSubCommands, ArraySize(sDoorlockEventSubCommands));
    sShellDoorlockSubCommands.RegisterCommands(sDoorlockSubCommands, ArraySize(sDoorlockSubCommands));

    Engine::Root().RegisterCommands(&sDoorLockCommand, 1);

    return CHIP_NO_ERROR;
}

void EventWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "EventWorkerFunction - Invalid work data"));

    EventData * data = reinterpret_cast<EventData *>(context);

    switch (data->eventId)
    {
    case Events::DoorLockAlarm::Id: {
        AlarmEventData * alarmData = reinterpret_cast<AlarmEventData *>(context);
        DoorLockServer::Instance().SendLockAlarmEvent(lockEndpoint, alarmData->alarmCode);
        break;
    }

    case Events::DoorStateChange::Id: {
        DoorStateEventData * doorStateData = reinterpret_cast<DoorStateEventData *>(context);
        DoorLockServer::Instance().SetDoorState(lockEndpoint, doorStateData->doorState);
        break;
    }

    default: {
        ChipLogError(Zcl, "Invalid Event Id %s, line %d", __func__, __LINE__);
        break;
    }
    }
}
