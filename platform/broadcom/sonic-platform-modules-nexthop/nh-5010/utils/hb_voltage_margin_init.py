#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""nh-5010 boot-time core-VDDC margining.

On boards with IDPROM Device Version (ONIE TLV 0x26) == 0 the core-VDDC rails
are margined low at boot, before syncd brings up the datapath.  The rail set and
their absolute targets come from the platform's voltage_margin_config.json.

Strictly gated: if the device version cannot be read, or is not 0, no rail is
touched (fail-safe).  Best-effort: any margining failure is logged to syslog but
the script always exits 0 so a margining miss can never block boot.
"""

import sys
import syslog

from nexthop import voltage_margin as vm

TAG = "nh_5010_voltage_margin_init"


def _log(msg):
    print(msg, flush=True)
    syslog.syslog(syslog.LOG_INFO, msg)


def _read_device_version():
    """IDPROM Device Version (TLV 0x26) as an int, or None if it can't be read.
    Read failure is deliberately distinct from a value -- the caller treats None
    as 'do not margin'."""
    try:
        from sonic_platform.platform import Platform

        raw = Platform().get_chassis().get_eeprom().eeprom_tlv_dict.get("0x26")
        if raw is None:
            return None
        return int(str(raw).strip())
    except Exception as e:
        _log("could not read IDPROM device version: %s" % e)
        return None


def main():
    syslog.openlog(TAG, syslog.LOG_PID, syslog.LOG_USER)
    dry_run = "--dry-run" in sys.argv[1:]

    dv = _read_device_version()
    if dv is None:
        _log("device version unavailable -- not margining (fail-safe)")
        return 0
    if dv != 0:
        _log("device version %d != 0 -- no boot-time margining required" % dv)
        return 0

    _log("device version 0 -- margining configured core-VDDC rails low")
    try:
        rails = vm.load_config()
    except vm.VoltageMarginError as e:
        _log("no voltage-margin config -- nothing to do: %s" % e)
        return 0

    for name in sorted(rails):
        try:
            vm.set_rail(name, vm.get_rail(rails, name), vm.LEVEL_LOW, dry_run=dry_run)
            _log("margined %s low%s" % (name, " [dry-run]" if dry_run else ""))
        except vm.VoltageMarginError as e:
            _log("! %s: failed to margin low: %s" % (name, e))

    return 0


if __name__ == "__main__":
    sys.exit(main())
