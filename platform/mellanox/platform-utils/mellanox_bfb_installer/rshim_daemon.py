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

"""Manage the global RShim service and its selected-only configuration."""

import logging
import os
import subprocess
import tempfile
import time
from typing import Dict, Iterable, Optional, Set

logger = logging.getLogger(__name__)

RSHIM_CONFIG_PATH = "/etc/rshim.conf"
RSHIM_SERVICE = "rshim.service"
RSHIM_WAIT_TIMEOUT_SEC = 30
SYSTEMCTL_TIMEOUT_SEC = 40


def _run_systemctl(action: str) -> bool:
    """Run one systemctl action for the global RShim service."""
    try:
        result = subprocess.run(
            ["systemctl", action, RSHIM_SERVICE],
            timeout=SYSTEMCTL_TIMEOUT_SEC,
        )
    except subprocess.TimeoutExpired:
        logger.error(
            "Timed out after %d seconds waiting for systemctl to %s %s; "
            "the systemd job may still be running",
            SYSTEMCTL_TIMEOUT_SEC,
            action,
            RSHIM_SERVICE,
        )
        return False
    except OSError as error:
        logger.error("Failed to %s %s: %s", action, RSHIM_SERVICE, error)
        return False
    if result.returncode != 0:
        logger.error(
            "Failed to %s %s: exit code %d", action, RSHIM_SERVICE, result.returncode
        )
        return False
    return True


def restart_global_service() -> bool:
    """Restart the global RShim service.

    Returns:
        ``True`` when systemd reports success.
    """
    return _run_systemctl("restart")


def stop_global_service() -> bool:
    """Stop the global RShim service.

    Returns:
        ``True`` when systemd reports success.
    """
    return _run_systemctl("stop")


def _write_selected_config(
    path: str,
    mappings: Dict[str, str],
    selected_rshims: Iterable[str],
) -> None:
    """Atomically write an installer-only RShim configuration."""
    directory = os.path.dirname(path)
    os.makedirs(directory, exist_ok=True)
    fd, temporary_path = tempfile.mkstemp(prefix=f".{os.path.basename(path)}.", dir=directory)
    replaced = False
    try:
        with os.fdopen(fd, "w") as output:
            output.write(_render_selected_config(mappings, selected_rshims))
            output.flush()
            os.fsync(output.fileno())
            os.fchmod(output.fileno(), 0o644)
        os.replace(temporary_path, path)
        replaced = True
    finally:
        if not replaced:
            try:
                os.unlink(temporary_path)
            except OSError:
                pass


def _render_selected_config(
    mappings: Dict[str, str],
    selected_rshims: Iterable[str],
) -> str:
    """Render the temporary config with deterministic selected and excluded mappings."""
    selected = set(selected_rshims)
    # Force Mode lets the host request ownership when another backend, such as the BMC over
    # USB, already owns an RShim. It also creates /dev/rshim<N> before attachment, so callers
    # must verify DEV_NAME rather than treating the device path as proof of a valid backend.
    rendered_lines = ["FORCE_MODE 1"]
    for rshim, bus_id in mappings.items():
        owner = rshim if rshim in selected else "none"
        rendered_lines.append(f"{owner} pcie-{bus_id}")
    return "\n".join(rendered_lines) + "\n"


class RshimConfigTransaction:
    """Apply selected-only RShim configuration and remove it after installation."""

    def __init__(
        self,
        mappings: Dict[str, str],
        selected_rshims: Iterable[str],
        config_path: str = RSHIM_CONFIG_PATH,
    ) -> None:
        """Initialize the transaction.

        Args:
            mappings: Ordered mapping of every platform RShim to its PCI BDF.
            selected_rshims: RShim names that may be attached by the service.
            config_path: RShim configuration to update.
        """
        self.mappings = mappings
        self.selected_rshims = list(selected_rshims)
        self.config_path = config_path
        self.applied = False

    def apply(self) -> bool:
        """Replace any existing config with the temporary selected-only config."""
        try:
            _write_selected_config(
                self.config_path,
                self.mappings,
                self.selected_rshims,
            )
        except OSError as error:
            logger.error("Failed to apply selected-only RShim configuration: %s", error)
            return False
        self.applied = True
        return True

    def reapply(self, selected_rshims: Iterable[str]) -> bool:
        """Atomically narrow an active transaction to a new selected RShim set."""
        if not self.applied:
            logger.error("Cannot reapply RShim configuration before the transaction is active")
            return False
        try:
            selected = list(selected_rshims)
            _write_selected_config(
                self.config_path,
                self.mappings,
                selected,
            )
        except OSError as error:
            logger.error("Failed to reapply selected-only RShim configuration: %s", error)
            return False
        self.selected_rshims = selected
        return True

    def remove(self) -> bool:
        """Remove the installer-owned RShim configuration."""
        try:
            os.unlink(self.config_path)
        except FileNotFoundError:
            pass
        except OSError as error:
            logger.error("Failed to remove RShim configuration: %s", error)
            return False
        self.applied = False
        return True


def _read_rshim_backend(rshim: str) -> Optional[str]:
    """Read the backend name exported by one RShim misc device."""
    misc_path = f"/dev/{rshim}/misc"
    try:
        with open(misc_path, "r") as misc_file:
            for line in misc_file:
                fields = line.split(None, 1)
                if len(fields) == 2 and fields[0] == "DEV_NAME":
                    return fields[1].strip()
    except OSError:
        return None
    return None


def get_valid_selected_rshims(
    selected_mappings: Dict[str, str],
    excluded_rshims: Iterable[str],
    timeout_secs: int = RSHIM_WAIT_TIMEOUT_SEC,
) -> Set[str]:
    """Return selected RShims with valid backends after waiting for service startup.

    Args:
        selected_mappings: Selected RShim names mapped to expected PCI BDFs.
        excluded_rshims: Platform RShim names that must not exist.
        timeout_secs: Maximum number of seconds to wait.

    Returns:
        Selected RShim names with exact backend mappings. An unexpected excluded
        RShim makes the complete result unsafe and returns an empty set.
    """
    deadline = time.monotonic() + timeout_secs
    excluded = list(excluded_rshims)
    valid_rshims = set()
    excluded_valid = False
    while time.monotonic() < deadline:
        valid_rshims = {
            rshim
            for rshim, bus_id in selected_mappings.items()
            if os.path.exists(f"/dev/{rshim}/boot") and
            _read_rshim_backend(rshim) == f"pcie-{bus_id}"
        }
        excluded_valid = all(
            not os.path.exists(f"/dev/{rshim}") for rshim in excluded
        )
        if len(valid_rshims) == len(selected_mappings) and excluded_valid:
            return valid_rshims
        time.sleep(1)

    for rshim, bus_id in selected_mappings.items():
        if rshim not in valid_rshims:
            actual = _read_rshim_backend(rshim)
            logger.error(
                "%s maps to %s; expected pcie-%s", rshim, actual or "missing", bus_id
            )
    for rshim in excluded:
        if os.path.exists(f"/dev/{rshim}"):
            logger.error("Excluded RShim unexpectedly exists: %s", rshim)
    return valid_rshims if excluded_valid else set()
