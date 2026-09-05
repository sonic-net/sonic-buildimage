#!/bin/bash
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

SXDKERNEL_INIT="/etc/init.d/sxdkernel"

[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment

# If PLATFORM is not set in sonic-environment, try to get it from /host/machine.conf
if [ -z "$PLATFORM" ]; then
    if [ -f /host/machine.conf ]; then
        PLATFORM=$(grep "^onie_platform=" /host/machine.conf | cut -d'=' -f2)
    else
        echo "Error: Failed to get platform"
        exit 1
    fi
fi

# Parse the device specific asic conf file, if it exists
ASIC_CONF=/usr/share/sonic/device/$PLATFORM/asic.conf
[ -f $ASIC_CONF ] && . $ASIC_CONF

NUM_ASIC=${NUM_ASIC:-1}
if [ "$NUM_ASIC" -gt 1 ]; then
    PREDEFINED_DEV_PCI_BUS=""
    for i in $(seq 0 $((NUM_ASIC-1))); do
        dev_id_var="DEV_ID_ASIC_$i"
        dev_id="${!dev_id_var}"
        if [ -n "$dev_id" ]; then
            if [ -n "$PREDEFINED_DEV_PCI_BUS" ]; then
                PREDEFINED_DEV_PCI_BUS="${PREDEFINED_DEV_PCI_BUS},$((i+1))@${dev_id}"
            else
                PREDEFINED_DEV_PCI_BUS="$((i+1))@${dev_id}"
            fi
        fi
    done
    export PREDEFINED_DEV_PCI_BUS
fi


# Check if the sxdkernel init script exists
if [ ! -x "$SXDKERNEL_INIT" ]; then
    echo "Error: sxdkernel init script not found at $SXDKERNEL_INIT"
    exit 1
fi

function getBootType()
{
    case "$(cat /proc/cmdline)" in
    *SONIC_BOOT_TYPE=warm*)
        TYPE='warm'
        ;;
    *SONIC_BOOT_TYPE=fastfast*)
        TYPE='fastfast'
        ;;
    *SONIC_BOOT_TYPE=express*)
        TYPE='express'
        ;;
    *SONIC_BOOT_TYPE=fast*|*fast-reboot*)
        TYPE='fast'
        ;;
    *)
        TYPE='cold'
    esac
    echo "${TYPE}"
}

start() {
    BOOT_TYPE=`getBootType`
    if [[ x"$BOOT_TYPE" == x"warm" || x"$BOOT_TYPE" == x"fastfast" || x"$BOOT_TYPE" == x"fast" ]]; then
        export FAST_BOOT=1
    fi

    echo "Starting SX kernel driver PREDEFINED_DEV_PCI_BUS=${PREDEFINED_DEV_PCI_BUS} BOOT_TYPE: $BOOT_TYPE ..."
    $SXDKERNEL_INIT start
}

stop() {
    echo "Stopping SX kernel driver..."
    $SXDKERNEL_INIT stop
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        echo "  start - Start the SX kernel driver"
        echo "  stop  - Stop the SX kernel driver"
        exit 1
        ;;
esac

exit 0
