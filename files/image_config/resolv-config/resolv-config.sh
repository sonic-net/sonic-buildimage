#!/bin/bash

WD=/var/run/resolvconf/
CONFIG_DIR=${WD}/interface/
STATIC_CONFIG_FILE=mgmt.static
DYNAMIC_CONFIG_FILE_TEMPLATE=*.dhclient

update_symlink()
{
    ln -sf /run/resolvconf/resolv.conf /etc/resolv.conf
}

start()
{
    update_symlink

    redis-dump -d 4 -k "DNS_NAMESERVER*" -y > /tmp/dns.json
    if [[ $? -eq 0 && "$(cat /tmp/dns.json)" != "{}" ]]; then
        # Apply static DNS configuration and disable updates
        /sbin/resolvconf --disable-updates
        pushd ${CONFIG_DIR}
        # Backup dynamic configuration to restore it when the static configuration is removed
        mv ${DYNAMIC_CONFIG_FILE_TEMPLATE} ${WD} || true

        sonic-cfggen -d -t /usr/share/sonic/templates/resolv.conf.j2,${STATIC_CONFIG_FILE}

        /sbin/resolvconf --enable-updates
        /sbin/resolvconf -u
        /sbin/resolvconf --disable-updates
        popd
    else
        # Dynamic DNS configuration. Enable updates. It is expected to receive configuraution for DHCP server
        /sbin/resolvconf --disable-updates
        pushd ${CONFIG_DIR}
        rm -f ${STATIC_CONFIG_FILE}
        # Restore dynamic configuration if it exists
        mv ${WD}/${DYNAMIC_CONFIG_FILE_TEMPLATE} ${CONFIG_DIR} || true

        /sbin/resolvconf --enable-updates
        /sbin/resolvconf -u
    fi
}

clean-dynamic-conf()
{
    rm -f ${WD}/${DYNAMIC_CONFIG_FILE_TEMPLATE}
    rm -f ${WD}/postponed-update
}

case $1 in
    start)
        start
        ;;
    cleanup)
        clean-dynamic-conf
        ;;
    *)
        echo "Usage: $0 {start|clean-dynamic-conf}"
        exit 2
        ;;
esac
