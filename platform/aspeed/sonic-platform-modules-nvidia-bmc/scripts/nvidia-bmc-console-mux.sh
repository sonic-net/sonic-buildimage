#!/bin/sh
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Drive the CPLD console mux around a console-monitor proxy instance.
#
# This board has no dedicated BMC->host UART. The switch CPU console is reached
# over the BMC's own console UART (ttyS12) only while the CPLD mux `uart_sel`
# is 3; ttyS1 is not wired to the host at any value. `uart_sel` is a 2-bit field
# (bits 7:6 of CPLD GP1 reg 0x2530) exposed by mlxreg-io.
#
#   0  BMC owns the console                     <- detach target, reset default
#   1  BMC receives host console, receive only
#   2  chassis console connector <- host CPU; BMC not in the path
#   3  host console also reaches BMC ttyS12, full duplex   <- attach target
#
# NOTE: the Serial Console Auto-Switch HLD defines only 0 and 2 and states that
# other values "shall not be relied on". Value 3 is nonetheless the only setting
# that puts the host console on the BMC -- measured, and used by NVIDIA's own
# OpenBMC SoL unit. This conflict is open with the architecture owner.
#
# Detach always returns the console to the BMC rather than restoring a
# remembered value. hw-management writes uart_sel from the power paths (host
# power-off routes it to the BMC), so a value sampled at attach time is stale by
# detach time; restoring it can point the console at a powered-off CPU, which
# the Auto-Switch HLD forbids. 0 is both the reset default and the state the
# platform returns to whenever the host is taken down, so it is the only correct
# detach target.
#
# hw-management owns uart_sel on the power paths and never writes 3: the
# power-off paths write 0, and only a physical power-button-on writes 2 --
# power_on() writes nothing at all. A host power transition therefore moves the
# mux out from under a live proxy, and nothing puts it back. The move is
# invisible to the proxy: the mux sits outside the SoC UART, so re-routing it
# raises no error and closes no descriptor (measured). `reconcile' closes that
# gap: one pass that puts the mux back to 3 if something moved it. It is run by
# nvidia-bmc-console-mux-reconcile@.path on every write to uart_sel or pwr_down,
# so the register (an I2C read) is only touched when something has changed.
#
# Usage: nvidia-bmc-console-mux.sh {attach|detach|reconcile} <line_id>

set -eu

UART_SEL=/run/hw-management/system/uart_sel
PWR_DOWN=/run/hw-management/system/pwr_down
MUXED_LINE=0
GETTY=serial-getty@ttyS12.service
ATTACH_VALUE=3
DETACH_VALUE=0

action=${1:-}
line=${2:-}

log() { logger -t nvidia-bmc-console-mux -p daemon.info "$*"; echo "$*"; }

# Only line 0 rides the muxed UART. Any other instance is left alone rather than
# yanking the mux, or the BMC getty, out from under it.
[ "$line" = "$MUXED_LINE" ] || exit 0

# Not a mux-equipped board (or hw-management not up yet): leave the proxy to run
# against whatever the udev rule named, rather than failing its start.
mux_present() {
    [ -w "$UART_SEL" ] && return 0
    log "uart_sel not writable at $UART_SEL; leaving console mux untouched"
    return 1
}

case "$action" in
attach)
    # The getty holds the tty open and eats the bytes; an immediate open also
    # races its teardown and reads EOF. Stop it, then settle.
    systemctl stop "$GETTY" || log "WARNING: failed to stop $GETTY"
    sleep 1
    mux_present || exit 0
    # Do not route the console to a powered-off CPU, but do not fail the proxy
    # start either: the DCE does not retry a failed start, so the line would stay
    # dead after the host powers on. Let the proxy run with the mux untouched;
    # the reconciler routes it on the pwr_down=0 write at power-on.
    if [ -r "$PWR_DOWN" ] && [ "$(cat "$PWR_DOWN" 2>/dev/null)" = "1" ]; then
        log "console mux: switch host is powered off (pwr_down=1); will route the console when it powers on"
        exit 0
    fi
    cur=$(cat "$UART_SEL" 2>/dev/null) || cur="?"
    if ! printf '%s\n' "$ATTACH_VALUE" > "$UART_SEL"; then
        log "ERROR: failed to set uart_sel=$ATTACH_VALUE"
        exit 1
    fi
    log "console mux: uart_sel $cur -> $ATTACH_VALUE (host console on ttyS12)"
    ;;
detach)
    if mux_present; then
        cur=$(cat "$UART_SEL" 2>/dev/null) || cur="?"
        if printf '%s\n' "$DETACH_VALUE" > "$UART_SEL"; then
            log "console mux: uart_sel $cur -> $DETACH_VALUE (console returned to the BMC)"
        else
            log "ERROR: failed to set uart_sel=$DETACH_VALUE"
        fi
    fi
    systemctl start "$GETTY" || log "WARNING: failed to start $GETTY"
    ;;
reconcile)
    mux_present || exit 0
    # Single pass, triggered by writes to uart_sel and pwr_down. Both are needed:
    # a power-off writes uart_sel=0 while pwr_down=1, so that pass must leave
    # the register alone, and power_on() then writes pwr_down=0 but never
    # touches uart_sel -- the pwr_down write is the only event that marks the
    # host coming back.
    if [ -r "$PWR_DOWN" ] && [ "$(cat "$PWR_DOWN" 2>/dev/null)" = "1" ]; then
        exit 0 # Host is down. Same rule as attach: do not route to a dead CPU.
    fi
    cur=$(cat "$UART_SEL" 2>/dev/null) || cur=""
    if [ -n "$cur" ] && [ "$cur" != "$ATTACH_VALUE" ]; then
        if printf '%s\n' "$ATTACH_VALUE" > "$UART_SEL"; then
            log "console mux: corrected uart_sel $cur -> $ATTACH_VALUE (moved by another writer)"
        else
            log "ERROR: failed to correct uart_sel to $ATTACH_VALUE"
            exit 1
        fi
    fi
    ;;
*)
    echo "usage: $0 {attach|detach|reconcile} <line_id>" >&2
    exit 2
    ;;
esac
