#!/usr/bin/env bash

CMD_SYNCD=/usr/bin/syncd

# dsserve: domain socket server for stdio
CMD_DSSERVE=/usr/bin/dsserve
CMD_DSSERVE_ARGS="$CMD_SYNCD --diag"

ENABLE_SAITHRIFT=0

PLATFORM_DIR=/usr/share/sonic/platform
HWSKU_DIR=/usr/share/sonic/hwsku

SONIC_ASIC_TYPE=$(sonic-cfggen -y /etc/sonic/sonic_version.yml -v asic_type)

if [ -x $CMD_DSSERVE ]; then
    CMD=$CMD_DSSERVE
    CMD_ARGS=$CMD_DSSERVE_ARGS
else
    CMD=$CMD_SYNCD
    CMD_ARGS=
fi

# Use temporary view between init and apply
CMD_ARGS+=" -u"

case "$(cat /proc/cmdline)" in
  *SONIC_BOOT_TYPE=fastfast*)
    if [ -e /var/warmboot/warm-starting ]; then
        FASTFAST_REBOOT='yes'
    fi
    ;;
  *SONIC_BOOT_TYPE=fast*|*fast-reboot*)
     FAST_REBOOT='yes'
    ;;
  *)
     FAST_REBOOT='no'
     FASTFAST_REBOOT='no'
    ;;
esac


function check_warm_boot()
{
    # FIXME: if we want to continue start option approach, then we need to add
    #        code here to support redis database query.
    # SYSTEM_WARM_START=`sonic-db-cli STATE_DB hget "WARM_RESTART_ENABLE_TABLE|system" enable`
    # SERVICE_WARM_START=`sonic-db-cli STATE_DB hget "WARM_RESTART_ENABLE_TABLE|${SERVICE}" enable`
    # SYSTEM_WARM_START could be empty, always make WARM_BOOT meaningful.
    # if [[ x"$SYSTEM_WARM_START" == x"true" ]] || [[ x"$SERVICE_WARM_START" == x"true" ]]; then
    #     WARM_BOOT="true"
    # else
        WARM_BOOT="false"
    # fi
}


function set_start_type()
{
    if [ x"$WARM_BOOT" == x"true" ]; then
        CMD_ARGS+=" -t warm"
    elif [ x"$FAST_REBOOT" == x"yes" ]; then
        CMD_ARGS+=" -t fast"
    elif [ x"$FASTFAST_REBOOT" == x"yes" ]; then
        CMD_ARGS+=" -t fastfast"
    fi
}


