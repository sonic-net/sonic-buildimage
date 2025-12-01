#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
import time
import argparse
import hashlib
import shlex
import subprocess
from dataclasses import dataclass
from typing import List, Optional, Tuple, Dict

from swsscommon.swsscommon import ConfigDBConnector
from sonic_py_common import logger as log

logger = log.Logger()

# ───────────── telemetry.service sync paths ─────────────
CONTAINER_TELEMETRY_SERVICE = "/usr/share/sonic/systemd_scripts/telemetry.service"
HOST_TELEMETRY_SERVICE = "/lib/systemd/system/telemetry.service"

def get_bool_env_var(name: str, default: bool = False) -> bool:
    val = os.getenv(name)
    if val is None:
        return default
    return val.strip().lower() in ("1", "true", "yes", "y", "on")


IS_V1_ENABLED = get_bool_env_var("IS_V1_ENABLED", default=False)

# ───────────── Config ─────────────
SYNC_INTERVAL_S = int(os.environ.get("SYNC_INTERVAL_S", "900"))  # seconds
NSENTER_BASE = ["nsenter", "--target", "1", "--pid", "--mount", "--uts", "--ipc", "--net"]

# CONFIG_DB reconcile env
GNMI_VERIFY_ENABLED = get_bool_env_var("TELEMETRY_CLIENT_CERT_VERIFY_ENABLED", default=False)
GNMI_CLIENT_CNAME = os.getenv("TELEMETRY_CLIENT_CNAME", "")
GNMI_CLIENT_ROLE = os.getenv("GNMI_CLIENT_ROLE", "gnmi_show_readonly")

logger.log_notice(f"IS_V1_ENABLED={IS_V1_ENABLED}")
logger.log_notice(f"GNMI_CLIENT_ROLE={GNMI_CLIENT_ROLE}")


@dataclass(frozen=True)
class SyncItem:
    src_in_container: str
    dst_on_host: str
    mode: int = 0o755

_TELEMETRY_SRC = (
    "/usr/share/sonic/systemd_scripts/telemetry_v1.sh"
    if IS_V1_ENABLED
    else "/usr/share/sonic/systemd_scripts/telemetry.sh"
)
logger.log_notice(f"telemetry source set to {_TELEMETRY_SRC}")

