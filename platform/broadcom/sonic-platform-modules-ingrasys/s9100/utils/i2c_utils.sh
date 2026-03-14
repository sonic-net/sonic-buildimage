#!/bin/bash

# Copyright (C) 2016 Ingrasys, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# trun on for more debug output
#DEBUG="on"

VERSION="1.1.0"
TRUE=200
FALSE=404

EXEC_FUNC=${1}
COLOR_LED=${2}
QSFP_PORT=${2}
QSFP_ACTION=${2}
MB_EEPROM_ACTION=${2}
ONOFF_LED=${3}
FAN_TRAY=${4}

############################################################
# Distributor ID: Debian
# Description:    Debian GNU/Linux 8.6 (jessie)
# Release:        8.6
# Codename:       jessie
# Linux debian 3.16.0-4-amd64 #1
# SMP Debian 3.16.36-1+deb8u1 (2016-09-03) x86_64 GNU/Linux
############################################################

# Color Definition
COLOR_TITLE="\e[1;32m"   ### Green ###
COLOR_WARNING="\e[1;33m" ### Yellow ###
COLOR_ERROR="\e[1;31m"   ### Red ###
COLOR_END="\e[0m"        ### END ###

NUM_IGB_DEVICE=0
NUM_I801_DEVICE=0
NUM_ISMT_DEVICE=$(( ${NUM_I801_DEVICE} + 1 ))
NUM_MUX1_CHAN0_DEVICE=$(( ${NUM_I801_DEVICE} + 2 ))
NUM_MUX1_CHAN1_DEVICE=$(( ${NUM_I801_DEVICE} + 3 ))
NUM_MUX1_CHAN2_DEVICE=$(( ${NUM_I801_DEVICE} + 4 ))
NUM_MUX1_CHAN3_DEVICE=$(( ${NUM_I801_DEVICE} + 5 ))
NUM_MUX1_CHAN4_DEVICE=$(( ${NUM_I801_DEVICE} + 6 ))
NUM_MUX1_CHAN5_DEVICE=$(( ${NUM_I801_DEVICE} + 7 ))
NUM_MUX1_CHAN6_DEVICE=$(( ${NUM_I801_DEVICE} + 8 ))
NUM_MUX1_CHAN7_DEVICE=$(( ${NUM_I801_DEVICE} + 9 ))
NUM_MUX3_CHAN0_DEVICE=$(( ${NUM_I801_DEVICE} + 10 ))
NUM_MUX4_CHAN0_DEVICE=$(( ${NUM_I801_DEVICE} + 18 ))
NUM_MUX5_CHAN0_DEVICE=$(( ${NUM_I801_DEVICE} + 26 ))
NUM_MUX6_CHAN0_DEVICE=$(( ${NUM_I801_DEVICE} + 34 ))

PATH_SYS_I2C_DEVICES="/sys/bus/i2c/devices"
PATH_HWMON_ROOT_DEVICES="/sys/class/hwmon"
PATH_HWMON_W83795_DEVICE="${PATH_HWMON_ROOT_DEVICES}/hwmon1"
PATH_I801_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_I801_DEVICE}"
PATH_ISMT_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_ISMT_DEVICE}"
PATH_MUX_CHAN0_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN0_DEVICE}"
PATH_MUX_CHAN1_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN1_DEVICE}"
PATH_MUX_CHAN2_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN2_DEVICE}"
PATH_MUX_CHAN3_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN3_DEVICE}"
PATH_MUX_CHAN4_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN4_DEVICE}"
PATH_MUX_CHAN5_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN5_DEVICE}"
PATH_MUX_CHAN6_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN6_DEVICE}"
PATH_MUX_CHAN7_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN7_DEVICE}"

#Power Supply Status
PSU_DC_ON=1
PSU_DC_OFF=0
PSU_EXIST=1
PSU_NOT_EXIST=0

#GPIO Offset
GPIO_OFFSET=0

# Help usage function
function _help {
    echo "========================================================="
    echo "# Description: Help Function"
    echo "# Version    : ${VERSION}"
    echo "========================================================="
    echo "----------------------------------------------------"
    echo "EX       : ${0} help"
    echo "         : ${0} i2c_init"
    echo "         : ${0} i2c_deinit"
    echo "         : ${0} i2c_temp_init"
    echo "         : ${0} i2c_fan_init"
    echo "         : ${0} i2c_volmon_init"
    echo "         : ${0} i2c_io_exp_init"
    echo "         : ${0} i2c_gpio_init"
    echo "         : ${0} i2c_gpio_deinit"
    echo "         : ${0} i2c_led_test"
    echo "         : ${0} i2c_psu_eeprom_get"
    echo "         : ${0} i2c_mb_eeprom_get"
    echo "         : ${0} i2c_qsfp_eeprom_get [1-32]"
    echo "         : ${0} i2c_qsfp_eeprom_init new|delete"
    echo "         : ${0} i2c_mb_eeprom_init new|delete"
    echo "         : ${0} i2c_qsfp_status_get [1-32]"
    echo "         : ${0} i2c_qsfp_type_get [1-32]"
    echo "         : ${0} i2c_board_type_get"
    echo "         : ${0} i2c_psu_status"
    echo "         : ${0} i2c_led_psu_status_set"
    echo "         : ${0} i2c_led_fan_status_set"
    echo "         : ${0} i2c_led_fan_tray_status_set"
    echo "         : ${0} i2c_cpld_version"
    echo "         : ${0} i2c_front_temp"
    echo "         : ${0} i2c_rear_temp"
    echo "         : ${0} i2c_test_all"
    echo "         : ${0} i2c_sys_led green|amber on|off"
    echo "         : ${0} i2c_fan_led green|amber on|off"
    echo "         : ${0} i2c_psu1_led green|amber on|off"
    echo "         : ${0} i2c_psu2_led green|amber on|off"
    echo "         : ${0} i2c_fan_tray_led green|amber on|off [1-4]"
    echo "----------------------------------------------------"
}

#Pause function
function _pause {
    read -p "$*"
}

#Retry command function
function _retry {
    local i
    for i in {1..5};
    do
       eval "${*}" && break || echo "retry"; sleep 1;
    done
}

