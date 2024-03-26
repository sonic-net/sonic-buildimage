#!/bin/bash

# init Host GPIO 0x74
i2cset -f -y -r 0 0x74 4 0x00
i2cset -f -y -r 0 0x74 5 0x00
i2cset -f -y -r 0 0x74 2 0x0F
i2cset -f -y -r 0 0x74 3 0xDF
i2cset -f -y -r 0 0x74 6 0x08
i2cset -f -y -r 0 0x74 7 0x1F

gpio_base=511

# gpio sysfs active_low - ABS Port 0-31
# gpio 480-511
start=$((gpio_base-31))
end=$gpio_base
for (( i=$start; i<=$end; i++ ))
do
    echo 1 > /sys/class/gpio/gpio${i}/active_low
done

# gpio sysfs active_low - Intr Port 0-31
# gpio 448-479
start=$((gpio_base-63))
end=$((gpio_base-32))
for (( i=$start; i<=$end; i++ ))
do
    echo 1 > /sys/class/gpio/gpio${i}/active_low
done


# gpio sysfs active_low - SFP Port
# gpio 432-447
sfp_active_low_array=(1 1 1 1 0 0 1 1 \
                      0 0 0 0 1 1 1 1)
start=$((gpio_base-79))
end=$((gpio_base-64))
for (( i=$start; i<=$end; i++ ))
do
    echo ${sfp_active_low_array[$((i-start))]} > /sys/class/gpio/gpio${i}/active_low
done

# gpio sysfs active_low - LP Mode Mode Port 0-31
# gpio 400-431
start=$((gpio_base-111))
end=$((gpio_base-80))
for (( i=$start; i<=$end; i++ ))
do
    echo 0 > /sys/class/gpio/gpio${i}/active_low
done

# gpio sysfs active_low - RST Mode Port 0-31
# gpio 368-399
start=$((gpio_base-143))
end=$((gpio_base-112))
for (( i=$start; i<=$end; i++ ))
do
    echo 1 > /sys/class/gpio/gpio${i}/active_low
    echo 0 > /sys/class/gpio/gpio${i}/value
done

# init QSFP port name
qsfp_bus_array=(10 9 12 11 14 13 16 15 18 17 \
                20 19 22 21 24 23 26 25 28 27 \
                30 29 32 31 34 33 36 35 38 37 \
                40 39)
for i in {0..31};
do
    echo $((i + 1)) > /sys/bus/i2c/devices/${qsfp_bus_array[i]}-0050/port_name
    #echo "echo $((i + 1)) > /sys/bus/i2c/devices/${qsfp_bus_array[i]}-0050/port_name"
done

# init SFP port name
sfp_bus_array=(45 46)
for i in {0..1};
do
    echo $((i + 33)) > /sys/bus/i2c/devices/${sfp_bus_array[i]}-0050/port_name
done

# _mac_vdd_init
# vid to mac vdd value mapping
vdd_val_array=( 0.85  0.82  0.77  0.87  0.74  0.84  0.79  0.89 )
# vid to rov reg value mapping
rov_reg_array=( 0x24  0x21  0x1C  0x26  0x19 0x23 0x1E 0x28 )

reg_val=$(eval "i2cget -f -y 44 0x33 0x42 2>/dev/null")
vid=$(($reg_val & 0x7))
mac_vdd_val=${vdd_val_array[vid]}
rov_reg=${rov_reg_array[vid]}
i2cset -f -y -r 8 0x22 0x21 ${rov_reg} w 2>/dev/null

echo "PDDF device post-create completed"

