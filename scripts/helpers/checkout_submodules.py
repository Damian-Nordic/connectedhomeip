#!/usr/bin/env python3

#
# Copyright (c) 2021 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse, subprocess, sys

ALL_GROUPS = ['android','efr32','esp32','linux','nrfconnect','qpg6100', 'zap']

# Define a map: submodule -> a list of platforms whose CI checks require the submodule
SUBMODULES = {
    'examples/common/QRCode/repo' : ['efr32', 'esp32'],
    'examples/common/m5stack-tft/repo"]' : ['esp32'],
    'third_party/bluez/repo' : ['linux'],
    'third_party/cirque/repo' : ['linux'],
    'third_party/efr32_sdk/repo' : ['efr32'],
    'third_party/freertos/repo' : ['efr32', 'esp32', 'qpg6100'],
    'third_party/happy/repo' : ['linux'],
    'third_party/mbedtls/repo' : ['android', 'efr32', 'esp32', 'linux', 'qpg6100'],
    'third_party/nanopb/repo' : ALL_GROUPS,
    'third_party/nlassert/repo' : ALL_GROUPS,
    'third_party/nlfaultinjection/repo' : ['esp32', 'linux', 'nrfconnect'],
    'third_party/nlio/repo' : ALL_GROUPS,
    'third_party/nlunit-test/repo' : ['esp32', 'linux', 'nrfconnect'],
    'third_party/openthread/repo' : ['efr32', 'esp32', 'linux', 'qpg6100'],
    'third_party/ot-br-posix/repo' : ['linux'],
    'third_party/ot-commissioner/repo' : ['android'],
    'third_party/pigweed/repo' : ALL_GROUPS,
    'third_party/qpg_sdk/repo' : ['qpg6100'],
    'third_party/zap/repo' : ['zap'],
}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('group', choices=ALL_GROUPS)
    args = parser.parse_args()

    submodules = list(filter(lambda submodule: args.group in SUBMODULES[submodule], SUBMODULES))
    print(submodules)
    subprocess.check_call(['git', 'submodule', 'update', '--init', '--recursive'] + submodules)

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        sys.stderr.write(str(e) + '\n')
