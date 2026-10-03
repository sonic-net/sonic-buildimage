#!/bin/bash
# =============================================================================
# ram2disk.sh — Sequential write of /var/log (tmpfs) to /var/log.disk
#                 (disk) backup using rsync
#
# Usage:
#   ram2disk.sh sync        — Periodic tmpfs to Disk sync with rename detection
#   ram2disk.sh restore     — Boot-time: archive previous session, restore
#                                active logs to tmpfs
#   ram2disk.sh pre-shutdown — Final sync before shutdown/reboot
#
# Design:
#   Disk layout Example:
#     /var/log.disk/
#     ├── current/                     live mirror of this boot's /var/log
#     │   ├── syslog
#     │   ├── syslog.1
#     │   ├── syslog.2.gz ...
#     │   └── frr/bgpd.log ...
#     ├── session_20260619_065000/     previous boot (immutable, compressed)
#     ├── session_20260618_120000/     older boot (immutable, compressed)
#     └── .boot_id
#
#   Cleanup:
#     - Oldest session_* directories are rm -rf'd when Disk free space is low
#
# =============================================================================

MODE="${1:-sync}"    # sync | restore | pre-shutdown

: "${SRC_RAM_DIR:=/var/log}"
: "${DST_DISK_DIR:=/var/log.disk}"
: "${CURRENT_MIRROR_DIR:=${DST_DISK_DIR}/current}"
: "${BOOT_ID_FILE:=${DST_DISK_DIR}/.boot_id}"

# Counter library that maintains per-file rotation counters in /dev/shm/logrotate.
COUNTER_FUNCS="/usr/local/bin/logrotate-counter-functions.sh"
[ -r "$COUNTER_FUNCS" ] && . "$COUNTER_FUNCS"

# Disk cleanup kicks in once disk usage of the DST_DISK_DIR filesystem exceeds this
# percentage
DISK_USED_MAX_PCT=80

# --- Logging ---
log_info()  { logger -p syslog.info -t "ram2disk" "$*" 2>/dev/null || printf 'ram2disk: %s\n' "$*" >&2; }
log_err()   { logger -p syslog.err -t "ram2disk" "$*" 2>/dev/null || printf 'ram2disk: ERROR: %s\n' "$*" >&2; }

# =============================================================================
# is_new_boot
#
# Compares current boot_id with stored one. Returns 0 (true) if new boot.
# =============================================================================
is_new_boot() {
    local current_boot_id stored_boot_id
    current_boot_id=$(cat /proc/sys/kernel/random/boot_id 2>/dev/null)

    if [ ! -f "$BOOT_ID_FILE" ]; then
        return 0
    fi

    stored_boot_id=$(cat "$BOOT_ID_FILE" 2>/dev/null)
    [ "$current_boot_id" != "$stored_boot_id" ]
}

save_boot_id() {
    cat /proc/sys/kernel/random/boot_id > "$BOOT_ID_FILE"
}

# =============================================================================
# archive_current_session
#
# Renames current/ to session_<timestamp>/
#
# Uses .boot_id mtime as the session timestamp (represents when that boot
# session started). Falls back to current time if unavailable.
#
# =============================================================================
archive_current_session() {
    [ -d "$CURRENT_MIRROR_DIR" ] || return 0

    # Use .boot_id mtime as session start timestamp
    local timestamp
    if [ -f "$BOOT_ID_FILE" ]; then
        timestamp=$(stat -c '%Y' "$BOOT_ID_FILE" \
                    | xargs -I{} date -d @{} '+%Y%m%d_%H%M%S' 2>/dev/null)
    fi
    [ -z "$timestamp" ] && timestamp=$(date '+%Y%m%d_%H%M%S')

    local session_dir="${DST_DISK_DIR}/session_${timestamp}"

    mv "$CURRENT_MIRROR_DIR" "$session_dir"

    log_info "Archived previous session → $(basename "$session_dir")"
}

