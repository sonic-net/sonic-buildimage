#!/bin/bash
set -x
set -e
path=$(dirname $0)
cd ${path}

product=$(decode-syseeprom | grep "Product Name" | awk -F ' ' '{print $5}' | tr -d '\n')
# Just for B4020
if [ "$product" != "B4020" ]; then
    exit 0
fi

function get_68127_0v75_low_threshold()
{
    echo "path 0x1d7 addr 0x5c read from 0x44 len 0x1 mode 0x3" > /sys/switch/debug/i2c/param
    val=`cat /sys/switch/debug/i2c/mem_dump | grep 0x44: | awk '{print $2}'| tr -d '\n'`
    echo "$val"
}

function set_68127_0v75_low_threshold()
{
    echo "path 0x1d7 addr 0x5c write inner 0x44 value $1 mode 0x3"> /sys/switch/debug/i2c/param
    cat /sys/switch/debug/i2c/do_write > /dev/null
    val=$(get_68127_0v75_low_threshold)
    if [ "$1" != "$val" ];then
        echo "Error 68127 setting $1 error, check value is $val"
        exit 1
    fi
}

if [ $1 = "install" ]; then
    set_68127_0v75_low_threshold 0x0000
elif [ $1 = "rollback" ]; then
    set_68127_0v75_low_threshold 0x02c4
fi
