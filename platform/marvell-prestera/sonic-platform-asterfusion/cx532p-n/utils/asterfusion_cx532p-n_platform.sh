#!/bin/bash

# Install cx532p-n python package
DEVICE="/usr/share/sonic/device"
PLATFORM=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)

if [ -e $DEVICE/$PLATFORM/sonic_platform-1.0-py2-none-any.whl ]; then
     pip install $DEVICE/$PLATFORM/sonic_platform-1.0-py2-none-any.whl
fi

