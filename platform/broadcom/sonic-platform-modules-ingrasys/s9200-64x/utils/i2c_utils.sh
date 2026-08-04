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
QSFP_INIT_ACTION=${2}
MB_EEPROM_ACTION=${2}
TARGET_10G_MUX=${2}
QSFP_ACTION=${3}
QSFP_VALUE=${4}
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

I2C_NUM=0
NUM_I801_DEVICE=$((I2C_NUM++)) #0 

#MUX PCA9548#0 CPLD (I2C_NUM 1~8)
NUM_MUX_9548_CPLD_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_CPLD_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548#1 UCD (I2C_NUM 9~16)
NUM_MUX_9548_UCD_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_UCD_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9545 PSU (I2C_NUM 17~20)
NUM_MUX_9545_PSU_CHAN=()
for (( i=0; i<4; ++i ))
do  
    NUM_MUX_9545_PSU_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP ROOT (I2C_NUM 21~28)
NUM_MUX_9548_QSFP_ROOT_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_ROOT_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP Port 0~7 (I2C_NUM 29~36)
NUM_MUX_9548_QSFP_0_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_0_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP Port 8~15 (I2C_NUM 37~44)
NUM_MUX_9548_QSFP_1_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_1_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP Port 16~23 (I2C_NUM 45~52)
NUM_MUX_9548_QSFP_2_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_2_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP Port 24~31 (I2C_NUM 53~60)
NUM_MUX_9548_QSFP_3_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_3_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP Port 32~39 (I2C_NUM 61~68)
NUM_MUX_9548_QSFP_4_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_4_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP Port 40~47 (I2C_NUM 69~76)
NUM_MUX_9548_QSFP_5_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_5_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP Port 48~55 (I2C_NUM 77~84)
NUM_MUX_9548_QSFP_6_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_6_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 QSFP Port 56~63 (I2C_NUM 85~92)
NUM_MUX_9548_QSFP_7_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_QSFP_7_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9545 SFP (I2C_NUM 93~96)
NUM_MUX_9545_SFP_CHAN=()
for (( i=0; i<4; ++i ))
do  
    NUM_MUX_9545_SFP_CHAN[i]=$((I2C_NUM++))
done

#MUX PCA9548 CPLD SW Upgrade  (I2C_NUM 97~104)
NUM_MUX_9548_CPLD_SW_CHAN=()
for (( i=0; i<8; ++i ))
do  
    NUM_MUX_9548_CPLD_SW_CHAN[i]=$((I2C_NUM++))
done

#MUX Alias
I2C_BUS_PSU1_EEPROM=${NUM_MUX_9545_PSU_CHAN[1]}
I2C_BUS_PSU2_EEPROM=${NUM_MUX_9545_PSU_CHAN[0]}
I2C_BUS_LED_BOARD=${NUM_MUX_9548_UCD_CHAN[1]}
I2C_BUS_ID_GPIO=${NUM_MUX_9548_UCD_CHAN[2]}
I2C_BUS_10GMUX=${NUM_MUX_9548_UCD_CHAN[4]}
I2C_BUS_HWM=${NUM_MUX_9548_UCD_CHAN[7]}
I2C_BUS_FAN_BOARD=${NUM_I801_DEVICE}
I2C_BUS_MB_EEPROM=${NUM_I801_DEVICE}
I2C_BUS_TH_RESET=${NUM_I801_DEVICE}
I2C_BUS_BOARD_TYPE=${NUM_I801_DEVICE}
I2C_BUS_BMC_BOARD_TYPE=${NUM_I801_DEVICE}
I2C_BUS_LM75_MAC=${NUM_MUX_9548_CPLD_CHAN[5]}
I2C_BUS_LM75_FRONT=${NUM_MUX_9548_CPLD_CHAN[6]}
I2C_BUS_LM75_REAR=${NUM_MUX_9548_CPLD_CHAN[5]}
I2C_BUS_PSU_INT=${NUM_I801_DEVICE}
I2C_BUS_PSU_STATUS=${NUM_I801_DEVICE}
I2C_BUS_MUX_RST=${NUM_I801_DEVICE}
#CPU Board
I2C_BUS_CPU_EEPROM=${NUM_I801_DEVICE}
I2C_BUS_CPU_TMP75=${NUM_I801_DEVICE}
I2C_BUS_MUX_CPU_CPLD=${NUM_I801_DEVICE}

PATH_SYS_I2C_DEVICES="/sys/bus/i2c/devices"
PATH_SYS_CPLD="/sys/devices/platform/ingrasys-s9200-cpld.0"
PATH_HWMON_ROOT_DEVICES="/sys/class/hwmon"
PATH_HWMON_W83795_DEVICE="${PATH_HWMON_ROOT_DEVICES}/hwmon5"
PATH_I801_DEVICE="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_I801_DEVICE}"
PATH_LM75_MAC="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_LM75_MAC}"
PATH_LM75_FRONT="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_LM75_FRONT}"
PATH_LM75_REAR="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_LM75_REAR}"
PATH_CPU_TMP75="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_I801_DEVICE}"
PATH_BMC_TMP75="${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_HWM}"
PATH_PSU1_EERPOM=${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU1_EEPROM}
PATH_PSU2_EEPROM=${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_PSU2_EEPROM}
PATH_10GMUX=${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_10GMUX}
#PATH for MUX PCA9548_QSFP_ROOT
PATH_MUX_9548_QSFP_ROOT="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9545_PSU_CHAN[2]}"
PATH_MUX_9548_QSFP_ROOT_CHAN=()
for (( i=0; i<8; ++i ))
do  
    PATH_MUX_9548_QSFP_ROOT_CHAN[i]="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9548_QSFP_ROOT_CHAN[i]}"
done
PATH_MUX_9545_SFP="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9545_PSU_CHAN[3]}"
PATH_MUX_9545_SFP_CHAN=()
for (( i=0; i<4; ++i ))
do  
    PATH_MUX_9545_SFP_CHAN[i]="${PATH_SYS_I2C_DEVICES}/i2c-${NUM_MUX_9545_SFP_CHAN[i]}"
done

#i2cmap and gpiomap path
PATH_I2CMAP=/tmp/i2cmap

#Mother Board
I2C_ADDR_MUX_9539_TH_RST=0x74
I2C_ADDR_LED_BOARD=0x75
I2C_ADDR_FAN_BOARD=0x20
I2C_ADDR_ID_GPIO=0x27
I2C_ADDR_MUX_9548_CPLD=0x70
I2C_ADDR_MUX_9548_UCD=0x76
I2C_ADDR_MUX_9545_PSU=0x72
I2C_ADDR_MUX_9548_QSFP_ROOT=0x71
I2C_ADDR_MUX_9548_QSFP=0x73
I2C_ADDR_MUX_9545_SFP=0x73
I2C_ADDR_CPLD=0x33
I2C_ADDR_MB_EEPROM=0x55
I2C_ADDR_QSFP_EEPROM=0x50
I2C_ADDR_PSU_EEPROM=0x50
I2C_ADDR_LM75_MAC=0x4E
I2C_ADDR_LM75_FRONT=0x4D
I2C_ADDR_LM75_REAR=0x4D
I2C_ADDR_10GMUX=0x67
#Dummy BMC Board
I2C_ADDR_HWM=0x2F
I2C_ADDR_PSU_INT=0x24        #PSU Interrup on Dummy BMC Board
I2C_ADDR_BMC_BOARD_TYPE=0x24 #PSU Status on Dummy BMC Board
I2C_ADDR_PSU_STATUS=0x25     #PSU Status on Dummy BMC Board
I2C_ADDR_MUX_RST=0x26        #MUX RST on Dummy BMC Board
I2C_ADDR_TMP75_BB=0x4A
#CPU Board
I2C_ADDR_CPU_EEPROM=0x51
I2C_ADDR_CPU_TMP75=0x4F
I2C_ADDR_MUX_CPU_CPLD=0x77

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

#LM75 Registers
LM75_REG_TEMP=0x00
LM75_REG_CONF=0x01
LM75_REG_HYST=0x02
LM75_REG_TOS=0x03

#Bit Mask
BIT_MASK=(1 2 4 8 16 32 64 128)

