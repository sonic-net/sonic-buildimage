#!/bin/bash
# =============================================================================
# ram2disk.sh — Sequential write of /var/log (tmpfs) to /var/log.disk
#                 (disk) backup using rsync
#
# Usage:
#   ram2disk.sh sync         — Periodic tmpfs to disk sync with rotation replay
#   ram2disk.sh pre-shutdown — Final sync before shutdown/reboot
#
# Design:
#   Disk layout example:
#     /var/log.disk/
#     ├── current_session/                 live mirror of this boot's /var/log
#     │   ├── syslog
#     │   ├── syslog.1
#     │   ├── syslog.2.gz ...
#     │   ├── syslog.5000~20261115T010203.gz  generation tmpfs deleted (disk only)
#     │   └── frr/bgpd.log ...
#     ├── legacy_20260619_065000/          (optional) stray files from a non-tmpfs boot
#     ├── session_20260619_065000/         previous boot (immutable)
#     ├── session_20260618_120000/         older boot (immutable)
#     ├── .ram2disk/staging/               mirror files in flight during a replay (Not used in normal operation)
#     └── .boot_id
#
#   Sync = replay rotations on the mirror, rsync, commit (sync_current_session):
#     1. ram2disk_replay.py replay   repeats on current_session/ the renames
#        tmpfs went through since the last sync, so rsync delta-updates the
#        same generation instead of overwriting another one. Policy agnostic:
#        it follows tmpfs inode numbers, not names or logrotate config.
#        Generations tmpfs deleted are kept as <name>~<mtime>.
#     2. rsync -a --inplace --no-whole-file (no --delete: the mirror never
#        loses a file because tmpfs did).
#     3. ram2disk_replay.py commit   records what the mirror now holds.
#
#   Replay state is boot-local in R2D_STATE_DIR (tmpfs)
#   Every mode runs under R2D_LOCK, which the logrotate wrapper also holds.
#
#   Cleanup:
#     - Oldest legacy_*/session_* directories, then oldest current_session files
# =============================================================================

MODE="${1:-sync}"    # sync | pre-shutdown

: "${SRC_RAM_DIR:=/var/log}"
: "${DST_DISK_DIR:=/var/log.disk}"
: "${CURRENT_MIRROR_DIR:=${DST_DISK_DIR}/current_session}"
: "${BOOT_ID_FILE:=${DST_DISK_DIR}/.boot_id}"
: "${KERNEL_BOOT_ID:=/proc/sys/kernel/random/boot_id}"
# Rotation lock, shared with the logrotate wrapper: rotation never overlaps a sync.
: "${R2D_LOCK:=/dev/shm/logrotate/synchronization.lock}"
: "${R2D_LOCK_WAIT:=120}"          # seconds to wait for the lock
# Replay state is boot-local; staging must be on the mirror's filesystem.
: "${R2D_STATE_DIR:=/dev/shm/ram2disk}"
: "${R2D_STAGING_DIR:=${DST_DISK_DIR}/.ram2disk/staging}"
: "${R2D_REPLAY:=$(dirname "$(readlink -f "$0")")/ram2disk_replay.py}"

# Disk cleanup kicks in once disk usage of the DST_DISK_DIR filesystem exceeds this
# percentage
DISK_USED_MAX_PCT=80

# --- Logging ---
log_info()  { logger -p syslog.info -t "ram2disk" "$*" 2>/dev/null || printf 'ram2disk: %s\n' "$*" >&2; }
log_err()   { logger -p syslog.err -t "ram2disk" "$*" 2>/dev/null || printf 'ram2disk: ERROR: %s\n' "$*" >&2; }


# =============================================================================
# Rotation lock
#
# One flock on R2D_LOCK serialises everything that renames log files, on tmpfs
# or on the mirror: sync and pre-shutdown here, and logrotate through
# its wrapper (flock on the same file around the real logrotate).
#   r2d_lock    take it for the rest of this shell, waiting up to R2D_LOCK_WAIT s
#   r2d_unlock  release it (exiting the shell releases it too)
# =============================================================================
r2d_lock() {
    [ -z "${R2D_LOCK_FD:-}" ] || return 0
    mkdir -p -- "${R2D_LOCK%/*}" && exec {R2D_LOCK_FD}>>"$R2D_LOCK" || return 1
    flock -w "$R2D_LOCK_WAIT" "$R2D_LOCK_FD" && return 0
    r2d_unlock
    return 1
}

r2d_unlock() {
    [ -n "${R2D_LOCK_FD:-}" ] || return 0
    exec {R2D_LOCK_FD}>&-                    # closing the descriptor releases the flock
    unset R2D_LOCK_FD
}


