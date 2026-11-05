#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2019 Edgecore Networks Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#
# HISTORY:
#    mm/dd/yyyy (A.D.)
#    03/01/2024: [Roger] Created for AS9817-64 thermal plan
#    03/19/2025: [Roger] Collected and sent MAC temperature,
#                        max port temperature, and its port number.
#    12/10/2025: [Roger] Refactored to modular class-based architecture
#                        with dual BMC/CPU thermal control paths.
#    01/05/2026: Richard_KUO Add the flag to control the tolerance
# ----------------------------------------------------------------------------------------
# ARCHITECTURE OVERVIEW:
#
# +======================================================================================+
# |                            AS9817-64 THERMAL CONTROL STACK                           |
# +======================================================================================+
# |                                   Hardware Layer                                     |
# |--------------------------------------------------------------------------------------|
# |  - CPU temperature sensor                                                            |
# |  - MAC/ASIC temperature sensor                                                       |
# |  - System fans (tachometer + PWM)                                                    |
# |  - XCVR modules with DOM temperature sensors                                         |
# |  - BMC                                                                               |
# +--------------------------------------+-----------------------------------------------+
#                                        |
#                                        v
# +--------------------------------------------------------------------------------------+
# |                                SONiC Layer                                           |
# |--------------------------------------------------------------------------------------|
# |  - sonic_platform.platform.Platform().get_chassis()                                  |
# |       • thermals  (CPU / MAC sensors)                                                |
# |       • fans      (PWM control, presence, speed)                                     |
# |       • sfps      (DOM access, presence, low-power mode)                             |
# |                                                                                      |
# |  - STATE_DB via swsscommon.DBConnector / Table                                       |
# |       • TRANSCEIVER_DOM_SENSOR table for per-port optical temperature                |
# |                                                                                      |
# |  - System utilities                                                                  |
# |       • ipmitool (BMC raw commands: version, policy enable, power control, etc.)     |
# |       • sonic_py_common.getstatusoutput_noshell() / subprocess                       |
# +--------------------------------------+-----------------------------------------------+
#                                        |
#                                        v
# +--------------------------------------------------------------------------------------+
# |                   accton_as9817_64_monitor (this process)                            |
# +--------------------------------------------------------------------------------------+
# |                                      main()                                          |
# |--------------------------------------------------------------------------------------|
# |  - Parses arguments (log options, etc.)                                              |
# |  - Creates DeviceMonitor                                                             |
# |  - Creates AlignedScheduler with:                                                    |
# |       • task_func   = DeviceMonitor.manage_fans                                      |
# |       • cleanup_func= DeviceMonitor.cleanup                                          |
# |  - Starts periodic monitoring loop                                                   |
# +--------------------------------------+-----------------------------------------------+
#                                        |
#                                        v
# +--------------------------------------------------------------------------------------+
# |                                   AlignedScheduler                                   |
# |--------------------------------------------------------------------------------------|
# |  - Provides time-aligned periodic execution (e.g. every 10 seconds)                  |
# |  - Handles signals (SIGINT/SIGTERM) and invokes cleanup                              |
# |  - Calls DeviceMonitor.manage_fans() at each interval                                |
# +--------------------------------------+-----------------------------------------------+
#                                        |
#                                        v
# +--------------------------------------------------------------------------------------+
# |                                   DeviceMonitor                                      |
# |--------------------------------------------------------------------------------------|
# |  Role: Orchestrator for thermal monitoring and fan control.                          |
# |--------------------------------------------------------------------------------------|
# |  Holds references to:                                                                |
# |    - platform chassis (thermals / fans / sfp_list)                                   |
# |    - STATE_DB connector and DOM sensor table                                         |
# |    - CommandRunner     (generic subprocess + retry logic)                            |
# |    - BmcController     (all BMC / ipmitool operations)                               |
# |    - FanPolicyEngine   (MIN/MID/MAX fan state machine)                               |
# |    - ThermalSnapshot   (per-iteration temperature snapshot object)                   |
# |    - Logging / configuration / thresholds (fan_thermal_spec, OTP limits, etc.)       |
# |--------------------------------------------------------------------------------------|
# |  Main responsibilities:                                                              |
# |    - Collect temperatures (CPU, MAC, XCVR) and build a ThermalSnapshot               |
# |    - Coordinate BMC-based thermal control when available                             |
# |    - Fallback to CPU-based fan policy control when BMC is unavailable                |
# |    - Enforce fan duty settings on platform fans                                      |
# |    - Perform Over-Temperature Protection (OTP) and safe shutdown via BMC             |
# +-----------+-------------------------+--------------------------+---------------------+
#             |                         |                          |                    ^
#             | uses                    | owns / uses              | queries            |
#             v                         v                          v                    |
# +----------------------+   +--------------------------+   +------------------------+  |
# |    CommandRunner     |   |      BmcController       |   |    FanPolicyEngine     |  |
# |----------------------|   |--------------------------|   |------------------------|  |
# | - Runs external      |   | - Tracks BMC FW version  |   | - Maintains current    |  |
# |   commands           |   |   (minimum required      |   |   fan state:           |  |
# | - Provides timeout   |   |    version for policy)   |   |   LEVEL_FAN_MIN/MID/   |  |
# |   and retry logic    |   | - Enables BMC thermal    |   |   MAX                  |  |
# | - Avoids shell=True  |   |   policy when supported  |   | - Evaluates CPU/MAC/   |  |
# +----------+-----------+   | - Reports MAC and max    |   |   XCVR temperatures    |  |
#            |               |   XCVR temperatures/port |   | - Applies thresholds   |  |
#            | subprocess    |   to the BMC             |   |   from fan_thermal_spec|  |
#            v               +--------------------------+   | - Decides new fan      |  |
# +----------------------+                                  |   state for this cycle |  |
# |  ipmitool / other    |                                  +------------------------+  |
# |  system utilities    |                                                              |
# +----------------------+                                                              |
#                                                                                       |
#             ---------------------------------------------------------------------------
#             |
#             | created and populated by DeviceMonitor
#             | for each monitoring cycle
#             |
# +----------------------+
# |   ThermalSnapshot    |
# |----------------------|
# |  - cpu_temp_mdeg     |
# |  - mac_temp_mdeg     |
# |  - list of per-port  |
# |    SFP entries:      |
# |      • temperature   |
# |      • presence      |
# |      • port index    |
# |  - max_sfp_temp_c    |
# |  - max_sfp_port      |
# +----------------------+
#
# ---------------------------------------------------------------------------------------
# MAIN CONTROL FLOW (every 10 seconds):
#
#   1. Collect temperatures from CPU, MAC, and all XCVR ports.
#   2. Build a ThermalSnapshot and compute the maximum XCVR temperature
#      and its port number.
#   3. Attempt BMC-based thermal control:
#      - Check BMC firmware version (>= 0.3.3 required).
#      - Ensure the BMC thermal policy is enabled.
#      - Send MAC temperature and max XCVR temperature/port to the BMC.
#      - If successful, the BMC controls the fan speed.
#   4. If BMC control fails, fall back to CPU-based control:
#      - Evaluate temperatures against fan_thermal_spec thresholds.
#      - Update the fan policy state (MIN/MID/MAX).
#      - Check all fans (presence, health, speed).
#      - If any fan has failed, force 100% duty cycle.
#      - Otherwise, set the fan duty cycle according to the policy:
#        * MIN = 30%, MID = 60%, MAX = 100%.
#   5. Over Temperature Protection (OTP) check:
#      - Evaluated only if the fan state at the beginning of the
#        iteration was LEVEL_FAN_MAX.
#      - CPU or MAC crossing the OTP shutdown threshold triggers:
#        * Enabling low-power mode on all XCVR ports.
#        * Synchronizing and trimming filesystems (sync & fstrim).
#        * Powering off the DUT via BMC.
#      - Any XCVR reaching the OTP warning threshold logs a warning
#        with the port number but does not shutdown the DUT.
#
# ---------------------------------------------------------------------------------------
# FAN POLICY STATE MACHINE (CPU-based fallback):
#
#   This state machine is used ONLY when BMC-based thermal control is not available.
#   The initial state is LEVEL_FAN_MID (60%).
#
#   Core policy (Idle mode):
#     - LEVEL_FAN_MIN (30%) is allowed ONLY when:
#         * CPU < 60C AND MAC < 60C
#         * AND no transceiver (SFP/QSFP) is present
#     - If any transceiver becomes present while in LEVEL_FAN_MIN,
#       the policy forces LEVEL_FAN_MIN -> LEVEL_FAN_MID immediately.
#
#   State transitions (with hysteresis):
#
#     +----------+  (temp >= min_to_mid_temp for any monitored point)  +----------+
#     | LEVEL_   | --------------------------------------------------> | LEVEL_   |
#     | FAN_MIN  |                                                     | FAN_MID  |
#     | (30%)    | <-------------------------------------------------- | (60%)    |
#     +----------+  (CPU < 60C AND MAC < 60C) AND (no SFP present)      +----------+
#                   NOTE: If any SFP is present, MIN is not allowed.
#
#     +----------+  (temp >= mid_to_max_temp for any monitored point)  +----------+
#     | LEVEL_   | --------------------------------------------------> | LEVEL_   |
#     | FAN_MID  |                                                     | FAN_MAX  |
#     | (60%)    | <-------------------------------------------------- | (100%)   |
#     +----------+  (temp < max_to_mid_temp for ALL present points)     +----------+
#
#   Temperature thresholds (millidegree Celsius):
#     - MIN to MID (warning):  CPU 60000, MAC 60000, SFP 70000
#     - MID to MAX (error):    CPU 85000, MAC 90000, SFP 70000
#     - MAX to MID (recovery): CPU 75000, MAC 80000, SFP 65000
#
#   OTP (Over Temperature Protection):
#     - Evaluated separately by DeviceMonitor and only when the fan state at the
#       beginning of the iteration is LEVEL_FAN_MAX.
#     - OTP shutdown thresholds: CPU 100000, MAC 105000
#     - OTP warning threshold for SFP: 70000 (warning only, no shutdown)
#
# --------------------------------------------------------------------------------------

