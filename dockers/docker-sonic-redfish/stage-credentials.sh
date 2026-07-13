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

set -u

# --- Configuration ---------------------------------------------------------
SRC="${ACMS_CRED_DIR:-/etc/sonic/credentials}"
# ACMS file names. These are the STABLE names (symlinks) that ACMS repoints on
# rotation -- not the underlying versioned files (<profile>_<ver>_<ts>_cert.pem),
# which change every rotation. All reads below follow symlinks transparently.
# Override via env if the names differ.
# TODO: confirm whether the CA (server_ca.crt) is also a rotated symlink or a
# stable plain file -- the read path works either way, but it affects what the
# watcher must react to.
SERVER_CERT="${ACMS_SERVER_CERT:-server.crt}"
SERVER_KEY="${ACMS_SERVER_KEY:-server.key}"
CA_CERT="${ACMS_CA_CERT:-server_ca.crt}"

# Destinations bmcweb reads (container-local, writable). Overridable for testing.
HTTPS_DIR="${BMCWEB_HTTPS_DIR:-/etc/ssl/certs/https}"
AUTH_DIR="${BMCWEB_AUTH_DIR:-/etc/ssl/certs/authority}"
SERVER_PEM="${HTTPS_DIR}/server.pem"
PDATA="${BMCWEB_PDATA:-/bmcweb_persistent_data.json}"

# Fingerprint of the last applied source set, to avoid redundant bmcweb bounces.
STAMP="${BMCWEB_STAMP:-/var/lib/bmcweb/.acms-credentials.stamp}"

# Watcher wake interval (seconds). inotify events do not reliably cross the
# /etc/sonic bind mount for host-side ACMS writes, so the watch also wakes on
# this timeout and reconciles. Each wake is a cheap fingerprint compare.
WATCH_INTERVAL="${BMCWEB_WATCH_INTERVAL:-60}"

# STATE_DB key (redis db 6) where the cert sync status is published, so a
# persistent out-of-sync (a rotation that did not land) is observable instead
# of silent. Read with: sonic-db-cli STATE_DB hgetall 'REDFISH_CERT_STATUS|global'
STATUS_KEY="REDFISH_CERT_STATUS|global"

log() { logger -t stage-credentials "$*"; echo "stage-credentials: $*"; }

# --- Helpers ---------------------------------------------------------------

# All required files present and the server cert parses. Kept separate so the
# watcher can test readiness BEFORE stopping a healthy bmcweb.
certs_ready() {
    [ -f "${SRC}/${SERVER_CERT}" ] && \
    [ -f "${SRC}/${SERVER_KEY}" ]  && \
    [ -f "${SRC}/${CA_CERT}" ]     || return 1
    openssl x509 -noout -in "${SRC}/${SERVER_CERT}" >/dev/null 2>&1 || return 1
    # The CA must parse too. It has no pair-guard to catch it below, and letting
    # a corrupt CA through would fail stage_certs at the hash step on every
    # reconcile, bouncing bmcweb once per watch interval forever.
    openssl x509 -noout -in "${SRC}/${CA_CERT}" >/dev/null 2>&1 || return 1

    # Guard the rotation window: the cert and key symlinks are repointed in two
    # separate (individually atomic) operations, so there is a brief moment where
    # one points to the new version and the other to the old. Staging that
    # mismatched pair would make bmcweb's TLS load fail. Only proceed when the
    # cert and key actually correspond -- compare the public key derived from
    # each (algorithm-agnostic: works for RSA and EC). If they do not match
    # (mid-rotation transient), skip this pass; the next event/reconcile sees the
    # consistent state.
    local cert_pub key_pub
    cert_pub="$(openssl x509 -in "${SRC}/${SERVER_CERT}" -noout -pubkey 2>/dev/null)"
    key_pub="$(openssl pkey -in "${SRC}/${SERVER_KEY}" -pubout 2>/dev/null)"
    [ -n "${cert_pub}" ] && [ "${cert_pub}" = "${key_pub}" ] || return 1
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
    # Only advance the stamp when staging actually succeeds. Writing the stamp
    # unconditionally would mark the new certs "applied" even if server.pem was
    # not rewritten, and every later reconcile would then short-circuit and never
    # retry. On failure, leave the stamp so the next reconcile tries again.
    if stage_certs; then
        enable_mtls
        enable_secure_mode
        fingerprint > "${STAMP}" 2>/dev/null
    else
        log "staging failed; leaving stamp unchanged so the next reconcile retries"
    fi
    # Always bring bmcweb back, even if staging failed, so it is not left down.
    supervisorctl start bmcweb >/dev/null 2>&1
}