#MUX Type
PCA9545="pca9545"
PCA9548="pca9548"

#PLATFORM Variables
PORT_NUM=64

#CPLD Variables
CPLD_QSFP_GROUP=(12 25 38 51 64)
CPLD_QSFP_STATUS_REG_BASE=0x20
CPLD_QSFP_MODE_SELECT_REG_BASE=0x30
CPLD_QSFP_ABS="abs"
CPLD_QSFP_INT="int"
CPLD_QSFP_RST="rst"
CPLD_QSFP_LP="lp"


# Help usage function
function _help {
    echo "========================================================="
    echo "# Description: Help Function"
    echo "========================================================="
    echo "----------------------------------------------------"
    echo "EX       : ${0} help"
    echo "         : ${0} i2c_init"
    echo "         : ${0} i2c_deinit"
    echo "         : ${0} i2c_temp_init"
    echo "         : ${0} i2c_fan_init"
    echo "         : ${0} i2c_volmon_init"
    echo "         : ${0} i2c_io_exp_init"
    echo "         : ${0} i2c_psu_eeprom_get"
    echo "         : ${0} i2c_mb_eeprom_get"
    echo "         : ${0} i2c_cpu_eeprom_get"
    echo "         : ${0} i2c_qsfp_eeprom_get [1-${PORT_NUM}]"
    echo "         : ${0} i2c_qsfp_eeprom_init new|delete"
    echo "         : ${0} i2c_mb_eeprom_init new|delete"
    echo "         : ${0} i2c_cpu_eeprom_init new|delete"
    echo "         : ${0} i2c_qsfp_status_get [1-${PORT_NUM}]"
    echo "         : ${0} i2c_qsfp_type_get [1-${PORT_NUM}]"
    echo "         : ${0} i2c_qsfp_signal_get [1-${PORT_NUM}] [abs|int|rst|lp]"
    echo "         : ${0} i2c_qsfp_signal_set [1-${PORT_NUM}] [rst|lp] [0|1] (low_active)"
    echo "         : ${0} i2c_board_type_get"
    echo "         : ${0} i2c_bmc_board_type_get"
    echo "         : ${0} i2c_psu_status"
    echo "         : ${0} i2c_led_psu_status_set"
    echo "         : ${0} i2c_led_fan_status_set"
    echo "         : ${0} i2c_led_fan_tray_status_set"
    echo "         : ${0} i2c_led_fan_tray_test"
    echo "         : ${0} i2c_cpld_version"
    echo "         : ${0} i2c_test_all"
    echo "         : ${0} i2c_led_test"
    echo "         : ${0} i2c_sys_led green|amber"
    echo "         : ${0} i2c_fan_led green|amber"
    echo "         : ${0} i2c_psu1_led green|amber"
    echo "         : ${0} i2c_psu2_led green|amber"
    echo "         : ${0} i2c_fan_tray_led green|amber on|off [1-4]"
    echo "         : ${0} i2c_10g_mux cpu|fp"
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

#logical(front panel) to physical (falcon core) port mapping
function _port_logic2phy {
    
    local logic_port=$1
    local phy_port=0
    
    if (( $logic_port >=1  && $logic_port <= 32 )) ; then
        phy_port=$(( (logic_port-1)/2*4 + (logic_port-1)%2 + 1))
    elif (( $logic_port >=33  && $logic_port <= 64 )) ; then
        phy_port=$(( (((logic_port-1)%32))/2*4 + (logic_port-1)%2 + 3))
    fi 
    
    echo $phy_port 
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

#remove kernel module if exists
function _util_rmmod {
    local mod=$1
    [ "$(lsmod | grep "^$mod ")" != "" ] && rmmod $mod
}

function _create_i2c_mux_device {
    local mux_type=$1
    local mux_addr=$2
    local mux_parent_chan=$3
    local mux_chan0=$4
    local mux_desc=$5
    
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/i2c-${mux_chan0}" ]; then
        _retry "echo '$mux_type ${mux_addr}' > ${mux_parent_chan}/new_device"
        _set_i2cmap ${mux_chan0} "${mux_desc}"
    else
        echo "${mux_desc} ${mux_addr} already init."
    fi
}

#I2C Init
function _i2c_init {
    echo "========================================================="
    echo "# Description: I2C Init"
    echo "========================================================="

    _util_rmmod i2c_i801
    modprobe i2c_i801 #disable_features=0x10
    modprobe i2c_dev
    modprobe i2c_mux_pca954x force_deselect_on_exit=1

    #add MUX_9548_CPLD on I801
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_CPLD ${PATH_I801_DEVICE} ${NUM_MUX_9548_CPLD_CHAN[0]} "PCA9548_CPLD"

    #add MUX_9548_UCD on I801
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_UCD ${PATH_I801_DEVICE} ${NUM_MUX_9548_UCD_CHAN[0]} "PCA9548_UCD"

    #add MUX_9545_PSU on I801
    _create_i2c_mux_device $PCA9545 $I2C_ADDR_MUX_9545_PSU ${PATH_I801_DEVICE} ${NUM_MUX_9545_PSU_CHAN[0]} "PCA9545_PSU"

    #add MUX_9548_QSFP_ROOT on MUX_9545_PSU_CHAN[2]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP_ROOT ${PATH_MUX_9548_QSFP_ROOT} ${NUM_MUX_9548_QSFP_ROOT_CHAN[0]} "PCA9548_QSPF_ROOT"

    #add MUX_9548_QSFP_0 on MUX_9548_QSFP_ROOT_CHAN[0]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP ${PATH_MUX_9548_QSFP_ROOT_CHAN[0]} ${NUM_MUX_9548_QSFP_0_CHAN[0]} "PCA9548_QSPF_0"

    #add MUX_9548_QSFP_1 on MUX_9548_QSFP_ROOT_CHAN[1]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP ${PATH_MUX_9548_QSFP_ROOT_CHAN[1]} ${NUM_MUX_9548_QSFP_1_CHAN[0]} "PCA9548_QSPF_1"

    #add MUX_9548_QSFP_2 on MUX_9548_QSFP_ROOT_CHAN[2]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP ${PATH_MUX_9548_QSFP_ROOT_CHAN[2]} ${NUM_MUX_9548_QSFP_2_CHAN[0]} "PCA9548_QSPF_2"

    #add MUX_9548_QSFP_3 on MUX_9548_QSFP_ROOT_CHAN[3]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP ${PATH_MUX_9548_QSFP_ROOT_CHAN[3]} ${NUM_MUX_9548_QSFP_3_CHAN[0]} "PCA9548_QSPF_3"

    #add MUX_9548_QSFP_4 on MUX_9548_QSFP_ROOT_CHAN[4]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP ${PATH_MUX_9548_QSFP_ROOT_CHAN[4]} ${NUM_MUX_9548_QSFP_4_CHAN[0]} "PCA9548_QSPF_4"

    #add MUX_9548_QSFP_5 on MUX_9548_QSFP_ROOT_CHAN[5]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP ${PATH_MUX_9548_QSFP_ROOT_CHAN[5]} ${NUM_MUX_9548_QSFP_5_CHAN[0]} "PCA9548_QSPF_5"

    #add MUX_9548_QSFP_6 on MUX_9548_QSFP_ROOT_CHAN[6]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP ${PATH_MUX_9548_QSFP_ROOT_CHAN[6]} ${NUM_MUX_9548_QSFP_6_CHAN[0]} "PCA9548_QSPF_6"

    #add MUX_9548_QSFP_7 on MUX_9548_QSFP_ROOT_CHAN[7]
    _create_i2c_mux_device $PCA9548 $I2C_ADDR_MUX_9548_QSFP ${PATH_MUX_9548_QSFP_ROOT_CHAN[7]} ${NUM_MUX_9548_QSFP_7_CHAN[0]} "PCA9548_QSPF_7"

    #add MUX_9545_SFP on QSFP_ROOT
    _create_i2c_mux_device $PCA9545 $I2C_ADDR_MUX_9545_SFP ${PATH_MUX_9545_SFP} ${NUM_MUX_9545_SFP_CHAN[0]} "PCA9545_SPF"

    #Init CPLD LED_CLR Register (QSFP LED)
    i2cset -y -m ${BIT_MASK[2]} ${NUM_I801_DEVICE} ${I2C_ADDR_MUX_9539_TH_RST} ${REG_OUT_0} 0xFF

    _util_rmmod coretemp
    _util_rmmod jc42
    _util_rmmod w83795
    _util_rmmod lm75

    _i2c_temp_init
    _i2c_volmon_init
    modprobe coretemp
    modprobe lm75
    modprobe jc42
    modprobe eeprom_mb
    _i2c_mb_eeprom_init "new"
    _i2c_cpu_eeprom_init "new"
    modprobe eeprom
    modprobe gpio-pca953x
    _i2c_sensors_init
    _i2c_fan_init
    _i2c_io_exp_init
    _i2c_psu_init
    _i2c_qsfp_eeprom_init "new"
    _i2c_led_psu_status_set
    _i2c_led_fan_status_set
    COLOR_LED="green"
    echo "${COLOR_LED}"
    _i2c_sys_led
    modprobe cpld
    _config_rmem
}

#I2C Deinit
function _i2c_deinit {
    for mod in cpld coretemp jc42 w83795 lm75 eeprom eeprom_mb gpio-pca953x i2c_mux_pca954x i2c_i801 ingrasys_s9200_64x_psu;
    do   
        _util_rmmod $mod 
    done
    _clear_i2cmap
}

#Temperature sensor Init
function _i2c_temp_init {
    local lm="lm75"
    local tmp="tmp75"
    local i2c_bus=${I2C_BUS_HWM}
    local i2c_addr=${I2C_ADDR_HWM}

    echo "========================================================="
    echo "# Description: TEMP INIT"
    echo "========================================================="

    # select bank0
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_BANK_SEL} 0x80
    echo "enable VDSEN14, VDSEN15, VDSEN16, and TR4 temperature monitoring"
    i2cset -y -r ${i2c_bus=} ${i2c_addr} ${REG_TEMP_CTRL2} 0x6A

    echo "export CPU sensor TMP75 to sysfs"
    echo "${tmp} ${I2C_ADDR_CPU_TMP75}" > ${PATH_CPU_TMP75}/new_device

    echo "Add Dummy Board LM75 to sysfs"
    echo "${lm} ${I2C_ADDR_LM75_MAC}" > ${PATH_LM75_MAC}/new_device
    echo "${lm} ${I2C_ADDR_LM75_FRONT}" > ${PATH_LM75_FRONT}/new_device
    echo "${lm} ${I2C_ADDR_LM75_REAR}" > ${PATH_LM75_REAR}/new_device

    echo "CLKIN clock frequency set as 48Mhz"
    i2cset -y -r ${i2c_bus} ${i2c_addr} 0x01 0x1C

    # select bank 2
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_BANK_SEL} 0x82
    echo "set PWM mode in FOMC"
    i2cset -y -r ${i2c_bus} ${i2c_addr} 0x0F 0x00
}

