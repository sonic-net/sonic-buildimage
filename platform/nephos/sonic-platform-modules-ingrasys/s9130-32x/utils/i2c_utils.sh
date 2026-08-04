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

# MAIN MUX PCA9548#0 0x73
NUM_MUX_9548_0_CH0=$(( ${NUM_I801_DEVICE} + 1 )) # ucd9090 0x34
NUM_MUX_9548_0_CH1=$(( ${NUM_I801_DEVICE} + 2 )) # PCA9539 0x76 for fp LED & HW ID
NUM_MUX_9548_0_CH7=$(( ${NUM_I801_DEVICE} + 8 )) # W83795 0x2F

# FRU MUX PCA9545#1 0x72
NUM_MUX_9545_1_CH0=$(( ${NUM_I801_DEVICE} + 9 )) # PSU2 0x50
NUM_MUX_9545_1_CH1=$(( ${NUM_I801_DEVICE} + 10 )) # PSU1 0x50
NUM_MUX_9545_1_CH2=$(( ${NUM_I801_DEVICE} + 11 )) # FAN board IO exander 0x20
NUM_MUX_9545_1_CH3=$(( ${NUM_I801_DEVICE} + 12 )) # TMP75#0 0x4C TMP75#1 0x49

# HOST MUX PCA9548#2 0X70
NUM_MUX_9548_2_CH0=$(( ${NUM_I801_DEVICE} + 13 )) # PCA9548#3 0x71 
NUM_MUX_9548_2_CH1=$(( ${NUM_I801_DEVICE} + 14 )) # PCA9548#4 0x71 
NUM_MUX_9548_2_CH2=$(( ${NUM_I801_DEVICE} + 15 )) # PCA9548#5 0x71 
NUM_MUX_9548_2_CH3=$(( ${NUM_I801_DEVICE} + 16 )) # PCA9548#6 0x71 
NUM_MUX_9548_2_CH4=$(( ${NUM_I801_DEVICE} + 17 )) # PCA9535#3~6 0x20~0x23 ZQSFP ABS/INT
NUM_MUX_9548_2_CH5=$(( ${NUM_I801_DEVICE} + 18 )) # PCA9535#7~10 0x20~0x23 ZQSFP LPMODE/RST
NUM_MUX_9548_3_CH0=$(( ${NUM_I801_DEVICE} + 21 )) # QSFP 0 EEPROM
NUM_MUX_9548_4_CH0=$(( ${NUM_I801_DEVICE} + 29 )) # QSFP 8 EEPROM
NUM_MUX_9548_5_CH0=$(( ${NUM_I801_DEVICE} + 37 )) # QSFP 16 EEPROM
NUM_MUX_9548_6_CH0=$(( ${NUM_I801_DEVICE} + 45 )) # QSFP 24 EEPROM

# MUX Alias
I2C_BUS_MAIN=${NUM_I801_DEVICE}
I2C_BUS_HWM=${NUM_MUX_9548_0_CH7}
I2C_BUS_FAN_STATUS=${NUM_MUX_9545_1_CH2}
I2C_BUS_SYS_LED=${NUM_MUX_9548_0_CH1}
I2C_BUS_HW_ID=${NUM_MUX_9548_0_CH1}
I2C_BUS_BMC_HW_ID=${I2C_BUS_MAIN}
I2C_BUS_PSU_STAT=${I2C_BUS_MAIN}
I2C_BUS_FANTRAY_LED=${NUM_MUX_9545_1_CH2}
I2C_BUS_MB_EEPROM=${I2C_BUS_MAIN}
I2C_BUS_CB_EEPROM=${I2C_BUS_MAIN}
I2C_BUS_PSU1_EEPROM=${NUM_MUX_9545_1_CH1}
I2C_BUS_PSU2_EEPROM=${NUM_MUX_9545_1_CH0}

PATH_SYS_I2C_DEVICES="/sys/bus/i2c/devices"
PATH_SYS_GPIO="/sys/class/gpio"
PATH_HWMON_ROOT_DEVICES="/sys/class/hwmon"
PATH_HWMON_W83795_DEVICE="${PATH_HWMON_ROOT_DEVICES}/hwmon5" 
PATH_I801_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_I801_DEVICE}"
PATH_MUX_9548_0_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH0}"
PATH_MUX_9548_0_CH1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH1}"
PATH_MUX_9548_0_CH7="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH7}"
PATH_MUX_9545_1_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9545_1_CH0}"
PATH_MUX_9545_1_CH1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9545_1_CH1}"
PATH_MUX_9545_1_CH2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9545_1_CH2}"
PATH_MUX_9545_1_CH3="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9545_1_CH3}"
PATH_MUX_9548_2_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH0}"
PATH_MUX_9548_2_CH1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH1}"
PATH_MUX_9548_2_CH2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH2}"
PATH_MUX_9548_2_CH3="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH3}"
PATH_MUX_9548_2_CH4="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH4}"
PATH_MUX_9548_2_CH5="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH5}"
PATH_MUX_9548_3_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_3_CH0}"
PATH_MUX_9548_4_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_4_CH0}"
PATH_MUX_9548_5_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_5_CH0}"
PATH_MUX_9548_6_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_6_CH0}"

