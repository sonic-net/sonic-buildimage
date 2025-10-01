#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import time
import argparse
import hashlib
import shlex
import subprocess
from dataclasses import dataclass
from typing import List, Optional, Tuple

from sonic_py_common import logger as log
logger = log.Logger()

def get_bool_env_var(name: str, default: bool = False) -> bool:
    val = os.getenv(name)
    if val is None:
        return default
    return val.strip().lower() in ("1", "true", "yes", "y", "on")

IS_V1_ENABLED = get_bool_env_var("IS_V1_ENABLED", default=False)

# ───────────── Config ─────────────
SYNC_INTERVAL_S = int(os.environ.get("SYNC_INTERVAL_S", "900"))  # seconds
NSENTER_BASE = ["nsenter", "--target", "1", "--pid", "--mount", "--uts", "--ipc", "--net"]

KUBE_USER = os.environ.get("KUBE_USER", "admin")
KUBE_CONFIG = "/etc/kubernetes/kubelet.conf"
KUBE_MODE = 0o600  # desired mode for kubeconfig + client cred files

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
logger.log_notice(f"IS_V1_ENABLED={IS_V1_ENABLED}; telemetry source set to {_TELEMETRY_SRC}")

SYNC_ITEMS: List[SyncItem] = [
    SyncItem(_TELEMETRY_SRC, "/usr/local/bin/telemetry.sh"),
    SyncItem("/usr/share/sonic/systemd_scripts/container_checker", "/bin/container_checker"),
]

# ───────────── subprocess helpers ─────────────
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

# ───────────── Host file ops via nsenter ─────────────
def host_read_bytes(path_on_host: str) -> Optional[bytes]:
    rc, out, _ = run_nsenter(["/bin/cat", path_on_host], text=False)
    if rc != 0:
        return None
    return out

def host_write_atomic(dst_on_host: str, data: bytes, mode: int) -> bool:
    tmp_path = f"/tmp/{os.path.basename(dst_on_host)}.tmp"

    rc, _, err = run_nsenter(["/bin/sh", "-lc", f"cat > {shlex.quote(tmp_path)}"], text=False, input_bytes=data)
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

def host_stat_info(path_on_host: str) -> Optional[Tuple[str, str, int]]:
    """Return (owner_name, group_name, mode_octal_int) for a host path, or None if missing/error."""
    rc, out, err = run_nsenter(["/usr/bin/stat", "-c", "%U %G %a", path_on_host], text=True)
    if rc != 0:
        return None
    try:
        owner, group, mode_str = out.strip().split()
        mode = int(mode_str, 8)
        return owner, group, mode
    except Exception:
        logger.log_error(f"stat parse failed for {path_on_host}: {out!r}")
        return None

def host_chown_mode(path_on_host: str, owner: str, mode: int) -> bool:
    ok = True
    rc, _, err = run_nsenter(["/bin/chown", f"{owner}:{owner}", path_on_host], text=True)
    if rc != 0:
        logger.log_error(f"chown failed for {path_on_host}: {str(err).strip()}")
        ok = False
    rc, _, err = run_nsenter(["/bin/chmod", f"{mode:o}", path_on_host], text=True)
    if rc != 0:
        logger.log_error(f"chmod failed for {path_on_host}: {str(err).strip()}")
        ok = False
    return ok

# ───────────── kube config ownership enforcement ─────────────
_CLIENT_PATH_RE = re.compile(r'^\s*client-(?:certificate|key)\s*:\s*(\S+)\s*$')

def parse_kube_client_paths_from_text(kubeconf_text: str) -> List[str]:
    paths: List[str] = []
    for line in kubeconf_text.splitlines():
        # strip trailing comments
        line = line.split('#', 1)[0]
        m = _CLIENT_PATH_RE.match(line)
        if not m:
            continue
        p = m.group(1).strip().strip('"').strip("'")
        if p and p not in paths:
            paths.append(p)
    return paths

def host_read_text(path_on_host: str) -> Optional[str]:
    b = host_read_bytes(path_on_host)
    return None if b is None else b.decode(errors="ignore")

def ensure_kube_credentials_owner_once() -> bool:
    """Immediate enforcement: fix kubeconfig & client files owner/mode if needed."""
    all_ok = True

    kc_text = host_read_text(KUBE_CONFIG)
    if kc_text is None:
        logger.log_warning(f"kubeconfig not readable on host: {KUBE_CONFIG}")
        return False

    # ensure kubeconfig itself
    info = host_stat_info(KUBE_CONFIG)
    if info is None or info[0] != KUBE_USER or info[2] != KUBE_MODE:
        logger.log_info(f"Fixing owner/mode for kubeconfig {KUBE_CONFIG} -> {KUBE_USER}:{KUBE_USER} {KUBE_MODE:o}")
        all_ok = host_chown_mode(KUBE_CONFIG, KUBE_USER, KUBE_MODE) and all_ok

    # ensure client cert/key(s)
    for p in parse_kube_client_paths_from_text(kc_text):
        info = host_stat_info(p)
        if info is None or info[0] != KUBE_USER or info[2] != KUBE_MODE:
            logger.log_info(f"Fixing owner/mode for kube client cred {p} -> {KUBE_USER}:{KUBE_USER} {KUBE_MODE:o}")
            all_ok = host_chown_mode(p, KUBE_USER, KUBE_MODE) and all_ok

    return all_ok

