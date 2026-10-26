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
PTP_DEVICE_DIR="/dev"
PTP_STATE_DIR="/run/phcsync"
SLEEP_INTERVAL=60
WARM_BOOT_SLEEP_INTERVAL=30

is_owned_ptp_device() {
    local dev="$1" identity
    [[ -c "$dev" && ! -L "$dev" ]] || return 1
    identity=$(cat "$PTP_STATE_DIR/${dev##*/}" 2>/dev/null) || return 1
    [[ -n "$identity" && "$identity" == "$(stat -c '%d:%i:%t:%T' "$dev" 2>/dev/null)" ]]
}

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
    
    # The driver can unregister/recreate PHCs while this container is retained.
    # Discover clocks through sysfs instead of Docker --device mappings, which
    # prevent container startup if a previously mapped host node is absent.
    if ! mkdir -p -m 0700 "$PTP_STATE_DIR"; then
        echo "Error: Failed to create $PTP_STATE_DIR" >&2
        sleep $SLEEP_INTERVAL
        continue
    fi

    # Only remove nodes created by this helper, not preexisting or replaced nodes.
    for record in "$PTP_STATE_DIR"/ptp[0-9]*; do
        [[ -f "$record" ]] || continue
        dev="$PTP_DEVICE_DIR/${record##*/}"
        if ! is_owned_ptp_device "$dev"; then
            rm -f -- "$record"
        elif [[ "$(cat "/sys/class/ptp/${dev##*/}/clock_name" 2>/dev/null)" != "sx_ptp" ]]; then
            rm -f -- "$dev" && rm -f -- "$record"
        fi
    done

    # Refresh the list of PTP devices on each iteration
    PTP_DEVICES=$(ls -d /sys/class/ptp/ptp[0-9]* 2>/dev/null)
    
    if [ -z "$PTP_DEVICES" ]; then
        sleep $SLEEP_INTERVAL
        continue
    fi
    
    for clock in $PTP_DEVICES; do
        # Extract device number from the sysfs PTP path
        dev_num=${clock##*/ptp}
        dev="$PTP_DEVICE_DIR/ptp${dev_num}"

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
        
        if [[ "$clock_name" == "sx_ptp" ]]; then
            device_number=$(cat "$clock/dev" 2>/dev/null) || continue
            [[ "$device_number" =~ ^([0-9]+):([0-9]+)$ ]] || continue
            major=${BASH_REMATCH[1]}
            minor=${BASH_REMATCH[2]}
            expected_device=$(printf '%x:%x' "$major" "$minor")

            if [[ -L "$dev" || ( -e "$dev" && ! -c "$dev" ) ]]; then
                echo "Error: Refusing to replace non-device $dev" >&2
                continue
            fi
            if [[ "$(stat -c '%t:%T' "$dev" 2>/dev/null)" != "$expected_device" ]]; then
                if [[ -e "$dev" ]] && ! is_owned_ptp_device "$dev"; then
                    echo "Error: Refusing to replace unowned device $dev" >&2
                    continue
                fi
                rm -f -- "$dev" || continue
                mknod -m 0600 "$dev" c "$major" "$minor" || continue
                stat -c '%d:%i:%t:%T' "$dev" > "$PTP_STATE_DIR/${dev##*/}" || continue
            fi

            # set CLOCK_REALTIME
            # Keep successful syncs silent to avoid rsyslogd ratelimit memory issue due to PID churn.
            "$PHC_CTL" -q -Q "$dev" set >/dev/null
            PHC_CTL_EXIT_CODE=$?
            if [[ $PHC_CTL_EXIT_CODE -ne 0 ]]; then
                echo "Error: Failed to sync clock for $dev (phc_ctl exit code: $PHC_CTL_EXIT_CODE)" >&2
            fi
        fi
    done
    sleep $SLEEP_INTERVAL
done
