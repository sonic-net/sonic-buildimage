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
SFP_PORT=${2}
QSFP_ACTION=${2}
SFP_ACTION=${2}
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

# PCA9548#0 0x70
NUM_MUX_9548_0_CH0=$(( ${NUM_I801_DEVICE} + 1 )) # CPLD1
NUM_MUX_9548_0_CH1=$(( ${NUM_I801_DEVICE} + 2 )) # CPLD2
NUM_MUX_9548_0_CH2=$(( ${NUM_I801_DEVICE} + 3 )) # CPLD3
NUM_MUX_9548_0_CH3=$(( ${NUM_I801_DEVICE} + 4 )) # CPLD4
NUM_MUX_9548_0_CH4=$(( ${NUM_I801_DEVICE} + 5 )) # CPLD5
NUM_MUX_9548_0_CH5=$(( ${NUM_I801_DEVICE} + 6 )) # LM75_1 LM75_2 LM86
NUM_MUX_9548_0_CH6=$(( ${NUM_I801_DEVICE} + 7 )) # LM75_3 LM75_4

# PCA9548#1 0x73
NUM_MUX_9548_1_CH0=$(( ${NUM_I801_DEVICE} + 9 )) # UCD9090
NUM_MUX_9548_1_CH1=$(( ${NUM_I801_DEVICE} + 10 )) # PCA9539#0 0x76 for FP LED & HW ID
NUM_MUX_9548_1_CH2=$(( ${NUM_I801_DEVICE} + 11 )) # NONE
NUM_MUX_9548_1_CH3=$(( ${NUM_I801_DEVICE} + 12 )) # EEPROM for debug 0x57
#NUM_MUX_9548_1_CH4=$(( ${NUM_I801_DEVICE} + 13 )) # NONE
NUM_MUX_9548_1_CH5=$(( ${NUM_I801_DEVICE} + 14 )) # CLK gen
NUM_MUX_9548_1_CH6=$(( ${NUM_I801_DEVICE} + 15 )) # ROV
NUM_MUX_9548_1_CH7=$(( ${NUM_I801_DEVICE} + 16 )) # HWM on BMC

# PCA9546#0 0X72
NUM_MUX_9546_0_CH0=$(( ${NUM_I801_DEVICE} + 17 )) # PSU1 0x58
NUM_MUX_9546_0_CH1=$(( ${NUM_I801_DEVICE} + 18 )) # PSU2 0x58
NUM_MUX_9546_0_CH2=$(( ${NUM_I801_DEVICE} + 19 )) # NONE
NUM_MUX_9546_0_CH3=$(( ${NUM_I801_DEVICE} + 20 )) # NONE

# PCA9548#2 0X71
NUM_MUX_9548_2_CH0=$(( ${NUM_I801_DEVICE} + 21 )) # PCA9548#3 0x74
NUM_MUX_9548_2_CH1=$(( ${NUM_I801_DEVICE} + 22 )) # PCA9548#4 0x74
NUM_MUX_9548_2_CH2=$(( ${NUM_I801_DEVICE} + 23 )) # PCA9548#5 0x74
NUM_MUX_9548_2_CH3=$(( ${NUM_I801_DEVICE} + 24 )) # PCA9548#6 0x74
NUM_MUX_9548_2_CH4=$(( ${NUM_I801_DEVICE} + 25 )) # PCA9548#7 0x74
NUM_MUX_9548_2_CH5=$(( ${NUM_I801_DEVICE} + 26 )) # PCA9548#8 0x74
NUM_MUX_9548_2_CH6=$(( ${NUM_I801_DEVICE} + 27 )) # PCA9548#9 0x74
NUM_MUX_9548_2_CH7=$(( ${NUM_I801_DEVICE} + 28 )) # PCA9548#10 0x74



# PCA9546#1 0X77
NUM_MUX_9546_1_CH0=$(( ${NUM_I801_DEVICE} + 29 )) # SFP0 EEPROM 0x50
NUM_MUX_9546_1_CH1=$(( ${NUM_I801_DEVICE} + 30 )) # SFP1 EEPROM 0x50
NUM_MUX_9546_1_CH2=$(( ${NUM_I801_DEVICE} + 31 )) # NONE
NUM_MUX_9546_1_CH3=$(( ${NUM_I801_DEVICE} + 32 )) # PCA9539#1 0x76 for sys control

# PCA9548#11 0X75
NUM_MUX_9548_11_CH0=$(( ${NUM_I801_DEVICE} + 33 )) # SW upgrade for CPLD1
NUM_MUX_9548_11_CH1=$(( ${NUM_I801_DEVICE} + 34 )) # SW upgrade for CPLD2
NUM_MUX_9548_11_CH2=$(( ${NUM_I801_DEVICE} + 35 )) # SW upgrade for CPLD3
NUM_MUX_9548_11_CH3=$(( ${NUM_I801_DEVICE} + 36 )) # SW upgrade for CPLD4
NUM_MUX_9548_11_CH4=$(( ${NUM_I801_DEVICE} + 37 )) # SW upgrade for CPLD5
NUM_MUX_9548_11_CH5=$(( ${NUM_I801_DEVICE} + 38 )) # EEPROM for debug 0x57
NUM_MUX_9548_11_CH6=$(( ${NUM_I801_DEVICE} + 39 )) # Retimer#0~3
NUM_MUX_9548_11_CH7=$(( ${NUM_I801_DEVICE} + 40 )) # Retimer#4~7

# PCA9548#3~10 0X74
NUM_MUX_9548_3_CH0=$(( ${NUM_I801_DEVICE} + 41 )) # QSFP 0 EEPROM
NUM_MUX_9548_4_CH0=$(( ${NUM_I801_DEVICE} + 49 )) # QSFP 8 EEPROM
NUM_MUX_9548_5_CH0=$(( ${NUM_I801_DEVICE} + 57 )) # QSFP 16 EEPROM
NUM_MUX_9548_6_CH0=$(( ${NUM_I801_DEVICE} + 65 )) # QSFP 24 EEPROM
NUM_MUX_9548_7_CH0=$(( ${NUM_I801_DEVICE} + 73 )) # QSFP 32 EEPROM
NUM_MUX_9548_8_CH0=$(( ${NUM_I801_DEVICE} + 81 )) # QSFP 40 EEPROM
NUM_MUX_9548_9_CH0=$(( ${NUM_I801_DEVICE} + 89 )) # QSFP 48 EEPROM
NUM_MUX_9548_10_CH0=$(( ${NUM_I801_DEVICE} + 97 )) # QSFP 56 EEPROM

# MUX Alias
I2C_BUS_MAIN=${NUM_I801_DEVICE}
I2C_BUS_HWM=${NUM_MUX_9548_1_CH7}
I2C_BUS_FAN_STATUS=${I2C_BUS_MAIN}
I2C_BUS_SYS_LED=${NUM_MUX_9548_1_CH1}
I2C_BUS_HW_ID=${NUM_MUX_9548_1_CH1}
I2C_BUS_BMC_HW_ID=${I2C_BUS_MAIN}
I2C_BUS_PSU_STAT=${I2C_BUS_MAIN}
I2C_BUS_FANTRAY_LED=${I2C_BUS_MAIN}
I2C_BUS_MB_EEPROM=${I2C_BUS_MAIN}
I2C_BUS_CB_EEPROM=${I2C_BUS_MAIN}
# TODO: check i2c channel of PSU1/PSU2, it swap in s9230
I2C_BUS_PSU1_EEPROM=${NUM_MUX_9546_0_CH1}
I2C_BUS_PSU2_EEPROM=${NUM_MUX_9546_0_CH0}
I2C_BUS_CPLD1=${NUM_MUX_9548_0_CH0}
I2C_BUS_CPLD2=${NUM_MUX_9548_0_CH1}
I2C_BUS_CPLD3=${NUM_MUX_9548_0_CH2}
I2C_BUS_CPLD4=${NUM_MUX_9548_0_CH3}
I2C_BUS_CPLD5=${NUM_MUX_9548_0_CH4}
I2C_BUS_ROV=${NUM_MUX_9548_1_CH6}