def enforce_kube_credentials_owner_periodic() -> bool:
    """Periodic enforcement: detect drift and correct it."""
    changed = False
    ok = True

    kc_text = host_read_text(KUBE_CONFIG)
    if kc_text is None:
        logger.log_warning(f"kubeconfig not readable on host: {KUBE_CONFIG}")
        return False

    # kubeconfig
    info = host_stat_info(KUBE_CONFIG)
    if info is None:
        logger.log_warning(f"stat failed for kubeconfig {KUBE_CONFIG}")
        ok = False
    else:
        if info[0] != KUBE_USER or info[2] != KUBE_MODE:
            logger.log_notice(
                f"Drift detected: {KUBE_CONFIG} owner/mode {info[0]}:{info[1]} {info[2]:o} -> {KUBE_USER}:{KUBE_USER} {KUBE_MODE:o}"
            )
            if host_chown_mode(KUBE_CONFIG, KUBE_USER, KUBE_MODE):
                changed = True
            else:
                ok = False

    # client cred(s)
    for p in parse_kube_client_paths_from_text(kc_text):
        info = host_stat_info(p)
        if info is None:
            logger.log_warning(f"stat failed for kube client cred {p}")
            ok = False
            continue
        if info[0] != KUBE_USER or info[2] != KUBE_MODE:
            logger.log_notice(
                f"Drift detected: {p} owner/mode {info[0]}:{info[1]} {info[2]:o} -> {KUBE_USER}:{KUBE_USER} {KUBE_MODE:o}"
            )
            if host_chown_mode(p, KUBE_USER, KUBE_MODE):
                changed = True
            else:
                ok = False

    if changed:
        logger.log_info("Kube credential ownership/mode corrected.")
    return ok

# ───────────── post-copy host actions ─────────────
POST_COPY_ACTIONS = {
    "/usr/local/bin/telemetry.sh": [
        ["sudo", "docker", "stop", "telemetry"],
        ["sudo", "docker", "rm", "telemetry"],
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "telemetry"],
    ],
    "/bin/container_checker": [
        ["sudo /bin/systemctl", "daemon-reload"],
        ["sudo /bin/systemctl", "restart", "monit"],
    ],
}

# ───────────── file Sync logic ─────────────
def read_file_bytes_local(path: str) -> Optional[bytes]:
    try:
        with open(path, "rb") as f:
            return f.read()
    except OSError as e:
        logger.log_error(f"read failed for {path}: {e}")
        return None

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

        # verify
        new_host_bytes = host_read_bytes(item.dst_on_host)
        new_sha = sha256_bytes(new_host_bytes)
        if new_sha != container_file_sha:
            logger.log_error(
                f"Post-copy SHA mismatch for {item.dst_on_host}: host {new_sha or 'read-failed'} vs container {container_file_sha}"
            )
            all_ok = False
        else:
            logger.log_info(f"Sync complete for {item.dst_on_host} (sha256={new_sha})")
            # Ensure kube credentials are fix-owned before restarting service
            if item.dst_on_host == "/usr/local/bin/telemetry.sh":
                ensure_kube_credentials_owner_once()
            run_host_actions_for(item.dst_on_host)

    return all_ok

def run_host_actions_for(path_on_host: str) -> None:
    actions = POST_COPY_ACTIONS.get(path_on_host, [])
    for cmd in actions:
        rc, _, err = run_nsenter(cmd, text=True)
        if rc == 0:
            logger.log_info(f"Post-copy action succeeded: {' '.join(cmd)}")
        else:
            logger.log_error(f"Post-copy action FAILED (rc={rc}): {' '.join(cmd)}; stderr={str(err).strip()}")

def ensure_sync() -> bool:
    ok_copy = sync_items(SYNC_ITEMS)
    ok_own = enforce_kube_credentials_owner_periodic()
    return ok_copy and ok_own

# ───────────── CLI ─────────────
def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Sync host scripts to the host via nsenter (with periodic kube-cred ownership enforcement).")
    p.add_argument("--once", action="store_true", help="Run one sync pass and exit")
    p.add_argument("--interval", type=int, default=SYNC_INTERVAL_S, help=f"Loop interval seconds (default: {SYNC_INTERVAL_S})")
    p.add_argument("--no-post-actions", action="store_true", help="Skip host systemctl/docker actions (debugging)")
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
