# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Install one BFB on selected DPUs through a single DOCA Installer process."""

from contextlib import contextmanager
from dataclasses import dataclass, field
import glob
import logging
import os
import shutil
import signal
import subprocess
import sys
import threading
import time
from typing import Dict, Iterator, List, Optional, Set

from mellanox_bfb_installer import device_selection
from mellanox_bfb_installer import install_executor
from mellanox_bfb_installer import platform_dpu
from mellanox_bfb_installer import reset_dpu
from mellanox_bfb_installer import rshim_daemon

logger = logging.getLogger(__name__)

DOCA_INSTALLER_BINARY = "doca-installer"
DOCA_INSTALL_TIMEOUT_SEC = 1200
DOCA_KILL_WAIT_SEC = 5
DOCA_TIMEOUT_EXIT_CODE = 124
INSTALL_PROGRESS_INTERVAL_SEC = 30
MST_STOP_TIMEOUT_SEC = 30
SYSTEMD_UNIT_START_TIMEOUT_SEC = 5
SYSTEMD_POLL_INTERVAL_SEC = 0.1

# DOCA Installer creates one timestamped directory per run and writes its own logs there,
# including each DPU's bfb-install output. Those files are the only record of what the DPU
# itself reported, and they are written whether or not the bundle supports firmware-version
# verification, so point the operator at them when an installation fails.
DOCA_LOG_ROOT = "/var/log/doca_installer_logs"
DOCA_LOG_DIR_GLOB = "doca_installer_*"

# Backoff (seconds) between RShim preflight retries. A previous failed deployment can leave the
# RShim in a state where the first attempt does not expose the selected devices; stop the service
# and retry with increasing backoff before giving up. No DPU reset or other recovery is attempted.
RSHIM_PREFLIGHT_RETRY_BACKOFF_SEC = (3, 5, 10)
HANDLED_SIGNALS = (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)
AUTO_RECOVERY_DB_ARGS = ("DEVICE_METADATA|localhost", "dpu_auto_recovery")

def _rshim_id(rshim: str) -> str:
    """Return the numeric portion of an RShim name (e.g. 'rshim0' -> '0')."""
    return rshim[5:] if rshim.startswith("rshim") else rshim


@dataclass
class _TransactionState:
    """How far the installation progressed, which decides what has to be undone."""

    active_targets: List[device_selection.TargetInfo] = field(default_factory=list)
    unbound_targets: List[device_selection.TargetInfo] = field(default_factory=list)
    failed_preflight_rshims: Set[str] = field(default_factory=set)
    doca_started: bool = False
    doca_started_at: float = 0.0
    doca_status: int = 1


def build_doca_command(
    *,
    bfb_path: str,
    selected_rshims: List[str],
    config_path: Optional[str],
) -> List[str]:
    """Construct the DOCA Installer 2.0.3 command.

    Args:
        bfb_path: BFB payload path.
        selected_rshims: Ordered RShim names selected by the user.
        config_path: Optional shared installer configuration.

    Returns:
        Command arguments suitable for ``subprocess.Popen``.
    """
    cmd = [DOCA_INSTALLER_BINARY, "-b", bfb_path]
    cmd.extend(["-r", *selected_rshims])
    if config_path:
        cmd.extend(["-c", config_path])
    return cmd


def _find_latest_doca_log_dir(
    log_root: str = DOCA_LOG_ROOT,
    newer_than: Optional[float] = None,
) -> Optional[str]:
    """Return the directory DOCA Installer wrote its own logs to for the current run.

    Args:
        log_root: Parent directory DOCA Installer creates its per-run directories in.
        newer_than: Ignore directories older than this epoch time.

    Returns:
        Path of the newest run directory, or ``None`` when there is none to report.
    """
    try:
        paths = glob.glob(os.path.join(log_root, DOCA_LOG_DIR_GLOB))
    except OSError as error:
        logger.warning("Could not look for DOCA Installer logs in %s: %s", log_root, error)
        return None

    try:
        candidates = [
            path
            for path in paths
            if os.path.isdir(path) and
            (newer_than is None or os.path.getmtime(path) >= newer_than)
        ]
    except OSError as error:
        logger.warning("Could not inspect DOCA Installer logs in %s: %s", log_root, error)
        return None
    return max(candidates) if candidates else None


