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
import re
import subprocess
import sys
import tempfile
import time
from typing import Iterator, List, Optional


from mellanox_bfb_installer import bfb_file
from mellanox_bfb_installer import device_selection
from mellanox_bfb_installer import install_executor
from mellanox_bfb_installer import bfb_install_core
from mellanox_bfb_installer import platform_dpu
from mellanox_bfb_installer import tmfifo_bridge

SCRIPT_NAME = "sonic-bfb-installer"
LOCK_FILE = "/var/lock/sonic-bfb-installer.lock"

# Ephemeral DPU diagnostic-dump receiver (over tmfifo). Constants used both
# here (to start the receiver) and inside the per-DPU bf.cfg so the
# in-installer recovery path knows where to POST its mstdump to.
DUMP_BRIDGE_NAME = "bridge-tmfifo"
DUMP_BRIDGE_CIDR = "192.168.100.254/24"
DUMP_RECEIVER_BIND_IP = "192.168.100.254"
DUMP_RECEIVER_PORT = 8090
DUMP_OUTPUT_DIR_DEFAULT = "/var/log/sonic-bfb-installer/dumps"

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
--debug-shell\t\tOn DPU-side installer command failure, drop into an interactive recovery shell
-h|--help\t\tHelp"""


def print_usage() -> None:
    """Print usage matching shell script."""
    click.echo(USAGE_SYNTAX)
    click.echo(USAGE_ARGUMENTS)


def _dpu_name_to_id(dpu_name: str) -> int:
    """Convert a DPU name (e.g. 'dpu0', 'dpu12') to its numeric DPU ID."""
    match = re.fullmatch(r"dpu(\d+)", dpu_name)
    if not match:
        raise ValueError(f"Cannot extract DPU ID from DPU name: {dpu_name!r}")
    return int(match.group(1))


def _generate_additional_config_lines(
    target: device_selection.TargetInfo,
    debug_shell: bool = False,
    collect_dump_on_failure: bool = False,
    dump_upload_base_url: Optional[str] = None,
) -> str:
    """Generate additional config lines for a specific target DPU."""
    lines = []
    # Used by DPU to roughly set the clock after installation to a recent value from the NPU.
    lines.append(f"NPU_TIME={int(time.time())}\n")
    # Used by the in-DPU installer to compute the per-DPU tmfifo0 recovery address
    # (192.168.100.{1+DPU_ID}/24); see platform/nvidia-bluefield/installer/install.sh.j2.
    lines.append(f"DPU_ID={_dpu_name_to_id(target.dpu)}\n")
    if debug_shell:
        # When set, the DPU-side installer drops into an interactive recovery bash on
        # chroot command failure; see ex_chroot in install.sh.j2.
        lines.append("DEBUG_SHELL=true\n")
    if collect_dump_on_failure != bool(dump_upload_base_url):
        raise ValueError(
            "dump_upload_base_url is only allowed, and must be given, "
            "when collect_dump_on_failure is True"
        )
    if collect_dump_on_failure and dump_upload_base_url:
        # Tells the DPU-side ex_chroot failure handler to collect platform-dump.sh
        # output (including mstdump) and PUT it to the NPU's ephemeral HTTP receiver
        # over the tmfifo bridge; see ex_chroot in install.sh.j2.
        lines.append("COLLECT_DUMP_ON_FAILURE=true\n")
        lines.append(f"DUMP_UPLOAD_BASE_URL={dump_upload_base_url}\n")
    return "".join(lines)


def _add_additional_config_lines(
    targets: List[device_selection.TargetInfo],
    tempdir: str,
    debug_shell: bool = False,
    collect_dump_on_failure: bool = False,
    dump_upload_base_url: Optional[str] = None,
) -> None:
    """Update the targets to use temporary copies of the config files with additional content.

    For each target, create a temp copy of its config file with the original contents plus
    DPU-specific additional lines (from _generate_additional_config_lines), and update the
    target to use the new path. If the config file is None, create an empty temp copy and
    append the additional lines. Targets that share a config_path each get a distinct temp
    file since the additional lines vary per-DPU.
    """
    for idx, target in enumerate(targets):
        config_path = target.config_path
        base = os.path.basename(config_path) if config_path else "empty-config"
        fd, new_path = tempfile.mkstemp(suffix="", prefix=f"{base}.", dir=tempdir)
        with os.fdopen(fd, "w") as f:
            if config_path:
                with open(config_path, "r") as orig:
                    f.write(orig.read())
            f.write("\n")
            f.write(
                _generate_additional_config_lines(
                    target,
                    debug_shell=debug_shell,
                    collect_dump_on_failure=collect_dump_on_failure,
                    dump_upload_base_url=dump_upload_base_url,
                )
            )
            f.write("\n")
        targets[idx] = device_selection.TargetInfo(
            dpu=target.dpu,
            rshim=target.rshim,
            dpu_pci_bus_id=target.dpu_pci_bus_id,
            rshim_pci_bus_id=target.rshim_pci_bus_id,
            config_path=new_path,
        )


@contextmanager
def _dump_receiver_subprocess(
    output_dir: str,
    bind_ip: str = DUMP_RECEIVER_BIND_IP,
    port: int = DUMP_RECEIVER_PORT,
    verbose: bool = False,
) -> Iterator[Optional[subprocess.Popen]]:
    """Run the dump_receiver as a child process for the lifetime of the block.

    Started after the tmfifo bridge is up so the bind IP exists. SIGTERMed on
    exit; SIGKILL fallback after a short grace period. We never let receiver
    issues abort the actual BFB install — we log and continue with proc=None.
    """
    cmd = [
        sys.executable,
        "-m",
        "mellanox_bfb_installer.dump_receiver",
        "--bind-ip",
        bind_ip,
        "--port",
        str(port),
        "--output-dir",
        output_dir,
    ]
    if verbose:
        cmd.append("--verbose")
    logger.info("Starting DPU dump receiver: %s", " ".join(cmd))

    proc: Optional[subprocess.Popen] = None
    try:
        proc = subprocess.Popen(
            cmd,
            stdin=subprocess.DEVNULL,
            # Inherit our stdout/stderr so the receiver's logs flow through
            # the same console + syslog pipeline as the parent.
            stdout=None,
            stderr=None,
        )
    except OSError as e:
        logger.warning(
            "Could not start DPU dump receiver: %s. Continuing without auto-dump collection.",
            e,
        )
        yield None
        return

    # Give it a moment to bind so DPUs that fail their first ex_chroot don't
    # race the receiver coming up.
    time.sleep(0.5)
    if proc.poll() is not None:
        logger.warning(
            "DPU dump receiver exited immediately with code %s; continuing without it.",
            proc.returncode,
        )
        yield None
        return

    try:
        yield proc
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                logger.warning(
                    "DPU dump receiver did not exit after SIGTERM; sending SIGKILL"
                )
                proc.kill()
                try:
                    proc.wait(timeout=2.0)
                except subprocess.TimeoutExpired:
                    logger.error("DPU dump receiver process %s could not be killed", proc.pid)
        logger.info("DPU dump receiver stopped")


def _install_on_dpus(
    bfb_path: str,
    work_dir: str,
    rshims: Optional[str],
    dpus: Optional[str],
    verbose: bool,
    configs: Optional[str],
    temp_work_dir: str,
    debug_shell: bool = False,
    collect_dump_on_failure: bool = False,
    dump_output_dir: str = DUMP_OUTPUT_DIR_DEFAULT,
) -> None:
    """Install BFB image on DPUs connected to the host, including all preparatory steps, reset, etc.

    bfb_path is the prepared BFB path; work_dir is used for per-device result files.

    When ``collect_dump_on_failure`` is True, an ephemeral host bridge
    (``bridge-tmfifo`` @ ``192.168.100.254/24``) and HTTP dump receiver are
    started before the parallel install kicks off and torn down on the way
    out, success or failure. The DPU-side installer (install.sh.j2) uses the
    matching ``COLLECT_DUMP_ON_FAILURE`` / ``DUMP_UPLOAD_BASE_URL`` values in
    its bf.cfg to POST mstdump-bearing tarballs back to that receiver if its
    chroot install steps fail.
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

    dump_upload_base_url: Optional[str] = None
    if collect_dump_on_failure:
        dump_upload_base_url = (
            f"http://{DUMP_RECEIVER_BIND_IP}:{DUMP_RECEIVER_PORT}"
        )

    _add_additional_config_lines(
        targets,
        temp_work_dir,
        debug_shell=debug_shell,
        collect_dump_on_failure=collect_dump_on_failure,
        dump_upload_base_url=dump_upload_base_url,
    )

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

    # Stand up the tmfifo bridge + dump receiver only when feature is on and
    # we actually have DPUs to install on; tear them down on exit.
    if collect_dump_on_failure and targets:
        bridge = tmfifo_bridge.TmfifoBridge(
            bridge_name=DUMP_BRIDGE_NAME, bridge_cidr=DUMP_BRIDGE_CIDR
        )
        try:
            bridge.setup()
        except subprocess.CalledProcessError as e:
            logger.warning(
                "Could not bring up %s (%s). Proceeding without DPU dump collection.",
                DUMP_BRIDGE_NAME,
                e,
            )
            failed = install_executor.run_parallel(len(targets), _install_one_dpu)
            if failed:
                sys.exit(1)
            return

        try:
            with _dump_receiver_subprocess(dump_output_dir, verbose=verbose):
                failed = install_executor.run_parallel(len(targets), _install_one_dpu)
        finally:
            bridge.teardown()
        if failed:
            sys.exit(1)
        return

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
    debug_shell: bool,
    collect_dump_on_failure: bool,
    dump_output_dir: str,
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
                debug_shell=debug_shell,
                collect_dump_on_failure=collect_dump_on_failure,
                dump_output_dir=dump_output_dir,
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
@click.option(
    "--debug-shell",
    is_flag=True,
    default=False,
    help="On DPU-side installer command failure, drop into an interactive recovery shell.",
)
@click.option(
    "--collect-dump-on-failure/--no-collect-dump-on-failure",
    "collect_dump_on_failure",
    default=False,
    show_default=True,
    help=(
        "Stand up an ephemeral tmfifo bridge ("
        f"{DUMP_BRIDGE_NAME} @ {DUMP_BRIDGE_CIDR}) and HTTP receiver on "
        f"{DUMP_RECEIVER_BIND_IP}:{DUMP_RECEIVER_PORT} for the duration of the "
        "install so DPUs can upload diagnostic dumps if their installer "
        "fails. Disable for environments where tmfifo network setup is "
        "undesired (e.g. CI mocks)."
    ),
)
@click.option(
    "--dump-output-dir",
    type=str,
    default=DUMP_OUTPUT_DIR_DEFAULT,
    show_default=True,
    help="Where the dump receiver writes uploaded DPU dump files.",
)
def main(
    bfb: Optional[str],
    rshim: Optional[str],
    dpu: Optional[str],
    skip_extract: bool,
    verbose: bool,
    config: Optional[str],
    debug_shell: bool,
    collect_dump_on_failure: bool,
    dump_output_dir: str,
) -> None:
    """SONiC BFB Installer - install BFB image on DPUs connected to the host."""
    setup_log_handlers()
    _main(
        bfb=bfb,
        rshim=rshim,
        dpu=dpu,
        skip_extract=skip_extract,
        verbose=verbose,
        config=config,
        debug_shell=debug_shell,
        collect_dump_on_failure=collect_dump_on_failure,
        dump_output_dir=dump_output_dir,
    )


if __name__ == "__main__":
    main()
