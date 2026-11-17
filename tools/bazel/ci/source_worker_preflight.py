#!/usr/bin/env python3
"""Measure the dedicated source-image worker without building packages."""

import argparse
import grp
import json
import os
from pathlib import Path
import platform
import re
import shutil
import stat
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.public_artifacts import safe_json

WORKERS = tuple(f"sonic-slave-lgh114-{number}" for number in range(1, 5))
MINIMUM_MEMORY_BYTES = 20 * 1024**3


def select_build_profile(memory_bytes, cpus):
    """Choose scheduling from the admitted worker's measured cgroup limits."""
    if (type(memory_bytes) is not int or memory_bytes < MINIMUM_MEMORY_BYTES or
            type(cpus) not in (int, float) or not 2 <= cpus <= 6):
        raise ValueError("Worker does not meet the existing bounded resource gate")
    parallel = cpus >= 4
    return {"name": "two_actions" if parallel else "one_action", "status": "pilot",
            "minimum_memory_bytes": MINIMUM_MEMORY_BYTES, "minimum_cpu": 4 if parallel else 2,
            "measured_memory_limit_bytes": memory_bytes, "measured_cpu_limit": cpus,
            "jvm_heap_gib": 3, "jobs": 2 if parallel else 1, "cpu": 2 if parallel else 1,
            "scheduler_memory_mb": 8000 if parallel else 4000, "loaded_container_memory_gib": 4}


def cpu_limit(value):
    quota, period = value.split()
    if quota == "max" or int(quota) <= 0 or int(period) <= 0:
        raise ValueError("A positive finite CPU quota is required")
    return int(quota) / int(period)


def marker_identity(path):
    info = path.lstat()
    if not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o222:
        raise ValueError("Marker must be a root-owned read-only regular file")
    value = json.loads(path.read_text())
    expected = {"uid": 1000, "gid": 1000, "runner_root": "/opt/actions-runner", "work_root": "/actions/_work"}
    if any(value.get(key) != item for key, item in expected.items()):
        raise ValueError("Unexpected runner profile")
    if not re.fullmatch(r"sha256:[0-9a-f]{64}", value.get("base_image", "")):
        raise ValueError("Missing immutable SONiC slave image")
    return {"base_image": value["base_image"]}


def measure(expected_worker):
    checks, facts = {}, {}
    build_profile = None

    def check(name, probe):
        try:
            checks[name] = bool(probe())
        except (OSError, ValueError, KeyError, subprocess.SubprocessError):
            checks[name] = False

    actual = os.environ.get("RUNNER_NAME", "")
    check("worker_affinity", lambda: actual == expected_worker and actual in WORKERS)
    facts["worker"] = actual if actual in WORKERS else "unexpected"
    facts["uid"], facts["gid"] = os.getuid(), os.getgid()
    check("build_user", lambda: facts["uid"] == facts["gid"] == 1000)
    facts["machine"] = platform.machine()
    check("native_amd64", lambda: facts["machine"] == "x86_64")
    check("trixie", lambda: platform.freedesktop_os_release().get("ID") == "debian"
          and platform.freedesktop_os_release().get("VERSION_CODENAME") == "trixie")
    check("container", lambda: Path("/.dockerenv").is_file())

    def marker():
        facts["slave"] = marker_identity(Path("/etc/sonic-slave-github-runner.json"))
        return True

    check("slave_identity", marker)
    cgroups = Path("/sys/fs/cgroup")
    check("private_cgroup_v2", lambda: Path("/proc/self/cgroup").read_text().strip() == "0::/"
          and (cgroups / "cgroup.type").read_text().strip() == "domain")
    check("delegated_controllers", lambda: {"cpu", "memory", "pids"}.issubset(
        (cgroups / "cgroup.controllers").read_text().split()))

    def resources():
        nonlocal build_profile
        facts["memory_limit"] = (cgroups / "memory.max").read_text().strip()
        facts["cpu_max"] = (cgroups / "cpu.max").read_text().strip()
        facts["memory_limit_bytes"] = int(facts["memory_limit"])
        facts["memory_current_bytes"] = int((cgroups / "memory.current").read_text())
        facts["cpu_limit"] = cpu_limit(facts["cpu_max"])
        build_profile = select_build_profile(facts["memory_limit_bytes"], facts["cpu_limit"])
        return True

    check("bounded_build_resources", resources)
    for name in ("bazel", "docker", "dockerd", "sudo", "git", "make", "readelf", "mountpoint", "findmnt", "unshare"):
        check("tool_" + name, lambda name=name: shutil.which(name) is not None)
    check("private_docker", lambda: not Path("/var/run/docker.sock").exists() and not any(
        os.environ.get(name) for name in ("DOCKER_HOST", "DOCKER_CONTEXT", "DOCKER_TLS_VERIFY", "DOCKER_CERT_PATH")))
    check("docker_group", lambda: grp.getgrnam("docker").gr_gid in os.getgroups())
    check("sudo", lambda: subprocess.run(["sudo", "-n", "true"], timeout=10, capture_output=True).returncode == 0)
    check("user_mount_namespace", lambda: subprocess.run(
        ["unshare", "--user", "--map-root-user", "--mount", "true"], timeout=10, capture_output=True).returncode == 0)
    for name, mount in (("workspace", "/actions"), ("cache", "/bazel_cache")):
        check(name + "_mount", lambda mount=mount: subprocess.run(
            ["mountpoint", "-q", mount], timeout=10, capture_output=True).returncode == 0)

        def storage(name=name, mount=mount):
            filesystem = subprocess.check_output(["findmnt", "-n", "-o", "FSTYPE", "-T", mount],
                                                 text=True, timeout=10).strip()
            free = shutil.disk_usage(mount).free
            facts[name + "_storage"] = {"filesystem": filesystem, "free_bytes": free}
            return filesystem in ("ext4", "xfs") and free >= 50 * 1024**3

        check(name + "_storage", storage)
    return {"schema": 1, "status": "passed" if all(checks.values()) else "failed", "checks": checks,
            "facts": facts, "build_profile": build_profile, "scope": "resource_and_namespace_preflight_only",
            "image_build_executed": False, "deb_generation_executed": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker", required=True, choices=WORKERS)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    receipt = measure(args.worker)
    receipt["dispatcher_revision"] = subprocess.check_output(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip()
    safe_json(receipt)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    print(json.dumps(receipt, indent=2, sort_keys=True))
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
