#!/bin/bash

. /usr/local/bin/syncd_common.sh

function remove_ethernet_interfaces() {
    debug "remove_ethernet_interfaces: Removing all Ethernet interfaces (NET_NS: $NET_NS)..."

    local start_time=$(date +%s)
    local ethernet_interfaces
    if [[ -n "$NET_NS" ]]; then
        ethernet_interfaces=$(ip netns exec "$NET_NS" ip link show | grep -o 'Ethernet[0-9]*' | sort -u)
    else
        ethernet_interfaces=$(ip link show | grep -o 'Ethernet[0-9]*' | sort -u)
    fi

    if [[ -n "$ethernet_interfaces" ]]; then
        local interface_count=$(echo "$ethernet_interfaces" | wc -w)
        debug "remove_ethernet_interfaces: Found $interface_count Ethernet interfaces to remove"

        for interface in $ethernet_interfaces; do
            if [[ -n "$NET_NS" ]]; then
                ip netns exec "$NET_NS" ip link del "$interface" 2>/dev/null || debug "Failed to remove interface: $interface"
            else
                ip link del "$interface" 2>/dev/null || debug "Failed to remove interface: $interface"
            fi
        done
        debug "remove_ethernet_interfaces: Finished removing $interface_count Ethernet interfaces"
    else
        debug "remove_ethernet_interfaces: No Ethernet interfaces found to remove"
    fi

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    debug "remove_ethernet_interfaces: Execution time: ${duration}s (NET_NS: $NET_NS)"
}

function reset_mellanox_drivers() {
    local mlx_dev="/proc/mlx_sx/sx_core"
    if [[ $DEV != "" ]]; then
        mlx_dev="/proc/mlx_sx/asic$DEV/sx_core"
    fi

    if [[ -f /var/run/mlx_sx_core_restart_required$DEV ]]; then
        debug "Restarting Mellanox drivers for ASIC $DEV"
        echo "pcidrv_restart" > $mlx_dev
        rm -f /var/run/mlx_sx_core_restart_required$DEV 2>/dev/null || debug "Warning: Could not remove restart flag"
        debug "Mellanox drivers restarted for ASIC $DEV"
    fi

    remove_ethernet_interfaces
}

