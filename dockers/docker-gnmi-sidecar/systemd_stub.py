#!/usr/bin/env python3
"""
GNMI sidecar: syncs stub scripts from container to host via nsenter.
Replaces systemd-managed gnmi container with K8s-managed one.
"""
from __future__ import annotations

import os
import re
import random
import subprocess
import time
import argparse
import traceback
from typing import List

from sonic_py_common.sidecar_common import (
    get_bool_env_var, logger, SyncItem, run_nsenter,
    read_file_bytes_local, host_read_bytes, host_write_atomic,
    sync_items, cleanup_native_container, SYNC_INTERVAL_S
)

IS_V1_ENABLED = get_bool_env_var("IS_V1_ENABLED", default=False)

logger.log_notice(f"IS_V1_ENABLED={IS_V1_ENABLED}")

# Gate syncing /bin/container_checker to the host.  Multiple sidecars ship the
# same per-branch container_checker, so leaving every sidecar to sync it causes
# them to race on /bin/container_checker.  Default off; the rollout enables it
# on exactly one sidecar where the updated checker is required.
CONTAINER_CHECKER_SYNC_ENABLED = get_bool_env_var("CONTAINER_CHECKER_SYNC_ENABLED", default=False)

logger.log_notice(f"CONTAINER_CHECKER_SYNC_ENABLED={CONTAINER_CHECKER_SYNC_ENABLED}")

# Compile regex patterns once at module level to avoid repeated compilation
_MASTER_PATTERN = re.compile(r'^(?:SONiC\.)?master\.\d+-[a-f0-9]+$', re.IGNORECASE)
_INTERNAL_PATTERN = re.compile(r'^(?:SONiC\.)?internal\.\d+-[a-f0-9]+$', re.IGNORECASE)
_DATE_PATTERN = re.compile(r'^(?:SONiC\.)?\d{8}\b', re.IGNORECASE)
_DATE_EXTRACT_PATTERN = re.compile(r'^(?:SONiC\.)?(\d{4})(\d{2})\d{2}\b', re.IGNORECASE)


def _get_branch_name() -> str:
    """
    Extract branch name from SONiC version at runtime.
    Follows the logic from sonic-mgmt/tests/test_pretest.py get_asic_and_branch_name().

    Supported patterns:
    1. Master: [SONiC.]master.921927-18199d73f -> returns "master"
    2. Internal: [SONiC.]internal.135691748-dbb8d29985 -> returns "internal"
    3. Official feature branch: [SONiC.]YYYYMMDD.XX -> returns YYYYMM (e.g., 202505)
    4. Private/unmatched: returns "private"
    """
    version = ""
    try:
        # Try reading from sonic_version.yml
        version_file = "/etc/sonic/sonic_version.yml"
        if os.path.exists(version_file):
            with open(version_file, 'r') as f:
                for line in f:
                    if 'build_version:' in line.lower():
                        version = line.split(':', 1)[1].strip().strip('"\'')
                        break

        if not version:
            # Fallback: try nsenter to host
            result = subprocess.run(
                ["nsenter", "-t", "1", "-m", "-u", "-i", "-n", "sonic-cfggen", "-y", "/etc/sonic/sonic_version.yml", "-v", "build_version"],
                capture_output=True, text=True, timeout=5
            )
            if result.returncode == 0:
                version = result.stdout.strip().strip('"\'')
    except Exception as e:
        logger.log_warning(f"Failed to read SONiC version: {e}")
        version = ""

    if not version:
        logger.log_error("No SONiC version found")
        return "private"

    # Pattern 1: Master - [SONiC.]master.XXXXXX-XXXXXXXX
    if _MASTER_PATTERN.match(version):
        logger.log_notice(f"Detected master branch from version: {version}")
        return "master"

    # Pattern 2: Internal - [SONiC.]internal.XXXXXXXXX-XXXXXXXXXX
    elif _INTERNAL_PATTERN.match(version):
        logger.log_notice(f"Detected internal branch from version: {version}")
        return "internal"

    # Pattern 3: Official feature branch - [SONiC.]YYYYMMDD.* (e.g., 20241110.kw.24)
    elif _DATE_PATTERN.match(version):
        date_match = _DATE_EXTRACT_PATTERN.search(version)
        if date_match:
            year, month = date_match.groups()
            branch = f"{year}{month}"
            logger.log_notice(f"Detected branch {branch} from version: {version}")
            return branch
        else:
            logger.log_warning(f"Failed to parse date from version: {version}")
            return "private"

    # Pattern 4: Private image or unmatched pattern
    else:
        logger.log_notice(f"Unmatched version pattern (private): {version}")
        return "private"


