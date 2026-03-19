#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import subprocess
import time
import argparse
from typing import Dict, List

from sonic_py_common.sidecar_common import (
    get_bool_env_var, logger, SyncItem, run_nsenter,
    read_file_bytes_local, host_read_bytes, host_write_atomic,
    db_hget, db_hgetall, db_hset, db_del, sync_items, SYNC_INTERVAL_S
)

IS_V1_ENABLED = get_bool_env_var("IS_V1_ENABLED", default=False)

# CONFIG_DB reconcile env
GNMI_VERIFY_ENABLED = get_bool_env_var("TELEMETRY_CLIENT_CERT_VERIFY_ENABLED", default=False)
def _parse_client_certs() -> List[Dict[str, str]]:
    """
    Build the list of GNMI client cert entries from env vars.

    Preferred: GNMI_CLIENT_CERTS  (JSON array of {"cname": ..., "role": ...})
    Fallback:  TELEMETRY_CLIENT_CNAME / GNMI_CLIENT_ROLE  (single entry, backward-compat)
    """
    raw = os.getenv("GNMI_CLIENT_CERTS", "").strip()
    if raw:
        try:
            entries = json.loads(raw)
            if not isinstance(entries, list):
                raise ValueError("GNMI_CLIENT_CERTS must be a JSON array")
            normalized: List[Dict[str, str]] = []
            for e in entries:
                if not isinstance(e, dict):
                    raise ValueError(f"Each entry must be an object: {e!r}")
                if "cname" not in e or "role" not in e:
                    raise ValueError(f"Each entry needs 'cname' and 'role': {e}")
                cname = str(e.get("cname", "")).strip()
                role = str(e.get("role", "")).strip()
                if not cname or not role:
                    raise ValueError(f"'cname' and 'role' must be non-empty strings: {e}")
                normalized.append({"cname": cname, "role": role})
            return normalized
        except (json.JSONDecodeError, ValueError) as exc:
            logger.log_error(f"Bad GNMI_CLIENT_CERTS env var: {exc}; falling back to legacy")

    # Legacy single-entry env vars
    cname = os.getenv("TELEMETRY_CLIENT_CNAME", "").strip()
    role = os.getenv("GNMI_CLIENT_ROLE", "gnmi_show_readonly").strip()
    if cname:
        return [{"cname": cname, "role": role}]
    return []


GNMI_CLIENT_CERTS: List[Dict[str, str]] = _parse_client_certs()

logger.log_notice(f"IS_V1_ENABLED={IS_V1_ENABLED}")
logger.log_notice(f"GNMI_CLIENT_CERTS={GNMI_CLIENT_CERTS}")

_TELEMETRY_SRC = (
    "/usr/share/sonic/systemd_scripts/telemetry_v1.sh"
    if IS_V1_ENABLED
    else "/usr/share/sonic/systemd_scripts/telemetry.sh"
)
logger.log_notice(f"telemetry source set to {_TELEMETRY_SRC}")

SYNC_ITEMS: List[SyncItem] = [
    SyncItem(_TELEMETRY_SRC, "/usr/local/bin/telemetry.sh"),
    SyncItem("/usr/share/sonic/scripts/k8s_pod_control.sh", "/usr/share/sonic/scripts/docker-telemetry-sidecar/k8s_pod_control.sh"),
]

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


POST_COPY_ACTIONS = {
    "/usr/local/bin/telemetry.sh": [
        ["sudo", "docker", "stop", "telemetry"],
        ["sudo", "docker", "rm", "telemetry"],
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "telemetry"],
    ],
    "/bin/container_checker": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "monit"],
    ],
    "/usr/local/lib/python3.11/dist-packages/health_checker/service_checker.py": [
        ["sudo", "systemctl", "restart", "system-health"],
    ],
    "/usr/share/sonic/scripts/docker-telemetry-sidecar/k8s_pod_control.sh": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "telemetry"],
    ],
}


def _ensure_user_auth_cert() -> None:
    cur = db_hget("TELEMETRY|gnmi", "user_auth")
    if cur != "cert":
        if db_hset("TELEMETRY|gnmi", "user_auth", "cert"):
            logger.log_notice(f"Set TELEMETRY|gnmi.user_auth=cert (was: {cur or '<unset>'})")
        else:
            logger.log_error("Failed to set TELEMETRY|gnmi.user_auth=cert")


def _ensure_cname_present(cname: str, role: str) -> None:
    key = f"GNMI_CLIENT_CERT|{cname}"
    entry = db_hgetall(key)
    if not entry:
        if db_hset(key, "role", role):
            logger.log_notice(f"Created {key} with role={role}")
        else:
            logger.log_error(f"Failed to create {key}")


def _ensure_cname_absent(cname: str) -> None:
    key = f"GNMI_CLIENT_CERT|{cname}"
    if db_hgetall(key):
        if db_del(key):
            logger.log_notice(f"Removed {key}")
        else:
            logger.log_error(f"Failed to remove {key}")

