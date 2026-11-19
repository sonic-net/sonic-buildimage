#!/bin/bash

# Get BMC mode
BMC_PRESENCE_SYS_PATH="/sys/bus/platform/devices/sys_cpld/bmc_present_l"
BMC_PRESENCE=`cat ${BMC_PRESENCE_SYS_PATH}`
PLATFORM=`sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf`
HW_SKU=`awk '{print $1}' /usr/share/sonic/device/$PLATFORM/default_sku`

if [ "${BMC_PRESENCE}" = "0" ]; then
    :
else

    # Expose pca953x device
    echo 0xa35 0xe5 > /sys/devices/platform/sys_cpld/setreg

    echo ucd90320 0x33 > /sys/bus/i2c/devices/i2c-14/new_device
    echo ucd90320 0x71 > /sys/bus/i2c/devices/i2c-14/new_device
    echo tps25990 0x4c > /sys/bus/i2c/devices/i2c-14/new_device

    echo ucd90120 0x36 > /sys/bus/i2c/devices/i2c-20/new_device

    echo mp2891 0x23 > /sys/bus/i2c/devices/i2c-108/new_device
    echo mp2971 0x78 > /sys/bus/i2c/devices/i2c-108/new_device
    echo mp2971 0x73 > /sys/bus/i2c/devices/i2c-108/new_device
    echo mp2971 0x7b > /sys/bus/i2c/devices/i2c-108/new_device

    echo mp2971 0x7B > /sys/bus/i2c/devices/i2c-110/new_device
    echo mp2891 0x21 > /sys/bus/i2c/devices/i2c-110/new_device
    echo tps546b24 0x22 > /sys/bus/i2c/devices/i2c-110/new_device
    echo tps546b24 0x24 > /sys/bus/i2c/devices/i2c-110/new_device

    echo mp2971 0x73 > /sys/bus/i2c/devices/i2c-113/new_device
    echo mp2971 0x76 > /sys/bus/i2c/devices/i2c-113/new_device
    echo mp2971 0x7B > /sys/bus/i2c/devices/i2c-113/new_device
    echo mp2971 0x7D > /sys/bus/i2c/devices/i2c-113/new_device
    echo mp2971 0x78 > /sys/bus/i2c/devices/i2c-113/new_device
    echo mp2891 0x28 > /sys/bus/i2c/devices/i2c-113/new_device

    echo mp2975 0x72 > /sys/bus/i2c/devices/i2c-114/new_device
    echo mp2975 0x73 > /sys/bus/i2c/devices/i2c-114/new_device
    echo mp9941 0x74 > /sys/bus/i2c/devices/i2c-114/new_device
    echo mp9941 0x75 > /sys/bus/i2c/devices/i2c-114/new_device
    echo mp9941 0x7A > /sys/bus/i2c/devices/i2c-114/new_device

    echo icp201xx 0x63 > /sys/bus/i2c/devices/i2c-123/new_device
    echo icp201xx 0x63 > /sys/bus/i2c/devices/i2c-124/new_device

    echo tla2024 0x48 > /sys/bus/i2c/devices/i2c-126/new_device

    echo tps25990 0x41 > /sys/bus/i2c/devices/i2c-136/new_device
    echo tps25990 0x42 > /sys/bus/i2c/devices/i2c-136/new_device
    echo tps25990 0x47 > /sys/bus/i2c/devices/i2c-136/new_device
    echo tps25990 0x4B > /sys/bus/i2c/devices/i2c-136/new_device
    echo tps25990 0x4C > /sys/bus/i2c/devices/i2c-136/new_device
    
fi

sync