# I2C BUS path
PATH_SYS_I2C_DEVICES="/sys/bus/i2c/devices"
PATH_HWMON_ROOT_DEVICES="/sys/class/hwmon"
# TODO: need to verify HWM deivce path after board ready
PATH_HWMON_W83795_DEVICE="${PATH_HWMON_ROOT_DEVICES}/hwmon1"
PATH_I801_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_I801_DEVICE}"
PATH_MUX_9548_0_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH0}"
PATH_MUX_9548_0_CH1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH1}"
PATH_MUX_9548_0_CH2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH2}"
PATH_MUX_9548_0_CH3="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH3}"
PATH_MUX_9548_0_CH4="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH4}"
PATH_MUX_9548_0_CH5="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH5}"
PATH_MUX_9548_0_CH6="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH6}"
PATH_MUX_9548_0_CH7="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_0_CH7}"
PATH_MUX_9548_1_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CH0}"
PATH_MUX_9548_1_CH1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CH1}"
PATH_MUX_9548_1_CH2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CH2}"
PATH_MUX_9548_1_CH3="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CH3}"
PATH_MUX_9548_1_CH4="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CH4}"
PATH_MUX_9548_1_CH5="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CH5}"
PATH_MUX_9548_1_CH6="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CH6}"
PATH_MUX_9548_1_CH7="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_1_CH7}"
PATH_MUX_9548_2_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH0}"
PATH_MUX_9548_2_CH1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH1}"
PATH_MUX_9548_2_CH2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH2}"
PATH_MUX_9548_2_CH3="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH3}"
PATH_MUX_9548_2_CH4="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH4}"
PATH_MUX_9548_2_CH5="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH5}"
PATH_MUX_9548_2_CH6="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH6}"
PATH_MUX_9548_2_CH7="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_2_CH7}"
PATH_MUX_9546_0_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9546_0_CH0}"
PATH_MUX_9546_0_CH1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9546_0_CH1}"
PATH_MUX_9546_0_CH2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9546_0_CH2}"
PATH_MUX_9546_0_CH3="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9546_0_CH3}"
PATH_MUX_9546_1_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9546_1_CH0}"
PATH_MUX_9546_1_CH1="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9546_1_CH1}"
PATH_MUX_9546_1_CH2="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9546_1_CH2}"
PATH_MUX_9546_1_CH3="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9546_1_CH3}"
PATH_MUX_9548_11_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_11_CH0}"
PATH_MUX_9548_3_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_3_CH0}"
PATH_MUX_9548_4_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_4_CH0}"
PATH_MUX_9548_5_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_5_CH0}"
PATH_MUX_9548_6_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_6_CH0}"
PATH_MUX_9548_7_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_7_CH0}"
PATH_MUX_9548_8_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_8_CH0}"
PATH_MUX_9548_9_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_9_CH0}"
PATH_MUX_9548_10_CH0="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_10_CH0}"
PATH_CPLD1_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_CPLD1}"
PATH_CPLD2_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_CPLD2}"
PATH_CPLD3_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_CPLD3}"
PATH_CPLD4_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_CPLD4}"
PATH_CPLD5_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_CPLD5}"


# I2C Address
### I2C MUX
I2C_ADDR_MUX_9548_0=0x70
I2C_ADDR_MUX_9548_1=0x73
I2C_ADDR_MUX_9548_2=0x71
I2C_ADDR_MUX_9546_0=0x72
I2C_ADDR_MUX_9546_1=0x71
I2C_ADDR_MUX_9548_11=0x75
I2C_ADDR_MUX_9548_3=0x74
I2C_ADDR_MUX_9548_4=0x74
I2C_ADDR_MUX_9548_5=0x74
I2C_ADDR_MUX_9548_6=0x74
I2C_ADDR_MUX_9548_7=0x74
I2C_ADDR_MUX_9548_8=0x74
I2C_ADDR_MUX_9548_9=0x74
I2C_ADDR_MUX_9548_10=0x74

### GPIO Expander
I2C_ADDR_MUX_9539_0=0x76 # LED & HW ID
I2C_ADDR_MUX_9539_1=0x76 # SYS config
I2C_ADDR_MUX_9539_2=0x76 # on CPU board, STATUS and ERR from CPLD
I2C_ADDR_MUX_9555_0=0x20 # on FAN board, fan status and led config
I2C_ADDR_MUX_9555_1=0x24 # on BMC board, INT and HW ID
I2C_ADDR_MUX_9555_2=0x25 # on BMC board, PSU status
I2C_ADDR_MUX_9555_3=0x26 # on BMC board, RST and SEL


### peripheral
I2C_ADDR_MB_EEPROM=0x55 # on main board
I2C_ADDR_CB_EEPROM=0x51 # on cpu board
I2C_ADDR_UCD9090=0x34
I2C_ADDR_W83795=0x2F
I2C_ADDR_PSU1_EEPROM=0x50
I2C_ADDR_PSU2_EEPROM=0x50
I2C_ADDR_LM75_1=0x4D # Rear Panel
I2C_ADDR_LM75_2=0x4E # Rear MAC
I2C_ADDR_LM86=0x4C   # TBD
I2C_ADDR_LM75_3=0x4D # Front Panel
I2C_ADDR_LM75_4=0x4E # Front MAC
I2C_ADDR_TMP75_CB=0x4F # on cpu board
I2C_ADDR_TMP75_BB=0x4A # on bmc board
I2C_ADDR_QSFP_EEPROM=0x50
I2C_ADDR_SFP_EEPROM=0x50
I2C_ADDR_CPLD=0x33
I2C_ADDR_ROV=0x76

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

# qsfp port number range
MIN_QSFP_PORT_NUM=1
MAX_QSFP_PORT_NUM=64

# sfp+ port number range
MIN_SFP_PORT_NUM=1
MAX_SFP_PORT_NUM=2

# CPLD access
# ROV status
CPLD_ROV_STATUS_KEY=cpld_rov_status
# port status
CPLD_QSFP_STATUS_KEY=cpld_qsfp_port_status
CPLD_SFP_STATUS_KEY=cpld_sfp_port_status
# bit define
CPLD_QSFP_STATUS_ABS_BIT=1
CPLD_SFP0_STATUS_PRES_BIT=0
CPLD_SFP1_STATUS_PRES_BIT=4

# fp port to phy port mapping
fp2phy_array=( 0  1  4  5  8  9 12 13 16 17 20 21 24 25 28 29
              32 33 36 37 40 41 44 45 48 49 52 53 56 57 60 61
               2  3  6  7 10 11 14 15 18 19 22 23 26 27 30 31
              34 35 38 39 42 43 46 47 50 51 54 55 58 59 62 63)
# fp port to led port mapping
fp2led_array=( 1  2  5  6  9 10 13 14  1  2  5  6  9 10 13 14
               1  2  5  6  9 10 13 14  1  2  5  6  9 10 13 14
               3  4  7  8 11 12 15 16  3  4  7  8 11 12 15 16
               3  4  7  8 11 12 15 16  3  4  7  8 11 12 15 16)
# vdd value for mac
rov_val_array=( 0.85 0.82 0.77 0.87 0.74 0.84 0.79 0.89 )
rov_reg_array=( 0x79 0x73 0x69 0x7D 0x63 0x77 0x6D 0x81 )

