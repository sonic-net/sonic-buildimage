#!/bin/bash

set -e

if /bin/bash /usr/local/bin/is-npu-or-dpu.sh -n; then
    cp /usr/share/sonic/templates/bridge-midplane.netdev /etc/systemd/network/
    cp /usr/share/sonic/templates/dummy-midplane.netdev /etc/systemd/network/
fi
