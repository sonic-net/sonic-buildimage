#!/bin/bash
# Micas USB network gadget initialization.

set -euo pipefail

GADGET_NAME="micas_usb_eth"
FUNCTION_TYPE="ncm"
INTERFACE_NAME="bmc0"
VENDOR_ID="0x1d6b"
PRODUCT_ID="0x0104"
SERIAL_NUMBER="0123456789"
MANUFACTURER="Micas"
PRODUCT_NAME="Micas USB Network Device"
DEV_MAC="02:00:00:00:00:01"
HOST_MAC="02:00:00:00:00:02"
PREFERRED_UDC="12060000.usb-vhub:p1"
UDC_WAIT_SECONDS=60
INTERFACE_WAIT_SECONDS=10
BMC_CONFIG_GLOBAL="/etc/sonic/bmc.json"
BMC_CONFIG_PLATFORM="/usr/share/sonic/platform/bmc.json"

log() {
    logger -t micas-usb-network "$*"
}

install_udev_rule() {
    /usr/local/bin/micas-usb-network-udev-init.sh
}

ensure_configfs() {
    if ! mountpoint -q /sys/kernel/config 2>/dev/null; then
        mount -t configfs none /sys/kernel/config
        log "ConfigFS mounted at /sys/kernel/config"
    fi
}

select_udc() {
    local udc_list
    local udc_name
    local wait_idx

    modprobe aspeed_vhub 2>/dev/null || true

    for wait_idx in $(seq 1 "${UDC_WAIT_SECONDS}"); do
        if [ -e "/sys/class/udc/${PREFERRED_UDC}" ]; then
            printf '%s\n' "${PREFERRED_UDC}"
            return 0
        fi

        udc_list=$(ls /sys/class/udc 2>/dev/null || true)
        if [ -n "${udc_list}" ]; then
            udc_name=$(printf '%s\n' "${udc_list}" | head -n1)
            printf '%s\n' "${udc_name}"
            return 0
        fi

        sleep 1
    done

    return 1
}

remove_existing_gadget() {
    local gadget_dir="/sys/kernel/config/usb_gadget/${GADGET_NAME}"

    if [ ! -d "${gadget_dir}" ]; then
        return 0
    fi

    if [ -f "${gadget_dir}/UDC" ]; then
        echo "" > "${gadget_dir}/UDC" 2>/dev/null || true
    fi

    rm -rf "${gadget_dir}" 2>/dev/null || true
}

create_gadget() {
    local gadget_dir="/sys/kernel/config/usb_gadget/${GADGET_NAME}"
    local udc_name="$1"

    ensure_configfs
    remove_existing_gadget

    log "Creating USB gadget configuration..."
    mkdir -p "${gadget_dir}"
    cd "${gadget_dir}"

    echo "${VENDOR_ID}" > idVendor
    echo "${PRODUCT_ID}" > idProduct
    echo "0x0100" > bcdDevice
    echo "0x0200" > bcdUSB

    mkdir -p strings/0x409
    echo "${SERIAL_NUMBER}" > strings/0x409/serialnumber
    echo "${MANUFACTURER}" > strings/0x409/manufacturer
    echo "${PRODUCT_NAME}" > strings/0x409/product

    mkdir -p configs/c.1/strings/0x409
    echo "${FUNCTION_TYPE^^} Network" > configs/c.1/strings/0x409/configuration
    echo "250" > configs/c.1/MaxPower

    mkdir -p "functions/${FUNCTION_TYPE}.${INTERFACE_NAME}"
    echo "${DEV_MAC}" > "functions/${FUNCTION_TYPE}.${INTERFACE_NAME}/dev_addr"
    echo "${HOST_MAC}" > "functions/${FUNCTION_TYPE}.${INTERFACE_NAME}/host_addr"
    ln -s "functions/${FUNCTION_TYPE}.${INTERFACE_NAME}" configs/c.1/

    log "Gadget '${GADGET_NAME}' created with ${FUNCTION_TYPE^^} function"
    log "Using UDC: ${udc_name}"
    echo "${udc_name}" > UDC
}

