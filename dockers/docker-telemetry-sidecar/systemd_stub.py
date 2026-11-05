#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import subprocess
import time
import argparse
from typing import Dict, List, Union

from sonic_py_common.sidecar_common import (
    get_bool_env_var, logger, SyncItem, run_nsenter,
    read_file_bytes_local, host_read_bytes, host_write_atomic,
    db_hget, db_hgetall, db_hset, db_hdel, db_del, db_get_table_keys,
    sync_items, SYNC_INTERVAL_S
)

IS_V1_ENABLED = get_bool_env_var("IS_V1_ENABLED", default=False)

# CONFIG_DB reconcile env
GNMI_VERIFY_ENABLED = get_bool_env_var("TELEMETRY_CLIENT_CERT_VERIFY_ENABLED", default=False)
def _parse_client_certs() -> List[Dict[str, Union[str, List[str]]]]:
    """
    Build the list of GNMI client cert entries from env vars.

    Returns a list of {"cname": str, "role": List[str]}.

    Preferred: GNMI_CLIENT_CERTS  (JSON array of {"cname": ..., "role": ...})
    Fallback:  TELEMETRY_CLIENT_CNAME / GNMI_CLIENT_ROLE  (single entry, backward-compat)
    """
    raw = os.getenv("GNMI_CLIENT_CERTS", "").strip()
    if raw:
        try:
            entries = json.loads(raw)
            if not isinstance(entries, list):
                raise ValueError("GNMI_CLIENT_CERTS must be a JSON array")
            normalized: List[Dict[str, Union[str, List[str]]]] = []
            for e in entries:
                if not isinstance(e, dict):
                    raise ValueError(f"Each entry must be an object: {e!r}")
                if "cname" not in e or "role" not in e:
                    raise ValueError(f"Each entry needs 'cname' and 'role': {e}")
                cname = str(e.get("cname", "")).strip()
                role_raw = e.get("role", "")
                if isinstance(role_raw, list):
                    role = [str(r).strip() for r in role_raw if str(r).strip()]
                else:
                    s = str(role_raw).strip()
                    role = [s] if s else []
                if not cname or not role:
                    raise ValueError(f"'cname' and 'role' must be non-empty: {e}")
                normalized.append({"cname": cname, "role": role})
            return normalized
        except (json.JSONDecodeError, ValueError) as exc:
            logger.log_error(f"Bad GNMI_CLIENT_CERTS env var: {exc}; falling back to legacy")

    # Legacy single-entry env vars
    cname = os.getenv("TELEMETRY_CLIENT_CNAME", "").strip()
    role = os.getenv("GNMI_CLIENT_ROLE", "gnmi_show_readonly").strip()
    if cname:
        return [{"cname": cname, "role": [role]}]
    return []


GNMI_CLIENT_CERTS: List[Dict[str, Union[str, List[str]]]] = _parse_client_certs()

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


def _apply_config_patch(patch: list) -> bool:
    """Apply an RFC 6902 JSON Patch to CONFIG_DB via 'config apply-patch'.

    YANG-validated and atomic.  Returns True on success.
    """
    if not patch:
        return True
    patch_json = json.dumps(patch)
    rc, out, err = run_nsenter(
        ["sudo", "-n", "config", "apply-patch", "/dev/stdin"],
        text=False,
        input_bytes=patch_json.encode("utf-8"),
    )
    if rc != 0:
        out_str = out.decode("utf-8", errors="replace").strip() if out else ""
        err_str = err.decode("utf-8", errors="replace").strip() if err else ""
        details = "; ".join(p for p in (f"stdout: {out_str}" if out_str else "",
                                        f"stderr: {err_str}" if err_str else "") if p)
        logger.log_error(f"config apply-patch failed (rc={rc}): {details}")
        return False
    logger.log_notice(f"config apply-patch succeeded ({len(patch)} op(s))")
    return True


