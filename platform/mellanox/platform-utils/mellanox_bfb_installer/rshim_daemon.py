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
Rshim daemon start/stop and wait for boot.
"""

import logging
import os
import subprocess
import time

logger = logging.getLogger(__name__)

RSHIM_BINARY = "/usr/sbin/rshim"
PIDFILE_DIR = "/var/run"
BOOT_WAIT_TIMEOUT_SEC = 10


def _pidfile_path(rid: str) -> str:
    """Return path to pidfile for the given rshim id (e.g. '0' -> /var/run/rshim_0.pid)."""
    return os.path.join(PIDFILE_DIR, f"rshim_{rid}.pid")


def start_rshim_daemon(rid: str, pci_bus: str) -> bool:
    """Start rshim daemon in background.

    Returns True on success, False on failure.
    """
    pidfile = _pidfile_path(rid)
    try:
        result = subprocess.run(
            [
                "start-stop-daemon",
                "--start",
                "--quiet",
                "--background",
                "--make-pidfile",
                "--pidfile",
                pidfile,
                "--exec",
                RSHIM_BINARY,
                "--",
                "-f",
                "-i",
                rid,
                "-d",
                f"pcie-{pci_bus}",
            ],
        )
        if result.returncode != 0:
            logger.error("Failed to start rshim for rshim%s: exit code %d", rid, result.returncode)
            return False
    except Exception as e:
        logger.error("Failed to start rshim for rshim%s: %s", rid, e)
        return False
    return True


def stop_rshim_daemon(rid: str) -> bool:
    """Stop rshim daemon if pidfile exists.

    Returns True on success, False on failure.
    """
    pidfile = _pidfile_path(rid)
    try:
        if not os.path.isfile(pidfile):
            logger.warning("Failed to stop rshim for rshim%s: missing pidfile %s", rid, pidfile)
            return False
        result = subprocess.run(
            [
                "start-stop-daemon",
                "--stop",
                "--quiet",
                "--pidfile",
                pidfile,
                "--remove-pidfile",
                "--retry",
                "TERM/15/KILL/5",
            ],
        )
        if result.returncode != 0:
            logger.warning("Failed to stop rshim for rshim%s: exit code %d", rid, result.returncode)
            return False
    except Exception as e:
        logger.error("Failed to stop rshim for rshim%s: %s", rid, e)
        return False
    return True


def wait_for_rshim_boot(rshim: str) -> bool:
    """Poll /dev/{rshim}/boot for up to BOOT_WAIT_TIMEOUT_SEC seconds.

    Returns True if boot file appeared, False otherwise.
    """
    boot_path = f"/dev/{rshim}/boot"
    timeout = BOOT_WAIT_TIMEOUT_SEC
    while timeout > 0:
        if os.path.exists(boot_path):
            return True
        time.sleep(1)
        timeout -= 1
    logger.error("%s: Error: Boot file did not appear after 10 seconds", rshim)
    return False
