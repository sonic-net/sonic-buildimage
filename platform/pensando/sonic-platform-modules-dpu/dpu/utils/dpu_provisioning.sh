#!/bin/bash

device="/usr/share/sonic/device"
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
        echo "python3 -m pip install $device/$platform/sonic_platform-1.0-py3-none-any.whl"
        python3 -m pip install $device/$platform/sonic_platform-1.0-py3-none-any.whl
        echo "cp /usr/share/sonic/device/$platform/config_db.json /etc/sonic/config_db.json"
        cp /usr/share/sonic/device/$platform/config_db.json /etc/sonic/config_db.json

        jq_command=$(cat <<EOF
        jq --arg val "$val" '
        .INTERFACE |= with_entries(
            if .key | test("Ethernet0\\\\|18\\\\.\\\\d+\\\\.202\\\\.1/31") then
                .key = (.key | gsub("18\\\\.\\\\d+\\\\.202\\\\.1"; "18.\($val).202.1"))
            else
                .
            end
        )' /etc/sonic/config_db.json > /etc/sonic/config_db.json.tmp && mv /etc/sonic/config_db.json.tmp /etc/sonic/config_db.json
EOF
        )

        echo "$jq_command"
        eval "$jq_command"

        # Update platform_components.json dynamically
        platform_components="/usr/share/sonic/device/$platform/platform_components.json"
        if [ -f "$platform_components" ]; then
            jq --arg val "$val" '
            .chassis |= with_entries(
                .value.component |= with_entries(
                    if .key | test("^DPU.*-0$") then
                        .key = (.key | gsub("-0$"; "-\($val)"))
                    else
                        .
                    end
                )
            )' "$platform_components" > "${platform_components}.tmp" && mv "${platform_components}.tmp" "$platform_components"
            echo "Updated platform_components.json with value: $val"
        else
            echo "platform_components.json not found, skipping update."
        fi
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

mkdir -p /host/images
chmod +x /boot/install_file

sleep 5
INTERFACE="eth0-midplane"
if ip link show "$INTERFACE" &> /dev/null; then
    echo "dhclient $INTERFACE"
    dhclient $INTERFACE
fi
