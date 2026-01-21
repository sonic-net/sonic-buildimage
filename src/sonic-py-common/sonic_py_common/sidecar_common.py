#!/usr/bin/env python3
"""
Shared utilities for SONiC sidecar containers (telemetry, restapi, etc.)
that sync files to host and reconcile CONFIG_DB entries.
"""
from __future__ import annotations

import os
import hashlib
import shlex
import subprocess
from dataclasses import dataclass
from typing import List, Optional, Tuple, Dict

from swsscommon.swsscommon import ConfigDBConnector
from sonic_py_common import logger as log

logger = log.Logger()


def get_bool_env_var(name: str, default: bool = False) -> bool:
    """Parse boolean environment variable with common true/false values."""
    val = os.getenv(name)
    if val is None:
        return default
    return val.strip().lower() in ("1", "true", "yes", "y", "on")


# ───────────── Base Config ─────────────
SYNC_INTERVAL_S = int(os.environ.get("SYNC_INTERVAL_S", "900"))  # seconds
NSENTER_BASE = ["nsenter", "--target", "1", "--pid", "--mount", "--uts", "--ipc", "--net"]


@dataclass(frozen=True)
class SyncItem:
    """Represents a file to sync from container to host."""
    src_in_container: str
    dst_on_host: str
    mode: int = 0o755


# ───────────── Subprocess utilities ─────────────

def run(args: List[str], *, text: bool = True, input_bytes: Optional[bytes] = None) -> Tuple[int, str | bytes, str | bytes]:
    """Run a subprocess command and return (returncode, stdout, stderr)."""
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
    """Run a command in the host namespace via nsenter."""
    return run(NSENTER_BASE + args, text=text, input_bytes=input_bytes)


# ───────── CONFIG_DB via ConfigDBConnector ─────────
_config_db: Optional[ConfigDBConnector] = None


def _get_config_db() -> Optional[ConfigDBConnector]:
    """Get or create ConfigDBConnector instance (singleton pattern)."""
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
    """Split a CONFIG_DB key 'TABLE|KEY' into (table, key)."""
    # Expect "TABLE|KEY"
    parts = key.split("|", 1)
    if len(parts) != 2:
        raise ValueError(f"Invalid CONFIG_DB key format (expected 'TABLE|KEY'): {key!r}")
    return parts[0], parts[1]


def db_hget(key: str, field: str) -> Optional[str]:
    """Get a single field value from CONFIG_DB hash."""
    db = _get_config_db()
    if db is None:
        return None
    try:
        table, entry_key = _split_redis_key(key)
        entry: Dict[str, str] = db.get_entry(table, entry_key)
    except Exception as e:
        logger.log_error(f"db_hget failed for {key} field {field}: {e}")
        return None

    val = entry.get(field)
    if val is None or val == "":
        return None
    return val


def db_hgetall(key: str) -> Dict[str, str]:
    """Get all field-value pairs from CONFIG_DB hash."""
    db = _get_config_db()
    if db is None:
        return {}
    try:
        table, entry_key = _split_redis_key(key)
        entry: Dict[str, str] = db.get_entry(table, entry_key)
        return entry or {}
    except Exception as e:
        logger.log_error(f"db_hgetall failed for {key}: {e}")
        return {}


def db_hset(key: str, field: str, value: str) -> bool:
    """Set a single field value in CONFIG_DB hash."""
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
        logger.log_error(f"db_hset failed for {key} field {field}: {e}")
        return False


def db_del(key: str) -> bool:
    """Delete an entry from CONFIG_DB."""
    db = _get_config_db()
    if db is None:
        return False
    try:
        table, entry_key = _split_redis_key(key)
        # In ConfigDBConnector, setting an empty dict deletes the key
        db.set_entry(table, entry_key, {})
        return True
    except Exception as e:
        logger.log_error(f"db_del failed for {key}: {e}")
        return False


# ───────────── File operations ─────────────

def read_file_bytes_local(path: str) -> Optional[bytes]:
    """Read file from container filesystem."""
    try:
        with open(path, "rb") as f:
            return f.read()
    except OSError as e:
        logger.log_error(f"read failed for {path}: {e}")
        return None


def host_read_bytes(path_on_host: str) -> Optional[bytes]:
    """Read file from host filesystem via nsenter."""
    rc, out, _ = run_nsenter(["/bin/cat", path_on_host], text=False)
    if rc != 0:
        return None
    return out  # type: ignore[return-value]


def host_write_atomic(dst_on_host: str, data: bytes, mode: int) -> bool:
    """Atomically write file to host filesystem via nsenter."""
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


# ───────────── SHA256 utilities ─────────────

def sha256_bytes(b: Optional[bytes]) -> str:
    """Compute SHA256 hash of bytes, or empty string if None."""
    if b is None:
        return ""
    h = hashlib.sha256()
    h.update(b)
    return h.hexdigest()


# ───────────── File sync logic ─────────────

def sync_items(items: List[SyncItem], post_copy_actions: Dict[str, List[List[str]]]) -> bool:
    """
    Sync files from container to host, executing post-copy actions on changes.
    
    Args:
        items: List of files to sync
        post_copy_actions: Dict mapping host paths to lists of commands to run after sync
    
    Returns:
        True if all syncs succeeded, False otherwise
    """
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
            _run_host_actions_for(item.dst_on_host, post_copy_actions)
    return all_ok


def _run_host_actions_for(path_on_host: str, post_copy_actions: Dict[str, List[List[str]]]) -> None:
    """Execute post-copy actions for a synced file."""
    actions = post_copy_actions.get(path_on_host, [])
    for cmd in actions:
        rc, _, err = run_nsenter(cmd, text=True)
        if rc == 0:
            logger.log_info(f"Post-copy action succeeded: {' '.join(cmd)}")
        else:
            logger.log_error(f"Post-copy action FAILED (rc={rc}): {' '.join(cmd)}; stderr={str(err).strip()}")
