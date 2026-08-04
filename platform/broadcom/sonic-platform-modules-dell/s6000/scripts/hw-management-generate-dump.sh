#!/bin/bash
platform="$(sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)"
source /usr/share/sonic/device/$platform/actions.sh

I2C_ADAPTER="/sys/bus/i2c/devices/"

save_cmd "10" "hexdump -C /dev/nvram" "NVRAM dump"
save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-10/10-0053/eeprom" "System EEPROM"
save_cmd "10" "grep -H '' /sys/devices/platform/dell-s6000-cpld.0/*" "CPLD"

save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-0029/*" "FAN"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-0029/hwmon/hwmon*/*" "FAN"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-002a/*" "FAN"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-002a/hwmon/hwmon*/*" "FAN"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-0042/hwmon/hwmon*/*" "PSU"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-0040/hwmon/hwmon*/*" "PSU"

save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-004c/*" "TMP75"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-004c/hwmon/hwmon*/*" "TMP75"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-004d/*" "TMP75"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-004d/hwmon/hwmon*/*" "TMP75"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-004e/*" "TMP75"
save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-11/11-004e/hwmon/hwmon*/*" "TMP75"

INDEX=20
while [ $INDEX -lt 52 ]; do
    save_cmd "10" "hexdump -C $I2C_ADAPTER/i2c-$INDEX/$INDEX-0050/eeprom" "SFP ports"
    save_cmd "10" "grep -H '' $I2C_ADAPTER/i2c-$INDEX/$INDEX-0050/name $I2C_ADAPTER/i2c-$INDEX/$INDEX-0050/modalias" "SFP ports"
    let INDEX=$INDEX+1
done

save_cmd "10" "smartctl -a /dev/sda" "SSD"
save_cmd "10" "sgdisk -p /dev/sda" "SSD Partitions"

save_cmd "10" "cat /host/reboot-cause/reboot-cause.txt" "Reboot Reason"
save_cmd "10" "show reboot-cause" "Reboot cause"

tar_file