# Read the secure-mode policy from CONFIG_DB
# (DEVICE_METADATA|localhost:redfish_secure_mode). True on provisioned units,
# where bmcweb must never fall back to a self-signed certificate.
secure_mode() {
    local v
    # Read via the mounted redis unix socket (CONFIG_DB is redis db 4). This is
    # the reliable path inside the redfish container: sonic-db-cli usually cannot
    # resolve the SONiC DB map here (no database_config.json) and errors out, so
    # it is only a fallback for environments where the socket is not present.
    v="$(redis-cli -s /var/run/redis/redis.sock -n 4 hget 'DEVICE_METADATA|localhost' redfish_secure_mode 2>/dev/null)"
    if [ -z "${v}" ] && command -v sonic-db-cli >/dev/null 2>&1; then
        v="$(sonic-db-cli CONFIG_DB hget 'DEVICE_METADATA|localhost' redfish_secure_mode 2>/dev/null)"
    fi
    [ "${v}" = "true" ] || [ "${v}" = "enabled" ]
}

# Turn the secure-mode policy ON once real ACMS certs have been staged, so the
# unit auto-transitions to production security (bmcweb never falls back to a
# self-signed cert). Written to CONFIG_DB (DEVICE_METADATA|localhost) via the
# redis socket, same path secure_mode() reads. Idempotent: only sets when not
# already on. Note: this sets the RUNNING config; it is re-applied on each boot
# by run_once when certs are present. Persisting it across reboots (config save)
# is a host-side action, out of scope for the container.
enable_secure_mode() {
    if [ "$(redis-cli -s /var/run/redis/redis.sock -n 4 hget 'DEVICE_METADATA|localhost' redfish_secure_mode 2>/dev/null)" != "true" ]; then
        redis-cli -s /var/run/redis/redis.sock -n 4 hset 'DEVICE_METADATA|localhost' redfish_secure_mode true >/dev/null 2>&1 \
            && log "enabled redfish_secure_mode (ACMS certificate provisioned)"
    fi
}

# True when bmcweb has a real (CA-signed, not self-signed) cert staged, along
# with the CA truststore and TLSStrict. Checks the DESTINATION only, so it holds
# even if the source is momentarily unavailable. A self-signed cert (issuer ==
# subject), such as bmcweb's own fallback, does not count as staged.
staged_ok() {
    [ -f "${SERVER_PEM}" ] || return 1
    local subj iss hash
    subj="$(openssl x509 -in "${SERVER_PEM}" -noout -subject 2>/dev/null)"
    iss="$(openssl x509 -in "${SERVER_PEM}" -noout -issuer 2>/dev/null)"
    [ -n "${subj}" ] || return 1
    [ "${subj#subject}" != "${iss#issuer}" ] || return 1
    [ -f "${AUTH_DIR}/CA-cert.pem" ] || return 1
    hash="$(openssl x509 -hash -noout -in "${AUTH_DIR}/CA-cert.pem" 2>/dev/null)"
    [ -n "${hash}" ] && [ -L "${AUTH_DIR}/${hash}.0" ] || return 1
    grep -q '"TLSStrict"[[:space:]]*:[[:space:]]*true' "${PDATA}" 2>/dev/null || return 1
}

