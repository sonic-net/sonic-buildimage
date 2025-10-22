#!/bin/bash
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

midplane_iface=eth0-midplane

start()
{
    /usr/bin/mst start

    /usr/bin/mlnx-fw-upgrade.sh --dry-run -v
    if [[ $? != "0" ]]; then
        exit 1
    fi
}

stop()
{
    /usr/bin/ip addr flush dev Ethernet0
    return 0
}

configure_midplane_iface()
{
    # Get the MAC address of the mgmt interface
    mgmt_mac=$(cat /sys/devices/platform/MLNXBF17:00/net/*/address)

    # Create systemd-networkd configuration directory if it doesn't exist
    ip link set dev $midplane_iface address $mgmt_mac
}

case "$1" in
    start|stop)
        $1
        ;;
    configure-midplane-iface)
        configure_midplane_iface
        ;;
    *)
        echo "Usage: $0 {start|stop|configure-midplane-iface}"
        exit 1
        ;;
esac
