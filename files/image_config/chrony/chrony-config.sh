#!/bin/bash

hwclock --show &> /dev/null
if [ $? -ne 0 ]; then
    echo "hwclock --show failed, attempting hwclock --systohc..."
    hwclock --systohc
fi

# Detect whether this platform is a switch BMC (declared via switch_bmc=1 in the
# platform's platform_env.conf). Used to gate BMC-specific chrony behavior such as
# stepping the clock and falling back to public NTP servers.
is_switch_bmc=$(python3 -c 'from sonic_py_common import device_info; print(int(device_info.is_switch_bmc()))' 2>/dev/null || echo 0)

sonic-cfggen -d -a "{\"is_switch_bmc\": $is_switch_bmc}" -t /usr/share/sonic/templates/chrony.conf.j2 >/etc/chrony/chrony.conf
sonic-cfggen -d -t /usr/share/sonic/templates/chrony.keys.j2 >/etc/chrony/chrony.keys
chmod o-r /etc/chrony/chrony.keys
