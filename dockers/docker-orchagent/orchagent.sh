#!/usr/bin/env bash

HWSKU_DIR=/usr/share/sonic/hwsku
SWSS_VARS_FILE=/usr/share/sonic/templates/swss_vars.j2

# Retrieve SWSS vars from sonic-cfggen
SWSS_VARS=$(sonic-cfggen -d -y /etc/sonic/sonic_version.yml -t $SWSS_VARS_FILE) || exit 1
export platform=$(echo $SWSS_VARS | jq -r '.asic_type')
export sub_platform=$(echo $SWSS_VARS | jq -r '.asic_subtype')

MAC_ADDRESS=$(echo $SWSS_VARS | jq -r '.mac')
if [ "$MAC_ADDRESS" == "None" ] || [ -z "$MAC_ADDRESS" ]; then
    MAC_ADDRESS=$(ip link show eth0 | grep ether | awk '{print $2}')
    logger "Mac address not found in Device Metadata, Falling back to eth0"
fi

# Create a folder for SwSS record files
mkdir -p /var/log/swss
ORCHAGENT_ARGS="-d /var/log/swss "

LOCALHOST_SWITCHTYPE=`sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" "switch_type"`
if [[ x"${LOCALHOST_SWITCHTYPE}" == x"chassis-packet" ]]; then
    # Set orchagent pop batch size to 128 for faster link notification handling 
    # during route-churn
    ORCHAGENT_ARGS+="-b 128 "
else
    # Set orchagent pop batch size to 1024
    ORCHAGENT_ARGS+="-b 1024 "
fi

# Set zmq mode by default for smartswitch DPU
# Otherwise, set synchronous mode if it is enabled in CONFIG_DB
SYNC_MODE=$(echo $SWSS_VARS | jq -r '.synchronous_mode')
SWITCH_TYPE=$(echo $SWSS_VARS | jq -r '.switch_type')
if [ "$SWITCH_TYPE" == "dpu" ]; then
    ORCHAGENT_ARGS+="-z zmq_sync "
elif [ "$SYNC_MODE" == "enable" ]; then
    ORCHAGENT_ARGS+="-s "
fi

# Check if there is an "asic_id field" in the DEVICE_METADATA in configDB.
#"DEVICE_METADATA": {
#    "localhost": {
#        ....
#        "asic_id": "0",
#    }
#},
# ID field could be integers just to denote the asic instance like 0,1,2...
# OR could be PCI device ID's which will be strings like "03:00.0"
# depending on what the SAI/SDK expects.
asic_id=$(echo $SWSS_VARS | jq -r '.asic_id')
if [ -n "$asic_id" ]
then
    ORCHAGENT_ARGS+="-i $asic_id "
fi

# for multi asic platforms add the asic name to the record file names
if [[ "$NAMESPACE_ID" ]]; then
    ORCHAGENT_ARGS+="-f swss.asic$NAMESPACE_ID.rec -j sairedis.asic$NAMESPACE_ID.rec "
fi

# Add platform specific arguments if necessary
if [ "$platform" == "broadcom" ]; then
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
elif [ "$platform" == "nephos" ]; then
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
elif [ "$platform" == "centec" ]; then
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
elif [ "$platform" == "barefoot" ]; then
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
elif [ "$platform" == "vs" ]; then
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
elif [ "$platform" == "mellanox" ]; then
    ORCHAGENT_ARGS+=""
elif [ "$platform" == "marvell-teralynx" ]; then
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
elif [ "$platform" == "nvidia-bluefield" ]; then
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
elif [ "$platform" == "pensando" ]; then
    MAC_ADDRESS=$(ip link show int_mnic0 | grep ether | awk '{print $2}')
    if [ "$MAC_ADDRESS" == "" ]; then
        MAC_ADDRESS=$(ip link show eth0-midplane | grep ether | awk '{print $2}')
    fi
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
elif [ "$platform" == "marvell-prestera" ]; then
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
    CREATE_SWITCH_TIMEOUT=`cat $HWSKU_DIR/sai.profile | grep "createSwitchTimeout" | cut -d'=' -f 2`
    if [[ ! -z $CREATE_SWITCH_TIMEOUT ]]; then
        ORCHAGENT_ARGS+=" -t $CREATE_SWITCH_TIMEOUT"
    fi
else
    # Should we use the fallback MAC in case it is not found in Device.Metadata
    ORCHAGENT_ARGS+="-m $MAC_ADDRESS"
fi

# Enable ZMQ for SmartSwitch
LOCALHOST_SUBTYPE=`sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" "subtype"`
if [[ x"${LOCALHOST_SUBTYPE}" == x"SmartSwitch" ]]; then
    midplane_mgmt_state=$( ip -json -4 addr show eth0-midplane | jq -r ".[0].operstate" )
    mgmt_ip=$( ip -json -4 addr show eth0 | jq -r ".[0].addr_info[0].local" )
    if [[ $midplane_mgmt_state == "UP" ]]; then
        # Enable ZMQ with eth0-midplane interface name
        ORCHAGENT_ARGS+=" -q tcp://eth0-midplane:8100"
    elif [[ $mgmt_ip != "" ]] && [[ $mgmt_ip != "null" ]]; then
        # If eth0-midplane interface does not up, enable ZMQ with eth0 address
        ORCHAGENT_ARGS+=" -q tcp://${mgmt_ip}:8100"
    else
        ORCHAGENT_ARGS+=" -q tcp://127.0.0.1:8100"
    fi
fi

# Add VRF parameter when mgmt-vrf enabled
MGMT_VRF_ENABLED=`sonic-db-cli CONFIG_DB hget  "MGMT_VRF_CONFIG|vrf_global" "mgmtVrfEnabled"`
if [[ x"${MGMT_VRF_ENABLED}" == x"true" ]]; then
    ORCHAGENT_ARGS+=" -v mgmt"
fi

# Enable ring buffer
ORCHDAEMON_RING_ENABLED=`sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" "ring_thread_enabled"`
if [[ x"${ORCHDAEMON_RING_ENABLED}" == x"true" ]]; then
    ORCHAGENT_ARGS+=" -R"
fi

# Add heartbeat interval when enabled
HEARTBEAT_INTERVAL=`sonic-db-cli CONFIG_DB hget  "HEARTBEAT|orchagent" "heartbeat_interval"`
if [ ! -z "$HEARTBEAT_INTERVAL" ] && [ $HEARTBEAT_INTERVAL != "null" ]; then
    ORCHAGENT_ARGS+=" -I $HEARTBEAT_INTERVAL"
fi

# Mask SIGHUP signal to avoid orchagent termination by logrotate before orchagent registers its handler.
trap '' SIGHUP

exec /usr/bin/orchagent ${ORCHAGENT_ARGS}
