#
#    Copyright (c) 2020 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
cmake_minimum_required(VERSION 3.13.1)

get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/connectedhomeip REALPATH)
get_filename_component(NRFCONNECT_COMMON ${CHIP_ROOT}/examples/platform/nrfconnect REALPATH)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CHIP_ROOT}/config/nrfconnect/)
include(nrfconnect-app)

project(chip-nrf52840-lighting-example)
target_include_directories(app PRIVATE main main/include ${NRFCONNECT_COMMON}/util/include)
target_sources(app PRIVATE
               main/AppTask.cpp
               main/DataModelHandler.cpp
               main/LightingManager.cpp
               main/main.cpp
               main/Server.cpp
               main/gen/call-command-handler.c
               main/gen/callback-stub.c
               main/gen/znet-bookkeeping.c
               ${NRFCONNECT_COMMON}/util/LEDWidget.cpp
               ${CHIP_ROOT}/src/app/util/af-event.cpp
               ${CHIP_ROOT}/src/app/util/attribute-size.c
               ${CHIP_ROOT}/src/app/util/attribute-storage.c
               ${CHIP_ROOT}/src/app/util/attribute-table.c
               ${CHIP_ROOT}/src/app/util/binding-table.cpp
               ${CHIP_ROOT}/src/app/util/chip-response.cpp
               ${CHIP_ROOT}/src/app/util/client-api.c
               ${CHIP_ROOT}/src/app/util/ember-print.cpp
               ${CHIP_ROOT}/src/app/util/message.c
               ${CHIP_ROOT}/src/app/util/process-cluster-message.c
               ${CHIP_ROOT}/src/app/util/process-global-message.c
               ${CHIP_ROOT}/src/app/util/util.c
               ${CHIP_ROOT}/src/app/clusters/on-off-server/on-off.c)