#!/bin/bash
#
# scouty.sh — SONiC log viewer wrapper
#
# Loads the latest syslog, swss, and sairedis logs by default.
# Supports --since <duration> to filter logs within a time window.
#
# Usage:
#   scouty.sh [--since <duration>] [-- <extra scouty-tui args>]
#
# Duration format: 30m, 1h, 5h, 1d, 3d, 7d (minutes, hours, days)
#

set -euo pipefail

SINCE=""
EXTRA_ARGS=()
NUM_FILES=3

while [[ $# -gt 0 ]]; do
    case "$1" in
        --since)
            shift
            if [[ $# -eq 0 ]]; then
                echo "Error: --since requires a duration argument (e.g. 30m, 1h, 1d, 3d)" >&2
                exit 1
            fi
            SINCE="$1"
            shift
            ;;
        --)
            shift
            EXTRA_ARGS=("$@")
            break
            ;;
        -h|--help)
            cat <<'EOF'
Usage: scouty.sh [--since <duration>] [-- <extra scouty-tui args>]

SONiC log viewer — loads the latest syslog, swss, and sairedis logs.

Options:
  --since <duration>   Only load logs within this time window.
                       Format: 30m, 1h, 5h, 1d, 3d, 7d
                       (m=minutes, h=hours, d=days)
  --                   Pass remaining arguments to scouty-tui
  -h, --help           Show this help

Examples:
  scouty.sh                        View latest 3 of each log type
  scouty.sh --since 1d             Logs from last 24 hours
  scouty.sh --since 30m            Logs from last 30 minutes
  scouty.sh --since 3d -- -f       Last 3 days, follow mode
  scouty.sh -- --filter 'level == "Error"'   Filter errors
EOF
            exit 0
            ;;
        *)
            echo "Unknown option: $1 (use -- to pass args to scouty-tui)" >&2
            exit 1
            ;;
    esac
done

duration_to_seconds() {
    local duration="$1"

    if ! [[ "$duration" =~ ^([0-9]+)([mhdMHD])$ ]]; then
        echo "Error: invalid duration '$duration'. Use format: 30m, 1h, 1d (m=min, h=hour, d=day)." >&2
        return 1
    fi

    local num="${BASH_REMATCH[1]}"
    local unit="${BASH_REMATCH[2]}"

    case "$unit" in
        m|M) echo $((num * 60)) ;;
        h|H) echo $((num * 3600)) ;;
        d|D) echo $((num * 86400)) ;;
    esac
}

collect_logs() {
    local pattern="$1"
    local max="$2"
    local cutoff_epoch="${3:-0}"
    local count=0
    local -a files=()

    # Sort by mtime desc, handle empty matches and filenames with spaces.
    mapfile -t files < <(stat --printf='%Y\t%n\n' $pattern 2>/dev/null | sort -rn | cut -f2-)

    for f in "${files[@]}"; do
        [[ -f "$f" ]] || continue

        if [[ "$cutoff_epoch" -gt 0 ]]; then
            local file_mtime
            file_mtime=$(stat -c %Y "$f" 2>/dev/null || echo 0)
            if [[ "$file_mtime" -lt "$cutoff_epoch" ]]; then
                continue
            fi
        fi

        echo "$f"
        count=$((count + 1))
        if [[ "$count" -ge "$max" ]]; then
            break
        fi
    done
}

CUTOFF_EPOCH=0
if [[ -n "$SINCE" ]]; then
    SINCE_SECONDS=$(duration_to_seconds "$SINCE")
    CUTOFF_EPOCH=$(( $(date +%s) - SINCE_SECONDS ))
fi

LOG_FILES=()

while IFS= read -r f; do
    [[ -n "$f" ]] && LOG_FILES+=("$f")
done < <(collect_logs "/var/log/syslog*" "$NUM_FILES" "$CUTOFF_EPOCH")

while IFS= read -r f; do
    [[ -n "$f" ]] && LOG_FILES+=("$f")
done < <(collect_logs "/var/log/swss/swss.rec*" "$NUM_FILES" "$CUTOFF_EPOCH")

while IFS= read -r f; do
    [[ -n "$f" ]] && LOG_FILES+=("$f")
done < <(collect_logs "/var/log/swss/sairedis.rec*" "$NUM_FILES" "$CUTOFF_EPOCH")

if [[ ${#LOG_FILES[@]} -eq 0 ]]; then
    echo "No log files found." >&2
    echo "Searched:" >&2
    echo "  /var/log/syslog*" >&2
    echo "  /var/log/swss/swss.rec*" >&2
    echo "  /var/log/swss/sairedis.rec*" >&2
    if [[ -n "$SINCE" ]]; then
        echo "  (filtered to last $SINCE)" >&2
    fi
    exit 1
fi

echo "Loading ${#LOG_FILES[@]} log file(s):" >&2
for f in "${LOG_FILES[@]}"; do
    echo "  $f" >&2
done

exec scouty-tui ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"} "${LOG_FILES[@]}"