def _ensure_user_auth_absent() -> None:
    cur = db_hget("TELEMETRY|gnmi", "user_auth")
    if cur is None:
        return
    if db_hdel("TELEMETRY|gnmi", "user_auth"):
        logger.log_notice(f"Removed TELEMETRY|gnmi.user_auth (was: {cur})")
        rc, _, err = run_nsenter(["sudo", "systemctl", "restart", "telemetry"])
        if rc != 0:
            logger.log_error(f"Failed to restart telemetry after user_auth removal: {err}")
    else:
        logger.log_error("Failed to remove TELEMETRY|gnmi.user_auth")


def _normalize_role(value) -> List[str]:
    """Normalize a role value from CONFIG_DB into a canonical List[str].

    Handles:
      - list (correct format from ConfigDBConnector leaf-list) → as-is
      - plain string "admin" → ["admin"]
      - JSON-encoded string '["admin","readonly"]' → ["admin", "readonly"]
    """
    if isinstance(value, list):
        return [str(v).strip() for v in value if str(v).strip()]
    if not isinstance(value, str) or not value.strip():
        return []
    s = value.strip()
    if s.startswith("["):
        try:
            parsed = json.loads(s)
            if isinstance(parsed, list):
                return [str(v).strip() for v in parsed if str(v).strip()]
        except (json.JSONDecodeError, ValueError):
            pass
    return [s]


def _build_enabled_patch() -> list:
    """Build RFC 6902 JSON Patch ops for the desired enabled state.

    Reads current CONFIG_DB to detect drift and returns a minimal patch
    containing only the operations needed to converge.
    Includes both TELEMETRY|gnmi.user_auth and GNMI_CLIENT_CERT entries.
    """
    patch: list = []

    # TELEMETRY|gnmi.user_auth must be "cert"
    if db_hget("TELEMETRY|gnmi", "user_auth") != "cert":
        patch.append({"op": "add", "path": "/TELEMETRY/gnmi/user_auth", "value": "cert"})

    # GNMI_CLIENT_CERT entries
    existing_cnames = set(db_get_table_keys("GNMI_CLIENT_CERT"))
    entries_needed: list = []
    for entry in GNMI_CLIENT_CERTS:
        cname, role = entry["cname"], entry["role"]
        if cname in existing_cnames:
            stored = db_hgetall(f"GNMI_CLIENT_CERT|{cname}")
            stored_role = stored.get("role") if stored else None
            if isinstance(stored_role, list) and sorted(_normalize_role(stored_role)) == sorted(role):
                continue
        entries_needed.append(entry)

    if entries_needed:
        if not existing_cnames:
            # Table absent – create with all entries in a single op
            patch.append({
                "op": "add",
                "path": "/GNMI_CLIENT_CERT",
                "value": {e["cname"]: {"role": e["role"]} for e in entries_needed},
            })
        else:
            for e in entries_needed:
                op = "replace" if e["cname"] in existing_cnames else "add"
                patch.append({
                    "op": op,
                    "path": f"/GNMI_CLIENT_CERT/{e['cname']}",
                    "value": {"role": e["role"]},
                })

    return patch


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
      - When false:
          * Remove TELEMETRY|gnmi.user_auth
          * Remove all entries under GNMI_CLIENT_CERT table
    """
    if GNMI_VERIFY_ENABLED:
        patch = _build_enabled_patch()
        if patch:
            _apply_config_patch(patch)
    else:
        _ensure_user_auth_absent()
        for cname in db_get_table_keys("GNMI_CLIENT_CERT"):
            _ensure_cname_absent(cname)

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

    if branch_name not in ("202411", "202412", "202505"):
        logger.log_error(f"Unsupported branch '{branch_name}'; aborting sync to trigger rollback")
        return False

    # For supported branches, use the branch-specific container_checker and service_checker
    container_checker_src = f"/usr/share/sonic/systemd_scripts/container_checker_{branch_name}"
    service_checker_src = f"/usr/share/sonic/systemd_scripts/service_checker.py_{branch_name}"

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
