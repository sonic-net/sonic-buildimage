#!/usr/bin/env bash

set -x

start_bcm()
{
    [ -e /dev/linux-bcm-knet ] || mknod /dev/linux-bcm-knet c 122 0
    [ -e /dev/linux-user-bde ] || mknod /dev/linux-user-bde c 126 0
    [ -e /dev/linux-kernel-bde ] || mknod /dev/linux-kernel-bde c 127 0
}

PLATFORM_DIR=/usr/share/sonic/platform
HWSKU_DIR=/usr/share/sonic/hwsku
# Default use python3 version
SONIC_PLATFORM_API_PYTHON_VERSION=3

mkdir -p /etc/sai.d/

if [ -f $HWSKU_DIR/sai.profile ]; then
   cp $HWSKU_DIR/sai.profile /etc/sai.d/sai.profile
fi

. /usr/bin/syncd_init_common.sh
config_syncd

# If the Python 3 sonic-platform package is not installed, try to install it
python3 -c "import sonic_platform" > /dev/null 2>&1 || pip3 show sonic-platform > /dev/null 2>&1
if [ $? -ne 0 ]; then
    SONIC_PLATFORM_WHEEL="/usr/share/sonic/platform/sonic_platform-1.0-py3-none-any.whl"
    echo "sonic-platform package not installed, attempting to install..."
    if [ -e ${SONIC_PLATFORM_WHEEL} ]; then
       pip3 install ${SONIC_PLATFORM_WHEEL}
       if [ $? -eq 0 ]; then
          echo "Successfully installed ${SONIC_PLATFORM_WHEEL}"
          SONIC_PLATFORM_API_PYTHON_VERSION=3
       else
          echo "Error: Failed to install ${SONIC_PLATFORM_WHEEL}"
       fi
    else
       echo "Error: Unable to locate ${SONIC_PLATFORM_WHEEL}"
    fi
else
    SONIC_PLATFORM_API_PYTHON_VERSION=3
fi

start_bcm

exec /usr/local/bin/supervisord


