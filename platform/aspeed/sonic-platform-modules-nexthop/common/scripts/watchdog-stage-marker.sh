#!/bin/bash
# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Boot stage attribution: write a one-byte boot-stage marker to the
# WDT4C scratch register (survives the SoC reset). U-Boot reads it on the next
# boot to attribute a WDT-caused reset to a boot stage.
#
# Writes are monotonic: the marker is only advanced, never moved backwards.
# hw-watchdog-mgrd starts early (After=local-fs.target, to adopt the u-boot
# watchdog) and writes its "daemon armed" marker on its first pet, which
# can land before the lower-valued stage units run.
#
# Usage: watchdog-stage-marker.sh <value>   e.g. 0x22

# Must match "watchdog.stage_scratch_reg" in the platform.json of every board
# this package ships for, which is what hw-watchdog-mgrd uses for its markers.
WDT_STAGE_SCRATCH="0x14c3704c"
WDT_STAGE_SCRATCH_MAGIC_CLEAR=0xEA000000

new="$1"
cur="$(busybox devmem "$WDT_STAGE_SCRATCH" 32 2>/dev/null)"
case "$cur" in
    0x*|0X*)
        if [ "$((cur))" -ge "$((new))" ] 2>/dev/null; then
            exit 0
        fi
        ;;
esac

# WDT4C scratch is set-only (bits are write-1-to-set); clearing requires the
# magic value, so clear-then-set to make the register read exactly $new.
busybox devmem "$WDT_STAGE_SCRATCH" 32 "$WDT_STAGE_SCRATCH_MAGIC_CLEAR" 2>/dev/null || true
busybox devmem "$WDT_STAGE_SCRATCH" 32 "$new" 2>/dev/null || true