#FAN Init
function _i2c_fan_init {
    local init_fan_speed=120

    echo "========================================================="
    echo "# Description: FAN INIT"
    echo "========================================================="

    if [ -e "${PATH_HWMON_W83795_DEVICE}" ]; then
        echo "Init Fan Speed to ${init_fan_speed} (MAX is 255)"
        echo $init_fan_speed > ${PATH_HWMON_W83795_DEVICE}/device/pwm1
        echo $init_fan_speed > ${PATH_HWMON_W83795_DEVICE}/device/pwm2
    else
        echo "FAIL, W83795 not found in path ${PATH_HWMON_W83795_DEVICE}"
    fi
}

#VOLMON Init
function _i2c_volmon_init {
    echo -n "VOLMON INIT"
    local i2c_bus=${I2C_BUS_HWM}
    local i2c_addr=${I2C_ADDR_HWM}

    #select bank0
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_BANK_SEL} 0x80
    #enable voltage monitoring VSEN 1~8
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_VOLT_CTRL1} 0xFF
    #enable voltage monitoring 3VDD and 3VBAT
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_VOLT_CTRL2} 0x50
    #eable voltage monitoring VSEN12 and VSEN13
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_TEMP_CTRL1} 0x0A

    echo "Done"
}

#IO Expander Init
function _i2c_io_exp_init {
    local i2c_bus=0
    local i2c_addr=0

    echo "========================================================="
    echo "# Description: I2C IO Expander Init"
    echo "========================================================="

    echo "Init PCA9539 TH RESET IO Expander"
    #PCA9539 TH RESET
    #0.0 TH_RST_L               OUT 1 
    #0.1 TH_PCIE_RST_L          OUT 1
    #0.2 LED_CLR                OUT 1
    #0.3 HOST_TO_BMC_I2C_GPIO   OUT 0
    #0.4 USB_MUX_SEL            OUT 0
    #0.5 UART_MUX_SEL           OUT 0
    #0.6 FIN0                   OUT 0
    #0.7 FIN7                   OUT 0
    #1.0 TH_INT_L               IN 
    #1.1 CPLD4_TO_CPU_INT_L     IN  
    #1.2 CPLD3_TO_CPU_INT_L     IN  
    #1.3 CPLD2_TO_CPU_INT_L     IN  
    #1.4 CPLD1_TO_CPU_INT_L     IN  
    #1.5 REF_SEL                OUT 0
    #1.6 I210_PE_RST_L          OUT 1
    #1.7 I210_RST_L             OUT 1
    i2c_bus=${I2C_BUS_TH_RESET}
    i2c_addr=${I2C_ADDR_MUX_9539_TH_RST}
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_0} $((2#00000111))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_1} $((2#11000000))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_0} 0x00 
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_0} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_1} $((2#00011111))

    #PCA9555 on BMC Dummy Board
    #set SYSTEM_LED_RST_L before init PCA9539 LED BOARD IO Expander 
    echo "Init PCA9555 MUX RST IO Expander"
    #PCA9555 MUX RST Expander
    #0.0 FRU_I2C_MUX_RST_L   OUT 1
    #0.1 MAIN_I2C_MUX_RST_L  OUT 1
    #0.2 SYSTEM_LED_RST_L    OUT 1
    #0.3 BMC_TO_UCD_RST_L    OUT 1
    #0.4 BMC_TO_HOST_RST_L   OUT 1
    #0.5 ETHX_RST_L          OUT 1
    #0.6 GPIOF4              IN 
    #0.7 N/A
    #1.0 CPU_to_FRU_I2C_EN   OUT 1
    #1.1 CPU_to_MAIN_I2C_EN  OUT 1
    #1.2 P3V3_FAN_EN         OUT 1
    #1.3 P3V3_I2C_EN         OUT 1
    #1.4 UCD9090_CNTRL       OUT 1
    #1.5 UART_MUX_SEL        OUT 0
    #1.6 USB_MUX_SEL         OUT 0
    #1.7 ETHX_CPU_EEPROM_SEL OUT 0
    i2c_bus=${I2C_BUS_MUX_RST}
    i2c_addr=${I2C_ADDR_MUX_RST}
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_0} $((2#00111111))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_1} $((2#00011111))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_0} 0x00 
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_0} $((2#01000000))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_1} 0x00

    echo "Init PCA9539 LED BOARD IO Expander"
    #PCA9539 LED BOARD
    #0.0 SYS_LED_G    OUT 1 
    #0.1 FAN_LED_EN   OUT 1
    #0.2 FAN_LED_Y    OUT 0
    #0.3 PSU1_LED_Y   OUT 0
    #0.4 PSU0_LED_Y   OUT 0
    #0.5~0.7 X
    #1.0 PSU1_PWROK   OUT 1
    #1.1 PSU0_PWROK   OUT 1
    i2c_bus=${I2C_BUS_LED_BOARD}
    i2c_addr=${I2C_ADDR_LED_BOARD}
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_0} $((2#00000011))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_1} $((2#00000011))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_0} 0x00 
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_0} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_1} 0x00

    echo "Init PCA9535 ID_GPIO IO Expander"
    #PCA9535 ID_GPIO
    #0.0 ~ 0.7 X  
    #1.0 Build_REV_0  IN 
    #1.1 Build_REV_1  IN  
    #1.2 HW_REV_0     IN  
    #1.3 HW_REV_1     IN  
    #1.4 Board_ID_0   IN  
    #1.5 Board_ID_1   IN
    #1.6 Board_ID_2   IN
    #1.7 Board_ID_3   IN
    i2c_bus=${I2C_BUS_ID_GPIO}
    i2c_addr=${I2C_ADDR_ID_GPIO}
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_0} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_0} 0xFF
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_1} 0xFF
    
    #I2C_ADDR_FAN_BOARD
    echo "Init PCA9535 FAN_BOARD IO Expander"
    #PCA9535 FAN_BOARD 
    #0.0 EN_FAN1_LED_G_H OUT 1
    #0.1 EN_FAN1_LED_Y_H OUT 0
    #0.2 FAN1_ABS#       IN  0
    #0.3 FAN1_DIR#       IN  0
    #0.4 EN_FAN2_LED_G_H OUT 1
    #0.5 EN_FAN2_LED_Y_H OUT 0
    #0.6 FAN2_ABS#       IN  0
    #0.7 FAN2_DIR#       IN  0
    #1.0 EN_FAN3_LED_G_H OUT 1
    #1.1 EN_FAN3_LED_Y_H OUT 0
    #1.2 FAN3_ABS#       IN  0
    #1.3 FAN3_DIR#       IN  0
    #1.4 EN_FAN4_LED_G_H OUT 1
    #1.5 EN_FAN4_LED_Y_H OUT 0
    #1.6 FAN4_ABS#       IN  0
    #1.7 FAN4_DIR#       IN  0
    i2c_bus=${I2C_BUS_FAN_BOARD}
    i2c_addr=${I2C_ADDR_FAN_BOARD}
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_0} $((2#00010001))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_1} $((2#00010001))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_0} 0x00 
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_0} $((2#11001100))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_1} $((2#11001100))

    #PCA9555 on BMC Dummy Board
    echo "Init PCA9555 Interrupt IO Expander"
    #PCA9555 Interrupt Expander
    #0.0 HWM_INT_L         IN 
    #0.1 UCD9090_INT_L     IN 
    #0.2 ETHX_INT_L        IN 
    #0.3 HOST_TO_BMC_INT_L IN 
    #0.4 THERMAL_ALT1#     IN 
    #0.5 FAN_STATUS_INT_L  IN
    #0.6 PSU0_INT_L        IN 
    #0.7 PSU1_INT_L        IN 
    #1.0 Build_REV_0       IN
    #1.1 Build_REV_1       IN
    #1.2 HW_REV_0          IN 
    #1.3 HW_REV_1          IN
    #1.4 Board_ID_0        IN
    #1.5 Board_ID_1        IN 
    #1.6 Board_ID_2        IN 
    #1.7 Board_ID_3        IN
    i2c_bus=${I2C_BUS_PSU_INT}
    i2c_addr=${I2C_ADDR_PSU_INT}
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_0} 0x00 
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_0} 0xFF
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_1} 0xFF

    #PCA9555 on BMC Dummy Board
    echo "Init PCA9555 PSU0_PWROK IO Expander"
    #PCA9555 PSU0_PWROK Expander
    #0.0 PSU0_PWROK           IN 
    #0.1 PSU0_PRSNT_L         IN 
    #0.2 PSU0_PWRON_L         OUT 0 
    #0.3 PSU1_PWROK           IN 
    #0.4 PSU1_PRSNT_L         IN 
    #0.5 PSU1_PWRON_L         OUT 0
    #0.6 TMP75_INT_L          IN 
    #0.7 GPIOF7               IN 
    #1.0 ALL_PWR_GOOD         IN
    #1.1 CPU_PRSNT_L          IN
    #1.2 FP_BTN_UARTSEL_EN_L  OUT 0
    #1.3 HOST_TO_BMC_I2C_GPIO OUT 0
    #1.4 HOST_CPU_PWRBTN_L    OUT 1
    #1.5 N/A 
    #1.6 N/A
    #1.7 N/A
    i2c_bus=${I2C_BUS_PSU_STATUS}
    i2c_addr=${I2C_ADDR_PSU_STATUS}
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_0} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_OUT_1} $((2#00010000))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_0} 0x00 
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_POLARITY_1} 0x00
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_0} $((2#11011011))
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_1} $((2#11100011))

    #PCA9539 on CPU Board
    echo "Init PCA9539 CPLD IO Expander on CPU"
    i2c_bus=${I2C_BUS_MUX_CPU_CPLD}
    i2c_addr=${I2C_ADDR_MUX_CPU_CPLD}
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_0} 0xFF
    i2cset -y -r ${i2c_bus} ${i2c_addr} ${REG_CFG_1} 0xFF
}

function _i2c_sensors_init {
    echo "SENSORS init"
    local dev_path

    # add w83795 to sysfs
    dev_path="${PATH_SYS_I2C_DEVICES}/${I2C_BUS_HWM}-$(printf "%04x" ${I2C_ADDR_HWM})"
    if ! [ -L ${dev_path} ]; then
        echo "w83795adg ${I2C_ADDR_HWM}" > ${PATH_SYS_I2C_DEVICES}/i2c-${I2C_BUS_HWM}/new_device
        while [ ! -e "${PATH_HWMON_W83795_DEVICE}" ]; do 
        sleep 1
    done
    else
        echo "${dev_path} already exist (w83795)"
    fi

    # BMC board thermal
    dev_path="${PATH_SYS_I2C_DEVICES}/${NUM_I801_DEVICE}-$(printf "%04x" ${I2C_ADDR_TMP75_BB})"
    if ! [ -L ${dev_path} ]; then
        echo "tmp75 ${I2C_ADDR_TMP75_BB}" > ${PATH_BMC_TMP75}/new_device
    else
        echo "${dev_path} already exist"
    fi

    echo "Done"
}

#Set FAN Tray LED
function _i2c_led_fan_tray_status_set {
    echo "========================================================="
    echo "# Description: FAN Tray Status Setup"
    echo "========================================================="

    #check W83795 exists
    if [ ! -e "${PATH_HWMON_W83795_DEVICE}" ]; then
        echo "FAIL, W83795 not found in path ${PATH_HWMON_W83795_DEVICE}"
        return 
    fi
    #FAN Status get
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN5_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN7_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`

    FAN_TRAY=1
    if [ "${FAN1_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        _i2c_fan_tray_led

        COLOR_LED="amber"
        ONOFF_LED="off"
        _i2c_fan_tray_led

        echo "set [FAN TRAY 1] [Green]=on [Amber]=off"
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        _i2c_fan_tray_led

        COLOR_LED="amber"
        ONOFF_LED="on"
        _i2c_fan_tray_led

        echo "set [FAN TRAY 1] [Green]=off [Amber]=on"
    fi

    FAN_TRAY=2
    if [ "${FAN3_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        _i2c_fan_tray_led

        COLOR_LED="amber"
        ONOFF_LED="off"
        _i2c_fan_tray_led

        echo "set [FAN TRAY 2] [Green]=on [Amber]=off"
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        _i2c_fan_tray_led

        COLOR_LED="amber"
        ONOFF_LED="on"
        _i2c_fan_tray_led

        echo "set [FAN TRAY 2] [Green]=off [Amber]=on"
    fi

    FAN_TRAY=3
    if [ "${FAN5_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        _i2c_fan_tray_led

        COLOR_LED="amber"
        ONOFF_LED="off"
        _i2c_fan_tray_led

        echo "set [FAN TRAY 3] [Green]=on [Amber]=off"
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        _i2c_fan_tray_led

        COLOR_LED="amber"
        ONOFF_LED="on"
        _i2c_fan_tray_led

        echo "set [FAN TRAY 3] [Green]=off [Amber]=on"
    fi

    FAN_TRAY=4
    if [ "${FAN7_ALARM}" == "0" ]; then
        COLOR_LED="green"
        ONOFF_LED="on"
        _i2c_fan_tray_led

        COLOR_LED="amber"
        ONOFF_LED="off"
        _i2c_fan_tray_led

        echo "set [FAN TRAY 4] [Green]=on [Amber]=off"
    else
        COLOR_LED="green"
        ONOFF_LED="off"
        _i2c_fan_tray_led

        COLOR_LED="amber"
        ONOFF_LED="on"
        _i2c_fan_tray_led

        echo "set [FAN TRAY 4] [Green]=off [Amber]=on"
    fi
}

#Test FAN Tray LED
function _i2c_led_fan_tray_test {
    echo "========================================================="
    echo "# Description: FAN Tray LED Test"
    echo "========================================================="

    for i in {1..4}
    do
        FAN_TRAY=$i
        COLOR_LED="green"
        ONOFF_LED="on"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="off"
        _i2c_fan_tray_led
        _pause "Check [Fan Tray ${i} LED] = Green and Press [Enter] key to continue..."

        COLOR_LED="green"
        ONOFF_LED="off"
        _i2c_fan_tray_led
        COLOR_LED="amber"
        ONOFF_LED="on"
        _i2c_fan_tray_led
        _pause "Check [Fan Tray ${i} LED] = Amber and Press [Enter] key to continue..."
    done
}
#Set FAN LED
function _i2c_led_fan_status_set {
    echo "========================================================="
    echo "# Description: FAN Status Setup"
    echo "========================================================="

    #check W83795 exists in hwmon2
    if [ ! -e "${PATH_HWMON_W83795_DEVICE}" ]; then 
        echo "FAIL, W83795 not found in path ${PATH_HWMON_W83795_DEVICE}"
        return 
    fi

    #PSU Status set
    FAN1_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan1_alarm`
    FAN3_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan3_alarm`
    FAN5_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan5_alarm`
    FAN7_ALARM=`cat ${PATH_HWMON_W83795_DEVICE}/device/fan7_alarm`

    if [ "${FAN1_ALARM}" == "0" ] \
       && [ "${FAN3_ALARM}" == "0" ] \
       && [ "${FAN5_ALARM}" == "0" ] \
       && [ "${FAN7_ALARM}" == "0" ] ; then
        COLOR_LED="green"
        _i2c_fan_led
    else
        COLOR_LED="amber"
        _i2c_fan_led
    fi
    echo "set [FAN LED] = ${COLOR_LED}"
}

#Set PSU LED on LED Board
function _i2c_led_psu_status_set {

    echo "========================================================="
    echo "# Description: PSU LED Status Setup"
    echo "========================================================="

    #Get PSU Status
    _i2c_psu_status

    #PSU1 Status
    echo "------------------------------"
    if [ "${psu1Exist}" == ${PSU_EXIST} ]; then
        if [ "${psu1PwGood}" == ${PSU_DC_ON} ]; then
            COLOR_LED="green"
            _i2c_psu1_led
        else
            COLOR_LED="amber"
            _i2c_psu1_led
        fi
    else
        COLOR_LED="amber"
        _i2c_psu1_led
    fi
    echo "set [PSU1 LED] = ${COLOR_LED}"

    #PSU2 Status
    echo "------------------------------"
    if [ "${psu2Exist}" == ${PSU_EXIST} ]; then
        if [ "${psu2PwGood}" == ${PSU_DC_ON} ]; then
            COLOR_LED="green"
            _i2c_psu2_led
        else
            COLOR_LED="amber"
            #ONOFF_LED="on"
            _i2c_psu2_led
        fi
    else
        COLOR_LED="amber"
        _i2c_psu2_led
    fi
    echo "set [PSU2 LED] = ${COLOR_LED}"
}

#LED Test
function _i2c_led_test {
    echo "========================================================="
    echo "# Description: I2C LED TEST"
    echo "========================================================="
    #sys led (green)
    _i2c_reset_led
    COLOR_LED="green"
    _i2c_sys_led
    _pause 'Check [SYS LED] = Green and Press [Enter] key to continue...'
    #sys led (amber)
    _i2c_reset_led
    COLOR_LED="amber"
    _i2c_sys_led
    _pause 'Check [SYS LED] = Amber and Press [Enter] key to continue...'

    #FAN led (green)
    _i2c_reset_led
    COLOR_LED="green"
    _i2c_fan_led
    _pause 'Check [FAN LED] Green and Press [Enter] key to continue...'
    #FAN led (amber)
    _i2c_reset_led
    COLOR_LED="amber"
    _i2c_fan_led
    _pause 'Check [FAN LED] = Amber and Press [Enter] key to continue...'

    #PSU1 led (green)
    _i2c_reset_led
    COLOR_LED="green"
    _i2c_psu1_led
    _pause 'Check [PSU1 LED] = Green and Press [Enter] key to continue...'
    #PSU1 led (amber)
    _i2c_reset_led
    COLOR_LED="amber"
    _i2c_psu1_led
    _pause 'Check [PSU1 LED] = Amber and Press [Enter] key to continue...'

    #PSU2 led (green)
    _i2c_reset_led
    COLOR_LED="green"
    _i2c_psu2_led
    _pause 'Check [PSU2 LED] = Green and Press [Enter] key to continue...'
    #PSU2 led (amber)
    _i2c_reset_led
    COLOR_LED="amber"
    _i2c_psu2_led
    _pause 'Check [PSU2 LED] = Amber and Press [Enter] key to continue...'

    #Turn OFF All LED
    _i2c_reset_led
    _pause 'Check turn off all LEDs and Press [Enter] key to continue...'
}

#Set QSFP Port variable
function _qsfp_port_i2c_var_set {
    #global variables
    cpld_group=0
    cpld_i2c_bus=0
    cpld_i2c_addr=$I2C_ADDR_CPLD
    cpld_qsfp_status_reg_offset=0
    cpld_qsfp_mode_reg_offset=0

    #local variables
    local i=0
    local port=$1
    local port_group=$(( (port - 1)/8 ))

    case ${port_group} in
        0)
            #the i2c expander base bus num to read qsfp eeprom
            eeprombusbase=${NUM_MUX_9548_QSFP_0_CHAN[0]}
        ;;
        1)
            eeprombusbase=${NUM_MUX_9548_QSFP_1_CHAN[0]}
        ;;
        2)
            eeprombusbase=${NUM_MUX_9548_QSFP_2_CHAN[0]}
        ;;
        3)
            eeprombusbase=${NUM_MUX_9548_QSFP_3_CHAN[0]}
        ;;
        4)
            eeprombusbase=${NUM_MUX_9548_QSFP_4_CHAN[0]}
        ;;
        5)
            eeprombusbase=${NUM_MUX_9548_QSFP_5_CHAN[0]}
        ;;
        6)
            eeprombusbase=${NUM_MUX_9548_QSFP_6_CHAN[0]}
        ;;
        7)
            eeprombusbase=${NUM_MUX_9548_QSFP_7_CHAN[0]}
        ;;
        *)
            echo "Please input 1~${PORT_NUM}"
            exit
        ;;
    esac

    #set cpld_group and cpld_i2c_bus according to port
    for ((i=0; i<5; ++i))
    do
        if (("${port}" <= "${CPLD_QSFP_GROUP[i]}"));
        then
            local port_offset=0
            cpld_group=$i
            cpld_i2c_bus=${NUM_MUX_9548_CPLD_CHAN[i]}

            if ((i>0))
            then
                port_offset=$(( port - CPLD_QSFP_GROUP[i-1] ))
            else
                port_offset=$port
            fi
            cpld_qsfp_status_reg_offset=$((CPLD_QSFP_STATUS_REG_BASE + port_offset))
            cpld_qsfp_mode_reg_offset=$((CPLD_QSFP_MODE_SELECT_REG_BASE + port_offset))

            break
        fi
    done
}