# =============================================================================
# ensure_current_dir
#
# Creates current/ with necessary subdirectories if it doesn't exist.
# =============================================================================
ensure_current_dir() {
    if [ ! -d "$CURRENT_MIRROR_DIR" ]; then
        mkdir -p "$CURRENT_MIRROR_DIR"
    fi
}

# =============================================================================
# =============================================================================
# replay_rotations
#
# Replays, on current/, every logrotate cycle that happened since the last run.
#
# The set of files to replay and their counts come entirely from
# /dev/shm/logrotate/rotation_counts (maintained by the logrotate-wrapper). We
# consume it with lrc_file_read_reset, which atomically
# fetches every "<path> <count>" line and clears the file, so the next run only
# sees rotations that occur after this one.
# =============================================================================
replay_rotations() {
    declare -F lrc_file_read_reset >/dev/null || return 0

    local path count base
    while read -r path count; do
        [ -n "$path" ] || continue
        [[ "$count" =~ ^[0-9]+$ ]] || continue
        [ "$count" -gt 0 ] || continue

        # Only mirror files that live under ${SRC_RAM_DIR} (what current/ mirrors)
	# Consider adding a warning for files not under SRC_RAM_DIR to understand other files managed by logrotate
	[[ "$path" == "${SRC_RAM_DIR}/"* ]] || continue
	base="${path#"${SRC_RAM_DIR}/"}"

        mirror_renames "$base" "$count"
    done < <(lrc_file_read_reset 2>/dev/null)
}

# =============================================================================
# mirror_renames <base> <shift>
#
# Performs the same rename chain on disk's current/ that logrotate did on tmpfs.
# =============================================================================
mirror_renames() {
    local base="$1"
    local shift="$2"
    local dst_base="${CURRENT_MIRROR_DIR}/${base}"

    [ "$shift" -eq 0 ] && return

    log_info "${base}: mirroring ${shift} rotation(s) on disk"

    # Find highest archive number in current/
    local max_n=0
    for f in "${dst_base}".*.gz; do
        [ -f "$f" ] || continue
        local n
        n=$(echo "$f" | grep -oP '\.\K[0-9]+(?=\.gz$)')
        [ -n "$n" ] && [ "$n" -gt "$max_n" ] && max_n=$n
    done
    [ -f "${dst_base}.1" ] && [ "$max_n" -lt 1 ] && max_n=1

    # Start the rename shifting
    for s in $(seq 1 "$shift"); do
        # Shift .gz archives up by 1 (highest first to avoid collision)
        for i in $(seq "$max_n" -1 2); do
            [ -f "${dst_base}.${i}.gz" ] && \
                mv "${dst_base}.${i}.gz" "${dst_base}.$((i + 1)).gz"
        done
        max_n=$((max_n + 1))

        # delaycompress: .1 to .2.gz (one gzip write)
        if [ -f "${dst_base}.1" ]; then
            gzip -c "${dst_base}.1" > "${dst_base}.2.gz"
            rm -f "${dst_base}.1"
        fi

        # active to .1
        if [ "$s" -eq 1 ] && [ -f "${dst_base}" ]; then
            mv "${dst_base}" "${dst_base}.1"
        fi
    done
}

# =============================================================================
# cleanup_disk
#
# Manages disk space based on actual filesystem usage of the DST_DISK_DIR mount. While
# usage exceeds DISK_USED_MAX_PCT, it first removes the oldest session_*
# directory, then falls back to removing individual archives from
# current/ if needed.
# =============================================================================
cleanup_disk() {
    local use_pct
    use_pct=$(df -P "$DST_DISK_DIR" 2>/dev/null | awk 'NR==2 {gsub("%","",$5); print $5}')
    [[ "$use_pct" =~ ^[0-9]+$ ]] || return

    while [ "$use_pct" -gt "$DISK_USED_MAX_PCT" ]; do
        # First: remove oldest session directory (entire previous boot)
        local oldest_session
        oldest_session=$(find "$DST_DISK_DIR" -maxdepth 1 -type d -name 'session_*' \
                         | sort | head -1)

        if [ -n "$oldest_session" ]; then
            log_info "Cleanup: removing $(basename "$oldest_session")"
            rm -rf "$oldest_session"
            use_pct=$(df -P "$DST_DISK_DIR" | awk 'NR==2 {gsub("%","",$5); print $5}')
            continue
        fi

        # Fallback: remove oldest archive from current/
        local oldest_archive
        oldest_archive=$(find "$CURRENT_MIRROR_DIR" -type f -name '*.gz' -printf '%T+ %p\n' \
                         | sort | awk 'NR==1 {print $2}')

        if [ -z "$oldest_archive" ]; then
            log_info "No archives to delete — disk may fill up"
            break
        fi

        log_info "Cleanup: deleting $oldest_archive"
        rm -f "$oldest_archive"
        use_pct=$(df -P "$DST_DISK_DIR" | awk 'NR==2 {gsub("%","",$5); print $5}')
    done
}