SUPPORTED_BRANCHES = sorted(["202311", "202405", "202411", "202505", "202511"])


def _resolve_branch(branch_name: str) -> str:
    """Map detected branch to the nearest lower supported branch.

    - Exact match in SUPPORTED_BRANCHES → use as-is.
    - "master" / "internal" / "private" → latest supported branch (WARN).
    - Numeric YYYYMM between two supported branches → highest supported <= it.
    - Below 202311 → falls back to 202311 (ERROR).
    """
    if branch_name in SUPPORTED_BRANCHES:
        return branch_name

    if branch_name in ("master", "internal", "private"):
        resolved = SUPPORTED_BRANCHES[-1]
        logger.log_warning(f"Branch '{branch_name}' mapped to latest supported: {resolved}")
        return resolved

    if not branch_name.isdigit():
        logger.log_error(f"Cannot resolve non-numeric branch: {branch_name}, falling back to {SUPPORTED_BRANCHES[0]}")
        return SUPPORTED_BRANCHES[0]

    # String comparison is safe: all YYYYMM values are fixed 6-digit format
    candidates = [b for b in SUPPORTED_BRANCHES if b <= branch_name]
    if not candidates:
        logger.log_error(f"Branch '{branch_name}' is below minimum supported, falling back to {SUPPORTED_BRANCHES[0]}")
        return SUPPORTED_BRANCHES[0]

    resolved = candidates[-1]
    if resolved != branch_name:
        logger.log_notice(f"Branch '{branch_name}' mapped to nearest lower supported: {resolved}")
    return resolved

_GNMI_SRC = (
    "/usr/share/sonic/systemd_scripts/gnmi_v1.sh"
    if IS_V1_ENABLED
    else "/usr/share/sonic/systemd_scripts/gnmi.sh"
)
logger.log_notice(f"gnmi source set to {_GNMI_SRC}")

# k8s_pod_control.sh must be synced before gnmi.sh because the new gnmi.sh is a
# thin wrapper that exec's k8s_pod_control.sh.  If gnmi.sh is synced first its
# post-copy action (systemctl restart gnmi) would fail with "No such file or
# directory".
SYNC_ITEMS: List[SyncItem] = [
    SyncItem("/usr/share/sonic/scripts/k8s_pod_control.sh", "/usr/share/sonic/scripts/docker-gnmi-sidecar/k8s_pod_control.sh"),
    SyncItem(_GNMI_SRC, "/usr/local/bin/gnmi.sh"),
]

POST_COPY_ACTIONS = {
    "/usr/local/bin/gnmi.sh": [
        ["sudo", "docker", "stop", "gnmi"],
        ["sudo", "docker", "rm", "gnmi"],
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "gnmi"],
    ],
    "/bin/container_checker": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "monit"],
    ],
    "/usr/share/sonic/scripts/docker-gnmi-sidecar/k8s_pod_control.sh": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "gnmi"],
    ],
}

# Previous sidecar versions overwrote /lib/systemd/system/gnmi.service
# with a variant containing "User=root" (needed for kubectl).  Now that kubectl
# is gone we no longer sync that file, but hosts upgraded from the old sidecar
# still carry the stale unit.  This one-shot cleanup restores the original
# build-template version (User=admin) packed inside this container.
_CONTAINER_GNMI_SERVICE = "/usr/share/sonic/systemd_scripts/gnmi.service"
_HOST_GNMI_SERVICE = "/lib/systemd/system/gnmi.service"
_STALE_UNIT_CLEANUP_ENABLED = get_bool_env_var("STALE_UNIT_CLEANUP_ENABLED", default=True)
_stale_unit_cleaned = False


