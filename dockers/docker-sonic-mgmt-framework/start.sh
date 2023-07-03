#!/usr/bin/env bash

mkdir -p /var/sonic
echo "# Config files managed by sonic-config-engine" > /var/sonic/config_status

TZ=$(cat /etc/timezone)
rm -rf /etc/localtime
ln -sf /usr/share/zoneinfo/$TZ /etc/localtime
