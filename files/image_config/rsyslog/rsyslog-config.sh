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

bridged_syslog_features="dhcp_server redfish"
for feature in $bridged_syslog_features; do
    if [ -n "$(sonic-db-cli CONFIG_DB keys "FEATURE|$feature")" ]; then
        docker0_ip=$(ip -o -4 addr list docker0 | awk '{print $4}' | cut -d/ -f1)
        break
    fi
done

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

# Profiles whose PEMs host-services copied to /etc/sonic/ssl-certs/<id>/.
# The Jinja template only emits TLS omfwd for ids listed here so missing certs
# never fall back to cleartext. The JSON is built with jq so directory names
# are safely encoded.
ssl_profiles_json="{}"
if [ -d /etc/sonic/ssl-certs ]; then
    for d in /etc/sonic/ssl-certs/*/; do
        [ -d "$d" ] || continue
        id=$(basename "$d")
        if [ -f "${d}ca-bundle.pem" ] && [ -f "${d}cert.pem" ] && [ -f "${d}key.pem" ]; then
            ssl_profiles_json=$(jq -c --arg id "$id" '. + {($id): true}' <<< "$ssl_profiles_json")
        fi
    done
fi

ssl_profile_ready() {
    local id="$1"
    local d="/etc/sonic/ssl-certs/${id}"
    [ -n "$id" ] && [ -f "${d}/ca-bundle.pem" ] && [ -f "${d}/cert.pem" ] && [ -f "${d}/key.pem" ]
}

# (rsyslog-config restart without a CONFIG_DB SYSLOG_SERVER change) still
# updates STATE_DB SYSLOG_SERVER|<addr> applied_transport_security.
publish_syslog_tls_apply_state() {
    local apply_ok="$1"
    local key proto ts profile peers applied reason proto_lc ts_lc k found

    mapfile -t cfg_keys < <(sonic-db-cli CONFIG_DB KEYS "SYSLOG_SERVER|*" 2>/dev/null || true)

    while IFS= read -r k; do
        [ -n "$k" ] || continue
        found=0
        for key in "${cfg_keys[@]}"; do
            if [ "$k" = "$key" ]; then
                found=1
                break
            fi
        done
        if [ "$found" -eq 0 ]; then
            sonic-db-cli STATE_DB DEL "$k" >/dev/null 2>&1 || true
        fi
    done < <(sonic-db-cli STATE_DB KEYS "SYSLOG_SERVER|*" 2>/dev/null || true)

    for key in "${cfg_keys[@]}"; do
        [ -n "$key" ] || continue
        proto=$(sonic-db-cli CONFIG_DB HGET "$key" protocol 2>/dev/null || true)
        ts=$(sonic-db-cli CONFIG_DB HGET "$key" transport_security 2>/dev/null || true)
        profile=$(sonic-db-cli CONFIG_DB HGET "$key" tls_profile_id 2>/dev/null || true)
        peers=$(sonic-db-cli CONFIG_DB HGET "$key" tls_permitted_peers 2>/dev/null || true)
        applied="false"
        reason="NONE"
        proto_lc=$(printf '%s' "$proto" | tr '[:upper:]' '[:lower:]')
        ts_lc=$(printf '%s' "$ts" | tr '[:upper:]' '[:lower:]')
        if [ "$proto_lc" = "tls" ] || [ "$ts_lc" = "true" ] || [ "$ts_lc" = "tls" ] || \
           [ "$ts_lc" = "enabled" ] || [ "$ts_lc" = "1" ]; then
            if [ -z "$profile" ]; then
                reason="NO_PROFILE"
            elif [ -z "$peers" ]; then
                reason="NO_PERMITTED_PEERS"
            elif ! ssl_profile_ready "$profile"; then
                reason="NO_CERTIFICATE"
            elif [ "$apply_ok" != "true" ]; then
                reason="RSYSLOG_APPLY_FAILED"
            else
                applied="true"
                reason="NONE"
            fi
        fi
        sonic-db-cli STATE_DB HSET "$key" applied_transport_security "$applied" >/dev/null 2>&1 || true
        sonic-db-cli STATE_DB HSET "$key" failure_reason "$reason" >/dev/null 2>&1 || true
    done
}

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

# Build the -a argument with jq so that runtime values (which can
# contain quotes, backslashes, or other JSON-breaking characters) are
# safely encoded, rather than interpolated as raw strings into a
# hand-built JSON literal. The rsyslog.conf.j2 template also strips
# newlines/quotes/backslashes/percent signs from values rendered into
# $template directives; this jq encoding ensures the JSON itself is
# well-formed so sonic-cfggen can parse it in the first place.
json_args=$(jq -n \
    --arg udp_server_ip "$udp_server_ip" \
    --arg docker0_ip "$docker0_ip" \
    --arg forward_with_osversion "$syslog_with_osversion" \
    --arg os_version "$os_version" \
    --arg syslog_counter "$syslog_counter" \
    --argjson ssl_profiles "$ssl_profiles_json" \
    '{udp_server_ip: $udp_server_ip, docker0_ip: $docker0_ip, forward_with_osversion: $forward_with_osversion, os_version: $os_version, syslog_counter: $syslog_counter, ssl_profiles: $ssl_profiles}')

sonic-cfggen -d -t /usr/share/sonic/templates/rsyslog.conf.j2 \
    -a "$json_args" \
    > "$TMPFILE"

if [ ! -f /etc/rsyslog.conf ] || ! cmp -s "$TMPFILE" /etc/rsyslog.conf; then
    # Config changed (or first boot) — install and restart
    if cp "$TMPFILE" /etc/rsyslog.conf; then
        systemctl restart rsyslog
        publish_syslog_tls_apply_state true
    else
        echo "Failed to update /etc/rsyslog.conf; not restarting rsyslog" >&2
        publish_syslog_tls_apply_state false
        exit 1
    fi
else
    if [[ ($NUM_ASIC -gt 1) ]]; then
        # multi-asic, docker0 IP interface may not be present when rsyslog.service was started.
        # restart the rsyslog for TCP port socket binding.
        systemctl restart rsyslog
    else
        # Config unchanged — just signal rsyslog to re-open log files
        systemctl kill -s HUP rsyslog
    fi
    publish_syslog_tls_apply_state true
fi