# Help usage function
function _help {
    echo "========================================================="
    echo "# Description: Help Function"
    echo "========================================================="
    echo "----------------------------------------------------"
    echo "EX       : ${0} help"
    echo "         : ${0} i2c_init"
    echo "         : ${0} i2c_deinit"
    echo "         : ${0} i2c_fan_speed_init"
    echo "         : ${0} i2c_io_exp_init"
    echo "         : ${0} i2c_led_test"
    echo "         : ${0} i2c_psu_eeprom_get"
    echo "         : ${0} i2c_mb_eeprom_get"
    echo "         : ${0} i2c_cb_eeprom_get"
    echo "         : ${0} i2c_qsfp_eeprom_get [${MIN_QSFP_PORT_NUM}-${MAX_QSFP_PORT_NUM}]"
    echo "         : ${0} i2c_sfp_eeprom_get [${MIN_SFP_PORT_NUM}-${MAX_SFP_PORT_NUM}]"
    echo "         : ${0} i2c_qsfp_eeprom_init new|delete"
    echo "         : ${0} i2c_sfp_eeprom_init new|delete"
    echo "         : ${0} i2c_mb_eeprom_init new|delete"
    echo "         : ${0} i2c_cb_eeprom_init new|delete"
    echo "         : ${0} i2c_qsfp_status_get [${MIN_QSFP_PORT_NUM}-${MAX_QSFP_PORT_NUM}]"
    echo "         : ${0} i2c_sfp_status_get [${MIN_SFP_PORT_NUM}-${MAX_SFP_PORT_NUM}]"
    echo "         : ${0} i2c_qsfp_type_get [${MIN_QSFP_PORT_NUM}-${MAX_QSFP_PORT_NUM}]"
    echo "         : ${0} i2c_sfp_type_get [${MIN_SFP_PORT_NUM}-${MAX_SFP_PORT_NUM}]"
    echo "         : ${0} i2c_qsfp_ddm_get [${MIN_QSFP_PORT_NUM}-${MAX_QSFP_PORT_NUM}]"
    echo "         : ${0} i2c_board_type_get"
    echo "         : ${0} i2c_bmc_board_type_get"
    echo "         : ${0} i2c_cpld_version"
    echo "         : ${0} i2c_psu_status"
    echo "         : ${0} i2c_led_psu_status_set"
    echo "         : ${0} i2c_led_fan_status_set"
    echo "         : ${0} i2c_led_fan_tray_status_set"
    echo "         : ${0} i2c_test_all"
    echo "         : ${0} i2c_sys_led green|amber"
    echo "         : ${0} i2c_fan_led green|amber|off"
    echo "         : ${0} i2c_psu1_led green|amber|off"
    echo "         : ${0} i2c_psu2_led green|amber|off"
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
       echo "${*}"
       eval "${*}" && break || echo "retry"; sleep 1;
    done
}

# front panel port to physical port mapping
function _port_fp2phy {

    local fp_port=$1
    local phy_port=0
    local index=$((${fp_port} - 1))
    # change to 1 base index for internal use
    phy_port=$((${fp2phy_array[$index]} + 1))
    #phy_port=${fp2phy_array[$index]}

    echo $phy_port
}

#front panel port to led port mapping
function _port_fp2led {

    local logic_port=$1
    local led_port=0
    led_port=${fp2led_array[$logic_port]}

    echo $led_port
}