import os
import sys
import getopt
import logging
import logging.handlers
import signal
import time
import re
import subprocess
from decimal import Decimal, getcontext

try:
    from sonic_platform import platform
    from swsscommon import swsscommon
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Defaults
VERSION = '2.0'
FUNCTION_NAME = 'accton_as9817_64_monitor'

STATE_DB = 'STATE_DB'
TRANSCEIVER_DOM_SENSOR_TABLE = 'TRANSCEIVER_DOM_SENSOR'
TEMPERATURE_FIELD_NAME = 'temperature'
TYPE_SENSOR = 'sensors'
TYPE_TRANSCEIVER = 'sfp'
CPU_TEMPERATURE_NAME = "CPU_Package_temp"
MAC_TEMPERATURE_NAME = "MAC"

FAN_DUTY_CYCLE_MAX = 100
FAN_DUTY_CYCLE_DEFAULT = 60
BMC_MIN_REQUIRED_VER = [0, 3, 3]
MONITOR_INTERVAL = 10
CPU_FAILURE_TEMPERATURE = 85.0
MAC_FAILURE_TEMPERATURE = 90.0
XCVR_FAILURE_TEMPERATURE = 0.0
TEMPERATURE_COMPENSATION = 5.0
TRANSCEIVER_NUM_MAX = 64
THERMAL_NUM_MAX = 2  # CPU + MAC

DEFAULT_SPEED_TOLERANCE = 20
FAN_SPEED_SETTLE_TIMEOUT_S = 40

# Global retry settings
DEFAULT_RETRIES = 3           # Number of retry attempts
DEFAULT_RETRY_INTERVAL = 1    # Time (seconds) to wait before retrying
DEFAULT_COMMAND_TIMEOUT = 3   # Timeout for each command execution (in seconds)

DEBUG = False

LEVEL_FAN_INIT = 0
LEVEL_FAN_MIN = 1
LEVEL_FAN_MID = 2
LEVEL_FAN_MAX = 3
LEVEL_FAN_OTP = 4
LEVEL_FAN_SHUTDOWN = 6

# Mapping fan states to string names for debug logging.
fan_state_dict = {
    LEVEL_FAN_INIT: 'level_fan_init',
    LEVEL_FAN_MIN: 'level_fan_min',
    LEVEL_FAN_MID: 'level_fan_mid',
    LEVEL_FAN_MAX: 'level_fan_max',
    LEVEL_FAN_OTP: 'level_fan_otp',
}

# Mapping fan states to [duty_percent, raw_hex] pairs.
FAN_POLICY = {
    LEVEL_FAN_INIT: [30, 0x4],
    LEVEL_FAN_MIN:  [30, 0x4],
    LEVEL_FAN_MID:  [60, 0x9],
    LEVEL_FAN_MAX:  [100, 0xf],
}

# Fan thermal thresholds (millidegree Celsius).
# Indexing: 0 -> CPU, 1 -> MAC, 2..65 -> SFP1..64
fan_thermal_spec = {
    "min_to_mid_temp": [(TYPE_SENSOR, 60000), (TYPE_SENSOR, 60000)],
    "mid_to_max_temp": [(TYPE_SENSOR, 85000), (TYPE_SENSOR, 90000)],
    "max_to_otp_temp": [(TYPE_SENSOR, 100000), (TYPE_SENSOR, 105000)],
    "max_to_mid_temp": [(TYPE_SENSOR, 75000), (TYPE_SENSOR, 80000)],
}
fan_thermal_spec["min_to_mid_temp"] += [(TYPE_TRANSCEIVER, 70000)] * TRANSCEIVER_NUM_MAX
fan_thermal_spec["mid_to_max_temp"] += [(TYPE_TRANSCEIVER, 70000)] * TRANSCEIVER_NUM_MAX
fan_thermal_spec["max_to_otp_temp"] += [(TYPE_TRANSCEIVER, 70000)] * TRANSCEIVER_NUM_MAX
fan_thermal_spec["max_to_mid_temp"] += [(TYPE_TRANSCEIVER, 65000)] * TRANSCEIVER_NUM_MAX