#I2C Init
function _i2c_init {
    echo "========================================================="
    echo "# Description: I2C Init"
    echo "========================================================="

    rmmod i2c_ismt
    rmmod i2c_i801
    modprobe i2c_i801
    modprobe i2c_ismt
    modprobe i2c_dev
    modprobe i2c_mux_pca954x force_deselect_on_exit=1

    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN0_DEVICE}" ]; then
        _retry "echo 'pca9548 0x70' > ${PATH_ISMT_DEVICE}/new_device"
    else
        echo "pca9548 0x70 already init."
    fi
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX3_CHAN0_DEVICE}" ]; then
        _retry "echo 'pca9548 0x71' > ${PATH_MUX_CHAN0_DEVICE}/new_device"
    else
        echo "pca9548 0x71 already init."
    fi
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX4_CHAN0_DEVICE}" ]; then
        _retry "echo 'pca9548 0x72' > ${PATH_MUX_CHAN1_DEVICE}/new_device"
    else
        echo "pca9548 0x72 already init."
    fi
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX5_CHAN0_DEVICE}" ]; then
        _retry "echo 'pca9548 0x73' > ${PATH_MUX_CHAN2_DEVICE}/new_device"
    else
        echo "pca9548 0x73 already init."
    fi
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX6_CHAN0_DEVICE}" ]; then
        _retry "echo 'pca9548 0x74' > ${PATH_MUX_CHAN3_DEVICE}/new_device"
    else
        echo "pca9548 0x74 already init."
    fi
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/${NUM_MUX1_CHAN6_DEVICE}-0075" ]; then
        _retry "echo 'pca9546 0x75' > ${PATH_MUX_CHAN6_DEVICE}/new_device"
    else
        echo "pca9548 0x75 already init."
    fi
    #Init CPLD LED_CLR Register (Front Port LED)
    i2cset -y ${NUM_I801_DEVICE} 0x33 0x34 0x10

    rmmod coretemp
    rmmod jc42
    rmmod w83795
    _i2c_temp_init
    _i2c_volmon_init
    modprobe coretemp
    modprobe w83795
    modprobe jc42
    modprobe eeprom
    modprobe eeprom_mb
    modprobe gpio-pca953x
    _i2c_fan_init
    _i2c_io_exp_init
    rmmod gpio_ich
    _i2c_gpio_init
    modprobe gpio_ich
    _i2c_cpld_init
    _i2c_qsfp_eeprom_init "new"
    _i2c_mb_eeprom_init "new"
    _i2c_psu_eeprom_init
    _i2c_led_psu_status_set
    _i2c_led_fan_status_set
    COLOR_LED="green"
    ONOFF_LED="on"
    echo "${COLOR_LED} ${ONOFF_LED}"
    _i2c_sys_led
    COLOR_LED="amber"
    ONOFF_LED="off"
    echo "${COLOR_LED} ${ONOFF_LED}"
    _i2c_sys_led
    _config_rmem
}

#I2C Deinit
function _i2c_deinit {
    _i2c_gpio_deinit
    for mod in i2c_cpld coretemp jc42 w83795 eeprom_mb gpio-pca953x i2c_mux_pca954x i2c_ismt i2c_i801;
    do
        [ "$(lsmod | grep "^$mod ")" != "" ] && rmmod $mod
    done
}

#Temperature sensor Init
function _i2c_temp_init {
    echo -n "TEMP INIT..."
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x00 0x80
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x05 0x7F
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x04 0x0A
    echo "Done"
}

#FAN Init
function _i2c_fan_init {
    echo -n "FAN INIT..."
    if [ -e "${PATH_HWMON_W83795_DEVICE}" ]; then
        echo 120 > ${PATH_HWMON_W83795_DEVICE}/device/pwm1
        echo 120 > ${PATH_HWMON_W83795_DEVICE}/device/pwm2
        echo "SUCCESS"
    else
        echo "FAIL"
    fi

}

#VOLMON Init
function _i2c_volmon_init {
    echo -n "VOLMON INIT..."
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x00 0x80
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x02 0xFF
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x03 0x50
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x04 0x0A
    echo "Done"
}

#IO Expander Init
function _i2c_io_exp_init {
    echo "========================================================="
    echo "# Description: I2C IO Expender Init"
    echo "========================================================="
    #SMBUS0 IO_EXPENDER
    i2cset -y -r ${NUM_I801_DEVICE} 0x27 4 0x00
    i2cset -y -r ${NUM_I801_DEVICE} 0x27 5 0x00
    i2cset -y -r ${NUM_I801_DEVICE} 0x27 2 0x00
    i2cset -y -r ${NUM_I801_DEVICE} 0x27 3 0x00
    i2cset -y -r ${NUM_I801_DEVICE} 0x27 6 0xFF
    i2cset -y -r ${NUM_I801_DEVICE} 0x27 7 0xFF

    #SMBUS1
    #ABS
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x20 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x20 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x20 6 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x20 7 0xFF

    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x21 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x21 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x21 6 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x21 7 0xFF

    #Transcevior INT
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 6 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 7 0xFF

    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x23 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x23 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x23 6 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x23 7 0xFF

    echo "Init ZQSFP IO Expender"
    echo "set ZQSFP LP_MODE = 0"
    #set ZQSFP LP_MODE = 0
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 2 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 6 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 7 0x00

    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x21 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x21 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x21 2 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x21 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x21 6 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x21 7 0x00

    echo "set ZQSFP RST = 1"
    #set ZQSFP RST = 1
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x22 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x22 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x22 2 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x22 3 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x22 6 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x22 7 0x00

    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x23 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x23 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x23 2 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x23 3 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x23 6 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x23 7 0x00

    echo "set ZQSFP mode"
    #ZQSFP mode
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x24 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x24 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x24 2 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x24 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x24 6 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x24 7 0x00

    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x25 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x25 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x25 2 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x25 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x25 6 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x25 7 0x00

    #ZQSFP/SFP+/E-Card General
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x20 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x20 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x20 6 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x20 7 0xFF

    #LED board after PVT (S9100_IO_EXP_LED_ID)
    echo "Init LED IO Expender"
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 6 0x00
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 7 0x00

    #PSU I/O (S9100_IO_EXP_PSU_ID)
    echo "Init PSU IO Expender"
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x23 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x23 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x23 2 0x00
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x23 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x23 6 0xBB
    i2cset -y -r ${NUM_MUX1_CHAN6_DEVICE} 0x23 7 0xFF

    #FAN I/O (S9100_IO_EXP_FAN_ID)
    echo "Init FAN IO Expender"
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x20 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x20 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x20 2 0x00
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x20 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x20 6 0xCC
    i2cset -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x20 7 0xCC
}

