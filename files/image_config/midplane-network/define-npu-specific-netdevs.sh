#!/bin/bash

set -e

if /bin/bash /usr/local/bin/is-npu-or-dpu.sh -n; then
    cp --remove-destination /usr/share/sonic/templates/bridge-midplane.netdev /etc/systemd/network/
    cp --remove-destination /usr/share/sonic/templates/dummy-midplane.netdev /etc/systemd/network/
fi