class CommandRunner(object):
    """
    Helper class for executing external commands with timeouts and retries.

    This class centralizes interaction with subprocess and encapsulates
    logging and retry policies. It avoids using shell=True to reduce
    the risk of shell injection.
    """

    def __init__(self, logger=None, should_stop_func=None):
        """
        Initialize the command runner.

        Args:
            logger: Optional logger instance. If None, a module-level logger
                will be used.
            should_stop_func: Optional callable that returns True when
                the runner should stop retrying (for example when a signal
                requests graceful shutdown).
        """
        self.logger = logger or logging.getLogger(FUNCTION_NAME)
        self.should_stop = should_stop_func

    def run_with_timeout(self, cmd, timeout=None):
        """
        Execute a command with a timeout.

        Args:
            cmd: List of command tokens to pass to subprocess.check_output.
            timeout: Timeout in seconds for the command.

        Returns:
            Tuple (status, output). Status 0 means success.
        """
        try:
            output = subprocess.check_output(
                cmd,
                universal_newlines=True,
                timeout=timeout,
                stderr=subprocess.STDOUT
            )
            return 0, output.rstrip('\n')
        except subprocess.TimeoutExpired:
            self.logger.debug(
                "[TIMEOUT] Command timed out after %ss: %s", timeout, cmd
            )
            return -1, "Timeout"
        except subprocess.CalledProcessError as ex:
            return ex.returncode, ex.output.rstrip('\n')
        except Exception as ex:
            return -1, str(ex)

    def retry(self, cmd, timeout=DEFAULT_COMMAND_TIMEOUT):
        """
        Execute a command with retries and timeout.

        Args:
            cmd: List of command tokens to execute.
            timeout: Timeout in seconds for each attempt.

        Returns:
            Tuple (status, output). Status 0 means success.
        """
        for attempt in range(1, DEFAULT_RETRIES + 1):
            if self.should_stop is not None and self.should_stop():
                self.logger.debug(
                    "Retry interrupted by stop request: %s", cmd
                )
                return -1, "Interrupted by stop request"

            self.logger.debug(
                "Attempt %d/%d: Running command %s",
                attempt, DEFAULT_RETRIES, cmd
            )
            status, output = self.run_with_timeout(cmd, timeout=timeout)
            if status == 0:
                return status, output

            self.logger.debug(
                "Attempt %d failed: [%d:%s]", attempt, status, output
            )

            if attempt < DEFAULT_RETRIES:
                self.logger.debug(
                    "Retrying in %d seconds...", DEFAULT_RETRY_INTERVAL
                )
                time.sleep(DEFAULT_RETRY_INTERVAL)

        self.logger.debug("All retry attempts failed.")
        return -1, ""


class BmcController(object):
    """
    Encapsulates all BMC-related logic:

      * Detecting BMC firmware version and capability.
      * Verifying and enabling BMC thermal policy.
      * Reporting thermal data to BMC.
    """

    def __init__(self, cmd_runner, logger=None):
        """
        Initialize a new BmcController.

        Args:
            cmd_runner: CommandRunner instance used to execute ipmitool.
            logger: Optional logger instance. If None, a module-level logger
                will be used.
        """
        self.cmd_runner = cmd_runner
        self.logger = logger or logging.getLogger(FUNCTION_NAME)
        self.bmc_checked = False
        self.bmc_capable = False
        self.bmc_version = [0, 0, 0]

    def _ipmi_raw(self, *args):
        """
        Execute an IPMI raw command using the shared CommandRunner.

        Args:
            *args: Variable length arguments for ipmitool 'raw'.

        Returns:
            Tuple (status, output).
        """
        cmd = ['ipmitool', 'raw'] + list(args)
        self.logger.debug("IPMI Command: %s", cmd)
        return self.cmd_runner.retry(cmd, timeout=DEFAULT_COMMAND_TIMEOUT)

    def _read_version(self):
        """
        Read BMC firmware version via 'ipmitool raw 0x6 0x1'.

        Returns:
            True on success, False on failure.
        """
        status, output = self._ipmi_raw('0x6', '0x1')
        if status != 0:
            return False

        parts = output.strip().split()
        if len(parts) < 12:
            self.logger.debug("Unexpected IPMI output: %s", output)
            return False

        # IPMI Device ID command output does not include completion code.
        # parts[2] -> Firmware Revision 1 (major, bit7 is "update in progress").
        try:
            raw_major = int(parts[2], 16)
            major = raw_major & 0x7F
            minor = int(parts[3], 16)
            aux = int(parts[11], 16)
        except (ValueError, IndexError) as ex:
            self.logger.debug("Failed to parse BMC version: %s", ex)
            return False

        old_version = list(self.bmc_version)
        self.bmc_version = [major, minor, aux]
        if old_version != self.bmc_version:
            self.logger.info(
                "BMC firmware version changed: %s -> %s",
                old_version, self.bmc_version
            )

        self.logger.debug(
            "BMC version detected: %d.%d.%d", major, minor, aux
        )
        return True

    def update_capability(self):
        """
        Refresh BMC capability status.

        This function always does a full version check by calling _read_version().

        Returns:
            True if BMC version >= BMC_MIN_REQUIRED_VER, otherwise False.
        """
        self.bmc_checked = True

        if not self._read_version():
            self.bmc_capable = False
            return False

        if self.bmc_version < BMC_MIN_REQUIRED_VER:
            self.logger.warning(
                "BMC version %s is below required minimum %s",
                self.bmc_version, BMC_MIN_REQUIRED_VER
            )
            self.bmc_capable = False
            return False

        self.bmc_capable = True
        return True

    def ensure_policy_enabled(self):
        """
        Ensure that the BMC thermal policy is enabled.

        This method queries current policy status and, if it is not enabled,
        attempts to enable it.

        Returns:
            True if the policy is enabled after this call, False otherwise.
        """
        status, output = self._ipmi_raw('0x34', '0x66')
        if status != 0:
            self.logger.debug("Failed to get BMC thermal policy status.")
            return False

        parts = output.strip().split()
        if not parts:
            self.logger.debug(
                "Empty output from BMC thermal policy query."
            )
            return False

        try:
            status_code = int(parts[0], 16)
        except ValueError:
            self.logger.debug(
                "Invalid BMC policy status format: %s", output
            )
            return False

        if status_code == 3:
            return True

        # Try to enable the policy.
        status, output = self._ipmi_raw('0x34', '0x67', '0', '3')
        if status != 0:
            self.logger.warning("Failed to enable BMC thermal policy.")
            return False

        return True

    def report_thermal(self, mac_temp_c, xcvr_temp_c, xcvr_port):
        """
        Report thermal data (MAC + max transceiver) to the BMC.

        This method assumes that the caller has already normalized the
        transceiver temperature and port values. In particular, if no
        optics are present, the caller should pass xcvr_temp_c = 0.0
        and xcvr_port = 0.

        Args:
            mac_temp_c: MAC temperature in degrees Celsius.
            xcvr_temp_c: Maximum transceiver temperature in degrees Celsius.
            xcvr_port: Port number of the transceiver with the maximum
                temperature, or 0 if none are present.

        Returns:
            True if the report was sent successfully, False otherwise.
        """
        cmd_args = [
            '0x34', '0x13',
            str(int(mac_temp_c)),
            str(int(xcvr_temp_c)),
            str(int(xcvr_port)),
        ]
        status, output = self._ipmi_raw(*cmd_args)
        if status != 0:
            self.logger.debug("Failed to send thermal report to BMC.")
            # Clear bmc_checked so that capability will be re-evaluated
            # on the next iteration.
            self.bmc_checked = False
            return False

        return True


