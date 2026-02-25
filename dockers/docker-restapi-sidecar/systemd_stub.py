#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import subprocess
import time
import argparse
import traceback
from typing import List

from sonic_py_common.sidecar_common import (
    get_bool_env_var, logger, SyncItem,
    sync_items, SYNC_INTERVAL_S
)

# ───────────── restapi.service sync paths ─────────────
HOST_RESTAPI_SERVICE = "/lib/systemd/system/restapi.service"

IS_V1_ENABLED = get_bool_env_var("IS_V1_ENABLED", default=False)

logger.log_notice(f"IS_V1_ENABLED={IS_V1_ENABLED}")


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
    master_pattern = re.compile(r'^(?:SONiC\.)?master\.\d+-[a-f0-9]+$', re.IGNORECASE)
    if master_pattern.match(version):
        logger.log_notice(f"Detected master branch from version: {version}")
        return "master"
    
    # Pattern 2: Internal - [SONiC.]internal.XXXXXXXXX-XXXXXXXXXX
    elif re.match(r'^(?:SONiC\.)?internal\.\d+-[a-f0-9]+$', version, re.IGNORECASE):
        logger.log_notice(f"Detected internal branch from version: {version}")
        return "internal"
    
    # Pattern 3: Official feature branch - [SONiC.]YYYYMMDD.* (e.g., 20241110.kw.24)
    elif re.match(r'^(?:SONiC\.)?\d{8}\b', version, re.IGNORECASE):
        date_match = re.search(r'^(?:SONiC\.)?(\d{4})(\d{2})\d{2}\b', version, re.IGNORECASE)
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
    "/lib/systemd/system/restapi.service": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "restapi"],
    ],
    "/usr/bin/restapi.sh": [
        ["sudo", "docker", "stop", "restapi"],
        ["sudo", "docker", "rm", "restapi"],
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "restapi"],
    ],
    "/bin/container_checker": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "monit"],
    ],
}


def ensure_sync() -> bool:
    # Evaluate branch and source paths each time to allow retry on runtime failures
    branch_name = _get_branch_name()
    
    # Map to available branch-specific files: {202311,202405,202411,202505,202511}
    if branch_name not in ["202311", "202405", "202411", "202505", "202511"]:
        logger.log_error(f"Unsupported branch: {branch_name}. Only 202311, 202405, 202411, 202505, 202511 are supported.")
        return False
    
    # restapi.sh: per-branch when IS_V1_ENABLED, otherwise use common restapi.sh
    restapi_src = (
        f"/usr/share/sonic/systemd_scripts/v1/restapi.sh_{branch_name}"
        if IS_V1_ENABLED
        else "/usr/share/sonic/systemd_scripts/restapi.sh"
    )
    
    container_checker_src = f"/usr/share/sonic/systemd_scripts/container_checker_{branch_name}"
    container_restapi_service = f"/usr/share/sonic/systemd_scripts/restapi.service_{branch_name}"
    
    # Construct SYNC_ITEMS with evaluated paths
    sync_items_list: List[SyncItem] = [
        SyncItem(restapi_src, "/usr/bin/restapi.sh", mode=0o755),
        SyncItem(container_checker_src, "/bin/container_checker", mode=0o755),
        SyncItem("/usr/share/sonic/scripts/k8s_pod_control.sh", "/usr/share/sonic/scripts/k8s_pod_control.sh", mode=0o755),
        SyncItem(container_restapi_service, HOST_RESTAPI_SERVICE, mode=0o644),
    ]
    return sync_items(sync_items_list, POST_COPY_ACTIONS)

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
            time.sleep(args.interval)
            ok = ensure_sync()
            if not ok:
                logger.log_error("Sync failed. Will retry in next iteration.")
        except Exception as e:
            logger.log_error(f"Sync loop iteration failed: {e}. Will retry in {args.interval} seconds.")
            logger.log_error(f"Traceback: {traceback.format_exc()}")
            # Continue to next iteration rather than crashing


if __name__ == "__main__":
    raise SystemExit(main())
