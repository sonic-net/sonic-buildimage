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

NUM_I801_DEVICE=0
NUM_ISMT_DEVICE=$(( ${NUM_I801_DEVICE} + 1 ))

#MUX PCA9548#0
NUM_MUX_9548_0_CHAN0=$(( ${NUM_I801_DEVICE} + 2 ))
NUM_MUX_9548_0_CHAN1=$(( ${NUM_I801_DEVICE} + 3 ))
NUM_MUX_9548_0_CHAN2=$(( ${NUM_I801_DEVICE} + 4 ))
#NUM_MUX_9548_0_CHAN3=$(( ${NUM_I801_DEVICE} + 5 ))
#NUM_MUX_9548_0_CHAN4=$(( ${NUM_I801_DEVICE} + 6 ))
#NUM_MUX_9548_0_CHAN5=$(( ${NUM_I801_DEVICE} + 7 ))
#NUM_MUX_9548_0_CHAN6=$(( ${NUM_I801_DEVICE} + 8 ))
#NUM_MUX_9548_0_CHAN7=$(( ${NUM_I801_DEVICE} + 9 ))

#MUX PCA9548#1
NUM_MUX_9548_1_CHAN0=$(( ${NUM_I801_DEVICE} + 10 ))
NUM_MUX_9548_1_CHAN1=$(( ${NUM_I801_DEVICE} + 11 ))
NUM_MUX_9548_1_CHAN2=$(( ${NUM_I801_DEVICE} + 12 ))
NUM_MUX_9548_1_CHAN3=$(( ${NUM_I801_DEVICE} + 13 ))
NUM_MUX_9548_1_CHAN4=$(( ${NUM_I801_DEVICE} + 14 ))
NUM_MUX_9548_1_CHAN5=$(( ${NUM_I801_DEVICE} + 15 ))
NUM_MUX_9548_1_CHAN6=$(( ${NUM_I801_DEVICE} + 16 ))
NUM_MUX_9548_1_CHAN7=$(( ${NUM_I801_DEVICE} + 17 ))

NUM_MUX_9548_2_CHAN0=$(( ${NUM_I801_DEVICE} + 18 ))
NUM_MUX_9548_3_CHAN0=$(( ${NUM_I801_DEVICE} + 26 ))
NUM_MUX_9548_4_CHAN0=$(( ${NUM_I801_DEVICE} + 34 ))
NUM_MUX_9548_5_CHAN0=$(( ${NUM_I801_DEVICE} + 42 ))

#MUX Alias
I2C_BUS_PSU1_EEPROM=${NUM_MUX_9548_0_CHAN1}
I2C_BUS_PSU2_EEPROM=${NUM_MUX_9548_0_CHAN2}
I2C_BUS_FAN_STATUS=${NUM_MUX_9548_1_CHAN0}
I2C_BUS_LED_BOARD=${NUM_MUX_9548_1_CHAN0}
I2C_BUS_MB_EEPROM=${NUM_MUX_9548_1_CHAN2}

PATH_SYS_I2C_DEVICES="/sys/bus/i2c/devices"
PATH_SYS_GPIO="/sys/class/gpio"
PATH_HWMON_ROOT_DEVICES="/sys/class/hwmon"
PATH_HWMON_W83795_DEVICE="${PATH_HWMON_ROOT_DEVICES}/hwmon1"
PATH_I801_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_I801_DEVICE}"
PATH_ISMT_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_ISMT_DEVICE}"
#PATH for MUX PCA9548#0
PATH_MUX_9548_0_CHAN0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CHAN0}"
PATH_MUX_9548_0_CHAN1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CHAN1}"
PATH_MUX_9548_0_CHAN2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CHAN2}"
#PATH for MUX PCA9548#1
PATH_MUX_9548_1_CHAN0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN0}"
PATH_MUX_9548_1_CHAN1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}"
PATH_MUX_9548_1_CHAN2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN2}"
PATH_MUX_9548_1_CHAN3="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN3}"
PATH_MUX_9548_1_CHAN4="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN4}"
PATH_MUX_9548_1_CHAN5="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN5}"
PATH_MUX_9548_1_CHAN6="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN6}"
PATH_MUX_9548_1_CHAN7="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN7}"

#i2cmap and gpiomap path
PATH_GPIOMAP=/tmp/gpiomap
PATH_I2CMAP=/tmp/i2cmap

