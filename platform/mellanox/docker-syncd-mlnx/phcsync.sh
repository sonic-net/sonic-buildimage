#!/bin/bash
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

#
# Script to synchronize ASIC PTP clocks with system realtime clock.
# This script runs continuously, syncing clocks every 60 seconds.
# This script shall not be running if PTP is enabled in the system.
#

PHC_CTL="/usr/sbin/phc_ctl"
SLEEP_INTERVAL=60
WARM_BOOT_SLEEP_INTERVAL=30

is_warm_reboot_in_progress() {
    if ! command -v sonic-db-cli >/dev/null 2>&1; then
        return 1
    fi
    
    local reboot_enabled
    reboot_enabled=$(sonic-db-cli STATE_DB hget "WARM_RESTART_ENABLE_TABLE|system" enable 2>/dev/null)
    if [[ "$reboot_enabled" == "true" ]]; then
        return 0
    fi
    
    return 1
}

# Check if phc_ctl is available
if [ ! -x "$PHC_CTL" ]; then
    echo "Error: phc_ctl not found. Exiting."
    exit 1
fi

while :; do
    #Avoid syncing clocks while ISSU is in progress
    if is_warm_reboot_in_progress; then
        echo "Warm reboot in progress. Skipping clock synchronization."
        sleep $WARM_BOOT_SLEEP_INTERVAL
        continue
    fi
    
    # Refresh the list of PTP devices on each iteration
    PTP_DEVICES=$(ls /dev/ptp[0-9]* 2>/dev/null)
    
    if [ -z "$PTP_DEVICES" ]; then
        sleep $SLEEP_INTERVAL
        continue
    fi
    
    for dev in $PTP_DEVICES; do
        # Extract device number from /dev/ptpXX
        dev_num=${dev##*/ptp}

        clock_name_file="/sys/class/ptp/ptp${dev_num}/clock_name"
        if [[ ! -f "$clock_name_file" ]]; then
            echo "Error: clock_name file not found for $dev ($clock_name_file), skipping" >&2
            continue
        fi
        
        clock_name=$(cat "$clock_name_file" 2>/dev/null)
        CLOCK_NAME_EXIT_CODE=$?
        if [[ $CLOCK_NAME_EXIT_CODE -ne 0 ]] || [[ -z "$clock_name" ]]; then
            echo "Error: Failed to read clock_name from $clock_name_file for $dev, skipping" >&2
            continue
        fi
        
        if [[ "$clock_name" != "mlx5_ptp" ]]; then
            # set CLOCK_REALTIME
            "$PHC_CTL" "$dev" set 2>/dev/null
            PHC_CTL_EXIT_CODE=$?
            if [[ $PHC_CTL_EXIT_CODE -ne 0 ]]; then
                echo "Error: Failed to sync clock for $dev (phc_ctl exit code: $PHC_CTL_EXIT_CODE)" >&2
            fi
        fi
    done
    sleep $SLEEP_INTERVAL
done
