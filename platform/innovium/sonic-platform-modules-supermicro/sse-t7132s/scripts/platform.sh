#!/bin/bash

i2c_config() {
    local count=0
    local MAX_BUS_RETRY=20
    local MAX_I2C_OP_RETRY=10

    i2c_bus_op=`echo "$@" | cut -d'>' -f 2`
    i2c_bus=$(dirname $i2c_bus_op)

    # check if bus exists
    while [[ "$count" -lt "$MAX_BUS_RETRY" ]]; do
        [[ -e $i2c_bus ]] && break || sleep .1
        count=$((count+1))
    done

    if [[ "$count" -eq "$MAX_BUS_RETRY" ]]; then
        echo "dell_i2c_utils : ERROR: $@ : i2c bus not created"
        return
    fi

    # perform the add/delete
    count=0
    while [[ "$count" -lt "$MAX_I2C_OP_RETRY" ]]; do
        eval "$@" > /dev/null 2>&1
        [[ $? == 0 ]] && break || sleep .2
        count=$((count+1))
    done

    if [[ "$count" -eq "$MAX_I2C_OP_RETRY" ]]; then
        echo "dell_i2c_utils : ERROR: $@ : i2c operation failed"
        return
    fi
}

# Attach/Detach syseeprom on CPU board
sys_eeprom() {
    case $1 in
        "new_device")    i2c_config "echo 24c02 0x53 > /sys/bus/i2c/devices/i2c-0/$1"
                         ;;
        "delete_device") i2c_config "echo 0x53 > /sys/bus/i2c/devices/i2c-0/$1"
                         ;;
        *)               echo "platform: sys_eeprom : invalid command !"
                         ;;
    esac
}

install_python_api_package() {
    device="/usr/share/sonic/device"
    platform=$(/usr/local/bin/sonic-cfggen -H -v DEVICE_METADATA.localhost.platform)

    rv=$(pip3 install $device/$platform/sonic_platform-1.0-py3-none-any.whl)
}

remove_python_api_package() {
    rv=$(pip3 show sonic-platform > /dev/null 2>/dev/null)
    if [ $? -eq 0 ]; then
        rv=$(pip3 uninstall -y sonic-platform > /dev/null 2>/dev/null)
    fi
}
update_share_password() {
    echo "Update shared password !!!"
    SONIC_VERSION=$(cat /etc/sonic/sonic_version.yml | grep "build_version" | sed -e "s/build_version: //g;s/'//g")
    image_dir=$(cat /proc/cmdline | sed -e 's/.*loop=\(\S*\)\/.*/\1/')
    host_images=$(find /host -maxdepth 1 -name "*image-*" | sed -e 's/\/host\/image-//')
    if [ -f /host/reboot-cause/platform/last_boot_image ]; then
        last_image_ver=$(cat /host/reboot-cause/platform/last_boot_image)
        echo "The last boot image is ${last_image_ver}"
    else
        last_image_ver=""
        echo "The last boot image tag file is not found."

        # try to detect reset-factory
        # reset-factory deletes all docker containers except "database"
        while read -r var; do
            names=$(find /host/image-${var}/docker/containers -name config.v2.json -exec jq ".Name" {} \;)
            echo "image-${var} docker containers: ${names}"
            if [ "$names" == '"/database"' ]; then
                echo "image-${var} was reset-factory"
                # if already found current image has reset-factory,
                # then keep last_image_ver to be current image,
                # to skip copy password
                if [ "$last_image_ver" != "${SONIC_VERSION}" ]; then
                    last_image_ver=${var}
                fi
            fi
        done <<< "$host_images"

        if [ -z "$last_image_ver" ]; then
            echo "No reset-factory is detected"
        fi
    fi

    echo "last_image_ver = ${last_image_ver}"
    COPIED=""
    if [ -n "$last_image_ver" ]; then
        if [ "$last_image_ver" == "${SONIC_VERSION}" ]; then
            echo "No need to copy password to the same firmware"
        else
            while read -r var; do
                #echo "var=${var} image_dir=${image_dir}"
                if [ "image-${var}" != "$image_dir" ]; then
                    cp /host/image-${var}/rw/etc/shadow /host/${image_dir}/rw/etc/shadow
                    cp /host/image-${var}/rw/etc/passwd /host/${image_dir}/rw/etc/passwd
                    cp /host/image-${var}/rw/etc/gshadow /host/${image_dir}/rw/etc/gshadow
                    cp /host/image-${var}/rw/etc/group /host/${image_dir}/rw/etc/group
                    echo "Copied password from image-${var} to ${image_dir}"
                    COPIED="image-${var}"
                fi
            done <<< "$host_images"
        fi
    fi
    echo "COPIED = ${COPIED}"
    if [ -z "$COPIED" ]; then
        echo "No password is copied"
    fi

    if [ ! -d /host/reboot-cause/platform ]; then
        mkdir -p /host/reboot-cause/platform
    fi
    echo "${SONIC_VERSION}" | sudo tee /host/reboot-cause/platform/last_boot_image > /dev/null
}


if [ "$1" == "init" ]; then
    echo "Initializing hardware components ..."
    depmod -a
    sys_eeprom "new_device"
    modprobe t7132s
    install_python_api_package
    update_share_password
    /usr/bin/python3 /usr/local/bin/delay_start_services.py &
elif [ "$1" == "deinit" ]; then
    echo "De-initializing hardware components ..."
    modprobe -r t7132s
    sys_eeprom "delete_device"
    remove_python_api_package
else
    echo "Invalid options !"
fi
