#!/bin/bash
# logrotate-counter-functions.sh - sourced library functions for logrotate rotation counters.
#
# Per-file counters:     /dev/shm/logrotate/rotation_counts
#   One "<file-path> <count>" line per rotated file.
#
# A consuming application reads these and resets them to zero; the logrotate
# service increments them.
#
# Public functions:
#   lrc_file_increment <path> [<path>...]   +1 for each existing path or adds new one

#   lrc_file_get [<path>]                    print one path's count, or whole file
#   lrc_file_read_reset [<path>]             print then zero one path, or print
#                                            whole file then clear it
#   lrc_file_reset [<path>]                  zero one path, or clear whole file

: "${LRC_DIR:=/dev/shm/logrotate}"
: "${LRC_COUNTS_FILE:=$LRC_DIR/rotation_counts}"
: "${LRC_LOCK:=$LRC_DIR/rotation_count.lock}"

_lrc_ensure() {
    mkdir -p "$LRC_DIR"
    [ -f "$LRC_COUNTS_FILE" ] || : > "$LRC_COUNTS_FILE"
    [ -f "$LRC_LOCK" ]   || : > "$LRC_LOCK"
}

lrc_file_increment() {
    (( $# )) || return 0
    _lrc_ensure
    local lockfd
    exec {lockfd}>"$LRC_LOCK"; flock "$lockfd"
    printf '%s\n' "$@" | awk '
        NR==FNR { inc[$0]++; next }
        { if ($1 in inc) { $2=$2+inc[$1]; delete inc[$1] } print }
        END { for (p in inc) print p" "inc[p] }
    ' - "$LRC_COUNTS_FILE" > "$LRC_COUNTS_FILE.tmp" && mv "$LRC_COUNTS_FILE.tmp" "$LRC_COUNTS_FILE"
    exec {lockfd}>&-
}

lrc_file_get() {
    local path=${1:-}
    _lrc_ensure
    if [ -n "$path" ]; then
        awk -v p="$path" '$1==p{print $2; f=1} END{if(!f) print 0}' "$LRC_COUNTS_FILE"
    else
        cat "$LRC_COUNTS_FILE"
    fi
}

lrc_file_read_reset() {
    local path=${1:-} lockfd
    _lrc_ensure
    exec {lockfd}>"$LRC_LOCK"; flock "$lockfd"
    if [ -n "$path" ]; then
        awk -v p="$path" '$1==p{print $2; f=1} END{if(!f) print 0}' "$LRC_COUNTS_FILE"
        awk -v p="$path" '$1==p{$2=0} {print}' "$LRC_COUNTS_FILE" > "$LRC_COUNTS_FILE.tmp" && mv "$LRC_COUNTS_FILE.tmp" "$LRC_COUNTS_FILE"
    else
        cat "$LRC_COUNTS_FILE"; : > "$LRC_COUNTS_FILE"
    fi
    exec {lockfd}>&-
}

lrc_file_reset() {
    local path=${1:-} lockfd
    _lrc_ensure
    exec {lockfd}>"$LRC_LOCK"; flock "$lockfd"
    if [ -n "$path" ]; then
        awk -v p="$path" '$1==p{$2=0} {print}' "$LRC_COUNTS_FILE" > "$LRC_COUNTS_FILE.tmp" && mv "$LRC_COUNTS_FILE.tmp" "$LRC_COUNTS_FILE"
    else
        : > "$LRC_COUNTS_FILE"
    fi
    exec {lockfd}>&-
}