def _print_install_progress(
    stop_event: threading.Event,
    interval_secs: int,
    total_secs: int,
) -> None:
    start = time.monotonic()
    while not stop_event.wait(interval_secs):
        elapsed = int(time.monotonic() - start)
        sys.stdout.write(f"Installing... {elapsed}/{total_secs} seconds elapsed\n")
        sys.stdout.flush()


def _kill_doca_unit(unit: str) -> None:
    """Kill the transient DOCA control group and wait for systemd to release it.

    Args:
        unit: Transient service whose control group holds DOCA Installer.
    """
    try:
        subprocess.run(
            ["systemctl", "kill", "--kill-whom=all", "--signal=SIGKILL", unit],
            timeout=DOCA_KILL_WAIT_SEC,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        deadline = time.monotonic() + DOCA_KILL_WAIT_SEC
        while time.monotonic() < deadline:
            result = subprocess.run(
                ["systemctl", "show", "--property=ActiveState", "--value", unit],
                timeout=DOCA_KILL_WAIT_SEC,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
                check=False,
            )
            if result.returncode != 0:
                logger.error(
                    "Could not query DOCA Installer systemd unit %s after SIGKILL",
                    unit,
                )
                return
            if result.stdout.strip() in ("", "inactive", "failed"):
                return
            time.sleep(SYSTEMD_POLL_INTERVAL_SEC)
    except subprocess.TimeoutExpired:
        # Reached while the caller ignores signals, so an unbounded systemctl call would
        # hang cleanup with no way for the operator to interrupt it.
        logger.error(
            "Timed out after %d seconds while stopping DOCA Installer systemd unit %s",
            DOCA_KILL_WAIT_SEC,
            unit,
        )
        return
    except OSError as error:
        logger.error("Could not kill DOCA Installer control group %s: %s", unit, error)
        return
    logger.error(
        "DOCA Installer control group %s was still not released %d seconds after SIGKILL; "
        "continuing with cleanup",
        unit,
        DOCA_KILL_WAIT_SEC,
    )


def _wait_for_doca_unit(proc: subprocess.Popen, unit: str) -> bool:
    """Wait until systemd has registered the transient DOCA service."""
    deadline = time.monotonic() + SYSTEMD_UNIT_START_TIMEOUT_SEC
    try:
        while time.monotonic() < deadline:
            result = subprocess.run(
                ["systemctl", "show", "--property=LoadState", "--value", unit],
                timeout=SYSTEMD_UNIT_START_TIMEOUT_SEC,
                capture_output=True,
                text=True,
                check=False,
            )
            if result.stdout.strip() == "loaded":
                return True
            # With --wait, an exited runner means the transient unit was already started,
            # completed, and possibly collected before systemctl could observe it.
            if proc.poll() is not None:
                return True
            time.sleep(SYSTEMD_POLL_INTERVAL_SEC)
    except subprocess.TimeoutExpired:
        # The caller blocks signals around this wait, so the query has to be bounded.
        logger.error(
            "Timed out after %d seconds while waiting for DOCA Installer systemd unit %s",
            SYSTEMD_UNIT_START_TIMEOUT_SEC,
            unit,
        )
    return False


@contextmanager
def _terminate_doca_on_signal() -> Iterator[None]:
    """Abort on a transaction signal and protect cleanup from repeated signals."""
    interrupted = False

    def handle_signal(signum, _frame) -> None:
        nonlocal interrupted
        if interrupted:
            return
        interrupted = True
        logger.warning("DOCA Installer interrupted by signal %s", signum)
        # Prevent another transaction signal from interrupting cleanup triggered by this one.
        for handled_signal in HANDLED_SIGNALS:
            signal.signal(handled_signal, signal.SIG_IGN)
        raise SystemExit(1)

    previous_handlers = {signum: signal.getsignal(signum) for signum in HANDLED_SIGNALS}
    for signum in HANDLED_SIGNALS:
        signal.signal(signum, handle_signal)
    try:
        yield
    finally:
        for signum, previous_handler in previous_handlers.items():
            signal.signal(signum, previous_handler)


def _run_doca_installer(
    cmd: List[str],
    *,
    timeout_secs: int,
    progress_interval_secs: int = INSTALL_PROGRESS_INTERVAL_SEC,
) -> int:
    """Run DOCA Installer with its output inherited by the caller.

    Args:
        cmd: Command produced by :func:`build_doca_command`.
        timeout_secs: Maximum installer runtime, also reported by the progress output.
        progress_interval_secs: Console progress interval.

    Returns:
        DOCA Installer exit status, 124 on timeout, or one when launch fails.
    """
    logger.info("Flashing BFB via: %s", " ".join(cmd))
    stop_progress = threading.Event()
    progress_thread = threading.Thread(
        target=_print_install_progress,
        args=(stop_progress, progress_interval_secs, timeout_secs),
    )
    unit = f"sonic-bfb-installer-doca-{os.getpid()}-{time.monotonic_ns()}.service"
    runner_cmd = [
        "systemd-run",
        "--quiet",
        "--wait",
        "--pipe",
        "--collect",
        "--service-type=oneshot",
        "--property=TimeoutStartSec=infinity",
        "--expand-environment=no",
        f"--working-directory={os.getcwd()}",
        "--setenv=PATH",
        f"--unit={unit}",
        "--property=KillMode=control-group",
        *cmd,
    ]
    proc = None
    timed_out = False
    previous_mask = signal.pthread_sigmask(signal.SIG_BLOCK, HANDLED_SIGNALS)
    try:
        try:
            proc = subprocess.Popen(runner_cmd)
            if not _wait_for_doca_unit(proc, unit):
                logger.error("DOCA Installer systemd unit %s did not start", unit)
                return 1
        except OSError:
            logger.exception("Failed to launch DOCA Installer")
            return 1
        finally:
            signal.pthread_sigmask(signal.SIG_SETMASK, previous_mask)

        # Started inside the try so that a thread the runtime refuses to create still leaves
        # DOCA Installer killed rather than running without anything watching it.
        progress_thread.start()
        try:
            proc.wait(timeout=timeout_secs)
        except subprocess.TimeoutExpired:
            timed_out = True
            logger.error(
                "DOCA Installer timed out after %d seconds",
                timeout_secs,
            )
    finally:
        if proc is not None:
            _kill_doca_unit(unit)
            if proc.poll() is None:
                proc.kill()
                proc.poll()
        stop_progress.set()
        if progress_thread.is_alive():
            progress_thread.join()

    status = DOCA_TIMEOUT_EXIT_CODE if timed_out else proc.returncode
    if status != 0:
        logger.error("DOCA Installer failed with exit code %s", status)
    return status


def _reset_targets_in_parallel(
    targets: List[device_selection.TargetInfo],
    verbose: bool,
) -> int:
    if not targets:
        return 0

    def _reset_one(index: int) -> int:
        target = targets[index]
        logger.info("Resetting DPU %s", target.dpu)
        if not reset_dpu.reset_dpu(target.dpu, verbose):
            logger.error("Resetting DPU %s failed", target.dpu)
            return 1
        return 0

    return install_executor.run_parallel(len(targets), _reset_one)


def _start_and_validate_rshim_service(
    selected_mappings: Dict[str, str],
    excluded_rshims: List[str],
) -> Set[str]:
    """Restart the global service and return selected RShims with valid mappings.

    Args:
        selected_mappings: Selected RShim names mapped to their expected PCI BDFs.
        excluded_rshims: Platform RShim names that must not appear.

    Returns:
        Selected RShim names with valid backend mappings after all retries.
    """
    retry_count = len(RSHIM_PREFLIGHT_RETRY_BACKOFF_SEC)
    valid_rshims = set()
    for retry, backoff in enumerate((0,) + RSHIM_PREFLIGHT_RETRY_BACKOFF_SEC):
        if backoff:
            logger.info(
                "RShim preflight failed; stopping and retrying in %ds (retry %d/%d)",
                backoff,
                retry,
                retry_count,
            )
            rshim_daemon.stop_global_service()
            time.sleep(backoff)
        if rshim_daemon.restart_global_service():
            valid_rshims = rshim_daemon.get_valid_selected_rshims(
                selected_mappings, excluded_rshims
            )
            if valid_rshims == set(selected_mappings):
                return valid_rshims
        else:
            valid_rshims = set()
    logger.error(
        "RShim service did not expose all selected RShims after %d attempts",
        retry_count + 1,
    )
    return valid_rshims


def _flash_targets(
    state: _TransactionState,
    *,
    targets: List[device_selection.TargetInfo],
    transaction: rshim_daemon.RshimConfigTransaction,
    all_mappings: Dict[str, str],
    selected_mappings: Dict[str, str],
    bfb_path: str,
    config_path: Optional[str],
    timeout_secs: int,
) -> None:
    """Bring up RShim, detach the CX functions and flash the surviving targets.

    Progress is recorded in ``state`` rather than returned, so the caller can undo exactly what
    was done even when this function is unwound by an interruption.

    Args:
        state: Transaction progress updated in place.
        targets: Ordered selected DPU targets.
        transaction: Applied RShim configuration transaction, reapplied when targets drop out.
        all_mappings: Every platform RShim name mapped to its PCI BDF.
        selected_mappings: Selected RShim names mapped to their PCI BDFs.
        bfb_path: BFB payload path.
        config_path: Optional shared installer configuration.
        timeout_secs: Installation timeout enforced on the DOCA Installer process.
    """
    active_mappings = selected_mappings
    while active_mappings:
        excluded_rshims = [
            rshim for rshim in all_mappings if rshim not in active_mappings
        ]
        valid_rshims = _start_and_validate_rshim_service(
            active_mappings, excluded_rshims
        )
        if valid_rshims == set(active_mappings):
            break

        newly_failed = set(active_mappings) - valid_rshims
        state.failed_preflight_rshims.update(newly_failed)
        logger.error(
            "Excluding DPUs whose RShims failed preflight: %s",
            ", ".join(sorted(newly_failed)),
        )
        if not valid_rshims:
            return
        if not rshim_daemon.stop_global_service() or not transaction.reapply(
            valid_rshims
        ):
            return
        active_mappings = {
            rshim: bus_id
            for rshim, bus_id in active_mappings.items()
            if rshim in valid_rshims
        }

    state.active_targets = [
        target for target in targets if target.rshim in active_mappings
    ]
    # Flashing a DPU whose CX function is still bound would pull the device out from under the
    # host driver, so stop before starting DOCA Installer rather than during the installation.
    for target in state.active_targets:
        if not target.dpu_pci_bus_id:
            continue
        # Record recovery intent before unbinding so an interruption cannot leave a detached
        # CX function outside the transaction state. Resetting conservatively is also safer
        # when a failed sysfs write may have partially changed the device state.
        state.unbound_targets.append(target)
        if not platform_dpu.unbind_cx7_pci_device(
            target.dpu_pci_bus_id, f"{target.rshim}: "
        ):
            logger.error(
                "Could not unbind the CX PCI device of %s; aborting before installation",
                target.rshim,
            )
            return

    cmd = build_doca_command(
        bfb_path=bfb_path,
        selected_rshims=[target.rshim for target in state.active_targets],
        config_path=config_path,
    )
    # Keep the logged command and transient service pinned to the executable selected from PATH.
    cmd[0] = shutil.which(cmd[0]) or cmd[0]
    state.doca_started = True
    state.doca_started_at = time.time()
    try:
        state.doca_status = _run_doca_installer(
            cmd,
            timeout_secs=timeout_secs,
        )
    except Exception as error:
        logger.error("DOCA Installer execution failed: %s", error)


def _stop_mst() -> bool:
    """Stop the MST service started by DOCA Installer."""
    try:
        result = subprocess.run(["mst", "stop"], timeout=MST_STOP_TIMEOUT_SEC)
    except subprocess.TimeoutExpired:
        logger.error(
            "Timed out after %d seconds while stopping MST",
            MST_STOP_TIMEOUT_SEC,
        )
        return False
    except OSError as error:
        logger.error("Could not stop MST service: %s", error)
        return False

    if result.returncode != 0:
        logger.error("Could not stop MST service; 'mst stop' exited with %s", result.returncode)
        return False
    return True


def _finalize_transaction(
    state: _TransactionState,
    transaction: rshim_daemon.RshimConfigTransaction,
    verbose: bool,
) -> int:
    """Undo the host-side changes and reset the DPUs that were exposed to DOCA Installer.

    Args:
        state: Transaction progress recorded by :func:`_flash_targets`.
        transaction: RShim configuration transaction to remove.
        verbose: Whether to print detailed reset output.

    Returns:
        Zero when the RShim service stopped, its configuration was removed and every reset
        that had to run succeeded.
    """
    if state.doca_started and state.doca_status != 0:
        logger.error(
            "Installation failed; attempting to stop RShim, remove its configuration "
            "and reset the active DPUs for recovery."
        )
        doca_log_dir = _find_latest_doca_log_dir(newer_than=state.doca_started_at)
        if doca_log_dir:
            logger.error(
                "DOCA Installer's own logs, including each DPU's bfb-install output, are in %s",
                doca_log_dir,
            )

    if state.doca_started and state.doca_status == 0:
        logger.info(
            "DOCA Installer completed for active RShims: %s",
            ", ".join(target.rshim for target in state.active_targets),
        )
        for target in state.active_targets:
            logger.info("%s: Installation Successful", _rshim_id(target.rshim))

    # DOCA Installer starts MST but does not stop it. Stop it before the reset changes the PCI
    # devices so the MST driver does not react to those expected changes as device errors.
    mst_stopped = not state.doca_started or _stop_mst()

    # RShim has to release the DPUs before they can be reset from the host, so a stop failure
    # blocks the reset. A config removal failure does not: it only leaves host configuration behind.
    rshim_stopped = rshim_daemon.stop_global_service()
    if not rshim_stopped:
        logger.error("RShim service could not be stopped; not resetting DPUs")
    config_removed = transaction.remove()
    if not config_removed:
        logger.error("RShim config could not be removed")

    reset_failures = 0
    targets_to_reset = state.active_targets if state.doca_started else state.unbound_targets
    if rshim_stopped and targets_to_reset:
        # A successful unbind must be undone even when a later unbind prevents the flash from
        # starting; resetting only those detached DPUs restores their host PCI connectivity.
        reset_failures = _reset_targets_in_parallel(targets_to_reset, verbose)

    return 1 if (not mst_stopped or not rshim_stopped or not config_removed or reset_failures) else 0


def _get_dpu_auto_recovery() -> Optional[str]:
    """Get the global DPU auto-recovery state from CONFIG_DB."""
    try:
        state = subprocess.check_output(
            [
                "sonic-db-cli",
                "CONFIG_DB",
                "HGET",
                *AUTO_RECOVERY_DB_ARGS,
            ],
            text=True,
        ).strip()
    except (OSError, subprocess.CalledProcessError) as error:
        logger.error("Could not read dpu_auto_recovery: %s", error)
        return None
    if not state:
        logger.info("dpu_auto_recovery is not configured; using platform default disable")
        return "disable"
    return state


def _set_dpu_auto_recovery(state: str) -> bool:
    """Set the global DPU auto-recovery state through CONFIG_DB."""
    try:
        # sonic-db-cli prints the redis reply, which is noise in the installer output.
        subprocess.check_output(
            [
                "sonic-db-cli",
                "CONFIG_DB",
                "HSET",
                *AUTO_RECOVERY_DB_ARGS,
                state,
            ]
        )
        return True
    except (OSError, subprocess.CalledProcessError) as error:
        logger.error("Could not set dpu_auto_recovery to %s: %s", state, error)
        return False


def install_bfb_on_targets(
    *,
    targets: List[device_selection.TargetInfo],
    bfb_path: str,
    config_path: Optional[str],
    verbose: bool,
    timeout_secs: int = DOCA_INSTALL_TIMEOUT_SEC,
) -> int:
    """Flash all selected targets with one DOCA Installer process.

    Args:
        targets: Ordered selected DPU targets.
        bfb_path: BFB payload path.
        config_path: Optional shared installer configuration.
        verbose: Whether to print detailed DPU reset output.
        timeout_secs: Installation timeout enforced on the DOCA Installer process.

    Returns:
        Zero when DOCA Installer reported success and cleanup and the resets succeeded. A
        success that DOCA Installer could not verify through exported YAML also returns zero,
        because a missing results file is not evidence that BFB delivery failed.
    """
    if not targets:
        logger.error("No DPU selected for installation")
        return 1

    if shutil.which(DOCA_INSTALLER_BINARY) is None:
        logger.error(
            "DOCA Installer executable '%s' was not found in PATH",
            DOCA_INSTALLER_BINARY,
        )
        return 1

    try:
        all_mappings = platform_dpu.get_rshim_pci_mappings()
    except ValueError as error:
        logger.error("Invalid platform RShim mapping: %s", error)
        return 1

    selected_mappings = {target.rshim: target.rshim_pci_bus_id for target in targets}
    mismatched = [
        rshim for rshim, bus_id in selected_mappings.items() if all_mappings.get(rshim) != bus_id
    ]
    if mismatched:
        logger.error(
            "PCI bus IDs of the selected RShims changed since device selection: %s",
            ", ".join(mismatched),
        )
        return 1
    with _terminate_doca_on_signal():
        for target in targets:
            reset_dpu.wait_for_module_transition_to_complete(target.dpu)

        auto_recovery_state = _get_dpu_auto_recovery()
        if auto_recovery_state is None:
            return 1

        restore_auto_recovery = auto_recovery_state == "enable"
        install_status = 1
        try:
            auto_recovery_ready = True
            if restore_auto_recovery:
                if _set_dpu_auto_recovery("disable"):
                    logger.info("Temporarily disabled dpu_auto_recovery")
                else:
                    auto_recovery_ready = False

            if auto_recovery_ready:
                transaction = rshim_daemon.RshimConfigTransaction(
                    all_mappings, selected_mappings.keys()
                )
                state = _TransactionState()
                apply_succeeded = False
                cleanup_status = 0
                try:
                    apply_succeeded = transaction.apply()
                    if apply_succeeded:
                        _flash_targets(
                            state,
                            targets=targets,
                            transaction=transaction,
                            all_mappings=all_mappings,
                            selected_mappings=selected_mappings,
                            bfb_path=bfb_path,
                            config_path=config_path,
                            timeout_secs=timeout_secs,
                        )
                finally:
                    if apply_succeeded:
                        cleanup_status = _finalize_transaction(state, transaction, verbose)
                    elif not transaction.remove():
                        cleanup_status = 1

                if apply_succeeded:
                    install_status = 1 if (
                        state.failed_preflight_rshims or
                        state.doca_status != 0 or
                        cleanup_status
                    ) else 0
        finally:
            if restore_auto_recovery:
                if _set_dpu_auto_recovery("enable"):
                    logger.info("Restored dpu_auto_recovery to enable")
                else:
                    install_status = 1

        return install_status
