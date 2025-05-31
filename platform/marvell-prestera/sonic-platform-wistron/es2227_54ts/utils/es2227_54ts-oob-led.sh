#!/bin/bash

COPPER_TYPE=0
FIBER_TYPE=1

currMediaType=0

RET=-1

set_led()
{
    if [ $1 -eq $COPPER_TYPE ]; then
        a=0; b=1
    else
        a=1; b=0
    fi

    echo $a > /sys/class/gpio/gpio498/value; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 1; fi

    echo $b > /sys/class/gpio/gpio499/value; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 2; fi
}

init()
{
    # init oob port led
    phytool write eth0/0/22 3;
    phytool write eth0/0/17 0x44a5;
    phytool write eth0/0/22 3;
    phytool write eth0/0/16 0x0240;
    phytool write eth0/0/22 0

    # set rj port sgmii amplitude
    phytool write eth0/0/22 2;
    phytool write eth0/0/26 0x8004;
    phytool write eth0/0/22 0

    echo 496 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio496/direction
    for k in $(seq 498 499); do
        echo $k > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio$k/direction
    done

    # default set sfp txdisable to off
    echo 0 > /sys/class/gpio/gpio496/value

    # default set to copper port mode
    currMediaType=$COPPER_TYPE
    set_led $COPPER_TYPE

    # Disable fiber Auto-Negotiation
    phytool write eth0/0/22 1; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 3; fi
    ethtool -s eth0 autoneg off speed 1000 duplex full; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 4; fi
    #echo "Disable fiber Auto-Negotiation"

    # Set auto select mode - Prefer fiber medium
    phytool write eth0/0/22 2; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 5; fi
    phytool write eth0/0/16 0x508; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 6; fi
    #echo "Set auto select mode - Prefer fiber medium"

    # Enable auto medium register selection
    phytool write eth0/0/22 0x8000; RET=`echo $?`
    if [ $RET -ne 0 ]; then exit 7; fi
    #echo "Enable auto medium register selection"
}

do_task()
{
    while true; do
        regVal=`phytool read eth0/0/22`; RET=`echo $?`
        if [ $RET -ne 0 ]; then exit 8; fi

        type=$((regVal & 0x1))

        if [ $type -ne $currMediaType ]; then
            if [ $type -eq $COPPER_TYPE ]; then
                set_led $COPPER_TYPE
                #echo "set led to copper mode"
            else
                set_led $FIBER_TYPE
                #echo "set led to fiber mode"
            fi

            currMediaType=$type
        fi

        sleep 2
    done
}

main()
{
    init
    do_task
}

main
