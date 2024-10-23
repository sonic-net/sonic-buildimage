#!/bin/bash
platform=$(grep 'onie_platform=' /host/machine.conf | cut -d '=' -f 2)
pipeline=`cat /usr/share/sonic/device/$platform/default_pipeline`
docker_name=$pipeline
if [ "$pipeline" == "rudra" ]; then
    docker_name="dpu"
fi
hex_val=$(docker exec -i $docker_name cpldapp -r 0xA | tr -d '\r')
val=$((hex_val))
echo "dpu provisioning for dpu $val"

if [ -f /boot/first_boot ]; then
    if [ "$platform" == "arm64-elba-asic-flash128-r0" ]; then
        echo "cp /usr/share/sonic/device/$platform/config_db.json /etc/sonic/config_db.json"
        cp /usr/share/sonic/device/$platform/config_db.json /etc/sonic/config_db.json
        echo 'sed -i "s/18.0.202.1/18.$val.202.1/g" /etc/sonic/config_db.json'
        sed -i "s/18.0.202.1/18.$val.202.1/g" /etc/sonic/config_db.json
    else
        echo "cp /usr/share/sonic/device/$platform/config_db_$pipeline.json /etc/sonic/config_db.json"
        cp /usr/share/sonic/device/$platform/config_db_$pipeline.json /etc/sonic/config_db.json
    fi
    echo "cp /etc/sonic/config_db.json /etc/sonic/init_cfg.json"
    cp /etc/sonic/config_db.json /etc/sonic/init_cfg.json
    echo "File copied successfully."
    rm /boot/first_boot
else
    echo "/boot/first_boot not found. No action taken."
fi

INTERFACE="eth0-midplane"
if ip link show "$INTERFACE" &> /dev/null; then
    echo "dhclient $INTERFACE"
    dhclient $INTERFACE
fi
