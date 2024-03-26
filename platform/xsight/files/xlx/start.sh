#!/bin/bash

SYS_MODE="xbm"
XPCI_NETDEV_ATTACH_IF="xcpu"
# default netdev name for asic mode if config file doesn't exist or interface doesn't exist
DEFAULT_ASIC_NETDEV_NAME="eth10g1"
SECOND_ASIC_NETDEV_NAME="eth10g0"
PORT_NUM=256
DEBUG_LEVEL=3
NETDEV_MODE=1
HW_IRQ_MODE=1
PCI_MODE=1
TX_CHECKSUMMING_MODE=0
DEFAULT_MTU=9200
ONIE_MACHINE=`sed -n -e 's/^.*onie_machine=//p' /host/machine.conf`
CFG_FILE="/etc/sonic/xlink.cfg"
LABEL_REVISION_FILE="/etc/sonic/hw_revision"
XPLT_UTL="/opt/xplt/utils"

if [[ ${ONIE_MACHINE,,} != *"kvm"* ]]; then
    # Probing cpu ixgbe net interfaces
    if [[ ! -d /sys/module/ixgbe ]]; then
        modprobe ixgbe
    fi
    # Working on HW box. Determine what to run XBM/ASIC
    if [[ -f ${CFG_FILE} ]]; then
        SYS_MODE=`sed -n -e 's/^.*sys_mode[[:blank:]]*=[[:blank:]]*//p' ${CFG_FILE}`
        XPCI_NETDEV_ATTACH_IF=`sed -n -e 's/^.*xpci_netdev[[:blank:]]*=[[:blank:]]*//p' ${CFG_FILE}`
    else
        echo ">>> WARN! Config file not found: ${CFG_FILE}"
        SYS_MODE="asic"
        XPCI_NETDEV_ATTACH_IF=${DEFAULT_ASIC_NETDEV_NAME}
    fi

    if [[ ${SYS_MODE,,} != "xbm" && ! -d /sys/class/net/${XPCI_NETDEV_ATTACH_IF} ]]; then
        echo ">>> WARN! No such interface: ${XPCI_NETDEV_ATTACH_IF}, set to ${DEFAULT_ASIC_NETDEV_NAME}"
        XPCI_NETDEV_ATTACH_IF=${DEFAULT_ASIC_NETDEV_NAME}
    fi
fi

# Checking the label revision
if [ ${SYS_MODE,,} != "xbm" ]; then
    decode-syseeprom -d | grep "Label Revision" | awk '{print $5}' > ${LABEL_REVISION_FILE}
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
echo ">>> Re-load NetDev"

# Reset X1 and PCI bus
if [ -f /tmp/xbooted ]; then
    if [ ${SYS_MODE,,} != "xbm" ]; then
        if [ -d $XPLT_UTL ]; then
            echo ">>> Resetting X1"
            $XPLT_UTL/es9632x_reset_x1.sh
            if [ $? -ne 0 ]; then
                echo "ERROR: On running es9632x_reset_x1.sh"
            fi
        else
            echo "ERROR: No $XPLT_UTL found!"
        fi
    fi
fi

rmmod xpci
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
            cd $XPLT_UTL/xcvrs && /opt/xplt/venv/bin/python config.py -m 1
        else
            echo "ERROR: No $XPLT_UTL found!"
        fi
    fi
    touch /tmp/xbooted
fi

# update default config from custom.json
FIRSTBOOT="/tmp/notify_firstboot_to_platform"
PLATFORM=$(sed -n 's/onie_platform=\(.*\)/\1/p' /host/machine.conf)
HWSKU=$(head -1 /usr/share/sonic/device/${PLATFORM}/default_sku | awk '{print $1}')

[ -f $FIRSTBOOT ] && {
    [ -f /usr/share/sonic/device/$PLATFORM/custom.json ] && {
        sonic-cfggen --from-db -j /usr/share/sonic/device/$PLATFORM/custom.json --print-data > /etc/sonic/config_db.json
        sonic-cfggen -j /usr/share/sonic/device/$PLATFORM/custom.json --write-to-db
    }
}

if [ $PLATFORM == "x86_64-kvm_x86_64-r0" ]; then
    copp_custom="/usr/share/sonic/device/$PLATFORM/$HWSKU/copp_custom.json"
else
    copp_custom="/usr/share/sonic/device/$PLATFORM/copp_custom.json"
fi

# update copp_cfg.json from copp_custom.json
[ -f $copp_custom ] && {
    sonic-cfggen -j /etc/sonic/copp_cfg.json -j $copp_custom --print-data > /tmp/copp_custom.json
    sudo mv /tmp/copp_custom.json /etc/sonic/copp_cfg.json
}
#echo ">>> Set XPCI debug level to INFO(3)"
#echo ${DEBUG_LEVEL} > /proc/sys/dev/xpci_dev/debug_level