class ThermalSnapshot(object):
    """
    Represents a snapshot of all relevant temperatures in one iteration.

    This class uses __slots__ to reduce memory footprint since instances
    are created frequently during monitoring.

    This includes:
      * CPU temperature.
      * MAC temperature.
      * SFP temperatures, presence, and port numbers.
    """

    __slots__ = (
        'cpu_temp_mdeg', 'mac_temp_mdeg', 'sfp_entries',
        'sfp_present_count', 'max_sfp_temp_c', 'max_sfp_port',
        '_cached_thermal_list'
    )

    def __init__(self):
        self.cpu_temp_mdeg = 0
        self.mac_temp_mdeg = 0
        self.sfp_entries = []  # List of dicts describing each SFP.
        self.sfp_present_count = 0
        self.max_sfp_temp_c = 0.0
        self.max_sfp_port = 0
        self._cached_thermal_list = None

    @property
    def cpu_temp_c(self):
        """Return CPU temperature in degrees Celsius."""
        return self.cpu_temp_mdeg / 1000.0

    @property
    def mac_temp_c(self):
        """Return MAC temperature in degrees Celsius."""
        return self.mac_temp_mdeg / 1000.0

    def add_sfp(self, port, name, present, temp_mdeg):
        """
        Add an SFP entry to the snapshot.

        Args:
            port: Transceiver port number (1-based).
            name: Logical interface name.
            present: True if the transceiver is present.
            temp_mdeg: Temperature in millidegrees Celsius.
        """
        entry = {
            "port": port,
            "name": name,
            "present": present,
            "temp_mdeg": temp_mdeg,
        }
        self.sfp_entries.append(entry)
        if present:
            self.sfp_present_count += 1

    def finalize(self):
        """
        Compute derived SFP values such as maximum temperature and port.

        This method must be called after all SFP entries are added.
        Only SFPs that are actually present are considered when
        determining the maximum temperature.

        Also invalidates the cached thermal list so it will be
        rebuilt on next access.
        """
        self._cached_thermal_list = None

        if self.sfp_present_count == 0:
            self.max_sfp_port = 0
            self.max_sfp_temp_c = 0.0
            return

        present_entries = [e for e in self.sfp_entries if e["present"]]
        if not present_entries:
            # Defensive fallback: if the presence counter was non-zero but
            # the entries do not reflect that, behave as if there were no
            # valid SFP temperature readings.
            self.max_sfp_port = 0
            self.max_sfp_temp_c = 0.0
            return

        max_entry = max(present_entries, key=lambda e: e["temp_mdeg"])
        self.max_sfp_port = max_entry["port"]
        self.max_sfp_temp_c = max_entry["temp_mdeg"] / 1000.0

    def build_flat_thermal_list(self):
        """
        Build or return cached flat list of all thermal points in the same index order
        as fan_thermal_spec.

        The list is cached after first construction and reused for subsequent
        calls within the same snapshot lifecycle. Call finalize() to invalidate
        the cache.

        Index mapping:
          0: CPU
          1: MAC
          2..65: SFP1..SFP64

        Each item is a tuple:
            (temp_type, current_temp_mdeg, name, present, port)

        For CPU and MAC entries, the port field is set to 0.
        """
        if self._cached_thermal_list is not None:
            return self._cached_thermal_list

        items = []

        # CPU
        items.append((
            TYPE_SENSOR,
            self.cpu_temp_mdeg,
            CPU_TEMPERATURE_NAME,
            True,
            0,
        ))

        # MAC
        items.append((
            TYPE_SENSOR,
            self.mac_temp_mdeg,
            MAC_TEMPERATURE_NAME,
            True,
            0,
        ))

        # SFPs
        for entry in self.sfp_entries:
            items.append((
                TYPE_TRANSCEIVER,
                entry["temp_mdeg"],
                entry["name"],
                entry["present"],
                entry["port"],
            ))

        self._cached_thermal_list = items
        return items

class FanPolicyEngine(object):
    """
    Encapsulates the fan speed state machine based on a ThermalSnapshot.

    The engine maintains and updates the current fan policy state:
      LEVEL_FAN_MIN -> LEVEL_FAN_MID -> LEVEL_FAN_MAX

    Hysteresis is implemented using:
      * min_to_mid_temp
      * mid_to_max_temp
      * max_to_mid_temp

    OTP (Over Temperature Protection) is evaluated separately by the
    DeviceMonitor because it requires platform-specific actions such as
    powering off the device and putting transceivers into low power mode.
    """

    def __init__(self, logger=None):
        """
        Initialize the fan policy engine.

        Args:
            logger: Optional logger instance. If None, a module-level logger
                will be used.
        """
        self.logger = logger or logging.getLogger(FUNCTION_NAME)
        self.state = LEVEL_FAN_MID

    def decide(self, snapshot):
        """
        Decide new fan policy state based on the given snapshot.

        Rules in this function:
        - Build a flat thermal list from snapshot. The list index must match
          fan_thermal_spec index.
        - Skip a transceiver item if it is not present.
        - If any transceiver is present, fan state MIN is not allowed:
          when current state is MIN, force MIN -> MID and return.
        - State change by temperature thresholds:
          * MIN -> MID: if any item reaches min_to_mid_temp threshold.
          * MID -> MAX: if any item reaches mid_to_max_temp threshold.
          * MAX -> MID: only if all monitored items are below max_to_mid_temp
            threshold (count-based check).
          * MID -> MIN: only if all CPU/MAC sensors are below min_to_mid_temp
            threshold and no transceiver is present.

        Returns:
            (old_state, new_state)
        """
        old_state = self.state
        current_state = self.state
        max_to_mid = 0
        mid_to_min = 0

        # Build a flat list so that indices align with fan_thermal_spec.
        thermal_list = snapshot.build_flat_thermal_list()
        self.logger.debug("thermal_list=%s", thermal_list)

        for i, (temp_type, current_temp, name, present, port) in enumerate(thermal_list):
            # Skip non-present transceivers.
            if temp_type == TYPE_TRANSCEIVER and not present:
                continue

            # Presence gate: if any transceiver is present, MIN is not allowed.
            if temp_type == TYPE_TRANSCEIVER and present and old_state == LEVEL_FAN_MIN:
                current_state = LEVEL_FAN_MID
                self.logger.info("Transceiver present (%s). Force fan state MIN -> MID.", name)
                self.state = current_state
                return old_state, current_state

            if old_state == LEVEL_FAN_MIN:
                if current_temp >= fan_thermal_spec["min_to_mid_temp"][i][1]:
                    current_state = LEVEL_FAN_MID
                    self.logger.warning(
                        "- Monitor %s, temperature is %.1f. "
                        "Temperature is over the warning threshold (%.1f) "
                        "of thermal policy.",
                        name,
                        current_temp / 1000.0,
                        fan_thermal_spec["min_to_mid_temp"][i][1] / 1000.0,
                    )
                    break

            elif old_state == LEVEL_FAN_MID:
                if current_temp >= fan_thermal_spec["mid_to_max_temp"][i][1]:
                    current_state = LEVEL_FAN_MAX
                    self.logger.warning(
                        "- Monitor %s, temperature is %.1f. "
                        "Temperature is over the error threshold (%.1f) "
                        "of thermal policy.",
                        name,
                        current_temp / 1000.0,
                        fan_thermal_spec["mid_to_max_temp"][i][1] / 1000.0,
                    )
                    break
                else:
                    # Count how many CPU/MAC sensors are safely below the
                    # min_to_mid threshold in case we can go back down to MIN.
                    if (temp_type == TYPE_SENSOR and
                            current_temp < fan_thermal_spec["min_to_mid_temp"][i][1]):
                        mid_to_min += 1

            elif old_state == LEVEL_FAN_MAX:
                # OTP handling is performed by the caller
                # (DeviceMonitor._check_otp_and_shutdown()).
                if current_temp < fan_thermal_spec["max_to_mid_temp"][i][1]:
                    max_to_mid += 1

        # Transition from MID back to MIN:
        if (old_state == LEVEL_FAN_MID and
                mid_to_min == THERMAL_NUM_MAX and
                snapshot.sfp_present_count == 0):
            current_state = LEVEL_FAN_MIN
            self.logger.info(
                "- Monitor CPU/MAC, temperature is less than the warning "
                "threshold of thermal policy."
            )

        # Transition from MAX back to MID:
        if (old_state == LEVEL_FAN_MAX and
                max_to_mid == (THERMAL_NUM_MAX + snapshot.sfp_present_count)):
            current_state = LEVEL_FAN_MID
            self.logger.info(
                "- Monitor CPU/MAC, temperature is less than the error "
                "threshold of thermal policy."
            )

        self.state = current_state
        return old_state, current_state



