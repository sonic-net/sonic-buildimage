#!/bin/bash

pipeline=$(cat /usr/share/sonic/device/arm64-elba-asic-flash128-r0/default_pipeline | tr -d '\r')
hex_val=$(docker exec -i $pipeline cpldapp -r 0xA | tr -d '\r')
val=$((hex_val))
echo "dpu provisioning for dpu $val"

if [ -f /boot/first_boot ]; then
    cp /usr/share/sonic/device/arm64-elba-asic-flash128-r0/config_db.json /etc/sonic/config_db.json
    sed -i "s/18.0.202.1/18.$val.202.1/g" /etc/sonic/config_db.json
    cp /etc/sonic/config_db.json /etc/sonic/init_cfg.json
    echo "File copied successfully."
    rm /boot/first_boot
else
    echo "/boot/first_boot not found. No action taken."
fi