def _cleanup_stale_service_unit() -> None:
    """If the host gnmi.service still has User=root from a prior sidecar, restore it."""
    global _stale_unit_cleaned
    if _stale_unit_cleaned:
        return
    if not _STALE_UNIT_CLEANUP_ENABLED:
        _stale_unit_cleaned = True
        return

    host_bytes = host_read_bytes(_HOST_GNMI_SERVICE)
    if host_bytes is None:
        return  # transient failure or file missing; retry next cycle

    host_content = host_bytes.decode("utf-8", errors="ignore")
    if "\nUser=root\n" not in f"\n{host_content}\n":
        _stale_unit_cleaned = True  # unit is clean; no further retries needed
        return

    clean_bytes = read_file_bytes_local(_CONTAINER_GNMI_SERVICE)
    if clean_bytes is None:
        logger.log_error(f"Cannot read restore file {_CONTAINER_GNMI_SERVICE}")
        return  # container file missing; retry next cycle

    logger.log_notice("Stale sidecar gnmi.service detected (User=root); restoring from packed file")
    if not host_write_atomic(_HOST_GNMI_SERVICE, clean_bytes, 0o644):
        logger.log_error("Failed to restore gnmi.service")
        return  # write failed; retry next cycle
    rc, _, err = run_nsenter(["sudo", "systemctl", "daemon-reload"])
    if rc != 0:
        logger.log_error(f"daemon-reload failed after gnmi.service restore: {err}")
        return  # retry next cycle
    rc, _, err = run_nsenter(["sudo", "systemctl", "restart", "gnmi"])
    if rc != 0:
        logger.log_error(f"gnmi restart failed after gnmi.service restore: {err}")
        return  # retry next cycle
    _stale_unit_cleaned = True
    logger.log_notice("Restored gnmi.service and restarted")


def ensure_sync() -> bool:
    _cleanup_stale_service_unit()
    cleanup_native_container("gnmi", IS_V1_ENABLED)
    items: List[SyncItem] = list(SYNC_ITEMS)
    if CONTAINER_CHECKER_SYNC_ENABLED:
        branch_name = _resolve_branch(_get_branch_name())
        container_checker_src = f"/usr/share/sonic/systemd_scripts/container_checker_{branch_name}"
        items.append(SyncItem(container_checker_src, "/bin/container_checker"))
    return sync_items(items, POST_COPY_ACTIONS)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Sync host scripts from this container to the host via nsenter (syslog logging)."
    )
    p.add_argument("--once", action="store_true", help="Run one sync pass and exit")
    p.add_argument(
        "--interval",
        type=int,
        default=SYNC_INTERVAL_S,
        help=f"Loop interval seconds (default: {SYNC_INTERVAL_S})",
    )
    p.add_argument(
        "--no-post-actions",
        action="store_true",
        help="(Optional) Skip host systemctl actions (for debugging)",
    )
    return p.parse_args()


def main() -> int:
    args = parse_args()
    jitter_pct = 0.1  # ±10% jitter applied to sync loop interval
    if args.no_post_actions:
        POST_COPY_ACTIONS.clear()
        logger.log_info("Post-copy host actions DISABLED for this run")

    try:
        ok = ensure_sync()
        if not ok:
            logger.log_error("Initial sync failed.")
    except Exception as e:
        logger.log_error(f"Initial sync failed: {e}")
        logger.log_error(f"Traceback: {traceback.format_exc()}")
        ok = False

    if args.once:
        return 0 if ok else 1
    while True:
        try:
            jitter = args.interval * random.uniform(-jitter_pct, jitter_pct)
            time.sleep(args.interval + jitter)
            ok = ensure_sync()
            if not ok:
                logger.log_error("Sync failed. Will retry in next iteration.")
        except Exception as e:
            logger.log_error(f"Sync loop iteration failed: {e}. Will retry in {args.interval} seconds.")
            logger.log_error(f"Traceback: {traceback.format_exc()}")
            # Continue to next iteration rather than crashing


if __name__ == "__main__":
    raise SystemExit(main())