#I2C Init
function _i2c_init {
    echo "========================================================="
    echo "# Description: I2C Init"
    echo "========================================================="

    #remove optoe module
    rmmod optoe

    #rmmod i2c_ismt
    _util_rmmod i2c_i801
    modprobe i2c_i801
    modprobe i2c_dev
    modprobe i2c_mux_pca954x force_deselect_on_exit=1

    # add MUX PCA9548#0 on I801, assume to be i2c-1~8
    if [ ! -e ${PATH_MUX_9548_0_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_0}' > ${PATH_I801_DEVICE}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_0} already init."
    fi

    # add MUX PCA9548#1 on I801, assume to be i2c-9~16
    if [ ! -e ${PATH_MUX_9548_1_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_1}' > ${PATH_I801_DEVICE}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_1} already init."
    fi
    
    # add MUX PCA9546#0 on I801, assume to be i2c-17~20
    if [ ! -e ${PATH_MUX_9546_0_CH0} ]; then
        _retry "echo 'pca9546 ${I2C_ADDR_MUX_9546_0}' > ${PATH_I801_DEVICE}/new_device"
    else
        echo "pca9546 ${I2C_ADDR_MUX_9546_0} already init."
    fi

    # add MUX PCA9548#2 on I801, assume to be i2c-21~28
    if [ ! -e ${PATH_MUX_9548_2_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_2}' > ${PATH_MUX_9546_0_CH2}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_2} already init."
    fi    

    # add MUX PCA9546#1 on I801, assume to be i2c-29~32
    if [ ! -e ${PATH_MUX_9546_1_CH0} ]; then
        _retry "echo 'pca9546 ${I2C_ADDR_MUX_9546_1}' > ${PATH_MUX_9546_0_CH3}/new_device"
    else
        echo "pca9546 ${I2C_ADDR_MUX_9546_1} already init."
    fi

    # add MUX PCA9548#11 on I801, assume to be i2c-33~40
    if [ ! -e ${PATH_MUX_9548_11_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_11}' > ${PATH_I801_DEVICE}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_11} already init."
    fi

    # add MUX PCA9548#3 on PCA9548#2 CH0, assume to be i2c-41~48
    if [ ! -e ${PATH_MUX_9548_3_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_3}' > ${PATH_MUX_9548_2_CH0}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_4} already init."
    fi

    # add MUX PCA9548#4 on PCA9548#2 CH1, assume to be i2c-49~56
    if [ ! -e ${PATH_MUX_9548_4_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_4}' > ${PATH_MUX_9548_2_CH1}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_4} already init."
    fi

    # add MUX PCA9548#5 on PCA9548#2 CH2, assume to be i2c-57~64
    if [ ! -e ${PATH_MUX_9548_5_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_5}' > ${PATH_MUX_9548_2_CH2}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_5} already init."
    fi

    # add MUX PCA9548#6 on PCA9548#2 CH3, assume to be i2c-65~72
    if [ ! -e ${PATH_MUX_9548_6_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_6}' > ${PATH_MUX_9548_2_CH3}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_6} already init."
    fi

    # add MUX PCA9548#7 on PCA9548#2 CH4, assume to be i2c-73~80
    if [ ! -e ${PATH_MUX_9548_7_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_7}' > ${PATH_MUX_9548_2_CH4}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_7} already init."
    fi

    # add MUX PCA9548#8 on PCA9548#2 CH5, assume to be i2c-81~88
    if [ ! -e ${PATH_MUX_9548_8_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_8}' > ${PATH_MUX_9548_2_CH5}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_8} already init."
    fi

    # add MUX PCA9548#9 on PCA9548#2 CH6, assume to be i2c-89~96
    if [ ! -e ${PATH_MUX_9548_9_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_9}' > ${PATH_MUX_9548_2_CH6}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_9} already init."
    fi

    # add MUX PCA9548#10 on PCA9548#2 CH7, assume to be i2c-97~104
    if [ ! -e ${PATH_MUX_9548_10_CH0} ]; then
        _retry "echo 'pca9548 ${I2C_ADDR_MUX_9548_10}' > ${PATH_MUX_9548_2_CH7}/new_device"
    else
        echo "pca9548 ${I2C_ADDR_MUX_9548_10} already init."
    fi

    _i2c_hwm_init
    _util_rmmod eeprom
    modprobe eeprom_mb
    modprobe gpio-pca953x
    _i2c_io_exp_init
    _i2c_sensors_init
    _i2c_cpld_init
    _i2c_psu_init

    _i2c_qsfp_eeprom_init "new"
    _i2c_sfp_eeprom_init "new"
    _i2c_mb_eeprom_init "new"
    _i2c_cb_eeprom_init "new"
    _i2c_fan_speed_init
    _i2c_led_psu_status_set
    _i2c_led_fan_status_set

    # clear port led
    _util_port_led_clear

    # rov for mac init
    _mac_vdd_init

    # trun on sys led
    echo "led_sys setup..."
    COLOR_LED="green"
    ONOFF_LED="on"
    echo "${COLOR_LED} ${ONOFF_LED}"
    _i2c_sys_led

    _config_rmem
}

function _mac_vdd_init {
    # read mac vid register value from CPLD
    cpld_index=1
    _i2c_cpld_reg_read ${cpld_index} ${CPLD_ROV_STATUS_KEY}
    # get vid form register value [0:2]
    vid=$(($cpld_reg_val & 0x7))
    # get rov val and reg according to vid
    rov_val=${rov_val_array[$vid]}
    rov_reg=${rov_reg_array[$vid]}
    echo "vid=${vid}, rov_val=${rov_val}, rov_reg=${rov_reg}"

    # write the rov reg to rov
    i2cset -y -r ${I2C_BUS_ROV} ${I2C_ADDR_ROV} 0x21 ${rov_reg} w

    if [ $? -eq 0 ]; then
        echo "set ROV for mac vdd done"
    else
        echo "set ROV for mac vdd fail"
    fi
}

#I2C Deinit
function _i2c_deinit {
    echo "i2c deinit..."
    for mod in coretemp jc42 w83795 ingrasys_s9280_64x_psu ingrasys_s9280_64x_i2c_cpld eeprom eeprom_mb gpio-pca953x i2c_mux_pca954x i2c_i801;
    do
        _util_rmmod $mod
    done
    echo "Done"
}

function _i2c_cpld_init {
    echo "CPLD init..."
    _util_rmmod ingrasys_s9280_64x_i2c_cpld
    modprobe ingrasys_s9280_64x_i2c_cpld

    # add cpld 1~5 to sysfs
    for i in {1..5};
    do
        local cpld_bus="I2C_BUS_CPLD${i}"
        local cpld_path="PATH_CPLD${i}_DEVICE"
        dev_path="${PATH_SYS_I2C_DEVICES}/${!cpld_bus}-$(printf "%04x" ${I2C_ADDR_CPLD})"
        if ! [ -L ${dev_path} ]; then
            echo "ingrasys_cpld${i} ${I2C_ADDR_CPLD}" > ${!cpld_path}/new_device
        else
            echo "${dev_path} already exist"
        fi
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
    modprobe coretemp #hwmon0
    # probe hwmon kernel module
    modprobe w83795
    # add w83795 to sysfs
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_1_CH7}-$(printf "%04x" ${I2C_ADDR_W83795})"
    if ! [ -L ${dev_path} ]; then
        echo "w83795adg ${I2C_ADDR_W83795}" > ${PATH_MUX_9548_1_CH7}/new_device #hwmon1
        sleep 0.1
    else
        echo "${dev_path} already exist"
    fi
    # add lm75/lm86 to sysfs
    ####Main board thermal
    ####lm75_1
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_0_CH5}-$(printf "%04x" ${I2C_ADDR_LM75_1})"
    if ! [ -L ${dev_path} ]; then
        echo "lm75 ${I2C_ADDR_LM75_1}" > ${PATH_MUX_9548_0_CH5}/new_device    # hwmon2
        sleep 0.1
    else
        echo "${dev_path} already exist"
    fi
    ####lm75_2
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_0_CH5}-$(printf "%04x" ${I2C_ADDR_LM75_2})"
    if ! [ -L ${dev_path} ]; then
        echo "lm75 ${I2C_ADDR_LM75_2}" > ${PATH_MUX_9548_0_CH5}/new_device    #hwmon3
        sleep 0.1
    else
        echo "${dev_path} already exist"
    fi
    ####lm86
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_0_CH5}-$(printf "%04x" ${I2C_ADDR_LM86})"
    if ! [ -L ${dev_path} ]; then
        echo "lm86 ${I2C_ADDR_LM86}" > ${PATH_MUX_9548_0_CH5}/new_device    #hwmon4
        sleep 0.1
    else
        echo "${dev_path} already exist"
    fi
    ####lm75_3
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_0_CH6}-$(printf "%04x" ${I2C_ADDR_LM75_3})"
    if ! [ -L ${dev_path} ]; then
        echo "lm75 ${I2C_ADDR_LM75_3}" > ${PATH_MUX_9548_0_CH6}/new_device    # hwmon5
        sleep 0.1
    else
        echo "${dev_path} already exist"
    fi
    ####lm75_4
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_0_CH6}-$(printf "%04x" ${I2C_ADDR_LM75_4})"
    if ! [ -L ${dev_path} ]; then
        echo "lm75 ${I2C_ADDR_LM75_4}" > ${PATH_MUX_9548_0_CH6}/new_device    #hwmon6
        sleep 0.1
    else
        echo "${dev_path} already exist"
    fi
    ####BMC board thermal
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_MUX_9548_1_CH7}-$(printf "%04x" ${I2C_ADDR_TMP75_BB})"
    if ! [ -L ${dev_path} ]; then
        echo "tmp75 ${I2C_ADDR_TMP75_BB}" > ${PATH_MUX_9548_1_CH7}/new_device #hwmon7
        sleep 0.1
    else
        echo "${dev_path} already exist"
    fi
    ####CPU board thermal
    dev_path="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_MAIN}-$(printf "%04x" ${I2C_ADDR_TMP75_CB})"
    if ! [ -L ${dev_path} ]; then
        echo "tmp75 ${I2C_ADDR_TMP75_CB}" > ${PATH_I801_DEVICE}/new_device #hwmon8
        sleep 0.1
    else
        echo "${dev_path} already exist"
    fi

    # probe jc42 kernel module
    modprobe jc42 #hwmon9

    echo "Done"
}

#FAN Speed Init
function _i2c_fan_speed_init {
    echo -n "FAN SPEED INIT..."
    if [ -e "${PATH_HWMON_W83795_DEVICE}" ]; then
        # init fan speed
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

#IO Expander Init
function _i2c_io_exp_init {
    echo "========================================================="
    echo "# Description: I2C IO Expender Init"
    echo "========================================================="

     # need to init BMC io expander first due to some io expander are reset default
    echo "Init BMC INT & HW ID IO Expander"
    # all input
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_1} ${REG_PORT0_DIR} 0xFF
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_1} ${REG_PORT1_DIR} 0xFF
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_1} ${REG_PORT0_POL} 0x00
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_1} ${REG_PORT1_POL} 0x00

    echo "Init BMC PSU status IO Expander"
    # PWRON default  0 (ACTIVE_LOW)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_2} ${REG_PORT0_OUT} 0x00
    # default 0 (ACTIVE_LOW)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_2} ${REG_PORT1_OUT} 0x00
    # I/O 0.2 0.5 output(PWRON), rest input
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_2} ${REG_PORT0_DIR} 0xDB
    # I/O 1.0~1.1 input, 1.2~1.4 output (1.5~1.7 not enable)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_2} ${REG_PORT1_DIR} 0xE3
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_2} ${REG_PORT0_POL} 0x00
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_2} ${REG_PORT1_POL} 0x00

    echo "Init BMC RST and SEL IO Expander"
    # RST default is 1 (ACTIVE_LOW)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_3} ${REG_PORT0_OUT} 0x3F
    # SEL default is 0 (HOST), EN default is 1 (ACTIVE_HIGH)
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_3} ${REG_PORT1_OUT} 0x1F
    # I/O 0.0~0.5 output, 0.6~0.7 not use
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_3} ${REG_PORT0_DIR} 0xD0
    # all output
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_3} ${REG_PORT1_DIR} 0x00
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_3} ${REG_PORT0_POL} 0x00
    _util_i2cset -y -r ${I2C_BUS_MAIN} ${I2C_ADDR_MUX_9555_3} ${REG_PORT1_POL} 0x00

    echo "Init System LED & HW ID IO Expander"
    # I/O_0.x for System LED default 0, I/O_1.x for HW ID
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT0_OUT} 0x00
    # System LED => all output
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT0_DIR} 0x00
    # HW ID => all input
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT1_DIR} 0xFF
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT0_POL} 0x00
    _util_i2cset -y -r ${I2C_BUS_SYS_LED} ${I2C_ADDR_MUX_9539_0} ${REG_PORT1_POL} 0x00

    echo "Init FAN Board Status IO Expander"
    # LED_G_H set to 1, LED_Y_G set to 0  (ACTIVE_HIGH)
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9555_0} ${REG_PORT0_OUT} 0x11
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9555_0} ${REG_PORT1_OUT} 0x11
    # DIR/ABS is input, LED_Y/LED_G is output
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9555_0} ${REG_PORT0_DIR} 0xCC
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9555_0} ${REG_PORT1_DIR} 0xCC
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9555_0} ${REG_PORT0_POL} 0x00
    _util_i2cset -y -r ${I2C_BUS_FAN_STATUS} ${I2C_ADDR_MUX_9555_0} ${REG_PORT1_POL} 0x00

    echo "Init System SEL and RST IO Expander"
    # default 0 (ACTIVE high)
    # 0.4~0.5 SEL set to value 0 (host)
    # 0.2 LED_CLR set to 0 will clear all switch LED bitmap, set to 1 here
    _util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT0_OUT} 0x04
    #  RST 1.6~1.7 default 1 (ACTIVE low),  INT 1.0~1.4 default 1 (ACTIVE low)
    _util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT1_OUT} 0xDF
    # all output, but ISO_TF_CORE_RST_N 0.0 need to set as input to prevent reboot issue
    _util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT0_DIR} 0x09
    # RST 1.6~1.7 output, rest are input
    _util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT1_DIR} 0x3F
    _util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT0_POL} 0x00
    _util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT1_POL} 0x00

    echo "Init CPU CPLD IO Expander (FIXME)"
    # all input
    #echo "NUM_MUX_9546_1_CH3=${NUM_MUX_9546_1_CH3}, I2C_ADDR_MUX_9539_2=${I2C_ADDR_MUX_9539_2}"
    #_util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT0_DIR} 0xFF
    # all input
    #_util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT1_DIR} 0xFF
    #_util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT0_POL} 0x00
    #_util_i2cset -y -r ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_2} ${REG_PORT1_POL} 0x00
}

