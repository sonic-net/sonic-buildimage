#!/bin/bash
#
# Watchdog keepalive daemon for Aspeed platforms.
#
# This daemon keeps the hardware watchdog alive by sending periodic keepalive
# signals. It opens /dev/watchdog and writes 'w' character every 60 seconds
# (with a 180 second watchdog timeout).
#

WATCHDOG_DEVICE="/dev/watchdog0"
KEEPALIVE_INTERVAL=60  # seconds
LOG_DIR="/host/bmc"
LOG_FILE="${LOG_DIR}/watchdog.log"
MAX_RETAINED_LOGS=5
KEEPALIVE_LOG_INTERVAL=3600  # Log keepalive status every hour (3600 seconds)

# Ensure log directory exists
if [ ! -d "$LOG_DIR" ]; then
    mkdir -p "$LOG_DIR" || {
        echo "ERROR: Failed to create log directory: $LOG_DIR" >&2
        exit 1
    }
fi

# Rotate old logs
rotate_logs() {
    ls -t ${LOG_DIR}/watchdog*.log 2>/dev/null | cat -n | while read n f; do
        if [ $n -gt $MAX_RETAINED_LOGS ]; then
            rm -f "$f"
        fi
    done
}

# Signal handler for graceful shutdown
cleanup() {
    echo "$(date -u): Watchdog keepalive daemon stopping" >> "$LOG_FILE"

    # Disarm the watchdog using watchdogutil
    if command -v watchdogutil &> /dev/null; then
        watchdogutil disarm >> "$LOG_FILE" 2>&1 || {
            echo "$(date -u): WARNING: Failed to disarm watchdog - system may reboot" >> "$LOG_FILE"
        }
    else
        echo "$(date -u): WARNING: watchdogutil not found - cannot disarm watchdog" >> "$LOG_FILE"
    fi

    # Close the file descriptor
    exec {wdt_fd}>&- 2>/dev/null || true

    echo "$(date -u): Watchdog keepalive daemon stopped" >> "$LOG_FILE"
    exit 0
}

trap 'cleanup' SIGTERM SIGINT

# Rotate logs
rotate_logs

# Start logging
echo "$(date -u): Watchdog keepalive daemon starting" > "$LOG_FILE"

# Set watchdog interval to 180 seconds
# Set watchdog timeout to 180 seconds
if command -v watchdogutil &> /dev/null; then
    watchdogutil arm -s 180 >> "$LOG_FILE" 2>&1
    echo "$(date -u): Watchdog timeout set to 180 seconds" >> "$LOG_FILE"
else
    echo "$(date -u): WARNING: watchdogutil not found, using default timeout" >> "$LOG_FILE"
fi

# Open watchdog device and assign FD to variable wdt_fd
exec {wdt_fd}>"$WATCHDOG_DEVICE"
if [ $? -ne 0 ]; then
    echo "$(date -u): ERROR: Failed to open $WATCHDOG_DEVICE" >> "$LOG_FILE"
    exit 1
fi

echo "$(date -u): Opened $WATCHDOG_DEVICE with FD $wdt_fd" >> "$LOG_FILE"

# Send first keepalive
echo "w" >&${wdt_fd}
echo "$(date -u): First watchdog keepalive sent" >> "$LOG_FILE"

# Main keepalive loop with reduced logging
keepalive_count=0
log_threshold=$((KEEPALIVE_LOG_INTERVAL / KEEPALIVE_INTERVAL))

while true; do
    sleep $KEEPALIVE_INTERVAL
    echo "w" >&${wdt_fd}

    # Only log periodically (every hour by default) to reduce eMMC writes
    keepalive_count=$((keepalive_count + 1))
    if [ $((keepalive_count % log_threshold)) -eq 0 ]; then
        echo "$(date -u): Watchdog keepalive active (sent $keepalive_count keepalives)" >> "$LOG_FILE"
    fi
done

