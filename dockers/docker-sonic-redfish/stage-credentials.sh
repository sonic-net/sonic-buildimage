#!/bin/bash
#
# stage-credentials.sh - bring externally provisioned certificates into the
# locations bmcweb expects, enable mTLS enforcement, and fail closed once the
# unit has been provisioned.
#
# bmcweb reads a single combined server PEM and a hashed CA truststore, only at
# startup, and its paths are compile-time rather than arguments. The configured
# paths are therefore where certificates are delivered; this script reshapes
# them into what bmcweb reads and restarts it when they change.
#
# Configuration (CONFIG_DB, defaults apply when unset):
#   REDFISH|config  port                    (read by the container start script)
#   REDFISH|certs   server_crt              server certificate path
#   REDFISH|certs   server_key              server private key path
#   REDFISH|certs   ca_crt                  CA certificate path
#   REDFISH|certs   client_crt_cname        trusted client common names,
#                                           read at startup by sonic-dbus-bridge
#
# Three modes (driven by supervisord):
#   --once   Stage what is already present, then exit. Runs before bmcweb, so a
#            box that boots with certificates installed serves them directly.
#   --guard  Gate every bmcweb start: re-stage from source when the staged files
#            are missing or invalid, refuse to start when the unit is
#            provisioned and none are valid, otherwise exec bmcweb.
#   --watch  Watch the source and staged locations; restage and bounce bmcweb on
#            install, rotation or tampering. Reconciles every
#            ${BMCWEB_WATCH_INTERVAL}s and publishes status to STATE_DB.

set -u

# --- Configuration ---------------------------------------------------------
# Destinations bmcweb reads. Compile-time in bmcweb; the overrides exist for
# testing only.
HTTPS_DIR="${BMCWEB_HTTPS_DIR:-/etc/ssl/certs/https}"
AUTH_DIR="${BMCWEB_AUTH_DIR:-/etc/ssl/certs/authority}"
SERVER_PEM="${HTTPS_DIR}/server.pem"
PDATA="${BMCWEB_PDATA:-/bmcweb_persistent_data.json}"
BMCWEB_BIN="${BMCWEB_BIN:-/usr/bin/bmcweb}"

# Used when CONFIG_DB carries no REDFISH|certs entry.
DEFAULT_SERVER_CRT="/etc/sonic/redfish/redfishserver.cer"
DEFAULT_SERVER_KEY="/etc/sonic/redfish/redfishserver.key"
DEFAULT_CA_CRT="/etc/sonic/credentials/ROOT_CERTIFICATE.pem"

# Set on the first successful staging and never removed here, so the unit
# stays marked as provisioned once it has been.
MARKER="${BMCWEB_MARKER:-/var/lib/bmcweb/provisioned}"

# Fingerprint of the last applied source set, to avoid redundant bmcweb bounces.
STAMP="${BMCWEB_STAMP:-/var/lib/bmcweb/.staged-credentials.stamp}"

# Upper bound on how long a change can sit unapplied when inotify events are
# missed.
INTERVAL="${BMCWEB_WATCH_INTERVAL:-60}"

# Certificate status (STATE_DB).
STATUS_KEY="REDFISH_CERT_STATUS|global"

# This container is bridge networked, so 127.0.0.1 is its own loopback and not
# redis. sonic-db-cli aborts on this image, so use the mounted socket with the
# fixed database ids from /var/run/redis/sonic-db/database_config.json.
REDIS_SOCK="${BMCWEB_REDIS_SOCK:-/var/run/redis/redis.sock}"
CONFIG_DB_ID="${BMCWEB_CONFIG_DB_ID:-4}"
STATE_DB_ID="${BMCWEB_STATE_DB_ID:-6}"

# A failure returns non-zero and an empty result, so callers fall back to their
# defaults rather than act on a half-read configuration.
redis_cmd() {
    local db="$1"
    shift
    redis-cli -s "${REDIS_SOCK}" -n "${db}" "$@" 2>/dev/null
}

log() { logger -t stage-credentials "$*"; echo "stage-credentials: $*"; }

# --- Configuration loading -------------------------------------------------