class AlignedScheduler(object):
    """
    A periodic task runner that executes a function at fixed intervals,
    aligned to the scheduler's own start time (base time), not system time.

    Unlike traditional interval chaining (where the next run starts after
    the previous one finishes), this scheduler aligns execution to
    consistent time offsets from its start point
    (e.g., T=0s, T=10s, T=20s... relative to start).

    This ensures consistent execution cadence even if tasks run long.
    """

    def __init__(self, task_func, interval_sec=10,
                 cleanup_func=None, logger=None):
        """
        Initialize the aligned scheduler.

        Args:
            task_func: The function to run periodically.
            interval_sec: Interval in seconds between scheduled runs.
            cleanup_func: Optional function to call on SIGINT/SIGTERM
                or normal exit.
            logger: Optional logger instance. If None, a module-level logger
                will be used.
        """
        if not callable(task_func):
            raise ValueError("task_func must be a callable function")
        self.task_func = task_func
        self.interval_sec = interval_sec
        if cleanup_func is not None and not callable(cleanup_func):
            raise ValueError(
                "cleanup_func must be a callable function if provided"
            )
        self.cleanup_func = cleanup_func
        self.logger = logger or logging.getLogger(FUNCTION_NAME)
        self._stop_flag = False

    def _cleanup(self):
        """
        Execute the cleanup function if one was provided.
        """
        if self.cleanup_func:
            try:
                self.cleanup_func()
            except Exception as e:
                self.logger.debug("Exception during cleanup: %s", e)

    def _signal_handler(self, signum, frame):
        """
        Signal handler that stops the scheduler and performs cleanup.
        """
        self.logger.info("Received signal %s. Stopping task...", signum)
        self._stop_flag = True
        self._cleanup()

    def start(self):
        """
        Start the periodic task loop and handle signals gracefully.
        """
        # Register signal handlers
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)

        self.logger.info(
            "Starting periodic task loop (interval: %s seconds)",
            self.interval_sec
        )

        # Use monotonic time for scheduling to avoid issues with system time
        # adjustments (for example NTP corrections).
        base_time = time.monotonic()

        while not self._stop_flag:
            start = time.monotonic()
            try:
                self.task_func()
            except Exception as e:
                self.logger.debug("Exception in periodic task: %s", e)

            now = time.monotonic()
            if now < base_time:
                self.logger.debug(
                    "Clock anomaly detected: now (%.6f) < base_time (%.6f)",
                    now, base_time
                )
                base_time = now
                continue

            elapsed = now - base_time
            sleep_time = self.interval_sec - (elapsed % self.interval_sec)
            if abs(sleep_time - self.interval_sec) < 1e-9:
                sleep_time = 0

            self.logger.debug(
                "Task ended, duration: %.2f s, sleeping %.2f s",
                now - start, sleep_time
            )
            time.sleep(sleep_time)

        self.logger.info("Periodic task exited gracefully.")


