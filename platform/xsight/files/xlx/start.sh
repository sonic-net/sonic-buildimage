#!/bin/bash

SYS_MODE="xbm"
XPCI_NETDEV_ATTACH_IF="xcpu"
# default netdev name for asic mode if config file doesn't exist or interface doesn't exist
DEFAULT_ASIC_NETDEV_NAME="xpcxd0"
SECOND_ASIC_NETDEV_NAME="eth10g0"
PORT_NUM=128
DEBUG_LEVEL=3
NETDEV_MODE=1
HW_IRQ_MODE=1
PCI_MODE=1
TX_CHECKSUMMING_MODE=0
DEFAULT_MTU=9200
ONIE_MACHINE=`sed -n -e 's/^.*onie_machine=//p' /host/machine.conf`
LABEL_REVISION_FILE="/etc/sonic/hw_revision"
XPLT_UTL="/opt/xplt/utils"
XSIGHT_PCI_SIG="1e6c"
XSIGHT_PCI_ID=""
XSIGHT_DEVICE=""
FIRSTBOOT="/tmp/notify_firstboot_to_platform"

if [ -f $FIRSTBOOT ]; then
    PLATFORM=$(sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf)

    # update default config from custom.json
    if [ -f /usr/share/sonic/device/$PLATFORM/custom.json ]; then
        sonic-cfggen --from-db -j /usr/share/sonic/device/$PLATFORM/custom.json --print-data > /etc/sonic/config_db.json
        sonic-cfggen -j /usr/share/sonic/device/$PLATFORM/custom.json --write-to-db
    fi
fi

fname=$(basename $0)

# The eth0 interface intermittently disappears on reboot.
# When this happens, eth0 is assigned by ice driver to different (0000:f4:00.0) interface,
# which concurrently comes up first. The udev tries to fix it later but fails to rename
# the interface since it already exists. The eth0 is released later, but no one tries to reuse it again.
#   sonic systemd-udevd[380]: eth1: Failed to rename network interface 3 from 'eth1' to 'eth0': File exists
#   sonic kernel: ice 0000:f4:00.0 eth10g0: renamed from eth0
# As a workaround, we reload the igb driver when the eth0 was busy during the igb probe.
if [[ ${ONIE_MACHINE,,} == *"es9618"* ]]; then
    out=$(ip a l eth0 | grep "UP")
    res=$?
    echo "$fname: eth0 link UP check result=$res, output=$out" | tee /dev/kmsg
    if [[ "$res" -ne 0 ]]; then
        echo "$fname: recover eth0" | tee /dev/kmsg
        modprobe -r igb && sleep 2 && modprobe igb && \
        sleep 5 && ifup eth0 && sleep 5
        out=$(ip a l eth0 | grep "UP")
        res=$?
        if [ $? -ne 0 ]; then
            echo "$fname: failed to recover eth0" | tee /dev/kmsg
            ifup eth0
        fi
    fi
fi

if [[ ${ONIE_MACHINE,,} != *"kvm"* ]]; then
    XSIGHT_PCI_ID=$(lspci | grep -E -o "${XSIGHT_PCI_SIG}:[0-9]{4}" | awk -F ":" '{print $2}')
    if [[ "${XSIGHT_PCI_ID}" == "0001" ]]; then
        XSIGHT_DEVICE="X1"
    elif [[ "${XSIGHT_PCI_ID}" == "0002" ]]; then
        XSIGHT_DEVICE="X2"
    else
        echo "Error: Unsuported Xsight device or no device found"
        exit 1
    fi

    if [[ "${XSIGHT_DEVICE}" == "X1" ]]; then
        PORT_NUM=256
        DEFAULT_ASIC_NETDEV_NAME="eth10g1"
    elif [[ "${XSIGHT_DEVICE}" == "X2" ]]; then
        PORT_NUM=128
        DEFAULT_ASIC_NETDEV_NAME="xpcxd0"
    fi

    # Probing cpu ixgbe net interfaces
    if [[ ! -d /sys/module/ixgbe ]]; then
        modprobe ixgbe
    fi

    SYS_MODE="asic"
    XPCI_NETDEV_ATTACH_IF=${DEFAULT_ASIC_NETDEV_NAME}

    if [[ ${SYS_MODE,,} != "xbm" && ! -d /sys/class/net/${XPCI_NETDEV_ATTACH_IF} ]]; then
        echo ">>> WARN! No such interface: ${XPCI_NETDEV_ATTACH_IF}, set to ${DEFAULT_ASIC_NETDEV_NAME}"
        XPCI_NETDEV_ATTACH_IF=${DEFAULT_ASIC_NETDEV_NAME}
    fi

    # Checking the label revision
    decode-syseeprom | grep "Label Revision" | awk '{print $5}' > ${LABEL_REVISION_FILE}
