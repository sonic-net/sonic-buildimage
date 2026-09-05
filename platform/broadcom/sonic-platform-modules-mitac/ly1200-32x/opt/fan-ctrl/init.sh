#!/bin/bash
DIR=$(dirname $0)

# board type
BOARD_TYPE=`cat /sys/bus/i2c/devices/1-0032/brd_type`

# fan tables
FORWARD_SENSOR_CONF=${DIR}/fan-zone_F2B.conf
FORWARD_PWM_CONF=${DIR}/fan-zone-thermal_F2B.conf
REVERSE_SENSOR_CONF=${DIR}/fan-zone_B2F.conf
REVERSE_PWM_CONF=${DIR}/fan-zone-thermal_B2F.conf

# Nixon: Please refer to "BMS CPLD Design Spec."
# board type : BRD_TYPE[3:0]
# 0x0 (0000b): BMS_AC_PSU_NORMAL_FAN,  0x2 (0010b): BMS_DC_PSU_NORMAL_FAN
# 0x1 (0001b): BMS_AC_PSU_REVERSE_FAN, 0x3 (0011b): BMS_DC_PSU_REVERSE_FAN
case $BOARD_TYPE in
    0x0|0x2)
        ln -sf $FORWARD_SENSOR_CONF ${DIR}/fan-zone.conf
        ln -sf $FORWARD_PWM_CONF ${DIR}/fan-zone-thermal.conf
    ;;
    0x1|0x3)
        ln -sf $REVERSE_SENSOR_CONF ${DIR}/fan-zone.conf
        ln -sf $REVERSE_PWM_CONF ${DIR}/fan-zone-thermal.conf
    ;;
esac
