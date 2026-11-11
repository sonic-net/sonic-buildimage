#!/bin/bash
#
# This script is used to print, list and set the hwskus under the same platform.
# Regular user can print and list the current hwsku and all available hwskus.
# To change the hwsku, you will need root access. The main idea of the script
# was that each hwsku reserves it's own portmaping, minigraph, json etc. When we change the hwsku,
# the script will pickup the right configuration files and reboot the box to take effect
# (script will ask for reboot confirmation). This will override the existing configuration.
# The following command line help should explain the syntax
#
display_help() {
    echo "Usage: $0 [-h] [-p] [-l] [-s HWSKU]" >&2
    echo
    echo "   -h, --help           Print this usage page"
    echo "   -p, --print          Print the current HWSKU"
    echo "   -l, --list           List the available HWSKUs"
    echo "   -s, --set            Set the HWSKU"
    exit 0
}

HWSKU_ROOT='/usr/share/sonic/device/'
onie_platform=`grep onie_platform /host/machine.conf`
platform=${onie_platform#*=}
printf -v target_hwsku "%s%s%s%s" $HWSKU_ROOT $platform '/' "default_sku"
default_sku=`cat ${target_hwsku}`
sku_array=($default_sku)
current_hwsku=${sku_array[0]}
RUNNING_PATH='/etc/sonic/'

config_hwsku_exec_1803() {
    config_hwsku=$1
    while true; do
        read -p "This will reset to the initial configuration with the port mode specified and restart all services. [Y/N] " yn
        case $yn in
            [Yy]* ) break;;
            [Nn]* ) echo "No changes were made"; exit;;
            * ) echo "Please input yes or no.";;
        esac
    done

    printf -v hwsku_minigraph "%s%s%s%s%s" $HWSKU_ROOT $platform '/' $config_hwsku '/' "minigraph.xml"
    printf -v target_minigraph "%s%s%s%s" $HWSKU_ROOT $platform '/' "minigraph.xml"
    running_minigraph=$RUNNING_PATH
    running_minigraph+="minigraph.xml"
    # default_sku
    printf -v hwsku "%s%s%s%s%s%s" $HWSKU_ROOT $platform '/' $config_hwsku '/' "default_sku"
    printf -v target_hwsku "%s%s%s%s" $HWSKU_ROOT $platform '/' "default_sku"

    if [ ! -e $hwsku_minigraph ]; then echo "Error: $hwsku_minigraph: No such file"; exit 1; fi
    if [ ! -e $hwsku ]; then echo "Error: $hwsku: No such file"; exit 1; fi

    #echo copy $hwsku_minigraph " to "  $target_minigraph
    cp $hwsku_minigraph $target_minigraph
    cp $hwsku_minigraph $running_minigraph
    #echo copy $hwsku " to "  $target_hwsku
    cp $hwsku $target_hwsku

    #load minigraph
    echo "Loading the new configuration"
    config load_minigraph -y

    # save the json after backup
    echo "Backup the old config_db.json to .bak file and save the new one."
    target_json=$RUNNING_PATH
    target_json+="config_db.json"
    backup_json=$target_json".bak"
    #echo copy $target_json to $backup_json
    cp $target_json $backup_json

    config save -y
    sync
    echo "The configuration has been updated. Please reboot the system!"
}

config_hwsku_exec() {
    config_hwsku=$1
    while true; do
        read -p "This will reset to the initial configuration with the port mode specified. [Y/N] " yn
        case $yn in
            [Yy]* ) break;;
            [Nn]* ) echo "No changes were made"; exit;;
            * ) echo "Please input yes or no.";;
        esac
    done

    # default_sku
    printf -v hwsku "%s%s%s%s%s%s" $HWSKU_ROOT $platform '/' $config_hwsku '/' "default_sku"
    printf -v target_hwsku "%s%s%s%s" $HWSKU_ROOT $platform '/' "default_sku"

    #echo copy $hwsku " to "  $target_hwsku
    if [ ! -e $hwsku ]; then echo "Error: $hwsku: No such file"; exit 1; fi
    cp $hwsku $target_hwsku

    # save the json after backup
    echo "Backup the old config_db.json to .bak file and save the new one."
    target_json=$RUNNING_PATH
    target_json+="config_db.json"
    backup_json=$target_json".bak"
    #echo copy $target_json to $backup_json
    cp $target_json $backup_json

    #load default_sku  
    CONFIG_DB_INDEX=4
    cursor=0
    Ethernet="*Ethernet*"
    echo "Loading the new configuration"
    PLATFORM=`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`
    PRESET=(`head -n 1 /usr/share/sonic/device/$PLATFORM/default_sku`)
    sonic-cfggen -H -k ${PRESET[0]} --preset ${PRESET[1]} > /etc/sonic/config_db.json
    while true; do
        result=$(redis-cli -n $CONFIG_DB_INDEX SCAN $cursor MATCH $Ethernet )
        cursor=$(echo "$result" | head -n 1)

        keys=$(echo "$result" | tail -n +2)

        if [ ! -z "$keys" ]; then
            redis-cli -n $CONFIG_DB_INDEX DEL $keys >/dev/null 2>&1
        fi

        if [ "$cursor" == "0" ]; then
            break
        fi
    done

    echo "The configuration is being updated, please wait for 20s."
    sleep 20s
    sonic-cfggen -j /etc/sonic/config_db.json --write-to-db
    redis-cli -n $CONFIG_DB_INDEX SET "CONFIG_DB_INITIALIZED" "1"

    config save -y
    sync
    echo "The configuration has been updated. Please reboot the system!"
}

check_supported_hwskus() {
    # Check the available HWSKUs
    platform_dir="$HWSKU_ROOT""$platform"/
    supported_hwskus=`ls -l $platform_dir | egrep '^d' | awk '{print $9}' | grep -v plugins | grep -v led-code | grep -v cancun | grep -v pycache`
}
config_hwsku_fun() {

    config_hwsku=$1
    # Check root privileges
    if [ "$EUID" -ne 0 ]
    then
      echo "Please run as root"
      exit
    fi

    if [ "$config_hwsku" == "$current_hwsku" ]; then
        echo "The input HWSKU configuration is running, no changes were made"
        exit
    fi


    # Check the available HWSKUs
    check_supported_hwskus

    sonic_version=`grep build_version /etc/sonic/sonic_version.yml`
    array=(${sonic_version//./ })
    branch=`echo ${array[1]} | sed $'s/\'//g'`
    for hwsku in $supported_hwskus
    do
    if [ $config_hwsku == $hwsku ]; then
        if [[ $branch =~ "201803" ]]; then
            config_hwsku_exec_1803 "$config_hwsku"
        else
            config_hwsku_exec "$config_hwsku"
        fi
    exit
    fi
    done

    # Not matching any HWSKU names, print error
    echo "Please use one of the options:"
    echo -e "$supported_hwskus"
    exit 1
}

main() {
    case "$1" in
    -h | --help)
        display_help
        ;;
    -p | --print)
        echo $current_hwsku
        ;;
    -l | --list)
        check_supported_hwskus
        echo -e "$supported_hwskus"
        ;;
    -s | --set)
        config_hwsku=$2
        config_hwsku_fun "$config_hwsku"
        ;;
    *)
        echo "Please use options as following:" >&2
        display_help
        ;;
    esac
}

main "$@"