# A read failure leaves the defaults in place: an unreachable CONFIG_DB must
# never stop a provisioned unit from staging its certificates.
load_config() {
    SERVER_CRT="$(redis_cmd "${CONFIG_DB_ID}" HGET 'REDFISH|certs' server_crt)"
    SERVER_KEY="$(redis_cmd "${CONFIG_DB_ID}" HGET 'REDFISH|certs' server_key)"
    CA_CRT="$(redis_cmd "${CONFIG_DB_ID}" HGET 'REDFISH|certs' ca_crt)"

    [ -n "${SERVER_CRT}" ] || SERVER_CRT="${DEFAULT_SERVER_CRT}"
    [ -n "${SERVER_KEY}" ] || SERVER_KEY="${DEFAULT_SERVER_KEY}"
    [ -n "${CA_CRT}" ]     || CA_CRT="${DEFAULT_CA_CRT}"
}

# Parents of the configured paths, deduplicated: the server pair and the CA may
# be delivered into different directories.
source_dirs() {
    printf '%s\n' "$(dirname "${SERVER_CRT}")" "$(dirname "${SERVER_KEY}")" \
                  "$(dirname "${CA_CRT}")" | sort -u
}

# --- Helpers ---------------------------------------------------------------

# All files present and parsing. Separate from staging so the watcher can test
# readiness before stopping a healthy bmcweb.
certs_ready() {
    [ -f "${SERVER_CRT}" ] && \
    [ -f "${SERVER_KEY}" ] && \
    [ -f "${CA_CRT}" ]     || return 1
    openssl x509 -noout -in "${SERVER_CRT}" >/dev/null 2>&1 || return 1
    openssl x509 -noout -in "${CA_CRT}" >/dev/null 2>&1 || return 1

    # Mid-rotation the cert and key are replaced in two separate operations, so
    # they can briefly disagree. Compare the public key derived from each
    # (works for RSA and EC) and skip the pass until they correspond.
    local cert_pub key_pub
    cert_pub="$(openssl x509 -in "${SERVER_CRT}" -noout -pubkey 2>/dev/null)"
    key_pub="$(openssl pkey -in "${SERVER_KEY}" -pubout 2>/dev/null)"
    [ -n "${cert_pub}" ] && [ "${cert_pub}" = "${key_pub}" ] || return 1
}

# Combined fingerprint of the three source files (content-based).
fingerprint() {
    cat "${SERVER_CRT}" "${SERVER_KEY}" "${CA_CRT}" 2>/dev/null \
        | sha256sum | awk '{print $1}'
}

# TLSStrict is enabled in bmcweb's persistent configuration.
tls_strict_set() {
    grep -q '"TLSStrict"[[:space:]]*:[[:space:]]*true' "${PDATA}" 2>/dev/null
}

# The staged output is complete and valid. Checked on the destination only, so
# the answer does not depend on the source paths. Issuer differing from subject
# is what separates a provisioned cert from bmcweb's self-signed fallback.
staged_ok() {
    openssl x509 -noout -in "${SERVER_PEM}" >/dev/null 2>&1 || return 1
    local issuer subject
    issuer="$(openssl x509 -noout -issuer -in "${SERVER_PEM}" 2>/dev/null)"
    subject="$(openssl x509 -noout -subject -in "${SERVER_PEM}" 2>/dev/null)"
    [ -n "${issuer}" ] && [ "${issuer#issuer=}" != "${subject#subject=}" ] || return 1
    [ -f "${AUTH_DIR}/CA-cert.pem" ] || return 1
    local hash
    hash="$(openssl x509 -hash -noout -in "${AUTH_DIR}/CA-cert.pem" 2>/dev/null)"
    [ -n "${hash}" ] && [ -e "${AUTH_DIR}/${hash}.0" ] || return 1
    tls_strict_set || return 1
}

# The unit has been provisioned at least once.
provisioned() {
    [ -f "${MARKER}" ]
}

# Record the provisioned state. The file explains itself to an operator.
mark_provisioned() {
    provisioned && return 0
    mkdir -p "$(dirname "${MARKER}")" 2>/dev/null
    cat > "${MARKER}" <<EOF
# Redfish certificate provisioning marker.
#
# While this file exists, bmcweb never falls back to a self-signed certificate:
# with no valid provisioned certificate staged it does not start at all, and it
# recovers on its own once valid certificates return.
#
# Do not delete this file except to return this BMC to the unprovisioned state.
provisioned_at=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
EOF
    log "recorded provisioned state in ${MARKER}"
}