function startplatform() {

    # platform specific tasks

    if [[ x"$sonic_asic_platform" == x"mellanox" ]]; then
        BOOT_TYPE=`getBootType`
        if [[ x"$WARM_BOOT" == x"true" || x"$BOOT_TYPE" == x"fast" ]]; then
            export FAST_BOOT=1
        fi

        # Clear container's temporary directory before starting
        rm -rf /tmp/nv-syncd-shared/$DEV/* 2>/dev/null

        local fw_upgrade_args="--status=all"
        if [[ $DEV != "" ]]; then
            fw_upgrade_args="--status=$DEV"
        fi
        mlnx-fw-manager $fw_upgrade_args
        if [[ $? != "0" ]]; then
            debug "ASIC$DEV firmware upgrade status check failed. Please check the firmware upgrade status manually."
            exit 1
        fi

        reset_mellanox_drivers

        if ! echo "mlx_sx_core_restart_required" > /var/run/mlx_sx_core_restart_required$DEV 2>/dev/null; then
            debug "Warning: Could not create restart flag file"
        fi
    fi

    if [[ x"$sonic_asic_platform" == x"broadcom" ]]; then
        if [[ x"$WARM_BOOT" != x"true" ]]; then
            . /host/machine.conf
            if [ -n "$aboot_platform" ]; then
                platform=$aboot_platform
            elif [ -n "$onie_platform" ]; then
                platform=$onie_platform
            else
                platform="unknown"
            fi
            if [[ x"$platform" =~ x"x86_64-arista_720dt_48s" ]]; then
                is_bcm0=$(ls /sys/class/net | grep bcm0)
                if [[ "$is_bcm0" == "bcm0" ]]; then
                    debug "stop SDK opennsl-modules ..."
                    /etc/init.d/opennsl-modules stop
                    debug "start SDK opennsl-modules ..."
                    /etc/init.d/opennsl-modules start
                    debug "started SDK opennsl-modules"
                fi
            fi
        fi
    fi

    if [[ x"$sonic_asic_platform" == x"barefoot" ]]; then
        is_usb0=$(ls /sys/class/net | grep usb0)
        if [[ "$is_usb0" == "usb0" ]]; then
            /usr/bin/ip link set usb0 down
            /usr/bin/ip link set usb0 up
        fi
    fi

    if [[ x"$sonic_asic_platform" == x"nvidia-bluefield" ]]; then
        /usr/bin/bfnet.sh start
        if [[ $? != "0" ]]; then
            debug "Failed to start Nvidia Bluefield"
            exit 1
        fi
    fi
}

function waitplatform() {

    BOOT_TYPE=`getBootType`
    if [[ x"$sonic_asic_platform" == x"mellanox" ]]; then
        PLATFORM=`$SONIC_DB_CLI CONFIG_DB hget 'DEVICE_METADATA|localhost' platform`
        PMON_IMMEDIATE_START="/usr/share/sonic/device/$PLATFORM/pmon_immediate_start"
        if [[ x"$BOOT_TYPE" = @(x"fast"|x"warm"|x"fastfast") ]] && [[ ! -f $PMON_IMMEDIATE_START ]]; then
            debug "PMON service is delayed by for better fast/warm boot performance"
        else
            debug "Starting pmon service..."
            /bin/systemctl start pmon
            debug "Started pmon service"
        fi
    fi
}

function stopplatform1() {
    if [[ x$sonic_asic_platform == x"mellanox" ]]; then
        echo "health_check_trigger del_dev 1" > /proc/mlx_sx/sx_core
    fi

    if [[ x$sonic_asic_platform != x"mellanox" ]] || [[ x$TYPE != x"cold" ]]; then
        # Invoke platform specific pre shutdown routine.
        PLATFORM=`$SONIC_DB_CLI CONFIG_DB hget 'DEVICE_METADATA|localhost' platform`
        PLATFORM_PRE_SHUTDOWN="/usr/share/sonic/device/$PLATFORM/plugins/syncd_request_pre_shutdown"
        [ -f $PLATFORM_PRE_SHUTDOWN ] && \
            /usr/bin/docker exec -i syncd$DEV /usr/share/sonic/platform/plugins/syncd_request_pre_shutdown --${TYPE}

        debug "${TYPE} shutdown syncd process ..."
        /usr/bin/docker exec -i syncd$DEV /usr/bin/syncd_request_shutdown --${TYPE}

        # wait until syncd quits gracefully or force syncd to exit after
        # waiting for 20 seconds
        start_in_secs=${SECONDS}
        end_in_secs=${SECONDS}
        timer_threshold=20
        while docker top syncd$DEV | grep -q /usr/bin/syncd \
                && [[ $((end_in_secs - start_in_secs)) -le $timer_threshold ]]; do
            sleep 0.1
            end_in_secs=${SECONDS}
        done

        if [[ $((end_in_secs - start_in_secs)) -gt $timer_threshold ]]; then
            debug "syncd process in container syncd$DEV did not exit gracefully"
        fi

        /usr/bin/docker exec -i syncd$DEV /bin/sync
        debug "Finished ${TYPE} shutdown syncd process ..."
    fi
}

function stopplatform2() {
    # platform specific tasks

     if [[ x"$WARM_BOOT" != x"true" ]]; then
         if [ x"$sonic_asic_platform" == x"nvidia-bluefield" ]; then
             /usr/bin/bfnet.sh stop
         fi
     fi
}

OP=$1
DEV=$2

SERVICE="syncd"
PEER="swss"
DEBUGLOG="/tmp/swss-syncd-debug$DEV.log"
LOCKFILE="/tmp/swss-syncd-lock$DEV"
NAMESPACE_PREFIX="asic"
if [ "$DEV" ]; then
    NET_NS="$NAMESPACE_PREFIX$DEV" #name of the network namespace
    SONIC_DB_CLI="sonic-db-cli -n $NET_NS"
else
    NET_NS=""
    SONIC_DB_CLI="sonic-db-cli"
fi

case "$1" in
    start|wait|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|wait|stop}"
        exit 1
        ;;
esac