function _i2c_get {
    local i2c_bus=$1
    local i2c_addr=$2
    local reg=$3
    echo `i2cget -y ${i2c_bus} ${i2c_addr} ${reg}`
}

function _i2c_set {
    local i2c_bus=$1
    local i2c_addr=$2
    local reg=$3
    local mask=$4
    local value=$5

    echo `i2cset -m $mask -y -r ${i2c_bus} ${i2c_addr} ${reg} ${value}`
}

#Set QSFP Port variable
function _qsfp_eeprom_var_set {
    #local variables
    local port=$1
    
    #global variables
    eeprombusidx=$(( (port - 1)%8 ))

    case $eeprombusidx in
        0)
          eeprombus=$(( $eeprombusbase + 0 ))
          ;;
        1)
          eeprombus=$(( $eeprombusbase + 1 ))
          ;;
        2)
          eeprombus=$(( $eeprombusbase + 2 ))
          ;;
        3)
          eeprombus=$(( $eeprombusbase + 3 ))
          ;;
        4)
          eeprombus=$(( $eeprombusbase + 4 ))
          ;;
        5)
          eeprombus=$(( $eeprombusbase + 5 ))
          ;;
        6)
          eeprombus=$(( $eeprombusbase + 6 ))
          ;;
        7)
          eeprombus=$(( $eeprombusbase + 7 ))
          ;;
    esac
    eepromAddr=${I2C_ADDR_QSFP_EEPROM}
}

