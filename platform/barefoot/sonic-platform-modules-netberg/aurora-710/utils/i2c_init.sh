#!/bin/bash

# vdd value for mac
rov_val_array=( 0.85 0.82 0.77 0.87 0.74 0.84 0.79 0.89 )
rov_reg_array=( 0x24 0x21 0x1c 0x26 0x19 0x23 0x1e 0x28 )

#GPIO Offset
GPIO_OFFSET=0

function _set_gpio_offset {
    GPIO_OFFSET=0
    gpio_base_max=`cat /sys/class/gpio/gpiochip*/base | sort -n | tail -1`
    GPIO_OFFSET=$(($gpio_base_max - 240))
    echo "set GPIO_OFFSET=${GPIO_OFFSET}"
}

#GPIO Init
function i2c_gpio_init {
    local i=0
    #ABS Port 0-15
    echo "pca9535 0x20" > /sys/bus/i2c/devices/i2c-5/new_device
    _set_gpio_offset
    #for i in {240..255};
    for((i=${GPIO_OFFSET}+240;i<=${GPIO_OFFSET}+255;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
    done

    #ABS Port 16-31
    echo "pca9535 0x21" > /sys/bus/i2c/devices/i2c-5/new_device
    #for i in {224..239};
    for((i=${GPIO_OFFSET}+224;i<=${GPIO_OFFSET}+239;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
    done

    #INT Port 0-15
    echo "pca9535 0x22" > /sys/bus/i2c/devices/i2c-5/new_device
    #for i in {208..223};
    for((i=${GPIO_OFFSET}+208;i<=${GPIO_OFFSET}+223;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
    done

    #INT Port 16-31
    echo "pca9535 0x23" > /sys/bus/i2c/devices/i2c-5/new_device
    #for i in {192..207};
    for((i=${GPIO_OFFSET}+192;i<=${GPIO_OFFSET}+207;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
    done

    #SFP+
    echo "pca9535 0x27" > /sys/bus/i2c/devices/i2c-5/new_device
    #for i in {176..191};
    for((i=${GPIO_OFFSET}+176;i<=${GPIO_OFFSET}+191;i++));
    do
        echo $i > /sys/class/gpio/export
        case ${i} in
            #176|177|178|179|182|183|188|189|190|191)
            $((${GPIO_OFFSET}+176)) | \
            $((${GPIO_OFFSET}+177)) | \
            $((${GPIO_OFFSET}+178)) | \
            $((${GPIO_OFFSET}+179)) | \
            $((${GPIO_OFFSET}+182)) | \
            $((${GPIO_OFFSET}+183)) | \
            $((${GPIO_OFFSET}+188)) | \
            $((${GPIO_OFFSET}+189)) | \
            $((${GPIO_OFFSET}+190)) | \
            $((${GPIO_OFFSET}+191)) )
                echo 1 > /sys/class/gpio/gpio${i}/active_low
            ;;
            #180|181|184|185|186|187)
            $((${GPIO_OFFSET}+180)) | \
            $((${GPIO_OFFSET}+181)) | \
            $((${GPIO_OFFSET}+184)) | \
            $((${GPIO_OFFSET}+185)) | \
            $((${GPIO_OFFSET}+186)) | \
            $((${GPIO_OFFSET}+187)) )
                echo out > /sys/class/gpio/gpio${i}/direction
            ;;
        esac
    
    done

    #LP Mode Port 0-15
    echo "pca9535 0x20" > /sys/bus/i2c/devices/i2c-6/new_device
    #for i in {160..175};
    for((i=${GPIO_OFFSET}+160;i<=${GPIO_OFFSET}+175;i++));
    do
        echo $i > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio${i}/direction
    done

    #LP Mode Port 16-31
    echo "pca9535 0x21" > /sys/bus/i2c/devices/i2c-6/new_device
    #for i in {144..159};
    for((i=${GPIO_OFFSET}+144;i<=${GPIO_OFFSET}+159;i++));
    do
        echo $i > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio${i}/direction
    done

    #RST Port 0-15
    echo "pca9535 0x22" > /sys/bus/i2c/devices/i2c-6/new_device
    #for i in {128..143};
    for((i=${GPIO_OFFSET}+128;i<=${GPIO_OFFSET}+143;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
        echo low > /sys/class/gpio/gpio${i}/direction
    done

    #RST Port 16-31
    echo "pca9535 0x23" > /sys/bus/i2c/devices/i2c-6/new_device
    #for i in {112..127};
    for((i=${GPIO_OFFSET}+112;i<=${GPIO_OFFSET}+127;i++));
    do
        echo $i > /sys/class/gpio/export
        echo 1 > /sys/class/gpio/gpio${i}/active_low
        echo low > /sys/class/gpio/gpio${i}/direction
        #echo out > /sys/class/gpio/gpio${i}/direction
        #echo 0 > /sys/class/gpio/gpio${i}/value
    done
    
    #PSU I/O on Dummy Board 0x25
    echo "pca9535 0x25" > /sys/bus/i2c/devices/i2c-0/new_device
    #for i in {96..111};
    for((i=${GPIO_OFFSET}+96;i<=${GPIO_OFFSET}+111;i++));
    do
        echo $i > /sys/class/gpio/export
        case ${i} in
            #97|98|100|101|102|105|106|108)
            $((${GPIO_OFFSET}+97))  | \
            $((${GPIO_OFFSET}+98))  | \
            $((${GPIO_OFFSET}+100)) | \
            $((${GPIO_OFFSET}+101)) | \
            $((${GPIO_OFFSET}+102)) | \
            $((${GPIO_OFFSET}+105)) | \
            $((${GPIO_OFFSET}+106)) | \
            $((${GPIO_OFFSET}+108)) )
                echo 1 > /sys/class/gpio/gpio${i}/active_low
            ;;
            #98|101|106|107|108)
            $((${GPIO_OFFSET}+98))  | \
            $((${GPIO_OFFSET}+101)) | \
            $((${GPIO_OFFSET}+106)) | \
            $((${GPIO_OFFSET}+107)) | \
            $((${GPIO_OFFSET}+108)) )
                echo out > /sys/class/gpio/gpio${i}/direction
            ;;
        esac
    done
}

#IO Expander Init
function _i2c_io_exp_init {
	# Reset BMC Dummy Board
	i2cset -y -r 0 0x26 4 0x00
	i2cset -y -r 0 0x26 5 0x00
	i2cset -y -r 0 0x26 2 0x3F
	i2cset -y -r 0 0x26 3 0x1F
	i2cset -y -r 0 0x26 6 0xC0
	i2cset -y -r 0 0x26 7 0x00

	# CPU Board
	i2cset -y -r 0 0x77 6 0xFF
	i2cset -y -r 0 0x77 7 0xFF

	# init SMBUS1 ABS
	i2cset -y -r 5 0x20 4 0x00
	i2cset -y -r 5 0x20 5 0x00
	i2cset -y -r 5 0x20 6 0xFF
	i2cset -y -r 5 0x20 7 0xFF

	i2cset -y -r 5 0x21 4 0x00
	i2cset -y -r 5 0x21 5 0x00
	i2cset -y -r 5 0x21 6 0xFF
	i2cset -y -r 5 0x21 7 0xFF

	i2cset -y -r 5 0x22 4 0x00
	i2cset -y -r 5 0x22 5 0x00
	i2cset -y -r 5 0x22 6 0xFF
	i2cset -y -r 5 0x22 7 0xFF

	i2cset -y -r 5 0x23 4 0x00
	i2cset -y -r 5 0x23 5 0x00
	i2cset -y -r 5 0x23 2 0xCF
	i2cset -y -r 5 0x23 3 0xF0
	i2cset -y -r 5 0x23 6 0xCF
	i2cset -y -r 5 0x23 7 0xF0

	# init SFP
	i2cset -y -r 5 0x27 4 0x00
	i2cset -y -r 5 0x27 5 0x00
	i2cset -y -r 5 0x27 2 0x00
	i2cset -y -r 5 0x27 3 0x00
	i2cset -y -r 5 0x27 6 0xCF
	i2cset -y -r 5 0x27 7 0xF0

	# set ZQSFP LP_MODE = 0
	i2cset -y -r 6 0x20 4 0x00
	i2cset -y -r 6 0x20 5 0x00
	i2cset -y -r 6 0x20 2 0x00
	i2cset -y -r 6 0x20 3 0x00
	i2cset -y -r 6 0x20 6 0x00
	i2cset -y -r 6 0x20 7 0x00

	i2cset -y -r 6 0x21 4 0x00
	i2cset -y -r 6 0x21 5 0x00
	i2cset -y -r 6 0x21 2 0x00
	i2cset -y -r 6 0x21 3 0x00
	i2cset -y -r 6 0x21 6 0x00
	i2cset -y -r 6 0x21 7 0x00

	# set ZQSFP RST = 1
	i2cset -y -r 6 0x22 4 0x00
	i2cset -y -r 6 0x22 5 0x00
	i2cset -y -r 6 0x22 2 0xFF
	i2cset -y -r 6 0x22 3 0xFF
	i2cset -y -r 6 0x22 6 0x00
	i2cset -y -r 6 0x22 7 0x00

	i2cset -y -r 6 0x23 4 0x00
	i2cset -y -r 6 0x23 5 0x00
	i2cset -y -r 6 0x23 2 0xFF
	i2cset -y -r 6 0x23 3 0xFF
	i2cset -y -r 6 0x23 6 0x00
	i2cset -y -r 6 0x23 7 0x00

	# init Host GPIO
	i2cset -y -r 0 0x74 4 0x00
	i2cset -y -r 0 0x74 5 0x00
	i2cset -y -r 0 0x74 2 0x0F
	i2cset -y -r 0 0x74 3 0xDF
	i2cset -y -r 0 0x74 6 0x08
	i2cset -y -r 0 0x74 7 0x1F

	# init Board ID
	i2cset -y -r 51 0x27 4 0x00
	i2cset -y -r 51 0x27 5 0x00
	i2cset -y -r 51 0x27 6 0xFF
	i2cset -y -r 51 0x27 7 0xFF

	# init Board ID of Dummy BMC Board
	i2cset -y -r 0 0x24 4 0x00
	i2cset -y -r 0 0x24 5 0x00
	i2cset -y -r 0 0x24 6 0xFF
	i2cset -y -r 0 0x24 7 0xFF

	# init PSU I/O (BAREFOOT_IO_EXP_PSU_ID)
	i2cset -y -r 0 0x25 4 0x00
	i2cset -y -r 0 0x25 5 0x00
	i2cset -y -r 0 0x25 2 0x00
	i2cset -y -r 0 0x25 3 0x1D
	i2cset -y -r 0 0x25 6 0xDB
	i2cset -y -r 0 0x25 7 0x03

	# init FAN I/O (BAREFOOT_IO_EXP_FAN_ID)
	i2cset -y -r 59 0x20 4 0x00
	i2cset -y -r 59 0x20 5 0x00
	i2cset -y -r 59 0x20 2 0x11
	i2cset -y -r 59 0x20 3 0x11
	i2cset -y -r 59 0x20 6 0xCC
	i2cset -y -r 59 0x20 7 0xCC

	# init Fan
	# select bank 0
	i2cset -y -r 56 0x2F 0x00 0x80

	# enable FANIN 1-8
	i2cset -y -r 56 0x2F 0x06 0xFF

	# disable FANIN 9-14
	i2cset -y -r 56 0x2F 0x07 0x00

	# select bank 2
	i2cset -y -r 56 0x2F 0x00 0x82

	# set PWM mode in FOMC
	i2cset -y -r 56 0x2F 0x0F 0x00

	# init VOLMON
	i2cset -y -r 56 0x2F 0x00 0x80
	i2cset -y -r 56 0x2F 0x01 0x1C
	i2cset -y -r 56 0x2F 0x00 0x80
	i2cset -y -r 56 0x2F 0x02 0xFF
	i2cset -y -r 56 0x2F 0x03 0x50
	i2cset -y -r 56 0x2F 0x04 0x0A
	i2cset -y -r 56 0x2F 0x00 0x80
	i2cset -y -r 56 0x2F 0x01 0x1D
}

function mac_vdd_init {
    # read mac vid register value from CPLD
    val=`i2cget -y 44 0x33 0x42 2>/dev/null`

    # get vid form register value [0:2]
    vid=$(($val & 0x7))

    # get rov val and reg according to vid
    rov_val=${rov_val_array[$vid]}
    rov_reg=${rov_reg_array[$vid]}
    echo "vid=${vid}, rov_val=${rov_val}, rov_reg=${rov_reg}"

    # write the rov reg to rov
    i2cset -y -r 8 0x22 0x21 ${rov_reg} w

    if [ $? -eq 0 ]; then
        echo "set ROV for mac vdd done"
    else
        echo "set ROV for mac vdd fail"
    fi
}

rmmod i2c_i801
modprobe i2c_i801
modprobe i2c_dev
modprobe i2c_mux_pca954x

echo 'pca9548 0x70' > /sys/bus/i2c/devices/i2c-0/new_device
#todo: switch to channels 
echo 'pca9548 0x71' > /sys/bus/i2c/devices/i2c-1/new_device
echo 'pca9548 0x71' > /sys/bus/i2c/devices/i2c-2/new_device
echo 'pca9548 0x71' > /sys/bus/i2c/devices/i2c-3/new_device
echo 'pca9548 0x71' > /sys/bus/i2c/devices/i2c-4/new_device
echo 'pca9548 0x71' > /sys/bus/i2c/devices/i2c-7/new_device
echo 'pca9548 0x76' > /sys/bus/i2c/devices/i2c-0/new_device
echo 'pca9545 0x72' > /sys/bus/i2c/devices/i2c-0/new_device


echo '-2' > /sys/bus/i2c/devices/0-0070/idle_state
echo '-2' > /sys/bus/i2c/devices/1-0071/idle_state
echo '-2' > /sys/bus/i2c/devices/2-0071/idle_state
echo '-2' > /sys/bus/i2c/devices/3-0071/idle_state
echo '-2' > /sys/bus/i2c/devices/4-0071/idle_state
echo '-2' > /sys/bus/i2c/devices/7-0071/idle_state
echo '-2' > /sys/bus/i2c/devices/0-0076/idle_state
echo '-2' > /sys/bus/i2c/devices/0-0072/idle_state



# i2cget -y 44 0x74 2

_i2c_io_exp_init 

echo 'w83795adg 0x2F' > /sys/bus/i2c/devices/i2c-56/new_device

sleep 1

echo 120 > /sys/class/hwmon/hwmon1/device/pwm1
echo 120 > /sys/class/hwmon/hwmon1/device/pwm2

#TMP75 Init
echo "lm86 0x4c" > /sys/bus/i2c/devices/i2c-53/new_device  # ASIC Coretemp and Front MAC
echo "tmp75 0x4f" > /sys/bus/i2c/devices/i2c-0/new_device #CPU Board
echo "tmp75 0x48" > /sys/bus/i2c/devices/i2c-53/new_device # Near PSU1
echo "tmp75 0x4a" > /sys/bus/i2c/devices/i2c-53/new_device # Rear MAC
echo "tmp75 0x4b" > /sys/bus/i2c/devices/i2c-53/new_device # Near Port 32
echo "tmp75 0x4d" > /sys/bus/i2c/devices/i2c-53/new_device # Near PSU2



rmmod gpio-pca953x
rmmod gpio_ich 

modprobe gpio-pca953x
sleep 1

i2c_gpio_init

modprobe at24

#Init MB EEPROM
echo "24c32 0x55" > /sys/bus/i2c/devices/i2c-0/new_device

# modprobe eeprom
# PS EEPROM
echo "spd 0x50" > /sys/bus/i2c/devices/i2c-58/new_device #PS0
echo "spd 0x50" > /sys/bus/i2c/devices/i2c-57/new_device #PS1

#modprobe pmbus

# PS PMBUS
echo "pmbus 0x58" > /sys/bus/i2c/devices/i2c-58/new_device #PS0
echo "pmbus 0x58" > /sys/bus/i2c/devices/i2c-57/new_device #PS1

#QSFP EEPROM
modprobe optoe
for i in {9..40};
do
    echo "optoe1 0x50" > /sys/bus/i2c/devices/i2c-$i/new_device
done

# init SFP0/1 EEPROM
echo "optoe2 0x50" > /sys/bus/i2c/devices/i2c-45/new_device
echo "optoe2 0x50" > /sys/bus/i2c/devices/i2c-46/new_device

mac_vdd_init

