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
#
# Exit status: 0 on success or when the marker is already at or past <value>,
# 2 on a usage error. A failed register write is reported but is not an error:
# attribution is best-effort and must never fail a boot stage.

set -euo pipefail

# The only SONiC-side copy of these: hw-watchdog-mgrd writes its markers by
# running this script ("watchdog.stage_marker_cmd" in platform.json) rather
# than touching the register itself. Must match the Nexthop U-Boot.
WDT_STAGE_SCRATCH="0x14c3704c"
WDT_STAGE_SCRATCH_MAGIC_CLEAR=0xEA000000

# Exactly one 32-bit hex value. Without this check a missing or malformed
# argument evaluates to 0 below and, when the register read fails, the marker
# is magic-cleared and then overwritten with garbage.
if [ $# -ne 1 ] || [[ ! "$1" =~ ^0[xX][0-9a-fA-F]{1,8}$ ]]; then
    echo "Usage: $0 <hex value>   e.g. 0x22" >&2
    exit 2
fi
new="$1"

# An unreadable register (devmem failure or non-hex output) is treated as
# unknown and the marker is written.
cur="$(busybox devmem "$WDT_STAGE_SCRATCH" 32 2>/dev/null)" || cur=""
if [[ "$cur" =~ ^0[xX][0-9a-fA-F]+$ ]] && [ "$((cur))" -ge "$((new))" ]; then
    exit 0
fi

# WDT4C scratch is set-only (bits are write-1-to-set); clearing requires the
# magic value, so clear-then-set to make the register read exactly $new.
if ! busybox devmem "$WDT_STAGE_SCRATCH" 32 "$WDT_STAGE_SCRATCH_MAGIC_CLEAR" 2>/dev/null; then
    echo "watchdog-stage-marker: failed to clear $WDT_STAGE_SCRATCH; marker $new not written" >&2
    exit 0
fi
if ! busybox devmem "$WDT_STAGE_SCRATCH" 32 "$new" 2>/dev/null; then
    echo "watchdog-stage-marker: failed to write marker $new to $WDT_STAGE_SCRATCH" >&2
fi
exit 0