fi

if [[ ${XPCI_NETDEV_ATTACH_IF} == "xcpu" ]]; then
    echo ">>> Configuring CPU port VETH devices"
    # Setup CPU port
    ip link add name veth0 type veth peer name xcpu
    ip link set dev veth0 mtu ${DEFAULT_MTU} up
fi

ip link set dev ${XPCI_NETDEV_ATTACH_IF} mtu ${DEFAULT_MTU} up

if [[ ${SYS_MODE,,} == "xbm" ]]; then
    HW_IRQ_MODE=0
    PCI_MODE=0
fi

rmmod xpci

echo ">>> Re-load NetDev"

if [[ ${ONIE_MACHINE,,} != *"kvm"* ]]; then
    if [[ "${XSIGHT_DEVICE}" == "X1" ]]; then
        XPLT_SWITCH_CHIP_RESET=$XPLT_UTL/es9632x_reset_x1.sh
    elif [[ "${XSIGHT_DEVICE}" == "X2" ]]; then
	sleep 12
        XPLT_SWITCH_CHIP_RESET=$XPLT_UTL/es9618x_reset_x2.sh
    fi

    # Reset switch chip and PCI bus
    if [ -f /tmp/xbooted ]; then
        if [ ${SYS_MODE,,} != "xbm" ]; then
            if [ -d $XPLT_UTL ]; then
                echo ">>> Resetting ${XSIGHT_DEVICE} chip"
                $XPLT_SWITCH_CHIP_RESET
                if [ $? -ne 0 ]; then
                    echo "ERROR: Cannot reset ${XSIGHT_DEVICE} chip"
                fi
            else
                echo "ERROR: No $XPLT_UTL found!"
            fi
        fi
    fi
fi

#read -p "--- Press enter to continue ---"
# TODO: change insmod to modprobe after moving xdrivers build into sonic-buildimage
# debug_level = 1 - Error
#               2 - Notice
#               3 - Info
#               4 - Debug
#               5 - Debug with Packet trace
modprobe xpci attach_if=${XPCI_NETDEV_ATTACH_IF} num_of_ports=${PORT_NUM} debug_level=${DEBUG_LEVEL} \
              netdev_mode=${NETDEV_MODE} hw_irq_mode=${HW_IRQ_MODE} pci_mode=${PCI_MODE} \
              tx_checksumming=${TX_CHECKSUMMING_MODE}

echo ">>> Sleeping 5"
sleep 5

if [ ! -f /tmp/xbooted ]; then
    if [ ${SYS_MODE,,} != "xbm" ]; then
        if [ -d $XPLT_UTL ]; then
            echo ">>> Configure xcvrs"
            cd $XPLT_UTL/xcvrs && ./config.py -m 2
        else
            echo "ERROR: No $XPLT_UTL found!"
        fi
    fi
    touch /tmp/xbooted
fi

#echo ">>> Set XPCI debug level to INFO(3)"
echo ${DEBUG_LEVEL} > /proc/sys/dev/xpci/debug_level

