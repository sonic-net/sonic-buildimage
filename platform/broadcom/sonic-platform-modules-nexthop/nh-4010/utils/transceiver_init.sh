#!/bin/bash

# Transceiver initialization script for NH-4010
# This script deasserts reset and disables low power mode for all OSFP transceivers

SYSLOG_IDENTIFIER="transceiver-init"
MAX_WAIT_TIME=30  # Maximum time to wait for transceiver control in seconds
XCVR_OSFP_I2C_BUSES_START=23  # Starting I2C bus number for OSFP transceivers
XCVR_OSFP_COUNT=64  # Number of OSFP transceivers

log_info() {
    logger -p info -t "${SYSLOG_IDENTIFIER}" "$@"
}

log_error() {
    logger -p err -t "${SYSLOG_IDENTIFIER}" "$@"
}

wait_for() {
    local function="$1"
    local max_wait_time="$2"
    local description="$3"
    local start_time=$(date +%s)
    local elapsed_time=0

    log_info "Waiting for ${description}."
    for ((elapsed_time=0; elapsed_time<max_wait_time; elapsed_time++)); do
        if "$function"; then
            return 0
        fi
        sleep 1
    done

    log_error "Timed out waiting for ${description}."
    return 1
}

is_xcvr_control_available() {
    local bus
    for ((i=0; i<XCVR_OSFP_COUNT; i++)); do
        bus=$((XCVR_OSFP_I2C_BUSES_START + i))
        if [ ! -f "/sys/bus/i2c/devices/${bus}-0008/xcvr_lpmode" ] || [ ! -f "/sys/bus/i2c/devices/${bus}-0008/xcvr_reset" ]; then
            return 1
        fi
    done
    return 0
}

init_xcvrs() {
    local bus
    local status=0
    log_info "Deasserting reset and disabling low power mode for all transceivers."
    for ((i=0; i<XCVR_OSFP_COUNT; i++)); do
        bus=$((XCVR_OSFP_I2C_BUSES_START + i))
        if ! echo 0 > "/sys/bus/i2c/devices/${bus}-0008/xcvr_reset"; then
            log_error "Failed to disable xcvr reset for bus ${bus}."
            status=1
            continue
        fi
        if ! echo 0 > "/sys/bus/i2c/devices/${bus}-0008/xcvr_lpmode"; then
            log_error "Failed to disable xcvr low power mode for bus ${bus}."
            status=1
            continue
        fi
    done
    return $status
}


log_info "Starting transceiver initialization"
wait_for is_xcvr_control_available "$MAX_WAIT_TIME" "transceiver control" || exit 1
init_xcvrs || exit 1
log_info "Transceiver initialization completed successfully"
exit 0
