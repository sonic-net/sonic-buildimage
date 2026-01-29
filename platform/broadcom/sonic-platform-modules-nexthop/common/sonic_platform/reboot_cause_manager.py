# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Manager for retrieving, analyzing, and logging reboot causes.

Provides classes and helpers for retrieving and analyzing reboot causes
from multiple sources (such as DPMs and SW-written text files), and
logging the causes to a history file.
"""

import datetime
import os
import re

from dateutil import parser
from typing import Any
from sonic_platform.adm1266 import Adm1266
from sonic_platform.dpm_logger import DpmLogger
from sonic_platform.dpm_base import DpmBase, DpmPowerUpEntry, RebootCause, UNKNOWN_TIMESTAMP
from sonic_py_common import syslogger


SYSLOG_IDENTIFIER = "sonic_platform.reboot_cause_manager"
logger = syslogger.SysLogger(SYSLOG_IDENTIFIER)

UNKNOWN_HW_REBOOT_CAUSE = RebootCause(
    type=RebootCause.Type.HARDWARE,
    source="unknown",
    timestamp=UNKNOWN_TIMESTAMP,
    cause="unknown",
    description="unknown",
    chassis_reboot_cause_category="REBOOT_CAUSE_HARDWARE_OTHER",
)


def ordinal(n: int) -> str:
    """Returns the ordinal string of a number, e.g., "1st", "2nd", "3rd", "4th", etc."""
    # Handle special cases for 11th, 12th, 13th
    if 11 <= n % 100 <= 13:
        suffix = "th"
    else:
        suffix = {1: "st", 2: "nd", 3: "rd"}.get(n % 10, "th")
    return f"{n}{suffix}"


def fill_in_missing_and_align_powerups(
    dpm_to_powerups: dict[DpmBase, list[DpmPowerUpEntry]],
) -> tuple[dict[DpmBase, list[DpmPowerUpEntry | None]], int]:
    """Fills in missing powerups with None, so that all DPMs have the same number of powerups and are aligned.

    Args:
        dpm_to_powerups: Dictionary of DPM to list of powerups (must be sorted old->new).

    Returns:
         a tuple (dict, int):
            - Dictionary of DPM to list of powerups, with missing powerups filled in with None.
            - Number of powerups as experienced by the DPMs (i.e., number of reboots)
    """
    # Step 1: for each DPM, fill in the missing powerups between
    #         the oldest powerup and the current powerup.
    #         Also, trim the current powerup since it hasn't experienced
    #         a power cycle yet. Likely the records attached with the
    #         current powerup are not related to reboot.
    #
    # Example:
    #   Given:
    #     DPM1: [1, 2, 3, 4, 5, 6]; current_powerup = 6
    #     DPM2: [6, 8];             current_powerup = 10
    #   Result:
    #     DPM1: [1, 2, 3, 4, 5]
    #     DPM2: [6, None, 8, None]
    #     num_power_cycles = 5
    #   Reason:
    #     - DPM1 has records of 5 power cycles (with counter 1 to 5).
    #       We trim the last one since it is the current powerup.
    #     - DPM2 has records of 2 power cycles. But in fact, as hinted by
    #       the current_powerup = 10, it experienced 4 power cycles (6, 7, 8, 9).
    #       We fill in the missing ones with None.
    #     - The system actually experienced 5 power cycles, as evidenced by DPM1.
    ret: dict[DpmBase, list[DpmPowerUpEntry | None]] = {}
    num_power_cycles = 0
    for dpm, powerups in dpm_to_powerups.items():
        current = dpm.get_powerup_counter()
        max_powerup = dpm.max_powerup_counter()
        full_powerups: list[DpmPowerUpEntry | None] = []

        # Process powerups in reverse, from newest to oldest.
        p = current
        for powerup in reversed(powerups):
            # Skip the current powerup since it hasn't experienced a power cycle yet.
            if p == current:
                p = (p - 1) % max_powerup
                if powerup.powerup_counter == current:
                    continue
            # Prepend this powerup and the missing powerups.
            missing = (p - powerup.powerup_counter) % max_powerup
            full_powerups[:0] = [powerup] + ([None] * missing)
            p = (powerup.powerup_counter - 1) % max_powerup

        ret[dpm] = full_powerups
        num_power_cycles = max(num_power_cycles, len(full_powerups))

    # Step 2: Prepend the missing powerups, as we now know the total number of power cycles.
    #         After this, each DPM should have the same number of powerups and are aligned.
    #
    # Example:
    #   Given:
    #     DPM1: [1, 2, 3, 4, 5]
    #     DPM2: [6, None, 8, None]
    #     num_power_cycles = 5
    #   Result:
    #     DPM1: [1, 2, 3, 4, 5]
    #     DPM2: [None, 6, None, 8, None]
    for dpm, powerups in ret.items():
        missing = num_power_cycles - len(powerups)
        if missing > 0:
            ret[dpm] = ([None] * missing) + powerups

    return ret, num_power_cycles


def squash_dpms_powerups_to_cause_per_reboot(
    dpm_to_powerups: dict[DpmBase, list[DpmPowerUpEntry]],
) -> list[RebootCause]:
    """Squashes the powerup entries from all DPMs into one cause per reboot.

    The function first aligns the powerups among all DPMs, so each DPM has a full
    view of all previous power cycles. It may look something like:

        DPM1: [unknown, unknown, fault_1]
        DPM2: [fault_2, fault_3, fault_4]
        DPM3: [unknown, unknown, unknown]

    Notes:
      - Each column represents a power cycle experienced by the system.
      - "fault_x" indicates that the DPM realized a power fault cause on that powerup.
      - "unknown" indicates that a power fault cause cannot be realized by the DPM on
        that powerup. However, the power cycle has happened. It's either the DPM
        does not have the capability to detect the cause, or the DPM did not log anything.

    To determine the reboot cause for each powerup, we look at each column from top to bottom,
    and arbitrarily take the first known fault cause that we encounter. This works because HWs
    are generally designed to not log the same cause across DPMs. Except for CPUCARD-driven
    event where the SWITCHCARD also logs a record (as exampled in DPM2's fault_4).
    To handle this case, we place the CPUCARD's DPM before the SWITCHCARD's DPM in `pd-plugin.json`.

    For this example, we return `[fault_2, fault_3, fault_1]`.

    Args:
        dpm_to_powerups: Dictionary of DPM to list of powerups (must be sorted old->new).

    Returns:
        List of reboot causes, one per power cycle. The length of the list is the number of
        power cycles experienced by the system (as seen by the DPMs).
    """
    if not dpm_to_powerups:
        return []

    # First, align the powerups among all DPMs, from left to right (oldest to newest).
    dpm_to_all_previous_powerups, num_power_cycles = fill_in_missing_and_align_powerups(
        dpm_to_powerups
    )

    # Second, for each powerup (`p`), pick the first-seen DPM that reports a power fault.
    results = []
    for p in range(num_power_cycles):
        picked = next(
            (
                powerup.power_fault_cause
                for powerups in dpm_to_all_previous_powerups.values()
                if (powerup := powerups[p]) and powerup.power_fault_cause
            ),
            None,
        )
        if picked:
            results.append(picked)
        else:
            logger.log_warning(
                f"Cannot determine power fault cause for the {ordinal(num_power_cycles - p)} previous reboot."
            )
            results.append(UNKNOWN_HW_REBOOT_CAUSE)
    return results


def parse_timestamp(timestamp_str: str) -> datetime.datetime:
    """Parse a timestamp string into a datetime object."""
    try:
        return parser.parse(timestamp_str)
    except Exception as e:
        logger.log_warning(f"Failed to parse timestamp '{timestamp_str}': {e}")
        return UNKNOWN_TIMESTAMP


def merge_sw_and_hw_causes(
    sw_cause: RebootCause | None, hw_causes: list[RebootCause]
) -> list[RebootCause]:
    """Merges the SW cause and HW causes into a single list, sorted by timestamp.

    This is best effort as some HW causes may not have the UTC timestamp (DPM clocks
    are reset on every power down. It's not until the next I2C device creation
    (during pddf-platform-init) that the DPM's clock is set to the UNIX time. Any
    DPM records created between the power down and the I2C device creation will
    not have the UTC timestamp).
    """
    if not sw_cause:
        return hw_causes

    # Find a place to insert the SW cause, using best effort.
    # If not found, prepend SW cause to the front, as it's more common that
    # SW cause is leading to a HW cause.
    insert_idx = 0
    for i, hw_cause in enumerate(hw_causes):
        if (
            not isinstance(hw_cause.timestamp, datetime.datetime)
            or hw_cause == UNKNOWN_HW_REBOOT_CAUSE
        ):
            continue
        if hw_cause.timestamp < sw_cause.timestamp:  # type: ignore
            insert_idx = i + 1

    # Insert the SW cause.
    ret = hw_causes[:insert_idx] + [sw_cause] + hw_causes[insert_idx:]
    return ret


def squash_sw_and_hw_causes(sorted_causes: list[RebootCause]) -> list[RebootCause]:
    """Squahes the SW cause and HW cause if they refer to the same reboot."""

    def time_diff(cause1: RebootCause, cause2: RebootCause) -> datetime.timedelta | None:
        if not isinstance(cause1.timestamp, datetime.datetime):
            return None
        if not isinstance(cause2.timestamp, datetime.datetime):
            return None
        return cause2.timestamp - cause1.timestamp

    ret = []
    i = 0
    while i < len(sorted_causes):
        cause = sorted_causes[i]
        next_cause = sorted_causes[i + 1] if i + 1 < len(sorted_causes) else None

        ret.append(cause)

        # Nexthop's SW "reboot" is always followed by a power cycle within 3m (10s for the buffer).
        if (
            cause.type == RebootCause.Type.SOFTWARE
            and cause.cause == "reboot"
            and next_cause
            and next_cause.type == RebootCause.Type.HARDWARE
            and (delta := time_diff(cause, next_cause))
            and delta <= datetime.timedelta(minutes=3, seconds=10)
        ):
            i = i + 2  # Skip the next cause.
        else:
            i = i + 1
    return ret


class RebootCauseManager:
    def __init__(self, pddf_plugin_data: dict[str, Any]):
        self._pddf_plugin_data = pddf_plugin_data
        self._sw_reboot_cause_filepath = pddf_plugin_data.get("REBOOT_CAUSE", {}).get(
            "reboot_cause_file", None
        )

        self._dpm_devices: list[DpmBase] = []
        for dpm_name, platform_spec in pddf_plugin_data.get("DPM", {}).items():
            dpm_type = platform_spec["type"]
            if dpm_type == "adm1266":
                self._dpm_devices.append(Adm1266(dpm_name, platform_spec))
            else:
                logger.log_warning(f"Unknown DPM type '{dpm_type}' for '{dpm_name}' DPM.")

    def _read_powerups_from_all_dpms(self) -> dict[DpmBase, list[DpmPowerUpEntry]]:
        """Returns a dictionary of DPM to a list of powerups."""
        dpm_to_powerups: dict[DpmBase, list[DpmPowerUpEntry]] = {}
        for dpm in self._dpm_devices:
            dpm_to_powerups[dpm] = dpm.get_powerup_entries()
        return dpm_to_powerups

    def _clear_records_on_all_dpms(self):
        for dpm in self._dpm_devices:
            dpm.clear_records()

    def _read_sw_reboot_cause(self) -> RebootCause | None:
        if not self._sw_reboot_cause_filepath or not os.path.exists(self._sw_reboot_cause_filepath):
            return None

        with open(self._sw_reboot_cause_filepath, "r", errors="replace") as file:
            content = file.read().strip()
            if content.lower() == "unknown":
                return None

            # Parse the SW cause here, so we can summarize it in Nexthop's history log.
            if match := re.search(r"User issued '(.*)' command \[.*Time: (.*)\]", content):
                # Normally, it is from one of the reboot scripts, e.g. 'reboot', 'warm-reboot'.
                cause = match.group(1)
                timestamp = parse_timestamp(match.group(2))
            elif match := re.search(r"(Kernel Panic.*) \[.*Time: (.*)\]", content):
                cause = match.group(1)
                timestamp = parse_timestamp(match.group(2))
            else:
                cause = content
                timestamp = UNKNOWN_TIMESTAMP
            return RebootCause(
                type=RebootCause.Type.SOFTWARE,
                source="SW",
                timestamp=timestamp,
                cause=cause,
                description=content,
                chassis_reboot_cause_category="REBOOT_CAUSE_NON_HARDWARE",
            )

    def summarize_reboot_causes(self) -> list[RebootCause]:
        """Returns a list of reboot causes, one per reboot.

        Reboot causes are derived from the knowledge of DPM records (HW)
        and the reboot cause file (SW).
        """
        dpm_to_powerups = self._read_powerups_from_all_dpms()
        hw_reboot_causes = squash_dpms_powerups_to_cause_per_reboot(dpm_to_powerups)
        sw_reboot_cause = self._read_sw_reboot_cause()
        ret_causes = merge_sw_and_hw_causes(sw_reboot_cause, hw_reboot_causes)

        # Save all records to history.
        DpmLogger().save(ret_causes, dpm_to_powerups)

        self._clear_records_on_all_dpms()
        return squash_sw_and_hw_causes(ret_causes)
