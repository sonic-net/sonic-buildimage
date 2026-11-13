#!/bin/bash
# Disarm AST2720 WDTA boot watchdog for ABR support
# Clears bit 0 of WDTA_CTRL_REG (0x14c3740c) when ABR is enabled via OTP

set -e

WDTA_CTRL_REG=0x14c3740c

cur_val=$(busybox devmem "$WDTA_CTRL_REG" 2>/dev/null)

new_val=$(printf "0x%08X" $(( cur_val & 0xFFFFFFFE )))
busybox devmem "$WDTA_CTRL_REG" 32 "$new_val" 2>/dev/null

echo "aspeed-disable-boot-watchdog: WDTA ctrl: $cur_val -> $new_val"