I2C_ADDR_MUX_9548_0=0x70
I2C_ADDR_MUX_9548_1=0x70
I2C_ADDR_MUX_9548_2=0x71
I2C_ADDR_MUX_9548_3=0x71
I2C_ADDR_MUX_9548_4=0x71
I2C_ADDR_MUX_9548_5=0x71
I2C_ADDR_DDR3=0x50
I2C_ADDR_RTC=0x68
I2C_ADDR_CPLD=0x33
I2C_ADDR_HWM=0x2F
I2C_ADDR_MUX_9535_FAN=0x20
I2C_ADDR_MUX_9535_LED=0x22
I2C_ADDR_MUX_9535_0=0x20
I2C_ADDR_MUX_9535_1=0x21
I2C_ADDR_MUX_9535_2=0x22
I2C_ADDR_MUX_9535_3=0x23
I2C_ADDR_MUX_9535_4=0x20
I2C_ADDR_MUX_9535_5=0x21
I2C_ADDR_MUX_9535_6=0x22
I2C_ADDR_MUX_9535_7=0x23
I2C_ADDR_MUX_9535_8=0x20
I2C_ADDR_MUX_9535_9=0x21
I2C_ADDR_MUX_9535_10=0x22
I2C_ADDR_MB_EEPROM=0x56
I2C_ADDR_QSFP_EEPROM=0x50
I2C_ADDR_PSU_EEPROM=0x51

#sysfs
PATH_SYSFS_PSU1="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_PSU1_EEPROM}-$(printf "%04x" $I2C_ADDR_PSU_EEPROM)"
PATH_SYSFS_PSU2="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_PSU2_EEPROM}-$(printf "%04x" $I2C_ADDR_PSU_EEPROM)"

#Active High/Low
ACTIVE_LOW=1
ACTIVE_HIGH=0
#GPIO Direction In/Out
DIR_IN=in
DIR_OUT=out

#Power Supply Status
PSU_DC_ON=1
PSU_DC_OFF=0
PSU_EXIST=1
PSU_NOT_EXIST=0

#W83795 Registers
REG_BANK_SEL=0x00
REG_VOLT_CTRL1=0x02
REG_VOLT_CTRL2=0x03
REG_TEMP_CTRL1=0x04
REG_TEMP_CTRL2=0x05

#PCA9535 Registers
REG_IN_0=0
REG_IN_1=1
REG_OUT_0=2
REG_OUT_1=3
REG_POLARITY_0=4
REG_POLARITY_1=5
REG_CFG_0=6
REG_CFG_1=7

#Bit Mask
BIT_MASK=(1 2 4 8 16 32 64 128)

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
    echo "         : ${0} i2c_led_fan_tray_test"
    echo "         : ${0} i2c_cpld_version"
    echo "         : ${0} i2c_test_all"
    echo "         : ${0} i2c_led_test"
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

#set i2cmap
function _set_i2cmap {
    local i2c_n=$1
    local alias=$2

    #create i2cmap dir if not exist
    mkdir -p $PATH_I2CMAP

    #check i2c_n exists in sysfs
    if [ ! -L ${PATH_SYS_I2C_DEVICES}/i2c-${i2c_n} ]; then
        echo "${PATH_SYS_I2C_DEVICES}/i2c-${i2c_n} does not exist."
        return
    fi

    #create or update link
    ln -sf ${PATH_SYS_I2C_DEVICES}/i2c-${i2c_n} ${PATH_I2CMAP}/${alias}
}

#clear i2cmap
function _clear_i2cmap {
    #delete i2cmap dir
    rm -rf ${PATH_I2CMAP}/
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

    #add MUX PCA9548#0 on I801
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CHAN0}" ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_0}' > ${PATH_I801_DEVICE}/new_device"
        _set_i2cmap ${NUM_MUX_9548_0_CHAN0} "PCA9548_0"
    else
        echo "pca9548#0 ${I2C_ADDR_MUX_9548_0} already init."
    fi
    #add MUX PCA9548#1 on ISMT
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN0}" ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_1}' > ${PATH_ISMT_DEVICE}/new_device"
        _set_i2cmap ${NUM_MUX_9548_1_CHAN0} "PCA9548_1"
    else
        echo "pca9548#1 ${I2C_ADDR_MUX_9548_1} already init."
    fi
    #add MUX PCA9548#2 on PCA9548#1
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CHAN0}" ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_2}' > ${PATH_MUX_9548_1_CHAN4}/new_device"
        _set_i2cmap ${NUM_MUX_9548_2_CHAN0} "PCA9548_2"
    else
        echo "pca9548#2 ${I2C_ADDR_MUX_9548_2} already init."
    fi
    #add MUX PCA9548#3 on PCA9548#1
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_3_CHAN0}" ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_3}' > ${PATH_MUX_9548_1_CHAN5}/new_device"
        _set_i2cmap ${NUM_MUX_9548_3_CHAN0} "PCA9548_3"
    else
        echo "pca9548#3 ${I2C_ADDR_MUX_9548_3} already init."
    fi
    #add MUX PCA9548#4 on PCA9548#1
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_4_CHAN0}" ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_4}' > ${PATH_MUX_9548_1_CHAN6}/new_device"
        _set_i2cmap ${NUM_MUX_9548_4_CHAN0} "PCA9548_4"
    else
        echo "pca9548#4 ${I2C_ADDR_MUX_9548_4} already init."
    fi
    #add MUX PCA9548#5 on PCA9548#1
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_5_CHAN0}" ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_5}' > ${PATH_MUX_9548_1_CHAN7}/new_device"
        _set_i2cmap ${NUM_MUX_9548_5_CHAN0} "PCA9548_5"
    else
        echo "pca9548#5 ${I2C_ADDR_MUX_9548_5} already init."
    fi
    #Init CPLD LED_CLR Register (Front Port LED)
    i2cset -y ${NUM_I801_DEVICE} ${I2C_ADDR_CPLD} 0x34 0x10

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
    _i2c_psu_init
    _i2c_qsfp_eeprom_init "new"
    _i2c_mb_eeprom_init "new"
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
    for mod in coretemp jc42 w83795 eeprom_mb gpio-pca953x i2c_mux_pca954x i2c_ismt i2c_i801 ingrasys_s8810_32q_psu;
    do
        [ "$(lsmod | grep "^$mod ")" != "" ] && rmmod $mod
    done
    _clear_i2cmap
}

