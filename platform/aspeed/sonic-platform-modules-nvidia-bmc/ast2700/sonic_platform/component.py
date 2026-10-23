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

"""
SONiC Platform API - firmware Component(s) for the NVIDIA AST2700 BMC.

The BMC is updated over PLDM-for-Firmware-Update (DSP0267) carried on MCTP. We
drive the ``pldm-fw`` CLI (from the aspeed ``pldm-fw-cli`` package), targeting
the BMC firmware device by its MCTP ``net,eid`` address. The EID is discovered
at call time from ``mctpd`` (see :mod:`sonic_platform.mctp`) rather than being
hard-coded, and is assigned once at boot by the ``mctp-bmc-setup`` oneshot.

ComponentCPLD programs Lattice CPLDs via the ``ispvme`` CLI (from the aspeed
``ispvme`` package), targeting the JTAG character device exposed by the AST2700
JTAG controller (``/dev/jtag0``). The firmware image is a Lattice VME file.
"""

import os
import re
import subprocess
import sys

from sonic_py_common.logger import Logger

from sonic_platform import mctp
from sonic_platform import pldm_errors
from sonic_platform.utils import HW_MANAGEMENT_ROOT, hw_mgmt_path, read_sysfs_text

try:
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform.eeprom import EepromBMC
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = Logger()

# Installed by the aspeed pldm-fw-cli package.
PLDM_FW = "pldm-fw"

# Installed by the aspeed ispvme package.
ISPVME = "ispvme"

# Returned when a version cannot be determined; matches fwutil conventions.
VERSION_NA = "N/A"


