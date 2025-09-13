#!/usr/bin/env python3

from __future__ import annotations

import os
import time
import hashlib
import subprocess
from dataclasses import dataclass
from typing import List, Tuple, Optional

from sonic_py_common import logger as log

logger = log.Logger()

TELEMETRY_CONTAINER_NAME = "k8s_telemetry"
SYNC_INTERVAL_S = 900  # 15 minutes

@dataclass(frozen=True)
class SyncItem:
    src_in_container: str
    dst_on_host: str
    mode: int = 0o755  # default permission

SYNC_ITEMS: List[SyncItem] = [
    SyncItem("/usr/share/sonic/systemd_script/telemetry.sh", "/usr/bin/telemetry.sh"),
    SyncItem("/usr/share/sonic/systemd_script/container_checker", "/bin/container_checker"),
]

# Post-copy actions keyed by the DESTINATION path on host
# Only executed if that specific file was updated successfully.
POST_COPY_ACTIONS = {
    "/usr/bin/telemetry.sh": [
        ["sudo", "systemctl", "reset-failed", "telemetry"],
        ["sudo", "systemctl", "restart", "telemetry"],
    ],
    "/bin/container_checker": [
        ["sudo", "systemctl", "reset-failed", "monit"],
        ["sudo", "systemctl", "restart", "monit"],
    ],
}

NSENTER_BASE = ["nsenter", "--target", "1", "--pid", "--mount", "--uts", "--ipc", "--net"]

def run(args: List[str], *, text: bool = True, input_bytes: Optional[bytes] = None) -> Tuple[int, str | bytes, str | bytes]:
    """
    Run a command safely with shell=False.
    Returns (rc, stdout, stderr). stdout/stderr are str if text=True else bytes.
    """
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

def run_nsenter(args: List[str], *, text: bool = True) -> Tuple[int, str | bytes, str | bytes]:
    """Run a command under host namespaces via nsenter."""
    return run(NSENTER_BASE + args, text=text)

def run_host_actions_for(path_on_host: str) -> None:
    """Run any configured post-copy actions for a given host path."""
    actions = POST_COPY_ACTIONS.get(path_on_host, [])
    for cmd in actions:
        rc, out, err = run_nsenter(cmd, text=True)
        if rc == 0:
            logger.log_info(f"Post-copy action succeeded: {' '.join(cmd)}")
        else:
            logger.log_error(f"Post-copy action FAILED (rc={rc}): {' '.join(cmd)}; stderr={err.strip()}")

# ----------------- docker/host helpers -----------------
def get_telemetry_container_id() -> str:
    """
    Resolve container ID by exact name using docker inspect.
    Returns empty string if not found.
    """
    # docker inspect -f '{{.Id}}' k8s_telemetry
    rc, out, err = run_nsenter(["docker", "inspect", "-f", "{{.Id}}", TELEMETRY_CONTAINER_NAME], text=True)
    if rc != 0:
        logger.log_debug(f"docker inspect failed: {err.strip()}")
        return ""
    return out.strip()

def file_sha256_host(path: str) -> str:
    """Compute SHA256 of a host file (streaming). Returns '' if not exists or read error."""
    if not os.path.exists(path):
        return ""
    h = hashlib.sha256()
    try:
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(1024 * 1024), b""):
                h.update(chunk)
        return h.hexdigest()
    except Exception as e:
        logger.log_error(f"Failed to read host file for SHA256: {path}: {e}")
        return ""

def file_bytes_in_container(container_id: str, path_in_container: str) -> Optional[bytes]:
    """
    Read a file from inside the container using `docker exec cat`.
    Returns bytes on success, None on failure.
    """
    rc, out, err = run_nsenter(["docker", "exec", container_id, "cat", path_in_container], text=False)
    if rc != 0:
        logger.log_error(f"docker exec cat failed for {path_in_container}: {err.decode(errors='ignore')}")
        return None
    return out  # bytes

