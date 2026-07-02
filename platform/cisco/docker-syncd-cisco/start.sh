#!/usr/bin/env bash

PLATFORM_DIR=/usr/share/sonic/platform
HWSKU_DIR=/usr/share/sonic/hwsku

SYNCD_SOCKET_FILE=/var/run/sswsyncd/sswsyncd.socket

# Remove stale files if they exist
rm -f /var/run/rsyslogd.pid
rm -f ${SYNCD_SOCKET_FILE}

supervisorctl start rsyslogd

# Remove dshell_client.conf on platforms that do not support dshell_client
if [ -f /etc/init.d/sxdkernel ]; then
    rm -f /etc/supervisor/conf.d/dshell_client.conf
    supervisorctl reread
    supervisorctl update
    echo "dshell_client and puntpkthandler disabled for SX driver"
fi

mkdir -p /etc/sai.d/

# Create/Copy the sai.profile to /etc/sai.d/sai.profile
if [ -f $HWSKU_DIR/sai.profile.j2 ]; then
    sonic-cfggen -d -t $HWSKU_DIR/sai.profile.j2 > /etc/sai.d/sai.profile
else
    if [ -f $HWSKU_DIR/sai.profile ]; then
        cp $HWSKU_DIR/sai.profile /etc/sai.d/sai.profile
    fi
fi

if grep -q '^model name.: VXR$' /proc/cpuinfo; then
    # if eth4 exists, wait until IPv4 address gets assigned
    if ip link show eth4; then
      for _ in {1..6}; do
        if inet=$(ip addr show eth4 | grep 'inet '); then
           break
        fi
        echo Waiting for eth4 to get configured
        sleep 5
      done
      if [ "$inet" == "" ]; then
        echo "WARNING: eth4 did not get configured"
      fi
    fi
    supervisorctl start vxr_simulator
fi

ln -sf /opt/cisco/syncd/bin/dshell_client.py /usr/bin/dshell_client.py

supervisorctl start syncd

# Flag start of syncd process for logging infra
echo "1">/var/cache/cisco/syncd_started
