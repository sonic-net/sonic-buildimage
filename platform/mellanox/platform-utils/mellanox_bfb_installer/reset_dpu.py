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
Reset a DPU.
"""

import logging
import sys
import threading
import time
from typing import Callable, Optional

import sonic_platform.dpuctlplat
from sonic_platform.dpuctlplat import DpuCtlPlat
from swsscommon.swsscommon import SonicV2Connector
from utilities_common.module import ModuleHelper

logger = logging.getLogger(__name__)

DPU_TRANSITION_WAIT_TIMEOUT_SECS = 13 * 60


def _is_chassis_module_table_present(dpu_name: str) -> bool:
    """Return True if STATE_DB has key CHASSIS_MODULE_TABLE|{dpu_name}."""
    try:
        db = SonicV2Connector()
        db.connect(db.STATE_DB)
        return bool(db.keys(db.STATE_DB, f"CHASSIS_MODULE_TABLE|{dpu_name.upper()}"))
    except Exception:
        logger.error("Error checking if CHASSIS_MODULE_TABLE is present for %s", dpu_name)
        return False


def _chassis_module_table_dpu_get_field(dpu_upper: str, field: str) -> Optional[str]:
    """Get one field from CHASSIS_MODULE_TABLE|<dpu_upper> in STATE_DB; None if missing or on error."""
    module_key = f"CHASSIS_MODULE_TABLE|{dpu_upper}"
    try:
        db = SonicV2Connector()
        db.connect(db.STATE_DB)
        v = db.get(db.STATE_DB, module_key, field)
        if v is None or v == "":
            return None
        return str(v)
    except Exception:
        return None


def wait_for_module_transition_to_complete(dpu: str) -> None:
    """Wait for CHASSIS_MODULE_TABLE transition_in_progress to clear (up to 13 min from start time)."""
    dpu_upper = dpu.upper()
    transition_in_progress = _chassis_module_table_dpu_get_field(
        dpu_upper, "transition_in_progress"
    )
    if transition_in_progress != "True":
        return

    raw_start = _chassis_module_table_dpu_get_field(dpu_upper, "transition_start_time")
    transition_start_time = None
    if raw_start is not None:
        try:
            transition_start_time = int(raw_start)
        except ValueError:
            pass
    if transition_start_time is None:
        transition_start_time = int(time.time())

    logger.info(
        "%s: Waiting for module transition to complete (timeout %.3g minutes from transition_start_time)",
        dpu,
        DPU_TRANSITION_WAIT_TIMEOUT_SECS / 60,
    )
    while True:
        time.sleep(2)
        logger.info("%s: Checking module transition status... ", dpu)
        transition_in_progress = _chassis_module_table_dpu_get_field(
            dpu_upper, "transition_in_progress"
        )
        if transition_in_progress != "True":
            logger.info("%s: Module transition flag cleared", dpu)
            break
        current_time = int(time.time())
        elapsed = current_time - transition_start_time
        if elapsed >= DPU_TRANSITION_WAIT_TIMEOUT_SECS:
            logger.info(
                "%s: Transition wait timeout (%.3g minutes) reached, proceeding",
                dpu,
                DPU_TRANSITION_WAIT_TIMEOUT_SECS / 60,
            )
            break


def _reboot_with_progress(dpu: str, reboot_fn: Callable[[], None]) -> None:
    """Run DPU reboot asynchronously and print elapsed time every 5s (matches sonic-bfb-installer.sh run_dpuctl_reset)."""
    start = time.time()
    err: list[Exception] = []

    def target() -> None:
        try:
            reboot_fn()
        except Exception as e:
            err.append(e)

    t = threading.Thread(target=target)
    t.start()
    while t.is_alive():
        elapsed = int(time.time() - start)
        sys.stdout.write(f"{dpu}: Reboot: {elapsed} seconds elapsed\n")
        t.join(timeout=5.0)
    t.join()
    total = int(time.time() - start)
    sys.stdout.write(f"{dpu}: Reboot: {total} seconds elapsed in total\n")
    if err:
        e = err[0]
        logger.error("An error occurred while rebooting %s: %s - %s", dpu, type(e).__name__, e)


def reset_dpu(dpu: str, use_verbose: bool) -> None:
    """Reset a DPU, including sensor management, PCI, and power cycling."""
    # Change dpuctlplat to use this script's custom root logger, rather than SysLogger.
    # This applies to all instances of DpuCtlPlat, including those created under-the-hood by the
    # module interface.
    sonic_platform.dpuctlplat.use_logger(logging.getLogger(sonic_platform.dpuctlplat.__name__))

    dpu_ctl = DpuCtlPlat(dpu)
    dpu_ctl.setup_logger(use_print=False, use_notice_level=True)
    dpu_ctl.verbosity = use_verbose

    # The module helper adds functionality for managing sensors/etc.
    # Only use this helper if PMON and chassisd are running. Otherwise, just use DpuCtlPlat to
    # reset the DPU and not handle sensors/etc.
    use_module_helper = _is_chassis_module_table_present(dpu.upper())
    if use_module_helper:
        # Use the module helper, which also handles sensors/etc.
        logger.info("Using ModuleHelper to reset %s", dpu)
        helper = ModuleHelper()
        helper.module_pre_shutdown(dpu)
        # Use DpuCtlPlat.dpu_reboot() directly, instead of ModuleHelper.reboot_module().
        # ModuleHelper.reboot_module() uses DpuCtlPlat.dpu_reboot() under the hood, but it doesn't
        # have the ability to pass force=True through to DpuCtlPlat. The module helper is a public
        # API and extending it to pass additional arguments without breaking other platforms will
        # take some work. Also, there are no options for verbosity passed through. So, just use
        # DpuCtlPlat directly, for now, until the ModuleHelper API is extended.
        dpu_ctl_dpu_reboot = lambda: dpu_ctl.dpu_reboot(forced=True, skip_pre_post=True)
        _reboot_with_progress(dpu, dpu_ctl_dpu_reboot)
        helper.module_post_startup(dpu)
    else:
        # Use DpuCtlPlat to reset the DPU and PCI bus only, not handle sensors/etc.
        logger.info("Using DpuCtlPlat to reset %s", dpu)
        dpu_ctl_dpu_reboot = lambda: dpu_ctl.dpu_reboot(forced=True, skip_pre_post=False)
        _reboot_with_progress(dpu, dpu_ctl_dpu_reboot)