class DeviceMonitor(object):
    """
    The main orchestrator that ties together platform sensors, BMC
    interaction, and the fan policy engine.
    """

    def __init__(self, log_file, log_level):
        """
        Initialize the device monitor.

        This sets up logging, SONiC platform objects (chassis, thermals,
        fans, SFPs), and helper components for BMC control and fan policy.

        Args:
            log_file: Path to the log file.
            log_level: Logging level for both file and console.
        """
        # Prepare logging to file.
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format='[%(asctime)s] {%(pathname)s:%(lineno)d} '
                   '%(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )

        # Optional logging to console when running in DEBUG level.
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter(
                '%(asctime)s %(name)-12s: %(levelname)-8s %(message)s',
                datefmt='%H:%M:%S'
            )
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        # Send INFO and above messages to syslog.
        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setLevel(logging.INFO)
        formatter = logging.Formatter('%(module)s: %(message)s')
        sys_handler.setFormatter(formatter)
        logging.getLogger('').addHandler(sys_handler)

        self.logger = logging.getLogger(FUNCTION_NAME)

        # Platform objects.
        self.platform_chassis = platform.Platform().get_chassis()
        self.thermals = self.platform_chassis.get_all_thermals()
        self.fans = self.platform_chassis.get_all_fans()
        self.sfps = self.platform_chassis.get_all_sfps()

        # High precision for Decimal computations in MAC temperature
        # conversion.
        getcontext().prec = 30

        # Stop flag used for graceful shutdown and for CommandRunner.
        self.stop_flag = False

        # Helper components.
        self.cmd_runner = CommandRunner(
            logger=self.logger,
            should_stop_func=self.is_stopping
        )
        self.bmc = BmcController(self.cmd_runner, logger=self.logger)
        self.fan_policy = FanPolicyEngine(logger=self.logger)

        # Temperature-related state.
        self.transceiver_dom_sensor_tbl = None

        # SFP maximum thermal info for BMC reporting.
        self.sfp_max_thermal_port = 0
        self.sfp_max_thermal_val = 0.0

        # Cache CPU thermal index for faster lookup.
        self._cpu_thermal_index = self._find_cpu_thermal_index()

        # Dynamically adjust tolerance
        self.fan_timer_start = time.time()
        self.pre_target_speed = [fan.get_target_speed() for fan in self.fans]

        self.logger.info("Device monitor initialized")

        self.set_fans_tolerance_mode("off")

    def is_stopping(self):
        """
        Return True if the monitor is in the process of shutting down.
        """
        return self.stop_flag

    def cleanup(self):
        """
        Request the monitor to stop and allow in-flight operations to
        finish gracefully.
        """
        self.stop_flag = True

    def set_fans_tolerance_mode(self, mode):
        """
        Set the tolerance mode for all fans in this group.
        Args:
            mode: "on" or "off"
        """
        if mode in ["on", "off"]:
            for fan in self.fans:
                fan.set_tolerance_mode(mode)

    def is_timer_expired(self):
        """
        Calculates if the timer has expired based on the fan speed settle timeout.
        """
        return (time.time() - self.fan_timer_start) >= FAN_SPEED_SETTLE_TIMEOUT_S

    def is_under_speed(self, fan):
        """
        Calculates if the fan speed is under the tolerated low speed threshold
        """
        speed = fan.get_speed()
        target_speed = fan.get_target_speed()
        tolerance = DEFAULT_SPEED_TOLERANCE
        return speed * 100 < target_speed * (100 - tolerance)

    def is_over_speed(self, fan):
        """
        Calculates if the fan speed is over the tolerated high speed threshold
        """
        speed = fan.get_speed()
        target_speed = fan.get_target_speed()
        tolerance = DEFAULT_SPEED_TOLERANCE
        return speed * 100 > target_speed * (100 + tolerance)

    def power_off_dut(self):
        """
        Execute commands to safely power off the device in case of
        critical temperatures.

        This method ensures data integrity by syncing the filesystem and
        trimming SSDs before shutdown.
        """
        # Flush file system buffers.
        status, output = getstatusoutput_noshell(['sync'])
        if status != 0:
            self.logger.warning("sync failed: %s", output)

        # Trim filesystems on SSDs.
        status, output = getstatusoutput_noshell(['/sbin/fstrim', '-av'])
        if status != 0:
            self.logger.warning("fstrim failed: %s", output)

        time.sleep(3)

        # Reset MAC and power off via BMC.
        status, output = self.cmd_runner.retry(
            ['ipmitool', 'raw', '0x34', '0x94', '3']
        )
        if status != 0:
            self.logger.warning("ipmitool power-off failed: %s", output)

        return True

    def reset_front_port_all(self):
        """
        Reset all front ports of the device.
        """

        for port_index in range(TRANSCEIVER_NUM_MAX):
            path = (
                "/sys/devices/platform/as9817_64_fpga/module_reset_%d"
                % (port_index + 1)
            )
            try:
                with open(path, 'w') as f:
                    f.write("1\n")
            except IOError as e:
                self.logger.warning(
                    "Failed to reset module %d via %s: %s",
                    port_index + 1, path, e
                )

        return True

    def enable_lpmode_front_port_all(self):
        """
        Enable low power mode for all present transceivers.
        """

        for sfp in self.sfps:
            if sfp.port_num > TRANSCEIVER_NUM_MAX:
                continue
            if sfp.get_presence():
                try:
                    sfp.set_lpmode(True)
                except Exception as e:
                    self.logger.warning(
                        "Failed to set LPMODE on SFP port %d: %s",
                        sfp.port_num, e
                    )

        return True

    def set_fan_speed(self, pwm):
        """
        Set the fan speed based on a PWM duty cycle percentage.

        Args:
            pwm: PWM duty cycle value in percent (0..100).

        Returns:
            True if the operation succeeded for all fans, False otherwise.
        """
        if pwm < 0 or pwm > 100:
            msg = "Error: Wrong duty cycle value %d" % pwm
            self.logger.warning(msg)
            print(msg)
            return False

        self.logger.debug("Set FAN speed to %d%%", pwm)

        if not self.fans:
            self.logger.warning(
                "No fans detected. Skipping fan speed setting."
            )
            return False

        for fan in self.fans:
            try:
                fan.set_speed(pwm)
            except Exception as e:
                self.logger.warning(
                    "Failed to set speed for fan %s: %s",
                    fan.get_name(), e
                )
        time.sleep(1)
        return True

    def _find_cpu_thermal_index(self):
        """
        Find and cache the index of the CPU thermal sensor.

        Scanning all thermals every iteration is wasteful when the CPU
        thermal sensor position is fixed. This method finds it once
        during initialization.

        Returns:
            Index of CPU thermal sensor in self.thermals, or None if not found.
        """
        for i, thermal in enumerate(self.thermals):
            try:
                name = thermal.get_name()
                if CPU_TEMPERATURE_NAME in name:
                    return i
            except (AttributeError, IOError, OSError):
                continue
        return None

    def get_cpu_temperature(self):
        """
        Retrieve the CPU temperature from thermal sensors.

        Returns:
            Tuple (TYPE_SENSOR, temp_millideg, name, present_flag).
        """
        if self._cpu_thermal_index is not None:
            thermal = self.thermals[self._cpu_thermal_index]
            try:
                temp_mdeg = thermal.get_temperature() * 1000.0
                return (TYPE_SENSOR, temp_mdeg, CPU_TEMPERATURE_NAME, True)
            except (AttributeError, IOError, OSError, TypeError) as e:
                self.logger.debug("Failed to read CPU temperature: %s", e)

        self.logger.warning("Failed to read CPU temperature; using fallback.")
        return (TYPE_SENSOR, CPU_FAILURE_TEMPERATURE * 1000.0,
                CPU_TEMPERATURE_NAME, True)

    def _get_mac_temperature_from_sdk(self):
        """
        Retrieve the MAC temperature using the SDK (bcmcmd 'show temp').

        Returns:
            Temperature in millidegrees Celsius, or None on failure.
        """
        cmd = ['bcmcmd', 'show temp']
        status, output = self.cmd_runner.run_with_timeout(
            cmd, timeout=DEFAULT_COMMAND_TIMEOUT
        )
        if status != 0:
            self.logger.warning(
                "Failed to read MAC temperature from SDK: %s", output
            )
            return None

        res_list = re.findall(
            r'Average current temperature is\s*(.+?)\n', output
        )
        if not res_list:
            self.logger.debug(
                "SDK output did not match expected format: %s", output
            )
            return None

        try:
            return float(res_list[0]) * 1000.0
        except ValueError:
            self.logger.debug(
                "Invalid MAC temperature value from SDK: %s", res_list[0]
            )
            return None

    def _i2cget(self, bus, addr, reg):
        """
        Read a register via 'i2cget' and return its value as an integer.

        Args:
            bus: I2C bus number.
            addr: I2C device address.
            reg: Register address.

        Returns:
            Integer value of the register.

        Raises:
            RuntimeError: If i2cget command fails.
            ValueError: If output cannot be parsed as hexadecimal.
        """
        cmd = ["i2cget", "-f", "-y", str(bus), hex(addr), hex(reg)]
        status, output = getstatusoutput_noshell(cmd)
        if status != 0:
            raise RuntimeError("i2cget failed: %s" % output.strip())

        try:
            return int(output.strip(), 16)
        except ValueError:
            raise ValueError(
                "Unexpected output from i2cget: %s" % output.strip()
            )

    def _get_mac_temperature_from_fpga(
            self, clk_addr_start, bus=0, i2c_addr=0x60):
        """
        Read a 32-bit frequency from FPGA over I2C and convert to temperature.

        Conversion is based on the following model:

          Step 1: Read frequency in Hz (from 4 consecutive registers).

          Step 2: Compute period:
              period = 1 / freq

          Step 3: Derive ADC data:
              period = (Data + 1) * 80 ns
              Data   = period / 80e-9 - 1

          Step 4: Convert ADC data to temperature:
              temperature = -0.317704 * Data + 476.359

        Args:
            clk_addr_start: Starting register address for the clock value.
            bus: I2C bus ID.
            i2c_addr: I2C address of the FPGA device.

        Returns:
            Temperature in degrees Celsius.

        Raises:
            RuntimeError, ValueError on communication or conversion errors.
        """
        try:
            clk_bytes = [
                self._i2cget(bus, i2c_addr, clk_addr_start + i)
                for i in range(4)
            ]
        except Exception as e:
            raise RuntimeError(
                "Failed to read clock registers from I2C: %s" % e
            )

        # Combine bytes into 32-bit little-endian frequency value.
        freq = 0
        for i, b in enumerate(clk_bytes):
            freq |= (b << (8 * i))

        if freq == 0:
            self.logger.warning(
                "I2C frequency registers at 0x%x returned all zero",
                clk_addr_start
            )
            raise ValueError("Frequency is zero, invalid clock data")

        freq_dec = Decimal(freq)

        # Compute period in seconds.
        period = Decimal(1) / freq_dec

        # Derive ADC data.
        period_unit = Decimal("80e-9")
        data = (period / period_unit) - Decimal(1)

        if not (Decimal(0) <= data <= Decimal(2047)):
            raise ValueError(
                "Calculated ADC value %s out of 11-bit range" % data
            )

        coefficient1 = Decimal("-0.317704")
        coefficient2 = Decimal("476.359")
        temperature = coefficient1 * data + coefficient2

        return float(temperature)

    def get_mac_temperature(self):
        """
        Retrieve the MAC temperature from SDK or FPGA fallback.

        This method first tries to use the SDK. If that fails, it falls
        back to reading two temperature values from the FPGA (min and max)
        via I2C and averaging them.

        Returns:
            Tuple (TYPE_SENSOR, temp_millideg, name, present_flag).
        """
        mac_temperature = self._get_mac_temperature_from_sdk()
        if mac_temperature is None:
            try:
                min_temp = self._get_mac_temperature_from_fpga(0xA0)
                min_temp += TEMPERATURE_COMPENSATION
            except Exception:
                min_temp = None

            try:
                max_temp = self._get_mac_temperature_from_fpga(0xA4)
                max_temp += TEMPERATURE_COMPENSATION
            except Exception:
                max_temp = None

            if min_temp is None or max_temp is None:
                self.logger.warning(
                    "Failed to read MAC temperature from FPGA; using fallback."
                )
                mac_temperature = MAC_FAILURE_TEMPERATURE * 1000.0
            else:
                mac_temperature = ((min_temp + max_temp) / 2.0) * 1000.0
                self.logger.debug(
                    "Read MAC temperature from FPGA: %d mdegC",
                    int(mac_temperature)
                )
        else:
            self.logger.debug(
                "Read MAC temperature from SDK: %d mdegC",
                int(mac_temperature)
            )

        return (TYPE_SENSOR, mac_temperature, MAC_TEMPERATURE_NAME, True)

    def _get_transceiver_dom_table(self):
        """
        Lazy initialization of the transceiver DOM sensor table.

        Returns:
            swsscommon.Table instance, or None on failure.
        """
        if self.transceiver_dom_sensor_tbl is None:
            try:
                state_db = swsscommon.DBConnector(STATE_DB, 0, False)
                self.transceiver_dom_sensor_tbl = swsscommon.Table(
                    state_db, TRANSCEIVER_DOM_SENSOR_TABLE
                )
            except Exception as e:
                self.logger.debug("DOM table init failed: %s", e)
                self.transceiver_dom_sensor_tbl = None

        return self.transceiver_dom_sensor_tbl

    def get_transceiver_temperature(self, iface_name):
        """
        Fetch the temperature of a specified transceiver via STATE_DB.

        Args:
            iface_name: Interface name of the transceiver.

        Returns:
            Temperature in degrees Celsius as a float, or
            XCVR_FAILURE_TEMPERATURE on failure.
        """
        dom_tbl = self._get_transceiver_dom_table()
        if dom_tbl is None:
            return XCVR_FAILURE_TEMPERATURE

        try:
            status, ret = dom_tbl.hget(iface_name, TEMPERATURE_FIELD_NAME)
            if status and ret is not None:
                return float(ret)
        except (TypeError, ValueError):
            pass

        return XCVR_FAILURE_TEMPERATURE

    def collect_temperature(self):
        """
        Collect temperatures from CPU, MAC, and all transceivers.

        Returns:
            ThermalSnapshot instance populated with the latest readings.
        """
        snapshot = ThermalSnapshot()

        sensor_cpu = self.get_cpu_temperature()
        sensor_mac = self.get_mac_temperature()
        snapshot.cpu_temp_mdeg = int(sensor_cpu[1])
        snapshot.mac_temp_mdeg = int(sensor_mac[1])

        for sfp in self.sfps:
            if sfp.port_num > TRANSCEIVER_NUM_MAX:
                continue
            name = sfp.get_name()
            port = sfp.port_num
            present = sfp.get_presence()
            temp_mdeg = 0
            if present:
                temp_c = self.get_transceiver_temperature(name)
                temp_mdeg = int(temp_c * 1000.0)
            snapshot.add_sfp(port, name, present, temp_mdeg)

        snapshot.finalize()

        self.sfp_max_thermal_port = snapshot.max_sfp_port
        self.sfp_max_thermal_val = snapshot.max_sfp_temp_c

        if snapshot.sfp_present_count > 0:
            self.logger.debug(
                "Max Transceiver Thermal: Port %d, %.1f C",
                snapshot.max_sfp_port, snapshot.max_sfp_temp_c
            )

        return snapshot

    def _check_otp_and_shutdown(self, snapshot, ori_state):
        """
        Check for over-temperature protection (OTP) conditions and
        perform shutdown actions if required.

        OTP evaluation is intentionally gated by the fan policy state at
        the beginning of the iteration. This preserves the legacy
        behavior where OTP checks are only performed when the controller
        is already in LEVEL_FAN_MAX.
        """

        # Only perform OTP checks when the previous fan state was MAX.
        # This matches the original implementation where OTP logic was
        # executed only under LEVEL_FAN_MAX.
        if ori_state != LEVEL_FAN_MAX:
            return False

        # Use the same flat thermal view as the fan policy engine.
        thermal_list = snapshot.build_flat_thermal_list()
        shutdown_triggered = False

        for i, (temp_type, current_temp, name, present, port) in enumerate(thermal_list):
            if temp_type == TYPE_TRANSCEIVER and not present:
                continue

            threshold = fan_thermal_spec["max_to_otp_temp"][i][1]
            if current_temp >= threshold:
                if temp_type == TYPE_SENSOR:
                    self.logger.critical(
                        "- Monitor %s, temperature is %.1f. "
                        "Temperature is over the shutdown threshold (%.1f) "
                        "of thermal policy, shutdown DUT.",
                        name,
                        current_temp / 1000.0,
                        threshold / 1000.0,
                    )
                    self.enable_lpmode_front_port_all()
                    self.power_off_dut()
                    shutdown_triggered = True
                else:
                    # For optics, include the port number in the log for easier
                    # correlation with front panel modules.
                    self.logger.warning(
                        "- Monitor %s, temperature is %.1f. "
                        "Temperature is over the error threshold (%.1f) "
                        "of thermal policy.",
                        name,
                        current_temp / 1000.0,
                        threshold / 1000.0,
                    )

        return shutdown_triggered

    def _control_thermal_policy_via_cpu(self, snapshot):
        """
        Control thermal policy purely via CPU/MAC/SFP temperatures
        using the local fan policy engine.
        """
        old_state, new_state = self.fan_policy.decide(snapshot)

        ori_duty_cycle = 0
        new_duty_cycle = 0
        fan_fail_list = []

        # Check fan status and determine the highest duty cycle among all fans.
        for fan in self.fans:
            if not fan.get_presence() or not fan.get_status() or not fan.get_speed():
                fan_fail_list.append(fan.get_name())
            else:
                ori_duty_cycle = max(ori_duty_cycle, fan.get_speed())

        self.logger.debug("Current Fan speed = %d%%", ori_duty_cycle)

        # Update fan state if changed.
        if new_state != old_state:
            if new_state > old_state:
                self.logger.warning(
                    "Increase fan duty_cycle from %d%% to %d%%.",
                    FAN_POLICY[old_state][0],
                    FAN_POLICY[new_state][0],
                )
            else:
                self.logger.info(
                    "Decrease fan duty_cycle from %d%% to %d%%.",
                    FAN_POLICY[old_state][0],
                    FAN_POLICY[new_state][0],
                )

        # Set the fan speed to the maximum if any fan has failed,
        # otherwise adjust according to the policy state.
        if not fan_fail_list:
            new_duty_cycle = FAN_POLICY[new_state][0]
        else:
            new_duty_cycle = FAN_DUTY_CYCLE_MAX
            for fan_name in fan_fail_list:
                self.logger.warning(
                    "%s has failed, so set the duty_cycle to 100%%",
                    fan_name,
                )

        if new_duty_cycle != ori_duty_cycle:
            self.set_fan_speed(new_duty_cycle)

        self.logger.debug(
            "fan_policy_state=%s, new_duty_cycle=%d%%, fan_fail_list=%s",
            fan_state_dict[new_state],
            new_duty_cycle,
            fan_fail_list,
        )

        # Evaluate OTP only after the fan state has been updated.
        # OTP is checked against the fan policy state at the beginning
        # of this iteration (old_state) to preserve the original
        # behavior where OTP is only evaluated under LEVEL_FAN_MAX.
        self._check_otp_and_shutdown(snapshot, old_state)

    def _control_thermal_policy_via_bmc(self, snapshot):
        """
        Attempt to delegate thermal control to the BMC.
        """
        if not self.bmc.update_capability():
            return False

        if not self.bmc.ensure_policy_enabled():
            return False

        mac_temp_c = snapshot.mac_temp_c
        xcvr_temp_c = snapshot.max_sfp_temp_c
        xcvr_port = snapshot.max_sfp_port

        # If no transceivers are present, normalize the values so that
        # the BMC can clearly distinguish "no optics" from a valid port.
        if snapshot.sfp_present_count == 0:
            xcvr_temp_c = 0.0
            xcvr_port = 0

        if not self.bmc.report_thermal(mac_temp_c, xcvr_temp_c, xcvr_port):
            return False

        return True

    def adjust_tolerance(self):
        speed_normal = True
        for i, fan in enumerate(self.fans):
            curr_target_speed = fan.get_target_speed()
            if curr_target_speed != self.pre_target_speed[i]:
                self.set_fans_tolerance_mode("off")
                self.fan_timer_start = time.time()

            elif self.is_under_speed(fan) or self.is_over_speed(fan):
                speed_normal = False

            self.pre_target_speed[i] = curr_target_speed

        if speed_normal:
            self.set_fans_tolerance_mode("off")
            self.fan_timer_start = time.time()

        if self.is_timer_expired():
            self.set_fans_tolerance_mode("on")

    def manage_fans(self):
        """
        Main entry point called by the scheduler every MONITOR_INTERVAL
        seconds.

        This method collects temperatures, attempts to use BMC-based
        thermal control, and if that fails, falls back to CPU-based
        control.
        """
        snapshot = self.collect_temperature()

        try:
            if self._control_thermal_policy_via_bmc(snapshot):
                self.logger.debug(
                    "Thermal control handled by BMC this iteration."
                )
                self.adjust_tolerance()
                return
        except Exception as e:
            self.logger.error(
                "BMC thermal control failed with exception: %s, "
                "falling back to CPU-based control.",
                e
            )

        self.logger.debug("Fallback to CPU-based thermal control.")
        self._control_thermal_policy_via_cpu(snapshot)
        self.adjust_tolerance()