# I2C Address
### I2C MUX
I2C_ADDR_MUX_9548_0=0x73 # MAIN MUX
I2C_ADDR_MUX_9545_1=0x72 # FRU MUX
I2C_ADDR_MUX_9548_2=0x70 # HOST MUX
I2C_ADDR_MUX_9548_3=0x71 # ZQSFP MUX #1 EEPROM
I2C_ADDR_MUX_9548_4=0x71 # ZQSFP MUX #2 EEPROM
I2C_ADDR_MUX_9548_5=0x71 # ZQSFP MUX #3 EEPROM
I2C_ADDR_MUX_9548_6=0x71 # ZQSFP MUX #4 EEPROM
### GPIO Expander
I2C_ADDR_MUX_9539_0=0x76 # LED & HW ID 
I2C_ADDR_MUX_9539_1=0x75 # BMC PRSNT & HWM reset
I2C_ADDR_MUX_9539_2=0x74 # SYS SEL & RST
I2C_ADDR_MUX_9535_3=0x20 # ZQSFP0~15 ABS
I2C_ADDR_MUX_9535_4=0x21 # ZQSFP16~31 ABS
I2C_ADDR_MUX_9535_5=0x22 # ZQSFP0~15 INT
I2C_ADDR_MUX_9535_6=0x23 # ZQSFP16~31 INT
I2C_ADDR_MUX_9535_7=0x20 # ZQSFP0~15 LPMODE
I2C_ADDR_MUX_9535_8=0x21 # ZQSFP16~31 LPMODE
I2C_ADDR_MUX_9535_9=0x22 # ZQSFP0~15 RST
I2C_ADDR_MUX_9535_10=0x23 # ZQSFP16~31 RST
I2C_ADDR_MUX_9535_11=0x20 # on FAN board, fan status and led config
I2C_ADDR_MUX_9555_12=0x24 # on BMC board, INT and HW ID
I2C_ADDR_MUX_9555_13=0x25 # on BMC board, PSU status
I2C_ADDR_MUX_9555_14=0x26 # on BMC board, RST and SEL
I2C_ADDR_MUX_9539_15=0x77 # on CPU board, STATUS and ERR from CPLD
### peripheral
I2C_ADDR_MB_EEPROM=0x55 # on main board 
I2C_ADDR_CB_EEPROM=0x51 # on cpu board
I2C_ADDR_UCD9090=0x34
I2C_ADDR_W83795=0x2F
I2C_ADDR_PSU1_EEPROM=0x50
I2C_ADDR_PSU2_EEPROM=0x50
I2C_ADDR_TMP75_REAR=0x4C
I2C_ADDR_TMP75_FRONT=0x49
I2C_ADDR_TMP75_CB=0x4F # on cpu board
I2C_ADDR_TMP75_BB=0x4A # on bmc board
I2C_ADDR_QSFP_EEPROM=0x50

#sysfs
PATH_SYSFS_PSU1="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_PSU1_EEPROM}-$(printf "%04x" $I2C_ADDR_PSU1_EEPROM)"
PATH_SYSFS_PSU2="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_PSU2_EEPROM}-$(printf "%04x" $I2C_ADDR_PSU2_EEPROM)"

#ACTIVE LOW enable flag
ACTIVE_LOW_EN=1
ACTIVE_HIGH_EN=0
#GPIO Direction In/Out
DIR_IN=in
DIR_OUT=out

#Power Supply Status
PSU_DC_ON=1
PSU_DC_OFF=0
PSU_EXIST=1
PSU_NOT_EXIST=0

# IO expander register
# direction
REG_PORT0_DIR=6
REG_PORT1_DIR=7
# polarity
REG_PORT0_POL=4
REG_PORT1_POL=5
# output
REG_PORT0_OUT=2
REG_PORT1_OUT=3
# input 
REG_PORT0_IN=0
REG_PORT1_IN=1

#ZQSFP GPIO sysfs index
ZQSFP_PORT0_ABS_GPIO_IDX=240 # 240~255
ZQSFP_PORT16_ABS_GPIO_IDX=224 # 224~239

# switch port number range
MIN_PORT_NUM=1
MAX_PORT_NUM=32

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
    echo "         : ${0} i2c_fan_speed_init"
    echo "         : ${0} i2c_io_exp_init"
    echo "         : ${0} i2c_gpio_init"
    echo "         : ${0} i2c_gpio_deinit"
    echo "         : ${0} i2c_led_test"
    echo "         : ${0} i2c_psu_eeprom_get"
    echo "         : ${0} i2c_mb_eeprom_get"
    echo "         : ${0} i2c_cb_eeprom_get"
    echo "         : ${0} i2c_eeprom_sync"
    echo "         : ${0} i2c_qsfp_eeprom_get [1-32]"
    echo "         : ${0} i2c_qsfp_eeprom_init new|delete"
    echo "         : ${0} i2c_mb_eeprom_init new|delete"
    echo "         : ${0} i2c_cb_eeprom_init new|delete"
    echo "         : ${0} i2c_qsfp_status_get [1-32]"
    echo "         : ${0} i2c_qsfp_type_get [1-32]"
    echo "         : ${0} i2c_board_type_get"
    echo "         : ${0} i2c_bmc_board_type_get"
    echo "         : ${0} i2c_psu_status"
    echo "         : ${0} i2c_led_psu_status_set"
    echo "         : ${0} i2c_led_fan_status_set"
    echo "         : ${0} i2c_led_fan_tray_status_set"
    echo "         : ${0} i2c_test_all"
    echo "         : ${0} i2c_sys_led green|amber"
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

    #rmmod i2c_ismt
    _util_rmmod i2c_i801
    modprobe i2c_i801 disable_features=0x10
    modprobe i2c_dev
    modprobe i2c_mux_pca954x force_deselect_on_exit=1

    # add MUX PCA9548#0 on I801, assume to be i2c-1~8
    if [ ! -e ${PATH_MUX_9548_0_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_0}' > ${PATH_I801_DEVICE}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_0} already init."
    fi
    # add MUX PCA9545#1 on I801, assume to be i2c-9~12
    if [ ! -e ${PATH_MUX_9545_1_CH0} ]; then
        _retry "echo 'pca9545 ${I2C_ADDR_MUX_9545_1}' > ${PATH_I801_DEVICE}/new_device"
    else
        echo "pca9545 ${I2C_ADDR_MUX_9545_1} already init."
    fi
    # add MUX PCA9548#2 on I801, assume to be i2c-13~20
    if [ ! -e ${PATH_MUX_9548_2_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_2}' > ${PATH_I801_DEVICE}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_2} already init."
    fi
    # add MUX PCA9548#3 on PCA9548#2 CH0, assume to be i2c-21~28
    if [ ! -e ${PATH_MUX_9548_3_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_3}' > ${PATH_MUX_9548_2_CH0}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_3} already init."
    fi
    # add MUX PCA9548#4 on PCA9548#2 CH1, assume to be i2c-29~36
    if [ ! -e ${PATH_MUX_9548_4_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_4}' > ${PATH_MUX_9548_2_CH1}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_4} already init."
    fi
    # add MUX PCA9548#5 on PCA9548#2 CH2, assume to be i2c-37~44
    if [ ! -e ${PATH_MUX_9548_5_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_5}' > ${PATH_MUX_9548_2_CH2}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_5} already init."
    fi
    # add MUX PCA9548#6 on PCA9548#2 CH3, assume to be i2c-45~52
    if [ ! -e ${PATH_MUX_9548_6_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_6}' > ${PATH_MUX_9548_2_CH3}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_6} already init."
    fi
    
    _i2c_hwm_init
    _util_rmmod eeprom
    modprobe eeprom_mb
    modprobe gpio-pca953x
    _i2c_io_exp_init
    rmmod gpio_ich
    _i2c_gpio_init
    modprobe gpio_ich
    _i2c_sensors_init
    _i2c_psu_init
    
    # Init LED_CLR register (pull shift register out of reset), should be after io exp init
    _port_led_clr_init
    _i2c_qsfp_eeprom_init "new"
    _i2c_mb_eeprom_init "new"
    _i2c_cb_eeprom_init "new"
    _i2c_fan_speed_init
    _i2c_led_psu_status_set
    _i2c_led_fan_status_set
    
    # sync eeprom content
    _i2c_eeprom_sync

    # trun on sys led
    echo "led_sys setup..."
    COLOR_LED="green"
    ONOFF_LED="on"
    echo "${COLOR_LED} ${ONOFF_LED}"
    _i2c_sys_led

    _config_rmem
}

#I2C Deinit
function _i2c_deinit {
    echo "i2c deinit..."
    _i2c_gpio_deinit
    for mod in coretemp jc42 w83795 eeprom eeprom_mb gpio-pca953x i2c_mux_pca954x i2c_i801 ingrasys_s9130_32x_psu;
    do
        _util_rmmod $mod
    done
    echo "Done"
}

function _i2c_sensors_init {
    echo "SENSORS init..."
    local dev_path
    # to make sure hwmon index in sysfs as expected, 
    # need to remove kernel module and then probe them in expected order
    # remove all sensors kernel module
    _util_rmmod coretemp
    _util_rmmod jc42
    _util_rmmod w83795
    # probe coretemp kernel module 
    modprobe coretemp
    # probe hwmon kernel module
    modprobe w83795
    # add tmp75 to sysfs
    ####Main board thermal
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9545_1_CH3}-$(printf "%04x" ${I2C_ADDR_TMP75_REAR})"
    if ! [ -L ${dev_path} ]; then
        echo "tmp75 ${I2C_ADDR_TMP75_REAR}" > ${PATH_MUX_9545_1_CH3}/new_device    # hwmon1
    else
        echo "${dev_path} already exist"
    fi
    sleep 0.5
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9545_1_CH3}-$(printf "%04x" ${I2C_ADDR_TMP75_FRONT})"
    if ! [ -L ${dev_path} ]; then
        echo "tmp75 ${I2C_ADDR_TMP75_FRONT}" > ${PATH_MUX_9545_1_CH3}/new_device    #hwmon2
    else
        echo "${dev_path} already exist"
    fi
    sleep 0.5
    ####BMC board thermal
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_0_CH7}-$(printf "%04x" ${I2C_ADDR_TMP75_BB})"
    if ! [ -L ${dev_path} ]; then
        echo "tmp75 ${I2C_ADDR_TMP75_BB}" > ${PATH_MUX_9548_0_CH7}/new_device #hwmon3
    else
        echo "${dev_path} already exist"
    fi
    sleep 0.5
    ####CPU board thermal
    dev_path="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MAIN}-$(printf "%04x" ${I2C_ADDR_TMP75_CB})"
    if ! [ -L ${dev_path} ]; then
        echo "tmp75 ${I2C_ADDR_TMP75_CB}" > ${PATH_I801_DEVICE}/new_device #hwmon4
    else
        echo "${dev_path} already exist"
    fi
    sleep 0.5
    # add w83795 to sysfs
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_0_CH7}-$(printf "%04x" ${I2C_ADDR_W83795})"
    if ! [ -L ${dev_path} ]; then
        echo "w83795adg ${I2C_ADDR_W83795}" > ${PATH_MUX_9548_0_CH7}/new_device #hwmon5
    else
        echo "${dev_path} already exist"
    fi
    sleep 0.5

    # probe jc42 kernel module
    modprobe jc42

    echo "Done"
}

function _port_led_clr_init {
    echo "port led init..."
    # gpio pin on GPIO MUX PCA9539#2 I/O 0.2
    # pull high to out of reset
    output_reg=${REG_PORT0_OUT}
    mask=0x04
    value=0x04
    _util_i2cset -m ${mask} -y ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_2} ${output_reg} ${value}
    echo "Done"
}

