#!/bin/bash
normal='0 : normal'
unpowered='2 : unpowered'
fault='4 : fault'
notinstalled='7 : not installed'
FAN_UNPLUG_NUM=0
FAN_LED_RED='fan_led_red'
NUM=1
FAN_NUM=5
FIRST_READ=0
SECOND_READ=0

PSOC_PATH="/tmp/psoc"
CPLD_ADDR="/sys/bus/i2c/devices/i2c-0/0-0077"
ROUTE="/sys/class/hwmon/"


#PSU_STAUS='000'
#switch is ready , transfer control of cpld to cpu
echo 1 > $CPLD_ADDR/ctl

while true
do

#monitor how many fan modules are unplugged


#first check
FAN_UNPLUG_NUM=0
FAN_ARR=$(cat $PSOC_PATH/$FAN_LED_RED?)

while read -r line; do
    fan_led_red_check=$(echo "$line")
    if [ $fan_led_red_check -eq 1 ]
    then
    let FAN_UNPLUG_NUM=FAN_UNPLUG_NUM+1
    fi 
done <<< "$FAN_ARR"
FIRST_READ=$FAN_UNPLUG_NUM

#second check
FAN_UNPLUG_NUM=0
FAN_ARR=$(cat $PSOC_PATH/$FAN_LED_RED?)

while read -r line; do
    fan_led_red_check=$(echo "$line")
    if [ $fan_led_red_check -eq 1 ]
    then
    let FAN_UNPLUG_NUM=FAN_UNPLUG_NUM+1
    fi 
done <<< "$FAN_ARR"
SECOND_READ=$FAN_UNPLUG_NUM

if [ $FIRST_READ -ne $SECOND_READ ]
then
  #echo "not equl:$FIRST_READ != $SECOND_READ"
  continue
fi 

if [ $FAN_UNPLUG_NUM -ge 2 ]
then
  #echo "solid red"  
  echo 7 > $CPLD_ADDR/red_led
  echo 0 > $CPLD_ADDR/grn_led
  sleep 1
  continue
elif [ $FAN_UNPLUG_NUM -eq 1 ]
then
  #solid orange
  echo 7 > $CPLD_ADDR/red_led
  echo 7 > $CPLD_ADDR/grn_led
  sleep 1
  continue
fi

  #echo "normal"
  psu0var=$(cat $CPLD_ADDR/psu0)    # bottom PSU
  psu1var=$(cat $CPLD_ADDR/psu1)    # top PSU

   if [ "$psu0var" = "$normal" ] &&
     [ "$psu1var" = "$normal" ]                          # PSU normal operatio
   then
      #solid green
      echo 7 > $CPLD_ADDR/grn_led
      echo 0 > $CPLD_ADDR/red_led
      #echo "solid green"
   else  
      if [ "$psu0var" = "$unpowered" ] ||
      [ "$psu1var" = "$unpowered" ]
      then
        #echo solid orange
        echo 7 > $CPLD_ADDR/grn_led
        echo 7 > $CPLD_ADDR/red_led
      fi
      
   fi 


sleep 1
done

