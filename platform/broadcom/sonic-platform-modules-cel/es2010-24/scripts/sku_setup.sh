#!/bin/bash

DEVICE="/usr/share/sonic/device"
PLATFORM=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)
PLATFORM_PATH="$DEVICE/$PLATFORM"
SKU_SETUP="${PLATFORM_PATH}/sku_setup"

update_hwsku()
{
    SKU1=3
    SKU2=2

    echo "Setting up board... "

    GET_PATH="/sys/devices/platform/sys_cpld1/getreg"

    echo "0xA209" > $GET_PATH
    hwsku=$(cat $GET_PATH | cut -d 'x' -f2)

    sku=$((0x$hwsku & 0x3))

    if [ $sku -eq $SKU1 ]; then
        pname="ES2010-24C"
    elif [ $sku -eq $SKU2 ]; then
        pname="ES2010-24CP"
    else
        echo "Error: Unknown SKU"
        return
    fi

    hwsku_file="$PLATFORM_PATH/default_sku"
    component_file="$PLATFORM_PATH/platform_components.json"

    echo "$pname t1"
    echo "$pname t1" > "$hwsku_file"
    sed -i "s/ES2010/$pname/1" $component_file &> /dev/null
}

main()
{
    update_hwsku
}

# Call the main function

if [ ! -f $SKU_SETUP ]; then
    main
    touch $SKU_SETUP
else
    echo "Board setup already done."
fi