wait_for_interface() {
    local wait_idx

    for wait_idx in $(seq 1 "${INTERFACE_WAIT_SECONDS}"); do
        if ip link show "${INTERFACE_NAME}" &>/dev/null; then
            log "Interface '${INTERFACE_NAME}' detected"
            return 0
        fi
        sleep 1
    done

    return 1
}

get_bmc_ipv4() {
    local bmc_config=""

    if [ -f "${BMC_CONFIG_GLOBAL}" ]; then
        bmc_config="${BMC_CONFIG_GLOBAL}"
    elif [ -f "${BMC_CONFIG_PLATFORM}" ]; then
        bmc_config="${BMC_CONFIG_PLATFORM}"
    else
        return 1
    fi

    log "Reading BMC configuration from ${bmc_config}"
    python3 - <<PY
import ipaddress
import json
import sys

config_path = "${bmc_config}"
interface_name = "${INTERFACE_NAME}"

try:
    with open(config_path, "r", encoding="utf-8") as fh:
        data = json.load(fh)

    config_if_name = data.get("bmc_if_name", "")
    bmc_addr = data.get("bmc_addr", "")
    bmc_netmask = data.get("bmc_net_mask", "")

    if config_if_name and config_if_name != interface_name:
        sys.exit(1)

    if not bmc_addr or not bmc_netmask:
        sys.exit(1)

    iface = ipaddress.IPv4Interface(f"{bmc_addr}/{bmc_netmask}")
    print(iface.with_prefixlen)
except Exception:
    sys.exit(1)
PY
}

configure_static_ipv4() {
    local bmc_ipv4
    local ipv4_addr

    if ! bmc_ipv4=$(get_bmc_ipv4 2>/dev/null); then
        log "No valid IPv4 configuration found in bmc.json for ${INTERFACE_NAME}"
        return 0
    fi

    ip addr add "${bmc_ipv4}" dev "${INTERFACE_NAME}"
    ipv4_addr=$(ip -4 addr show dev "${INTERFACE_NAME}" | grep -oP 'inet \K[0-9.]+' | head -n1 || true)
    log "IPv4 address configured: ${ipv4_addr}"
}

configure_interface() {
    local udc_name="$1"
    local link_state
    local ipv6_addr

    ip addr flush dev "${INTERFACE_NAME}" 2>/dev/null || true
    ip link set "${INTERFACE_NAME}" up
    sysctl -w net.ipv6.conf."${INTERFACE_NAME}".disable_ipv6=0 2>/dev/null || true
    configure_static_ipv4
    sleep 1

    link_state=$(ip link show "${INTERFACE_NAME}" | grep -oP 'state \K\w+' || true)

    log "USB network gadget initialized successfully"
    log "Interface: ${INTERFACE_NAME} (MAC: ${DEV_MAC}), State: ${link_state}, UDC: ${udc_name}"

    if [ "${link_state}" = "DOWN" ]; then
        log "Interface is DOWN - waiting for USB host connection"
        echo "USB network interface '${INTERFACE_NAME}' is ready but link is DOWN (no host connected)"
        return 0
    fi

    ipv6_addr=$(ip -6 addr show dev "${INTERFACE_NAME}" scope link | grep -oP 'fe80::[0-9a-f:]+' | head -n1 || true)
    log "Link is UP - IPv6 link-local: ${ipv6_addr}"
    echo "USB network interface '${INTERFACE_NAME}' is UP with IPv6 link-local: ${ipv6_addr}"
}

main() {
    local udc_name

    log "Starting USB network gadget initialization..."
    install_udev_rule

    if ! udc_name=$(select_udc); then
        log "ERROR: No UDC (USB Device Controller) found after ${UDC_WAIT_SECONDS} seconds"
        echo "ERROR: No UDC found. Make sure aspeed_vhub is available and the USB vHub has probed." >&2
        exit 1
    fi

    create_gadget "${udc_name}"

    if ! wait_for_interface; then
        log "WARNING: Interface '${INTERFACE_NAME}' not found after ${INTERFACE_WAIT_SECONDS} seconds"
        exit 1
    fi

    configure_interface "${udc_name}"
}

main "$@"
