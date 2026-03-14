#!/bin/bash

#platform init script for B6510-48VS8CQ

install_python_api_package() {
    device="/usr/share/sonic/device"
    platform=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)

    rv=$(pip3 install $device/$platform/sonic_platform-1.0-py3-none-any.whl)
}

remove_python_api_package() {
    rv=$(pip show sonic-platform > /dev/null 2>/dev/null)
    if [ $? -eq 0 ]; then
        rv=$(pip3 uninstall -y sonic-platform > /dev/null 2>/dev/null)
    fi
}

if [[ "$1" == "init" ]]; then
    echo "b6510 init"
    install_python_api_package

elif [[ "$1" == "deinit" ]]; then
    remove_python_api_package
else
     echo "B6510-48VS8CQ_PLATFORM : Invalid option !"
fi
