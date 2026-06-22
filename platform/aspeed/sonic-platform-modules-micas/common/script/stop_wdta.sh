#!/bin/sh

set -u

REG_ADDR="0x14C3740c"
REG_WIDTH="32"
REG_VALUE="0x00"
LOG_TAG="m2-w6950-128oc-stop-watchdog"
MAX_RETRIES=3

log_info() {
    logger -t "${LOG_TAG}" "$1"
}

log_warn() {
    logger -p user.warning -t "${LOG_TAG}" "$1"
}

run_devmem() {
    if command -v busybox >/dev/null 2>&1; then
        busybox devmem "${REG_ADDR}" "${REG_WIDTH}" "${REG_VALUE}"
        return $?
    fi

    if [ -x /bin/busybox ]; then
        /bin/busybox devmem "${REG_ADDR}" "${REG_WIDTH}" "${REG_VALUE}"
        return $?
    fi

    if command -v devmem >/dev/null 2>&1; then
        devmem "${REG_ADDR}" "${REG_WIDTH}" "${REG_VALUE}"
        return $?
    fi

    return 127
}

log_info "Starting watchdog stop sequence for ${REG_ADDR}"

attempt=1
while [ "${attempt}" -le "${MAX_RETRIES}" ]; do
    if run_devmem >/dev/null 2>&1; then
        log_info "Stopped watchdog successfully on attempt ${attempt}"
        exit 0
    fi

    log_warn "Failed to stop watchdog on attempt ${attempt}"
    attempt=$((attempt + 1))
    sleep 1
done

log_warn "All attempts to stop watchdog failed; continuing boot for temporary delivery policy"
exit 0