SYNC_ITEMS: List[SyncItem] = [
    SyncItem(_TELEMETRY_SRC, "/usr/local/bin/telemetry.sh"),
    SyncItem("/usr/share/sonic/systemd_scripts/container_checker", "/bin/container_checker"),
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

def run(args: List[str], *, text: bool = True, input_bytes: Optional[bytes] = None) -> Tuple[int, str | bytes, str | bytes]:
    logger.log_debug("Running: " + " ".join(args))
    p = subprocess.Popen(
        args,
        text=text,
        stdin=subprocess.PIPE if input_bytes is not None else None,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    out, err = p.communicate(input=input_bytes if input_bytes is not None else None)
    return p.returncode, out, err

def run_nsenter(args: List[str], *, text: bool = True, input_bytes: Optional[bytes] = None) -> Tuple[int, str | bytes, str | bytes]:
    return run(NSENTER_BASE + args, text=text, input_bytes=input_bytes)


# ───────── CONFIG_DB via ConfigDBConnector ─────────
_config_db: Optional[ConfigDBConnector] = None


def _get_config_db() -> Optional[ConfigDBConnector]:
    global _config_db
    if _config_db is None:
        try:
            db = ConfigDBConnector()
            db.connect()
            _config_db = db
            logger.log_info("Connected to CONFIG_DB via ConfigDBConnector")
        except Exception as e:
            logger.log_error(f"Failed to connect to CONFIG_DB: {e}")
            _config_db = None
            # Do not cache failed connection; try again next time
    return _config_db


def _split_redis_key(key: str) -> Tuple[str, str]:
    # Expect "TABLE|KEY"
    parts = key.split("|", 1)
    if len(parts) != 2:
        raise ValueError(f"Invalid CONFIG_DB key format (expected 'TABLE|KEY'): {key!r}")
    return parts[0], parts[1]


def _db_hget(key: str, field: str) -> Optional[str]:
    db = _get_config_db()
    if db is None:
        return None
    try:
        table, entry_key = _split_redis_key(key)
        entry: Dict[str, str] = db.get_entry(table, entry_key)
    except Exception as e:
        logger.log_error(f"_db_hget failed for {key} field {field}: {e}")
        return None

    val = entry.get(field)
    if val is None or val == "":
        return None
    return val


def _db_hgetall(key: str) -> Dict[str, str]:
    db = _get_config_db()
    if db is None:
        return {}
    try:
        table, entry_key = _split_redis_key(key)
        entry: Dict[str, str] = db.get_entry(table, entry_key)
        return entry or {}
    except Exception as e:
        logger.log_error(f"_db_hgetall failed for {key}: {e}")
        return {}


def _db_hset(key: str, field: str, value: str) -> bool:
    db = _get_config_db()
    if db is None:
        return False
    try:
        table, entry_key = _split_redis_key(key)
        entry: Dict[str, str] = db.get_entry(table, entry_key)
        entry[field] = value
        db.set_entry(table, entry_key, entry)
        return True
    except Exception as e:
        logger.log_error(f"_db_hset failed for {key} field {field}: {e}")
        return False


def _db_del(key: str) -> bool:
    db = _get_config_db()
    if db is None:
        return False
    try:
        table, entry_key = _split_redis_key(key)
        # In ConfigDBConnector, setting an empty dict deletes the key
        db.set_entry(table, entry_key, {})
        return True
    except Exception as e:
        logger.log_error(f"_db_del failed for {key}: {e}")
        return False


def _ensure_user_auth_cert() -> None:
    cur = _db_hget("GNMI|gnmi", "user_auth")
    if cur != "cert":
        if _db_hset("GNMI|gnmi", "user_auth", "cert"):
            logger.log_notice(f"Set GNMI|gnmi.user_auth=cert (was: {cur or '<unset>'})")
        else:
            logger.log_error("Failed to set GNMI|gnmi.user_auth=cert")

def _ensure_cname_present(cname: str) -> None:
    if not cname:
        logger.log_warning("TELEMETRY_CLIENT_CNAME not set; skip CNAME creation")
        return

    key = f"GNMI_CLIENT_CERT|{cname}"
    entry = _db_hgetall(key)
    if not entry:
        if _db_hset(key, "role", GNMI_CLIENT_ROLE):
            logger.log_notice(f"Created {key} with role={GNMI_CLIENT_ROLE}")
        else:
            logger.log_error(f"Failed to create {key}")


def _ensure_cname_absent(cname: str) -> None:
    if not cname:
        return
    key = f"GNMI_CLIENT_CERT|{cname}"
    if _db_hgetall(key):
        if _db_del(key):
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


def read_file_bytes_local(path: str) -> Optional[bytes]:
    try:
        with open(path, "rb") as f:
            return f.read()
    except OSError as e:
        logger.log_error(f"read failed for {path}: {e}")
        return None

def host_read_bytes(path_on_host: str) -> Optional[bytes]:
    rc, out, _ = run_nsenter(["/bin/cat", path_on_host], text=False)
    if rc != 0:
        return None
    return out


def host_write_atomic(dst_on_host: str, data: bytes, mode: int) -> bool:
    tmp_path = f"/tmp/{os.path.basename(dst_on_host)}.tmp"
    rc, _, err = run_nsenter(["/bin/sh", "-c", f"cat > {shlex.quote(tmp_path)}"], text=False, input_bytes=data)
    if rc != 0:
        emsg = err.decode(errors="ignore") if isinstance(err, (bytes, bytearray)) else str(err)
        logger.log_error(f"host write tmp failed: {emsg.strip()}")
        return False

    rc, _, err = run_nsenter(["/bin/chmod", f"{mode:o}", tmp_path], text=True)
    if rc != 0:
        logger.log_error(f"host chmod failed: {str(err).strip()}")
        run_nsenter(["/bin/rm", "-f", tmp_path], text=True)
        return False

    parent = os.path.dirname(dst_on_host) or "/"
    rc, _, err = run_nsenter(["/bin/mkdir", "-p", parent], text=True)
    if rc != 0:
        logger.log_error(f"host mkdir failed for {parent}: {str(err).strip()}")
        run_nsenter(["/bin/rm", "-f", tmp_path], text=True)
        return False

    rc, _, err = run_nsenter(["/bin/mv", "-f", tmp_path, dst_on_host], text=True)
    if rc != 0:
        logger.log_error(f"host mv failed to {dst_on_host}: {str(err).strip()}")
        run_nsenter(["/bin/rm", "-f", tmp_path], text=True)
        return False
    return True


def run_host_actions_for(path_on_host: str) -> None:
    actions = POST_COPY_ACTIONS.get(path_on_host, [])
    for cmd in actions:
        rc, _, err = run_nsenter(cmd, text=True)
        if rc == 0:
            logger.log_info(f"Post-copy action succeeded: {' '.join(cmd)}")
        else:
            logger.log_error(f"Post-copy action FAILED (rc={rc}): {' '.join(cmd)}; stderr={str(err).strip()}")

def sha256_bytes(b: Optional[bytes]) -> str:
    if b is None:
        return ""
    h = hashlib.sha256()
    h.update(b)
    return h.hexdigest()


def sync_items(items: List[SyncItem]) -> bool:
    all_ok = True
    for item in items:
        src_bytes = read_file_bytes_local(item.src_in_container)
        if src_bytes is None:
            logger.log_error(f"Cannot read {item.src_in_container} in this container")
            all_ok = False
            continue

        container_file_sha = sha256_bytes(src_bytes)
        host_bytes = host_read_bytes(item.dst_on_host)
        host_sha = sha256_bytes(host_bytes)

        if host_sha == container_file_sha:
            logger.log_info(f"{os.path.basename(item.dst_on_host)} up-to-date (sha256={host_sha})")
            continue

        logger.log_info(
            f"{os.path.basename(item.dst_on_host)} differs "
            f"(container {container_file_sha} vs host {host_sha or 'missing'}), updating…"
        )
        if not host_write_atomic(item.dst_on_host, src_bytes, item.mode):
            logger.log_error(f"Copy/update failed for {item.dst_on_host}")
            all_ok = False
            continue

        new_host_bytes = host_read_bytes(item.dst_on_host)
        new_sha = sha256_bytes(new_host_bytes)
        if new_sha != container_file_sha:
            logger.log_error(
                f"Post-copy SHA mismatch for {item.dst_on_host}: "
                f"host {new_sha or 'read-failed'} vs container {container_file_sha}"
            )
            all_ok = False
        else:
            logger.log_info(f"Sync complete for {item.dst_on_host} (sha256={new_sha})")
            run_host_actions_for(item.dst_on_host)
    return all_ok


def ensure_sync() -> bool:
    return sync_items(SYNC_ITEMS)

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