# =============================================================================
# is_new_boot
#
# Compares current boot_id with stored one. Returns 0 (true) if new boot.
# =============================================================================
is_new_boot() {
    local current_boot_id stored_boot_id
    current_boot_id=$(cat "$KERNEL_BOOT_ID" 2>/dev/null)

    if [ ! -f "$BOOT_ID_FILE" ]; then
        return 0
    fi

    stored_boot_id=$(cat "$BOOT_ID_FILE" 2>/dev/null)
    [ "$current_boot_id" != "$stored_boot_id" ]
}

save_boot_id() {
    cat "$KERNEL_BOOT_ID" > "$BOOT_ID_FILE"
}


# =============================================================================
# archive_current_session
#
# Renames current_session/ to session_<timestamp>/
#
# Uses .boot_id mtime as the session timestamp (represents when that boot
# session started). Falls back to current time if unavailable.
# =============================================================================
archive_current_session() {
    [ -d "$CURRENT_MIRROR_DIR" ] || return 0

    # A replay interrupted by the reboot may have left mirror files in staging:
    # file them with the session they belong to. On failure they stay in
    # staging and the first sync of this boot files them instead.
    recover_replay_staging || log_err "Cannot recover replay staging; continuing"
    # Use .boot_id mtime as session start timestamp
    local timestamp
    if [ -f "$BOOT_ID_FILE" ]; then
        timestamp=$(stat -c '%Y' "$BOOT_ID_FILE" \
                    | xargs -I{} date -d @{} '+%Y%m%d_%H%M%S' 2>/dev/null)
    fi
    [ -z "$timestamp" ] && timestamp=$(date '+%Y%m%d_%H%M%S')

    local session_dir="${DST_DISK_DIR}/session_${timestamp}"

    mv "$CURRENT_MIRROR_DIR" "$session_dir" || return
    # The inode-map describes the mirror that was just archived.
    rm -f -- "$R2D_STATE_DIR/inode-map" "$R2D_STATE_DIR/inode-map.next" || return

    log_info "Archived previous session → $(basename "$session_dir")"
}


# =============================================================================
# archive_legacy_files
#
# DST_DISK_DIR may hold logs written directly to it by an earlier boot where the
# disk image was mounted on /var/log (non-tmpfs mode). ram2disk only manages
# current_session/ and session_*/, so move any other top-level entries into a single
# legacy_<timestamp>/ directory. No-op when nothing needs moving.
# =============================================================================
archive_legacy_files() {
    local -a stray=()
    local entry
    while IFS= read -r -d '' entry; do
        stray+=("$entry")
    done < <(find "$DST_DISK_DIR" -mindepth 1 -maxdepth 1 \
                 ! -name current_session ! -name 'session_*' ! -name 'legacy_*' \
                 ! -name lost+found ! -name "$(basename "$BOOT_ID_FILE")" \
                 ! -name "$(basename "$(dirname "$R2D_STAGING_DIR")")" -print0)

    [ "${#stray[@]}" -gt 0 ] || return 0

    local legacy_dir="${DST_DISK_DIR}/legacy_$(date '+%Y%m%d_%H%M%S')"
    mkdir -p "$legacy_dir" && mv -- "${stray[@]}" "$legacy_dir/"
    log_info "Moved ${#stray[@]} legacy top-level entries → $(basename "$legacy_dir")"
}


# =============================================================================
# ensure_current_dir
#
# Creates current_session/ with necessary subdirectories if it doesn't exist.
# =============================================================================
ensure_current_dir() {
    if [ ! -d "$CURRENT_MIRROR_DIR" ]; then
        mkdir -p "$CURRENT_MIRROR_DIR"
    fi
}


# =============================================================================
# Rotation replay (ram2disk_replay.py, see its header for the details)
#
#   replay_rotations   before rsync: finish any interrupted replay, then make
#                      current_session/ match tmpfs's names (renames, moves for
#                      copytruncate, history for deleted generations).
#   replay_commit      after a successful rsync: record what the mirror holds.
#   recover_replay_staging  put staged mirror files back: finish the plan if
#                      it still exists, else (plan lost in a reboot) keep them
#                      under current_session/ram2disk-recovered~<time>/.
# Any replay operation error leaves its plan in R2D_STATE_DIR and is retried
# by the next call; rsync is skipped until replay succeeds.
# =============================================================================
run_replay_helper() {
    python3 "$R2D_REPLAY" "$1" "$SRC_RAM_DIR" "$CURRENT_MIRROR_DIR" \
            "$R2D_STATE_DIR" "$R2D_STAGING_DIR"
}

replay_rotations() {
    local summary
    if [ "$(stat -f -c %T -- "$SRC_RAM_DIR")" != tmpfs ]; then
        log_err "$SRC_RAM_DIR is not tmpfs; refusing to sync"
        return 1
    fi
    summary=$(run_replay_helper replay) || return
    [ -z "$summary" ] || log_info "$summary"
}

replay_commit() {
    run_replay_helper commit
}

recover_replay_staging() {
    run_replay_helper recover-staging
}

