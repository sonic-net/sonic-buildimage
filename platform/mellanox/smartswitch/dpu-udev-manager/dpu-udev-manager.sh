#!/bin/bash
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

do_start() {
    kill_stale_udevd
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
