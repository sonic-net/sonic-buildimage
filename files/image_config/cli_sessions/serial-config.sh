#!/bin/bash

# generate conf file for sysrq capabilities.
sonic-cfggen -d -t /usr/share/sonic/templates/sysrq-sysctl.conf.j2 > /etc/sysctl.d/95-sysrq-sysctl.conf

SYSRQ_CONF=0
# update sysrq for current boot.
sysrq_conf=`sonic-db-cli CONFIG_DB HGET "SERIAL_CONSOLE|POLICIES" sysrq_capabilities`
if [ ${sysrq_conf} = "enabled" ]; then
        SYSRQ_CONF=1
fi
sudo echo $SYSRQ_CONF > /proc/sys/kernel/sysrq

# generate env file for profile.d to set auto-logout timeout for serial consoles.
sonic-cfggen -d -t /usr/share/sonic/templates/tmout-env.sh.j2 > /etc/profile.d/tmout-env.sh
