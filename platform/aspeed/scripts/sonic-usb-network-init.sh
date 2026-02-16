#!/bin/bash
# USB Network Gadget Initialization Script for ASPEED AST2700
# Sets up USB network device using ConfigFS

set -e

# Configuration
GADGET_NAME="g1"
FUNCTION_TYPE="ncm"  # Options: ncm, ecm, eem, rndis
INTERFACE_NAME="usb0"
# BMC_IP="192.168.100.1"
SUBNET_MASK="24"
VENDOR_ID="0x1d6b"    # Linux Foundation
PRODUCT_ID="0x0104"   # Multifunction Composite Gadget
SERIAL_NUMBER="0123456789"
MANUFACTURER="ASPEED BMC"
PRODUCT_NAME="USB Network Device"
DEV_MAC="02:00:00:00:00:01"
HOST_MAC="02:00:00:00:00:02"

logger -t usb-network "Starting USB network gadget initialization..."

# Step 1: Load kernel modules
logger -t usb-network "Loading kernel modules..."
modprobe libcomposite 2>/dev/null || true
modprobe usb_f_${FUNCTION_TYPE} 2>/dev/null || true
modprobe u_ether 2>/dev/null || true
modprobe aspeed_vhub 2>/dev/null || true

# Step 2: Mount ConfigFS
if ! mountpoint -q /sys/kernel/config 2>/dev/null; then
    mount -t configfs none /sys/kernel/config
    logger -t usb-network "ConfigFS mounted at /sys/kernel/config"
fi

# Step 3: Create gadget configuration
GADGET_DIR="/sys/kernel/config/usb_gadget/${GADGET_NAME}"

# Remove existing gadget if present
if [ -d "${GADGET_DIR}" ]; then
    logger -t usb-network "Removing existing gadget configuration..."
    # Disable gadget first
    if [ -f "${GADGET_DIR}/UDC" ]; then
        echo "" > "${GADGET_DIR}/UDC" 2>/dev/null || true
    fi
    # Remove symlinks
    rm -f "${GADGET_DIR}/configs/c.1/${FUNCTION_TYPE}.${INTERFACE_NAME}" 2>/dev/null || true
    # Remove directories
    rmdir "${GADGET_DIR}/functions/${FUNCTION_TYPE}.${INTERFACE_NAME}" 2>/dev/null || true
    rmdir "${GADGET_DIR}/configs/c.1/strings/0x409" 2>/dev/null || true
    rmdir "${GADGET_DIR}/configs/c.1" 2>/dev/null || true
    rmdir "${GADGET_DIR}/strings/0x409" 2>/dev/null || true
    rmdir "${GADGET_DIR}" 2>/dev/null || true
fi

# Create new gadget
logger -t usb-network "Creating USB gadget configuration..."
mkdir -p "${GADGET_DIR}"
cd "${GADGET_DIR}"

# Set USB device descriptor
echo "${VENDOR_ID}" > idVendor
echo "${PRODUCT_ID}" > idProduct
echo "0x0100" > bcdDevice  # v1.0.0
echo "0x0200" > bcdUSB     # USB 2.0

# Create English strings
mkdir -p strings/0x409
echo "${SERIAL_NUMBER}" > strings/0x409/serialnumber
echo "${MANUFACTURER}" > strings/0x409/manufacturer
echo "${PRODUCT_NAME}" > strings/0x409/product

# Create configuration
mkdir -p configs/c.1/strings/0x409
echo "${FUNCTION_TYPE^^} Network" > configs/c.1/strings/0x409/configuration
echo "250" > configs/c.1/MaxPower  # 250mA

# Create network function
mkdir -p "functions/${FUNCTION_TYPE}.${INTERFACE_NAME}"
echo "${DEV_MAC}" > "functions/${FUNCTION_TYPE}.${INTERFACE_NAME}/dev_addr"
echo "${HOST_MAC}" > "functions/${FUNCTION_TYPE}.${INTERFACE_NAME}/host_addr"

# Link function to configuration
ln -s "functions/${FUNCTION_TYPE}.${INTERFACE_NAME}" configs/c.1/

logger -t usb-network "Gadget '${GADGET_NAME}' created with ${FUNCTION_TYPE^^} function"

# Step 4: Enable the gadget
# Find the first available UDC (USB Device Controller)
UDC_NAME=$(ls /sys/class/udc 2>/dev/null | head -n1)

if [ -z "${UDC_NAME}" ]; then
    logger -t usb-network "ERROR: No UDC (USB Device Controller) found!"
    echo "ERROR: No UDC found. Make sure aspeed_vhub module is loaded." >&2
    exit 1
fi

logger -t usb-network "Using UDC: ${UDC_NAME}"
echo "${UDC_NAME}" > UDC

# Step 5: Wait for interface to appear
logger -t usb-network "Waiting for network interface ${INTERFACE_NAME}..."
for i in {1..10}; do
    if ip link show "${INTERFACE_NAME}" &>/dev/null; then
        logger -t usb-network "Interface '${INTERFACE_NAME}' detected"
        break
    fi
    sleep 1
    logger -t usb-network "Interface '${INTERFACE_NAME}' still not up"
done

if ! ip link show "${INTERFACE_NAME}" &>/dev/null; then
    logger -t usb-network "WARNING: Interface '${INTERFACE_NAME}' not found after 10 seconds"
    exit 1
fi

# Step 6: Configure network interface
logger -t usb-network "Configuring network interface..."
ip addr flush dev "${INTERFACE_NAME}" 2>/dev/null || true

# Bring interface up (required for IPv6)
ip link set "${INTERFACE_NAME}" up

# Enable IPv6 on the interface
sysctl -w net.ipv6.conf.${INTERFACE_NAME}.disable_ipv6=0 2>/dev/null || true

# Wait a moment for interface to stabilize
sleep 1

# Check interface state
LINK_STATE=$(ip link show "${INTERFACE_NAME}" | grep -oP 'state \K\w+')

logger -t usb-network "USB network gadget initialized successfully"
logger -t usb-network "Interface: ${INTERFACE_NAME} (MAC: ${DEV_MAC}), State: ${LINK_STATE}, UDC: ${UDC_NAME}"

# IPv6 link-local address will be auto-configured when link comes up
# Provide guidance on next steps
if [ "${LINK_STATE}" = "DOWN" ]; then
    logger -t usb-network "Interface is DOWN - waiting for USB host connection"
    logger -t usb-network "To bring switch CPU out of reset, run: switch_cpu_utils.sh reset-out"
    echo "USB network interface '${INTERFACE_NAME}' is ready but link is DOWN (no host connected)"
    echo "To bring switch CPU out of reset and establish USB connection, run:"
    echo "  switch_cpu_utils.sh reset-out"
else
    # Link is UP - show IPv6 address
    IPV6_ADDR=$(ip -6 addr show dev "${INTERFACE_NAME}" scope link | grep -oP 'fe80::[0-9a-f:]+' | head -n1)
    logger -t usb-network "Link is UP - IPv6 link-local: ${IPV6_ADDR}"
    echo "USB network interface '${INTERFACE_NAME}' is UP with IPv6 link-local: ${IPV6_ADDR}"
fi

exit 0