# To set the global variable GPIO_OFFSET
function _set_gpio_offset {
    GPIO_OFFSET=0
    for d in `ls /sys/class/gpio/ | grep gpiochip`
    do
        gpiochip_no=${d##gpiochip}
        if [ $gpiochip_no -gt 255 ]; then
            GPIO_OFFSET=256
            break
        fi
    done
    #echo "set GPIO_OFFSET=${GPIO_OFFSET}"
}

#GPIO Init
function _i2c_gpio_init {
    #ABS Port 0-15 (zQSFP00 ~ zQSFP15)
    echo "pca9535 0x20" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN4_DEVICE}/new_device
    _set_gpio_offset
    for((i=${GPIO_OFFSET}+240;i<=${GPIO_OFFSET}+255;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
    done

    #echo 240 > /sys/class/gpio/export
    #echo 241 > /sys/class/gpio/export
    #echo 242 > /sys/class/gpio/export
    #echo 243 > /sys/class/gpio/export
    #echo 244 > /sys/class/gpio/export
    #echo 245 > /sys/class/gpio/export
    #echo 246 > /sys/class/gpio/export
    #echo 247 > /sys/class/gpio/export
    #echo 248 > /sys/class/gpio/export
    #echo 249 > /sys/class/gpio/export
    #echo 250 > /sys/class/gpio/export
    #echo 251 > /sys/class/gpio/export
    #echo 252 > /sys/class/gpio/export
    #echo 253 > /sys/class/gpio/export
    #echo 254 > /sys/class/gpio/export
    #echo 255 > /sys/class/gpio/export
    #echo 1 > /sys/class/gpio/gpio241/active_low #zQSFP00
    #echo 1 > /sys/class/gpio/gpio240/active_low #zQSFP01
    #echo 1 > /sys/class/gpio/gpio243/active_low #zQSFP02
    #echo 1 > /sys/class/gpio/gpio242/active_low #zQSFP03
    #echo 1 > /sys/class/gpio/gpio245/active_low #zQSFP04
    #echo 1 > /sys/class/gpio/gpio244/active_low #zQSFP05
    #echo 1 > /sys/class/gpio/gpio247/active_low #zQSFP06
    #echo 1 > /sys/class/gpio/gpio246/active_low #zQSFP07
    #echo 1 > /sys/class/gpio/gpio249/active_low #zQSFP08
    #echo 1 > /sys/class/gpio/gpio248/active_low #zQSFP09
    #echo 1 > /sys/class/gpio/gpio251/active_low #zQSFP10
    #echo 1 > /sys/class/gpio/gpio250/active_low #zQSFP11
    #echo 1 > /sys/class/gpio/gpio253/active_low #zQSFP12
    #echo 1 > /sys/class/gpio/gpio252/active_low #zQSFP13
    #echo 1 > /sys/class/gpio/gpio255/active_low #zQSFP14
    #echo 1 > /sys/class/gpio/gpio254/active_low #zQSFP15

    #ABS Port 16-31 (zQSFP16 ~ zQSFP31)
    echo "pca9535 0x21" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN4_DEVICE}/new_device
    for((i=${GPIO_OFFSET}+224;i<=${GPIO_OFFSET}+239;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
    done

    #echo 224 > /sys/class/gpio/export
    #echo 225 > /sys/class/gpio/export
    #echo 226 > /sys/class/gpio/export
    #echo 227 > /sys/class/gpio/export
    #echo 228 > /sys/class/gpio/export
    #echo 229 > /sys/class/gpio/export
    #echo 230 > /sys/class/gpio/export
    #echo 231 > /sys/class/gpio/export
    #echo 232 > /sys/class/gpio/export
    #echo 233 > /sys/class/gpio/export
    #echo 234 > /sys/class/gpio/export
    #echo 235 > /sys/class/gpio/export
    #echo 236 > /sys/class/gpio/export
    #echo 237 > /sys/class/gpio/export
    #echo 238 > /sys/class/gpio/export
    #echo 239 > /sys/class/gpio/export
    #echo 1 > /sys/class/gpio/gpio225/active_low #zQSFP16
    #echo 1 > /sys/class/gpio/gpio224/active_low #zQSFP17
    #echo 1 > /sys/class/gpio/gpio227/active_low #zQSFP18
    #echo 1 > /sys/class/gpio/gpio226/active_low #zQSFP19
    #echo 1 > /sys/class/gpio/gpio229/active_low #zQSFP20
    #echo 1 > /sys/class/gpio/gpio228/active_low #zQSFP21
    #echo 1 > /sys/class/gpio/gpio231/active_low #zQSFP22
    #echo 1 > /sys/class/gpio/gpio230/active_low #zQSFP23
    #echo 1 > /sys/class/gpio/gpio233/active_low #zQSFP24
    #echo 1 > /sys/class/gpio/gpio232/active_low #zQSFP25
    #echo 1 > /sys/class/gpio/gpio235/active_low #zQSFP26
    #echo 1 > /sys/class/gpio/gpio234/active_low #zQSFP27
    #echo 1 > /sys/class/gpio/gpio237/active_low #zQSFP28
    #echo 1 > /sys/class/gpio/gpio236/active_low #zQSFP29
    #echo 1 > /sys/class/gpio/gpio239/active_low #zQSFP30
    #echo 1 > /sys/class/gpio/gpio238/active_low #zQSFP31

    #INT Port 0-15 (zQSFP00 ~ zQSFP15)
    echo "pca9535 0x22" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN4_DEVICE}/new_device
    for((i=${GPIO_OFFSET}+208;i<=${GPIO_OFFSET}+223;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
    done

    #echo 208 > /sys/class/gpio/export
    #echo 209 > /sys/class/gpio/export
    #echo 210 > /sys/class/gpio/export
    #echo 211 > /sys/class/gpio/export
    #echo 212 > /sys/class/gpio/export
    #echo 213 > /sys/class/gpio/export
    #echo 214 > /sys/class/gpio/export
    #echo 215 > /sys/class/gpio/export
    #echo 216 > /sys/class/gpio/export
    #echo 217 > /sys/class/gpio/export
    #echo 218 > /sys/class/gpio/export
    #echo 219 > /sys/class/gpio/export
    #echo 220 > /sys/class/gpio/export
    #echo 221 > /sys/class/gpio/export
    #echo 222 > /sys/class/gpio/export
    #echo 223 > /sys/class/gpio/export
    #echo 1 > /sys/class/gpio/gpio208/active_low
    #echo 1 > /sys/class/gpio/gpio209/active_low
    #echo 1 > /sys/class/gpio/gpio210/active_low
    #echo 1 > /sys/class/gpio/gpio211/active_low
    #echo 1 > /sys/class/gpio/gpio212/active_low
    #echo 1 > /sys/class/gpio/gpio213/active_low
    #echo 1 > /sys/class/gpio/gpio214/active_low
    #echo 1 > /sys/class/gpio/gpio215/active_low
    #echo 1 > /sys/class/gpio/gpio216/active_low
    #echo 1 > /sys/class/gpio/gpio217/active_low
    #echo 1 > /sys/class/gpio/gpio218/active_low
    #echo 1 > /sys/class/gpio/gpio219/active_low
    #echo 1 > /sys/class/gpio/gpio220/active_low
    #echo 1 > /sys/class/gpio/gpio221/active_low
    #echo 1 > /sys/class/gpio/gpio222/active_low
    #echo 1 > /sys/class/gpio/gpio223/active_low

    #INT Port 16-31 (zQSFP16 ~ zQSFP31)
    echo "pca9535 0x23" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN4_DEVICE}/new_device
    for((i=${GPIO_OFFSET}+192;i<=${GPIO_OFFSET}+207;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
    done

    #echo 192 > /sys/class/gpio/export
    #echo 193 > /sys/class/gpio/export
    #echo 194 > /sys/class/gpio/export
    #echo 195 > /sys/class/gpio/export
    #echo 196 > /sys/class/gpio/export
    #echo 197 > /sys/class/gpio/export
    #echo 198 > /sys/class/gpio/export
    #echo 199 > /sys/class/gpio/export
    #echo 200 > /sys/class/gpio/export
    #echo 201 > /sys/class/gpio/export
    #echo 202 > /sys/class/gpio/export
    #echo 203 > /sys/class/gpio/export
    #echo 204 > /sys/class/gpio/export
    #echo 205 > /sys/class/gpio/export
    #echo 206 > /sys/class/gpio/export
    #echo 207 > /sys/class/gpio/export
    #echo 1 > /sys/class/gpio/gpio192/active_low
    #echo 1 > /sys/class/gpio/gpio193/active_low
    #echo 1 > /sys/class/gpio/gpio194/active_low
    #echo 1 > /sys/class/gpio/gpio195/active_low
    #echo 1 > /sys/class/gpio/gpio196/active_low
    #echo 1 > /sys/class/gpio/gpio197/active_low
    #echo 1 > /sys/class/gpio/gpio198/active_low
    #echo 1 > /sys/class/gpio/gpio199/active_low
    #echo 1 > /sys/class/gpio/gpio200/active_low
    #echo 1 > /sys/class/gpio/gpio201/active_low
    #echo 1 > /sys/class/gpio/gpio202/active_low
    #echo 1 > /sys/class/gpio/gpio203/active_low
    #echo 1 > /sys/class/gpio/gpio204/active_low
    #echo 1 > /sys/class/gpio/gpio205/active_low
    #echo 1 > /sys/class/gpio/gpio206/active_low
    #echo 1 > /sys/class/gpio/gpio207/active_low

    #LP Mode Port 0-15
    echo "pca9535 0x20" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN5_DEVICE}/new_device
    for((i=${GPIO_OFFSET}+176;i<=${GPIO_OFFSET}+191;i++));
    do
        echo $i > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio${i}/direction
    done

    #echo 176 > /sys/class/gpio/export
    #echo 177 > /sys/class/gpio/export
    #echo 178 > /sys/class/gpio/export
    #echo 179 > /sys/class/gpio/export
    #echo 180 > /sys/class/gpio/export
    #echo 181 > /sys/class/gpio/export
    #echo 182 > /sys/class/gpio/export
    #echo 183 > /sys/class/gpio/export
    #echo 184 > /sys/class/gpio/export
    #echo 185 > /sys/class/gpio/export
    #echo 186 > /sys/class/gpio/export
    #echo 187 > /sys/class/gpio/export
    #echo 188 > /sys/class/gpio/export
    #echo 189 > /sys/class/gpio/export
    #echo 190 > /sys/class/gpio/export
    #echo 191 > /sys/class/gpio/export
    #echo out > /sys/class/gpio/gpio176/direction
    #echo out > /sys/class/gpio/gpio177/direction
    #echo out > /sys/class/gpio/gpio178/direction
    #echo out > /sys/class/gpio/gpio179/direction
    #echo out > /sys/class/gpio/gpio180/direction
    #echo out > /sys/class/gpio/gpio181/direction
    #echo out > /sys/class/gpio/gpio182/direction
    #echo out > /sys/class/gpio/gpio183/direction
    #echo out > /sys/class/gpio/gpio184/direction
    #echo out > /sys/class/gpio/gpio185/direction
    #echo out > /sys/class/gpio/gpio186/direction
    #echo out > /sys/class/gpio/gpio187/direction
    #echo out > /sys/class/gpio/gpio188/direction
    #echo out > /sys/class/gpio/gpio189/direction
    #echo out > /sys/class/gpio/gpio190/direction
    #echo out > /sys/class/gpio/gpio191/direction

    #LP Mode Port 16-31
    echo "pca9535 0x21" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN5_DEVICE}/new_device
    for((i=${GPIO_OFFSET}+160;i<=${GPIO_OFFSET}+175;i++));
    do
        echo $i > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio${i}/direction
    done

    #echo 160 > /sys/class/gpio/export
    #echo 161 > /sys/class/gpio/export
    #echo 162 > /sys/class/gpio/export
    #echo 163 > /sys/class/gpio/export
    #echo 164 > /sys/class/gpio/export
    #echo 165 > /sys/class/gpio/export
    #echo 166 > /sys/class/gpio/export
    #echo 167 > /sys/class/gpio/export
    #echo 168 > /sys/class/gpio/export
    #echo 169 > /sys/class/gpio/export
    #echo 170 > /sys/class/gpio/export
    #echo 171 > /sys/class/gpio/export
    #echo 172 > /sys/class/gpio/export
    #echo 173 > /sys/class/gpio/export
    #echo 174 > /sys/class/gpio/export
    #echo 175 > /sys/class/gpio/export
    #echo out > /sys/class/gpio/gpio160/direction
    #echo out > /sys/class/gpio/gpio161/direction
    #echo out > /sys/class/gpio/gpio162/direction
    #echo out > /sys/class/gpio/gpio163/direction
    #echo out > /sys/class/gpio/gpio164/direction
    #echo out > /sys/class/gpio/gpio165/direction
    #echo out > /sys/class/gpio/gpio166/direction
    #echo out > /sys/class/gpio/gpio167/direction
    #echo out > /sys/class/gpio/gpio168/direction
    #echo out > /sys/class/gpio/gpio169/direction
    #echo out > /sys/class/gpio/gpio170/direction
    #echo out > /sys/class/gpio/gpio171/direction
    #echo out > /sys/class/gpio/gpio172/direction
    #echo out > /sys/class/gpio/gpio173/direction
    #echo out > /sys/class/gpio/gpio174/direction
    #echo out > /sys/class/gpio/gpio175/direction

    #RST Port 0-15
    echo "pca9535 0x22" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN5_DEVICE}/new_device
    for((i=${GPIO_OFFSET}+144;i<=${GPIO_OFFSET}+159;i++));        
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
        echo low > /sys/class/gpio/gpio${i}/direction
        #echo out > /sys/class/gpio/gpio${i}/direction
        #echo 0 > /sys/class/gpio/gpio${i}/value
    done

    #echo 144 > /sys/class/gpio/export
    #echo 145 > /sys/class/gpio/export
    #echo 146 > /sys/class/gpio/export
    #echo 147 > /sys/class/gpio/export
    #echo 148 > /sys/class/gpio/export
    #echo 149 > /sys/class/gpio/export
    #echo 150 > /sys/class/gpio/export
    #echo 151 > /sys/class/gpio/export
    #echo 152 > /sys/class/gpio/export
    #echo 153 > /sys/class/gpio/export
    #echo 154 > /sys/class/gpio/export
    #echo 155 > /sys/class/gpio/export
    #echo 156 > /sys/class/gpio/export
    #echo 157 > /sys/class/gpio/export
    #echo 158 > /sys/class/gpio/export
    #echo 159 > /sys/class/gpio/export
    #echo out > /sys/class/gpio/gpio144/direction
    #echo out > /sys/class/gpio/gpio145/direction
    #echo out > /sys/class/gpio/gpio146/direction
    #echo out > /sys/class/gpio/gpio147/direction
    #echo out > /sys/class/gpio/gpio148/direction
    #echo out > /sys/class/gpio/gpio149/direction
    #echo out > /sys/class/gpio/gpio150/direction
    #echo out > /sys/class/gpio/gpio151/direction
    #echo out > /sys/class/gpio/gpio152/direction
    #echo out > /sys/class/gpio/gpio153/direction
    #echo out > /sys/class/gpio/gpio154/direction
    #echo out > /sys/class/gpio/gpio155/direction
    #echo out > /sys/class/gpio/gpio156/direction
    #echo out > /sys/class/gpio/gpio157/direction
    #echo out > /sys/class/gpio/gpio158/direction
    #echo out > /sys/class/gpio/gpio159/direction
    #echo 1 > /sys/class/gpio/gpio144/active_low
    #echo 1 > /sys/class/gpio/gpio145/active_low
    #echo 1 > /sys/class/gpio/gpio146/active_low
    #echo 1 > /sys/class/gpio/gpio147/active_low
    #echo 1 > /sys/class/gpio/gpio148/active_low
    #echo 1 > /sys/class/gpio/gpio149/active_low
    #echo 1 > /sys/class/gpio/gpio150/active_low
    #echo 1 > /sys/class/gpio/gpio151/active_low
    #echo 1 > /sys/class/gpio/gpio152/active_low
    #echo 1 > /sys/class/gpio/gpio153/active_low
    #echo 1 > /sys/class/gpio/gpio154/active_low
    #echo 1 > /sys/class/gpio/gpio155/active_low
    #echo 1 > /sys/class/gpio/gpio156/active_low
    #echo 1 > /sys/class/gpio/gpio157/active_low
    #echo 1 > /sys/class/gpio/gpio158/active_low
    #echo 1 > /sys/class/gpio/gpio159/active_low
    #echo 0 > /sys/class/gpio/gpio144/value
    #echo 0 > /sys/class/gpio/gpio145/value
    #echo 0 > /sys/class/gpio/gpio146/value
    #echo 0 > /sys/class/gpio/gpio147/value
    #echo 0 > /sys/class/gpio/gpio148/value
    #echo 0 > /sys/class/gpio/gpio149/value
    #echo 0 > /sys/class/gpio/gpio150/value
    #echo 0 > /sys/class/gpio/gpio151/value
    #echo 0 > /sys/class/gpio/gpio152/value
    #echo 0 > /sys/class/gpio/gpio153/value
    #echo 0 > /sys/class/gpio/gpio154/value
    #echo 0 > /sys/class/gpio/gpio155/value
    #echo 0 > /sys/class/gpio/gpio156/value
    #echo 0 > /sys/class/gpio/gpio157/value
    #echo 0 > /sys/class/gpio/gpio158/value
    #echo 0 > /sys/class/gpio/gpio159/value

    #RST Port 16-31
    echo "pca9535 0x23" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN5_DEVICE}/new_device
    for((i=${GPIO_OFFSET}+128;i<=${GPIO_OFFSET}+143;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
        echo low > /sys/class/gpio/gpio${i}/direction
        #echo out > /sys/class/gpio/gpio${i}/direction
        #echo 0 > /sys/class/gpio/gpio${i}/value
    done

    #echo 128 > /sys/class/gpio/export
    #echo 129 > /sys/class/gpio/export
    #echo 130 > /sys/class/gpio/export
    #echo 131 > /sys/class/gpio/export
    #echo 132 > /sys/class/gpio/export
    #echo 133 > /sys/class/gpio/export
    #echo 134 > /sys/class/gpio/export
    #echo 135 > /sys/class/gpio/export
    #echo 136 > /sys/class/gpio/export
    #echo 137 > /sys/class/gpio/export
    #echo 138 > /sys/class/gpio/export
    #echo 139 > /sys/class/gpio/export
    #echo 140 > /sys/class/gpio/export
    #echo 141 > /sys/class/gpio/export
    #echo 142 > /sys/class/gpio/export
    #echo 143 > /sys/class/gpio/export
    #echo out > /sys/class/gpio/gpio128/direction
    #echo out > /sys/class/gpio/gpio129/direction
    #echo out > /sys/class/gpio/gpio130/direction
    #echo out > /sys/class/gpio/gpio131/direction
    #echo out > /sys/class/gpio/gpio132/direction
    #echo out > /sys/class/gpio/gpio133/direction
    #echo out > /sys/class/gpio/gpio134/direction
    #echo out > /sys/class/gpio/gpio135/direction
    #echo out > /sys/class/gpio/gpio136/direction
    #echo out > /sys/class/gpio/gpio137/direction
    #echo out > /sys/class/gpio/gpio138/direction
    #echo out > /sys/class/gpio/gpio139/direction
    #echo out > /sys/class/gpio/gpio140/direction
    #echo out > /sys/class/gpio/gpio141/direction
    #echo out > /sys/class/gpio/gpio142/direction
    #echo out > /sys/class/gpio/gpio143/direction
    #echo 1 > /sys/class/gpio/gpio128/active_low
    #echo 1 > /sys/class/gpio/gpio129/active_low
    #echo 1 > /sys/class/gpio/gpio130/active_low
    #echo 1 > /sys/class/gpio/gpio131/active_low
    #echo 1 > /sys/class/gpio/gpio132/active_low
    #echo 1 > /sys/class/gpio/gpio133/active_low
    #echo 1 > /sys/class/gpio/gpio134/active_low
    #echo 1 > /sys/class/gpio/gpio135/active_low
    #echo 1 > /sys/class/gpio/gpio136/active_low
    #echo 1 > /sys/class/gpio/gpio137/active_low
    #echo 1 > /sys/class/gpio/gpio138/active_low
    #echo 1 > /sys/class/gpio/gpio139/active_low
    #echo 1 > /sys/class/gpio/gpio140/active_low
    #echo 1 > /sys/class/gpio/gpio141/active_low
    #echo 1 > /sys/class/gpio/gpio142/active_low
    #echo 1 > /sys/class/gpio/gpio143/active_low
    #echo 0 > /sys/class/gpio/gpio128/value
    #echo 0 > /sys/class/gpio/gpio129/value
    #echo 0 > /sys/class/gpio/gpio130/value
    #echo 0 > /sys/class/gpio/gpio131/value
    #echo 0 > /sys/class/gpio/gpio132/value
    #echo 0 > /sys/class/gpio/gpio133/value
    #echo 0 > /sys/class/gpio/gpio134/value
    #echo 0 > /sys/class/gpio/gpio135/value
    #echo 0 > /sys/class/gpio/gpio136/value
    #echo 0 > /sys/class/gpio/gpio137/value
    #echo 0 > /sys/class/gpio/gpio138/value
    #echo 0 > /sys/class/gpio/gpio139/value
    #echo 0 > /sys/class/gpio/gpio140/value
    #echo 0 > /sys/class/gpio/gpio141/value
    #echo 0 > /sys/class/gpio/gpio142/value
    #echo 0 > /sys/class/gpio/gpio143/value

}

#GPIO DeInit
function _i2c_gpio_deinit {
    echo "0x20" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN4_DEVICE}/delete_device
    echo "0x21" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN4_DEVICE}/delete_device
    echo "0x22" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN4_DEVICE}/delete_device
    echo "0x23" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN4_DEVICE}/delete_device
    echo "0x20" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN5_DEVICE}/delete_device
    echo "0x21" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN5_DEVICE}/delete_device
    echo "0x22" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN5_DEVICE}/delete_device
    echo "0x23" > /sys/bus/i2c/devices/i2c-${NUM_MUX1_CHAN5_DEVICE}/delete_device
}

