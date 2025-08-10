#!/bin/bash

# Install escc601-32q python package
DEVICE="/usr/share/sonic/device"
str=$(cat /host/machine.conf | grep onie_platform)
PLATFORM=${str#*=}

if [ -e $DEVICE/$PLATFORM/sonic_platform-1.0-py2-none-any.whl ]; then
     pip install $DEVICE/$PLATFORM/sonic_platform-1.0-py2-none-any.whl
fi

if [ -e $DEVICE/$PLATFORM/sonic_platform-1.0-py3-none-any.whl ]; then
     pip3 install $DEVICE/$PLATFORM/sonic_platform-1.0-py3-none-any.whl
fi