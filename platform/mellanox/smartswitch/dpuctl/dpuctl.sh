#!/usr/bin/env bash
#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
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

# By default the dark mode is enabled
DARK_MODE=true

bf3_pci_id="15b3:c2d5"
dpu2pcie[0]="08:00.0"
dpu2pcie[1]="07:00.0"
dpu2pcie[2]="01:00.0"
dpu2pcie[3]="02:00.0"

if [[ -f /etc/mlnx/dpu.conf ]]; then
    . /etc/mlnx/dpu.conf
fi

do_start() {
    if [[ $DARK_MODE == "true" ]]; then
        # By default all the DPUs are on. Power off the DPUs when is dark mode is required.

        for dpu_id in ${!dpu2pcie[@]}; do
            pci_id=$(lspci -n | grep "${dpu2pcie[$dpu_id]}" | awk '{print $3}')
            if [[ $pci_id == $bf3_pci_id ]]; then
                dpuctl dpu-power-off dpu${dpu_id} &
            fi
        done

        # Wait for all dpuctl processes to finish
        wait
    else
        # Start RSHIM per each DPU to create interfaces
        systemctl start rshim-manager.service
    fi
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
