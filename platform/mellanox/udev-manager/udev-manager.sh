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

# Management interface should be handled first, so it has higher priority
declare -r udev_mgmt_file="/etc/udev/rules.d/01-mgmt-intf.rules"
declare -r udev_midplane_file="/etc/udev/rules.d/92-midplane-intf.rules"

declare -r platform=$(grep 'onie_platform=' /host/machine.conf | cut -d '=' -f 2)
declare -r platform_json="/usr/share/sonic/device/$platform/platform.json"

# Kill any stale initramfs udevd that survived switch_root.
# During boot, the initramfs starts systemd-udevd before the overlayfs root is
# created. If device firmware init is slow (e.g. DPU FW timeout), udevd workers
# block and the initramfs cannot stop udevd before switch_root. The stale
# process ends up with a broken root filesystem view (its root still points to
# the initramfs rootfs, not the overlayfs). Writing udev rules below triggers
# the stale udevd to access file system for the updated rules which crashes
# because dir_fd_is_root() triggers assertion failure for a
# non-chrooted process in systemd.
kill_stale_udevd() {
    local sysd_pid
    sysd_pid=$(systemctl show systemd-udevd -p MainPID --value 2>/dev/null)
    for pid in $(pgrep -f "systemd-udevd --daemon"); do
        if [ "$pid" != "$sysd_pid" ]; then
            logger "dpu-udev-manager: killing stale initramfs udevd PID $pid (systemd udevd is PID $sysd_pid)"
            kill -9 "$pid" 2>/dev/null || true
        fi
    done
}

handle_mgmt_interface() {
    local mgmt_interfaces
    mgmt_interfaces=$(jq -r '(.mgmt_interfaces // {}) | keys[]' "$platform_json" 2>/dev/null) || true

    if [ -z "$mgmt_interfaces" ] || [ "$mgmt_interfaces" = "null" ]; then
        return 0
    fi

    echo > "$udev_mgmt_file"

    local mgmt_interface mgmt_interface_bus_info
    while IFS= read -r mgmt_interface; do
        [ -z "$mgmt_interface" ] && continue
        mgmt_interface_bus_info=$(jq -r --arg name "$mgmt_interface" '.mgmt_interfaces[$name].pci_bus_info // empty' "$platform_json")
        [ -z "$mgmt_interface_bus_info" ] || [ "$mgmt_interface_bus_info" = "null" ] && continue

        echo SUBSYSTEM==\"net\", ACTION==\"add\", KERNELS==\"$mgmt_interface_bus_info\", NAME=\"$mgmt_interface\" >> "$udev_mgmt_file"
    done <<< "$mgmt_interfaces"
}

handle_midplane_interface() {
    local query='(.DPUS // {}) | to_entries[] | "\(.key) \(.value.bus_info)"'
    local dpu_pci_bus_infos
    dpu_pci_bus_infos=$(jq -r "$query" "$platform_json" 2>/dev/null) || true

    if [ -z "$dpu_pci_bus_infos" ]; then
        return 0
    fi

    echo > "$udev_midplane_file"

    local dpu_pci_bus_info dpu bus_info
    while IFS= read -r dpu_pci_bus_info; do
        [ -z "$dpu_pci_bus_info" ] && continue
        dpu=$(echo "$dpu_pci_bus_info" | cut -d ' ' -f 1)
        bus_info=$(echo "$dpu_pci_bus_info" | cut -d ' ' -f 2)
        [ -z "$dpu" ] || [ -z "$bus_info" ] && continue

        echo SUBSYSTEM==\"net\", ACTION==\"add\", KERNELS==\"$bus_info\", NAME=\"$dpu\" >> "$udev_midplane_file"
    done <<< "$dpu_pci_bus_infos"
}

do_start() {
    kill_stale_udevd
    handle_mgmt_interface
    handle_midplane_interface
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
