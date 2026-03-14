#!/bin/bash

# Install Netberg Aurora python package
DEVICE="/usr/share/sonic/device"
PLATFORM=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)

if [ -e $DEVICE/$PLATFORM/sonic_platform-1.0-py3-none-any.whl ]; then
     pip install $DEVICE/$PLATFORM/sonic_platform-1.0-py3-none-any.whl
fi

depmod -a

systemctl enable sonic-platform-netberg-aurora-610
systemctl start sonic-platform-netberg-aurora-610