# =============================================================================
# do_sync — Periodic sync (default mode)
# =============================================================================
do_sync() {
    # --- Step 1: Handle reboot if restore wasn't called ---
    if is_new_boot; then
        archive_current_session
        ensure_current_dir
        save_boot_id
    fi

    ensure_current_dir

    # --- Step 2: Replay logrotate rotations on current/ ---
    replay_rotations

    # --- Step 3: rsync into disk's current/ ---
    rsync -a --inplace --no-whole-file "${SRC_RAM_DIR}/" "${CURRENT_MIRROR_DIR}/"

    # --- Step 4: Disk space management ---
    cleanup_disk

    log_info "Sync complete"
}

# =============================================================================
# do_restore — Boot-time: restore active logs from disk back to tmpfs,
# then archive previous session
#
# Called early in boot before rsyslogd starts.
# 1. Restores active log files from the previous boot's current/ to tmpfs
# 2. Archives previous boot's current/ → session_<timestamp>/
# 3. Creates fresh current/ for this boot
#
# Archives stay on disk only — tmpfs starts clean except for active files.
# =============================================================================
do_restore() {
	log_info "Boot-time restore active logs to /var/log (tmpfs)"

    if [ -d "$CURRENT_MIRROR_DIR" ]; then
        log_info "Restoring active logs from current/"
        rsync -a \
            --exclude='*.[0-9]' \
            --exclude='*.[0-9][0-9]' \
            --exclude='*.[0-9][0-9][0-9]' \
            --exclude='*.[0-9].gz' \
            --exclude='*.[0-9][0-9].gz' \
            --exclude='*.[0-9][0-9][0-9].gz' \
            "${CURRENT_MIRROR_DIR}/" "${SRC_RAM_DIR}/"
    else
        log_info "No previous current/ to restore from"
    fi

    # Archive previous boot's data (mv current/ → session_<timestamp>/)
    archive_current_session

    # Create fresh current/ for this boot
    ensure_current_dir
    save_boot_id

    log_info "Restore complete"
}

# =============================================================================
# do_pre_shutdown — Final sync before shutdown/reboot
#
# Same as sync but skips cleanup
# Ensures disk's current/ has the latest data before it gets archived on next boot.
# =============================================================================
do_pre_shutdown() {
    log_info "Pre-reboot: final sync"

    ensure_current_dir

    # Replay logrotate rotations + rsync
    replay_rotations

    rsync -a --inplace --no-whole-file "${SRC_RAM_DIR}/" "${CURRENT_MIRROR_DIR}/"
    save_boot_id
    log_info "Pre-reboot sync complete"
}

# =============================================================================
# Main
# =============================================================================
# checking if DST_DISK_DIR is present or mounted
if ! mountpoint -q "$DST_DISK_DIR" 2>/dev/null; then
    log_err "$DST_DISK_DIR is not mounted, skipping"
    exit 1
fi

case "$MODE" in
    sync)       do_sync ;;
    restore)    do_restore ;;
    pre-shutdown) do_pre_shutdown ;;
    *)
        echo "Usage: $0 {sync|restore|pre-shutdown}"
        exit 1
        ;;
esac
