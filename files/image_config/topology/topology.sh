#!/bin/bash
# This script is invoked by topology.service only
# for multi-asic virtual platform. For multi-asic platform
# multiple Database instances are present
# and HWKSU information is retrieved from first database instance.
#

get_hwsku() {
    # Get HWSKU from config_db. If HWSKU is not available in config_db
    # get HWSKU from minigraph.xml if minigraph file exists.
    HWSKU=`sonic-cfggen -d -v 'DEVICE_METADATA["localhost"]["hwsku"]' 2>&1`
    if [[ $? -ne 0 || $HWSKU == "" ]]; then
            if [[ -f "/etc/sonic/minigraph.xml" ]]; then
                HWSKU=`sonic-cfggen -m /etc/sonic/minigraph.xml -v "DEVICE_METADATA['localhost']['hwsku']" 2>&1`
                if [[ $? -ne 0 || $HWSKU == "" ]]; then
                    HWSKU=""
                fi
            else
                HWSKU=""
            fi
    fi
    echo "${HWSKU}"
}

bind_ports_to_ns() {
    PLATFORM="$1"
    HWSKU="$2"
    BIND="$3"

    # Check if the directory exists
    if [[ ! -d "/usr/share/sonic/device/$PLATFORM/$HWSKU" ]]; then
        echo "Directory /usr/share/sonic/device/$PLATFORM/$HWSKU does not exist"
        exit 1
    fi

    # Read NUM_ASIC from asic.conf file
    asic_conf="/usr/share/sonic/device/$PLATFORM/asic.conf"
    if [[ ! -f "$asic_conf" ]]; then
        echo "Error: $asic_conf file not found"
        exit 1
    fi

    # Read NUM_ASIC from asic.conf file
    num_asic=$(awk -F "=" '/^NUM_ASIC=/ {print $2}' "$asic_conf")
    if [[ -z $num_asic ]]; then
        echo "NUM_ASIC not found in $asic_conf"
        exit 1
    fi

    for ((asic_num = 0; asic_num < num_asic; asic_num++)); do
        echo "Processing $PLATFORM/$HWSKU/$asic_num"
        asic_dir="/usr/share/sonic/device/$PLATFORM/$HWSKU/$asic_num"

        # Check if the directory exists for the ASIC number
        if [[ ! -d "$asic_dir" ]]; then
            echo "Directory $asic_dir does not exist"
            exit 1
        fi

        lanemap_ini="$asic_dir/lanemap.ini"

        if [[ ! -f "$lanemap_ini" ]]; then
            echo "lanemap.ini not found in $asic_dir"
            exit 1
        fi

        # Loop through each line of lanemap.ini
        while IFS= read -r line; do
            # Extract interface before ":"
            intf=$(echo "$line" | cut -d ":" -f 1)
            if [[ $BIND == true ]]; then
                echo "Moving interface $intf to asic$asic_num"
                if [[ $intf == "Cpu0" ]]; then
                    # Extract the numeric part of the interface name
                    num="${prev#eth}"
                    # Increment the numeric part
                    ((num++))
                    # Construct the new interface name
                    cur="eth$num"
                    echo "Renaming $cur to $intf"
                    ip link sev dev $cur down
                    ip link set dev $cur name $intf
                fi
                ip link set dev $intf netns asic$asic_num
                sudo ip netns exec asic$asic_num ip link set dev $intf mtu 9100
                sudo ip netns exec asic$asic_num ip link set $intf up
            else
                echo "Moving interface $intf back to default ns"
                sudo ip netns exec asic$asic_num ip link set dev $intf netns 1
                if [[ $intf == "Cpu0" ]]; then
                    # Extract the numeric part of the interface name
                    num="${prev#eth}"
                    # Increment the numeric part
                    ((num++))
                    # Construct the new interface name
                    cur="eth$num"
                    echo "Renaming $intf to $cur"
                    ip link set dev $intf down
                    ip link set dev $intf name $cur
                    ip link set dev $cur up
                fi
            fi
            prev=$intf
            done < "$lanemap_ini"
    done
    exit 0  # Exit script with success
}


start() {
    TOPOLOGY_SCRIPT="topology.sh"
    PLATFORM=`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`
    HWSKU=`get_hwsku`
    if [[ $HWSKU == "msft_"* ]]; then
        /usr/share/sonic/device/$PLATFORM/$HWSKU/$TOPOLOGY_SCRIPT start
    elif [[ $HWSKU != "" ]]; then
        bind_ports_to_ns "$PLATFORM" "$HWSKU" true
    else
        echo "Failed to get HWSKU"
        exit 1
    fi
}

stop() {
    TOPOLOGY_SCRIPT="topology.sh"
    PLATFORM=`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`
    HWSKU=`get_hwsku`
    if [[ $HWSKU == "msft_"* ]]; then
        /usr/share/sonic/device/$PLATFORM/$HWSKU/$TOPOLOGY_SCRIPT stop
    elif [[ $HWSKU != "" ]]; then
        bind_ports_to_ns "$PLATFORM" "$HWSKU" false
    else
        echo "Failed to get HWSKU"
        exit 1
    fi
}

# read SONiC immutable variables
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment

case "$1" in
    start|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        ;;
esac