# Publish cert sync status to STATE_DB so a rotation that did not land is
# observable (in_sync=false) instead of silently serving the old cert. Computes
# state itself: in_sync means the source certs equal what was last applied AND
# the staged output is valid. Best-effort; never fails the caller.
publish_status() {
    local src_fp applied served insync err ts
    src_fp="$(fingerprint 2>/dev/null)"
    applied="$(cat "${STAMP}" 2>/dev/null)"
    served="$(openssl x509 -in "${SERVER_PEM}" -noout -serial 2>/dev/null | cut -d= -f2)"
    ts="$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null)"
    if certs_ready && [ -n "${src_fp}" ] && [ "${src_fp}" = "${applied}" ] && staged_ok; then
        insync=true; err=""
    else
        insync=false
        if ! certs_ready; then
            err="credentials not ready (missing files, unparseable cert, or cert/key mismatch)"
        elif [ "${src_fp}" != "${applied}" ]; then
            err="source certs changed but not applied (staging pending or failing)"
        elif ! staged_ok; then
            err="staged cert missing, tampered, or self-signed"
        else
            err="out of sync"
        fi
    fi
    redis-cli -s /var/run/redis/redis.sock -n 6 hset "${STATUS_KEY}" \
        source_fingerprint "${src_fp}" \
        applied_fingerprint "${applied}" \
        served_serial "${served}" \
        in_sync "${insync}" \
        last_update "${ts}" \
        last_error "${err}" >/dev/null 2>&1
}

# --- Modes -----------------------------------------------------------------

run_once() {
    if ! certs_ready; then
        log "no ACMS credentials at ${SRC}; bmcweb will use its self-signed cert"
        publish_status
        return 0
    fi
    # bmcweb has not started yet, so no stop/start is needed here. Advance the
    # stamp only on a successful stage, same as apply_and_bounce, so a failed
    # boot-time stage does not mark the certs "applied" and block later reconciles.
    if stage_certs; then
        enable_mtls
        enable_secure_mode
        fingerprint > "${STAMP}" 2>/dev/null
        log "staged ACMS credentials before bmcweb start"
    else
        log "staging failed at boot; leaving stamp unchanged so the watcher retries"
    fi
    publish_status
}

run_watch() {
    mkdir -p "${SRC}" 2>/dev/null
    log "watching ${SRC} for changes"
    # Re-arm the watch in a loop: inotify follows inodes, so an atomic
    # directory/file replace can end the watch (and -e returns per batch).
    while true; do
        # Reconcile FIRST, before blocking on inotifywait. Re-stage when the
        # source content changed (fingerprint differs from the last applied).
        # The source must be present to restore from; if it is gone, nothing is
        # staged and the running bmcweb keeps its in-memory cert until a restart
        # (secure-mode then decides at start time). Reconciling at the top of the
        # loop also catches source changes made while the watcher was down
        # (inotify does not queue events for a dead process). Idempotent: a
        # re-stage happens only on a real content change.
        if certs_ready && [ "$(fingerprint)" != "$(cat "${STAMP}" 2>/dev/null)" ]; then
            apply_and_bounce
        fi
        # Publish the sync status to STATE_DB every cycle, so a persistent
        # out-of-sync (a rotation that did not land) is visible, not silent.
        publish_status
        # Block until the next change, OR until WATCH_INTERVAL elapses. The
        # timeout is the safety net: host-side ACMS writes to the bind-mounted
        # /etc/sonic often do not deliver inotify events inside the container, so
        # we cannot rely on events alone; the periodic wake reconciles regardless.
        inotifywait -q -t "${WATCH_INTERVAL}" \
            -e create -e modify -e moved_to -e close_write -e delete \
            "${SRC}" >/dev/null 2>&1
        # Settle briefly so a multi-file ACMS drop is handled as one change.
        sleep 1
    done
}

# --guard : gate bmcweb startup. Runs as bmcweb's supervisord command. If a valid
# staged cert is missing but the ACMS source is available, stage it first
# (self-heal). In secure mode, refuse to start bmcweb without a valid staged cert
# so it can never self-sign (fail closed); the watcher will start bmcweb once
# ACMS provisions certs. Otherwise (bootstrap, secure mode off) let bmcweb start
# and self-sign as before.
run_guard() {
    if ! staged_ok && certs_ready; then
        log "guard: staged certificate missing or invalid; staging from ${SRC}"
        stage_certs && enable_mtls && enable_secure_mode && fingerprint > "${STAMP}" 2>/dev/null
    fi
    if secure_mode && ! staged_ok; then
        log "guard: secure_mode is on and no valid ACMS certificate is staged; refusing to start bmcweb (fail closed)"
        sleep 5
        exit 1
    fi
    log "guard: starting bmcweb"
    exec /usr/bin/bmcweb
}

case "${1:-}" in
    --once)  run_once ;;
    --watch) run_watch ;;
    --guard) run_guard ;;
    *) echo "usage: $0 --once|--watch|--guard" >&2; exit 2 ;;
esac
