#!/usr/bin/env python3
from __future__ import annotations

import os
import time
import argparse
from typing import List

from sonic_py_common.sidecar_common import (
    get_bool_env_var, logger, SyncItem,
    db_hget, db_hgetall, db_hset, db_del, sync_items, SYNC_INTERVAL_S
)

# ───────────── telemetry.service sync paths ─────────────
CONTAINER_TELEMETRY_SERVICE = "/usr/share/sonic/systemd_scripts/telemetry.service"
HOST_TELEMETRY_SERVICE = "/lib/systemd/system/telemetry.service"

IS_V1_ENABLED = get_bool_env_var("IS_V1_ENABLED", default=False)

# CONFIG_DB reconcile env
GNMI_VERIFY_ENABLED = get_bool_env_var("TELEMETRY_CLIENT_CERT_VERIFY_ENABLED", default=False)
GNMI_CLIENT_CNAME = os.getenv("TELEMETRY_CLIENT_CNAME", "")
GNMI_CLIENT_ROLE = os.getenv("GNMI_CLIENT_ROLE", "gnmi_show_readonly")

logger.log_notice(f"IS_V1_ENABLED={IS_V1_ENABLED}")
logger.log_notice(f"GNMI_CLIENT_ROLE={GNMI_CLIENT_ROLE}")

_TELEMETRY_SRC = (
    "/usr/share/sonic/systemd_scripts/telemetry_v1.sh"
    if IS_V1_ENABLED
    else "/usr/share/sonic/systemd_scripts/telemetry.sh"
)
logger.log_notice(f"telemetry source set to {_TELEMETRY_SRC}")

SYNC_ITEMS: List[SyncItem] = [
    SyncItem(_TELEMETRY_SRC, "/usr/local/bin/telemetry.sh"),
    SyncItem("/usr/share/sonic/systemd_scripts/container_checker", "/bin/container_checker"),
    SyncItem("/usr/share/sonic/scripts/k8s_pod_control.sh", "/usr/share/sonic/scripts/k8s_pod_control.sh"),
    SyncItem(CONTAINER_TELEMETRY_SERVICE, HOST_TELEMETRY_SERVICE, mode=0o644),
]

POST_COPY_ACTIONS = {
    "/lib/systemd/system/telemetry.service": [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "telemetry"],
    ],
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
}


def _ensure_user_auth_cert() -> None:
    cur = db_hget("GNMI|gnmi", "user_auth")
    if cur != "cert":
        if db_hset("GNMI|gnmi", "user_auth", "cert"):
            logger.log_notice(f"Set GNMI|gnmi.user_auth=cert (was: {cur or '<unset>'})")
        else:
            logger.log_error("Failed to set GNMI|gnmi.user_auth=cert")


def _ensure_cname_present(cname: str) -> None:
    if not cname:
        logger.log_warning("TELEMETRY_CLIENT_CNAME not set; skip CNAME creation")
        return

    key = f"GNMI_CLIENT_CERT|{cname}"
    entry = db_hgetall(key)
    if not entry:
        if db_hset(key, "role", GNMI_CLIENT_ROLE):
            logger.log_notice(f"Created {key} with role={GNMI_CLIENT_ROLE}")
        else:
            logger.log_error(f"Failed to create {key}")


def _ensure_cname_absent(cname: str) -> None:
    if not cname:
        return
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
          * Ensure GNMI|gnmi.user_auth=cert
          * Ensure GNMI_CLIENT_CERT|<CNAME> exists with role=<GNMI_CLIENT_ROLE>
      - When false: ensure the CNAME row is absent
    """
    if GNMI_VERIFY_ENABLED:
        _ensure_user_auth_cert()
        _ensure_cname_present(GNMI_CLIENT_CNAME)
    else:
        _ensure_cname_absent(GNMI_CLIENT_CNAME)


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
