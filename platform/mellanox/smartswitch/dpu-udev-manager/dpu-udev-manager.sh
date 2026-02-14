#!/usr/bin/env bash
#
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

declare -r udev_file="/etc/udev/rules.d/92-midplane-intf.rules"
declare -r platform=$(grep 'onie_platform=' /host/machine.conf | cut -d '=' -f 2)
declare -r platform_json="/usr/share/sonic/device/$platform/platform.json"

declare -r query='.DPUS | to_entries[] | "\(.key) \(.value.bus_info)"'

do_start() {
    jq -r "$query" $platform_json | while read -r dpu bus_info; do
        echo SUBSYSTEM==\"net\", ACTION==\"add\", KERNELS==\"$bus_info\", NAME=\"$dpu\"
    done > $udev_file
}

case "$1" in
    start)
        do_start
        ;;
    *)
        echo "Error: Invalid argument."
        echo "Usage: $0 {start}"
        exit 1
        ;;
esac
