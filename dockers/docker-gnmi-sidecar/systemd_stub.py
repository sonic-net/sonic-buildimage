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
    get_bool_env_var, logger, SyncItem, run_nsenter,
    read_file_bytes_local, host_read_bytes, host_write_atomic,
    sync_items, SYNC_INTERVAL_S
)

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
    SyncItem("/usr/share/sonic/scripts/k8s_pod_control.sh", "/usr/share/sonic/scripts/docker-gnmi-sidecar/k8s_pod_control.sh"),
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
