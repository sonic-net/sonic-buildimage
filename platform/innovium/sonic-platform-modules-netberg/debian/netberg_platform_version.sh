#! /bin/sh
# driver version

PLATFROM=$1
PLATFROM_PATH=$2

export PLATFORM_DRIVER_VER='"0.3.16"'

echo "PLATFORM_DRIVER_VER = $PLATFORM_DRIVER_VER"

util_name=$(echo $PLATFROM)

j2 $PLATFROM_PATH/templates/netberg_${util_name}_util.py.j2 -o $PLATFROM_PATH/utils/netberg_${util_name}_util.py
chmod 775 $PLATFROM_PATH/utils/netberg_${util_name}_util.py