#Get QSFP EEPROM Information
function _i2c_qsfp_eeprom_get {
    local value=0
    local status=0
 
    echo "========================================================="
    echo "# Description: QSFP EEPROM GET"
    echo "========================================================="

    #status: 0 -> Down, 1 -> Up
    _i2c_qsfp_signal_get ${QSFP_PORT} ${CPLD_QSFP_ABS}

    value=$qsfp_signal
    status=$(( value?0:1 ))

    if [ $status = 0 ]; then
        echo "[Fail] QSFP ${QSFP_PORT} is not present"
        exit
    fi

    echo "------------------------------"
    cat ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/eeprom | hexdump -C
}

#Init QSFP EEPROM
function _i2c_qsfp_eeprom_init {
    echo "========================================================="
    echo "# Description: QSFP EEPROM INIT"
    echo "========================================================="

    #Action check
    action=$1
    if [ -z "${action}" ]; then
        echo "No action, skip"
        return
    elif [ "${action}" != "new" ] && [ "${action}" != "delete" ]; then
        echo "Error action, skip"
        return
    fi

    #Init EEPROM
    local i
    for (( i=1; i<=${PORT_NUM}; i++ ))
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
}

#get qsfp singals (ABS, INT, RST, LP) from CPLD
function _i2c_qsfp_signal_get {
    local port=$1 #port start from 1
    local signal=$2
    local attr_file=""
    local phy_port=0
    local active="low_active"

    #global 
    qsfp_signal=0
    qsfp_signals=0

    #verify port
    if [ -z "${port}" ]; then
        echo "No port, skip"
        return
    fi

    #verify port
    if (( $port < 1  || $port > $PORT_NUM )) ; then
    echo "invalid port, skip"
    fi

    #get physical port
    phy_port=$(_port_logic2phy $port)

    _qsfp_port_i2c_var_set ${phy_port}
    _qsfp_eeprom_var_set ${phy_port}

    #verify signal
    if [ -z "${signal}" ]; then
        echo "No signal, skip"
        return
    elif [ "${signal}" == "${CPLD_QSFP_ABS}" ]; then
        attr_file="qsfp_modprs"
    elif [ "${signal}" == "${CPLD_QSFP_INT}" ]; then
        attr_file="qsfp_int"
    elif [ "${signal}" == "${CPLD_QSFP_RST}" ]; then
        attr_file="qsfp_reset"
    elif [ "${signal}" == "${CPLD_QSFP_LP}" ]; then
        attr_file="qsfp_lpmode"
        active="high_active"
    else
       echo "invalid signal ${signal}, skip"
       return
    fi

    mask=${BIT_MASK[bit_shift]}

    #signals of all ports
    qsfp_signals=`cat ${PATH_SYS_CPLD}/${attr_file}`
    #signal of single port
    qsfp_signal=$(( (qsfp_signals >> (phy_port-1)) & 0x1))
    #print signals
    printf "[port]=%d, [signal]=%s, [value]=%d (${active}), [qsfp_signals]=%016x\n" $port $signal $qsfp_signal $qsfp_signals
}