config_syncd_bcm()
{
    PLATFORM_COMMON_DIR=/usr/share/sonic/device/x86_64-broadcom_common
    PLT_CONFIG_BCM=$(find $HWSKU_DIR -name '*.bcm')

    if [ ! -z "$PLT_CONFIG_BCM" ]; then
       cp -f $HWSKU_DIR/*.config.bcm /tmp
       cp -f /etc/sai.d/sai.profile /tmp
       CONFIG_BCM=$(find /tmp -name '*.bcm')
       SAI_PROFILE=$(find /tmp -name 'sai.profile')
       sed -i 's+/usr/share/sonic/hwsku+/tmp+g' $SAI_PROFILE

       #Get first three characters of chip id
       readline=$(grep '0x14e4' /proc/linux-kernel-bde)
       chip_id=${readline#*0x14e4:0x}
       chip_id=${chip_id::3}
       COMMON_CONFIG_BCM=$(find $PLATFORM_COMMON_DIR/x86_64-broadcom_${chip_id} -name '*.bcm')
   
       if [ -f $PLATFORM_COMMON_DIR/x86_64-broadcom_${chip_id}/*.bcm ]; then
          for file in $CONFIG_BCM; do
              echo "" >> $file
              echo "# Start of chip common properties" >> $file
              while read line
              do
                line=$( echo $line | xargs )
                if [ ! -z "$line" ];then
                    if [ "${line::1}" == '#' ];then
                        echo $line >> $file
                    else
                        sedline=${line%=*}
                        if grep -q $sedline $file ;then
                           echo "Keep the config $(grep $sedline $file) in $file"
                        else
                           echo $line >> $file
                        fi
                    fi
                fi
              done < $COMMON_CONFIG_BCM
              echo "# End of chip common properties" >> $file
          done
          echo "Merging $PLT_CONFIG_BCM with $COMMON_CONFIG_BCM, merge files stored in $CONFIG_BCM"
       fi
    #sync the file system
    sync

    # copy the final config.bcm and sai.profile to the shared folder for 'show tech'
    cp -f /tmp/sai.profile /var/run/sswsyncd/
    cp -f /tmp/*.bcm /var/run/sswsyncd/
    fi

    if [ -f "/tmp/sai.profile" ]; then
        CMD_ARGS+=" -p /tmp/sai.profile"
    elif [ -f "/etc/sai.d/sai.profile" ]; then
        CMD_ARGS+=" -p /etc/sai.d/sai.profile"
    else
        CMD_ARGS+=" -p $HWSKU_DIR/sai.profile"
    fi

    [ -e /dev/linux-bcm-knet ] || mknod /dev/linux-bcm-knet c 122 0
    [ -e /dev/linux-user-bde ] || mknod /dev/linux-user-bde c 126 0
    [ -e /dev/linux-kernel-bde ] || mknod /dev/linux-kernel-bde c 127 0
}

config_syncd_mlnx()
{
    CMD_ARGS+=" -p /tmp/sai.profile"

    [ -e /dev/sxdevs/sxcdev ] || ( mkdir -p /dev/sxdevs && mknod /dev/sxdevs/sxcdev c 231 193 )

    # Read MAC address
    MAC_ADDRESS="$(sonic-cfggen -d -v DEVICE_METADATA.localhost.mac)"

    # Write MAC address into /tmp/profile file.
    cat $HWSKU_DIR/sai.profile > /tmp/sai.profile
    echo "DEVICE_MAC_ADDRESS=$MAC_ADDRESS" >> /tmp/sai.profile
    echo "SAI_WARM_BOOT_WRITE_FILE=/var/warmboot/" >> /tmp/sai.profile
}

config_syncd_centec()
{
    CMD_ARGS+=" -p $HWSKU_DIR/sai.profile"

    [ -e /dev/linux_dal ] || mknod /dev/linux_dal c 198 0
    [ -e /dev/net/tun ] || ( mkdir -p /dev/net && mknod /dev/net/tun c 10 200 )
}

config_syncd_marvell_prestera()
{
    CMD_ARGS+=" -p $HWSKU_DIR/sai.profile"

    [ -e /dev/net/tun ] || ( mkdir -p /dev/net && mknod /dev/net/tun c 10 200 )
}

config_syncd_barefoot()
{
    PROFILE_FILE="$HWSKU_DIR/sai.profile"
    if [ ! -f $PROFILE_FILE ]; then
        # default profile file
        PROFILE_FILE="/tmp/sai.profile"
        echo "SAI_KEY_WARM_BOOT_WRITE_FILE=/var/warmboot/sai-warmboot.bin" > $PROFILE_FILE
        echo "SAI_KEY_WARM_BOOT_READ_FILE=/var/warmboot/sai-warmboot.bin" >> $PROFILE_FILE
    fi
    CMD_ARGS+=" -p $PROFILE_FILE"

    # Check and load SDE profile
    P4_PROFILE=$(sonic-cfggen -d -v 'DEVICE_METADATA["localhost"]["p4_profile"]')
    if [[ -n "$P4_PROFILE" ]]; then
        if [[ ( -d /opt/bfn/install_${P4_PROFILE} ) && ( -L /opt/bfn/install || ! -e /opt/bfn/install ) ]]; then
            ln -srfn /opt/bfn/install_${P4_PROFILE} /opt/bfn/install
        fi
    fi
    export PYTHONHOME=/opt/bfn/install/
    export PYTHONPATH=/opt/bfn/install/
    export ONIE_PLATFORM=`grep onie_platform /etc/machine.conf | awk 'BEGIN { FS = "=" } ; { print $2 }'`
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/bfn/install/lib/platform/$ONIE_PLATFORM:/opt/bfn/install/lib:/opt/bfn/install/lib/tofinopd/switch
    ./opt/bfn/install/bin/dma_setup.sh
}

config_syncd_nephos()
{
    CMD_ARGS+=" -p $HWSKU_DIR/sai.profile"
}

config_syncd_vs()
{
    CMD_ARGS+=" -p $HWSKU_DIR/sai.profile"
}

config_syncd_marvell_teralynx()
{
    CMD_ARGS+=" -p $HWSKU_DIR/sai.profile"
    ulimit -s 65536
    export II_ROOT="/var/log/mrvl_teralynx"
    export II_APPEND_LOG=1
    mkdir -p $II_ROOT
}

config_syncd()
{
    check_warm_boot

    if [ "$SONIC_ASIC_TYPE" == "broadcom" ]; then
        config_syncd_bcm
    elif [ "$SONIC_ASIC_TYPE" == "mellanox" ]; then
        config_syncd_mlnx
    elif [ "$SONIC_ASIC_TYPE" == "centec" ]; then
        config_syncd_centec
    elif [ "$SONIC_ASIC_TYPE" == "marvell-prestera" ]; then
        config_syncd_marvell_prestera
     elif [ "$SONIC_ASIC_TYPE" == "barefoot" ]; then
         config_syncd_barefoot
    elif [ "$SONIC_ASIC_TYPE" == "nephos" ]; then
        config_syncd_nephos
    elif [ "$SONIC_ASIC_TYPE" == "vs" ]; then
        config_syncd_vs
    elif [ "$SONIC_ASIC_TYPE" == "marvell-teralynx" ]; then
        config_syncd_marvell_teralynx
    else
        echo "Unknown ASIC type $SONIC_ASIC_TYPE"
        exit 1
    fi

    set_start_type

    if [ ${ENABLE_SAITHRIFT} == 1 ]; then
        CMD_ARGS+=" -r -m $HWSKU_DIR/port_config.ini"
    fi

    [ -r $PLATFORM_DIR/syncd.conf ] && . $PLATFORM_DIR/syncd.conf
}

