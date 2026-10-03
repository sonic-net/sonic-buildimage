#!/usr/bin/env python3
#
# BMC accessor.
#
# Communicates with the Baseboard Management Controller over
# IPMI (KCS, /dev/ipmi0).  This module provides a BMC
# object that satisfies the interface used by /usr/local/bin/bmc_techsupport.py
# (invoked by generate_dump / "show techsupport"):
#
#     trigger_bmc_debug_log_dump() -> (ret, (task_id, err_msg))
#     get_bmc_debug_log_dump(task_id, filename, path, timeout)
#                                  -> (ret, err_msg)
#
# The debug dump is produced by collecting BMC diagnostics with ipmitool
# (controller info, System Event Log, sensor records, FRU inventory and the
# watchdog state) into a compressed tar archive.

import logging
import os
import subprocess
import tarfile
import tempfile

logger = logging.getLogger(__name__)


class CienaBmc:
    """IPMI-over-KCS accessor for the Ciena BMC."""

    IPMITOOL = "/usr/bin/ipmitool"
    TASK_ID = "ciena-ipmi-bmc-dump"
    INVALID_TASK_ID = "-1"

    # filename inside the archive -> ipmitool argument vector
    _COLLECTORS = {
        "mc_info.txt": ["mc", "info"],
        "mc_watchdog.txt": ["mc", "watchdog", "get"],
        "sel_info.txt": ["sel", "info"],
        "sel_elist.txt": ["sel", "elist"],
        "sdr_elist.txt": ["sdr", "elist"],
        "fru.txt": ["fru", "print"],
        "chassis_status.txt": ["chassis", "status"],
    }

    def _ipmitool(self, args, timeout=30):
        """Run an ipmitool sub-command; return (returncode, combined_output)."""
        try:
            proc = subprocess.run(
                [self.IPMITOOL] + args,
                capture_output=True, text=True, timeout=timeout,
            )
            return proc.returncode, (proc.stdout or "") + (proc.stderr or "")
        except (OSError, subprocess.SubprocessError) as exc:
            return 1, str(exc)

    # Board supply rails whose collapse toward 0V uniquely indicates that INPUT
    # power was physically removed.  The BMC standby/RTC rail (P3V3_BMC_BATT)
    # only browns out when input power is actually lost -- a software reboot
    # leaves it powered and logs no SEL rail events at all -- so it is the
    # authoritative "power was physically removed" discriminator, immune to
    # transient runtime threshold crossings on ordinary payload rails.
    _STANDBY_RAIL_HINTS = ("BMC_BATT",)

    @staticmethod
    def _parse_sel_epoch(date, tstamp):
        """Parse SEL 'MM/DD/YYYY' + 'HH:MM:SS AM/PM UTC' -> UTC epoch int, or None.

        Only UTC-labelled timestamps are accepted (the BMC logs UTC) and the
        result is range-checked to reject an unset/insane BMC clock (e.g. a
        year-2000 default or a far-future value).  Never raises.
        """
        import calendar
        import time as _time
        try:
            toks = tstamp.split()
            # Expect "HH:MM:SS", "AM"|"PM", "UTC".
            if len(toks) < 3 or toks[-1].upper() != "UTC":
                return None
            st = _time.strptime(
                "{} {} {}".format(date, toks[0], toks[1]),
                "%m/%d/%Y %I:%M:%S %p")
            epoch = calendar.timegm(st)
        except (ValueError, OverflowError, TypeError):
            return None
        # 2015-01-01 .. now+1d: reject an unset/garbage or far-future BMC clock.
        if epoch < 1420070400 or epoch > int(_time.time()) + 86400:
            return None
        return epoch

    def get_last_power_event(self, timeout=15):
        """Return info about the most recent BMC-observed hard power event, or None.

        The BMC is standby/battery powered and records supply-rail threshold
        crossings in its System Event Log even across a power cycle.  A true
        loss of input power collapses the board rails -- including the BMC's own
        standby rail (P3V3_BMC_BATT) -- toward 0V ("Voltage <rail> ... going
        low ... Asserted"); when power is restored the rails recover
        ("Deasserted").  A software reboot leaves the BMC powered and produces
        NO such SEL burst, so the presence of a standby-rail collapse is the
        discriminator between a real power cycle and a software reboot.

        Returns a dict, or None if no standby-rail collapse is recorded:
            {
              'loss_str':      BMC-formatted power-loss time string, verbatim
                               (e.g. "08/31/2026 08:58:53 PM UTC");
              'loss_epoch':    int UTC epoch of the power loss, or None if the
                               BMC timestamp could not be parsed/validated;
              'restore_epoch': int UTC epoch power was restored (latest event of
                               the recovery burst; used to correlate the event
                               with a specific boot), or None.
            }
        Never raises.
        """
        ret, out = self._ipmitool(["sel", "elist"], timeout=timeout)
        if ret != 0 or not out.strip():
            return None

        rows = []
        for line in out.splitlines():
            parts = [p.strip() for p in line.split("|")]
            if len(parts) < 6:
                continue
            date, tstamp, sensor, event, state = (
                parts[1], parts[2], parts[3], parts[4], parts[5])
            if "voltage" not in sensor.lower():
                continue
            if "/" not in date or ":" not in tstamp:
                continue
            rows.append({
                "sensor": sensor,
                "event": event.lower(),
                "state": state.lower(),
                "epoch": self._parse_sel_epoch(date, tstamp),
                "raw_str": "{} {}".format(date, tstamp),
            })
        if not rows:
            return None

        # Most-recent standby-rail collapse == the true power-loss instant.
        # ipmitool prints oldest->newest, so scan from the end.
        loss_idx = None
        for i in range(len(rows) - 1, -1, -1):
            r = rows[i]
            if (any(h.lower() in r["sensor"].lower()
                    for h in self._STANDBY_RAIL_HINTS)
                    and "going low" in r["event"]
                    and r["state"] == "asserted"):
                loss_idx = i
                break
        if loss_idx is None:
            return None

        loss = rows[loss_idx]
        # Recovery-burst tail: the latest voltage-event timestamp at/after the
        # loss.  After a real power loss the board is off, so nothing else is
        # logged until power is restored -- these trailing events mark power's
        # return (and, for a long outage, are far later than the collapse).
        restore_epoch = loss["epoch"]
        for r in rows[loss_idx:]:
            if r["epoch"] is not None and (
                    restore_epoch is None or r["epoch"] >= restore_epoch):
                restore_epoch = r["epoch"]

        return {
            "loss_str": loss["raw_str"],
            "loss_epoch": loss["epoch"],
            "restore_epoch": restore_epoch,
        }

    def get_last_power_loss_time(self, timeout=15):
        """Backward-compatible accessor: verbatim last power-loss time, or None."""
        ev = self.get_last_power_event(timeout=timeout)
        return ev["loss_str"] if ev else None

    # SEL watchdog "action taken" strings that correspond to a host reset/power
    # event (as opposed to a mere pre-timeout interrupt, which does not reboot).
    _WATCHDOG_ACTION_HINTS = ("hard reset", "power cycle", "power down",
                              "power off", "reset")

    def get_last_watchdog_event(self, timeout=15):
        """Return the most recent BMC watchdog reset event, or None.

        The volatile IPMI 'Timer Use Expiration flags' (Get Watchdog Timer,
        App 0x25 byte[3]) are NOT reliable on this platform: the early-init
        code re-arms the BMC watchdog on every boot, and this BMC clears the
        latched flags on any Set Watchdog Timer, so the flags are already zero
        by the time reboot-cause runs.

        The BMC System Event Log, however, records a persistent watchdog event
        when the timer expires and resets the host, e.g.:

            <id> | MM/DD/YYYY | HH:MM:SS AM/PM UTC | Watchdog2 watchdog_host0 \
                 | Hard reset | Asserted

        This survives the reset and the boot-time re-arm, so it is the reliable
        watchdog-reset discriminator.

        Returns a dict, or None if no watchdog reset event is recorded:
            {
              'action_str':  the action the BMC took, verbatim (e.g. "Hard reset");
              'event_epoch': int UTC epoch of the watchdog event, or None if the
                             BMC timestamp could not be parsed/validated;
              'raw_str':     the verbatim "MM/DD/YYYY HH:MM:SS AM/PM UTC" string.
            }
        Never raises.
        """
        ret, out = self._ipmitool(["sel", "elist"], timeout=timeout)
        if ret != 0 or not out.strip():
            return None

        latest = None
        # ipmitool prints oldest->newest; scan from the end for the most recent.
        for line in reversed(out.splitlines()):
            parts = [p.strip() for p in line.split("|")]
            if len(parts) < 6:
                continue
            date, tstamp, sensor, event, state = (
                parts[1], parts[2], parts[3], parts[4], parts[5])
            if "watchdog" not in sensor.lower():
                continue
            if state.lower() != "asserted":
                continue
            if not any(h in event.lower() for h in self._WATCHDOG_ACTION_HINTS):
                continue
            if "/" not in date or ":" not in tstamp:
                continue
            latest = {
                "action_str": event,
                "event_epoch": self._parse_sel_epoch(date, tstamp),
                "raw_str": "{} {}".format(date, tstamp),
            }
            break
        return latest


    def trigger_bmc_debug_log_dump(self):
        """Verify the BMC is reachable over IPMI.

        IPMI collection is synchronous, so there is no asynchronous task to
        start; this simply confirms the BMC responds and returns a task id.

        Returns:
            (ret, (task_id, err_msg))
        """
        ret, out = self._ipmitool(["mc", "info"], timeout=15)
        if ret != 0:
            return (1, (self.INVALID_TASK_ID,
                        "BMC not responding to IPMI: {}".format(out.strip())))
        return (0, (self.TASK_ID, ""))

    def get_bmc_debug_log_dump(self, task_id, filename, path, timeout=120):
        """Collect BMC diagnostics via IPMI into path/filename (.tar.xz).

        Returns:
            (ret, err_msg)
        """
        if task_id == self.INVALID_TASK_ID:
            return (1, "Invalid task id")
        try:
            os.makedirs(path, exist_ok=True)
            out_path = os.path.join(path, filename)
            # Split the overall budget across the collectors.
            per_cmd_timeout = max(10, int(timeout / (len(self._COLLECTORS) + 1)))

            with tempfile.TemporaryDirectory() as tmpdir:
                for fname, args in self._COLLECTORS.items():
                    _, text = self._ipmitool(args, timeout=per_cmd_timeout)
                    with open(os.path.join(tmpdir, fname), "w") as fh:
                        fh.write(text)
                with tarfile.open(out_path, "w:xz") as tar:
                    tar.add(tmpdir, arcname="bmc-ipmi-dump")
            return (0, "")
        except Exception as exc:  # noqa: BLE001 - never fail the parent dump
            logger.error("BMC IPMI debug dump failed: %s", exc)
            return (1, str(exc))