#FAN Speed Init
function _i2c_fan_speed_init {
    echo -n "FAN SPEED INIT..."
    if [ -e "${PATH_HWMON_W83795_DEVICE}" ]; then
        # init fan speed
        echo 120 > ${PATH_HWMON_W83795_DEVICE}/device/pwm1
        echo 120 > ${PATH_HWMON_W83795_DEVICE}/device/pwm2
        echo "SUCCESS"
    else
        echo "FAIL"
    fi
}

# HWM init
function _i2c_hwm_init {
    echo "HWM INIT..."
    # select bank0
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x00 0x80
    # SW reset, Disable monitor
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x01 0x9C
    # disable TR5/TR6 DTS
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x04 0x0
    # enable FANIN1~8
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x06 0xFF
    # disable FANIN9~14
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x07 0x00
    # CLKIN clock frequency set as 48Mhz
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x01 0x1C
    # select bank 2
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x00 0x82
    # set PWM mode in FOMC 
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x0F 0x00
    # set 25KHz fan output frequency in F1OPFP&F2OPFP
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x18 0x84
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x19 0x84
}

#Temperature sensor Init
function _i2c_temp_init {
    echo "TEMP INIT..."
    # enable temp monitor on w83795
    # select bank0
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x00 0x80
    # enable TR4 temperature monitoring
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x05 0x40
    # disable TR5/TR6 DTS
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x04 0x0
    echo "Done"
}

#VOLMON Init
function _i2c_volmon_init {
    echo "VOLMON INIT..."
    # enable voltage monitor on w83795
    # VSEN1 P0V9
    # VSEN2 VDD
    # VSEN3 P1V2
    # VSEN4 P1V8
    # select bank0
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x00 0x80
    # enable vsen1~4, disable vsen5~8
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x02 0x0F
    # enable 3VDD,VBAT
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x03 0x50
    echo "Done"
}

#FANIN Init
function _i2c_fan_init {
    echo "FANIN INIT..."
    # enable fan monitor on w83795
    # 4 fantray with 8 FANIN
    # select bank0
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x00 0x80
    # enable FANIN1~8
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x06 0xFF
    # disable FANIN9~14
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x07 0x00

    # select bank 2
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x00 0x82
    # set PWM mode in FOMC 
    _util_i2cset -y -r ${I2C_BUS_HWM} ${I2C_ADDR_W83795} 0x0F 0x00

    echo "Done"
}

