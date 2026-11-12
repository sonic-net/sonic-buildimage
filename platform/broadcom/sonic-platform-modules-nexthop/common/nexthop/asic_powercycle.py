#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Mark an ASIC as intentionally down while asic_init.sh power cycles it.

--begin publishes PCIE_DETACH_INFO for each ASIC endpoint still on the bus so
pcied does not report the ASIC missing, and with --xcvr-cache snapshots xcvr
presence for Sfp.get_presence() to serve. --end clears both; call it from an
exit hook.

Usage:
    nh_asic_powercycle --begin [--xcvr-cache] | --end
"""

import argparse
import os
import subprocess
import sys
import syslog

from nexthop import pcie_lib
from nexthop.xcvr_presence_cache import (
    POWER_CYCLE_TTL_S,
    XCVR_PRESENCE_CACHE_FILE,
    snapshot_presence,
    write_presence_cache,
)

# Field names must match pcied's PCIE_DETACH_* constants.
PCIE_DETACH_TABLE = "PCIE_DETACH_INFO"
PCIE_DETACH_BUS_INFO_FIELD = "bus_info"
PCIE_DETACH_DEVICE_STATE_FIELD = "device_state"
DETACH_STATE = "detaching"

SONIC_DB_CLI = "sonic-db-cli"
DB_CLI_TIMEOUT_S = 5

# lspci elides domain 0; pcied writes bus_info with it.
PCI_DOMAIN = "0000"

# One script: HSET leaves an existing TTL alone, and a row that never expires
# would silence pcied for good.
SET_DETACHING_LUA = """
redis.call('HSET', KEYS[1], ARGV[1], ARGV[2], ARGV[3], ARGV[4])
redis.call('EXPIRE', KEYS[1], ARGV[5])
"""


def _pmon_can_observe_us() -> bool:
    """True once pddf-platform-init has finished, so pmon may be running."""
    return (
        subprocess.run(
            ["systemctl", "is-active", "--quiet", "pddf-platform-init.service"]
        ).returncode
        == 0
    )


def _asic_endpoints() -> list[str]:
    """Every ASIC endpoint pcie.yaml declares, as lspci renders it."""
    return pcie_lib.get_pcie_device_bdfs(device_type=pcie_lib.PcieDeviceType.ASIC)


def asic_bdfs() -> list[str]:
    """Every ASIC endpoint, domain-qualified as pcied writes it."""
    return [f"{PCI_DOMAIN}:{bdf}" for bdf in _asic_endpoints()]


def enumerated_asic_bdfs() -> list[str]:
    """asic_bdfs() minus any endpoint that is already off the bus.

    An endpoint that left before we did is pcied's to report.
    """
    bdfs = []
    for bdf in _asic_endpoints():
        try:
            present = pcie_lib.pci_device_present(bdf)
        except Exception as e:
            # An lspci failure is not evidence the ASIC went anywhere.
            syslog.syslog(syslog.LOG_WARNING, f"{bdf} presence unknown: {e}")
            present = True
        if present:
            bdfs.append(f"{PCI_DOMAIN}:{bdf}")
        else:
            syslog.syslog(
                syslog.LOG_NOTICE, f"{bdf} is already gone, leaving it to pcied"
            )
    return bdfs


def _db_cli(*args) -> None:
    subprocess.run(
        [SONIC_DB_CLI, "STATE_DB", *args],
        check=True,
        capture_output=True,
        timeout=DB_CLI_TIMEOUT_S,
    )


def set_detaching(bdfs) -> None:
    """Claim each endpoint so pcied stops probing it."""
    for bdf in bdfs:
        _db_cli(
            "EVAL",
            SET_DETACHING_LUA,
            "1",
            f"{PCIE_DETACH_TABLE}|{bdf}",
            PCIE_DETACH_BUS_INFO_FIELD,
            bdf,
            PCIE_DETACH_DEVICE_STATE_FIELD,
            DETACH_STATE,
            str(POWER_CYCLE_TTL_S),
        )
        syslog.syslog(syslog.LOG_INFO, f"{bdf} is {DETACH_STATE}")


def clear_detaching(bdfs) -> None:
    """Release each endpoint back to pcied."""
    for bdf in bdfs:
        _db_cli("DEL", f"{PCIE_DETACH_TABLE}|{bdf}")
        syslog.syslog(syslog.LOG_INFO, f"{bdf} detach cleared")


def _load_chassis():
    # Deferred so --begin without --xcvr-cache never imports the platform API.
    from sonic_platform.platform import Platform

    return Platform().get_chassis()


def write_xcvr_cache() -> None:
    """Snapshot xcvr presence to XCVR_PRESENCE_CACHE_FILE. Never raises."""
    try:
        n = write_presence_cache(
            XCVR_PRESENCE_CACHE_FILE, snapshot_presence(_load_chassis())
        )
    except Exception as e:
        syslog.syslog(
            syslog.LOG_WARNING,
            f"xcvr presence cache skipped, reads will not be suppressed during ASIC power cycle: {e}",
        )
        return

    syslog.syslog(syslog.LOG_INFO, f"xcvr presence cache written ({n} ports)")


def begin(xcvr_cache: bool) -> None:
    if not _pmon_can_observe_us():
        syslog.syslog(syslog.LOG_INFO, "pddf-platform-init not finished, nothing to suppress")
        return

    bdfs = enumerated_asic_bdfs()
    if not bdfs:
        syslog.syslog(syslog.LOG_NOTICE, "No ASIC endpoint to detach")
    else:
        try:
            set_detaching(bdfs)
        except Exception as e:
            syslog.syslog(syslog.LOG_WARNING, f"Could not publish ASIC detach: {e}")

    if xcvr_cache:
        write_xcvr_cache()


def end() -> None:
    XCVR_PRESENCE_CACHE_FILE.unlink(missing_ok=True)
    if not _pmon_can_observe_us():
        return

    try:
        clear_detaching(asic_bdfs())
    except Exception as e:
        # The rows expire anyway, so this only delays pcied noticing.
        syslog.syslog(syslog.LOG_WARNING, f"Could not clear ASIC detach: {e}")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--begin", action="store_true", help="the ASIC is going down")
    action.add_argument("--end", action="store_true", help="the ASIC is back")
    parser.add_argument(
        "--xcvr-cache",
        action="store_true",
        help="also snapshot xcvr presence (platforms that cycle the whole dataplane)",
    )
    args = parser.parse_args(argv)

    syslog.openlog(os.environ.get("LOG_TAG") or "nh_asic_powercycle")

    if args.begin:
        begin(args.xcvr_cache)
    else:
        end()
    return 0


if __name__ == "__main__":
    sys.exit(main())
