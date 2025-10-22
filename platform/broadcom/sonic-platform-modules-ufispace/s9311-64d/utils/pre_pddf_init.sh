#!/bin/bash

# import sonic env
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment

TRUE=0
FALSE=1

PROTO=0
ALPHA=1
BETA=2
PVT=3

PLATFORM=${PLATFORM:-x86_64-ufispace_s9311_64d-r0}
PDDF_DEV_PATH="/usr/share/sonic/device/$PLATFORM/pddf"
PDDF_DEV_FILE="/usr/share/sonic/device/$PLATFORM/pddf/pddf-device.json"
IO_PORT_FILE="/dev/port"

function _check_filepath {
    filepath=$1
    if [ -z "${filepath}" ]; then
        echo "[ERR] the ipnut string is empty!!!"
        return ${FALSE}
    elif [ ! -f "$filepath" ] && [ ! -c "$filepath" ]; then
        echo "[ERR] No such file: ${filepath}"
        return ${FALSE}
    else
        return ${TRUE}
    fi
}

if _check_filepath "$IO_PORT_FILE" ; then
   MASK=2#00000011
   REG="0x$(xxd -s 0xE01 -p -l 1 -c 1 /dev/port)"
   HW_REV_ID=$(( $REG & $MASK ))
else
   HW_REV_ID=$BETA
fi

if [ "$HW_REV_ID" -ge "$BETA" ]; then

   src="$PDDF_DEV_PATH/pddf-device-beta.json"
   if _check_filepath $src; then
         ln -rsf "$src" "$PDDF_DEV_FILE"
   fi
else
   src="$PDDF_DEV_PATH/pddf-device-alpha.json"
   if _check_filepath $src; then
         ln -rsf "$src" "$PDDF_DEV_FILE"
   fi
fi
