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

cp_iface=Ethernet0
pidfile=/run/dhcl-internal.$cp_iface.pid
leasefile=/var/lib/dhcp/dhcl-internal.$cp_iface.leases

stop_cp_dhclient()
{
    if [[ -f $pidfile ]]; then
        kill $(cat $pidfile)
	rm -f $pidfile
    fi
    rm -f $leasefile
}

start_cp_dhclient()
{
    stop_cp_dhclient

    /sbin/dhclient -pf $pidfile -lf $leasefile $cp_iface -nw
}

start()
{
    modprobe mlx5_core
    /usr/bin/mst start

    hwsku=$(sonic-cfggen -d -v 'DEVICE_METADATA["localhost"]["hwsku"]')
    if [[ $hwsku == *"-C1" ]]; then
        start_cp_dhclient
    fi
}

stop()
{
    stop_cp_dhclient

    /usr/bin/mst stop
    rmmod mlx5_ib mlx5_core
}

case "$1" in
    start|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

