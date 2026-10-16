#!/usr/bin/env bash

TOPO_CONF=/usr/share/sonic/platform/topo.conf
if [ -f ${TOPO_CONF} ]; then
 topo=`cat ${TOPO_CONF}`
else
 topo="none"
fi

if [ ${topo} != "none" ]; then
    BUFFER_CONFIG=/usr/share/sonic/hwsku/buffers_defaults_${topo}.j2
    QOS_CONFIG=/usr/share/sonic/hwsku/qos_defaults_${topo}.j2

    if [ "$(/usr/local/bin/sonic-cfggen -d -v TC_TO_PRIORITY_GROUP_MAP)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v MAP_PFC_PRIORITY_TO_QUEUE)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v TC_TO_QUEUE_MAP)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v DSCP_TO_TC_MAP)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v SCHEDULER)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v PFC_PRIORITY_TO_PRIORITY_GROUP_MAP)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v PORT_QOS_MAP)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v WRED_PROFILE)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v QUEUE)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v CABLE_LENGTH)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v BUFFER_POOL)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v BUFFER_PROFILE)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v BUFFER_PG)" ] || \
       [ "$(/usr/local/bin/sonic-cfggen -d -v BUFFER_QUEUE)" ]; then
        echo "Database has QoS settings, skip loading defaults"
    else
        # Generate buffers and QoS defaults independently: a HwSKU may ship
        # only one of the two templates for a given topology, and a missing
        # QoS template must not prevent the buffers configuration from being
        # loaded (and vice versa).
        if [ -f "$BUFFER_CONFIG" ]; then
            /usr/local/bin/sonic-cfggen -d -t $BUFFER_CONFIG >/tmp/buffers.json
            /usr/local/bin/sonic-cfggen -j /tmp/buffers.json --write-to-db
        else
            echo "Warning: buffers template ${BUFFER_CONFIG} not found, skip loading buffers defaults"
        fi
        if [ -f "$QOS_CONFIG" ]; then
            /usr/local/bin/sonic-cfggen -d -t $QOS_CONFIG -y /etc/sonic/sonic_version.yml >/tmp/qos.json
            /usr/local/bin/sonic-cfggen -j /tmp/qos.json --write-to-db
        else
            echo "Warning: qos template ${QOS_CONFIG} not found, skip loading qos defaults"
        fi
    fi
else
   echo "Skip QoS config"
fi
