#!/bin/bash
#
# console_connect.sh
#
# nokia c1-specific console session handler. Launched via os.execvp()
# from consutil/lib.py after Python has resolved the target, validated
# config, and built the final picocom command.
#
# Usage: console_connect.sh <line_num> <escape_display> <picocom_cmd>
#
# Arguments (all supplied by Python):
#   line_num       - console line number
#   escape_display - uppercase escape char for the banner (e.g. "A")
#   picocom_cmd    - fully-formed picocom command string

set -u

LINE_NUM="${1:?missing line_num}"
ESCAPE_DISPLAY="${2:?missing escape_display}"
PICOCOM_CMD="${3:?missing picocom_cmd}"

console_state_set() {
    local ln="$1" st="$2" pid="${3:-}" start="${4:-}"
    local key="CONSOLE_PORT|${ln}"
    sonic-db-cli STATE_DB hset "$key" state "$st" > /dev/null 2>&1 || true
    sonic-db-cli STATE_DB hset "$key" pid "$pid" > /dev/null 2>&1 || true
    sonic-db-cli STATE_DB hset "$key" start_time "$start" > /dev/null 2>&1 || true
}

picocom_pid=""

forward_signal() {
    if [ -n "$picocom_pid" ]; then
        kill -TERM "$picocom_pid" > /dev/null 2>&1 || true
    fi
}

cleanup() {
    if [ -n "$picocom_pid" ]; then
        wait "$picocom_pid" 2>/dev/null || true
    fi
    console_state_set "$LINE_NUM" idle "" ""
}

trap cleanup EXIT
trap forward_signal HUP INT TERM

printf 'Successful connection to line [%s]\nPress ^%s ^X to disconnect\n' "$LINE_NUM" "$ESCAPE_DISPLAY"
bash -c "$PICOCOM_CMD" < /dev/tty > /dev/tty 2>&1 &
picocom_pid=$!
console_state_set "$LINE_NUM" busy "$picocom_pid" "$(ps -p "$picocom_pid" -o lstart= | sed 's/^ *//')"
wait "$picocom_pid"
rc=$?
stty sane 2>/dev/null
printf '\nTerminating...\nThanks for using picocom\n'
exit "$rc"