#set qsfp singals (RST, LP) via CPLD
function _i2c_qsfp_signal_set {
    local port=$1 #port start from 1
    local signal=$2
    local value=$3
    local values_ori=0
    local value_ori=0
    local values=0
    local attr_file=""

    #verify port
    if [ -z "${port}" ]; then
        echo "No port, skip"
        return
    fi

    #verify port
    if (( $port < 1  || $port > $PORT_NUM )) ; then
        echo "invalid port, skip"
    fi

    #verify signal
    if [ -z "${signal}" ]; then
        echo "No signal, skip"
        return
    elif [ "${signal}" == "${CPLD_QSFP_RST}" ]; then
        attr_file="qsfp_reset"
    elif [ "${signal}" == "${CPLD_QSFP_LP}" ]; then
        attr_file="qsfp_lpmode"
    else
       echo "invalid signal ${signal}, skip"
       return
    fi

    #get current signal values
    _i2c_qsfp_signal_get $port $signal
    value_ori=$qsfp_signal
    values_ori=$qsfp_signals
    
    #if signal value is not changed, return
    if [ "${value}" == "${value_ori}" ]; then
        echo "value not changed, skip set operation"
        return
    fi 

    #toggle the bit for update
    values=$(( values_ori ^ (1 << (port-1))))

    #set value to register
    echo "$(printf "%016x" $values)" > ${PATH_SYS_CPLD}/${attr_file}

    #get updated signal values
    _i2c_qsfp_signal_get $port $signal 
    values=$qsfp_signals

    #result
    printf "Before: %016x\n" $values_ori
    printf "After:  %016x\n" $values
}

#Init EEPROM
function _i2c_eeprom_init {
    local i2c_bus=$1
    local i2c_addr=$2
    local action=$3
    local desc=$4
    local sys_i2c_dev=$PATH_SYS_I2C_DEVICES

    echo -n "${desc} EEPROM INIT"

    #Action check
    if [ -z "${action}" ]; then
        echo "No action, skip"
        return
    elif [ "${action}" != "new" ] && [ "${action}" != "delete" ]; then
        echo "Error action, skip"
        return
    fi

    #Init EEPROM
    if [ "${action}" == "new" ] && \
        ! [ -L ${sys_i2c_dev}/${i2c_bus}-$(printf "%04x" $i2c_addr) ]; then
        echo "mb_eeprom ${i2c_addr}" > ${sys_i2c_dev}/i2c-${i2c_bus}/new_device
    elif [ "${action}" == "delete" ] && \
        [ -L ${sys_i2c_dev}/${i2c_bus}-$(printf "%04x" $i2c_addr) ]; then
        echo "${i2c_addr}" > ${sys_i2c_dev}/i2c-${i2c_bus}/delete_device
    fi
    echo "DONE"
}

#Init Main Board EEPROM
function _i2c_mb_eeprom_init {
    local action=$1
    local i2c_bus=$I2C_BUS_MB_EEPROM
    local i2c_addr=$I2C_ADDR_MB_EEPROM
    local desc="Main Board"

    _i2c_eeprom_init $i2c_bus $i2c_addr $action $desc
}

#Init CPU EEPROM
function _i2c_cpu_eeprom_init {
    local action=$1
    local i2c_bus=$I2C_BUS_CPU_EEPROM
    local i2c_addr=$I2C_ADDR_CPU_EEPROM
    local desc="CPU Board"

    _i2c_eeprom_init $i2c_bus $i2c_addr $action $desc
}

#get QSFP Status
function _i2c_qsfp_status_get {
    local value=0
    local status=0

    #status: 0 -> Down, 1 -> Up
    _i2c_qsfp_signal_get ${QSFP_PORT} ${CPLD_QSFP_ABS}
    value=$qsfp_signal
    status=$(( value?0:1 ))
    echo "status=$status"
}

