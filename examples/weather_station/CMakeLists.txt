#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE release)
endif()

set(CONF_FILE "prj_${CMAKE_BUILD_TYPE}.conf")

get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../ REALPATH)
get_filename_component(NLIO_ROOT ${CHIP_ROOT}/third_party/nlio/repo REALPATH)
get_filename_component(NRFCONNECT_COMMON ${CHIP_ROOT}/examples/platform/nrfconnect REALPATH)
include(${CHIP_ROOT}/config/nrfconnect/app/check-nrfconnect-version.cmake)
list(APPEND ZEPHYR_EXTRA_MODULES ${CHIP_ROOT}/config/nrfconnect/chip-module)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-weather-station)

include(${CHIP_ROOT}/config/nrfconnect/app/enable-gnu-std.cmake)

# NORDIC SDK APP START
target_include_directories(app PRIVATE src ${NRFCONNECT_COMMON}/util/include ${NLIO_ROOT}/include)

target_sources(app PRIVATE
               src/app_task.cpp
               src/main.cpp
               src/gen/attribute-size.cpp
               src/gen/IMClusterCommandHandler.cpp
               src/gen/callback-stub.cpp
               ${NRFCONNECT_COMMON}/util/LEDWidget.cpp
               ${CHIP_ROOT}/src/app/util/DataModelHandler.cpp
               ${CHIP_ROOT}/src/app/reporting/reporting-default-configuration.cpp
               ${CHIP_ROOT}/src/app/reporting/reporting.cpp
               ${CHIP_ROOT}/src/app/util/af-event.cpp
               ${CHIP_ROOT}/src/app/util/af-main-common.cpp
               ${CHIP_ROOT}/src/app/util/attribute-list-byte-span.cpp
               ${CHIP_ROOT}/src/app/util/attribute-size-util.cpp
               ${CHIP_ROOT}/src/app/util/attribute-storage.cpp
               ${CHIP_ROOT}/src/app/util/attribute-table.cpp
               ${CHIP_ROOT}/src/app/util/binding-table.cpp
               ${CHIP_ROOT}/src/app/util/chip-message-send.cpp
               ${CHIP_ROOT}/src/app/util/client-api.cpp
               ${CHIP_ROOT}/src/app/util/ember-compatibility-functions.cpp
               ${CHIP_ROOT}/src/app/util/ember-print.cpp
               ${CHIP_ROOT}/src/app/util/message.cpp
               ${CHIP_ROOT}/src/app/util/process-cluster-message.cpp
               ${CHIP_ROOT}/src/app/util/process-global-message.cpp
               ${CHIP_ROOT}/src/app/util/util.cpp
               ${CHIP_ROOT}/src/app/server/EchoHandler.cpp
               ${CHIP_ROOT}/src/app/server/Mdns.cpp
               ${CHIP_ROOT}/src/app/server/OnboardingCodesUtil.cpp
               ${CHIP_ROOT}/src/app/server/RendezvousServer.cpp
               ${CHIP_ROOT}/src/app/server/Server.cpp
               ${CHIP_ROOT}/src/app/server/StorablePeerConnection.cpp
               ${CHIP_ROOT}/src/app/clusters/basic/basic.cpp
               ${CHIP_ROOT}/src/app/clusters/bindings/bindings.cpp
               ${CHIP_ROOT}/src/app/clusters/diagnostic-logs-server/diagnostic-logs-server.cpp
               ${CHIP_ROOT}/src/app/clusters/general-commissioning-server/general-commissioning-server.cpp
               ${CHIP_ROOT}/src/app/clusters/network-commissioning/network-commissioning-ember.cpp
               ${CHIP_ROOT}/src/app/clusters/network-commissioning/network-commissioning.cpp
               ${CHIP_ROOT}/src/app/clusters/operational-credentials-server/operational-credentials-server.cpp)
# NORDIC SDK APP END
