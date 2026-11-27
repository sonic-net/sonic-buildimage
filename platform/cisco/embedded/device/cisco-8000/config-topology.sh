#!/bin/bash
# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# This script is invoked via /usr/bin/config-topology.sh by config-topology.service.
# This is platform plugin script which could be used for platform specific topology configuration
#
set -x

# List of LC services to be enabled and started on modular chassis linecards
# Services are sourced from /opt/cisco/etc/services directory
LC_SERVICES=(
    "platform-lc-rp-cmd-client.service"
)

# Service directories
SERVICE_SRC_DIR="/opt/cisco/etc/services"
SERVICE_DEST_DIR="/lib/systemd/system"

publish_inventory() {
    # Publish inventory data to redis server
    if [ -f /var/cache/cisco/bootstrap.yaml ] && \
           [ -f /opt/cisco/etc/cisco.bin.inventory/logging.yaml ]; then
        /usr/bin/python3 -m cisco.bin.inventory publish
    fi
}

generate_platform_chassis_config(){
    if [ -f /opt/cisco/etc/chassis_modular ]; then
        /usr/bin/python3 /opt/cisco/tools/bin/platform-chassis-config.py
    fi
}

check_is_lc() {
    # Check if current card is a Linecard (LC) using xr_sysctl
    # Returns 0 if LC, 1 otherwise
    /opt/cisco/bin/xr_sysctl -n bootstrap.card_type 2>/dev/null | grep -q "^LC"
}

enable_and_start_lc_services() {
    # Enable and start LC services on linecard during first boot
    # This function only runs on LC cards (non-modular won't have LC)
    # Once enabled, services start automatically on subsequent reboots via systemd
    if ! check_is_lc; then
        return 0
    fi

    echo "Configuring LC services on linecard..."

    for service_name in "${LC_SERVICES[@]}"; do
        local service_src_path="${SERVICE_SRC_DIR}/${service_name}"
        local service_dest_path="${SERVICE_DEST_DIR}/${service_name}"

        # Check if service file exists in /opt/cisco/etc/services
        if [ ! -f "$service_src_path" ]; then
            echo "Warning: Service file not found: $service_src_path"
            continue
        fi

        # Copy service file to /lib/systemd/system if not already there
        if [ ! -f "$service_dest_path" ]; then
            echo "Copying service file to systemd: $service_name"
            cp "$service_src_path" "$service_dest_path" || echo "Failed to copy $service_name"
        fi

        # Check if service is already enabled (persistent across reboots)
        if systemctl is-enabled "$service_name" &>/dev/null; then
            continue
        else
            # First boot: enable service (persists across reboots and starts automatically via systemd)
            # Service will start after config-setup.service based on unit file dependency
            echo "Enabling service on first boot: $service_name"
            systemctl enable "$service_name" || echo "Failed to enable $service_name"

            # Add service to generated_services.conf
            local services_conf="/etc/sonic/generated_services.conf"
            if ! grep -q "^${service_name}$" "$services_conf" 2>/dev/null; then
                echo "Adding service to generated_services.conf: $service_name"
                echo "$service_name" >> "$services_conf"
            fi

            # Start service immediately
            echo "Starting service: $service_name"
            systemctl start "$service_name" || echo "Failed to start $service_name"
        fi
    done
    if [ "${#LC_SERVICES[@]}" -gt 0 ]; then
            systemctl daemon-reload
    fi

    echo "LC service configuration completed."
}

check_migration_context() {
    local migration_context="/mnt/obfl/.xr2sonic"
    local asic_yaml="/var/cache/cisco/asic.yaml"
    if [ -e "$migration_context" ]; then
        echo "Migration context detected at $migration_context"
        echo "Removing $asic_yaml due to migration context"
        rm -f "$asic_yaml"
    fi
}

setup_pktio_mgr_service() {
    MODULE="cisco_pktio"
    PKTIO_SRV_SRC="/opt/cisco/etc/platform-pktio-mgr.service"
    PKTIO_SRV_DST="/lib/systemd/system/platform-pktio-mgr.service"
    CARD_TYPE=$(/opt/cisco/bin/xr_sysctl -n bootstrap.card_type)
    if [ -f "/opt/cisco/etc/is_chassis_c" ] && [[ $CARD_TYPE == "RP" ]]; then
        if lsmod | grep -q "^$MODULE\b"; then
            echo "Module $MODULE is already loaded."
        else
            echo "Module cisco-pktio is not loaded. Loading..."
            modprobe $MODULE
        fi
        if [ -f $PKTIO_SRV_SRC ] && [ ! -f $PKTIO_SRV_DST ]; then
            # setup pktio mgr service as its not setup
            cp $PKTIO_SRV_SRC $PKTIO_SRV_DST
            systemctl enable platform-pktio-mgr.service
            systemctl start platform-pktio-mgr.service
        fi
    fi
}


start() {
    publish_inventory
    generate_platform_chassis_config
    check_migration_context
    enable_and_start_lc_services
    setup_pktio_mgr_service
}

stop() {
    echo "nothing to do!"
}

case "$1" in
    start|stop)
        $1
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        ;;
esac