#IO Expander Init
function _i2c_io_exp_init {
    echo "========================================================="
    echo "# Description: I2C IO Expender Init"
    echo "========================================================="

     # need to init BMC io expander first due to some io expander are reset default
    echo "Init BMC INT & HW ID IO Expander"
    # all input
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_12} ${REG_PORT0_DIR} 0xFF
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_12} ${REG_PORT1_DIR} 0xFF 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_12} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_12} ${REG_PORT1_POL} 0x00 

    echo "Init BMC PSU status IO Expander"
    # PWRON default  0 (ACTIVE_LOW)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_13} ${REG_PORT0_OUT} 0x00 
    # default 0 (ACTIVE_LOW)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_13} ${REG_PORT1_OUT} 0x00 
    # I/O 0.2 0.5 output(PWRON), rest input
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_13} ${REG_PORT0_DIR} 0xDB 
    # I/O 1.0~1.1 input, 1.2~1.4 output (1.5~1.7 not enable)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_13} ${REG_PORT1_DIR} 0xE3 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_13} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_13} ${REG_PORT1_POL} 0x00 

    echo "Init BMC RST and SEL IO Expander"
    # RST default is 1 (ACTIVE_LOW)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_14} ${REG_PORT0_OUT} 0x3F 
    # SEL default is 0 (HOST), EN default is 1 (ACTIVE_HIGH)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_14} ${REG_PORT1_OUT} 0x1F
    # I/O 0.0~0.5 output, 0.6~0.7 not use
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_14} ${REG_PORT0_DIR} 0xC0
    # all output
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_14} ${REG_PORT1_DIR} 0x00 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_14} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_14} ${REG_PORT1_POL} 0x00 

    echo "Init System LED & HW ID IO Expander"
    # I/O_0.x for System LED default 0, I/O_1.x for HW ID
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT0_OUT} 0x00 
    # System LED => all output 
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT0_DIR} 0x00 
    # HW ID => all input
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT1_DIR} 0xFF
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT1_POL} 0x00 

    echo "Init System PRSNT and HWM RST IO Expander"
    # HWM_RST_L default 1 (ACTIVE_LOW)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_1} ${REG_PORT0_OUT} 0x04 
    # all input expect HWM_RST_L (0.2) 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_1} ${REG_PORT0_DIR} 0xFB
    # port1 not used
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_1} ${REG_PORT1_DIR} 0xFF
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_1} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_1} ${REG_PORT1_POL} 0x00 

    echo "Init System SEL and RST IO Expander"
    # RST 0.0~0.3 default 1 (ACTIVE low), rest default 0
    # SEL set to value 0 (host)
    # LED_CLR also do init in _port_led_clr_init
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_2} ${REG_PORT0_OUT} 0x0F
    #  RST 1.6~1.7 default 1 (ACTIVE low),  INT 1.0~1.4 default 1 (ACTIVE low)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_2} ${REG_PORT1_OUT} 0xDF
    # all output, but MAC_RST_L 0.0 need to set as input to prevent reboot issue 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_2} ${REG_PORT0_DIR} 0x09
    # RST 1.5 !~ 1.7 output, rest are input
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_2} ${REG_PORT1_DIR} 0x1F
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_2} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_2} ${REG_PORT1_POL} 0x00 

    echo "Init FAN Board Status IO Expander"
    # LED_G_L set to 0, LED_Y_L set to 1  (ACTIVE_LOW) 
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_11} ${REG_PORT0_OUT} 0x22
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_11} ${REG_PORT1_OUT} 0x22
    # DIR/ABS is input, LED_Y/LED_G is output
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_11} ${REG_PORT0_DIR} 0xCC
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_11} ${REG_PORT1_DIR} 0xCC
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_11} ${REG_PORT0_POL} 0x00
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9535_11} ${REG_PORT1_POL} 0x00

    echo "Init CPU CPLD IO Expander" 
    # all input 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_15} ${REG_PORT0_DIR} 0xFF 
    # all input
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_15} ${REG_PORT1_DIR} 0xFF
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_15} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9539_15} ${REG_PORT1_POL} 0x00 
    
    echo "Init ZQSFP IO Expender"

    echo "set ZQSFP ABS"
    #zQSFP 0-15 ABS
    # all input
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_3} ${REG_PORT0_DIR} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_3} ${REG_PORT1_DIR} 0xFF
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_3} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_3} ${REG_PORT1_POL} 0x00 
    #zQSFP 16-31 ABS
    # all input
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_4} ${REG_PORT0_DIR} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_4} ${REG_PORT1_DIR} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_4} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_4} ${REG_PORT1_POL} 0x00 

    echo "set ZQSFP INT"
    #zQSFP 0-15 INT
    # all input
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_5} ${REG_PORT0_DIR} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_5} ${REG_PORT1_DIR} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_5} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_5} ${REG_PORT1_POL} 0x00 
    #zQSFP 16-31 INT
    # all input
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_6} ${REG_PORT0_DIR} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_6} ${REG_PORT1_DIR} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_6} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH4} ${I2C_ADDR_MUX_9535_6} ${REG_PORT1_POL} 0x00 

    echo "set ZQSFP LP_MODE = 0"
    #ZQSFP 0-15 LP_MODE 
    # default is 0
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_7} ${REG_PORT0_OUT} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_7} ${REG_PORT1_OUT} 0x00 
    # all output
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_7} ${REG_PORT0_DIR} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_7} ${REG_PORT1_DIR} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_7} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_7} ${REG_PORT1_POL} 0x00 
    #ZQSFP 16-31 LP_MODE 
    # default is 0
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_8} ${REG_PORT0_OUT} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_8} ${REG_PORT1_OUT} 0x00 
    # all output
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_8} ${REG_PORT0_DIR} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_8} ${REG_PORT1_DIR} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_8} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_8} ${REG_PORT1_POL} 0x00 

    echo "set ZQSFP RST = 1"
    #ZQSFP 0-15 RST 
    # default is 1 (ACTIVE_LOW)
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_9} ${REG_PORT0_OUT} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_9} ${REG_PORT1_OUT} 0xFF 
    # all output
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_9} ${REG_PORT0_DIR} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_9} ${REG_PORT1_DIR} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_9} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_9} ${REG_PORT1_POL} 0x00 
    #ZQSFP 16-31 RST 
    # default is 1 (ACTIVE_LOW)
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_10} ${REG_PORT0_OUT} 0xFF 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_10} ${REG_PORT1_OUT} 0xFF 
    # all output
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_10} ${REG_PORT0_DIR} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_10} ${REG_PORT1_DIR} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_10} ${REG_PORT0_POL} 0x00 
    _util_i2cset -y -r ${NUM_MUX_9548_2_CH5} ${I2C_ADDR_MUX_9535_10} ${REG_PORT1_POL} 0x00 

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
    local start=255
    local end=255
    local ch_num=16

    #ABS Port 0-15
    echo "pca9535 ${I2C_ADDR_MUX_9535_3}" > ${PATH_MUX_9548_2_CH4}/new_device
    _set_gpio_offset
    start=$[ ${start}+${GPIO_OFFSET} ]
    end=$[ ${end}+${GPIO_OFFSET} ]
    start=$[ ${end}-${ch_num}+1]
    for (( i=$start; i<=$end; i++ ))   
    do        
        _util_gpio_export ${i} ${DIR_IN} ${ACTIVE_LOW_EN}     
    done

    #ABS Port 16-31
    end=$[ ${start}-1]
    echo "pca9535 ${I2C_ADDR_MUX_9535_4}" > ${PATH_MUX_9548_2_CH4}/new_device    
    start=$[ ${end}-${ch_num}+1]
    for (( i=$start; i<=$end; i++ ))  
    do        
        _util_gpio_export ${i} ${DIR_IN} ${ACTIVE_LOW_EN}     
    done

    #INT Port 0-15
    end=$[ ${start}-1]
    echo "pca9535 ${I2C_ADDR_MUX_9535_5}" > ${PATH_MUX_9548_2_CH4}/new_device    
    start=$[ ${end}-${ch_num}+1]
    for (( i=$start; i<=$end; i++ ))  
    do        
        _util_gpio_export ${i} ${DIR_IN} ${ACTIVE_LOW_EN}     
    done

    #INT Port 16-31
    end=$[ ${start}-1]
    echo "pca9535 ${I2C_ADDR_MUX_9535_6}" > ${PATH_MUX_9548_2_CH4}/new_device    
    start=$[ ${end}-${ch_num}+1]
    for (( i=$start; i<=$end; i++ ))  
    do        
        _util_gpio_export ${i} ${DIR_IN} ${ACTIVE_LOW_EN}     
    done

    #LP Mode Port 0-15
    end=$[ ${start}-1]
    echo "pca9535 ${I2C_ADDR_MUX_9535_7}" > ${PATH_MUX_9548_2_CH5}/new_device    
    start=$[ ${end}-${ch_num}+1]
    for (( i=$start; i<=$end; i++ ))  
    do        
        _util_gpio_export ${i} ${DIR_OUT} ${ACTIVE_HIGH_EN}     
    done

    #LP Mode Port 16-31
    end=$[ ${start}-1]
    echo "pca9535 ${I2C_ADDR_MUX_9535_8}" > ${PATH_MUX_9548_2_CH5}/new_device    
    start=$[ ${end}-${ch_num}+1]
    for (( i=$start; i<=$end; i++ ))  
    do        
        _util_gpio_export ${i} ${DIR_OUT} ${ACTIVE_HIGH_EN}     
    done

    #RESET Port 0-15
    end=$[ ${start}-1]
    echo "pca9535 ${I2C_ADDR_MUX_9535_9}" > ${PATH_MUX_9548_2_CH5}/new_device    
    start=$[ ${end}-${ch_num}+1]
    for (( i=$start; i<=$end; i++ ))  
    do        
        # need to set value to low (became ACTIVE_HIGH) to take port out of reset
        _util_gpio_export ${i} ${DIR_OUT} ${ACTIVE_LOW_EN} 0    
    done

    #RESET Port 16-31
    end=$[ ${start}-1]
    echo "pca9535 ${I2C_ADDR_MUX_9535_10}" > ${PATH_MUX_9548_2_CH5}/new_device    
    start=$[ ${end}-${ch_num}+1]
    for (( i=$start; i<=$end; i++ ))  
    do        
        # need to set value to low (became ACTIVE_HIGH) to take port out of reset
        _util_gpio_export ${i} ${DIR_OUT} ${ACTIVE_LOW_EN} 0   
    done
}

