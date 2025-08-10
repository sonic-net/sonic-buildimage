#! /bin/sh
# driver version

PLATFORM=$1
PLATFORM_PATH=$2

export PLATFORM_DRIVER_VER='"1.1.0"'

echo "PLATFORM_DRIVER_VER = $PLATFORM_DRIVER_VER"

util_name=$(echo $PLATFORM | cut -d "-" -f 1)

j2 $PLATFORM_PATH/templates/cameo_${util_name}_util.py.j2 -o $PLATFORM_PATH/utils/cameo_${util_name}_util.py
chmod 775 $PLATFORM_PATH/utils/cameo_${util_name}_util.py
