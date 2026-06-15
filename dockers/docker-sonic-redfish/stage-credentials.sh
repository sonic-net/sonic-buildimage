#!/bin/bash
#
# stage-credentials.sh - bring ACMS-installed certificates into the locations
# bmcweb expects, and enable mTLS once a CA is present.
#
# Background:
#   ACMS installs certificates on the host under /etc/sonic/credentials/, which
#   is already bind-mounted read-only into this (redfish) container. bmcweb,
#   however, reads a single combined server PEM and a hashed CA truststore, and
#   only does so at startup. This script reshapes the ACMS files into what bmcweb
#   expects and (re)starts bmcweb when they appear or change.
#
# Two modes (driven by supervisord):
#   --once   Stage whatever is already present, then exit. Run BEFORE bmcweb so a
#            box that boots with ACMS certs already installed comes up directly
#            with the real cert. If nothing is present it is a no-op and bmcweb
#            falls back to its self-signed certificate.
#   --watch  Watch /etc/sonic/credentials and (re)stage + bounce bmcweb whenever
#            ACMS installs or renews certificates.
#
# No bmcweb code change is required; we only fill the paths it already reads.

set -u

# --- Configuration ---------------------------------------------------------
# Source directory (host /etc/sonic/credentials, mounted read-only).
SRC="${ACMS_CRED_DIR:-/etc/sonic/credentials}"
# ACMS file names. TODO: confirm the exact names ACMS writes; override via env
# if they differ. Defaults follow the existing manual certificate convention.
SERVER_CERT="${ACMS_SERVER_CERT:-server-cert.pem}"
SERVER_KEY="${ACMS_SERVER_KEY:-server-key.pem}"
CA_CERT="${ACMS_CA_CERT:-CA-cert.pem}"

# Destinations bmcweb reads (container-local, writable). Overridable for testing.
HTTPS_DIR="${BMCWEB_HTTPS_DIR:-/etc/ssl/certs/https}"
AUTH_DIR="${BMCWEB_AUTH_DIR:-/etc/ssl/certs/authority}"
SERVER_PEM="${HTTPS_DIR}/server.pem"
PDATA="${BMCWEB_PDATA:-/bmcweb_persistent_data.json}"

# Fingerprint of the last applied source set, to avoid redundant bmcweb bounces.
STAMP="${BMCWEB_STAMP:-/var/lib/bmcweb/.acms-credentials.stamp}"

log() { logger -t stage-credentials "$*"; echo "stage-credentials: $*"; }

# --- Helpers ---------------------------------------------------------------

# All required files present and the server cert parses. Kept separate so the
# watcher can test readiness BEFORE stopping a healthy bmcweb.
certs_ready() {
    [ -f "${SRC}/${SERVER_CERT}" ] && \
    [ -f "${SRC}/${SERVER_KEY}" ]  && \
    [ -f "${SRC}/${CA_CERT}" ]     || return 1
    openssl x509 -noout -in "${SRC}/${SERVER_CERT}" >/dev/null 2>&1 || return 1
}

# Combined fingerprint of the three source files (content-based).
fingerprint() {
    cat "${SRC}/${SERVER_CERT}" "${SRC}/${SERVER_KEY}" "${SRC}/${CA_CERT}" 2>/dev/null \
        | sha256sum | awk '{print $1}'
}

# Refresh the cert files bmcweb reads. Runs on first install AND every renewal.
stage_certs() {
    certs_ready || return 1
    mkdir -p "${HTTPS_DIR}" "${AUTH_DIR}"

    # Combine cert + key into the single PEM bmcweb expects; swap atomically.
    cat "${SRC}/${SERVER_CERT}" "${SRC}/${SERVER_KEY}" > "${SERVER_PEM}.tmp" || return 1
    mv -f "${SERVER_PEM}.tmp" "${SERVER_PEM}"

    # Install the CA into the truststore and create the hash symlink that
    # OpenSSL's add_verify_path() lookup requires.
    cp -f "${SRC}/${CA_CERT}" "${AUTH_DIR}/CA-cert.pem"
    local hash
    hash="$(openssl x509 -hash -noout -in "${AUTH_DIR}/CA-cert.pem" 2>/dev/null)"
    if [ -n "${hash}" ]; then
        ln -sf CA-cert.pem "${AUTH_DIR}/${hash}.0"
    else
        log "WARNING: could not compute CA hash; truststore symlink not created"
        return 1
    fi
    log "staged server.pem and CA truststore from ${SRC}"
}

# Enable mTLS enforcement ONCE. mTLS does not engage unless TLSStrict is set, but
# the value persists in ${PDATA} across restarts, so a renewal must not rewrite
# it (that would reset saved sessions). bmcweb rewrites this file itself, so the
# caller MUST stop bmcweb before this runs.
enable_mtls() {
    if grep -q '"TLSStrict"[[:space:]]*:[[:space:]]*true' "${PDATA}" 2>/dev/null; then
        return 0   # already enabled
    fi
    cat > "${PDATA}" <<'EOF'
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

# Stop bmcweb, (re)stage certs + enable mTLS, then start it. bmcweb only reads
# certs and ${PDATA} at startup, so a bounce is required to pick up changes.
apply_and_bounce() {
    certs_ready || { log "credentials not ready; skipping"; return 1; }
    log "applying credentials and bouncing bmcweb"
    supervisorctl stop bmcweb >/dev/null 2>&1
    stage_certs
    enable_mtls
    supervisorctl start bmcweb >/dev/null 2>&1
    fingerprint > "${STAMP}" 2>/dev/null
}

# --- Modes -----------------------------------------------------------------

run_once() {
    if ! certs_ready; then
        log "no ACMS credentials at ${SRC}; bmcweb will use its self-signed cert"
        return 0
    fi
    # bmcweb has not started yet, so no stop/start is needed here.
    stage_certs
    enable_mtls
    fingerprint > "${STAMP}" 2>/dev/null
    log "staged ACMS credentials before bmcweb start"
}

run_watch() {
    mkdir -p "${SRC}" 2>/dev/null
    log "watching ${SRC} for ACMS credential changes"
    # Re-arm the watch in a loop: inotify follows inodes, so an atomic
    # directory/file replace can end the watch (and -e returns per batch).
    while true; do
        inotifywait -q -e create -e modify -e moved_to -e close_write -e delete \
            "${SRC}" >/dev/null 2>&1
        # Settle briefly so a multi-file ACMS drop is handled as one change.
        sleep 1
        if certs_ready; then
            local fp
            fp="$(fingerprint)"
            if [ "${fp}" != "$(cat "${STAMP}" 2>/dev/null)" ]; then
                apply_and_bounce
            fi
        fi
    done
}

case "${1:-}" in
    --once)  run_once ;;
    --watch) run_watch ;;
    *) echo "usage: $0 --once|--watch" >&2; exit 2 ;;
esac
