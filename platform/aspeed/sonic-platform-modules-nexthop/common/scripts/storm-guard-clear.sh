#!/bin/bash
# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Reboot-storm guard: clear the consecutive-WDT-reset counter once SONiC has
# reached steady state (past the last boot stage). U-Boot increments this
# counter on each WDT0-caused reset and halts autoboot at the limit, so a
# successful boot must reset it for the limit to count only consecutive
# failures.

# Counter location shared with the Nexthop U-Boot 'nh_stormguard' command: a
# word in AST2700 on-chip SRAM that survives a WDT SoC reset. Must match the
# address compiled into U-Boot.
STORMGUARD_COUNT=0x14C021A0

if ! busybox devmem "$STORMGUARD_COUNT" 32 0; then
    echo "storm-guard-clear: failed to clear reboot-storm counter at $STORMGUARD_COUNT" >&2
    exit 1
fi
