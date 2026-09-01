#!/bin/bash
# bes2348t post device creation script
echo "Disable SFP28 PORT49 TX disable"
echo 0 > /sys/class/gpio/gpio480/value

echo "Disable SFP28 PORT50 TX disable"
echo 0 > /sys/class/gpio/gpio484/value

echo "Disable SFP28 PORT51 TX disable"
echo 0 > /sys/class/gpio/gpio488/value

echo "Disable SFP28 PORT52 TX disable"
echo 0 > /sys/class/gpio/gpio492/value

set_pca_mux_idle_disconnect()
{
    echo -2 | tee /sys/bus/i2c/drivers/pca954x/*-00*/idle_state >& /dev/null
    if [ $? -ne 0 ]; then
        echo Fail to set pca954x mux idle disconnect
        exit 2
    fi
}

set_pca_mux_idle_disconnect
