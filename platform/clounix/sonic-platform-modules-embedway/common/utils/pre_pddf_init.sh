#!/bin/bash

#为确保hid-cp2112被挂载在i2c-0，先rmmod全部i2c adapter，再重新insmod hid-cp2112
i2c_driver_rmmod()
{
    module_list=("i2c_i801" "i2c_ismt" "hid_cp2112")
    for module in ${module_list[@]};do
        echo $module >> /tmp/a.txt
        if [ `lsmod | grep -c $module ` -ne 0 ]; then
            rmmod -f $module 
        fi
    done
}

i2c_driver_init()
{
    #没放在/lib/modules/4.19.0-12-2-amd64/路径，防止被depmod后探测到设备自动加载(此时rmmod有概率会失败）
    modprobe hid
    #sonic image(20250218)将hid-cp2112.ko放到了/lib/modules/4.19.0-12-2-amd64/extra/
    kernel=`uname -r`
    insmod /lib/modules/$kernel/extra/hid-cp2112.ko
}

bmc_driver_init()
{
    result=$(i2cget -f -y 0 0x62 0x50 2>/dev/null)
    hi_byte=${result:2:1}
    case $hi_byte in
        0|4|8|c)
            bmc_armd="FALSE"
            echo "BMC is not armed"
            ;;
        *)
            bmc_armd="TRUE"
            echo "BMC is armed"
            ;;
    esac

    PLATFORM=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)
    DEVICE="/usr/share/sonic/device"
    if [ x"$bmc_armd" == x"TRUE" ]; then
        modprobe ipmi_devintf
        modprobe ipmi_ssif
        modprobe ipmi_msghandler
        modprobe ipmi_si type=kcs ports=0xca2 regspacings=1
        sleep 1

        echo "Detecting BMC sensor..."
        while true; do
            MBA1_TEMP1=$(ipmitool sdr | grep MBA1_TEMP1 | grep ok 2>/dev/null)
            if [ -n "$MBA1_TEMP1" ]; then
                echo "BMC sensor Ready"
                break
            else
                echo "BMC sensor not Ready, Wait 5s and try again"
                sleep 5
            fi
        done

        kernel=`uname -r`
        rmmod ipmi_i2c_driver 2>/dev/null
        insmod /lib/modules/$kernel/extra/ipmi_i2c_driver.ko
        sleep 1

        #replace pddf.json
        cp -arf $DEVICE/$PLATFORM/pddf/bmc_pddf-device.json $DEVICE/$PLATFORM/pddf/pddf-device.json
    else
	 #replace pddf.json
        detect_0x55=`i2cget -f -y 0 0x55 2>/dev/null`
        if [ "${detect_0x55}" ]; then
            echo "EEPROM 0x55 detected"
            cp -arf $DEVICE/$PLATFORM/pddf/default_pddf-device_0x55.json $DEVICE/$PLATFORM/pddf/pddf-device.json
        else
            cp -arf $DEVICE/$PLATFORM/pddf/default_pddf-device.json $DEVICE/$PLATFORM/pddf/pddf-device.json
        fi
    fi
}

i2c_driver_rmmod
i2c_driver_init
modprobe i2c_dev
sleep 1
bmc_driver_init

echo "esx previous PDDF driver install completed"