#GPIO DeInit
function _i2c_gpio_deinit {
    echo ${I2C_ADDR_MUX_9535_3} > /sys/bus/i2c/devices/i2c-${NUM_MUX_9548_2_CH4}/delete_device
    echo ${I2C_ADDR_MUX_9535_4} > /sys/bus/i2c/devices/i2c-${NUM_MUX_9548_2_CH4}/delete_device
    echo ${I2C_ADDR_MUX_9535_5} > /sys/bus/i2c/devices/i2c-${NUM_MUX_9548_2_CH4}/delete_device
    echo ${I2C_ADDR_MUX_9535_6} > /sys/bus/i2c/devices/i2c-${NUM_MUX_9548_2_CH4}/delete_device
    echo ${I2C_ADDR_MUX_9535_7} > /sys/bus/i2c/devices/i2c-${NUM_MUX_9548_2_CH5}/delete_device
    echo ${I2C_ADDR_MUX_9535_8} > /sys/bus/i2c/devices/i2c-${NUM_MUX_9548_2_CH5}/delete_device
    echo ${I2C_ADDR_MUX_9535_9} > /sys/bus/i2c/devices/i2c-${NUM_MUX_9548_2_CH5}/delete_device
    echo ${I2C_ADDR_MUX_9535_10} > /sys/bus/i2c/devices/i2c-${NUM_MUX_9548_2_CH5}/delete_device
}