# Refresh the cert files bmcweb reads. Runs on first install AND every renewal.
stage_certs() {
    certs_ready || return 1
    mkdir -p "${HTTPS_DIR}" "${AUTH_DIR}"

    # Combine cert + key into the single PEM bmcweb expects; swap atomically.
    cat "${SERVER_CRT}" "${SERVER_KEY}" > "${SERVER_PEM}.tmp" || return 1
    mv -f "${SERVER_PEM}.tmp" "${SERVER_PEM}"

    # The hash symlink is what OpenSSL's add_verify_path() looks up.
    cp -f "${CA_CRT}" "${AUTH_DIR}/CA-cert.pem.tmp" || return 1
    mv -f "${AUTH_DIR}/CA-cert.pem.tmp" "${AUTH_DIR}/CA-cert.pem"
    local hash
    hash="$(openssl x509 -hash -noout -in "${AUTH_DIR}/CA-cert.pem" 2>/dev/null)"
    if [ -n "${hash}" ]; then
        ln -sfn CA-cert.pem "${AUTH_DIR}/${hash}.0"
    else
        log "WARNING: could not compute CA hash; truststore symlink not created"
        return 1
    fi
    log "staged server.pem and CA truststore from ${SERVER_CRT} and ${CA_CRT}"
}

# TLSStrict requires a verified client certificate; MTLSCommonNameParseMode 2
# takes its CommonName as the session identity, which sonic-dbus-bridge
# validates against client_crt_cname.
#
# Written only when TLSStrict is not already set, so renewals do not reset saved
# sessions. bmcweb rewrites this file while running, so the caller must stop
# bmcweb first.
apply_auth_config() {
    tls_strict_set && return 0

    cat > "${PDATA}" <<EOF
{
  "auth_config": {
    "BasicAuth": true,
    "Cookie": true,
    "SessionToken": true,
    "XToken": true,
    "TLS": true,
    "TLSStrict": true,
    "MTLSCommonNameParseMode": 2
  },
  "sessions": [],
  "revision": 1
}
EOF
    log "enabled mTLS enforcement (TLSStrict) in ${PDATA}"
}

# bmcweb reads certs and ${PDATA} only at startup, so changes need a bounce.
# bmcweb is always started again, even after a failed stage, and the guard
# decides whether that start is allowed. The stamp advances only on success:
# advancing it on failure would pin the old certificate silently.
apply_and_bounce() {
    certs_ready || { log "credentials not ready; skipping"; return 1; }
    log "applying credentials and bouncing bmcweb"
    supervisorctl stop bmcweb >/dev/null 2>&1
    local rc=0
    if stage_certs; then
        apply_auth_config
        mark_provisioned
        fingerprint > "${STAMP}" 2>/dev/null
    else
        log "staging failed; stamp not advanced, will retry on next reconcile"
        rc=1
    fi
    supervisorctl start bmcweb >/dev/null 2>&1
    return "${rc}"
}

# Published every watcher cycle, so a rotation that fails to land is visible.
# in_sync is the field to monitor; last_update doubles as a watcher heartbeat.
publish_status() {
    local src_fp applied serial in_sync last_error now mtls
    src_fp="$(fingerprint)"
    applied="$(cat "${STAMP}" 2>/dev/null || true)"
    serial="$(openssl x509 -noout -serial -in "${SERVER_PEM}" 2>/dev/null | cut -d= -f2)"
    now="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    if ! certs_ready; then
        in_sync="false"
        last_error="credentials not ready (missing files, unparseable cert, or cert/key mismatch)"
    elif [ "${src_fp}" != "${applied}" ]; then
        in_sync="false"
        last_error="source changed but not applied (staging pending or failing)"
    elif ! staged_ok; then
        in_sync="false"
        last_error="staged output missing, invalid, or self-signed"
    else
        in_sync="true"
        last_error=""
    fi
    if staged_ok && tls_strict_set; then
        mtls="true"
    else
        mtls="false"
    fi
    redis_cmd "${STATE_DB_ID}" HSET "${STATUS_KEY}" \
        source_fingerprint "${src_fp}" \
        applied_fingerprint "${applied}" \
        served_serial "${serial}" \
        in_sync "${in_sync}" \
        mtls_enforced "${mtls}" \
        last_update "${now}" \
        last_error "${last_error}" >/dev/null || \
        log "WARNING: could not publish cert status to STATE_DB"
}

