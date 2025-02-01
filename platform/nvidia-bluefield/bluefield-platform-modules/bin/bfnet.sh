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

pci_iface=eth0-midplane

start()
{
    modprobe mlx5_core
    /usr/bin/mst start

    /usr/bin/mlnx-fw-upgrade.sh --dry-run -v
    if [[ $? != "0" ]]; then
        exit 1
    fi
}

stop()
{
    /usr/bin/mst stop
    rmmod mlx5_ib mlx5_core
}

configure_pci_iface()
{
    mgmt_mac=$(cat /sys/devices/platform/MLNXBF17:00/net/*/address)

    # Set PCI interface MAC address to the MAC address of the mgmt interface
    ip link set dev $pci_iface address $mgmt_mac
}

case "$1" in
    start|stop)
        $1
        ;;
    configure-pci-iface)
        configure_pci_iface
        ;;
    *)
        echo "Usage: $0 {start|stop|configure-pci-iface}"
        exit 1
        ;;
esac

