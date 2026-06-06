#!/bin/bash

#!/bin/bash

function debug()
{
    /usr/bin/logger $1
}

start() {
   
    platforms=( \
            "x86_64-ruijie_b6990-128qc2xs-r0" \
            )

    result=$(cat /host/machine.conf | grep onie_platform | cut -d = -f 2)
    echo $result

    ext_phy_set=0
    for i in ${platforms[*]}; do
        if [ $result == $i ];
        then
            ext_phy_set=1
            break
        fi
    done

    if [ $ext_phy_set -eq 1 ];
    then
		# Restore the shared register page
		i2cset -f -y 157 0x18 0xff 0x00
		# Write 4 channel registers at the same time, display channel 0 register page
		i2cset -f -y 157 0x18 0xff 0x0c
		# Reset four channel registers simultaneously
		i2cset -f -y 157 0x18 0x00 0x04
		# Display the result of channel register modification
		i2cdump -f -y 157 0x18 b | debug


		# At the same time, set four channels to enter the CTLE and DFE adaptive mode
		i2cset -f -y 157 0x18 0x31 0x40
		# Restore the shared register page
		i2cset -f -y 157 0x18 0xff 0x00
		# Data is written only to the channel 0 register and the channel 0 register page is displayed
		i2cset -f -y 157 0x18 0xff 0x04
		# Set the preemphasis of channel 0 to -3.3db
		i2cset -f -y 157 0x18 0x15 0x54
		# Display the result of channel register modification
		i2cdump -f -y 157 0x18 b | debug
		# Read back register 0x15 to verify the write result
		value=$(i2cget -f -y 157 0x18 0x15)
		# Determines whether the value of register 0x15 is 0x54
		if [ "$value" == "0x54" ]; then
			debug "Channel 0 writes address 0x15 to 0x54 successfully"
		else
			debug "Channel 0 writes address 0x15 to 0x54 failed"
		fi

		# Restore the shared register page
		i2cset -f -y 157 0x18 0xff 0x00
		# Data is written only to the channel 1 register and the channel 1 register page is displayed
		i2cset -f -y 157 0x18 0xff 0x05
		# Set the de-emphasis of channel 1 to -3.3db
		i2cset -f -y 157 0x18 0x15 0x54
		# Display the result of channel register modification
		i2cdump -f -y 157 0x18 b | debug
		# Read back register 0x15 to verify the write result
		value=$(i2cget -f -y 157 0x18 0x15)
		# Determines whether the value of register 0x15 is 0x54
		if [ "$value" == "0x54" ]; then
			debug "Channel 1 writes address 0x15 to 0x54 successfully"
		else
			debug "Channel 1 writes address 0x15 to 0x54 failed"
		fi
    fi

}

wait() {
    exit 0
}

stop() {
    exit 0
}

case "$1" in
    start|wait|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|wait|stop}"
        exit 1
        ;;
esac
