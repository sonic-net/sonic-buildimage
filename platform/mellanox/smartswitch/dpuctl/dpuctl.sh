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

# Function to get PCI addresses dynamically from platform.json
get_dpu_pci_addresses() {
    local dpu_list
    local -A pci_to_dpu_map
    
    # Get list of all DPUs from platform.json
    dpu_list=$(dpumap.sh listdpus 2>/dev/null)
    if [[ $? -ne 0 ]]; then
        echo "Error: Failed to get DPU list from platform.json" >&2
        return 1
    fi
    
    # Get PCI address for each DPU and build the mapping
    while read -r dpu; do
        if [[ -n "$dpu" ]]; then
            rshim_bus_pci_addr=$(dpumap.sh dpu2rshim_bus_info "$dpu" 2>/dev/null)
            if [[ $? -eq 0 && "$rshim_bus_pci_addr" != "null" ]]; then
                pci_to_dpu_map["$rshim_bus_pci_addr"]="$dpu"
            fi
            # If DPU has older image, the bus_info pcie value should be considered 
            pci_addr=$(dpumap.sh dpu2pcie "$dpu" 2>/dev/null)
            if [[ $? -eq 0 && "$pci_addr" != "null" ]]; then
                pci_to_dpu_map["$pci_addr"]="$dpu"
            fi
        fi
    done <<< "$dpu_list"
    
    # Return the mapping as a string that can be eval'd
    declare -p pci_to_dpu_map
}

if [[ -f /etc/mlnx/dpu.conf ]]; then
    . /etc/mlnx/dpu.conf
fi

do_start() {
    if [[ $DARK_MODE == "true" ]]; then
        # By default all the DPUs are on. Power off the DPUs when is dark mode is required.

        # Get PCI addresses dynamically
        eval $(get_dpu_pci_addresses)
        if [[ ${#pci_to_dpu_map[@]} -eq 0 ]]; then
            echo "Error: No PCI addresses found from platform.json" >&2
            exit 1
        fi
        
        # Get all PCI addresses that match bf3_pci_id and extract their PCI IDs
        pci_ids=$(lspci -Dn | grep "$bf3_pci_id" | awk '{print $1}')
        
        # For each PCI ID, find the corresponding DPU from the mapping
        while read -r pci_id; do
            if [[ -n "$pci_id" ]]; then
                dpu_name=${pci_to_dpu_map[$pci_id]}
                if [[ -n "$dpu_name" ]]; then
                    dpuctl dpu-power-off "$dpu_name" &
                fi
            fi
        done <<< "$pci_ids"

        # Wait for all dpuctl processes to finish
        wait
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
