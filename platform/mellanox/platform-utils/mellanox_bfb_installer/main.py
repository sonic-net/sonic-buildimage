#!/usr/bin/env python3
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
SONiC BFB Installer - Install BFB image on DPUs connected to the host.
"""

import click
from contextlib import contextmanager
import fcntl
import logging
import logging.handlers
import os
import sys
import tempfile
import time
from typing import Dict, List, Optional


from mellanox_bfb_installer import bfb_file
from mellanox_bfb_installer import device_selection
from mellanox_bfb_installer import install_executor
from mellanox_bfb_installer import bfb_install_core
from mellanox_bfb_installer import platform_dpu

SCRIPT_NAME = "sonic-bfb-installer"
LOCK_FILE = "/var/lock/sonic-bfb-installer.lock"

logger: Optional[logging.Logger] = logging.getLogger(SCRIPT_NAME)


def setup_log_handlers() -> None:
    """Configure a single logger that outputs to stdout and syslog.

    Prints just the message text to stdout.
    Prints a formatted message to syslog, including the script name prefix.
    """
    global logger
    logger = logging.getLogger()
    logger.handlers.clear()

    # Stdout: print message directly without any other log line prefix text.
    stdout_handler = logging.StreamHandler(sys.stdout)
    stdout_handler.setFormatter(logging.Formatter("%(message)s"))
    logger.addHandler(stdout_handler)

    # Syslog: includes script name prefix before the message.
    syslog = logging.handlers.SysLogHandler(address="/dev/log")
    syslog.setFormatter(logging.Formatter(f"{SCRIPT_NAME}: %(message)s"))
    logger.addHandler(syslog)


def set_logging_level(verbose: bool = False) -> None:
    global logger
    level = logging.DEBUG if verbose else logging.INFO
    logger = logging.getLogger()
    logger.setLevel(level)


@contextmanager
def _lock_file_or_exit(lock_file_path: str = LOCK_FILE):
    """
    Context manager for non-blocking lock on LOCK_FILE, ensuring only one running instance.

    Exits with code 1 if the lock file is already locked.
    """

    @contextmanager
    def _open_with_best_effort_close(path, mode):
        """Helper for managing opening/closing lock file with our special exception handling."""
        # Open, with custom exception class wrapping
        try:
            lock_file = open(path, mode)
        except Exception as e:
            logger.error(f"Could not open lock file {lock_file_path}: {e}")
            sys.exit(1)

        try:
            yield lock_file
        finally:
            # Close, but swallow errors after logging them
            try:
                lock_file.close()
            except Exception as e:
                logger.warning(f"Could not close lock file: {e}")

    with _open_with_best_effort_close(lock_file_path, "w") as lock_file:
        try:
            fcntl.flock(lock_file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as e:
            logger.debug(f"Could not lock file {lock_file_path}: {e}")
            logger.error(f"Another instance of {SCRIPT_NAME} is already running")
            sys.exit(1)

        yield lock_file

        pass  # Closing the file will unlock it.


def check_for_root() -> None:
    """Exit if not running as root."""
    if os.geteuid() != 0:
        logger.error("Please run the script in sudo mode")
        sys.exit(1)


USAGE_SYNTAX = (
    f"Syntax: {SCRIPT_NAME} -b|--bfb <BFB_Image_Path> --dpu|-d <dpu1,..dpuN> "
    "--verbose|-v --config|-c <Config_Path> --help|-h"
)
USAGE_ARGUMENTS = """Arguments:
-b|--bfb\t\tProvide custom path for bfb tar archive
-d|--dpu\t\tInstall on specified DPUs, mention all if installation is required on all connected DPUs
-s|--skip-extract\tSkip extracting the bfb image
-v|--verbose\t\tVerbose installation result output
-c|--config\t\tConfig file
-h|--help\t\tHelp"""


def print_usage() -> None:
    """Print usage matching shell script."""
    click.echo(USAGE_SYNTAX)
    click.echo(USAGE_ARGUMENTS)


def _generate_additional_config_lines() -> str:
    """Generate additional config lines."""
    # Used by DPU to roughly set the clock after installation to a recent value from the NPU.
    return f"NPU_TIME={int(time.time())}\n"


def _add_additional_config_lines(
    targets: List[device_selection.TargetInfo], temp_config_lines: str, tempdir: str
) -> None:
    """Update the targets to use temporary copies of the config files with additional content.

    For each config file encountered (by path), create a temp copy with original contents plus
    temp_config_lines, and update the target to use the new path. If the config file at a given
    path has already been processed, re-use the existing temp copy. If the config file is None,
    create an empty temp copy and append the additional lines.
    """
    processed_configs: Dict[Optional[str], str] = {}
    for target in targets:
        config_path = target.config_path
        if config_path in processed_configs:
            continue
        base = os.path.basename(config_path) if config_path else "empty-config"
        fd, new_path = tempfile.mkstemp(suffix="", prefix=f"{base}.", dir=tempdir)
        with os.fdopen(fd, "w") as f:
            if config_path:
                with open(config_path, "r") as orig:
                    f.write(orig.read())
            f.write("\n")
            f.write(temp_config_lines)
            f.write("\n")
        processed_configs[config_path] = new_path
    for idx, target in enumerate(targets):
        targets[idx] = device_selection.TargetInfo(
            dpu=target.dpu,
            rshim=target.rshim,
            dpu_pci_bus_id=target.dpu_pci_bus_id,
            rshim_pci_bus_id=target.rshim_pci_bus_id,
            config_path=processed_configs[target.config_path],  # Replacing
        )


def _install_on_dpus(
    bfb_path: str,
    work_dir: str,
    rshims: Optional[str],
    dpus: Optional[str],
    verbose: bool,
    configs: Optional[str],
    temp_work_dir: str,
) -> None:
    """Install BFB image on DPUs connected to the host, including all preparatory steps, reset, etc.

    bfb_path is the prepared BFB path; work_dir is used for per-device result files.
    """
    # Turn the user-provided parameters into a concrete list of dpus/devices/configs.
    # Then do the parallel installations.

    targets = device_selection.get_targets(
        dpus=dpus,
        rshims=rshims,
        configs=configs,
        script_name=SCRIPT_NAME,
        print_usage_callback=print_usage,
    )

    additional_config_lines = _generate_additional_config_lines()
    _add_additional_config_lines(targets, additional_config_lines, temp_work_dir)

    def _install_one_dpu(idx: int, child_pids: install_executor.PidCollection) -> int:
        target = targets[idx]
        rshim_name = target.rshim
        return bfb_install_core.full_install_bfb_on_device(
            rshim_name=rshim_name,
            rshim_id=rshim_name[5:] if rshim_name.startswith("rshim") else rshim_name,
            dpu_name=target.dpu,
            rshim_pci_bus_id=target.rshim_pci_bus_id,
            dpu_pci_bus_id=target.dpu_pci_bus_id,
            config_path=target.config_path,
            bfb_path=bfb_path,
            work_dir=work_dir,
            verbose=verbose,
            child_pids=child_pids,
        )

    failed = install_executor.run_parallel(len(targets), _install_one_dpu)
    if failed:
        sys.exit(1)


def _main(
    bfb: Optional[str],
    rshim: Optional[str],
    dpu: Optional[str],
    skip_extract: bool,
    verbose: bool,
    config: Optional[str],
) -> None:
    set_logging_level(verbose=verbose)
    check_for_root()

    if rshim:
        logger.warning(
            "DEPRECATION WARNING: The --rshim option is deprecated and will be removed in the future. Use --dpu instead."
        )

    platform_dpu.validate_platform()

    with _lock_file_or_exit():
        if not bfb:
            logger.debug("Error: bfb image is not provided.")
            print_usage()
            sys.exit(1)

        temp_work_dir = tempfile.TemporaryDirectory(prefix=SCRIPT_NAME + ".")
        try:
            bfb_path = bfb_file.prepare_bfb(bfb, temp_work_dir.name, skip_extract)

            _install_on_dpus(
                bfb_path,
                temp_work_dir.name,
                rshims=rshim,
                dpus=dpu,
                verbose=verbose,
                configs=config,
                temp_work_dir=temp_work_dir.name,
            )
        finally:
            try:
                temp_work_dir.cleanup()
            except Exception as e:
                logger.warning(f"Could not cleanup temporary work directory: {e}")


@click.command(
    context_settings=dict(help_option_names=["-h", "--help"], max_content_width=120),
    name=SCRIPT_NAME,
)
@click.option("-b", "--bfb", type=str, default=None, help="Provide custom path for bfb tar archive")
@click.option(
    "-r",
    "--rshim",
    type=str,
    default=None,
    hidden=True,
    help="(DEPRECATED: Use --dpu instead.) Install only on DPUs connected to rshim interfaces provided, mention all if installation is required on all connected DPUs",
)
@click.option(
    "-d",
    "--dpu",
    type=str,
    default=None,
    help="Install on specified DPUs, mention all if installation is required on all connected DPUs",
)
@click.option(
    "-s", "--skip-extract", is_flag=True, default=False, help="Skip extracting the bfb image"
)
@click.option(
    "-v", "--verbose", is_flag=True, default=False, help="Verbose installation result output"
)
@click.option("-c", "--config", type=str, default=None, help="Config file")
def main(
    bfb: Optional[str],
    rshim: Optional[str],
    dpu: Optional[str],
    skip_extract: bool,
    verbose: bool,
    config: Optional[str],
) -> None:
    """SONiC BFB Installer - install BFB image on DPUs connected to the host."""
    setup_log_handlers()
    _main(bfb=bfb, rshim=rshim, dpu=dpu, skip_extract=skip_extract, verbose=verbose, config=config)


if __name__ == "__main__":
    main()