#Set FAN Tray LED
function _i2c_led_fan_tray_status_set {
    echo "FAN Tray Status Setup"
    #FAN Status get
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`
    FAN2_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN4_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`

    # check if io expander for fan tray exist
    result=`i2cget -y ${I2C_BUS_FANTRAY_LED} ${I2C_ADDR_MUX_9555_0} ${REG_PORT0_IN} 2>/dev/null`
    err_code=$?
    if [ "$err_code" != "0" ]; then
        echo "fan tray not exist!"
        return
    fi

    for FAN_TRAY in {1..4};
    do
        FAN_ALARM="FAN${FAN_TRAY}_ALARM"
        if [ "${!FAN_ALARM}" == "0" ]; then
            COLOR_LED="green"
            ONOFF_LED="on"
            _i2c_fan_tray_led

            COLOR_LED="amber"
            ONOFF_LED="off"
            _i2c_fan_tray_led

            echo "set [FAN TRAY ${FAN_TRAY}] [Green]=on [Amber]=off"
        else
            COLOR_LED="green"
            ONOFF_LED="off"
            _i2c_fan_tray_led

            COLOR_LED="amber"
            ONOFF_LED="on"
            _i2c_fan_tray_led

            echo "set [FAN TRAY ${FAN_TRAY}] [Green]=off [Amber]=on"
        fi
    done
}