#I2C CPLD init
function _i2c_cpld_init {
    echo "========================================================="
    echo "# Description: I2C CPLD Init..."
    echo "========================================================="

    ## modprobe i2c_cpld
    modprobe i2c_cpld
    ## Add CPLD device
    echo "i2c_cpld 0x33" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_I801_DEVICE}/new_device

    echo "done..."
}

#Set FAN Tray LED
function _i2c_led_fan_tray_status_set {
    echo "FAN Tray Status Setup"
    #FAN Status get
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`
    FAN2_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan2_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN4_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan4_alarm`
    FAN5_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN6_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan6_alarm`
    FAN7_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`
    FAN8_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan8_alarm`

    if [ "${FAN1_ALARM}" == "0" ] && [ "${FAN2_ALARM}" == "0" ]; then
	FAN_TRAY=1
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
	FAN_TRAY=1
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    fi

    if [ "${FAN3_ALARM}" == "0" ] && [ "${FAN4_ALARM}" == "0" ]; then
	FAN_TRAY=2
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
	FAN_TRAY=2
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    fi

    if [ "${FAN5_ALARM}" == "0" ] && [ "${FAN6_ALARM}" == "0" ]; then
	FAN_TRAY=3
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
	FAN_TRAY=3
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    fi

    if [ "${FAN7_ALARM}" == "0" ] && [ "${FAN8_ALARM}" == "0" ]; then
	FAN_TRAY=4
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
	FAN_TRAY=4
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    fi
}

#Set FAN LED
function _i2c_led_fan_status_set {
    echo "FAN Status Setup"
    #PSU Status set
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`
    FAN2_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan2_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN4_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan4_alarm`
    FAN5_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN6_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan6_alarm`
    FAN7_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`
    FAN8_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan8_alarm`

    if [ "${FAN1_ALARM}" == "0" ] && [ "${FAN2_ALARM}" == "0" ] \
       && [ "${FAN3_ALARM}" == "0" ] && [ "${FAN4_ALARM}" == "0" ] \
       && [ "${FAN5_ALARM}" == "0" ] && [ "${FAN6_ALARM}" == "0" ] \
       && [ "${FAN7_ALARM}" == "0" ] && [ "${FAN8_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_led
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_led
    fi
}

#Set Power Supply LED
function _i2c_led_psu_status_set {
    echo "PSU LED Status Setup"

    #PSU Status set
    _i2c_psu_status

    #PSU1 Status
    if [ "${psu1Exist}" == ${PSU_EXIST} ]; then
        if [ "${psu1PwGood}" == ${PSU_DC_ON} ]; then
            COLOR_LED="green"
            ONOFF_LED="on"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu1_led
            COLOR_LED="amber"
            ONOFF_LED="off"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu1_led
        else
            COLOR_LED="green"
            ONOFF_LED="off"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu1_led
            COLOR_LED="amber"
            ONOFF_LED="on"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu1_led
        fi
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_psu1_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_psu1_led
    fi

    #PSU2 Status
    if [ "${psu2Exist}" == ${PSU_EXIST} ]; then
        if [ "${psu2PwGood}" == ${PSU_DC_ON} ]; then
            COLOR_LED="green"
            ONOFF_LED="on"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu2_led
            COLOR_LED="amber"
            ONOFF_LED="off"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu2_led
        else
            COLOR_LED="green"
            ONOFF_LED="off"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu2_led
            COLOR_LED="amber"
            ONOFF_LED="on"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu2_led
        fi
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_psu2_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_psu2_led
    fi
}

#LED Test
function _i2c_led_test {
    echo "========================================================="
    echo "# Description: I2C LED TEST..."
    echo "========================================================="
    #sys led (green)
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x7F
    _pause 'Check SYS LED green light and Press [Enter] key to continue...'
    #sys led (amber)
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xBF
    _pause 'Check SYS LED amber light and Press [Enter] key to continue...'

    #FAN led (green)
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xF7
    _pause 'Check FAN LED green light and Press [Enter] key to continue...'
    #FAN led (amber)
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFB
    _pause 'Check FAN LED amber light and Press [Enter] key to continue...'

    #PSU2 led (green)
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xDF
    _pause 'Check PSU2 LED green light and Press [Enter] key to continue...'
    #PSU2 led (amber)
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xEF
    _pause 'Check PSU2 LED amber light and Press [Enter] key to continue...'

    #PSU1 led (green)
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFD
    _pause 'Check PSU1 LED green light and Press [Enter] key to continue...'
    #PSU1 led (amber)
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFE
    _pause 'Check PSU1 LED amber light and Press [Enter] key to continue...'

    #Turn OFF All LED
    i2cset -y ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    _pause 'Check turn off all LEDs and Press [Enter] key to continue...'
    echo "done..."
}

#Set QSFP Port variable
function _qsfp_port_i2c_var_set {
    local port=$1
    case ${port} in
        1|2|3|4|5|6|7|8)
            i2cbus=${NUM_MUX1_CHAN4_DEVICE}
            regAddr=0x20
            dataAddr=0
            eeprombusbase=${NUM_MUX3_CHAN0_DEVICE}
            gpioBase=$((${GPIO_OFFSET}+240))
            #gpioBase=240
        ;;
        9|10|11|12|13|14|15|16)
            i2cbus=${NUM_MUX1_CHAN4_DEVICE}
            regAddr=0x20
            dataAddr=1
            eeprombusbase=${NUM_MUX4_CHAN0_DEVICE}
            gpioBase=$((${GPIO_OFFSET}+240))
            #gpioBase=240
        ;;
        17|18|19|20|21|22|23|24)
            i2cbus=${NUM_MUX1_CHAN4_DEVICE}
            regAddr=0x21
            dataAddr=0
            eeprombusbase=${NUM_MUX5_CHAN0_DEVICE}
            gpioBase=$((${GPIO_OFFSET}+224-16))
            #gpioBase=$((224 - 16))
        ;;
        25|26|27|28|29|30|31|32)
            i2cbus=${NUM_MUX1_CHAN4_DEVICE}
            regAddr=0x21
            dataAddr=1
            eeprombusbase=${NUM_MUX6_CHAN0_DEVICE}
            gpioBase=$((${GPIO_OFFSET}+224-16))
            #gpioBase=$((224 - 16))
        ;;
        #33)
        #    i2cbus=${NUM_MUX1_CHAN7_DEVICE}
        #    regAddr=0x22
        #    dataAddr=0
        #    gpioBase=${GPIO_OFFSET}
        #;;
        #34)
        #    i2cbus=${NUM_MUX1_CHAN7_DEVICE}
        #    regAddr=0x22
        #    dataAddr=1
        #    gpioBase=${GPIO_OFFSET}
        #;;
        *)
            echo "Please input 1~32"
            exit
        ;;
    esac
}

