#!/bin/sh
# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Cold-boot initializer for reset-tolerant GPIOs.
#
# Reset tolerance (per-pin control register bit6) keeps the direction (bit1) and
# output data (bit0) bits from being cleared by a WDT_SOC/EXTRSTN reset, so an
# output can hold its level across a BMC reboot. It does not stop software from
# rewriting the pin after the reset, which is why these lines must not be
# gpio-hogged in the device tree: a hog is applied unconditionally when the
# gpiochip probes and would clobber the level that survived the reset.
#
# So apply the power-on default only when this boot was not a watchdog-caused
# reset; on a warm reset re-drive the level the pin came up with, which is a
# no-op on the pad but re-arms reset tolerance and re-muxes the pad to GPIO for
# the next reset.
#
# Boot cause comes from the watchdog bootstatus, which the aspeed_wdt driver
# derives from the SCU WDT reset log. Note that U-Boot clears the power-on and
# EXTRST flags before Linux runs, so a reset that is not attributable to a WDT
# is treated as a cold boot here.
#
# The per-pin control register is passed in rather than derived: libgpiod v2
# dropped gpiofind, and gpioinfo's output is not a stable parsing target. For
# AST2700 the address is <controller base> + 0x180 + <line offset> * 4, e.g.
# cpe_ctrl is GPIOE2, offset 34 on gpio1 (0x14c0b000), giving
# 0x14c0b000 + 0x180 + 34 * 4 = 0x14c0b208.
#
# Usage: gpio-cold-init.sh <gpio-line-name> <ctrl-reg> <cold-boot-value>

set -u

if [ $# -ne 3 ]; then
    echo "usage: $(basename "$0") <gpio-line-name> <ctrl-reg> <cold-boot-value>" >&2
    exit 1
fi

LINE=$1
CTRL_REG=$2
COLD_VALUE=$3

WDIOF_CARDRESET=32      # 0x0020, linux/watchdog.h: card previously reset the CPU
CTRL_DATA=1             # per-pin control bit0: output data
CTRL_TOLERANT_OUT=66    # bit6 reset tolerance | bit1 direction=output

log() {
    echo "gpio-cold-init[${LINE}]: $*"
}

wdt_reset() {
    for f in /sys/class/watchdog/*/bootstatus; do
        [ -r "${f}" ] || continue
        if [ $(( $(cat "${f}" 2>/dev/null || echo 0) & WDIOF_CARDRESET )) -ne 0 ]; then
            return 0
        fi
    done
    return 1
}

if ! before=$(busybox devmem "${CTRL_REG}" 32); then
    log "cannot read ${CTRL_REG}"
    exit 1
fi

if ! wdt_reset; then
    value=${COLD_VALUE}
    log "cold boot (no WDT reset): applying default ${value}, ${CTRL_REG}=${before}"
elif [ $(( before & CTRL_TOLERANT_OUT )) -ne "${CTRL_TOLERANT_OUT}" ]; then
    # Tolerance and/or direction did not survive, so the data bit is not
    # meaningful either. Fall back to the default rather than drive a guess.
    value=${COLD_VALUE}
    log "warm reset but ${CTRL_REG}=${before} is not a tolerant output: applying default ${value}"
else
    value=$(( before & CTRL_DATA ))
    log "warm reset: preserving level ${value}, ${CTRL_REG}=${before}"
fi

# libgpiod v2 addresses lines by name, so neither gpiofind (removed in v2) nor a
# chip/offset lookup is needed. Requesting the line muxes the pad to GPIO
# (pinctrl gpio_request_enable) and arms reset tolerance (gpiolib requests
# PIN_CONFIG_PERSIST_STATE for a non-transitory line); gpioset then drives the
# level and exits. The aspeed pinctrl implements no gpio_disable_free, so
# releasing the line does not undo the mux and the pin keeps driving.
if ! gpioset --strict --consumer gpio-cold-init "${LINE}=${value}"; then
    log "gpioset failed for ${LINE}=${value}"
    exit 1
fi

log "${CTRL_REG}: ${before} -> $(busybox devmem "${CTRL_REG}" 32)"