class ComponentBMC(ComponentBase):
    """
    Firmware component for the NVIDIA AST2700 BMC, updated via PLDM over MCTP.
    """

    NAME = "BMC"
    DESCRIPTION = "Aspeed AST2700 BMC firmware (PLDM over MCTP)"

    # Activation model: the new firmware becomes active only after a BMC power
    # cycle (not a full system reboot).
    ACTIVATION_NOTIFICATION = (
        "A BMC power cycle (not a full system reboot) is required to activate "
        "the new BMC firmware."
    )

    # Inventory / pkg-info are quick; a firmware transfer can take a while.
    _INVENTORY_TIMEOUT = 60
    _PKGINFO_TIMEOUT = 30
    _UPDATE_TIMEOUT = 1800
    # A pre-update cancel is a single quick PLDM request/response.
    _CANCEL_TIMEOUT = 30

    # PLDM package component that carries AST2700 BMC firmware (.fwpkg).
    # DSP0240 ComponentClassification Firmware == 0x000D (13). pldm-fw may
    # render that as the name "Firmware" or as an unresolved "Value(13)".
    _PKG_COMPONENT_CLASSIFICATION = "firmware"
    _PKG_COMPONENT_CLASSIFICATION_VALUE = 13
    # ComponentIdentifier used by NVIDIA AST2700 BMC .fwpkg images.
    _PKG_COMPONENT_IDENTIFIER = 0x000a

    def __init__(self, network=None):
        """
        Args:
            network: optional MCTP network id to restrict EID discovery to. When
                ``None``, the IRoT (``mctpirot0``) network is resolved at address
                lookup time so pldm-fw always targets that link.
        """
        super().__init__()
        self._network = network
        self._eeprom = EepromBMC()

    def get_name(self):
        return self.NAME

    def get_description(self):
        return self.DESCRIPTION

    def get_presence(self):
        return True

    def get_model(self):
        return self._eeprom.get_part_number()

    def get_serial(self):
        return self._eeprom.get_serial_number()

    def get_status(self):
        return os.path.isdir(HW_MANAGEMENT_ROOT)

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False

    def get_firmware_update_notification(self, image_path):
        """
        Return the action required to complete a BMC firmware update.

        The BMC activates a newly transferred image only after a BMC power
        cycle, so always advertise that requirement.
        """
        return self.ACTIVATION_NOTIFICATION

    def _address(self):
        """Resolve the BMC's ``"net,eid"`` MCTP address (may raise MctpError)."""
        network = self._network
        if network is None:
            network = mctp.get_interface_network(mctp.IROT_INTERFACE)
        return mctp.mctp_address(network=network)

    def _run_pldm_fw(self, args, timeout):
        """Run ``pldm-fw`` with `args`; returns the CompletedProcess."""
        cmd = [PLDM_FW, *args]
        return subprocess.run(
            cmd,
            check=True,
            capture_output=True,
            text=True,
            timeout=timeout,
        )

    @staticmethod
    def _format_error(exc):
        stderr = getattr(exc, "stderr", None)
        if not stderr:
            return str(exc)
        return f"{exc}: {pldm_errors.annotate(stderr.strip())}"

    def get_firmware_version(self):
        """
        Return the BMC's currently active firmware version (read from HW).

        Queries ``pldm-fw inventory`` (GetFirmwareParameters) and reports the
        active version of the first component (``Component [0]``) rather than
        the top-level device/package version. Returns ``"N/A"`` if the FD
        cannot be reached.
        """
        try:
            out = self._run_pldm_fw(
                ["inventory", self._address()], self._INVENTORY_TIMEOUT
            ).stdout
        except (mctp.MctpError, subprocess.SubprocessError, OSError) as exc:
            logger.log_error(
                f"BMC firmware version query failed: {self._format_error(exc)}"
            )
            return VERSION_NA
        return self._parse_active_version(out) or VERSION_NA

    @staticmethod
    def _parse_active_version(text):
        """Extract Component [0]'s active version from ``pldm-fw inventory`` output.

        The first component block under ``Components:`` carries the BMC
        firmware version fwutil should display. Only the version string is
        returned; any trailing comparison stamp (e.g. ``[88006022]``) is
        stripped.
        """
        in_components = False
        in_first_component = False
        for line in text.splitlines():
            if not in_components:
                if re.match(r"\s*Components:\s*$", line):
                    in_components = True
                continue
            # Component blocks are introduced by a bare index header: [0], [1]...
            header = re.match(r"\s*\[(\d+)\]\s*$", line)
            if header:
                if header.group(1) == "0":
                    in_first_component = True
                    continue
                # Reached a later component; stop once [0] has been seen.
                if in_first_component:
                    break
                continue
            if not in_first_component:
                continue
            match = re.match(r"\s*Active Version:\s*(.+?)\s*$", line, re.IGNORECASE)
            if match:
                # Drop a trailing comparison stamp such as " [88006022]".
                version = re.sub(r"\s*\[[^\]]*\]\s*$", "", match.group(1)).strip()
                return version or None
        return None

    def get_available_firmware_version(self, image_path):
        """
        Return the BMC firmware version contained in `image_path` (a .fwpkg).

        Parses ``pldm-fw pkg-info``. Returns ``"N/A"`` on any error.
        """
        if not image_path or not os.path.isfile(image_path):
            logger.log_error(f"firmware image not found: {image_path}")
            return VERSION_NA
        try:
            out = self._run_pldm_fw(
                ["pkg-info", image_path], self._PKGINFO_TIMEOUT
            ).stdout
        except (subprocess.SubprocessError, OSError) as exc:
            logger.log_error(
                f"pkg-info failed for {image_path}: {self._format_error(exc)}"
            )
            return VERSION_NA
        return self._parse_component_version(out) or VERSION_NA

    @staticmethod
    def _classification_matches(text):
        """Return True if `text` names PLDM Firmware classification.

        Accepts both the symbolic name (``Firmware``) and the unresolved enum
        form emitted by ``pldm-fw`` (``Value(13)``).
        """
        classification = (text or "").strip().lower()
        if classification == ComponentBMC._PKG_COMPONENT_CLASSIFICATION:
            return True
        match = re.match(r"value\((\d+)\)\s*$", classification)
        return bool(
            match
            and int(match.group(1)) == ComponentBMC._PKG_COMPONENT_CLASSIFICATION_VALUE
        )

    @staticmethod
    def _parse_component_version(text):
        """Extract the BMC component ``version:`` from ``pldm-fw pkg-info`` output."""
        in_components = False
        current = {}

        def _block_matches(fields):
            if not ComponentBMC._classification_matches(
                fields.get("classification", "")
            ):
                return False
            identifier = fields.get("identifier")
            return identifier == ComponentBMC._PKG_COMPONENT_IDENTIFIER

        def _version_from_block(fields):
            if _block_matches(fields):
                return fields.get("version")
            return None

        for line in text.splitlines():
            if re.match(r"\s*Components:\s*$", line):
                in_components = True
                continue
            if not in_components:
                continue
            if re.match(r"\s*\d+:\s*$", line):
                version = _version_from_block(current)
                if version is not None:
                    return version
                current = {}
                continue
            match = re.match(r"\s*classification:\s*(.+?)\s*$", line, re.IGNORECASE)
            if match:
                current["classification"] = match.group(1)
                continue
            match = re.match(r"\s*identifier:\s*(.+?)\s*$", line, re.IGNORECASE)
            if match:
                try:
                    current["identifier"] = int(match.group(1).strip(), 0)
                except ValueError:
                    current.pop("identifier", None)
                continue
            match = re.match(r"\s*version:\s*(.+?)\s*$", line, re.IGNORECASE)
            if match:
                current["version"] = match.group(1)
        return _version_from_block(current)

    def _cancel_update(self, address):
        """Best-effort ``pldm-fw cancel`` to clear a stuck FD update state.

        A previously aborted update can leave the firmware device in a
        non-IDLE state (e.g. ``ReadyXfer``), which makes the next update fail
        with an "invalid state" protocol error. Issuing a cancel first returns
        the FD to IDLE. ``pldm-fw cancel`` exits 0 even when the FD is already
        IDLE, so this is safe to run unconditionally; any failure is logged and
        ignored so a transient cancel problem never blocks the real update.
        """
        try:
            self._run_pldm_fw(["cancel", address], self._CANCEL_TIMEOUT)
        except (mctp.MctpError, subprocess.SubprocessError, OSError) as exc:
            logger.log_warning(
                f"BMC firmware pre-update cancel failed (continuing): "
                f"{self._format_error(exc)}"
            )

    def _do_update(self, image_path, force_update=False):
        """Run the ``pldm-fw update`` firmware transfer (no BMC power cycle).

        Sends a preventative ``pldm-fw cancel`` first to clear any stuck FD
        update state left by a prior aborted update. The MCTP address is
        resolved once and reused for both the cancel and the update.
        """
        address = self._address()
        self._cancel_update(address)
        args = ["update"]
        if force_update:
            args.append("--force-update")
        args.extend([address, image_path, "-y"])
        self._run_pldm_fw(args, self._UPDATE_TIMEOUT)

    def _run_firmware_update(self, image_path, force_update=False):
        """
        Validate `image_path` and invoke :meth:`_do_update`.

        Returns:
            False if the image file does not exist; otherwise None on success.

        Raises:
            mctp.MctpError, subprocess.SubprocessError, OSError: if the update fails.
        """
        if not image_path or not os.path.isfile(image_path):
            logger.log_error(f"firmware image not found: {image_path}")
            return False
        self._do_update(image_path, force_update=force_update)

    def install_firmware(self, image_path, force_update=False):
        """
        Install BMC firmware from `image_path`.

        Args:
            image_path: path to a .fwpkg image.
            force_update: when True, pass ``--force-update`` to ``pldm-fw`` so
                the FD applies the image even when version checks would block it.

        Returns:
            bool: True on success, False on failure (or missing image).
        """
        try:
            if self._run_firmware_update(image_path, force_update=force_update) is False:
                return False
        except (mctp.MctpError, subprocess.SubprocessError, OSError) as exc:
            logger.log_error(
                f"BMC firmware install failed: {self._format_error(exc)}"
            )
            return False
        logger.log_info("BMC firmware install completed")
        return True

    def update_firmware(self, image_path, force_update=False):
        """
        Update BMC firmware from `image_path`.

        Performs the PLDM firmware transfer only; it does not power cycle the
        BMC. Activation requires a manual BMC power cycle (surfaced via
        :meth:`get_firmware_update_notification`), after which the new image
        becomes active. This matches how other SONiC components handle
        reboot/power-cycle activation and how ``fwutil`` presents it.

        Args:
            image_path: path to a .fwpkg image.
            force_update: when True, pass ``--force-update`` to ``pldm-fw``.

        Returns:
            False if `image_path` does not exist; otherwise None on success.

        Raises:
            RuntimeError: if the update fails.
        """
        # Intentional failure-contract difference from install_firmware:
        # install_firmware returns False on update failures, whereas
        # update_firmware honors the ComponentBase contract by raising
        # RuntimeError for operational failures and reserving False solely
        # for a missing image.
        try:
            if self._run_firmware_update(image_path, force_update=force_update) is False:
                return False
        except (mctp.MctpError, subprocess.SubprocessError, OSError) as exc:
            raise RuntimeError(
                f"BMC firmware update failed: {self._format_error(exc)}"
            ) from exc
        logger.log_info(
            "BMC firmware transfer completed; a BMC power cycle is required to activate"
        )
        return None