#Set QSFP Port variable
function _qsfp_eeprom_var_set {
    local port=$1
    eeprombusidx=$(( ${port} % 8))
    case $eeprombusidx in
        1)
          eeprombus=$(( $eeprombusbase + 1 ))
          eepromAddr=0x50
          ;;
        2)
          eeprombus=$(( $eeprombusbase + 0 ))
          eepromAddr=0x50
          ;;
        3)
          eeprombus=$(( $eeprombusbase + 3 ))
          eepromAddr=0x50
          ;;
        4)
          eeprombus=$(( $eeprombusbase + 2 ))
          eepromAddr=0x50
          ;;
        5)
          eeprombus=$(( $eeprombusbase + 5 ))
          eepromAddr=0x50
          ;;
        6)
          eeprombus=$(( $eeprombusbase + 4 ))
          eepromAddr=0x50
          ;;
        7)
          eeprombus=$(( $eeprombusbase + 7 ))
          eepromAddr=0x50
          ;;
        0)
          eeprombus=$(( $eeprombusbase + 6 ))
          eepromAddr=0x50
          ;;
    esac
}

#Get QSFP EEPROM Information
function _i2c_qsfp_eeprom_get {

    _qsfp_port_i2c_var_set ${QSFP_PORT}

    #status: 0 -> Down, 1 -> Up
    status=`cat /sys/class/gpio/gpio$(( $(($gpioBase + (${QSFP_PORT} - 1) ^ 1)) ))/value`
    echo $status

    if [ $status = 0 ]; then
        exit
    fi

    _qsfp_eeprom_var_set ${QSFP_PORT}

    cat ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/eeprom | hexdump -C
}

