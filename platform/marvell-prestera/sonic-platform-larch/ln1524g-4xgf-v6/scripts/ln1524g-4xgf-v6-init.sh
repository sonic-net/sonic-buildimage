#!/bin/bash

# Platform init script
ln1524g_4xgf_v6_profile()
{
	MAC_ADDR=$(fw_printenv -n ethaddr 2>/dev/null)
	if [ -z "$MAC_ADDR" ]; then
	    echo "$(basename "$0"): could not read ethaddr from the U-Boot environment;"
	    echo "$(basename "$0"): leaving switchMacAddress in profile.ini untouched" >&2
	    return 0
	fi
	find /usr/share/sonic/device/*ln1524g-4xgf-v6* -name profile.ini | xargs sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g"
	echo "ln1524g-4xgf-v6: Updating switch mac address ${MAC_ADDR}"

    # Set SFP+ Port LED in link/active mode.
	BUS=0
    ADDR=0x30
    REG=0xA0
    MASK=0x01

    old=$(i2cget -y $BUS $ADDR $REG)
    new=$(printf "0x%02X" $(( old | MASK )))
    echo "CPLD reg $REG: $old -> $new"
    i2cset -y $BUS $ADDR $REG $new
        
    modprobe optoe

    # TODO: figure out why first load doesn't bind device to the driver
    #rmmod i2c_mux_mlxcpld
    #modprobe i2c_mux_mlxcpld   
}

# Main
ln1524g_4xgf_v6_profile

exit 0