# --- Modes -----------------------------------------------------------------

run_once() {
    if ! certs_ready; then
        if provisioned; then
            # If the staged copy is gone too, the guard is about to fail closed.
            log "WARNING: no credentials at ${SERVER_CRT} on a provisioned unit; bmcweb will start only if a valid staged certificate is present"
        else
            log "no credentials at ${SERVER_CRT}; bmcweb will use its self-signed cert"
        fi
        return 0
    fi
    # bmcweb has not started yet, so no bounce is needed. Staging now means its
    # first start already uses the real certificate.
    if stage_certs; then
        apply_auth_config
        mark_provisioned
        fingerprint > "${STAMP}" 2>/dev/null
        log "staged provisioned credentials before bmcweb start"
    else
        log "staging failed at container start; guard/watcher will retry"
    fi
}

# supervisord runs this as the bmcweb program, so the checks hold whoever
# restarts bmcweb. exec hands supervisord bmcweb itself.
run_guard() {
    # Self-heal first, so a healthy unit never fails the gate spuriously.
    # bmcweb is not running here, so writing ${PDATA} is safe.
    if ! staged_ok && certs_ready; then
        log "guard: staged certificates missing or invalid; re-staging from source"
        if stage_certs; then
            apply_auth_config
            mark_provisioned
            fingerprint > "${STAMP}" 2>/dev/null
        fi
    fi
    # Fail closed: a provisioned unit never falls back to self-signed.
    # supervisord retries and eventually marks bmcweb FATAL; the watcher starts
    # it again once valid certificates appear. This path must stay fast: an exit
    # after supervisord's startsecs counts as a critical-process EXIT and
    # restarts the container.
    if provisioned && ! staged_ok; then
        log "guard: unit was provisioned before and no valid staged certificate exists; refusing to start bmcweb"
        exit 1
    fi
    exec "${BMCWEB_BIN}"
}

run_watch() {
    log "watching for credential changes (reconcile every ${INTERVAL}s)"
    # Re-arm each pass: inotify follows inodes, so an atomic replace ends the
    # watch.
    while true; do
        # Re-read so a change to the delivered paths needs no restart.
        load_config
        local dirs
        mapfile -t dirs < <(source_dirs)
        mkdir -p "${dirs[@]}" "${HTTPS_DIR}" "${AUTH_DIR}" 2>/dev/null

        # Reconcile before blocking: inotify queues nothing while this watcher
        # is down, so a change made in that window is caught here. Restage when
        # the source changed or the staged output itself is broken.
        if certs_ready; then
            local fp
            fp="$(fingerprint)"
            if [ "${fp}" != "$(cat "${STAMP}" 2>/dev/null)" ] || ! staged_ok; then
                apply_and_bounce
            fi
        fi
        # Recover from fail-closed. FATAL ends supervisord's own retries, not
        # explicit starts. Only FATAL is recovered; an operator stop is not.
        if staged_ok && supervisorctl status bmcweb 2>/dev/null | grep -q FATAL; then
            log "bmcweb is FATAL with a valid staged certificate; starting it"
            supervisorctl start bmcweb >/dev/null 2>&1
        fi
        publish_status
        # The staged locations are watched too, so tampering raises an event
        # rather than waiting for the next reconcile.
        inotifywait -q -t "${INTERVAL}" \
            -e create -e modify -e moved_to -e close_write -e delete \
            "${dirs[@]}" "${HTTPS_DIR}" "${AUTH_DIR}" >/dev/null 2>&1
        # Settle briefly so a multi-file drop is handled as one change.
        sleep 1
    done
}

load_config

case "${1:-}" in
    --once)  run_once ;;
    --guard) run_guard ;;
    --watch) run_watch ;;
    *) echo "usage: $0 --once|--guard|--watch" >&2; exit 2 ;;
esac