#Init QSFP EEPROM
function _i2c_qsfp_eeprom_init {
    echo -n "QSFP EEPROM INIT..."

    #Action check
    action=$1
    if [ -z "${action}" ]; then
        echo "No action, skip"
        return
    elif [ "${action}" != "new" ] && [ "${action}" != "delete" ]; then
        echo "Error action, skip"
        return
    fi

    #Init 1-32 ports EEPROM
    local i
    for i in {1..32};
    do
        _qsfp_port_i2c_var_set ${i}

        _qsfp_eeprom_var_set ${i}

        if [ "${action}" == "new" ] && \
           ! [ -L ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr) ]; then
            echo "sff8436 $eepromAddr" > ${PATH_SYS_I2C_DEVICES}/i2c-$eeprombus/new_device
        elif [ "${action}" == "delete" ] && \
             [ -L ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr) ]; then
            echo "$eepromAddr" > ${PATH_SYS_I2C_DEVICES}/i2c-$eeprombus/delete_device
        fi
    done
    echo "DONE"
}

#Init Main Board EEPROM
function _i2c_mb_eeprom_init {
    echo -n "Main Board EEPROM INIT..."

    #Action check
    action=$1
    if [ -z "${action}" ]; then
        echo "No action, skip"
        return
    elif [ "${action}" != "new" ] && [ "${action}" != "delete" ]; then
        echo "Error action, skip"
        return
    fi

    #Init 1-32 ports EEPROM
    if [ "${action}" == "new" ] && \
           ! [ -L ${PATH_SYS_I2C_DEVICES}/${NUM_MUX1_CHAN7_DEVICE}-0054 ]; then
        echo "mb_eeprom 0x54" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN7_DEVICE}/new_device
    elif [ "${action}" == "delete" ] && \
           [ -L ${PATH_SYS_I2C_DEVICES}/${NUM_MUX1_CHAN7_DEVICE}-0054 ]; then
        echo "0x54" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN7_DEVICE}/delete_device
    fi
    echo "DONE"
}

