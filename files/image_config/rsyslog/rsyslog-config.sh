#!/bin/bash

PLATFORM=$(sonic-db-cli CONFIG_DB HGET 'DEVICE_METADATA|localhost' platform)

# Parse the device specific asic conf file, if it exists
ASIC_CONF=/usr/share/sonic/device/$PLATFORM/asic.conf
if [ -f "$ASIC_CONF" ]; then
    source $ASIC_CONF
fi

# On Multi NPU platforms we need to start  the rsyslog server on the docker0 ip address
# for the syslogs from the containers in the namespaces to work.
# on Single NPU platforms we continue to use loopback adddres

if [[ ($NUM_ASIC -gt 1) ]]; then
    udp_server_ip=$(ip -o -4 addr list docker0 | awk '{print $4}' | cut -d/ -f1)
else
    udp_server_ip=$(ip -j -4 addr list lo scope host | jq -r -M '.[0].addr_info[0].local')
fi

contain_dhcp_server=$(sonic-db-cli CONFIG_DB keys "FEATURE|dhcp_server")
if [ $contain_dhcp_server ]; then
    docker0_ip=$(ip -o -4 addr list docker0 | awk '{print $4}' | cut -d/ -f1)
fi

hostname=$(hostname)

syslog_with_osversion=$(sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" "syslog_with_osversion")
if [ -z "$syslog_with_osversion" ]; then
    syslog_with_osversion="false"
fi

os_version=$(sonic-cfggen -y /etc/sonic/sonic_version.yml -v build_version)
if [ -z "$os_version" ]; then
    os_version="Unknown"
fi

syslog_counter=$(sonic-db-cli CONFIG_DB hget "DEVICE_METADATA|localhost" "syslog_counter")
if [ -z "$syslog_counter" ]; then
    syslog_counter="false"
fi

# Generate config to a temp file so we can compare before restarting.
# On Debian 13 (Trixie), rsyslog.service has systemd sandboxing directives
# (PrivateTmp, ProtectSystem, etc.) that add ~4 seconds of overhead per
# restart due to namespace setup/teardown.  Skipping the restart when the
# config is unchanged avoids this delay on warm/fast reboot and config_reload
# where nothing actually changed.
#
# When the config IS unchanged we still send SIGHUP so rsyslog re-opens its
# log files (needed after log rotation or /var/log remounts).
#
# See: https://github.com/sonic-net/sonic-buildimage/issues/25382

TMPFILE=$(mktemp /tmp/rsyslog.conf.XXXXXX)
trap 'rm -f "$TMPFILE"' EXIT

sonic-cfggen -d -t /usr/share/sonic/templates/rsyslog.conf.j2 \
    -a "{\"udp_server_ip\": \"$udp_server_ip\", \"hostname\": \"$hostname\", \"docker0_ip\": \"$docker0_ip\", \"forward_with_osversion\": \"$syslog_with_osversion\", \"os_version\": \"$os_version\", \"syslog_counter\": \"$syslog_counter\"}" \
    > "$TMPFILE"

if [ ! -f /etc/rsyslog.conf ] || ! cmp -s "$TMPFILE" /etc/rsyslog.conf; then
    # Config changed (or first boot) — install and restart
    if cp "$TMPFILE" /etc/rsyslog.conf; then
        systemctl restart rsyslog
    else
        echo "Failed to update /etc/rsyslog.conf; not restarting rsyslog" >&2
        exit 1
    fi
else
    # Config unchanged — just signal rsyslog to re-open log files
    systemctl kill -s HUP rsyslog
fi
