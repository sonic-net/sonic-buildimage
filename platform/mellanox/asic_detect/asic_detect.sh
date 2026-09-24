#!/usr/bin/env bash
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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

SUCCESS_CODE=0
RC=-1
RETURN_PCI_ID=false
RETURN_MST_DEV=false

# Parse command line arguments
#   -p : print the ASIC PCI id (e.g. 03:00.0)
#   -m : print the MST cr-space device node (e.g. /dev/mst/mt52100_pci_cr0)
while getopts "pm" opt; do
    case $opt in
        p)
            RETURN_PCI_ID=true
            ;;
        m)
            RETURN_MST_DEV=true
            ;;
        *)
            ;;
    esac
done

# Resolve the MST cr-space device node for a given PCI id. The *_pci_cr0 node uses
# fast memory-mapped ("PCI direct access") register access; the bare PCI id or the
# *_pciconf0 node use slow PCI config cycles, which make a CPLD burn ~10x slower.
# Requires `mst start` to have created the device nodes. Prints the node, returns
# 0 on success and non-zero if no matching cr-space node is found.
get_mst_device() {
    local pci_id="$1"
    local node=""

    # Match the *_pci_cr0 node whose PCI address matches ours (mst status -v prints
    # the node on one line and "domain:bus:dev.fn=<pci>" on the following line).
    if [[ -n "$pci_id" ]]; then
        node=$(mst status -v 2>/dev/null | awk -v pci="$pci_id" '
            /\/dev\/mst\// { dev=$1 }
            index($0, pci) { if (dev ~ /_pci_cr0$/) { print dev; exit } }')
    fi

    # Fallback: a single cr-space node is present.
    if [[ -z "$node" ]]; then
        node=$(ls /dev/mst/*_pci_cr0 2>/dev/null)
        if [[ $(echo "$node" | grep -c .) -ne 1 ]]; then
            node=""
        fi
    fi

    [[ -n "$node" ]] || return 1
    echo "$node"
    return $SUCCESS_CODE
}

if [[ -f "/usr/bin/asic_detect/asic_type" ]] && [[ -f "/usr/bin/asic_detect/asic_pci_id" ]]; then
    if [[ "$RETURN_MST_DEV" == true ]]; then
        get_mst_device "$(cat /usr/bin/asic_detect/asic_pci_id)"
    elif [[ "$RETURN_PCI_ID" == true ]]; then
        cat /usr/bin/asic_detect/asic_pci_id
    else
        cat /usr/bin/asic_detect/asic_type
    fi
    exit $?
fi

# make sure that DEVICE_DICT keys are the same as values in DEVICE_ORDER
declare -A DEVICE_DICT=(
    ["cb84"]="spc1"
    ["cf6c"]="spc2"
    ["cf70"]="spc3"
    ["cf80"]="spc4"
    ["cf82"]="spc5"
    ["cf84"]="spc6"
    ["a2dc"]="bf3"
)
TYPE_UNKNOWN="unknown"
VENDOR_ID="15b3"
DEVICE_TYPE=$TYPE_UNKNOWN
DEVICE_PCI_ID=""

# bf3 should be the last device in the list
DEVICE_ORDER=("cb84" "cf6c" "cf70" "cf80" "cf82" "cf84" "a2dc")

lspci_output=$(lspci -n 2>/dev/null)
if [[ -n "$lspci_output" ]]; then
    for key in "${DEVICE_ORDER[@]}"; do
        if echo "$lspci_output" | grep "$VENDOR_ID:$key" &>/dev/null; then
            DEVICE_TYPE="${DEVICE_DICT[$key]}"
            DEVICE_PCI_ID=$(echo "$lspci_output" | grep "$VENDOR_ID:$key" | awk '{print $1}')
            RC=$SUCCESS_CODE
            break
        fi
    done
    if [[ -n "$DEVICE_TYPE" ]]; then
        if [[ "$DEVICE_TYPE" != "$TYPE_UNKNOWN" ]] && [[ -n "$DEVICE_PCI_ID" ]]; then
            echo "$DEVICE_TYPE" > /usr/bin/asic_detect/asic_type
            echo "$DEVICE_PCI_ID" > /usr/bin/asic_detect/asic_pci_id
        fi
        if [[ "$RETURN_MST_DEV" == true ]]; then
            get_mst_device "$DEVICE_PCI_ID"
            RC=$?
        elif [[ "$RETURN_PCI_ID" == true ]]; then
            echo "$DEVICE_PCI_ID"
        else
            echo "$DEVICE_TYPE"
        fi
    fi
    exit $RC
fi
