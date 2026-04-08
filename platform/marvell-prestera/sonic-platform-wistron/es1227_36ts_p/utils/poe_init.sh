#!/bin/sh

echo 1 > /sys/bus/i2c/devices/0-0033/poe_en_ctrl

# Set temporary matrix
#poe_main 0x11 0 4 255
#poe_main 0x11 1 5 255
#poe_main 0x11 2 6 255
#poe_main 0x11 3 7 255
#poe_main 0x11 4 0 255
#poe_main 0x11 5 1 255
#poe_main 0x11 6 2 255
#poe_main 0x11 7 3 255
#poe_main 0x11 8 12 255
#poe_main 0x11 9 13 255
#poe_main 0x11 10 14 255
#poe_main 0x11 11 15 255
#poe_main 0x11 12 8 255
#poe_main 0x11 13 9 255
#poe_main 0x11 14 10 255
#poe_main 0x11 15 11 255
#poe_main 0x11 16 19 255
#poe_main 0x11 17 18 255
#poe_main 0x11 18 17 255
#poe_main 0x11 19 16 255
#poe_main 0x11 20 23 255
#poe_main 0x11 21 22 255
#poe_main 0x11 22 21 255
#poe_main 0x11 23 20 255

# Set 2 pair AT compliant modes
#for port in $(seq 0 23);do
#    poe_main 0x02 $port 1 1 9 3
#done

# Set 4 pair BT compliant modes
#for port in $(seq 32 47);do
#    poe_main 0x02 $port 1 1 0 3
#done

# Get 'Product Name
#product_name=$(show platform syseeprom 2>/dev/null | awk '/0x21/ {for (i=5; i<=NF; i++) printf $i " "; print ""}' | sed 's/ *$//')
#echo "Product_name $product_name"
product_name=$(show ver 2>/dev/null | awk '/Model Number:/ {for (i=3; i<=NF; i++) printf $i " "; print ""}' | sed 's/ *$//')
echo "[POE init] show ver product_name: $product_name"

# [Method 1] Read CPLD driver values
#sku1_psu1=$(cat /sys/bus/i2c/devices/0-0033/sku1_psu1 2>/dev/null || echo "0")
#sku2_psu2=$(cat /sys/bus/i2c/devices/0-0033/sku2_psu2 2>/dev/null || echo "0")

# [Method 2] Read Board ID to determine SKU ID
board_info=$(cat /sys/bus/i2c/devices/0-0033/board_id 2>/dev/null || echo "")
sku_id=""

if echo "$board_info" | grep -q "SKU ID: "; then
    sku_id=$(echo "$board_info" | grep -o "SKU ID: [0-1]" | awk '{print $NF}')
fi
if [ -z "$sku_id" ]; then
    echo "Error: Unable to determine SKU ID. Using default value (SKU ID: 0)."
    sku_id=0
fi

# Logical checks and actions
if { [ -z "$product_name" ] || [ "$product_name" = "ES-1227-36TS-B-250" ]; } && [ "$sku_id" -eq 0 ]; then
    echo "Configuring power banks for SKU0 (PSU 250W active, Product: $product_name)"
    poetool mgmt set_power_banks 1 150 585 480 0xa
    poetool mgmt set_power_banks 2 150 585 480 0xa
    poetool mgmt set_power_banks 3 150 585 480 0xa

elif [ "$product_name" = "ES-1227-36TS-B-400" ] && [ "$sku_id" -eq 1 ]; then
    echo "Configuring power banks for SKU1 (PSU 400W active, Product: $product_name)"
    poetool mgmt set_power_banks 1 300 585 480 0xa
    poetool mgmt set_power_banks 2 300 585 480 0xa
    poetool mgmt set_power_banks 3 300 585 480 0xa

else
    echo "Error: Unknown SKU ID, using default values"
    poetool mgmt set_power_banks 1 150 585 480 0xa
    poetool mgmt set_power_banks 2 150 585 480 0xa
    poetool mgmt set_power_banks 3 150 585 480 0xa
fi

# Program global matrix
#poe_main 0x13
# Save system settings
#poe_main 0x15

# PoE power bank setting
#poe_power.sh#
# reload PoE configuration
#poe_cfg_init.py