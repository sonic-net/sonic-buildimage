#!/bin/bash

# import sonic env
[ -f /etc/sonic/sonic-environment ] && . /etc/sonic/sonic-environment

TRUE=0
FALSE=1

PROTO=0
ALPHA=1
BETA=2
PVT=3

PLATFORM=${PLATFORM:-x86_64-ufispace_s9110_32x-r0}
PLAT_CONF_PATH="/usr/share/sonic/device/$PLATFORM"
PLAT_CONF_FILE="/usr/share/sonic/device/$PLATFORM/platform.json"
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
   REG="0x$(xxd -s 0x701 -p -l 1 -c 1 /dev/port)"
   HW_REV_ID=$(( $REG & $MASK ))
else
   HW_REV_ID=$PVT
fi

if [ "$HW_REV_ID" = "$BETA" ]; then

   src="$PDDF_DEV_PATH/pddf-device-beta.json"
   if _check_filepath $src; then
         ln -rsf "$src" "$PDDF_DEV_FILE"
   fi

   src="$PLAT_CONF_PATH/platform-beta.json"
   if _check_filepath $src; then
         ln -rsf $src "$PLAT_CONF_FILE"
   fi

elif [ "$HW_REV_ID" -ge "$PVT" ]; then
   src="$PDDF_DEV_PATH/pddf-device-pvt.json"
   if _check_filepath $src; then
         ln -rsf "$src" "$PDDF_DEV_FILE"
   fi

   src="$PLAT_CONF_PATH/platform-pvt.json"
   if _check_filepath $src; then
         ln -rsf $src "$PLAT_CONF_FILE"
   fi
else
   src="$PDDF_DEV_PATH/pddf-device-pvt.json"
   if _check_filepath $src; then
         ln -rsf "$src" "$PDDF_DEV_FILE"
   fi

   src="$PLAT_CONF_PATH/platform-pvt.json"
   if _check_filepath $src; then
         ln -rsf $src "$PLAT_CONF_FILE"
   fi
fi
