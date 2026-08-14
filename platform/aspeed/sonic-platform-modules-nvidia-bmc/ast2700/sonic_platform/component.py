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
"""

import os
import re
import subprocess

from sonic_py_common.logger import Logger

from sonic_platform import mctp

try:
    from sonic_platform_base.component_base import ComponentBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = Logger()

# Installed by the aspeed pldm-fw-cli package.
PLDM_FW = "pldm-fw"

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

    def get_name(self):
        return self.NAME

    def get_description(self):
        return self.DESCRIPTION

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
        return f"{exc}: {stderr.strip()}" if stderr else str(exc)

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
