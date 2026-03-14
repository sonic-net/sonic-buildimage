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

VERSION="1.0.0"
TRUE=200
FALSE=404

EXEC_FUNC=${1}
COLOR_LED=${2}
QSFP_PORT=${2}
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

# Help usage function
function _help {
    echo "========================================================="
    echo "# Description: Help Function"
    echo "========================================================="
    echo "----------------------------------------------------"
    echo "EX       : ${0} help"
    echo "         : ${0} i2c_init"
    echo "         : ${0} i2c_temp_init"
    echo "         : ${0} i2c_fan_init"
    echo "         : ${0} i2c_volmon_init"
    echo "         : ${0} i2c_io_exp_init"
    echo "         : ${0} i2c_led_test"
    echo "         : ${0} i2c_psu_eeprom_get"
    echo "         : ${0} i2c_mb_eeprom_get"
    echo "         : ${0} i2c_qsfp_eeprom_get [1-64]"
    echo "         : ${0} i2c_qsfp_status_get [1-64]"
    echo "         : ${0} i2c_qsfp_type_get [1-64]"
    echo "         : ${0} i2c_qsfp_abs_get [1-64]"
    echo "         : ${0} i2c_qsfp_rst_get [49-64]"
    echo "         : ${0} i2c_qsfp_lpmode_get [49-64]"
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
    echo "         : ${0} i2c_fan_tray_led green|amber on|off [1-5]"
    echo "----------------------------------------------------"
}

#Pause function
function _pause {
    read -p "$*"
}

#Retry command function
function _retry {
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
        _retry "echo 'pca9548 0x72' > ${PATH_ISMT_DEVICE}/new_device"
    else
        echo "pca9548 0x72 already init."
    fi
    #Init CPLD LED_CLR Register (Front Port LED)
    i2cset -y ${NUM_I801_DEVICE} 0x33 0x34 0x10

    rmmod coretemp
    rmmod jc42
    rmmod w83795
    rmmod lm75
    _i2c_temp_init
    _i2c_volmon_init
    _i2c_hwmon_init
    modprobe coretemp
    modprobe w83795
    modprobe lm75
    modprobe jc42
    modprobe sff_8436_eeprom
    modprobe eeprom
    modprobe eeprom_mb
    modprobe qsfp_cpld
    _i2c_fan_init
    _i2c_io_exp_init
    _i2c_cpld_init
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

    echo "tmp75 0x48" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN3_DEVICE}/new_device
    echo "tmp75 0x49" > ${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX1_CHAN3_DEVICE}/new_device
    echo "Mount Main Board EEPROM"
    echo "mb_eeprom 0x54" > /sys/bus/i2c/devices/i2c-7/new_device
    
    _config_rmem
}

#I2C Deinit
function _i2c_deinit {
    for mod in coretemp jc42 w83795 lm75 eeprom_mb i2c_mux_pca954x i2c_ismt i2c_i801;
    do
        [ "$(lsmod | grep "^$mod ")" != "" ] && rmmod $mod
    done
}

