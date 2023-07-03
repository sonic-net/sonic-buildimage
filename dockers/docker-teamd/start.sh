#!/usr/bin/env bash

rm -f /var/run/teamd/*

mkdir -p /var/warmboot/teamd

TZ=$(cat /etc/timezone)
rm -rf /etc/localtime
ln -sf /usr/share/zoneinfo/$TZ /etc/localtime