#Set FAN LED
function _i2c_led_fan_status_set {
    echo "FAN Status Setup"
    #PSU Status set
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN5_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN7_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`

    echo "led_fan setup..."
    # all fan ok
    if [ "${FAN1_ALARM}" == "0" ] \
       && [ "${FAN3_ALARM}" == "0" ] \
       && [ "${FAN5_ALARM}" == "0" ] \
       && [ "${FAN7_ALARM}" == "0" ]; then
        COLOR_LED="green"
        echo "${COLOR_LED}"
        _i2c_fan_led
    # all fan fail
    elif [  "${FAN1_ALARM}" == "1" ] \
       && [ "${FAN3_ALARM}" == "1" ] \
       && [ "${FAN5_ALARM}" == "1" ] \
       && [ "${FAN7_ALARM}" == "1" ] ; then
        COLOR_LED="amber"
        echo "${COLOR_LED}"
        _i2c_fan_led
    # partial fan fail
    else
        COLOR_LED="amber"
        echo "${COLOR_LED}"
        _i2c_fan_led
    fi
    # if fan not present, no led need to be set
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
            echo "${COLOR_LED}"
            _i2c_psu1_led
        else
            COLOR_LED="amber"
            echo "${COLOR_LED}"
            _i2c_psu1_led
        fi
    else
        COLOR_LED="off"
        echo "${COLOR_LED}"
        _i2c_psu1_led
    fi

    #PSU2 Status
    echo "led_psu2 setup..."
    if [ "${psu2Exist}" == ${PSU_EXIST} ]; then
        if [ "${psu2PwGood}" == ${PSU_DC_ON} ]; then
            COLOR_LED="green"
            echo "${COLOR_LED}"
            _i2c_psu2_led
        else
            COLOR_LED="amber"
            echo "${COLOR_LED}"
            _i2c_psu2_led
        fi
    else
        COLOR_LED="off"
        echo "${COLOR_LED}"
        _i2c_psu2_led
    fi
}

#LED Test
function _i2c_led_test {
    echo "========================================================="
    echo "# Description: I2C SYSTEM LED TEST..."
    echo "========================================================="

    #sys led (green)
    COLOR_LED="green"
    _i2c_sys_led
    _pause 'Check SYS LED green light and Press [Enter] key to continue...'
    #sys led (amber)
    COLOR_LED="amber"
    _i2c_sys_led
    _pause 'Check SYS LED amber light and Press [Enter] key to continue...'

    #FAN led (green)
    COLOR_LED="green"
    _i2c_fan_led
    _pause 'Check FAN LED green light and Press [Enter] key to continue...'
    #FAN led (amber)
    COLOR_LED="amber"
    _i2c_fan_led
    _pause 'Check FAN LED amber light and Press [Enter] key to continue...'

    #PSU1 led (green)
    COLOR_LED="green"
    _i2c_psu1_led
    _pause 'Check PSU1 LED green light and Press [Enter] key to continue...'
    #PSU1 led (amber)
    COLOR_LED="amber"
    _i2c_psu1_led
    _pause 'Check PSU1 LED amber light and Press [Enter] key to continue...'

    #PSU2 led (green)
    COLOR_LED="green"
    _i2c_psu2_led
    _pause 'Check PSU2 LED green light and Press [Enter] key to continue...'
    #PSU2 led (amber)
    COLOR_LED="amber"
    _i2c_psu2_led
    _pause 'Check PSU2 LED amber light and Press [Enter] key to continue...'

    #Turn OFF All LED (can't trun off system led)
    COLOR_LED="off"
    #_i2c_sys_led
    _i2c_fan_led
    _i2c_psu1_led
    _i2c_psu2_led
    _pause 'Check turn off all LEDs (exclude SYS LED) and Press [Enter] key to continue...'

    # restore sys led
    COLOR_LED="green"
    _i2c_sys_led
}

#Set QSFP Port cpld variable
function _qsfp_cpld_var_set {
    local port=$1
    local reg_port_base
    local reg_port_shift
    
    if [[ $1 -le 12  && $1 -ge 1 ]]; then
        cpld_index=1
        reg_port_base=0
    elif [[ $1 -le 25  && $1 -ge 13 ]]; then
        cpld_index=2
        reg_port_base=12
    elif [[ $1 -le 38  && $1 -ge 26 ]]; then
        cpld_index=3
        reg_port_base=25
    elif [[ $1 -le 51  && $1 -ge 39 ]]; then
        cpld_index=4
        reg_port_base=38
    elif [[ $1 -le 64  && $1 -ge 52 ]]; then
        cpld_index=5
        reg_port_base=51
    else
        echo "invalid port number"
    fi

     cpld_port_index=$(( $port - $reg_port_base ))
}

#Set QSFP Port eeporm variable
function _qsfp_eeprom_var_set {
    local port=$1
    local eeprombusbase
    local eeprombusshift
    # port 1 => zqsfp0
    # port 2 => zqsfp1
    # ...
    local port_group=$(( ($port - 1) / 8 ))

    case ${port_group} in
        0)
            eeprombusbase=${NUM_MUX_9548_3_CH0}
        ;;
        1)
            eeprombusbase=${NUM_MUX_9548_4_CH0}
        ;;
        2)
            eeprombusbase=${NUM_MUX_9548_5_CH0}
        ;;
        3)
            eeprombusbase=${NUM_MUX_9548_6_CH0}
        ;;
        4)
            eeprombusbase=${NUM_MUX_9548_7_CH0}
        ;;
        5)
            eeprombusbase=${NUM_MUX_9548_8_CH0}
        ;;
        6)
            eeprombusbase=${NUM_MUX_9548_9_CH0}
        ;;
        7)
            eeprombusbase=${NUM_MUX_9548_10_CH0}
        ;;
        *)
        ;;
    esac

    eeprombusshift=$(( (${port} - 1) % 8))
    eepromBus=$(( ${eeprombusbase} + ${eeprombusshift} ))
    eepromAddr=${I2C_ADDR_QSFP_EEPROM}
}


#Set SFP Port cpld variable
function _sfp_cpld_var_set {
    local port=$1
    case ${port} in
        1)
            cpld_index=1
        ;;
        2)
            cpld_index=1
        ;;
        *)
        ;;
    esac
}

#Set QSFP Port eeporm variable
function _sfp_eeprom_var_set {
    local port=$1

    case ${port} in
        1)
            eepromBus=${NUM_MUX_9546_1_CH0}
        ;;
        2)
            eepromBus=${NUM_MUX_9546_1_CH1}
        ;;
        *)
        ;;
    esac

    eepromAddr=${I2C_ADDR_SFP_EEPROM}
}

#Get QSFP EEPROM Information
function _i2c_qsfp_eeprom_get {
    local phy_port=0

    # input parameter validation
    _util_input_check "${QSFP_PORT}" "${MIN_QSFP_PORT_NUM}" "${MAX_QSFP_PORT_NUM}"

    #get physical port
    phy_port=$(_port_fp2phy $QSFP_PORT)

    _qsfp_eeprom_var_set ${phy_port}

    _util_get_qsfp_abs

    if [ $status = 0 ]; then
        exit
    fi

    # debug
    echo "${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf '%04x' $eepromAddr)/eeprom | hexdump -C"
    cat ${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr)/eeprom | hexdump -C
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

    #Init 1-64 ports EEPROM
    local i
    for i in {1..64};
    do

        #get physical port
        phy_port=$(_port_fp2phy $i)

        _qsfp_eeprom_var_set ${phy_port}

        if [ "${action}" == "new" ] && \
           ! [ -L ${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr) ]; then
            #echo "sff8436 $eepromAddr" > ${PATH_SYS_I2C_DEVICES}/i2c-$eepromBus/new_device
            echo "optoe1 $eepromAddr" > ${PATH_SYS_I2C_DEVICES}/i2c-$eepromBus/new_device
        elif [ "${action}" == "delete" ] && \
             [ -L ${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr) ]; then
            echo "$eepromAddr" > ${PATH_SYS_I2C_DEVICES}/i2c-$eepromBus/delete_device
        fi
    done
    echo "Done"
}

#Get SFP EEPROM Information
function _i2c_sfp_eeprom_get {

    # input parameter validation
    _util_input_check "${SFP_PORT}" "${MIN_SFP_PORT_NUM}" "${MAX_SFP_PORT_NUM}"
    _util_get_sfp_pres

    if [ $status = 0 ]; then
        exit
    fi

    _sfp_eeprom_var_set ${SFP_PORT}

    cat ${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr)/eeprom | hexdump -C
}

#Init SFP EEPROM
function _i2c_sfp_eeprom_init {
    echo "SFP EEPROM INIT..."

    #Action check
    action=$1
    if [ -z "${action}" ]; then
        echo "No action, skip"
        return
    elif [ "${action}" != "new" ] && [ "${action}" != "delete" ]; then
        echo "Error action, skip"
        return
    fi

    #Init 1-2 sfp+ EEPROM
    local i
    for i in {1..2};
    do
        _sfp_eeprom_var_set ${i}

        if [ "${action}" == "new" ] && \
           ! [ -L ${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr) ]; then
            #echo "sff8436 $eepromAddr" > ${PATH_SYS_I2C_DEVICES}/i2c-$eepromBus/new_device
            echo "optoe1 $eepromAddr" > ${PATH_SYS_I2C_DEVICES}/i2c-$eepromBus/new_device
        elif [ "${action}" == "delete" ] && \
             [ -L ${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr) ]; then
            echo "$eepromAddr" > ${PATH_SYS_I2C_DEVICES}/i2c-$eepromBus/delete_device
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

#Init PSU Kernel Module
function _i2c_psu_init {
    echo "========================================================="
    echo "# Description: I2C PSU Init"
    echo "========================================================="
    modprobe ingrasys_s9280_64x_psu

    echo "psu1 ${I2C_ADDR_PSU1_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU1_EEPROM}/new_device
    echo "psu2 ${I2C_ADDR_PSU2_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU2_EEPROM}/new_device
}

#Deinit PSU Kernel Module
function _i2c_psu_deinit {
    echo "${I2C_ADDR_PSU1_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU1_EEPROM}/delete_device
    echo "${I2C_ADDR_PSU2_EEPROM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU2_EEPROM}/delete_device
    _util_rmmod ingrasys_s9280_64x_psu
}

#get QSFP Status
function _i2c_qsfp_status_get {

    # input parameter validation
    _util_input_check "${QSFP_PORT}" "${MIN_QSFP_PORT_NUM}" "${MAX_QSFP_PORT_NUM}"

    local stat
    _util_get_qsfp_abs
    echo "status=$status"
}

#get QSFP Type
function _i2c_qsfp_type_get {
    local phy_port=0

    phy_port=$(_port_fp2phy ${QSFP_PORT})

    # input parameter validation
    _util_input_check "${QSFP_PORT}" "${MIN_QSFP_PORT_NUM}" "${MAX_QSFP_PORT_NUM}"

    _qsfp_eeprom_var_set ${phy_port}

    #Get QSFP EEPROM info
    local size=255
    eeprom_path="${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr)/eeprom"
    #echo "get ${eeprom_path}"
    qsfp_info=$(dd if=${eeprom_path} bs=${size} count=1 2>/dev/null | base64)

    identifier=$(echo $qsfp_info | base64 -d -i | hexdump -s 128 -n 1 -e '"%x"')
    connector=$(echo $qsfp_info | base64 -d -i | hexdump -s 130 -n 1 -e '"%x"')
    transceiver=$(echo $qsfp_info | base64 -d -i | hexdump -s 131 -n 1 -e '"%x"')

    echo "identifier=$identifier"
    echo "connector=$connector"
    echo "transceiver=$transceiver"
}

function _i2c_sfp_status_get {

    # input parameter validation
    _util_input_check "${SFP_PORT}" "${MIN_SFP_PORT_NUM}" "${MAX_SFP_PORT_NUM}"

    local stat
    _util_get_sfp_pres
    #status: 0 -> Down, 1 -> Up
    if [ $status = 0 ]; then
        stat="down"
    else
        stat="up"
    fi
    echo "status is $stat"
}

#get SFP Type
function _i2c_sfp_type_get {

    # input parameter validation
    _util_input_check "${SFP_PORT}" "${MIN_SFP_PORT_NUM}" "${MAX_SFP_PORT_NUM}"

    _sfp_eeprom_var_set ${SFP_PORT}

    #Get QSFP EEPROM info
    sfp_info=$(base64 ${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr)/eeprom)

    identifier=$(echo $sfp_info | base64 -d -i | hexdump -s 128 -n 1 -e '"%x"')
    connector=$(echo $sfp_info | base64 -d -i | hexdump -s 130 -n 1 -e '"%x"')
    transceiver=$(echo $sfp_info | base64 -d -i | hexdump -s 131 -n 1 -e '"%x"')

    echo "identifier=$identifier"
    echo "connector=$connector"
    echo "transceiver=$transceiver"
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
    if [ "${COLOR_LED}" == "green" ]; then
        # set fan_led_en (0.6) = 1 & fan_led_y (0.5) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x60
        value=0x40
    elif [ "${COLOR_LED}" == "amber" ]; then
        # set fan_led_en (0.6) = 1 & fan_led_y (0.5) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x60
        value=0x60
    elif [ "${COLOR_LED}" == "off" ]; then
        # set fan_led_en (0.6) = 0 & fan_led_y (0.5) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x60
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

#Set PSU1 LED
function _i2c_psu1_led {
    if [ "${COLOR_LED}" == "green" ]; then
        # set psu1_pwr_ok_oe (0.4) = 1 & psu1_led_y (0.3) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x18
        value=0x10
    elif [ "${COLOR_LED}" == "amber" ]; then
        # set psu1_pwr_ok_oe (0.4) = 1 & psu1_led_y (0.3) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x18
        value=0x18
    elif [ "${COLOR_LED}" == "off" ]; then
        # set psu1_pwr_ok_oe (0.4) = 0 & psu1_led_y (0.3) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x18
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

#Set PSU2 LED
function _i2c_psu2_led {
    if [ "${COLOR_LED}" == "green" ]; then
        # set psu0_pwr_ok_oe (0.2) = 1 & psu0_led_y (0.1) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x06
        value=0x04
    elif [ "${COLOR_LED}" == "amber" ]; then
        # set psu0_pwr_ok_oe (0.2) = 1 & psu0_led_y (0.1) = 1
        output_reg=${REG_PORT0_OUT}
        mask=0x06
        value=0x06
    elif [ "${COLOR_LED}" == "off" ]; then
        # set psu0_pwr_ok_oe (0.2) = 0 & psu0_led_y (0.1) = 0
        output_reg=${REG_PORT0_OUT}
        mask=0x06
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

#Set FAN Tray LED
function _i2c_fan_tray_led {

    i2cAddr=${I2C_ADDR_MUX_9555_0}
    output_reg=${REG_PORT0_OUT}

    case ${FAN_TRAY} in
        1)
            output_reg=${REG_PORT0_OUT}
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x01
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x02
            fi
            ;;
        2)
            output_reg=${REG_PORT0_OUT}
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x10
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x20
            fi
            ;;
        3)
            output_reg=${REG_PORT1_OUT}
            if [ "${COLOR_LED}" == "green" ]; then
                mask=0x01
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=0x02
            fi
            ;;
        4)
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

    # LED PIN value is ACTIVE HIGH
    if [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "on" ]; then
        _util_i2cset -m $mask -y -r ${I2C_BUS_FANTRAY_LED} $i2cAddr ${output_reg} 0xff
    elif [ "${COLOR_LED}" == "green" ] && [ "${ONOFF_LED}" == "off" ]; then
        _util_i2cset -m $mask -y -r ${I2C_BUS_FANTRAY_LED} $i2cAddr ${output_reg} 0x00
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "on" ]; then
        _util_i2cset -m $mask -y -r ${I2C_BUS_FANTRAY_LED} $i2cAddr ${output_reg} 0xff
    elif [ "${COLOR_LED}" == "amber" ] && [ "${ONOFF_LED}" == "off" ]; then
        _util_i2cset -m $mask -y -r ${I2C_BUS_FANTRAY_LED} $i2cAddr ${output_reg} 0x00
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
    printf "MAIN_BOARD BOARD_ID is 0x%02x, HW Rev 0x%02x, Build Rev 0x%02x\n" $boardId $boardHwRev $boardBuildRev
}

#Get BMC Board Version and Type
function _i2c_bmc_board_type_get {
    # read input port 1 value from io expander
    input_reg=${REG_PORT1_IN}
    boardType=`i2cget -y ${I2C_BUS_BMC_HW_ID} ${I2C_ADDR_MUX_9555_1} ${input_reg}`
    boardBuildRev=$((($boardType) & 0x03))
    boardHwRev=$((($boardType) >> 2 & 0x03))
    boardId=$((($boardType) >> 4))
    printf "BMC_BOARD BOARD_ID is 0x%02x, HW Rev 0x%02x, Build Rev 0x%02x\n" $boardId $boardHwRev $boardBuildRev
}

#Get CPLD Version
function _i2c_cpld_version {
    echo "========================================================="
    echo "# Description: CPLD Version"
    echo "========================================================="

    for i in {1..5};
    do
        local cpld_bus="I2C_BUS_CPLD${i}"
        local cpld_path="PATH_CPLD${i}_DEVICE"
        local file_path="${PATH_SYS_I2C_DEVICES}/${!cpld_bus}-$(printf "%04x" ${I2C_ADDR_CPLD})/cpld_version"
        printf "[CPLD %d] %s\n" ${i}  $(cat ${file_path})
    done
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

#Get register value from CPLD
function _i2c_cpld_reg_read {
    local idx=$1
    local file_name=$2
    local cpld_i2c_addr=${I2C_ADDR_CPLD}
    local reg_file_path

    case ${idx} in
        1)
        cpld_i2c_bus=${I2C_BUS_CPLD1}
            ;;
        2)
        cpld_i2c_bus=${I2C_BUS_CPLD2}
            ;;
        3)
        cpld_i2c_bus=${I2C_BUS_CPLD3}
            ;;
        4)
        cpld_i2c_bus=${I2C_BUS_CPLD4}
            ;;
        5)
        cpld_i2c_bus=${I2C_BUS_CPLD5}
            ;;
        *)
            echo "invalid cpld index"
            exit
            ;;
    esac

    reg_file_path="${PATH_SYS_I2C_DEVICES}/${cpld_i2c_bus}-$(printf "%04x" ${cpld_i2c_addr})/${file_name}"
    cpld_reg_val=`cat ${reg_file_path}`
}

#util functions
function _util_i2cset {
    if [ "$DEBUG" == "on" ]; then
        i2cset $@
    else
        i2cset $@ 1>/dev/null
    fi
}

function _i2c_set {
    local i2c_bus=$1
    local i2c_addr=$2
    local reg=$3
    local mask=$4
    local value=$5

    echo `i2cset -m $mask -y -r ${i2c_bus} ${i2c_addr} ${reg} ${value}`
}

function _util_rmmod {
    local mod=$1
    [ "$(lsmod | grep "^$mod ")" != "" ] && rmmod $mod
}

# get qsfp presence
function _util_get_qsfp_abs {
    local phy_port=0

    #get physical port
    phy_port=$(_port_fp2phy $QSFP_PORT)

    # read status from cpld
    _qsfp_cpld_var_set ${phy_port}
    cpld_reg_file_name="${CPLD_QSFP_STATUS_KEY}_${cpld_port_index}"
    _i2c_cpld_reg_read ${cpld_index} ${cpld_reg_file_name}
    #status: 0 -> Down, 1 -> Up (ACTIVE_LOW in ABS_BIT)
    status=$(( $(( $((${cpld_reg_val})) & (1 << ($CPLD_QSFP_STATUS_ABS_BIT)) ))?0:1 ))
}

# get sfp presence
function _util_get_sfp_pres {
    local pres_bit
    # read status from cpld
    _sfp_cpld_var_set ${SFP_PORT}
    cpld_reg_file_name="${CPLD_SFP_STATUS_KEY}"
    _i2c_cpld_reg_read ${cpld_index} ${cpld_reg_file_name}
    if [ "${SFP_PORT}" == "1" ]; then
        pres_bit=${CPLD_SFP0_STATUS_PRES_BIT}
    else
        pres_bit=${CPLD_SFP1_STATUS_PRES_BIT}
    fi
    #status: 0 -> Down, 1 -> Up (ACTIVE_LOW in PRES_BIT)
    status=$(( $(( $((${cpld_reg_val})) & (1 << ($pres_bit)) ))?0:1 ))
}

# valid input number
function _util_input_check {
    # input parameter validation
    if [[ $1 -lt $2  || $1 -gt $3 ]]; then
        echo "Please input number $2~$3"
        exit
    fi
}

# clear all switch port led bitmap
function _util_port_led_clear {
    echo "port led clear..."
    # gpio pin on GPIO MUX PCA9539#1 I/O 0.2
    # pull low to reset bitamp
    output_reg=${REG_PORT0_OUT}
    mask=0x04
    value=0x00
    _util_i2cset -m ${mask} -y ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_1} ${output_reg} ${value}
    sleep 1
    # pull high to out of reset
    value=0xFF
    _util_i2cset -m ${mask} -y ${NUM_MUX_9546_1_CH3} ${I2C_ADDR_MUX_9539_1} ${output_reg} ${value}
    echo "Done"
}

#Increase read socket buffer for CoPP Test
function _config_rmem {
    echo "109430400" > /proc/sys/net/core/rmem_max
}

# util function to get logx value
function logx {
    v=$1
    n=$2
    logx_res=$(echo "${v} ${n}" | awk '{printf "%f\n",log($1)/log($2)}')
}

# get qsfp ddm data
function _i2c_qsfp_ddm_get {
    local phy_port=0

    phy_port=$(_port_fp2phy ${QSFP_PORT})

    # input parameter validation
    _util_input_check "${QSFP_PORT}" "${MIN_QSFP_PORT_NUM}" "${MAX_QSFP_PORT_NUM}"

    # check if port presence
    #status: 0 -> Down, 1 -> Up
    _util_get_qsfp_abs
    if [ "${status}" == "0" ]; then
        echo "port ${QSFP_PORT} not presence"
        return
    fi

    _qsfp_eeprom_var_set ${phy_port}

    # Get QSFP EEPROM info
    # only need first 128 bytes (page0) for ddm parsing
    local size=128
    eeprom_path="${PATH_SYS_I2C_DEVICES}/$eepromBus-$(printf "%04x" $eepromAddr)/eeprom"
    #echo "get ${eeprom_path}"
    qsfp_info=$(dd if=${eeprom_path} bs=${size} count=1 2>/dev/null | base64)

    # temperature
    temp_val1=$(echo $qsfp_info | base64 -d -i | hexdump -s 22 -n 1 -e '"%d"')
    temp_val2=$(echo $qsfp_info | base64 -d -i | hexdump -s 23 -n 1 -e '"%d"')
    temp=$(echo "$temp_val1 $temp_val2" | awk '{printf "%f\n", $1 + $2/256.0}')
    #temp=$(( ${temp_val1} + ${temp_val2}/256.0 ))
    echo "temp=$temp"
    # voltage
    volt_val1=$(echo $qsfp_info | base64 -d -i | hexdump -s 26 -n 1 -e '"%d"')
    volt_val2=$(echo $qsfp_info | base64 -d -i | hexdump -s 27 -n 1 -e '"%d"')
    #volt=$(((($volt_val1 << 8) | volt_val2) / 10000))
    volt_val3=$(( ($volt_val1 << 8) | $volt_val2 ))
    volt=$(echo "$volt_val3" | awk '{printf "%f\n", $1/10000.0}')
    echo "volt=$volt"

    # 4 channels
    for i in {0..3};
    do
        echo "channel $i:"
        # txBias
        offset=$(( 42 + $i*2 ))
        txBias_val1=$(echo $qsfp_info | base64 -d -i | hexdump -s $offset -n 1 -e '"%d"')
        offset=$(( 43 + $i*2 ))
        txBias_val2=$(echo $qsfp_info | base64 -d -i | hexdump -s $offset -n 1 -e '"%d"')
        txBias_val3=$(( ($txBias_val1 << 8) | $txBias_val2 ))
        txBias=$(echo "$txBias_val3" | awk '{printf "%f\n", (131.0*$1)/65535}')
        echo "   txBias=$txBias"
        # txPower
        offset=$(( 50 + $i*2 ))
        txPower_val1=$(echo $qsfp_info | base64 -d -i | hexdump -s $offset -n 1 -e '"%d"')
        offset=$(( 51 + $i*2 ))
        txPower_val2=$(echo $qsfp_info | base64 -d -i | hexdump -s $offset -n 1 -e '"%d"')
        txPower_val3=$(( ($txPower_val1 << 8) | $txPower_val2 ))
        txPower_val4=$(echo "$txPower_val3" | awk '{printf "%f\n", $1*0.0001}')
        logx $txPower_val4 10
        txPower=$(echo "$logx_res" | awk '{printf "%f\n", $1*10}')
        echo "   txPower=$txPower"
        # rxPower
        offset=$(( 34 + $i*2 ))
        rxPower_val1=$(echo $qsfp_info | base64 -d -i | hexdump -s $offset -n 1 -e '"%d"')
        offset=$(( 35 + $i*2 ))
        rxPower_val2=$(echo $qsfp_info | base64 -d -i | hexdump -s $offset -n 1 -e '"%d"')
        rxPower_val3=$(( ($rxPower_val1 << 8) | $rxPower_val2 ))
        rxPower_val4=$(echo "$rxPower_val3" | awk '{printf "%f\n", $1*0.0001}')
        logx $rxPower_val4 10
        rxPower=$(echo "$logx_res" | awk '{printf "%f\n", $1*10}')
        echo "   rxPower=$rxPower"
    done
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
    elif [ "${EXEC_FUNC}" == "i2c_fan_speed_init" ]; then
        _i2c_fan_speed_init
    elif [ "${EXEC_FUNC}" == "i2c_io_exp_init" ]; then
        _i2c_io_exp_init
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
    elif [ "${EXEC_FUNC}" == "i2c_sfp_eeprom_get" ]; then
        _i2c_sfp_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_eeprom_init" ]; then
        _i2c_qsfp_eeprom_init ${QSFP_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_sfp_eeprom_init" ]; then
        _i2c_sfp_eeprom_init ${SFP_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_mb_eeprom_init" ]; then
        _i2c_mb_eeprom_init ${MB_EEPROM_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_cb_eeprom_init" ]; then
        _i2c_cb_eeprom_init ${MB_EEPROM_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_status_get" ]; then
        _i2c_qsfp_status_get
    elif [ "${EXEC_FUNC}" == "i2c_sfp_status_get" ]; then
        _i2c_sfp_status_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_type_get" ]; then
        _i2c_qsfp_type_get
    elif [ "${EXEC_FUNC}" == "i2c_sfp_type_get" ]; then
        _i2c_sfp_type_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_ddm_get" ]; then
        _i2c_qsfp_ddm_get
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
    elif [ "${EXEC_FUNC}" == "i2c_cpld_version" ]; then
        _i2c_cpld_version
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