#PSU EEPROM init
function _i2c_psu_eeprom_init {
    echo "========================================================="
    echo "# Description: I2C PSU EEPROM Init..."
    echo "========================================================="

    ## modprobe eeprom
    modprobe eeprom
    ## PUS(0) EEPROM
    echo "eeprom 0x50" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN6_DEVICE}/new_device

    ## PUS(1) EEPROM
    echo "eeprom 0x50" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN7_DEVICE}/new_device

    echo "done..."
}

#get QSFP Status
function _i2c_qsfp_status_get {

    _qsfp_port_i2c_var_set ${QSFP_PORT}

    #status: 0 -> Down, 1 -> Up
    status=`cat /sys/class/gpio/gpio$(( $(($gpioBase + (${QSFP_PORT} - 1) ^ 1)) ))/value`
    echo "status=$status"
}

#get QSFP Type
function _i2c_qsfp_type_get {

    _qsfp_port_i2c_var_set ${QSFP_PORT}

    _qsfp_eeprom_var_set ${QSFP_PORT}

    #Get QSFP EEPROM info
    qsfp_info=$(base64 ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/eeprom)

    identifier=$(echo $qsfp_info | base64 -d -i | hexdump -s 128 -n 1 -e '"%x"')
    connector=$(echo $qsfp_info | base64 -d -i | hexdump -s 130 -n 1 -e '"%x"')
    transceiver=$(echo $qsfp_info | base64 -d -i | hexdump -s 131 -n 1 -e '"%x"')

    echo "identifier=$identifier"
    echo "connector=$connector"
    echo "transceiver=$transceiver"
}

#Get PSU EEPROM Information
function _i2c_psu_eeprom_get {
    echo "========================================================="
    echo "# Description: I2C PSU EEPROM Get..."
    echo "========================================================="

    ## modprobe eeprom
    modprobe eeprom
    ## PUS(0) EEPROM
    cat ${PATH_SYS_I2C_DEVICES}/${NUM_MUX1_CHAN6_DEVICE}-0050/eeprom | hexdump -C

    ## PUS(1) EEPROM
    cat ${PATH_SYS_I2C_DEVICES}/${NUM_MUX1_CHAN7_DEVICE}-0050/eeprom | hexdump -C

    echo "done..."
}

#Get MotherBoard EEPROM Information
function _i2c_mb_eeprom_get {
    echo "========================================================="
    echo "# Description: I2C MB EEPROM Get..."
    echo "========================================================="

    ## MB EEPROM
    cat ${PATH_SYS_I2C_DEVICES}/${NUM_MUX1_CHAN7_DEVICE}-0054/eeprom | hexdump -C
    echo "done..."
}

#Set System Status LED
function _i2c_sys_led {
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x80 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x80 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x40 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x40 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    echo "done..."
}

#Set PSU2 LED
function _i2c_psu2_led {
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x20 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x20 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x10 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x10 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    echo "done..."
}

#Set FAN Tray LED
function _i2c_fan_tray_led {
    case ${FAN_TRAY} in
        1)
            i2cAddr=0x20
            ioPort=2
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x01
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x02
            fi
            ;;
        2)
            i2cAddr=0x20
            ioPort=2
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x10
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x20
            fi
            ;;
        3)
            i2cAddr=0x20
            ioPort=3
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x01
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x02
            fi
            ;;
        4)
            i2cAddr=0x20
            ioPort=3
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x10
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x20
            fi
            ;;
        *)
            echo "Please input 1~4"
            exit
        ;;
    esac

    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m $mask -y -r ${NUM_MUX1_CHAN7_DEVICE} $i2cAddr $ioPort 0x33
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m $mask -y -r ${NUM_MUX1_CHAN7_DEVICE} $i2cAddr $ioPort 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m $mask -y -r ${NUM_MUX1_CHAN7_DEVICE} $i2cAddr $ioPort 0x33
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m $mask -y -r ${NUM_MUX1_CHAN7_DEVICE} $i2cAddr $ioPort 0x00
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    echo "done..."
}

