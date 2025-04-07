#!/bin/bash

directories=(
    # When both PSUs are plugged PSU1 is hwmon5 and PSU2 is hwmon4
    # If only one PSU is plugged, it  is hwmon4
    "/sys/bus/i2c/devices/3-0059/hwmon/hwmon5" 
    "/sys/bus/i2c/devices/3-0059/hwmon/hwmon4"
    "/sys/bus/i2c/devices/3-0058/hwmon/hwmon4"
)

# PSU corresponding parameters
VOLTAGE_110V_MIN=90
VOLTAGE_110V_MIN_MW=90000
VOLTAGE_110V_MAX=132
VOLTAGE_220V_MIN=180
VOLTAGE_220V_MAX=264
VOLTAGE_110_1_VALUE=810  # 110V for one bank
VOLTAGE_110_2_VALUE=1810  # 110V for two banks
VOLTAGE_220_1_VALUE=1410  # 220V for one bank
VOLTAGE_220_2_VALUE=2400  # 220V for two banks

# Initialize variables for minimum value and its corresponding parameter
min_value=""
min_param=""

# List of parameters to extract
parameters=("in1_input")

# Read and find the minimum parameter value from each directory
for directory in "${directories[@]}"; do
    # echo "directory $directory"	
    for param in "${parameters[@]}"; do
	# echo "param $param"    
        file_path="$directory/$param"
        if [ -e "$file_path" ]; then
            value=$(<"$file_path")
            
            # Skip iteration if value is less than 110V requirement  
            if [ "$value" -lt "$VOLTAGE_110V_MIN_MW" ]; then
                continue
            fi

            # echo "Found value: $value for parameter: $param in directory: $directory"
            if [ -z "$min_value" ] || [ "$value" -lt "$min_value" ]; then
                # echo "Updating minimum value to: $value for parameter: $param"
                min_value="$value"
                min_param="$param"
            fi
        fi
    done
done


# Decide the boardID
# 3 : EVT
# 1 : DVT
BOARD_ID_EVT=3
BOARD_ID_DVT=1

xboard_id="$BOARD_ID_EVT"
file_path="/sys/bus/i2c/devices/0-0033/board_id"

# Check if the file exists
if [ -f "$file_path" ]; then
    # Read the content of the file and assign it to xboard_id
    xboard_id=$(cat "$file_path")
fi
# echo "xboard_id: $xboard_id"

# Define the variables for 110V/220V
PSU_VSENSE_110V=0
PSU_VSENSE_220V=1

#value_watts=$(awk "BEGIN {print $min_value / 1000}")
value_watts=$(printf "%.0f" $(awk "BEGIN {print $min_value / 1000}"))
vsense_file_path="/sys/bus/i2c/devices/0-0033/psu_vsense"
if [ "$value_watts" -ge "$VOLTAGE_220V_MIN" ] && [ "$value_watts" -le "$VOLTAGE_220V_MAX" ]; then
    # echo "220V"
    if [ "$xboard_id" -eq "$BOARD_ID_EVT" ]; then
        # echo "EVT"
        # For 110v PSUx2
        poetool mgmt set_power_banks 3 2400 570 440 0xa
        # For 110v PSUx1
        poetool mgmt set_power_banks 1 1410 570 440 0xa
        poetool mgmt set_power_banks 2 1410 570 440 0xa
    else
        value_to_write=$PSU_VSENSE_220V
        echo "$value_to_write" > "$vsense_file_path"
        # echo "Value written to $vsense_file_path: $value_to_write"
    fi    
else
    # echo "110V"
    if [ "$xboard_id" -eq "$BOARD_ID_EVT" ]; then
        # echo "EVT"
         # For 110v PSUx2
        poetool mgmt set_power_banks 3 1810 570 440 0xa
        # For 110v PSUx1
        poetool mgmt set_power_banks 1 810 570 440 0xa
        poetool mgmt set_power_banks 2 810 570 440 0xa
    else
        value_to_write=$PSU_VSENSE_110V
        echo "$value_to_write" > "$vsense_file_path"
        # echo "Value written to $vsense_file_path: $value_to_write"
    fi    
       
fi