def is_database_ready():
    """
    Check whether the SONiC database service is active.

    Returns:
        True if database.service is active, False otherwise.
    """
    status, output = getstatusoutput_noshell(
        ["systemctl", "is-active", "database.service"]
    )
    return output.strip() == "active"


def usage(progname):
    """
    Print usage information for this script.
    """
    print(
        "Usage: %s [-d] [-l <log_file>]" % progname
    )
    print("  -d               Enable debug logging")
    print("  -l <log_file>    Specify log file path")

def main(argv):
    """
    Main entry point for the device monitor script.

    This function parses command-line arguments, initializes logging and
    platform access, and starts the aligned scheduler that periodically
    runs the fan management loop.
    """
    if os.geteuid() != 0:
        print("Error: Root privileges are required")
        sys.exit(1)

    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO

    try:
        # Options:
        #   -h : help
        #   -d : debug
        #   -l : log file (requires argument)
        opts, args = getopt.getopt(
            argv, "hdl:", ["debug", "lfile="]
        )
    except getopt.GetoptError:
        usage(sys.argv[0])
        return 1

    for opt, arg in opts:
        if opt == '-h':
            usage(sys.argv[0])
            return 0
        elif opt in ('-d', '--debug'):
            log_level = logging.DEBUG
        elif opt in ('-l', '--lfile'):
            log_file = arg

    monitor = DeviceMonitor(log_file, log_level)

    task = AlignedScheduler(
        monitor.manage_fans,
        interval_sec=MONITOR_INTERVAL,
        cleanup_func=monitor.cleanup,
        logger=logging.getLogger(FUNCTION_NAME)
    )
    task.start()
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
