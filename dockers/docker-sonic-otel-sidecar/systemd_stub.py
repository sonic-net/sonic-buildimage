#!/usr/bin/env python3
from __future__ import annotations

import time
import argparse
from typing import Dict, List

from sonic_py_common.sidecar_common import (
    logger, SyncItem,
    sync_items, SYNC_INTERVAL_S
)

SYNC_ITEMS: List[SyncItem] = [
    SyncItem("/usr/share/sonic/systemd_scripts/otel.sh", "/usr/local/bin/otel.sh"),
    SyncItem("/usr/share/sonic/scripts/k8s_pod_control.sh",
             "/usr/share/sonic/scripts/docker-sonic-otel-sidecar/k8s_pod_control.sh"),
    SyncItem("/usr/share/sonic/systemd_scripts/container_checker", "/bin/container_checker"),
    SyncItem("/usr/share/sonic/systemd_scripts/service_checker.py",
             "/usr/local/lib/python3.11/dist-packages/health_checker/service_checker.py"),
]

POST_COPY_ACTIONS: Dict[str, List[List[str]]] = {
    "/usr/local/bin/otel.sh": [
        ["sudo", "docker", "stop", "otel"],
        ["sudo", "docker", "rm", "otel"],
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "otel"],
    ],
    "/bin/container_checker": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "monit"],
    ],
    "/usr/local/lib/python3.11/dist-packages/health_checker/service_checker.py": [
        ["sudo", "systemctl", "restart", "system-health"],
    ],
    "/usr/share/sonic/scripts/docker-sonic-otel-sidecar/k8s_pod_control.sh": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "otel"],
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
        ok = ensure_sync()


if __name__ == "__main__":
    raise SystemExit(main())