class ComponentCPLD(ComponentBase):
    """
    Firmware component for one Lattice CPLD (``CPLD<idx>``) on the NVIDIA
    AST2700 BMC, updated via ispvme over the Linux JTAG interface.

    Name, description and version format match the switch host's
    ``ComponentCPLD`` so both sides report the same CPLDs identically.
    """

    NAME = "CPLD{}"
    DESCRIPTION = "CPLD - Complex Programmable Logic Device"

    # Number of CPLDs whose version attributes hw-management publishes on the
    # BMC (system/cpld1..4_*). platform_components.json lists the same set.
    # Unlike the switch host, which reads its count from hw-management at
    # runtime, the set is fixed here for the AST2700 BMC.
    CPLD_COUNT = 4

    # Same layout as the switch host: zero-padded part number, then the
    # zero-padded major and minor revision (e.g. CPLD000454_REV0400).
    FW_VERSION_FORMAT = "CPLD{:0>6}_REV{:0>2}{:0>2}"

    # Lattice ispVM JTAG programming file extension.
    FIRMWARE_EXTENSION = ".vme"

    # Activation model: JTAG programming writes the CPLD's configuration flash;
    # the CPLD loads the new image only after a full system (AC) power cycle.
    # A VME file programs every device it targets on the JTAG chain, whichever
    # CPLD<idx> it is installed through, so say so rather than imply the update
    # is limited to the selected component.
    ACTIVATION_NOTIFICATION = (
        "A full system power cycle (AC) is required to activate the new CPLD "
        "firmware. The firmware image programs every CPLD it targets on the "
        "JTAG chain, regardless of which CPLD component is selected."
    )

    # AST2700 BMC Linux JTAG character device.
    JTAG_DEVICE = "/dev/jtag0"

    # hw-management sysfs paths for the CPLD part number and firmware version
    # (major and minor fields), written at startup from the CPLD registers.
    _PART_NUMBER_SYSFS = "system/cpld{}_pn"
    _VERSION_SYSFS = "system/cpld{}_version"
    _VERSION_MIN_SYSFS = "system/cpld{}_version_min"

    # Active-low gate signal: write 0 to enable JTAG burn mode, restore 1 after.
    _JTAG_BURN_EN_L_SYSFS = "system/GP_JTAG_BMC_CPLD_BURN_EN_L"

    # CPLD VME programming typically completes in under a minute; allow 5 min.
    _UPDATE_TIMEOUT = 300

    def __init__(self, idx=1, jtag_device=None, eeprom=None):
        """
        Args:
            idx: 1-based CPLD index; selects the ``cpld<idx>_*`` attributes.
            jtag_device: path to the JTAG character device. Defaults to
                ``JTAG_DEVICE`` (``/dev/jtag0``).
            eeprom: ``EepromBMC`` instance to delegate ``get_model``/
                ``get_serial`` to. Accept an injected instance (e.g. the
                chassis's own) rather than always constructing a new one, so
                the FRU isn't read via a duplicate ``ipmi-fru`` subprocess
                call. Defaults to a new ``EepromBMC()``.
        """
        super().__init__()
        self.idx = idx
        self._jtag_device = jtag_device or self.JTAG_DEVICE
        self._eeprom = eeprom or EepromBMC()

    @classmethod
    def get_component_list(cls, eeprom=None):
        """Return one component per CPLD, sharing ``eeprom`` between them."""
        eeprom = eeprom or EepromBMC()
        return [cls(idx, eeprom=eeprom) for idx in range(1, cls.CPLD_COUNT + 1)]

    def get_name(self):
        return self.NAME.format(self.idx)

    def get_description(self):
        return self.DESCRIPTION

    def get_presence(self):
        return True

    def get_model(self):
        return self._eeprom.get_part_number()

    def get_serial(self):
        return self._eeprom.get_serial_number()

    def get_status(self):
        return os.path.isdir(HW_MANAGEMENT_ROOT)

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False

    def get_firmware_update_notification(self, image_path):
        """
        Return the action required to complete a CPLD firmware update.

        The CPLD loads a newly programmed image only after a full system (AC)
        power cycle, so always advertise that requirement.
        """
        return self.ACTIVATION_NOTIFICATION

    def get_firmware_version(self):
        """
        Return the CPLD's active firmware version, e.g. ``CPLD000454_REV0400``.

        Reads the hw-management attributes written at startup from the CPLD
        registers. As on the switch host, a missing part number or minor
        revision reads as ``0``; returns ``"N/A"`` if the major revision is
        missing or unreadable.
        """
        major_sysfs = self._VERSION_SYSFS.format(self.idx)
        major = read_sysfs_text(hw_mgmt_path(major_sysfs), default=None)
        if not major:
            logger.log_warning(
                f"{self.get_name()} firmware version unavailable: {major_sysfs} not found"
            )
            return VERSION_NA
        part_number = read_sysfs_text(
            hw_mgmt_path(self._PART_NUMBER_SYSFS.format(self.idx)), default=None) or "0"
        minor = read_sysfs_text(
            hw_mgmt_path(self._VERSION_MIN_SYSFS.format(self.idx)), default=None) or "0"
        return self.FW_VERSION_FORMAT.format(part_number, major, minor)

    def get_available_firmware_version(self, image_path):
        """
        VME files do not carry a standard extractable version string; return
        ``"N/A"`` unconditionally.
        """
        return VERSION_NA

    def _run_ispvme(self, vme_path):
        """Run ``ispvme -d <jtag_device> <vme_path>``; returns CompletedProcess.

        Asserts the active-low JTAG burn-enable gate (GP_JTAG_BMC_CPLD_BURN_EN_L)
        before programming and restores it unconditionally afterward. Without
        asserting this signal the CPLD JTAG path is hardware-gated off and
        ispvme fails with a verification error even though the VME is valid.

        Restoration is attempted in a ``finally`` block, so it still runs even
        if programming is interrupted by something other than a subprocess
        error (e.g. KeyboardInterrupt) -- otherwise the board would be left
        with JTAG burn-enable asserted. A failure restoring the gate is
        raised rather than only logged, since install_firmware/update_firmware
        must not report success while that's true. If an exception is already
        propagating from the programming attempt, that's the more actionable
        diagnostic: it's preserved (this method's finally block only logs the
        restoration failure and lets it continue propagating unchanged).

        ispvme writes its progress and diagnostic text (including its pass/fail
        banner) via printf to stdout, not stderr; both streams are captured and
        merged here so a failure carries ispvme's actual diagnostic text rather
        than just its exit code.
        """
        burn_en_path = hw_mgmt_path(self._JTAG_BURN_EN_L_SYSFS)
        try:
            with open(burn_en_path, "w") as f:
                f.write("0\n")
        except OSError as exc:
            raise OSError(
                f"Failed to assert JTAG burn enable ({burn_en_path}): {exc}"
            ) from exc

        try:
            return subprocess.run(
                [ISPVME, "-d", self._jtag_device, vme_path],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=self._UPDATE_TIMEOUT,
            )
        finally:
            # Snapshot before the nested try/except below: inside that
            # except clause, sys.exc_info() would reflect the deassert
            # OSError itself, not whatever (if anything) is propagating
            # from the programming attempt above.
            programming_failed = sys.exc_info()[0] is not None
            try:
                with open(burn_en_path, "w") as f:
                    f.write("1\n")
            except OSError as deassert_exc:
                if programming_failed:
                    logger.log_warning(
                        f"Failed to deassert JTAG burn enable ({burn_en_path}): "
                        f"{deassert_exc}"
                    )
                else:
                    raise OSError(
                        f"ispvme succeeded but failed to deassert JTAG burn "
                        f"enable ({burn_en_path}): {deassert_exc}"
                    ) from deassert_exc

    @staticmethod
    def _format_error(exc):
        # stdout/output is the primary channel (see _run_ispvme); stderr is a
        # defensive fallback in case a caller passes it separately.
        output = getattr(exc, "stdout", None) or getattr(exc, "stderr", None) or b""
        if isinstance(output, bytes):
            output = output.decode(errors="replace")
        # ispvme's stdout can embed raw VME file bytes (via its printf-based
        # progress/diagnostic output), including NUL. syslog.syslog() raises
        # ValueError on an embedded NUL, which would otherwise mask this
        # error behind an unrelated exception.
        output = output.replace("\x00", "")
        return f"{exc}: {output.strip()}" if output.strip() else str(exc)

    def _check_file_validity(self, image_path):
        """
        Return True if `image_path` exists and has the expected VME extension.

        This is a filename-level sanity check only, not a parse of the VME
        file's contents — malformed VME content is caught by ispvme itself
        during programming.
        """
        if not image_path or not os.path.isfile(image_path):
            logger.log_error(f"CPLD firmware image not found: {image_path}")
            return False
        if os.path.splitext(image_path)[1] != self.FIRMWARE_EXTENSION:
            logger.log_error(
                f"CPLD firmware image {image_path} does not have the expected "
                f"{self.FIRMWARE_EXTENSION} extension"
            )
            return False
        return True

    def _run_firmware_update(self, image_path):
        """
        Validate `image_path` and run ispvme.

        Returns:
            False if the image file is invalid or the JTAG device is missing;
            otherwise None on success.

        Raises:
            subprocess.SubprocessError, OSError: if ispvme exits non-zero or
                cannot be launched.
        """
        if not self._check_file_validity(image_path):
            return False
        if not os.path.exists(self._jtag_device):
            logger.log_error(f"JTAG device not found: {self._jtag_device}")
            return False
        self._run_ispvme(image_path)

    def install_firmware(self, image_path):
        """
        Install CPLD firmware from `image_path` (a Lattice VME file).

        Returns:
            bool: True on success, False on failure (or missing image/device).
        """
        try:
            if self._run_firmware_update(image_path) is False:
                return False
        except (subprocess.SubprocessError, OSError) as exc:
            logger.log_error(
                f"CPLD firmware install failed: {self._format_error(exc)}"
            )
            return False
        logger.log_notice(f"{self.get_name()} firmware install completed")
        return True

    def update_firmware(self, image_path):
        """
        Update CPLD firmware from `image_path` (a Lattice VME file).

        Programs the CPLD only; activation requires a full system (AC) power
        cycle (surfaced via :meth:`get_firmware_update_notification`).

        Returns:
            False if `image_path` or the JTAG device does not exist; otherwise
            None on success.

        Raises:
            RuntimeError: if ispvme exits with a non-zero status.
        """
        try:
            if self._run_firmware_update(image_path) is False:
                return False
        except (subprocess.SubprocessError, OSError) as exc:
            raise RuntimeError(
                f"CPLD firmware update failed: {self._format_error(exc)}"
            ) from exc
        logger.log_notice(f"{self.get_name()} firmware update completed")
        return None