#Set FAN Tray LED
function _i2c_led_fan_tray_status_set {
    echo "FAN Tray Status Setup"
    #FAN Status get
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan8_alarm`
    FAN2_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan6_alarm`
    FAN4_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN5_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan4_alarm`
    FAN6_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN7_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan2_alarm`
    FAN8_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`

    # check if io expander for fan tray exist 
    result=`i2cget -y ${I2C_BUS_FANTRAY_LED} ${I2C_ADDR_MUX_9535_11} ${REG_PORT0_IN} 2>/dev/null`
    err_code=$?
    if [ "$err_code" != "0" ]; then
        echo "fan tray not exist!"
        return
    fi 

    if [ "${FAN1_ALARM}" == "0" ] && [ "${FAN2_ALARM}" == "0" ]; then
        FAN_TRAY=1
        echo "FAN_TRAY${FAN_TRAY}..."
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
        echo "FAN_TRAY${FAN_TRAY}..."
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
        echo "FAN_TRAY${FAN_TRAY}..."
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
        echo "FAN_TRAY${FAN_TRAY}..."
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
        echo "FAN_TRAY${FAN_TRAY}..."
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
        echo "FAN_TRAY${FAN_TRAY}..."
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
        echo "FAN_TRAY${FAN_TRAY}..."
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
        echo "FAN_TRAY${FAN_TRAY}..."
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

    echo "led_fan setup..."
    # all fan ok
    if [ "${FAN1_ALARM}" == "0" ] && [ "${FAN2_ALARM}" == "0" ] \
       && [ "${FAN3_ALARM}" == "0" ] && [ "${FAN4_ALARM}" == "0" ] \
       && [ "${FAN5_ALARM}" == "0" ] && [ "${FAN6_ALARM}" == "0" ] \
       && [ "${FAN7_ALARM}" == "0" ] && [ "${FAN8_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_led
    # all fan fail
    elif [  "${FAN1_ALARM}" == "1" ] && [ "${FAN2_ALARM}" == "1" ] \
       && [ "${FAN3_ALARM}" == "1" ] && [ "${FAN4_ALARM}" == "1" ] \
       && [ "${FAN5_ALARM}" == "1" ] && [ "${FAN6_ALARM}" == "1" ] \
       && [ "${FAN7_ALARM}" == "1" ] && [ "${FAN8_ALARM}" == "1" ]; then
        COLOR_LED="green"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_led
    # partial fan fail
    else
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
    echo "led_psu1 setup..."
    if [ "${psu1Exist}" == ${PSU_EXIST} ]; then
        if [ "${psu1PwGood}" == ${PSU_DC_ON} ]; then
            COLOR_LED="green"
            ONOFF_LED="on"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu1_led
        else
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
    fi

    #PSU2 Status
    echo "led_psu2 setup..."
    if [ "${psu2Exist}" == ${PSU_EXIST} ]; then
        if [ "${psu2PwGood}" == ${PSU_DC_ON} ]; then
            COLOR_LED="green"
            ONOFF_LED="on"
            echo "${COLOR_LED} ${ONOFF_LED}"
            _i2c_psu2_led
        else
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
    fi
}

#LED Test
function _i2c_led_test {
    echo "========================================================="
    echo "# Description: I2C SYSTEM LED TEST..."
    echo "========================================================="
    local output_reg=${REG_PORT0_OUT}
    local mask=0xFF
    local value=0xFF

    #sys led (green)
    # set sys_led_g (0.7) = 1
    mask=0x80
    value=0x80
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check SYS LED green light and Press [Enter] key to continue...'
    #sys led (amber)
    # set sys_led_g (0.7) = 0
    mask=0x80
    value=0x00
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check SYS LED amber light and Press [Enter] key to continue...'

    #FAN led (green)
    # set fan_led_en (0.6) = 1 & fan_led_y (0.5) = 0
    mask=0x60
    value=0x40
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check FAN LED green light and Press [Enter] key to continue...'
    #FAN led (amber)
    # set fan_led_en (0.6) = 1 & fan_led_y (0.5) = 1
    mask=0x60
    value=0x60
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check FAN LED amber light and Press [Enter] key to continue...'

    #PSU1 led (green)
    # set psu1_pwr_ok_oe (0.4) = 1 & psu1_led_y (0.3) = 0
    mask=0x18
    value=0x10
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check PSU1 LED green light and Press [Enter] key to continue...'
    #PSU1 led (amber)
    # set psu1_pwr_ok_oe (0.4) = 1 & psu1_led_y (0.3) = 1
    mask=0x18
    value=0x18
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check PSU1 LED amber light and Press [Enter] key to continue...'

    #PSU2 led (green)
    # set psu0_pwr_ok_oe (0.2) = 1 & psu0_led_y (0.1) = 0
    mask=0x06
    value=0x04
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check PSU2 LED green light and Press [Enter] key to continue...'
    #PSU2 led (amber)
    # set psu0_pwr_ok_oe (0.2) = 1 & psu0_led_y (0.1) = 1
    mask=0x06
    value=0x06
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check PSU2 LED amber light and Press [Enter] key to continue...'

    #Turn OFF All LED (can't trun off system led)
    # set set fan_led_en (0.6), psu1_pwr_ok_oe (0.4), psu0_pwr_ok_oe (0.2) = 0
    mask=0x54
    value=0x00
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    _pause 'Check turn off all LEDs (exclude SYS LED) and Press [Enter] key to continue...'

    # restore sys led
    COLOR_LED="green"
    ONOFF_LED="on"
    echo "${COLOR_LED} ${ONOFF_LED}"
    _i2c_sys_led

    echo "Done"
}

#Set QSFP Port variable
function _qsfp_port_i2c_var_set {
    local port=$1
    case ${port} in
        1|2|3|4|5|6|7|8)
            i2cbus=${NUM_MUX_9548_2_CH4}
            eeprombusbase=${NUM_MUX_9548_3_CH0}
            gpioBase=$((${GPIO_OFFSET}+${ZQSFP_PORT0_ABS_GPIO_IDX}))
            #gpioBase=${ZQSFP_PORT0_ABS_GPIO_IDX}
        ;;
        9|10|11|12|13|14|15|16)
            i2cbus=${NUM_MUX_9548_2_CH4}
            eeprombusbase=${NUM_MUX_9548_4_CH0}
            gpioBase=$((${GPIO_OFFSET}+${ZQSFP_PORT0_ABS_GPIO_IDX}))
            #gpioBase=${ZQSFP_PORT0_ABS_GPIO_IDX}
        ;;
        17|18|19|20|21|22|23|24)
            i2cbus=${NUM_MUX_9548_2_CH4}
            eeprombusbase=${NUM_MUX_9548_5_CH0}
            gpioBase=$((${GPIO_OFFSET}+${ZQSFP_PORT16_ABS_GPIO_IDX}))
            #gpioBase=${ZQSFP_PORT16_ABS_GPIO_IDX}
        ;;
        25|26|27|28|29|30|31|32)
            i2cbus=${NUM_MUX_9548_2_CH4}
            eeprombusbase=${NUM_MUX_9548_6_CH0}
            gpioBase=$((${GPIO_OFFSET}+${ZQSFP_PORT16_ABS_GPIO_IDX}))
            #gpioBase=${ZQSFP_PORT16_ABS_GPIO_IDX}
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
    # port 1 => zqsfp0
    # port 2 => zqsfp1
    # ...
    eeprombusidx=$(( (${port} - 1) % 8)) 
    eeprombus=$(( ${eeprombusbase} + ${eeprombusidx} ))
    eepromAddr=${I2C_ADDR_QSFP_EEPROM}
}

#Get QSFP EEPROM Information
function _i2c_qsfp_eeprom_get {

    # input parameter validation
    _util_input_check "${QSFP_PORT}" "${MIN_PORT_NUM}" "${MAX_PORT_NUM}"

    _util_get_qsfp_abs

    if [ $status = 0 ]; then
        exit
    fi

    _qsfp_eeprom_var_set ${QSFP_PORT}

    cat ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/eeprom | hexdump -C
}

#Init QSFP EEPROM
function _i2c_qsfp_eeprom_init {
    echo "QSFP EEPROM INIT..."

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
    echo "Done"
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

    #Init mb EEPROM
    if [ "${action}" == "new" ] && \
        ! [ -L ${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MB_EEPROM}-$(printf "%04x" $I2C_ADDR_MB_EEPROM) ]; then
        echo "mb_eeprom ${I2C_ADDR_MB_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_MB_EEPROM}/new_device
    elif [ "${action}" == "delete" ] && \
        [ -L ${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MB_EEPROM}-$(printf "%04x" $I2C_ADDR_MB_EEPROM) ]; then
        echo "$I2C_ADDR_MB_EEPROM" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_MB_EEPROM}/delete_device
    fi
    echo "Done"
}

#Init CPU Board EEPROM
function _i2c_cb_eeprom_init {
    echo -n "CPU Board EEPROM INIT..."

    #Action check
    action=$1
    if [ -z "${action}" ]; then
        echo "No action, skip"
        return
    elif [ "${action}" != "new" ] && [ "${action}" != "delete" ]; then
        echo "Error action, skip"
        return
    fi

    #Init cpu EEPROM
    if [ "${action}" == "new" ] && \
        ! [ -L ${PATH_SYS_I2C_DEVICES}/${I2C_BUS_CB_EEPROM}-$(printf "%04x" $I2C_ADDR_CB_EEPROM) ]; then
        echo "mb_eeprom ${I2C_ADDR_CB_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_CB_EEPROM}/new_device
    elif [ "${action}" == "delete" ] && \
        [ -L ${PATH_SYS_I2C_DEVICES}/${I2C_BUS_CB_EEPROM}-$(printf "%04x" $I2C_ADDR_CB_EEPROM) ]; then
        echo "$I2C_ADDR_CB_EEPROM" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_CB_EEPROM}/delete_device
    fi
    echo "Done"
}


#get QSFP Status
function _i2c_qsfp_status_get {

    # input parameter validation
    _util_input_check "${QSFP_PORT}" "${MIN_PORT_NUM}" "${MAX_PORT_NUM}"

    local stat
    _util_get_qsfp_abs
    echo "status=$status"
}

#get QSFP Type
function _i2c_qsfp_type_get {

    # input parameter validation
    _util_input_check "${QSFP_PORT}" "${MIN_PORT_NUM}" "${MAX_PORT_NUM}"

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
    modprobe ingrasys_s9130_32x_psu

    echo "psu1 ${I2C_ADDR_PSU1_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU1_EEPROM}/new_device
    echo "psu2 ${I2C_ADDR_PSU2_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU2_EEPROM}/new_device
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

#Get Main Board EEPROM Information
function _i2c_mb_eeprom_get {
    echo "========================================================="
    echo "# Description: I2C MB EEPROM Get..."
    echo "========================================================="
    _i2c_sys_eeprom_get mb
}

#Get CPU Board EEPROM Information
function _i2c_cb_eeprom_get {
    echo "========================================================="
    echo "# Description: I2C CB EEPROM Get..."
    echo "========================================================="
    _i2c_sys_eeprom_get cb
}

#Get system EEPROM Information
##input: "cb" for cpu board, "mb" for main board
function _i2c_sys_eeprom_get {
    local eeprom_dev

    if [ "$1" == "cb" ]; then
        eeprom_dev="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_CB_EEPROM}-$(printf "%04x" $I2C_ADDR_CB_EEPROM)/eeprom"
    elif  [ "$1" == "mb" ]; then
        eeprom_dev="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MB_EEPROM}-$(printf "%04x" $I2C_ADDR_MB_EEPROM)/eeprom"
    else
         echo "wrong eeprom type"
	 return
    fi

    # check if eeprom device exist in sysfs
    if [ ! -f ${eeprom_dev} ]; then  
        echo "eeprom device not init"
	return
    fi

    cat ${eeprom_dev} | hexdump -C
    echo "Done"
}

#sync eeprom content between mb and cb eeprom
function _i2c_eeprom_sync {
    echo "========================================================="
    echo "# Description: EEPROM sync..."
    echo "========================================================="

    local mb_eeprom_dev="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MB_EEPROM}-$(printf "%04x" $I2C_ADDR_MB_EEPROM)/eeprom"
    local cb_eeprom_dev="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_CB_EEPROM}-$(printf "%04x" $I2C_ADDR_CB_EEPROM)/eeprom"

    # check if eeprom device exist in sysfs
    if [[ ! -f ${mb_eeprom_dev} || ! -f ${cb_eeprom_dev} ]]; then  
        echo "eeprom device not init"
	return
    fi

    ## check if MB eeprom is empty
    if [ ! -z "$(cat ${mb_eeprom_dev} | hexdump -n2 | grep ffff)" ]; then  
        echo "copy cb eeprom to mb eeprom..."
        cat ${cb_eeprom_dev} > ${mb_eeprom_dev}
    else 
        echo "no need to sync"
    fi

    echo "Done"
}

#Set System Status LED
function _i2c_sys_led {
    # only green/amber, on/off can't control 
    if [ "${COLOR_LED}" == "green" ]; then
        # set sys_led_g (0.7) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x80
        value=0x80
    elif [ "${COLOR_LED}" == "amber" ]; then
        # set sys_led_g (0.7) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x80
        value=0x00
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    #apply to io expander
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    echo "Done"
}

#Set FAN LED
function _i2c_fan_led {
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        # set fan_led_en (0.6) = 1 & fan_led_y (0.5) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x60
        value=0x40
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        # set fan_led_en (0.6) = 0 & fan_led_y (0.5) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x60
        value=0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        # set fan_led_en (0.6) = 1 & fan_led_y (0.5) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x60
        value=0x60
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        # set fan_led_en (0.6) = 0 & fan_led_y (0.5) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x60
        value=0x20
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    #apply to io expander
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    echo "Done"
}

#Set PSU1 LED
function _i2c_psu1_led {
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        # set psu1_pwr_ok_oe (0.4) = 1 & psu1_led_y (0.3) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x18
        value=0x10
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        # set psu1_pwr_ok_oe (0.4) = 0 & psu1_led_y (0.3) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x18
        value=0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        # set psu1_pwr_ok_oe (0.4) = 1 & psu1_led_y (0.3) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x18
        value=0x18
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        # set psu1_pwr_ok_oe (0.4) = 0 & psu1_led_y (0.3) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x18
        value=0x08
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    #apply to io expander
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    echo "Done"
}

#Set PSU2 LED
function _i2c_psu2_led {
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        # set psu0_pwr_ok_oe (0.2) = 1 & psu0_led_y (0.1) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x06
        value=0x04
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        # set psu0_pwr_ok_oe (0.2) = 0 & psu0_led_y (0.1) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x06
        value=0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        # set psu0_pwr_ok_oe (0.2) = 1 & psu0_led_y (0.1) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x06
        value=0x06
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        # set psu0_pwr_ok_oe (0.2) = 0 & psu0_led_y (0.1) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x06
        value=0x02
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    #apply to io expander
    _util_i2cset -m ${mask} -y ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${output_reg} ${value}
    echo "Done"
}

#Set FAN Tray LED
function _i2c_fan_tray_led {

    i2cAddr=${I2C_ADDR_MUX_9535_11}
    output_reg=${REG_PORT0_OUT}

    case ${FAN_TRAY} in
        4)
            output_reg=${REG_PORT0_OUT}
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x01
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x02
            fi
            ;;
        3)
            output_reg=${REG_PORT0_OUT}
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x10
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x20
            fi
            ;;
        2)
            output_reg=${REG_PORT1_OUT}
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x01
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x02
            fi
            ;;
        1)
            output_reg=${REG_PORT1_OUT}
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
        _util_i2cset -m $mask -y -r ${I2C_BUS_FANTRAY_LED} $i2cAddr ${output_reg} 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        _util_i2cset -m $mask -y -r ${I2C_BUS_FANTRAY_LED} $i2cAddr ${output_reg} 0x33
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        _util_i2cset -m $mask -y -r ${I2C_BUS_FANTRAY_LED} $i2cAddr ${output_reg} 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        _util_i2cset -m $mask -y -r ${I2C_BUS_FANTRAY_LED} $i2cAddr ${output_reg} 0x33
    else
        echo "Invalid Parameters, Exit!!!"
        _help
        exit ${FALSE}
    fi

    echo "Done"
}

#Get Board Version and Type
function _i2c_board_type_get {
    # read input port 1 value from io expander
    input_reg=${REG_PORT1_IN}
    boardType=`i2cget -y ${I2C_BUS_HW_ID} ${I2C_ADDR_MUX_9539_0} ${input_reg}`
    boardBuildRev=$((($boardType) & 0x03))
    boardHwRev=$((($boardType) >> 2 & 0x03))
    boardId=$((($boardType) >> 4))
    printf "MAIN_BOARD BOARD_ID is 0x%02x, HW Rev %d, Build Rev %d\n" $boardId $boardHwRev $boardBuildRev
}

#Get BMC Board Version and Type
function _i2c_bmc_board_type_get {
    # read input port 1 value from io expander
    input_reg=${REG_PORT1_IN}
    boardType=`i2cget -y ${I2C_BUS_BMC_HW_ID} ${I2C_ADDR_MUX_9555_12} ${input_reg}`
    boardBuildRev=$((($boardType) & 0x03))
    boardHwRev=$((($boardType) >> 2 & 0x03))
    boardId=$((($boardType) >> 4))
    printf "BMC_BOARD BOARD_ID is 0x%02x, HW Rev %d, Build Rev %d\n" $boardId $boardHwRev $boardBuildRev
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

#util functions
function _util_i2cset {
    if [ "$DEBUG" == "on" ]; then
        i2cset $@
    else
        i2cset $@ 1>/dev/null
    fi
}

function _util_rmmod {
    local mod=$1
    [ "$(lsmod | grep "^$mod ")" != "" ] && rmmod $mod
}

# get qsfp presence 
function _util_get_qsfp_abs {
    _qsfp_port_i2c_var_set ${QSFP_PORT}

    #status: 0 -> Down, 1 -> Up (ACTIVE_LOW_EN)
    status=`cat /sys/class/gpio/gpio$(( $(($gpioBase + (${QSFP_PORT} - 1) % 16 )) ))/value`
}

# gpio init util function
function _util_gpio_export {
    local gpio_n=$1    
    local direction=$2     
    local active_low=$3    
    local value=$4

    if [ -z "${gpio_n}" ]; then
        echo "[gpio_init]  gpio_n(${gpio_n}) is not provided"
        return
    fi
    if [[ ${gpio_n} < 0 || ${gpio_n} > $(( 255+${GPIO_OFFSET} )) ]]; then
        echo "[gpio_init]  gpio_n(${gpio_n}) is invalid value"
        return    
    fi

    #export gpio
    echo ${gpio_n} > /sys/class/gpio/export
    #set gpio direction
    echo ${direction} > /sys/class/gpio/gpio${gpio_n}/direction
    #set gpio active_low
    echo ${active_low} > /sys/class/gpio/gpio${gpio_n}/active_low
    #set value
    if [ ! -z "${value}" ]; then
        echo ${value} > /sys/class/gpio/gpio${gpio_n}/value
    fi
}

# valid input number
function _util_input_check {
    # input parameter validation
    if [[ $1 -lt $2  || $1 -gt $3 ]]; then
        echo "Please input number $2~$3"        
        exit
    fi
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
    elif [ "${EXEC_FUNC}" == "i2c_fan_speed_init" ]; then
        _i2c_fan_speed_init
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
    elif [ "${EXEC_FUNC}" == "i2c_cb_eeprom_get" ]; then
        _i2c_cb_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_eeprom_sync" ]; then
        _i2c_eeprom_sync
    elif [ "${EXEC_FUNC}" == "i2c_psu_eeprom_get" ]; then
        _i2c_psu_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_eeprom_get" ]; then
        _i2c_qsfp_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_eeprom_init" ]; then
        _i2c_qsfp_eeprom_init ${QSFP_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_mb_eeprom_init" ]; then
        _i2c_mb_eeprom_init ${MB_EEPROM_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_cb_eeprom_init" ]; then
        _i2c_cb_eeprom_init ${MB_EEPROM_ACTION}
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
    elif [ "${EXEC_FUNC}" == "i2c_bmc_board_type_get" ]; then
        _i2c_bmc_board_type_get
    elif [ "${EXEC_FUNC}" == "i2c_psu_status" ]; then
        _i2c_psu_status
    elif [ "${EXEC_FUNC}" == "i2c_test_all" ]; then
        _i2c_init
        _i2c_led_test
        _i2c_psu_eeprom_get
        _i2c_mb_eeprom_get
	_i2c_cb_eeprom_get
        _i2c_board_type_get
	_i2c_bmc_board_type_get
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
