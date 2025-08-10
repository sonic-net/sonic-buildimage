#!/usr/bin/env bash

PLATFORM_DIR=/usr/share/sonic/platform
HWSKU_DIR=/usr/share/sonic/hwsku

SYNCD_SOCKET_FILE=/var/run/sswsyncd/sswsyncd.socket

# Function: wait until syncd has created the socket for bcmcmd to connect to
wait_syncd() {
    while true; do
        if [ -e ${SYNCD_SOCKET_FILE} ]; then
            socket_created=true
            break
        fi
        sleep 1
    done

    # wait until bcm sdk is ready to get a request
    sleep 3
}


# Remove stale files if they exist
rm -f /var/run/rsyslogd.pid
rm -f ${SYNCD_SOCKET_FILE}

supervisorctl start rsyslogd

mkdir -p /etc/sai.d/

# Create/Copy the sai.profile to /etc/sai.d/sai.profile
if [ -f $HWSKU_DIR/sai.profile.j2 ]; then
    sonic-cfggen -d -t $HWSKU_DIR/sai.profile.j2 > /etc/sai.d/sai.profile
else
    if [ -f $HWSKU_DIR/sai.profile ]; then
        cp $HWSKU_DIR/sai.profile /etc/sai.d/sai.profile
    fi
fi

supervisorctl start syncd

# If this platform has a pre_emphasis setting file, load it
if [[ -r $HWSKU_DIR/pre-emphasis_PAM4_optics.soc && ! -f /var/warmboot/warm-starting ]]; then
    wait_syncd
    /usr/bin/bcmcmd -t 60 "rcload $HWSKU_DIR/pre-emphasis_PAM4_optics.soc"
fi

# If this platform has an initialization file for the Broadcom LED microprocessor, load it
if [[ -r ${PLATFORM_DIR}/led_proc_init.soc && ! -f /var/warmboot/warm-starting ]]; then
    [[ -z "$socket_created" ]] && wait_syncd
    supervisorctl start ledinit
fi
