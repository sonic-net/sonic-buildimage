#!/bin/bash

hwclock --show &> /dev/null
if [ $? -ne 0 ]; then
    echo "hwclock --show failed, attempting hwclock --systohc..."
    hwclock --systohc
fi

sonic-cfggen -d -t /usr/share/sonic/templates/chrony.conf.j2 >/etc/chrony/chrony.conf
sonic-cfggen -d -t /usr/share/sonic/templates/chrony.keys.j2 >/etc/chrony/chrony.keys
chmod o-r /etc/chrony/chrony.keys
