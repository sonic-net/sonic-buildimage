#!/bin/bash

FAN1_EEPROM="-y 31 0x51 0x0a"
FAN2_EEPROM="-y 32 0x52 0x0a"
FAN3_EEPROM="-y 33 0x53 0x0a"
FAN4_EEPROM="-y 34 0x54 0x0a"
FAN5_EEPROM="-y 35 0x55 0x0a"
LED_CONTROL="/sys/devices/platform/delta-ag9032v1-swpld.0/led_control"
FAN1_FRONT_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-37/37-002c/fan5_input"
FAN1_REAR_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-38/38-002d/fan5_input"
FAN2_FRONT_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-37/37-002c/fan4_input"
FAN2_REAR_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-38/38-002d/fan4_input"
FAN3_FRONT_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-37/37-002c/fan3_input"
FAN3_REAR_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-38/38-002d/fan3_input"
FAN4_FRONT_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-37/37-002c/fan2_input"
FAN4_REAR_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-38/38-002d/fan2_input"
FAN5_FRONT_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-37/37-002c/fan1_input"
FAN5_REAR_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-3/i2c-38/38-002d/fan1_input"

PSU1_EEPROM="-y 40 0x50 0x00"
PSU2_EEPROM="-y 41 0x50 0x00"
PSU1_FAN_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-4/i2c-40/40-0058/fan1_input"
PSU2_FAN_RPM="/sys/devices/pci0000:00/0000:00:13.0/i2c-1/i2c-4/i2c-41/41-0058/fan1_input"

catfaneeprom(){
    fan_eeprom_num=0
    i2cget $FAN1_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num+1))
    elif [ "`echo $?`" -eq "2" ]; then
        fan_eeprom_num=$((fan_eeprom_num))
    fi
        i2cget $FAN2_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num+1))
    elif [ "`echo $?`" -eq "2" ]; then
        fan_eeprom_num=$((fan_eeprom_num_num))
    fi

    i2cget $FAN3_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num+1))
    elif [ "`echo $?`" -eq "2" ]; then
        fan_eeprom_num=$((fan_eeprom_num))
    fi

    i2cget $FAN4_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num+1))
    elif [ "`echo $?`" -eq "2" ]; then
        fan_eeprom_num=$((fan_eeprom_num))
    fi

    i2cget $FAN5_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num+1))
    elif [ "`echo $?`" -eq "2" ]; then
        fan_eeprom_num=$((fan_eeprom_num))
    fi
}

catfanspeed(){
    fan_rpm_normal_num=0
    fan1_rpm_normal_num=0
    fan1_front_rpm=`cat $FAN1_FRONT_RPM` 
    fan1_rear_rpm=`cat $FAN1_REAR_RPM`
    if [ "${fan1_front_rpm}" -ne "960" ] && [ "${fan1_rear_rpm}" -ne "960" ] && [ "${fan1_front_rpm}" -ne "0" ] && [ "${fan1_rear_rpm}" -ne "0" ]; then
        fan1_rpm_normal_num=$((fan1_rpm_normal_num+1))
    elif [ "${fan1_front_rpm}" -eq "960" ] || [ "${fan1_rear_rpm}" -eq "960" ] || [ "${fan1_front_rpm}" -eq "0" ] || [ "${fan1_rear_rpm}" -eq "0" ]; then
        fan1_rpm_normal_num=$((fan1_rpm_normal_num))
    fi
	
    fan2_rpm_normal_num=0
    fan2_front_rpm=`cat $FAN2_FRONT_RPM` 
    fan2_rear_rpm=`cat $FAN2_REAR_RPM`
    if [ "${fan2_front_rpm}" -ne "960" ] && [ "${fan2_rear_rpm}" -ne "960" ] && [ "${fan2_front_rpm}" -ne "0" ] && [ "${fan2_rear_rpm}" -ne "0" ]; then
        fan2_rpm_normal_num=$((fan2_rpm_normal_num+1))
    elif [ "${fan2_front_rpm}" -eq "960" ] || [ "${fan2_rear_rpm}" -eq "960" ] || [ "${fan2_front_rpm}" -eq "0" ] || [ "${fan2_rear_rpm}" -eq "0" ]; then
        fan2_rpm_normal_num=$((fan2_rpm_normal_num))
    fi
	
    fan3_rpm_normal_num=0
    fan3_front_rpm=`cat $FAN3_FRONT_RPM` 
    fan3_rear_rpm=`cat $FAN3_REAR_RPM`
    if [ "${fan3_front_rpm}" -ne "960" ] && [ "${fan3_rear_rpm}" -ne "960" ] && [ "${fan3_front_rpm}" -ne "0" ] && [ "${fan3_rear_rpm}" -ne "0" ]; then
        fan3_rpm_normal_num=$((fan3_rpm_normal_num+1))
    elif [ "${fan3_front_rpm}" -eq "960" ] || [ "${fan3_rear_rpm}" -eq "960" ] || [ "${fan3_front_rpm}" -eq "0" ] || [ "${fan3_rear_rpm}" -eq "0" ]; then
        fan3_rpm_normal_num=$((fan3_rpm_normal_num))
    fi
	
    fan4_rpm_normal_num=0
    fan4_front_rpm=`cat $FAN4_FRONT_RPM` 
    fan4_rear_rpm=`cat $FAN4_REAR_RPM`
    if [ "${fan4_front_rpm}" -ne "960" ] && [ "${fan4_rear_rpm}" -ne "960" ] && [ "${fan4_front_rpm}" -ne "0" ] && [ "${fan4_rear_rpm}" -ne "0" ]; then
        fan4_rpm_normal_num=$((fan4_rpm_normal_num+1))
    elif [ "${fan4_front_rpm}" -eq "960" ] || [ "${fan4_rear_rpm}" -eq "960" ] || [ "${fan4_front_rpm}" -eq "0" ] || [ "${fan4_rear_rpm}" -eq "0" ]; then
        fan4_rpm_normal_num=$((fan4_rpm_normal_num))
    fi
	
    fan5_rpm_normal_num=0
    fan5_front_rpm=`cat $FAN5_FRONT_RPM` 
    fan5_rear_rpm=`cat $FAN5_REAR_RPM`
    if [ "${fan5_front_rpm}" -ne "960" ] && [ "${fan5_rear_rpm}" -ne "960" ] && [ "${fan5_front_rpm}" -ne "0" ] && [ "${fan5_rear_rpm}" -ne "0" ]; then
        fan5_rpm_normal_num=$((fan5_rpm_normal_num+1))
    elif [ "${fan5_front_rpm}" -eq "960" ] || [ "${fan5_rear_rpm}" -eq "960" ] || [ "${fan5_front_rpm}" -eq "0" ] || [ "${fan5_rear_rpm}" -eq "0" ]; then
        fan5_rpm_normal_num=$((fan5_rpm_normal_num))
    fi
	
    fan_rpm_normal_num=$((fan1_rpm_normal_num+fan2_rpm_normal_num+fan3_rpm_normal_num+fan4_rpm_normal_num+fan5_rpm_normal_num))
	
}

