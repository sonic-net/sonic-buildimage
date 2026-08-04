#!/bin/bash
# This script is invoked by config-topology.service.
# This script invokes platform plugin script if present
# which could be used for platform specific topology configuration
#
start() {
    PLATFORM=${PLATFORM:-`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`}
    #Path to platform topology script
    TOPOLOGY_SCRIPT="/usr/share/sonic/device/$PLATFORM/plugins/config-topology.sh"
    #if topology script file not present, do nothing and return 0
    [ ! -f $TOPOLOGY_SCRIPT ] && exit 0
    $TOPOLOGY_SCRIPT start
}

stop() {
    PLATFORM=${PLATFORM:-`sonic-cfggen -H -v DEVICE_METADATA.localhost.platform`}
    #Path to platform topology script
    TOPOLOGY_SCRIPT="/usr/share/sonic/device/$PLATFORM/plugins/config-topology.sh"
    #if topology script file not present, do nothing and return 0
    [ ! -f $TOPOLOGY_SCRIPT ] && exit 0
    $TOPOLOGY_SCRIPT stop
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