def reconcile_config_db_once() -> None:
    """
    Idempotent drift-correction for CONFIG_DB:
      - When TELEMETRY_CLIENT_CERT_VERIFY_ENABLED=true:
          * Ensure TELEMETRY|gnmi.user_auth=cert
          * Ensure every GNMI_CLIENT_CERT|<CNAME> entry exists with its role
      - When false: ensure all CNAME rows are absent
    """
    if GNMI_VERIFY_ENABLED:
        _ensure_user_auth_cert()
        for entry in GNMI_CLIENT_CERTS:
            _ensure_cname_present(entry["cname"], entry["role"])
    else:
        for entry in GNMI_CLIENT_CERTS:
            _ensure_cname_absent(entry["cname"])

# Host destination for service_checker.py
HOST_SERVICE_CHECKER = "/usr/local/lib/python3.11/dist-packages/health_checker/service_checker.py"

# TO-be-deleted in next rounds releases, as long as telemetry.service rollouted have been restored.
# Previous sidecar versions overwrote /lib/systemd/system/telemetry.service
# with a variant containing "User=root" (needed for kubectl).  Now that kubectl
# is gone we no longer sync that file, but hosts upgraded from the old sidecar
# still carry the stale unit.  This one-shot cleanup restores the original
# build-template version (User=admin) packed inside this container.
_CONTAINER_TELEMETRY_SERVICE = "/usr/share/sonic/systemd_scripts/telemetry.service"
_HOST_TELEMETRY_SERVICE = "/lib/systemd/system/telemetry.service"
_STALE_UNIT_CLEANUP_ENABLED = get_bool_env_var("STALE_UNIT_CLEANUP_ENABLED", default=True)
_stale_unit_cleaned = False

def _cleanup_stale_service_unit() -> None:
    """If the host telemetry.service still has User=root from a prior sidecar, restore it."""
    global _stale_unit_cleaned
    if _stale_unit_cleaned:
        return
    if not _STALE_UNIT_CLEANUP_ENABLED:
        _stale_unit_cleaned = True
        return

    host_bytes = host_read_bytes(_HOST_TELEMETRY_SERVICE)
    if host_bytes is None:
        return  # transient failure or file missing; retry next cycle

    host_content = host_bytes.decode("utf-8", errors="ignore")
    if "\nUser=root\n" not in f"\n{host_content}\n":
        _stale_unit_cleaned = True  # unit is clean; no further retries needed
        return

    clean_bytes = read_file_bytes_local(_CONTAINER_TELEMETRY_SERVICE)
    if clean_bytes is None:
        logger.log_error(f"Cannot read restore file {_CONTAINER_TELEMETRY_SERVICE}")
        return  # container file missing; retry next cycle

    logger.log_notice("Stale sidecar telemetry.service detected (User=root); restoring from packed file")
    if not host_write_atomic(_HOST_TELEMETRY_SERVICE, clean_bytes, 0o644):
        logger.log_error("Failed to restore telemetry.service")
        return  # write failed; retry next cycle
    rc, _, err = run_nsenter(["sudo", "systemctl", "daemon-reload"])
    if rc != 0:
        logger.log_error(f"daemon-reload failed after telemetry.service restore: {err}")
        return  # retry next cycle
    rc, _, err = run_nsenter(["sudo", "systemctl", "restart", "telemetry"])
    if rc != 0:
        logger.log_error(f"telemetry restart failed after telemetry.service restore: {err}")
        return  # retry next cycle
    _stale_unit_cleaned = True
    logger.log_notice("Restored telemetry.service and restarted")


def ensure_sync() -> bool:
    _cleanup_stale_service_unit()
    branch_name = _get_branch_name()

    if branch_name in ("202411", "202412", "202505"):
        # For 202411/202412/202505 branches, use the branch-specific container_checker
        container_checker_src = f"/usr/share/sonic/systemd_scripts/container_checker_{branch_name}"
        # For 202411/202412/202505 branches, use the branch-specific service_checker
        service_checker_src = f"/usr/share/sonic/systemd_scripts/service_checker.py_{branch_name}"
    else:
        # For other branches, use the default container_checker
        container_checker_src = "/usr/share/sonic/systemd_scripts/container_checker"
        # For other branches, use the default service_checker
        service_checker_src = "/usr/share/sonic/systemd_scripts/service_checker.py"

    items: List[SyncItem] = SYNC_ITEMS + [
        SyncItem(container_checker_src, "/bin/container_checker"),
        SyncItem(service_checker_src, HOST_SERVICE_CHECKER),
    ]
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
    if args.no_post_actions:
        POST_COPY_ACTIONS.clear()
        logger.log_info("Post-copy host actions DISABLED for this run")

    # Reconcile CONFIG_DB before any file sync so auth is correct ASAP
    reconcile_config_db_once()
    ok = ensure_sync()
    if args.once:
        return 0 if ok else 1
    while True:
        time.sleep(args.interval)
        reconcile_config_db_once()
        ok = ensure_sync()


if __name__ == "__main__":
    raise SystemExit(main())
