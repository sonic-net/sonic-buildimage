#!/bin/bash
platform="$(sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)"
source /usr/share/sonic/device/$platform/actions.sh

I2C_ADAPTER="/sys/bus/i2c/devices/"

save_cmd "10" "hexdump -C /dev/port" "SMF dump"
save_cmd "10" "hexdump -C /dev/nvram" "NVRAM dump"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-2/2-0050/eeprom" "System EEPROM"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-14/14-0050/eeprom" "IOM 1 EEPROM"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-16/16-0050/eeprom" "IOM 2 EEPROM"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-15/15-0050/eeprom" "IOM 3 EEPROM"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-17/17-0050/eeprom" "IOM 4 EEPROM"

save_cmd "10" "pcisysfs.py --get --offset 0x108 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 " "ISMT MCTRL"
save_cmd "10" "pcisysfs.py --get --offset 0x10c --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 " "ISMT MSTS"
save_cmd "10" "pcisysfs.py --get --offset 0x38c --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 " "ISMT DBSTS"
save_cmd "10" "pcisysfs.py --get --offset 0x300 --res /sys/devices/pci0000\:00/0000\:00\:13.0/resource0 " "ISMT SPGT"

IOM_INDEX=66
CURRENT_IOM=1
while [ $CURRENT_IOM -lt 5 ]; do
    let CHANNEL=$CURRENT_IOM+3
    let I2C_SWITCH=$CURRENT_IOM+5
    let PORT_INDEX=$IOM_INDEX
    SECOND_CHANNEL=0
    while [ $SECOND_CHANNEL -lt 8 ]; do
        save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-0/0-0070/channel-$CHANNEL/$I2C_SWITCH-0071/channel-$SECOND_CHANNEL/$PORT_INDEX-0050/eeprom" "First 8 ports of IOM $CURRENT_IOM PORT $PORT_INDEX"
        let PORT_INDEX=$PORT_INDEX+1
        let SECOND_CHANNEL=$SECOND_CHANNEL+1
    done
    SECOND_CHANNEL=0
    while [ $SECOND_CHANNEL -lt 8 ]; do
        save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-0/0-0070/channel-$CHANNEL/$I2C_SWITCH-0072/channel-$SECOND_CHANNEL/$PORT_INDEX-0050/eeprom" "Second 8 ports of IOM $CURRENT_IOM PORT $PORT_INDEX"
        let PORT_INDEX=$PORT_INDEX+1
        let SECOND_CHANNEL=$SECOND_CHANNEL+1
    done
    let IOM_INDEX=$IOM_INDEX-16
    let CURRENT_IOM=$CURRENT_IOM+1
done

save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-0/0-0070/channel-2/4-0071/channel-4/14-003e/modalias" "IOM 1 CPLD MUX"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-0/0-0070/channel-2/4-0071/channel-6/16-003e/modalias" "IOM 2 CPLD MUX"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-0/0-0070/channel-2/4-0071/channel-5/15-003e/modalias" "IOM 3 CPLD MUX"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-0/0-0070/channel-2/4-0071/channel-7/17-003e/modalias" "IOM 4 CPLD MUX"

save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-0/0-0070/channel-2/4-0071/channel-1/11-0050/eeprom" "SFP ports"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-0/0-0070/channel-2/4-0071/channel-2/12-0050/eeprom" "SFP ports"

save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-14/14-003e/*" "IOM1"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-16/16-003e/*" "IOM2"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-15/15-003e/*" "IOM3"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-17/17-003e/*" "IOM4"

save_cmd "30" "grep -H '' /sys/devices/platform/SMF.512/hwmon/hwmon*/*" "SMF DIR"
save_cmd "10" "i2cdetect -l" "I2C trees"

save_cmd "10" "smartctl -a /dev/sda" "SSD"
save_cmd "10" "sgdisk -p /dev/sda" "SSD partitions"

save_cmd "10" "cat /host/reboot-cause/platform/reset_reason" "Reset Reason"
save_cmd "10" "cat /host/reboot-cause/platform/reboot_reason" "Reboot Reason"
save_cmd "10" "cat /host/reboot-cause/platform/bios_minor_version" "BIOS Version"
save_cmd "10" "show reboot-cause" "Reboot cause"

tar_file