#Temperature sensor Init
function _i2c_temp_init {
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x00 0x80
    i2cset -y -r ${NUM_I801_DEVICE} 0x2F 0x05 0xFF
    echo "TEMP INIT Done"
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

#HWMON Init
function _i2c_hwmon_init {
    echo -n "HWMON INIT..."
    i2cset -y ${NUM_I801_DEVICE} 0x2f 0x00 0x80
    i2cset -y ${NUM_I801_DEVICE} 0x2f 0x06 0xFF
    i2cset -y ${NUM_I801_DEVICE} 0x2f 0x07 0x03
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

    #LED board after PVT (S8900_IO_EXP_LED_ID)
    echo "Init LED IO Expender"
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 6 0x00
    i2cset -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 7 0xFF

    #PSU I/O (S8900_IO_EXP_PSU_ID)
    echo "Init PSU IO Expender"
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 2 0x44
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 6 0xBB
    i2cset -y -r ${NUM_MUX1_CHAN5_DEVICE} 0x20 7 0xFF

    #FAN I/O (S8900_IO_EXP_FAN_ID)
    echo "Init FAN1 IO Expender"
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x20 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x20 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x20 2 0x11
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x20 3 0x11
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x20 6 0xCC
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x20 7 0xCC

    #FAN I/O (S8900_IO_EXP_FAN_ID)
    echo "Init FAN2 IO Expender"
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x21 4 0x00
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x21 5 0x00
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x21 2 0x10
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x21 3 0x00
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x21 6 0xCC
    i2cset -y -r ${NUM_MUX1_CHAN3_DEVICE} 0x21 7 0xFF
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
    #FAN Status set
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`
    FAN2_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan2_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN4_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan4_alarm`
    FAN5_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN6_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan6_alarm`
    FAN7_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`
    FAN8_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan8_alarm`
    FAN9_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan9_alarm`
    FAN10_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan10_alarm`

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
    if [ "${FAN9_ALARM}" == "0" ] && [ "${FAN10_ALARM}" == "0" ]; then
        FAN_TRAY=5
        COLOR_LED="green"
        ONOFF_LED="on"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        echo "${COLOR_LED} ${ONOFF_LED}"
        _i2c_fan_tray_led
    else
        FAN_TRAY=5
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
    FAN9_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan9_alarm`
    FAN10_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan10_alarm`

    if [ "${FAN1_ALARM}" == "0" ] && [ "${FAN2_ALARM}" == "0" ] \
       && [ "${FAN3_ALARM}" == "0" ] && [ "${FAN4_ALARM}" == "0" ] \
       && [ "${FAN5_ALARM}" == "0" ] && [ "${FAN6_ALARM}" == "0" ] \
       && [ "${FAN7_ALARM}" == "0" ] && [ "${FAN8_ALARM}" == "0" ] \
       && [ "${FAN9_ALARM}" == "0" ] && [ "${FAN10_ALARM}" == "0" ]; then
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
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xBF
    _pause 'Check SYS LED green light and Press [Enter] key to continue...'
    #sys led (amber)
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x7F
    _pause 'Check SYS LED amber light and Press [Enter] key to continue...'

    #FAN led (green)
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xEF
    _pause 'Check FAN LED green light and Press [Enter] key to continue...'
    #FAN led (amber)
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xDF
    _pause 'Check FAN LED amber light and Press [Enter] key to continue...'

    #PSU2 led (green)
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFB
    _pause 'Check PSU2 LED green light and Press [Enter] key to continue...'
    #PSU2 led (amber)
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xF7
    _pause 'Check PSU2 LED amber light and Press [Enter] key to continue...'

    #PSU1 led (green)
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFE
    _pause 'Check PSU1 LED green light and Press [Enter] key to continue...'
    #PSU1 led (amber)
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFD
    _pause 'Check PSU1 LED amber light and Press [Enter] key to continue...'

    #Turn OFF All LED
    i2cset -y ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    _pause 'Check turn off all LEDs and Press [Enter] key to continue...'
    echo "done..."
}

#Set QSFP Port variable
function _qsfp_port_i2c_var_set {
    local port=$1
    case ${port} in
        1|2|3|4|5|6|7|8)
            i2cbus=${NUM_MUX1_CHAN0_DEVICE}
            regAddr=0x33
            idx=$(( ((${port} - 1) / 2) ))
            dataAddr=$(( (32 + $idx) ))
            presentChan=$(( $((${port} % 2)) == 1 ? 0x1 : 0x5 ))
            eeprombus=${NUM_MUX1_CHAN0_DEVICE}
            eepromidx=${port}
            eepromAddr=0x50
        ;;
        9|10|11|12|13|14|15|16)
            i2cbus=${NUM_MUX1_CHAN0_DEVICE}
            regAddr=0x33
            idx=$(( ((${port} - 1) / 2) ))
            dataAddr=$(( (32 + $idx) ))
            presentChan=$(( $((${port} % 2)) == 1 ? 0x1 : 0x5 ))
            eeprombus=${NUM_MUX1_CHAN0_DEVICE}
            eepromidx=${port}
            eepromAddr=0x50
        ;;
        17|18|19|20)
            i2cbus=${NUM_MUX1_CHAN0_DEVICE}
            regAddr=0x33
            idx=$(( ((${port} - 1) / 2) ))
            dataAddr=$(( (32 + $idx) ))
            presentChan=$(( $((${port} % 2)) == 1 ? 0x1 : 0x5 ))
            eeprombus=${NUM_MUX1_CHAN0_DEVICE}
            eepromidx=${port}
            eepromAddr=0x50
        ;;
        21|22|23|24)
            i2cbus=${NUM_MUX1_CHAN0_DEVICE}
            regAddr=0x33
            idx=$(( ((${port} - 1) / 2) ))
            dataAddr=$(( (38 + $idx) ))
            presentChan=$(( $((${port} % 2)) == 1 ? 0x1 : 0x5 ))
            eeprombus=${NUM_MUX1_CHAN0_DEVICE}
            eepromidx=${port}
            eepromAddr=0x50
        ;;
        25|26|27|28|29|30|31|32)
            i2cbus=${NUM_MUX1_CHAN1_DEVICE}
            regAddr=0x33
            idx=$(( ((${port} - 25) / 2) ))
            dataAddr=$(( (32 + $idx) ))
            presentChan=$(( $((${port} % 2)) == 1 ? 0x1 : 0x5 ))
            eeprombus=${NUM_MUX1_CHAN1_DEVICE}
            eepromidx=$(( (${port} - 24) ))
            eepromAddr=0x50
        ;;
        33|34|35|36|37|38|39|40)
            i2cbus=${NUM_MUX1_CHAN1_DEVICE}
            regAddr=0x33
            idx=$(( ((${port} - 25) / 2) ))
            dataAddr=$(( (32 + $idx) ))
            presentChan=$(( $((${port} % 2)) == 1 ? 0x1 : 0x5 ))
            eeprombus=${NUM_MUX1_CHAN1_DEVICE}
            eepromidx=$(( (${port} - 24) ))
            eepromAddr=0x50
        ;;
        41|42|43|44)
            i2cbus=${NUM_MUX1_CHAN1_DEVICE}
            regAddr=0x33
            idx=$(( ((${port} - 25) / 2) ))
            dataAddr=$(( (32 + $idx) ))
            presentChan=$(( $((${port} % 2)) == 1 ? 0x1 : 0x5 ))
            eeprombus=${NUM_MUX1_CHAN1_DEVICE}
            eepromidx=$(( (${port} - 24) ))
            eepromAddr=0x50
        ;;
        45|46|47|48)
            i2cbus=${NUM_MUX1_CHAN1_DEVICE}
            regAddr=0x33
            idx=$(( ((${port} - 25) / 2) ))
            dataAddr=$(( (38 + $idx) ))
            presentChan=$(( $((${port} % 2)) == 1 ? 0x1 : 0x5 ))
            eeprombus=${NUM_MUX1_CHAN1_DEVICE}
            eepromidx=$(( (${port} - 24) ))
            eepromAddr=0x50
        ;;
        49|50|51|52|53|54|55|56)
            i2cbus=${NUM_MUX1_CHAN2_DEVICE}
            regAddr=0x33
            idx=$(( (${port} - 49) ))
            dataAddr=$(( (32 + $idx) ))
            rstsellpdataAddr=$(( (48 + $idx) ))
            intChan=0x0
            rstChan=0x0
            presentChan=0x1
            modselChan=0x1
            lpmodeChan=0x2
            eeprombus=${NUM_MUX1_CHAN2_DEVICE}
            eepromidx=$(( (${port} - 48) ))
            intabsidx=$(( (${port} - 28) ))
            rstsellpidx=$(( (${port} - 18) ))
            eepromAddr=0x50
        ;;
        57|58|59|60|61|62|63|64)
            i2cbus=${NUM_MUX1_CHAN2_DEVICE}
            regAddr=0x33
            idx=$(( (${port} - 49) ))
            dataAddr=$(( (32 + $idx) ))
            rstsellpdataAddr=$(( (48 + $idx) ))
            intChan=0x0
            rstChan=0x0
            presentChan=0x1
            modselChan=0x1
            lpmodeChan=0x2
            eeprombus=${NUM_MUX1_CHAN2_DEVICE}
            eepromidx=$(( (${port} - 48) ))
            intabsidx=$(( (${port} - 28) ))
            rstsellpidx=$(( (${port} - 18) ))
            eepromAddr=0x50
        ;;
        *)
            echo "Please input 1~64"
            exit
        ;;
    esac
}

#Get SFP/QSFP EEPROM Information
function _i2c_qsfp_eeprom_get {
    _qsfp_port_i2c_var_set ${QSFP_PORT}

    regData=`cat /sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_modprs`
    port_num=$(( ${QSFP_PORT} - 1 ))

    #status: 0 -> Down, 1 -> Up
    status=$(( $(($regData & ( 1 << $port_num)))  != 0 ? 0 : 1 ))


    if [ $status = 0 ]; then
        echo "Module not present."
        exit
    fi

    #Read SFP/QSFP EEPROM
    if [ ${QSFP_PORT} -lt 49 ]; then
        cat ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/sfp${QSFP_PORT} | hexdump -C
    elif [ ${QSFP_PORT} -ge 49 ]; then
        cat ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/qsfp${QSFP_PORT} | hexdump -C
    fi
}

#Get QSFP Status for QSFP monitor service
function _i2c_qsfp_status_get {
    _i2c_qsfp_abs_get
    echo "status=$status"
}

#Get QSFP type for QSFP monitor server
function _i2c_qsfp_type_get {
    _qsfp_port_i2c_var_set ${QSFP_PORT}

    regData=`cat /sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_modprs`
    port_num=$(( ${QSFP_PORT} - 1 ))

    #status: 0 -> Down, 1 -> Up
    status=$(( $(($regData & ( 1 << $port_num)))  != 0 ? 0 : 1 ))
    
    if [ $status = 0 ]; then
        echo "Module not present."
        exit
    fi

    #Read SFP/QSFP EEPROM
    if [ ${QSFP_PORT} -lt 49 ]; then
        qsfp_info=$(base64 ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/sfp${QSFP_PORT})
    elif [ ${QSFP_PORT} -ge 49 ]; then
        qsfp_info=$(base64 ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/qsfp${QSFP_PORT})
    fi

    if [ ${QSFP_PORT} -ge 1 ] && [ ${QSFP_PORT} -le 48 ]; then
        echo "sfp"
        # 1~48 port is sfp port
        identifier=$(echo $qsfp_info | base64 -d -i | hexdump -s 0 -n 1 -e '"%x"')
        connector=$(echo $qsfp_info | base64 -d -i | hexdump -s 2 -n 1 -e '"%x"')
        transceiver=$(echo $qsfp_info | base64 -d -i | hexdump -s 3 -n 1 -e '"%x"')
    else
        echo "qsfp"
        # 49~64 port is qsfp port
        identifier=$(echo $qsfp_info | base64 -d -i | hexdump -s 128 -n 1 -e '"%x"')
        connector=$(echo $qsfp_info | base64 -d -i | hexdump -s 130 -n 1 -e '"%x"')
        transceiver=$(echo $qsfp_info | base64 -d -i | hexdump -s 131 -n 1 -e '"%x"')
    fi

    echo "identifier=$identifier"
    echo "connector=$connector"
    echo "transceiver=$transceiver"
}

#Get QSFP ABS
function _i2c_qsfp_abs_get {
    _qsfp_port_i2c_var_set ${QSFP_PORT}

    regData=`cat /sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_modprs`
    port_num=$(( ${QSFP_PORT} - 1 ))

    #status: 0 -> Down, 1 -> Up
    status=$(( $(($regData & ( 1 << $port_num)))  != 0 ? 0 : 1 ))
    echo "ABS=$status regData=$regData"


}

#Get QSFP RST
function _i2c_qsfp_rst_get {

    if [ ${QSFP_PORT} -lt 49 ] || [ ${QSFP_PORT} -gt 64 ]; then
        echo "Please input 49~64"
        exit
    fi
    _qsfp_port_i2c_var_set ${QSFP_PORT}


    regData=`cat /sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_reset`
    port_num=$(( ${QSFP_PORT} - 49 ))

    #status: 0 -> Down, 1 -> Up
    status=$(( $(($regData & ( 1 << $port_num)))  != 0 ? 0 : 1 ))
    echo "RST=$status regData=$regData"

}

#Get QSFP LPMODE
function _i2c_qsfp_lpmode_get {

    if [ ${QSFP_PORT} -lt 49 ] || [ ${QSFP_PORT} -gt 64 ]; then
        echo "Please input 49~64"
        exit
    fi
    _qsfp_port_i2c_var_set ${QSFP_PORT}


    regData=`cat /sys/devices/platform/ingrasys-s8900-64xc-cpld.0/qsfp_lpmode`
    port_num=$(( ${QSFP_PORT} - 49 ))

    #status: 0 -> Down, 1 -> Up
    status=$(( $(($regData & ( 1 << $port_num)))  != 0 ? 1 : 0 ))
    echo "LPMODE=$status regData=$regData"

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

    ## modprobe eeprom
    modprobe eeprom_mb

    ## MB EEPROM
    cat ${PATH_SYS_I2C_DEVICES}/${NUM_MUX1_CHAN5_DEVICE}-0054/eeprom | hexdump -C
    echo "done..."
}

#Set System Status LED
function _i2c_sys_led {
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x40 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x40 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x80 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x80 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
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
        i2cset -m 0x04 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x04 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x08 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x08 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
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
        5)
            i2cAddr=0x21
            ioPort=2
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x10
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x20
            fi
            ;;
        *)
            echo "Please input 1~5"
            exit
        ;;
    esac

    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m $mask -y -r ${NUM_MUX1_CHAN3_DEVICE} $i2cAddr $ioPort 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m $mask -y -r ${NUM_MUX1_CHAN3_DEVICE} $i2cAddr $ioPort 0x33
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m $mask -y -r ${NUM_MUX1_CHAN3_DEVICE} $i2cAddr $ioPort 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m $mask -y -r ${NUM_MUX1_CHAN3_DEVICE} $i2cAddr $ioPort 0x33
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
        i2cset -m 0x10 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x10 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x20 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x20 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
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
        i2cset -m 0x01 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x01 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        i2cset -m 0x02 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        i2cset -m 0x02 -y -r ${NUM_MUX1_CHAN4_DEVICE} 0x22 2 0xFF
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
    psu1PwGood=$(($((($psuPwGood) >> 2 & 0x01))?1:0))
    psu2PwGood=$(($((($psuPwGood) >> 2 & 0x02))?1:0))
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
    elif [ "${EXEC_FUNC}" == "i2c_led_test" ]; then
        _i2c_led_test
    elif [ "${EXEC_FUNC}" == "i2c_mb_eeprom_get" ]; then
        _i2c_mb_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_psu_eeprom_get" ]; then
        _i2c_psu_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_eeprom_get" ]; then
        _i2c_qsfp_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_status_get" ]; then
        _i2c_qsfp_status_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_type_get" ]; then
        _i2c_qsfp_type_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_abs_get" ]; then
        _i2c_qsfp_abs_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_rst_get" ]; then
        _i2c_qsfp_rst_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_lpmode_get" ]; then
        _i2c_qsfp_lpmode_get
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
