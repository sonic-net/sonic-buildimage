#!/bin/bash

hwclock --show &> /dev/null
if [ $? -ne 0 ]; then
    echo "hwclock --show failed, attempting hwclock --systohc..."
    hwclock --systohc
fi

PLATFORM_ENV_CONF=/usr/share/sonic/platform/platform_env.conf
switch_bmc=0
if [[ -f "$PLATFORM_ENV_CONF" ]] && grep -q '^switch_bmc=1$' "$PLATFORM_ENV_CONF"; then
    switch_bmc=1
fi

sonic-cfggen -d \
    -a "{\"switch_bmc\": ${switch_bmc}}" \
    -t /usr/share/sonic/templates/chrony.conf.j2 >/etc/chrony/chrony.conf
sonic-cfggen -d -t /usr/share/sonic/templates/chrony.keys.j2 >/etc/chrony/chrony.keys
chmod o-r /etc/chrony/chrony.keys
