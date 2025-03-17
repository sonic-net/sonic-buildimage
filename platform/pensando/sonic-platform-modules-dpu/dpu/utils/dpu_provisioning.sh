#!/bin/bash

device="/usr/share/sonic/device"
platform=$(grep 'onie_platform=' /host/machine.conf | cut -d '=' -f 2)
pipeline=`cat /usr/share/sonic/device/$platform/default_pipeline`
docker_name=$pipeline
if [ "$pipeline" == "rudra" ]; then
    docker_name="dpu"
fi

iteration=10
val=""
echo "DPU Provisioning log" > /boot/first_boot.log
for i in $(seq $iteration); do
    hex_val=$(docker exec -i $docker_name cpldapp -r 0xA | tr -d '\r')
    echo "Iteration : $i" >> /boot/first_boot.log
    if [[ -n "$hex_val" ]]; then
        val=$((hex_val))
        echo "$hex_val, $val" >> /boot/first_boot.log
        break
    fi
    sleep 1
    if [[ $i -eq $iteration ]]; then
        echo "Command failed after $iteration attempts" >> /boot/first_boot.log
        exit 1
    fi
done

echo "dpu provisioning for dpu $val"

if [ -f /boot/first_boot ]; then
    if [ "$platform" == "arm64-elba-asic-flash128-r0" ]; then
        cp /usr/share/sonic/device/$platform/init_cfg.json /etc/sonic/platform_init.json
        cp /usr/share/sonic/device/$platform/minigraph.xml /etc/sonic/minigraph.xml
        sonic-cfggen -H -m /etc/sonic/minigraph.xml --print-data > /etc/sonic/minigraph.json
        sonic-cfggen -j /etc/sonic/minigraph.json -j /etc/sonic/platform_init.json -j /etc/sonic/init_cfg.json --print-data > /etc/sonic/config_db.json

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

        sonic-cfggen -H -j /etc/sonic/config_db.json --write-to-db
        echo "config_db.json written to db"

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
    fi

    echo "File copied successfully."
    rm /boot/first_boot
else
    echo "/boot/first_boot not found. No action taken."
fi

mkdir -p /host/images
mkdir -p /data
chmod +x /boot/install_file