def file_sha256_in_container(container_id: str, path_in_container: str) -> str:
    """Compute SHA256 of a container file by reading its bytes."""
    data = file_bytes_in_container(container_id, path_in_container)
    if data is None:
        return ""
    h = hashlib.sha256()
    h.update(data)
    return h.hexdigest()

def copy_from_container(container_id: str, src_in_container: str, dst_on_host: str, mode: int) -> bool:
    """
    Copy file from container to host atomically.
    Tries `docker cp` first; if it fails, falls back to `docker exec cat` -> write bytes.
    """
    parent = os.path.dirname(dst_on_host) or "/"
    tmp_path = os.path.join("/tmp", os.path.basename(dst_on_host) + ".tmp")

    # Ensure parent directory exists
    try:
        os.makedirs(parent, exist_ok=True)
    except Exception as e:
        logger.log_error(f"Failed to ensure parent dir {parent}: {e}")
        return False

    # Try docker cp
    rc, _, err = run_nsenter(["docker", "cp", f"{container_id}:{src_in_container}", tmp_path], text=True)
    if rc != 0:
        logger.log_debug(f"'docker cp' failed, falling back to exec+cat: {err.strip()}")
        # Fallback to reading bytes then writing locally
        data = file_bytes_in_container(container_id, src_in_container)
        if data is None:
            return False
        try:
            with open(tmp_path, "wb") as f:
                f.write(data)
        except Exception as e:
            logger.log_error(f"Failed writing temp file {tmp_path}: {e}")
            return False

    try:
        os.chmod(tmp_path, mode)
        os.replace(tmp_path, dst_on_host)
    except Exception as e:
        logger.log_error(f"Failed finalizing copy to {dst_on_host}: {e}")

        try:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
        except Exception:
            pass
        return False

    logger.log_info(f"Updated {dst_on_host}")
    return True

# ----------------- core sync logic -----------------
def sync_items(container_id: str, items: List[SyncItem]) -> bool:
    """
    For each item:
      - compute container SHA256
      - compare with host SHA256
      - if different/missing, copy (docker cp, fallback exec+cat), set mode, atomic replace
      - verify final SHA256
      - if updated and verified, run any configured post-copy actions
    Returns True if all items succeeded or were already up to date.
    """
    all_ok = True
    for item in items:
        cont_sha = file_sha256_in_container(container_id, item.src_in_container)
        if not cont_sha:
            logger.log_error(f"Cannot compute SHA256 for {item.src_in_container} in container {container_id}")
            all_ok = False
            continue

        host_sha = file_sha256_host(item.dst_on_host)
        if host_sha == cont_sha:
            logger.log_info(f"{os.path.basename(item.dst_on_host)} up-to-date (sha256={host_sha})")
            continue

        logger.log_info(
            f"{os.path.basename(item.dst_on_host)} differs "
            f"(container {cont_sha} vs host {host_sha or 'missing'}), updating…"
        )
        if not copy_from_container(container_id, item.src_in_container, item.dst_on_host, item.mode):
            logger.log_error(f"Copy/update failed for {item.dst_on_host}")
            all_ok = False
            continue

        # Verify
        new_sha = file_sha256_host(item.dst_on_host)
        if new_sha != cont_sha:
            logger.log_error(
                f"Post-copy SHA mismatch for {item.dst_on_host}: host {new_sha} vs container {cont_sha}"
            )
            all_ok = False
        else:
            logger.log_info(f"Sync complete for {item.dst_on_host} (sha256={new_sha})")
            # Run post-copy host actions for this file, if any
            run_host_actions_for(item.dst_on_host)

    return all_ok

def ensure_sync() -> bool:
    cid = get_telemetry_container_id()
    if not cid:
        logger.log_info("telemetry container not found; skipping sync")
        return True
    return sync_items(cid, SYNC_ITEMS)

def main() -> None:
    ok = ensure_sync()
    if not ok:
        logger.log_error("One or more sync operations encountered errors")

if __name__ == "__main__":
    while True:
        main()
        time.sleep(SYNC_INTERVAL_S)