catpsueeprom(){
    psu1_eeprom_num=0	
    i2cget $PSU1_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        psu1_eeprom_num=$((psu1_eeprom_num+1))
    elif [ "`echo $?`" -eq "2" ]; then
        psu1_eeprom_num=$((psu1_eeprom_num))
    fi
	
    psu2_eeprom_num=0	
    i2cget $PSU2_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        psu2_eeprom_num=$((psu2_eeprom_num+1))
    elif [ "`echo $?`" -eq "2" ]; then
        psu2_eeprom_num=$((psu2_eeprom_num))
    fi
}

catpsufanspeed(){

    psu1_rpm_normal_num=0
    psu1_rpm=`cat $PSU1_FAN_RPM`
	
    if [ "${psu1_rpm}" -ne "960" ] && [ "${psu1_rpm}" -ne "0" ]; then
        psu1_rpm_normal_num=$((psu1_rpm_normal_num+1))
    elif [ "${psu1_rpm}" -eq "960" ] || [ "${psu1_rpm}" -eq "0" ]; then
        psu1_rpm_normal_num=$((psu1_rpm_normal_num))
    fi
	
    psu2_rpm_normal_num=0
    psu2_rpm=`cat $PSU2_FAN_RPM` 
	
    if [ "${psu2_rpm}" -ne "960" ] && [ "${psu2_rpm}" -ne "0" ]; then
        psu2_rpm_normal_num=$((psu2_rpm_normal_num+1))
    elif [ "${psu2_rpm}" -eq "960" ] || [ "${psu2_rpm}" -eq "0" ]; then
        psu2_rpm_normal_num=$((psu2_rpm_normal_num))
    fi

}

setfanled(){
    if [ "${fan_eeprom_num}" -eq "5" ] && [ "${fan_rpm_normal_num}" -eq "5" ]; then
        echo "fan_green" > $LED_CONTROL
    elif [ "${fan_eeprom_num}" -lt "5" ] || [ "${fan_rpm_normal_num}" -lt "5" ]; then
        echo "fan_amber" > $LED_CONTROL
    fi
}

setpsuled(){
	if [ "${psu1_eeprom_num}" -eq "1" ] && [ "${psu1_rpm_normal_num}" -eq "1" ]; then
        echo "pwr1_green" > $LED_CONTROL
    elif [ "${psu1_eeprom_num}" -eq "0" ] || [ "${psu1_rpm_normal_num}" -eq "0" ]; then
        echo "pwr1_amber" > $LED_CONTROL
    fi
	
	if [ "${psu2_eeprom_num}" -eq "1" ] && [ "${psu2_rpm_normal_num}" -eq "1" ]; then
        echo "pwr2_green" > $LED_CONTROL
    elif [ "${psu2_eeprom_num}" -eq "0" ] || [ "${psu2_rpm_normal_num}" -eq "0" ]; then
        echo "pwr2_amber" > $LED_CONTROL
    fi
}

setfantrayled(){
    if [ "${fan1_rpm_normal_num}" -eq "1" ]; then
        echo "fan1_green" > $LED_CONTROL
    else
        echo "fan1_red" > $LED_CONTROL
    fi
	
    if [ "${fan2_rpm_normal_num}" -eq "1" ]; then
        echo "fan2_green" > $LED_CONTROL
    else
        echo "fan2_red" > $LED_CONTROL
    fi
	
    if [ "${fan3_rpm_normal_num}" -eq "1" ]; then
        echo "fan3_green" > $LED_CONTROL
    else
        echo "fan3_red" > $LED_CONTROL
    fi
	
    if [ "${fan4_rpm_normal_num}" -eq "1" ]; then
        echo "fan4_green" > $LED_CONTROL
    else
        echo "fan4_red" > $LED_CONTROL
    fi
	
    if [ "${fan5_rpm_normal_num}" -eq "1" ]; then
        echo "fan5_green" > $LED_CONTROL
    else
        echo "fan5_red" > $LED_CONTROL
    fi
}

platformstatus(){
    
    echo "sys_green" > $LED_CONTROL
    catfaneeprom
    catfanspeed
    setfanled
    setfantrayled
	
    catpsueeprom
    catpsufanspeed
    setpsuled	
}

while true
do
    platformstatus
    sleep 1
done

