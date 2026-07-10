#!/bin/bash

FAN1_EEPROM="-y 0 0x55 0x0a"
FAN2_EEPROM="-y 0 0x56 0x0a"
FAN1_RPM="/sys/bus/i2c/devices/0-002e/fan1_input"
FAN2_RPM="/sys/bus/i2c/devices/0-002e/fan2_input"
FAN_TRAY1_LED="/sys/devices/platform/delta-et6248brb-gpio.0/FAN/fan1_led_ag"
FAN_TRAY2_LED="/sys/devices/platform/delta-et6248brb-gpio.0/FAN/fan2_led_ag"

if [ `uname -a | awk '{print $3}'` = "4.9.0-11-2-amd64" ]; then
    SYS_LED_G="/sys/class/gpio/gpio453/value"
    SYS_LED_R="/sys/class/gpio/gpio454/value"
    PWR_LED_G="/sys/class/gpio/gpio455/value"
    PWR_LED_R="/sys/class/gpio/gpio489/value"
    FAN_LED_G="/sys/class/gpio/gpio485/value"
    FAN_LED_R="/sys/class/gpio/gpio494/value"
else
    SYS_LED_G="/sys/class/gpio/gpio197/value"
    SYS_LED_R="/sys/class/gpio/gpio198/value"
    PWR_LED_G="/sys/class/gpio/gpio199/value"
    PWR_LED_R="/sys/class/gpio/gpio233/value"
    FAN_LED_G="/sys/class/gpio/gpio229/value"
    FAN_LED_R="/sys/class/gpio/gpio238/value"
fi

PSU1_EEPROM="-y 2 0x50 0x00"
PSU2_EEPROM="-y 3 0x51 0x00"
PSU1_PG="/sys/devices/platform/delta-et6248brb-gpio.0/PSU/psu1_pg"
PSU2_PG="/sys/devices/platform/delta-et6248brb-gpio.0/PSU/psu2_pg"

catfaneeprom(){
    fan_eeprom_num=0
    i2cget $FAN1_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num+1))
    elif [ "`echo $?`" -ne "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num))
    fi

    i2cget $FAN2_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num+1))
    elif [ "`echo $?`" -ne "0" ]; then
        fan_eeprom_num=$((fan_eeprom_num_num))
    fi
}

catfanspeed(){

    fan_rpm_normal_num=0
    fan1_rpm_normal_num=0
    fan1_rpm=`cat $FAN1_RPM`
	
    if [ "${fan1_rpm}" -ne "960" ] && [ "${fan1_rpm}" -ne "0" ]; then
        fan1_rpm_normal_num=$((fan1_rpm_normal_num+1))
    elif [ "${fan1_rpm}" -eq "960" ] || [ "${fan1_rpm}" -eq "0" ]; then
        fan1_rpm_normal_num=$((fan1_rpm_normal_num))
    fi
	
    fan2_rpm_normal_num=0
    fan2_rpm=`cat $FAN2_RPM` 
	
    if [ "${fan2_rpm}" -ne "960" ] && [ "${fan2_rpm}" -ne "0" ]; then
        fan2_rpm_normal_num=$((fan2_rpm_normal_num+1))
    elif [ "${fan2_rpm}" -eq "960" ] || [ "${fan2_rpm}" -eq "0" ]; then
        fan2_rpm_normal_num=$((fan2_rpm_normal_num))
    fi

    fan_rpm_normal_num=$((fan1_rpm_normal_num+fan2_rpm_normal_num))
}

setfanled(){
    if [ "${fan_eeprom_num}" -eq "2" ] && [ "${fan_rpm_normal_num}" -eq "2" ]; then
        echo "1" > $FAN_LED_G
        echo "0" > $FAN_LED_R
    elif [ "${fan_eeprom_num}" -lt "2" ] || [ "${fan_rpm_normal_num}" -lt "2" ]; then
        echo "1" > $FAN_LED_R
        echo "0" > $FAN_LED_G
    fi
}

setfantrayled(){
    if [ "${fan1_rpm_normal_num}" -eq "1" ]; then
        echo "0x02" > $FAN_TRAY1_LED
    else
        echo "0x01" > $FAN_TRAY1_LED
    fi
	
    if [ "${fan2_rpm_normal_num}" -eq "1" ]; then
        echo "0x02" > $FAN_TRAY2_LED
    else
        echo "0x01" > $FAN_TRAY2_LED
    fi
}

catpsueeprom(){
    psu_eeprom_num=0	
    i2cget $PSU1_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        psu_eeprom_num=$((psu_eeprom_num+1))
    elif [ "`echo $?`" -ne "0" ]; then
        psu_eeprom_num=$((psu_eeprom_num))
    fi
	
    i2cget $PSU2_EEPROM > /dev/null 2>&1
    if [ "`echo $?`" -eq "0" ]; then
        psu_eeprom_num=$((psu_eeprom_num+1))
    elif [ "`echo $?`" -ne "0" ]; then
        psu_eeprom_num=$((psu_eeprom_num))
    fi
}

catpsupowergood(){

    psu_normal_num=0
    psu1_normal_num=0
    psu1_good=`cat $PSU1_PG | head -n 1`

    if [ "${psu1_good}" -eq "1" ]; then
        psu1_normal_num=$((psu1_normal_num+1))
    elif [ "${psu1_good}" -eq "0" ]; then
        psu1_normal_num=$((psu1_normal_num))
    fi

    psu2_normal_num=0
    psu2_good=`cat $PSU2_PG | head -n 1`

    if [ "${psu2_good}" -eq "1" ]; then
        psu2_normal_num=$((psu2_normal_num+1))
    elif [ "${psu2_good}" -eq "0" ]; then
        psu2_normal_num=$((psu2_normal_num))
    fi

    psu_normal_num=$((psu1_normal_num+psu2_normal_num))
}

setpsuled(){
    if [ "${psu_eeprom_num}" -eq "2" ] && [ "${psu_normal_num}" -eq "2" ]; then
        echo "1" > $PWR_LED_G
        echo "0" > $PWR_LED_R
    elif [ "${psu_eeprom_num}" -lt "2" ] || [ "${psu_normal_num}" -lt "2" ]; then
        echo "1" > $PWR_LED_R
        echo "0" > $PWR_LED_G
    fi
}

platformstatus(){    
    echo "1" > $SYS_LED_G
    echo "0" > $SYS_LED_R
    catfaneeprom
    catfanspeed
    setfanled
    setfantrayled
	
    catpsueeprom
    catpsupowergood
    setpsuled	
}

while true
do
    platformstatus
    sleep 1
done

