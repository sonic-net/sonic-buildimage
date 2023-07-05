#!/usr/bin/env bash

rm -f /var/run/nat/*

mkdir -p /var/warmboot/nat

TZ=$(cat /etc/timezone)
rm -rf /etc/localtime
ln -sf /usr/share/zoneinfo/$TZ /etc/localtime