#Set FAN LED
function _i2c_fan_led {
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x08 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x08 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x04 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x04 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    echo "done..."
}

#Set PSU1 LED
function _i2c_psu1_led {
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x02 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x02 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x01 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x01 -y -r ${NUM_MUX1_CHAN7_DEVICE} 0x22 2 0xFF
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    echo "done..."
}

#Get Board Version and Type
function _i2c_board_type_get {
    boardType=`cat ${PATH_SYS_I2C_DEVICES}/0-0033/cpld_board_type`
    boardBuildRev=$((($boardType) & 0x03))
    boardHwRev=$((($boardType) >> 2 & 0x03))
    boardId=$((($boardType) >> 4))
    printf "BOARD_ID is 0x%02x, HW Rev %d, Build Rev %d\n" $boardId $boardHwRev $boardBuildRev

}

#Get CPLD Version
function _i2c_cpld_version {
    cpldRev=`cat ${PATH_SYS_I2C_DEVICES}/0-0033/cpld_version`
    cpldRelease=$((($cpldRev) >> 6 & 0x01))
    cpldVersion=$((($cpldRev) & 0x3F))
    printf "CPLD is %s version(0:RD 1:Release), Revision is 0x%02x\n" $cpldRelease $cpldVersion

}

#Get PSU Status
function _i2c_psu_status {
    psuPresent=`cat ${PATH_SYS_I2C_DEVICES}/0-0033/cpld_pw_abs`
    psu1Exist=$(($((($psuPresent) & 0x01))?0:1))
    psu2Exist=$(($((($psuPresent) & 0x02))?0:1))
    psuPwGood=`cat ${PATH_SYS_I2C_DEVICES}/0-0033/cpld_pw_good`
    psu1PwGood=$(($((($psuPwGood) >> 3 & 0x01))?1:0))
    psu2PwGood=$(($((($psuPwGood) >> 3 & 0x02))?1:0))
    printf "PSU1 Exist:%d PSU1 PW Good:%d\n" $psu1Exist $psu1PwGood
    printf "PSU2 Exist:%d PSU2 PW Good:%d\n" $psu2Exist $psu2PwGood
}

#Get Front Sensor Temperature
function _i2c_front_temp {
    #Front MAC
    sensors | grep 'Front MAC Temp' -A 1
}

#Get Rear Sensor Temperature
function _i2c_rear_temp {
    #Rear MAC
    sensors | grep 'Rear MAC Temp' -A 1
}

#Increase read socket buffer for CoPP Test
function _config_rmem {
    echo "109430400" > /proc/sys/net/core/rmem_max
}

#Main Function
function _main {
    tart_time_str=`date`
    start_time_sec=$(date +%s)

    _set_gpio_offset
    if [ "${EXEC_FUNC}" == "help" ]; then
        _help
    elif [ "${EXEC_FUNC}" == "i2c_init" ]; then
        _i2c_init
    elif [ "${EXEC_FUNC}" == "i2c_deinit" ]; then
        _i2c_deinit
    elif [ "${EXEC_FUNC}" == "i2c_temp_init" ]; then
        _i2c_temp_init
    elif [ "${EXEC_FUNC}" == "i2c_fan_init" ]; then
        _i2c_fan_init
    elif [ "${EXEC_FUNC}" == "i2c_volmon_init" ]; then
        _i2c_volmon_init
    elif [ "${EXEC_FUNC}" == "i2c_io_exp_init" ]; then
        _i2c_io_exp_init
    elif [ "${EXEC_FUNC}" == "i2c_gpio_init" ]; then
        _i2c_gpio_init
    elif [ "${EXEC_FUNC}" == "i2c_gpio_deinit" ]; then
        _i2c_gpio_deinit
    elif [ "${EXEC_FUNC}" == "i2c_led_test" ]; then
        _i2c_led_test
    elif [ "${EXEC_FUNC}" == "i2c_mb_eeprom_get" ]; then
        _i2c_mb_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_psu_eeprom_get" ]; then
        _i2c_psu_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_eeprom_get" ]; then
        _i2c_qsfp_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_eeprom_init" ]; then
        _i2c_qsfp_eeprom_init ${QSFP_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_mb_eeprom_init" ]; then
        _i2c_mb_eeprom_init ${MB_EEPROM_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_status_get" ]; then
        _i2c_qsfp_status_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_type_get" ]; then
        _i2c_qsfp_type_get
    elif [ "${EXEC_FUNC}" == "i2c_led_psu_status_set" ]; then
        _i2c_led_psu_status_set
    elif [ "${EXEC_FUNC}" == "i2c_led_fan_status_set" ]; then
        _i2c_led_fan_status_set
    elif [ "${EXEC_FUNC}" == "i2c_led_fan_tray_status_set" ]; then
        _i2c_led_fan_tray_status_set
    elif [ "${EXEC_FUNC}" == "i2c_sys_led" ]; then
        _i2c_sys_led
    elif [ "${EXEC_FUNC}" == "i2c_fan_led" ]; then
        _i2c_fan_led
    elif [ "${EXEC_FUNC}" == "i2c_fan_tray_led" ]; then
        _i2c_fan_tray_led
    elif [ "${EXEC_FUNC}" == "i2c_psu1_led" ]; then
        _i2c_psu1_led
    elif [ "${EXEC_FUNC}" == "i2c_psu2_led" ]; then
        _i2c_psu2_led
    elif [ "${EXEC_FUNC}" == "i2c_board_type_get" ]; then
        _i2c_board_type_get
    elif [ "${EXEC_FUNC}" == "i2c_cpld_version" ]; then
        _i2c_cpld_version
    elif [ "${EXEC_FUNC}" == "i2c_psu_status" ]; then
        _i2c_psu_status
    elif [ "${EXEC_FUNC}" == "i2c_front_temp" ]; then
        _i2c_front_temp
    elif [ "${EXEC_FUNC}" == "i2c_rear_temp" ]; then
        _i2c_rear_temp
    elif [ "${EXEC_FUNC}" == "i2c_test_all" ]; then
        _i2c_init
        _i2c_temp_init
        _i2c_fan_init
        _i2c_io_exp_init
        _i2c_led_test
        _i2c_psu_eeprom_get
        _i2c_mb_eeprom_get
        _i2c_board_type_get
        _i2c_cpld_version
        _i2c_psu_status
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    if [ "$DEBUG" == "on" ]; then
        echo "-----------------------------------------------------"
        end_time_str=`date`
        end_time_sec=$(date +%s)
        diff_time=$[ ${end_time_sec} - ${start_time_sec} ]
        echo "Start Time: ${start_time_str} (${start_time_sec})"
        echo "End Time  : ${end_time_str} (${end_time_sec})"
        echo "Total Execution Time: ${diff_time} sec"

        echo "done!!!"
    fi
}

_main