#get QSFP Type
function _i2c_qsfp_type_get {
    local phy_port=0
    local value=0
    local status=0

    echo "========================================================="
    echo "# Description: QSFP TYPE Get"
    echo "========================================================="

    phy_port=$(_port_logic2phy ${QSFP_PORT})
    _qsfp_port_i2c_var_set ${phy_port}
    _qsfp_eeprom_var_set ${phy_port}

    #get abs signal
    _i2c_qsfp_signal_get ${QSFP_PORT} ${CPLD_QSFP_ABS}
    value=$qsfp_signal
    status=$(( value?0:1 ))

    #check qsfp is present
    if [ $status = 0 ]; then
        echo "[Fail] QSFP ${QSFP_PORT} is not present"
        exit
    fi

    #Get QSFP EEPROM info
    if [ ! -e "${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf '%04x' $eepromAddr)/eeprom" ]; then
        echo "FAIL, qsfp eeprom file does not exist. "
        echo "qsfp eeprom file path: ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf '%04x' $eepromAddr)/eeprom"
        return
    fi

    qsfp_info=$(base64 ${PATH_SYS_I2C_DEVICES}/$eeprombus-$(printf "%04x" $eepromAddr)/eeprom)

    identifier=$(echo $qsfp_info | base64 -d -i | hexdump -s 128 -n 1 -e '"%x"')
    connector=$(echo $qsfp_info | base64 -d -i | hexdump -s 130 -n 1 -e '"%x"')
    transceiver=$(echo $qsfp_info | base64 -d -i | hexdump -s 131 -n 1 -e '"%x"')

    echo "------------------------------"
    echo "identifier  = $identifier"
    echo "connector   = $connector"
    echo "transceiver = $transceiver"
}

#Init PSU Kernel Module
function _i2c_psu_init {
    echo "========================================================="
    echo "# Description: I2C PSU Init"
    echo "========================================================="
    modprobe ingrasys_s9200_64x_psu

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

    echo "PUS1 EEPROM"
    eeprom_psu1="${PATH_SYSFS_PSU1}/psu_eeprom"
    cat ${eeprom_psu1} | hexdump -C

    echo "------------------------------"
    echo "PUS2 EEPROM"
    eeprom_psu2="${PATH_SYSFS_PSU2}/psu_eeprom"
    cat ${eeprom_psu2} | hexdump -C
}

#Get MotherBoard EEPROM Information
function _i2c_mb_eeprom_get {
    local path_i2c_dev=${PATH_SYS_I2C_DEVICES}
    local i2c_bus=${I2C_BUS_MB_EEPROM}
    local i2c_addr=${I2C_ADDR_MB_EEPROM}
    echo "========================================================="
    echo "# Description: I2C MB EEPROM Get"
    echo "========================================================="

    ## MB EEPROM
    cat ${path_i2c_dev}/${i2c_bus}-$(printf "%04x" ${i2c_addr})/eeprom | hexdump -v -C
}

#Get CPU EEPROM Information
function _i2c_cpu_eeprom_get {
    local path_i2c_dev=${PATH_SYS_I2C_DEVICES}
    local i2c_bus=${I2C_BUS_CPU_EEPROM}
    local i2c_addr=${I2C_ADDR_CPU_EEPROM}
    echo "========================================================="
    echo "# Description: I2C CPU EEPROM Get"
    echo "========================================================="

    ## CPU EEPROM
    cat ${path_i2c_dev}/${i2c_bus}-$(printf "%04x" ${i2c_addr})/eeprom | hexdump -v -C
}

#Set FAN Tray LED
function _i2c_fan_tray_led {
    local value=0

    echo "========================================================="
    echo "# Description: Set Fan Tray LED"
    echo "========================================================="

    case ${FAN_TRAY} in
        1)
            ioPort=$REG_OUT_0
            if [ "${COLOR_LED}" == "green" ]; then
                mask=${BIT_MASK[0]}
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=${BIT_MASK[1]}
            fi
            ;;
        2)
            ioPort=$REG_OUT_0
            if [ "${COLOR_LED}" == "green" ]; then
                mask=${BIT_MASK[4]}
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=${BIT_MASK[5]}
            fi
            ;;
        3)
            ioPort=$REG_OUT_1
            if [ "${COLOR_LED}" == "green" ]; then
                mask=${BIT_MASK[0]}
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=${BIT_MASK[1]}
            fi
            ;;
        4)
            ioPort=$REG_OUT_1
            if [ "${COLOR_LED}" == "green" ]; then
                mask=${BIT_MASK[4]}
            elif [ "${COLOR_LED}" == "amber" ]; then
                mask=${BIT_MASK[5]}
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
    i2cset -m $mask -y -r ${I2C_BUS_FAN_BOARD} ${I2C_ADDR_FAN_BOARD} $ioPort $value

}

#Set System Status LED
function _i2c_sys_led {
    local value=0
    local mask=${BIT_MASK[0]}

    if [ "${COLOR_LED}" == "green" ]; then 
        value=0xFF
    elif [ "${COLOR_LED}" == "amber" ]; then 
        value=0x00
    else 
        echo "Invalid Parameters ${COLOR_LED}, Exit!!!"
        _help
        exit ${FALSE}
    fi

    _i2c_set_led $value $mask
}

#Set FAN LED
function _i2c_fan_led {
    local value=0
    local mask=${BIT_MASK[2]}

    if [ "${COLOR_LED}" == "green" ]; then 
        value=0x00
    elif [ "${COLOR_LED}" == "amber" ]; then 
        value=0xFF
    else 
        echo "Invalid Parameters ${COLOR_LED}, Exit!!!"
        _help
        exit ${FALSE}
    fi

    _i2c_set_led $value $mask
}

#Set PSU1 LED
function _i2c_psu1_led {
    local value=0
    local mask=${BIT_MASK[3]}

    if [ "${COLOR_LED}" == "green" ]; then 
        value=0x00
    elif [ "${COLOR_LED}" == "amber" ]; then 
        value=0xFF
    else 
        echo "Invalid Parameters ${COLOR_LED}, Exit!!!"
        _help
        exit ${FALSE}
    fi

    _i2c_set_led $value $mask
}

#Set PSU2 LED
function _i2c_psu2_led {
    local reg=${REG_OUT_0}
    local value=0
    local mask=${BIT_MASK[4]}

    if [ "${COLOR_LED}" == "green" ]; then 
        value=0x00
    elif [ "${COLOR_LED}" == "amber" ]; then 
        value=0xFF
    else 
        echo "Invalid Parameters ${COLOR_LED}, Exit!!!"
        _help
        exit ${FALSE}
    fi

    _i2c_set_led $value $mask
}

#Set LEDs in LED Board
function _i2c_set_led {
    local value=$1
    local mask=$2
    local reg=${REG_OUT_0}
    local i2c_bus=$I2C_BUS_LED_BOARD
    local i2c_addr=$I2C_ADDR_LED_BOARD

    _i2c_set $i2c_bus $i2c_addr $reg $mask $value
}

