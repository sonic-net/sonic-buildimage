#!/usr/bin/env python3
"""
GNMI sidecar: syncs stub scripts from container to host via nsenter.
Replaces systemd-managed gnmi container with K8s-managed one.
"""
from __future__ import annotations

import time
import argparse
from typing import List

from sonic_py_common.sidecar_common import (
    get_bool_env_var, logger, SyncItem, sync_items, SYNC_INTERVAL_S
)

# ───────────── gnmi.service sync paths ─────────────
CONTAINER_GNMI_SERVICE = "/usr/share/sonic/systemd_scripts/gnmi.service"
HOST_GNMI_SERVICE = "/lib/systemd/system/gnmi.service"

IS_V1_ENABLED = get_bool_env_var("IS_V1_ENABLED", default=False)

logger.log_notice(f"IS_V1_ENABLED={IS_V1_ENABLED}")

_GNMI_SRC = (
    "/usr/share/sonic/systemd_scripts/gnmi_v1.sh"
    if IS_V1_ENABLED
    else "/usr/share/sonic/systemd_scripts/gnmi.sh"
)
logger.log_notice(f"gnmi source set to {_GNMI_SRC}")

SYNC_ITEMS: List[SyncItem] = [
    SyncItem(_GNMI_SRC, "/usr/local/bin/gnmi.sh"),
    SyncItem("/usr/share/sonic/systemd_scripts/container_checker", "/bin/container_checker"),
    SyncItem("/usr/share/sonic/scripts/k8s_pod_control.sh", "/usr/share/sonic/scripts/k8s_pod_control.sh"),
    SyncItem(CONTAINER_GNMI_SERVICE, HOST_GNMI_SERVICE, mode=0o644),
]

POST_COPY_ACTIONS = {
    "/lib/systemd/system/gnmi.service": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "gnmi"],
    ],
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
}


def ensure_sync() -> bool:
    return sync_items(SYNC_ITEMS, POST_COPY_ACTIONS)


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

    ok = ensure_sync()
    if args.once:
        return 0 if ok else 1
    while True:
        time.sleep(args.interval)
        ensure_sync()


if __name__ == "__main__":
    raise SystemExit(main())