# Replay, rsync and commit; shared by periodic and shutdown syncs.
sync_current_session() {
    local rc
    if ! replay_rotations; then
        log_err "Cannot replay rotations; will resume on next sync"
        return 1
    fi
    rsync -a --inplace --no-whole-file "${SRC_RAM_DIR}/" "${CURRENT_MIRROR_DIR}/"
    rc=$?
    # 24: some source files vanished during the transfer, normal for logs.
    if [ "$rc" -ne 0 ] && [ "$rc" -ne 24 ]; then
        log_err "Sync failed (rsync exit $rc)"
        return 1
    fi
    if ! replay_commit; then
        log_err "Cannot record the replay inode-map"
        return 1
    fi
}


# =============================================================================
# cleanup_disk
#
# Manages disk space based on actual filesystem usage of the DST_DISK_DIR mount. While
# usage exceeds DISK_USED_MAX_PCT, it first removes the oldest legacy_*/session_*
# directory, then falls back to removing the oldest individual file from
# current_session/ (preserved history is oldest, so it goes first).
# =============================================================================
cleanup_disk() {
    local use_pct
    use_pct=$(df -P "$DST_DISK_DIR" 2>/dev/null | awk 'NR==2 {gsub("%","",$5); print $5}')
    [[ "$use_pct" =~ ^[0-9]+$ ]] || return

    while [ "$use_pct" -gt "$DISK_USED_MAX_PCT" ]; do
        # First: remove oldest legacy/session directory (entire previous boot)
        local oldest_session
        oldest_session=$(find "$DST_DISK_DIR" -maxdepth 1 -type d \( -name 'legacy_*' -o -name 'session_*' \) \
                         -printf '%f\t%p\n' | sort -t '_' -k2,2 -k3,3 | head -n 1 | cut -f2-)

        if [ -n "$oldest_session" ]; then
            log_info "Cleanup: removing $(basename "$oldest_session")"
            rm -rf "$oldest_session"
            use_pct=$(df -P "$DST_DISK_DIR" | awk 'NR==2 {gsub("%","",$5); print $5}')
            continue
        fi

        # Fallback: remove the oldest file from current_session/
        local oldest_archive
        oldest_archive=$(find "$CURRENT_MIRROR_DIR" -type f -printf '%T@\t%p\n' \
                         | sort -n | head -n 1 | cut -f 2-)

        if [ -z "$oldest_archive" ]; then
            log_info "No files to delete — disk may fill up"
            break
        fi

        log_info "Cleanup: deleting $oldest_archive"
        rm -f "$oldest_archive"
        use_pct=$(df -P "$DST_DISK_DIR" | awk 'NR==2 {gsub("%","",$5); print $5}')
    done
}


# =============================================================================
# initialize_current_boot_session — Initialize this boot before periodic or shutdown sync
# =============================================================================
initialize_current_boot_session() {
    if is_new_boot; then
        archive_current_session || return
        archive_legacy_files || return
        ensure_current_dir || return
        save_boot_id || return
    fi

    ensure_current_dir
}


# =============================================================================
# do_sync — Periodic sync (default mode)
# =============================================================================
do_sync() {
    if ! initialize_current_boot_session; then
        log_err "Cannot initialize current session"
        return 1
    fi

    cleanup_disk
    sync_current_session || return
    log_info "Sync complete"
}


# =============================================================================
# do_pre_shutdown — Final sync before shutdown/reboot
#
# Same as sync but skips cleanup
# Ensures disk's current_session/ has the latest data before it gets archived on next boot.
# =============================================================================
do_pre_shutdown() {
    log_info "Pre-reboot: final sync"

    if ! initialize_current_boot_session; then
        log_err "Cannot initialize current session for pre-reboot sync"
        return 1
    fi

    # Replay rotations + rsync + commit
    if ! sync_current_session; then
        log_err "Pre-reboot sync failed"
        return 1
    fi
    log_info "Pre-reboot sync complete"
}


# =============================================================================
# Main
# =============================================================================
case "$MODE" in
    sync|pre-shutdown) ;;
    *)
        echo "Usage: $0 {sync|pre-shutdown}"
        exit 1 ;;
esac

# checking if DST_DISK_DIR is present or mounted
if ! mountpoint -q "$DST_DISK_DIR" 2>/dev/null; then
    log_err "$DST_DISK_DIR is not mounted, skipping"
    exit 1
fi

if ! r2d_lock; then
    if [ "$MODE" = sync ]; then
        log_err "Rotation lock busy for ${R2D_LOCK_WAIT}s; skipping this sync"
        exit 1
    fi
    # The final sync must not be skipped.
    log_err "Rotation lock busy for ${R2D_LOCK_WAIT}s; continuing without it"
fi

case "$MODE" in
    sync)         do_sync ;;
    pre-shutdown) do_pre_shutdown ;;
esac
rc=$?
r2d_unlock
exit "$rc"

