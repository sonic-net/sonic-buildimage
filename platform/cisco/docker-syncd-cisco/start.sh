#!/usr/bin/env bash
#
# Copyright (c) 2026 Cisco Systems, Inc. and/or its affiliates.
# SPDX-License-Identifier: Apache-2.0

PLATFORM_DIR=/usr/share/sonic/platform
HWSKU_DIR=/usr/share/sonic/hwsku

CISCO_PLATFORM_CAPABILITIES=${CISCO_PLATFORM_CAPABILITIES-dshell}

cisco_platform_capability_supported()
{
    case ",${CISCO_PLATFORM_CAPABILITIES}," in
        *,"$1",*) return 0 ;;
        *) return 1 ;;
    esac
}

SYNCD_SOCKET_FILE=/var/run/sswsyncd/sswsyncd.socket

# Remove stale files if they exist
rm -f /var/run/rsyslogd.pid
rm -f ${SYNCD_SOCKET_FILE}

supervisorctl start rsyslogd

# Start the optional DShell client only when the platform advertises support.
if cisco_platform_capability_supported dshell; then
    supervisorctl start dshell_client
else
    echo "dshell_client disabled by platform capability"
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