#Temperature sensor Init
function _i2c_temp_init {
    echo -n "TEMP INIT..."
    #select bank0
    i2cset -y -r ${NUM_I801_DEVICE} ${I2C_ADDR_HWM} ${REG_BANK_SEL} 0x80
    #enable TR1, TR2, TR3 temperature monitoring
    i2cset -y -r ${NUM_I801_DEVICE} ${I2C_ADDR_HWM} ${REG_TEMP_CTRL2} 0x7F

    # CLKIN clock frequency set as 48Mhz
    i2cset -y -r ${NUM_I801_DEVICE} ${I2C_ADDR_HWM} 0x01 0x1C
}

#FAN Init
function _i2c_fan_init {

    local init_fan_speed=120
    echo -n "FAN INIT..."
    if [ -e "${PATH_HWMON_W83795_DEVICE}" ]; then
        #Init Fan Speed
        echo $init_fan_speed > ${PATH_HWMON_W83795_DEVICE}/device/pwm1
        echo $init_fan_speed > ${PATH_HWMON_W83795_DEVICE}/device/pwm2
        echo "SUCCESS"
    else
        echo "FAIL, W83795 not found in path ${PATH_HWMON_W83795_DEVICE}"
    fi
}

#VOLMON Init
function _i2c_volmon_init {
    #3.3V 3VDD
    #VSEN1 ROV
    #VSEN4 1V
    #VSEN5 1.8V
    #VSEN7 5V
    echo -n "VOLMON INIT..."
    #select bank0
    i2cset -y -r ${NUM_I801_DEVICE} ${I2C_ADDR_HWM} ${REG_BANK_SEL} 0x80
    #enable voltage monitoring VSEN1-8
    i2cset -y -r ${NUM_I801_DEVICE} ${I2C_ADDR_HWM} ${REG_VOLT_CTRL1} 0xFF
    #enable voltage monitoring 3VDD and 3VBAT
    i2cset -y -r ${NUM_I801_DEVICE} ${I2C_ADDR_HWM} ${REG_VOLT_CTRL2} 0x50
    #disable voltage monitoring VSEN12 and VSEN13
    i2cset -y -r ${NUM_I801_DEVICE} ${I2C_ADDR_HWM} ${REG_TEMP_CTRL1} 0x00
    echo "Done"
}

