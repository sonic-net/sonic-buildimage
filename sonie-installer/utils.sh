#!/bin/sh
#
# Shared utility functions for SONiC/ONIE installers
#

# Logs an info message to stdout.
log_info() { printf "INFO: %s\n" "$*"; }

# Logs a warning message to stderr.
log_warn() { printf "WARN: %s\n" "$*" >&2; }

# Logs an error message to stderr.
log_error() { printf "ERROR: %s\n" "$*" >&2; }

# Checks if given directory is a mountpoint; used in cases where we can't be sure mountpoint is installed.
is_mounted() {
  awk -v arg="$1" \
    '$1 == arg || $2 == arg { found = 1; exit } END { exit found ? 0 : 1; }' \
    /proc/mounts
}

# Appends a command to a trap, preserving existing traps.
_trap_push() {
    local next="${1}"
    eval "trap_push() {
        local oldcmd='$(echo "${next}" | sed -e s/\'/\'\\\\\'\'/g)'
        local newcmd=\"\${1}; \${oldcmd}\"
        trap -- \"\${newcmd}\" EXIT INT TERM HUP
        _trap_push \"\${newcmd}\"
    }"
}

# Safe wrapper for partprobe with fallback to partx/blockdev.
partprobe_safe() {
    local dev="$1"
    if command -v partprobe >/dev/null 2>&1; then
        partprobe "$@"
    elif command -v partx >/dev/null 2>&1; then
        # Fallback to partx -u if partprobe is missing
        if [ -n "$dev" ]; then
             partx -u "$dev" || partx -a "$dev" || true
        else
             echo "partx requires a device argument."
        fi
    elif command -v blockdev >/dev/null 2>&1; then
        # Fallback to blockdev --rereadpt if partprobe is missing
        if [ -n "$dev" ]; then
             blockdev --rereadpt "$dev"
        else
             echo "partprobe not found, and blockdev requires a device argument."
        fi
    else
        echo "partprobe/partx/blockdev not found, assuming partition table updated by sgdisk"
    fi
}

# Decompress and untar pipeline supporting gzip and zstd
decompress_and_untar() {
    payload="$1"
    archive="$2"
    dest_dir="$3"
    extra_options="$4"

    decompressor="cat" # Default to no-op (uncompressed tar)
    case "$archive" in
        *.gz|*.tgz)
            decompressor="gzip -dc"
            ;;
        *.zst|*.zstd)
            if command -v zstd >/dev/null 2>&1; then
                decompressor="zstd -dc"
            else
                echo "ERROR: zstd not found, cannot decompress $archive" >&2
                return 1
            fi
            ;;
    esac

    unzip -op "$payload" "$archive" | $decompressor | tar x $extra_options -f - -C "$dest_dir"
}
