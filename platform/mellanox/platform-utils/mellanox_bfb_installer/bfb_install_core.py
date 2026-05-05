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
The machinery to install the BFB image to a DPU.

The caller of this library is responsible for tasks such as fetching and preparing the BFB image,
getting the dpu and rshim device names, validating their presence on the pci bus, etc.
"""

import logging
import os
import random
import subprocess
import sys
import tempfile
import threading
import time
from typing import Optional

from mellanox_bfb_installer import install_executor
from mellanox_bfb_installer import platform_dpu
from mellanox_bfb_installer import reset_dpu
from mellanox_bfb_installer import rshim_daemon

logger = logging.getLogger(__name__)

BFB_INSTALL_TIMEOUT_SEC = 1200


def _run_bfb_install_image_delivery(
    *,
    rshim: str,
    rshim_id: str,
    bfb_path: str,
    result_file_path: str,
    child_pids: install_executor.PidCollection,
    config_path: Optional[str] = None,
    verbose: bool = False,
    timeout_secs: int = BFB_INSTALL_TIMEOUT_SEC,
) -> int:
    """Copy the bfb image to the device by running the `bfb-install` command.

    The caller must ensure the device is ready to receive the image, restart the device after, etc.

    Returns the bfb-install exit status (0 = success).
    """
    cmd = ["timeout", f"{timeout_secs}s", "bfb-install", "-b", bfb_path, "-r", rshim]
    if config_path:
        cmd.extend(["-c", config_path])
    cmd_str = " ".join(cmd)
    logger.info("Installing bfb image on DPU connected to %s using %s", rshim, cmd_str)

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    def capture_output():
        with open(result_file_path, "w") as result_file:
            for line in proc.stdout:
                result_file.write(
                    f"{rshim_id}: {line}" if line.endswith("\n") else f"{rshim_id}: {line}\n"
                )
                result_file.flush()
        if hasattr(proc.stdout, "close"):  # Bypass close() on mock objects that don't have it.
            proc.stdout.close()

    def progress_loop():
        elapsed = 0
        interval = random.randint(3, 10)
        while proc.poll() is None and elapsed < timeout_secs:
            time.sleep(interval)
            elapsed += interval
            if elapsed > timeout_secs:
                elapsed = timeout_secs
            sys.stdout.write(
                f"\r{rshim_id}: Installing... {elapsed}/{timeout_secs} seconds elapsed"
            )
            sys.stdout.flush()
        sys.stdout.write("\n")
        sys.stdout.flush()

    def maybe_output_result_file(exit_status: int):
        if verbose or exit_status != 0:
            with open(result_file_path) as f:
                sys.stdout.write(f.read())
            sys.stdout.flush()

    try:
        reader = None
        progress = None
        child_pids.append(proc.pid)
        reader = threading.Thread(target=capture_output)
        progress = threading.Thread(target=progress_loop)
        reader.start()
        progress.start()
        proc.wait()
    except Exception as e:
        logger.error("%s: Error: Installation failed on connected DPU! Exception: %s", rshim_id, e)
        maybe_output_result_file(1)
        return 1
    finally:
        if reader:
            reader.join()
        if progress:
            progress.join()
        child_pids.remove_if_contains(proc.pid)

    exit_status = proc.returncode
    if exit_status != 0:
        logger.error(
            "%s: Error: Installation failed on connected DPU! Exit code: %s", rshim_id, exit_status
        )
    else:
        logger.info("%s: Installation Successful", rshim_id)
    maybe_output_result_file(exit_status)
    return exit_status


def full_install_bfb_on_device(
    *,
    rshim_name: str,
    rshim_id: str,
    dpu_name: str,
    rshim_pci_bus_id: str,
    dpu_pci_bus_id: Optional[str],
    config_path: Optional[str],
    bfb_path: str,
    work_dir: str,
    verbose: bool,
    child_pids: install_executor.PidCollection,
) -> int:
    """Run the full install sequence for one device.

    The workflow includes starting/stopping the rshim daemon, managing PCI devices, uploading the
    BFB image, and resetting the DPU.

    Returns the bfb-install exit status (0 = success) or nonzero for other errors.
    Run the full install sequence for one device: rshim daemon, wait for boot,
    remove CX PCI device, run bfb-install, then stop daemon and reset DPU.
    Returns the bfb-install exit status (0 = success).
    """
    reset_dpu.wait_for_module_transition_to_complete(dpu_name)

    if not rshim_pci_bus_id:
        # Should not happen. Handled by caller.
        logger.error("Error: Could not find rshim PCI bus ID for DPU %s", dpu_name)
        return 1
    if not rshim_daemon.start_rshim_daemon(rshim_id, rshim_pci_bus_id):
        logger.info(
            "%s: Rshim couldn't start. Attempting to stop it and then start it again.", rshim_id
        )
        stop_ret = rshim_daemon.stop_rshim_daemon(rshim_id)
        if not rshim_daemon.start_rshim_daemon(rshim_id, rshim_pci_bus_id):
            logger.error(
                "%s: Rshim couldn't start after stopping. (Stopping returned %s.) Giving up.",
                rshim_id,
                stop_ret,
            )
            return 1
    try:
        if not rshim_daemon.wait_for_rshim_boot(rshim_name):
            return 1
        if dpu_pci_bus_id:
            platform_dpu.remove_cx7_pci_device(dpu_pci_bus_id, f"{rshim_id}: ")
        result_file = tempfile.NamedTemporaryFile(
            dir=work_dir, prefix="result_file.", suffix="", delete=False
        )
        result_file.close()
        try:
            return _run_bfb_install_image_delivery(
                rshim=rshim_name,
                rshim_id=rshim_id,
                bfb_path=bfb_path,
                result_file_path=result_file.name,
                child_pids=child_pids,
                config_path=config_path,
                verbose=verbose,
            )
        finally:
            try:
                os.unlink(result_file.name)
            except OSError:
                pass
    finally:
        rshim_daemon.stop_rshim_daemon(rshim_id)
        logger.info("%s: Resetting DPU %s", rshim_id, dpu_name)
        reset_dpu.reset_dpu(dpu_name, verbose)