#Reset all system leds
function _i2c_reset_led {
    local mask=0xFF
    local value=$((2#00000011))
    local reg=${REG_OUT_0}
    local i2c_bus=$I2C_BUS_LED_BOARD
    local i2c_addr=$I2C_ADDR_LED_BOARD

    _i2c_set $i2c_bus $i2c_addr $reg $mask $value
}

#Get Mother Board Version and Type 
function _i2c_board_type_get {
    echo "========================================================="
    echo "# Description: I2C MB Board Type Get"
    echo "========================================================="
    local i2c_bus=${I2C_BUS_ID_GPIO}
    local i2c_addr=${I2C_ADDR_ID_GPIO}
    local reg=${REG_IN_1}
    local value=0

    value=$(_i2c_get $i2c_bus $i2c_addr $reg)

    #Bit 0-1
    boardBuildRev=$((value & 0x03))
    #Bit 2-3
    boardHwRev=$((value >> 2 & 0x03))
    #Bit 4-7
    boardId=$((value>>4))
    printf "[MB BOARD_ID] = 0x%02x, [HW Rev] = %d, [Build Rev] = %d\n" $boardId $boardHwRev $boardBuildRev
}

#Get Board Version and Type from Dummy BMC Board
function _i2c_bmc_board_type_get {
    echo "========================================================="
    echo "# Description: I2C BMC Board Type Get"
    echo "========================================================="

    local i2c_bus=${I2C_BUS_BMC_BOARD_TYPE}
    local i2c_addr=${I2C_ADDR_BMC_BOARD_TYPE}
    local reg=${REG_IN_1}
    local value=0

    value=$(_i2c_get $i2c_bus $i2c_addr $reg)

    #Bit 0-1
    boardBuildRev=$((($value) & 0x03))
    #Bit 2-3
    boardHwRev=$((($value) >> 2 & 0x03))
    #Bit 4-7
    boardId=$((($value) >> 4))
    printf "[BMC BOARD_ID] = 0x%02x, [HW Rev] = %d, [Build Rev] = %d\n" $boardId $boardHwRev $boardBuildRev
}

#Get CPLD Version
function _i2c_cpld_version {
    echo "========================================================="
    echo "# Description: CPLD Version"
    echo "========================================================="
    cat ${PATH_SYS_CPLD}/cpld_version
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

#Set 10G Mux to CPU or Front Panel
function _i2c_10g_mux {

    echo "========================================================="
    echo "# Description: 10G Mux"
    echo "========================================================="

    local target=${TARGET_10G_MUX}
    local i2c_bus=${I2C_BUS_10GMUX}
    local i2c_addr=${I2C_ADDR_10GMUX}
    local reg=0
    local mask=0xff
    local value=0
    local file_10g_mux="${PATH_SYS_CPLD}/cpld_10g_mux"
    
    #get current 10g mux register value on CPLD
    value=`cat ${file_10g_mux}`

    if [ "${target}" == "cpu" ]; then
        #Set XE0: slave address access
        _i2c_set $i2c_bus $i2c_addr 0x06 $mask 0x18
        #Set XE0: CH4 D_IN0-S_OUTA0 EQ
        _i2c_set $i2c_bus $i2c_addr 0x2c $mask 0x00
        #Set XE0: CH5 NC-S_OUTB0 DEM
        _i2c_set $i2c_bus $i2c_addr 0x35 $mask 0x80
        #Set XE0: CH5 NC-S_OUTB0 VOD
        _i2c_set $i2c_bus $i2c_addr 0x34 $mask 0xab
        #Set XE0: CH1 D_OUT0-S_INB0 EQ
        _i2c_set $i2c_bus $i2c_addr 0x16 $mask 0x01
        #Set XE0: CH1 D_OUT0-S_INB0 DEM
        _i2c_set $i2c_bus $i2c_addr 0x18 $mask 0x80
        #Set XE0: CH1 D_OUT0-S_INB0 VOD
        _i2c_set $i2c_bus $i2c_addr 0x17 $mask 0xab
        #Set XE1: CH6 D_IN1-S_OUTA1 EQ
        _i2c_set $i2c_bus $i2c_addr 0x3a $mask 0x00
        #Set XE1: CH7 NC-S_OUTB1 EQ
        _i2c_set $i2c_bus $i2c_addr 0x41 $mask 0x00
        #Set XE1: CH7 NC-S_OUTB1 DEM
        _i2c_set $i2c_bus $i2c_addr 0x43 $mask 0x80
        #Set XE1: CH7 NC-S_OUTB1 VOD
        _i2c_set $i2c_bus $i2c_addr 0x42 $mask 0xab
        #Set XE1: CH3 D_OUT1-S_INB1 EQ
        _i2c_set $i2c_bus $i2c_addr 0x24 $mask 0x01
        #Set XE1: CH3 D_OUT1-S_INB1 DEM
        _i2c_set $i2c_bus $i2c_addr 0x26 $mask 0x80
        #Set XE1: CH3 D_OUT1-S_INB1 VOD
        _i2c_set $i2c_bus $i2c_addr 0x25 $mask 0xab
        #Enable auto negotiation of XE ports
        eval "bcmcmd 'port xe0,xe1 an=1 if=kr en=1'"
        #delay 1 sec
        sleep 1

        #switch 10G MUX to cpu on CPLD
        value=$(( value & 0xf3 ))
        echo "$(printf "%x" ${value})" > ${file_10g_mux}
    
        echo "Switch 10G Mux to [CPU]"

    elif [ "${COLOR_LED}" == "fp" ]; then
        #Set XE0: slave address access
        _i2c_set $i2c_bus $i2c_addr 0x06 $mask 0x18
        #Set XE0: CH4 D_IN0-S_OUTA0 EQ
        _i2c_set $i2c_bus $i2c_addr 0x2c $mask 0x00
        #Set XE0: CH4 D_IN0-S_OUTA0 DEM
        _i2c_set $i2c_bus $i2c_addr 0x2e $mask 0x80
        #Set XE0: CH4 D_IN0-S_OUTA0 VOD
        _i2c_set $i2c_bus $i2c_addr 0x2d $mask 0xab
        #Set XE0: CH0 NC-S_INA0 EQ
        _i2c_set $i2c_bus $i2c_addr 0x0f $mask 0x00
        #Set XE0: CH1 D_OUT0-S_INB0 DEM
        _i2c_set $i2c_bus $i2c_addr 0x18 $mask 0x80
        #Set XE0: CH1 D_OUT0-S_INB0 VOD
        _i2c_set $i2c_bus $i2c_addr 0x17 $mask 0xab
        #Set XE1: CH6 D_IN1-S_OUTA1 EQ
        _i2c_set $i2c_bus $i2c_addr 0x3a $mask 0x00
        #Set XE1: CH6 D_IN1-S_OUTA1 DEM
        _i2c_set $i2c_bus $i2c_addr 0x3c $mask 0x80
        #Set XE1: CH6 D_IN1-S_OUTA1 VOD
        _i2c_set $i2c_bus $i2c_addr 0x3b $mask 0xab
        #Set XE1: CH2 NC-S_INA1 EQ
        _i2c_set $i2c_bus $i2c_addr 0x1d $mask 0x00
        #Set XE1: CH3 D_OUT1-S_INB1 DEM
        _i2c_set $i2c_bus $i2c_addr 0x26 $mask 0x80
        #Set XE1: CH3 D_OUT1-S_INB1 VOD
        _i2c_set $i2c_bus $i2c_addr 0x25 $mask 0xab
        #Enable auto negotiation of XE ports
        eval "bcmcmd 'port xe0,xe1 an=0 if=kr en=1'"
        #delay 1 sec
        sleep 1 

        #switch 10G MUX to front panel on CPLD
        value=$(( value | 0x0c ))
        echo "$(printf "%x" ${value})" > ${file_10g_mux}

        echo "Switch 10G Mux to [Front Panel]"
    else
        echo "invalid target, please set target to cpu/fp"
    fi
}

#Increase read socket buffer for CoPP Test
function _config_rmem {
    echo "109430400" > /proc/sys/net/core/rmem_max
}

#Main Function
function _main {
    start_time_str=`date`
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
    elif [ "${EXEC_FUNC}" == "i2c_cpu_eeprom_get" ]; then
        _i2c_cpu_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_psu_eeprom_get" ]; then
        _i2c_psu_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_eeprom_get" ]; then
        _i2c_qsfp_eeprom_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_eeprom_init" ]; then
        _i2c_qsfp_eeprom_init ${QSFP_INIT_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_mb_eeprom_init" ]; then
        _i2c_mb_eeprom_init ${MB_EEPROM_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_cpu_eeprom_init" ]; then
        _i2c_cpu_eeprom_init ${MB_EEPROM_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_status_get" ]; then
        _i2c_qsfp_status_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_type_get" ]; then
        _i2c_qsfp_type_get
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_signal_get" ]; then
        _i2c_qsfp_signal_get  ${QSFP_PORT} ${QSFP_ACTION}
    elif [ "${EXEC_FUNC}" == "i2c_qsfp_signal_set" ]; then
        _i2c_qsfp_signal_set ${QSFP_PORT} ${QSFP_ACTION} ${QSFP_VALUE}
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
    elif [ "${EXEC_FUNC}" == "i2c_bmc_board_type_get" ]; then
        _i2c_bmc_board_type_get
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
        _i2c_cpu_eeprom_get
        _i2c_board_type_get
        _i2c_bmc_board_type_get
        _i2c_cpld_version
        _i2c_psu_status
    elif [ "${EXEC_FUNC}" == "i2c_10g_mux" ]; then
        _i2c_10g_mux
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