#IO Expander Init
function _i2c_io_exp_init {
    echo "========================================================="
    echo "# Description: I2C IO Expander Init"
    echo "========================================================="

    #SMBUS1

    echo "Init FAN Status IO Expander"
    #PCA9535_FAN FAN_STATUS
    #command byte 2/3,  output logic level is 0
    #command byte 4/5,  polarity is not inverted
    #command byte 6/7,  direction is input
    i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_FAN} ${REG_OUT_0} 0x11
    i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_FAN} ${REG_OUT_1} 0x11
    i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_FAN} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_FAN} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_FAN} ${REG_CFG_0} 0xCC
    i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_FAN} ${REG_CFG_1} 0xCC

    echo "Init LED Status IO Expander"
    #PCA9535_LED LED_BOARD
    #command byte 2/3,  output logic level is 0
    #command byte 4/5,  polarity is not inverted
    #command byte 6/7,  direction is output
    i2cset -y -r ${I2C_BUS_LED_BOARD} ${I2C_ADDR_MUX_9535_LED} ${REG_OUT_0} 0xFF #active low
    i2cset -y -r ${I2C_BUS_LED_BOARD} ${I2C_ADDR_MUX_9535_LED} ${REG_OUT_1} 0xFF #active low
    i2cset -y -r ${I2C_BUS_LED_BOARD} ${I2C_ADDR_MUX_9535_LED} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${I2C_BUS_LED_BOARD} ${I2C_ADDR_MUX_9535_LED} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${I2C_BUS_LED_BOARD} ${I2C_ADDR_MUX_9535_LED} ${REG_CFG_0} 0x00
    i2cset -y -r ${I2C_BUS_LED_BOARD} ${I2C_ADDR_MUX_9535_LED} ${REG_CFG_1} 0x00

    #PCA9535#0-10 zQSFP

    echo "Init ZQSFP IO Expander"

    echo "set ZQSFP ABS"
    #zQSFP 0-31 ABS
    #command byte 4/5,  polarity is not inverted
    #command byte 6/7,  direction is input
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_0} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_0} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_0} ${REG_CFG_0} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_0} ${REG_CFG_1} 0xFF

    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_1} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_1} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_1} ${REG_CFG_0} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_1} ${REG_CFG_1} 0xFF

    echo "set ZQSFP INT"
    #zQSFP 0-31 INT
    #command byte 4/5,  polarity is not inverted
    #command byte 6/7,  direction is input
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_2} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_2} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_2} ${REG_CFG_0} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_2} ${REG_CFG_1} 0xFF

    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_3} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_3} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_3} ${REG_CFG_0} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN1} ${I2C_ADDR_MUX_9535_3} ${REG_CFG_1} 0xFF

    echo "set ZQSFP LP_MODE"
    #zQSFP 0-31 LP_MODE
    #command byte 2/3,  output logic level is 0
    #command byte 4/5,  polarity is not inverted
    #command byte 6/7,  direction is output
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_4} ${REG_OUT_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_4} ${REG_OUT_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_4} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_4} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_4} ${REG_CFG_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_4} ${REG_CFG_1} 0x00

    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_5} ${REG_OUT_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_5} ${REG_OUT_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_5} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_5} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_5} ${REG_CFG_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_5} ${REG_CFG_1} 0x00

    echo "set ZQSFP RST"
    #zQSFP 0-31 RST
    #command byte 2/3,  output logic level is 0
    #command byte 4/5,  polarity is not inverted
    #command byte 6/7,  direction is output
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_6} ${REG_OUT_0} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_6} ${REG_OUT_1} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_6} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_6} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_6} ${REG_CFG_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_6} ${REG_CFG_1} 0x00

    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_7} ${REG_OUT_0} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_7} ${REG_OUT_1} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_7} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_7} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_7} ${REG_CFG_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN2} ${I2C_ADDR_MUX_9535_7} ${REG_CFG_1} 0x00

    echo "set ZQSFP Mode Select"
    #zQSFP 0-31 Mode Select
    #command byte 2/3,  output logic level is 0
    #command byte 4/5,  polarity is not inverted
    #command byte 6/7,  direction is output
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_8} ${REG_OUT_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_8} ${REG_OUT_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_8} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_8} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_8} ${REG_CFG_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_8} ${REG_CFG_1} 0x00

    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_9} ${REG_OUT_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_9} ${REG_OUT_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_9} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_9} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_9} ${REG_CFG_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_9} ${REG_CFG_1} 0x00

    #ZQSFP ABS0_15, ABS16_31, INT0_15, INT16_31, PSU1_PWROFF, PSU2_PWROFF
    #command byte 2/3,  output logic level is 0
    #command byte 4/5,  polarity is not inverted
    #command byte 6/7,  I/O 1.0 and I/O 1.1 are output, others are input
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_10} ${REG_OUT_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_10} ${REG_OUT_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_10} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_10} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_10} ${REG_CFG_0} 0xFF
    i2cset -y -r ${NUM_MUX_9548_1_CHAN3} ${I2C_ADDR_MUX_9535_10} ${REG_CFG_1} 0xFC
}

#GPIO Init Utility Function
function _gpio_export {
    local gpio_n=$1
    local direction=$2
    local active_low=$3
    local value=$4

    if [ -z "${gpio_n}" ]; then
        echo "[_gpio_init]  gpio_n(${gpio_n}) is not provided"
        return
    fi
    if [ "${gpio_n}" -lt "0" ] || [ "${gpio_n}" -gt "255" ]; then
        echo "[_gpio_init]  gpio_n(${gpio_n}) is invalid value"
        return
    fi

    #export gpio
    echo ${gpio_n} > ${PATH_SYS_GPIO}/export

    #set gpio direction
    echo ${direction} > ${PATH_SYS_GPIO}/gpio${gpio_n}/direction

    #set gpio active_low
    echo ${active_low} > ${PATH_SYS_GPIO}/gpio${gpio_n}/active_low

    #set value
    if [ ! -z "${value}" ]; then
        echo ${value} > ${PATH_SYS_GPIO}/gpio${gpio_n}/value
    fi
}

