#!/bin/bash

DEBUGLOG=$1
ONIE_MACHINE=`sed -n -e 's/^.*onie_machine=//p' /host/machine.conf`

function debug()
{
    /usr/bin/logger $1
    /bin/echo `date` "- $1" >> ${DEBUGLOG}
}

if [[ ${ONIE_MACHINE,,} == *"kvm"* ]]; then
   debug "Stopping XBM"
   pkill -x xbm
fi