#set gpiomap
function _set_gpiomap {
    local gpio_n=$1
    local alias=$2

    #create gpiomap dir if not exist
    mkdir -p $PATH_GPIOMAP

    #check gpio_n exists in sysfs
    if [ ! -L ${PATH_SYS_GPIO}/gpio${gpio_n} ]; then
        echo "${PATH_SYS_GPIO}/gpio${gpio_n} does not exist."
        return
    fi

    #create or update link
    ln -sf ${PATH_SYS_GPIO}/gpio${gpio_n} ${PATH_GPIOMAP}/${alias}
}

#clear gpiomap
function _clear_gpiomap {
    #delete gpiomap dir
    rm -rf ${PATH_GPIOMAP}
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
    local i=0

    #ABS Port 0-31
    echo "pca9535 ${I2C_ADDR_MUX_9535_1}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}/new_device
    _set_gpio_offset
    echo "pca9535 ${I2C_ADDR_MUX_9535_0}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}/new_device
    #for i in {224..255}
    for((i=${GPIO_OFFSET}+224;i<=${GPIO_OFFSET}+255;i++));
    do
        _gpio_export ${i} ${DIR_IN} ${ACTIVE_LOW}
        _set_gpiomap ${i} "QSFP$(( i - 223 ))_ABS"
    done

    #INT Port 0-31
    echo "pca9535 ${I2C_ADDR_MUX_9535_3}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}/new_device
    echo "pca9535 ${I2C_ADDR_MUX_9535_2}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}/new_device
    #for i in {192..223}
    for((i=${GPIO_OFFSET}+192;i<=${GPIO_OFFSET}+223;i++));
    do
        _gpio_export ${i} ${DIR_IN} ${ACTIVE_LOW}
        _set_gpiomap ${i} "QSFP$(( i - 191 ))_INT"
    done

    #LP Mode Port 0-31
    echo "pca9535 ${I2C_ADDR_MUX_9535_5}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN2}/new_device
    echo "pca9535 ${I2C_ADDR_MUX_9535_4}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN2}/new_device
    #for i in {160..191}
    for((i=${GPIO_OFFSET}+160;i<=${GPIO_OFFSET}+191;i++));
    do
        _gpio_export ${i} ${DIR_OUT} ${ACTIVE_HIGH}
        _set_gpiomap ${i} "QSFP$(( i - 159 ))_LPMODE"
    done

    #RST Port 0-31
    echo "pca9535 ${I2C_ADDR_MUX_9535_7}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN2}/new_device
    echo "pca9535 ${I2C_ADDR_MUX_9535_6}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN2}/new_device
    #for i in {128..159}
    for((i=${GPIO_OFFSET}+128;i<=${GPIO_OFFSET}+159;i++));
    do
        _gpio_export ${i} ${DIR_OUT} ${ACTIVE_LOW}
        _set_gpiomap ${i} "QSFP$(( i - 127 ))_RST"
        #Reset QSFP
        echo "${ACTIVE_LOW}" > ${PATH_SYS_GPIO}/gpio${i}/value
        echo "${ACTIVE_HIGH}" > ${PATH_SYS_GPIO}/gpio${i}/value
    done

    #MODSEL Port 0-31
    echo "pca9535 ${I2C_ADDR_MUX_9535_9}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN3}/new_device
    echo "pca9535 ${I2C_ADDR_MUX_9535_8}" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN3}/new_device
    #for i in {96..127}
    for((i=${GPIO_OFFSET}+96;i<=${GPIO_OFFSET}+127;i++));
    do
        _gpio_export ${i} ${DIR_OUT} ${ACTIVE_LOW}
        _set_gpiomap ${i} "QSFP$(( i - 95 ))_MODSEL"
    done
}

#GPIO DeInit
function _i2c_gpio_deinit {
    echo ${I2C_ADDR_MUX_9535_0} > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}/delete_device
    echo ${I2C_ADDR_MUX_9535_1} > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}/delete_device
    echo ${I2C_ADDR_MUX_9535_2} > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}/delete_device
    echo ${I2C_ADDR_MUX_9535_3} > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN1}/delete_device
    echo ${I2C_ADDR_MUX_9535_4} > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN2}/delete_device
    echo ${I2C_ADDR_MUX_9535_5} > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CHAN2}/delete_device
    _clear_gpiomap
}

#Set FAN Tray LED
function _i2c_led_fan_tray_status_set {
    echo "FAN Tray Status Setup"

    #check W83795 exists in hwmon2
    if [ ! -e "${PATH_HWMON_W83795_DEVICE}" ]; then
        echo "FAIL, W83795 not found in path ${PATH_HWMON_W83795_DEVICE}"
        return
    fi
    #FAN Status get
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`
    FAN2_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan2_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN4_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan4_alarm`
    FAN5_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN6_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan6_alarm`
    FAN7_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`
    FAN8_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan8_alarm`

        FAN_TRAY=1
    if [ "${FAN1_ALARM}" == "0" ] && [ "${FAN2_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    fi

        FAN_TRAY=2
    if [ "${FAN3_ALARM}" == "0" ] && [ "${FAN4_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    fi

        FAN_TRAY=3
    if [ "${FAN5_ALARM}" == "0" ] && [ "${FAN6_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    fi

        FAN_TRAY=4
    if [ "${FAN7_ALARM}" == "0" ] && [ "${FAN8_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
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

#Test FAN Tray LED
function _i2c_led_fan_tray_test {
    echo "FAN Tray LED Test"

    for i in {1..4}
    do
            FAN_TRAY=$i
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        _pause 'Check Fan Tray ${FAN_TRAY} LED green light and Press [Enter] key to continue...'

        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        _pause 'Check Fan Tray ${FAN_TRAY} LED amber light and Press [Enter] key to continue...'
    done
}
#Set FAN LED
function _i2c_led_fan_status_set {
    echo "FAN Status Setup"

    #check W83795 exists in hwmon2
    if [ ! -e "${PATH_HWMON_W83795_DEVICE}" ]; then
        echo "FAIL, W83795 not found in path ${PATH_HWMON_W83795_DEVICE}"
        return
    fi

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
    _i2c_reset_led
    COLOR_LED="green"
    ONOFF_LED="on"
    _i2c_sys_led
    _pause 'Check SYS LED green light and Press [Enter] key to continue...'
    #sys led (amber)
    _i2c_reset_led
    COLOR_LED="amber"
    ONOFF_LED="on"
    _i2c_sys_led
    _pause 'Check SYS LED amber light and Press [Enter] key to continue...'

    #FAN led (green)
    _i2c_reset_led
    COLOR_LED="green"
    ONOFF_LED="on"
    _i2c_fan_led
    _pause 'Check FAN LED green light and Press [Enter] key to continue...'
    #FAN led (amber)
    _i2c_reset_led
    COLOR_LED="amber"
    ONOFF_LED="on"
    _i2c_fan_led
    _pause 'Check FAN LED amber light and Press [Enter] key to continue...'

    #PSU2 led (green)
    _i2c_reset_led
    COLOR_LED="green"
    ONOFF_LED="on"
    _i2c_psu2_led
    _pause 'Check PSU2 LED green light and Press [Enter] key to continue...'
    #PSU2 led (amber)
    _i2c_reset_led
    COLOR_LED="amber"
    ONOFF_LED="on"
    _i2c_psu2_led
    _pause 'Check PSU2 LED amber light and Press [Enter] key to continue...'

    #PSU1 led (green)
    _i2c_reset_led
    COLOR_LED="green"
    ONOFF_LED="on"
    _i2c_psu1_led
    _pause 'Check PSU1 LED green light and Press [Enter] key to continue...'
    #PSU1 led (amber)
    _i2c_reset_led
    COLOR_LED="amber"
    ONOFF_LED="on"
    _i2c_psu1_led
    _pause 'Check PSU1 LED amber light and Press [Enter] key to continue...'

    #Turn OFF All LED
    _i2c_reset_led
    _pause 'Check turn off all LEDs and Press [Enter] key to continue...'
    echo "done..."

    #sys led (green)
    COLOR_LED="green"
    ONOFF_LED="on"
    _i2c_sys_led
}

#Set QSFP Port variable
function _qsfp_port_i2c_var_set {
    local port=$1
    case ${port} in
        1|2|3|4|5|6|7|8)
            i2cbus=${NUM_MUX_9548_1_CHAN1}
            regAddr=0x20
            dataAddr=0
            eeprombusbase=${NUM_MUX_9548_2_CHAN0}
            gpioBase=$((${GPIO_OFFSET}+224))
            #gpioBase=224
        ;;
        9|10|11|12|13|14|15|16)
            i2cbus=${NUM_MUX_9548_1_CHAN1}
            regAddr=0x20
            dataAddr=1
            eeprombusbase=${NUM_MUX_9548_3_CHAN0}
            gpioBase=$((${GPIO_OFFSET}+224))
            #gpioBase=224
        ;;
        17|18|19|20|21|22|23|24)
            i2cbus=${NUM_MUX_9548_1_CHAN1}
            regAddr=0x21
            dataAddr=0
            eeprombusbase=${NUM_MUX_9548_4_CHAN0}
            gpioBase=$((${GPIO_OFFSET}+240))
            #gpioBase=240
        ;;
        25|26|27|28|29|30|31|32)
            i2cbus=${NUM_MUX_9548_1_CHAN1}
            regAddr=0x21
            dataAddr=1
            eeprombusbase=${NUM_MUX_9548_5_CHAN0}
            gpioBase=$((${GPIO_OFFSET}+240))
            #gpioBase=240
        ;;
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
          eeprombus=$(( $eeprombusbase + 0 ))
          ;;
        2)
          eeprombus=$(( $eeprombusbase + 1 ))
          ;;
        3)
          eeprombus=$(( $eeprombusbase + 2 ))
          ;;
        4)
          eeprombus=$(( $eeprombusbase + 3 ))
          ;;
        5)
          eeprombus=$(( $eeprombusbase + 4 ))
          ;;
        6)
          eeprombus=$(( $eeprombusbase + 5 ))
          ;;
        7)
          eeprombus=$(( $eeprombusbase + 6 ))
          ;;
        0)
          eeprombus=$(( $eeprombusbase + 7 ))
          ;;
    esac
    eepromAddr=${I2C_ADDR_QSFP_EEPROM}
}

#Get QSFP EEPROM Information
function _i2c_qsfp_eeprom_get {

    _qsfp_port_i2c_var_set ${QSFP_PORT}

    #status: 0 -> Down, 1 -> Up
    status=`cat ${PATH_SYS_GPIO}/gpio$(( gpioBase + (QSFP_PORT-1)%16 ))/value`
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

    #Init MB EEPROM
    if [ "${action}" == "new" ] && \
        ! [ -L ${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MB_EEPROM}-$(printf "%04x" $I2C_ADDR_MB_EEPROM) ]; then
        echo "mb_eeprom ${I2C_ADDR_MB_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_MB_EEPROM}/new_device
    elif [ "${action}" == "delete" ] && \
        [ -L ${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MB_EEPROM}-$(printf "%04x" $I2C_ADDR_MB_EEPROM) ]; then
        echo "${I2C_ADDR_MB_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_MB_EEPROM}/delete_device
    fi
    echo "DONE"
}

#get QSFP Status
function _i2c_qsfp_status_get {

    _qsfp_port_i2c_var_set ${QSFP_PORT}

    #status: 0 -> Down, 1 -> Up

    status=`cat ${PATH_SYS_GPIO}/gpio$(( gpioBase + (QSFP_PORT-1)%16 ))/value`
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

#Init PSU Kernel Module
function _i2c_psu_init {
    echo "========================================================="
    echo "# Description: I2C PSU Init"
    echo "========================================================="
    modprobe ingrasys_s8810_32q_psu

    echo "psu1 ${I2C_ADDR_PSU_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU1_EEPROM}/new_device
    echo "psu2 ${I2C_ADDR_PSU_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU2_EEPROM}/new_device
}

#Get PSU EEPROM Information
function _i2c_psu_eeprom_get {
    local eeprom_psu1=""
    local eeprom_psu2=""

    echo "========================================================="
    echo "# Description: I2C PSU EEPROM Get..."
    echo "========================================================="

    eeprom_psu1="${PATH_SYSFS_PSU1}/psu_eeprom"
    cat ${eeprom_psu1} | hexdump -C

    eeprom_psu2="${PATH_SYSFS_PSU2}/psu_eeprom"
    cat ${eeprom_psu2} | hexdump -C
}

#Get MotherBoard EEPROM Information
function _i2c_mb_eeprom_get {
    echo "========================================================="
    echo "# Description: I2C MB EEPROM Get..."
    echo "========================================================="

    ## MB EEPROM
    cat ${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MB_EEPROM}-$(printf "%04x" $I2C_ADDR_MB_EEPROM)/eeprom | hexdump -C
    echo "done..."
}

#Set FAN Tray LED
function _i2c_fan_tray_led {
    local value=0
    case ${FAN_TRAY} in
        1)
            ioPort=$REG_OUT_0
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x01
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x02
            fi
            ;;
        2)
            ioPort=$REG_OUT_0
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x10
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x20
            fi
            ;;
        3)
            ioPort=$REG_OUT_1
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x01
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x02
            fi
            ;;
        4)
            ioPort=$REG_OUT_1
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

    if [ "${ONOFF_LED}" == "on" ]; then
        value=0xFF
    elif [ "${ONOFF_LED}" == "off" ]; then
        value=0x00
    else
        echo "Invalid Parameters ${ONOFF_LED}, Exit!!!"
        _help
        exit ${FALSE}
    fi
    i2cset -m $mask -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_FAN} $ioPort $value

    #echo "done..."
}

#Set System Status LED
function _i2c_sys_led {
    local green_mask=${BIT_MASK[7]}
    local amber_mask=${BIT_MASK[6]}

    _i2c_set_led $green_mask $amber_mask
}

#Set FAN LED
function _i2c_fan_led {
    local green_mask=${BIT_MASK[3]}
    local amber_mask=${BIT_MASK[2]}

    _i2c_set_led $green_mask $amber_mask
}

#Set PSU1 LED
function _i2c_psu1_led {
    local green_mask=${BIT_MASK[1]}
    local amber_mask=${BIT_MASK[0]}

    _i2c_set_led $green_mask $amber_mask
}

#Set PSU2 LED
function _i2c_psu2_led {
    local green_mask=${BIT_MASK[5]}
    local amber_mask=${BIT_MASK[4]}

    _i2c_set_led $green_mask $amber_mask
}

#Set LEDs in LED Board
function _i2c_set_led {
    local green_mask=$1
    local amber_mask=$2
    local mask=0
    local value=0


    if [ "${COLOR_LED}" == "green" ]; then
        mask=$green_mask
    elif [ "${COLOR_LED}" == "amber" ]; then
        mask=$amber_mask
    else
        echo "Invalid Parameters ${COLOR_LED}, Exit!!!"
        _help
        exit ${FALSE}
    fi

    # Singals are active_low
    if [ "${ONOFF_LED}" == "on" ]; then
        value=0x00
    elif [ "${ONOFF_LED}" == "off" ]; then
        value=0xFF
    else
        echo "Invalid Parameters ${ONOFF_LED}, Exit!!!"
        _help
        exit ${FALSE}
    fi

    i2cset -m $mask -y -r ${I2C_BUS_LED_BOARD} ${I2C_ADDR_MUX_9535_LED} ${REG_OUT_0} $value
}

#Reset all system leds
function _i2c_reset_led {
    i2cset -y ${I2C_BUS_LED_BOARD} ${I2C_ADDR_MUX_9535_LED} ${REG_OUT_0} 0xFF
}

#Get Board Version and Type
function _i2c_board_type_get {
    local reg_board=0x00
    local reg_ext_board=0x07
    local boardType=0
    local boardBuildRev=0
    local boardHwRev=0
    local boardId=0
    local extBoardType=0

    boardType=`i2cget -y ${NUM_I801_DEVICE} ${I2C_ADDR_CPLD} ${reg_board}`
    #Bit 0-1
    boardBuildRev=$((($boardType) & 0x03))
    #Bit 2-3
    boardHwRev=$((($boardType) >> 2 & 0x03))
    #Bit 4-7
    boardId=$((($boardType) >> 4))

    extBoardType=`i2cget -y ${NUM_I801_DEVICE} ${I2C_ADDR_CPLD} ${reg_ext_board}`
    #Bit 0-3
    extBoardType=$((($extBoardType) & 0x0F))

    printf "BOARD_ID=0x%02x, HW Rev=%d, Build Rev=%d, Ext_BOARD_ID=0x%02x\n" $boardId $boardHwRev $boardBuildRev $extBoardType

}

#Get CPLD Version
function _i2c_cpld_version {
    local reg_cpld_rev=0x01
    cpldRev=`i2cget -y ${NUM_I801_DEVICE}  ${I2C_ADDR_CPLD} ${reg_cpld_rev}`
    cpldRelease=$((($cpldRev) >> 6 & 0x01))
    cpldVersion=$((($cpldRev) & 0x3F))
    printf "CPLD is %s version(0:RD 1:Release), Revision is 0x%02x\n" $cpldRelease $cpldVersion

}

#Get PSU Status
function _i2c_psu_status {
    local psu_abs=""

    psu1PwGood=`cat ${PATH_SYSFS_PSU1}/psu_pg`
    psu_abs=`cat ${PATH_SYSFS_PSU1}/psu_abs`
    if [ "$psu_abs" == "0" ]; then
        psu1Exist=1
    else
        psu1Exist=0
    fi

    psu2PwGood=`cat ${PATH_SYSFS_PSU2}/psu_pg`
    psu_abs=`cat ${PATH_SYSFS_PSU2}/psu_abs`
    if [ "$psu_abs" == "0" ]; then
        psu2Exist=1
    else
        psu2Exist=0
    fi

    printf "PSU1 Exist:%x PSU1 PW Good:%d\n" $psu1Exist $psu1PwGood
    printf "PSU2 Exist:%d PSU2 PW Good:%d\n" $psu2Exist $psu2PwGood
}

#Increase read socket buffer for CoPP Test
function _config_rmem {
    echo "109430400" > /proc/sys/net/core/rmem_max
}

#Main Function
function _main {
    start_time_str=`date`
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
    elif [ "${EXEC_FUNC}" == "i2c_led_fan_tray_test" ]; then
        _i2c_led_fan_tray_test
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
    elif [ "${EXEC_FUNC}" == "i2c_test_all" ]; then
        _i2c_deinit
        _i2c_init
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
