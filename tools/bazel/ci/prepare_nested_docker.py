#!/usr/bin/env python3
"""Prepare a disposable, private cgroup namespace for nested Docker."""

import errno
import json
import os
from pathlib import Path
import time


def require(condition, message):
    if not condition:
        raise ValueError(message)


def prepare():
    """Enable domain controllers without changing the host's cgroup hierarchy.

    The caller must start the slave with --cgroupns=private. As in Moby's
    hack/dind and the native VS worker, move its processes into a leaf before
    enabling controllers; otherwise nested runc cannot apply resource limits.
    https://github.com/moby/moby/blob/38805f20f9bcc5e87869d6c79d432b166e1c88b4/hack/dind
    """
    require(os.geteuid() == 0 and Path("/.dockerenv").is_file(),
            "nested Docker preparation must run as root inside its slave")
    root = Path("/sys/fs/cgroup")
    if not (root / "cgroup.controllers").exists():
        return {"version": 1}
    require(Path("/proc/self/cgroup").read_text().strip() == "0::/"
            and (root / "cgroup.type").read_text().strip() == "domain",
            "nested Docker preparation requires a fresh private domain")
    controllers = (root / "cgroup.controllers").read_text().split()
    require({"cpu", "memory", "pids"}.issubset(controllers),
            "nested Docker lacks delegated CPU, memory, or PID controllers")
    leaf = root / "sonic-docker-init"
    leaf.mkdir()
    for attempt in range(5):
        for pid in (root / "cgroup.procs").read_text().split():
            require(pid.isdecimal() and int(pid) > 0, "invalid slave cgroup process")
            try:
                (leaf / "cgroup.procs").write_text(pid)
            except ProcessLookupError:
                pass  # A process can exit between reading and moving it.
        try:
            (root / "cgroup.subtree_control").write_text(
                " ".join("+" + controller for controller in controllers))
            return {"version": 2, "controllers": controllers,
                    "process_leaf": leaf.name}
        except OSError as error:
            if error.errno != errno.EBUSY or attempt == 4:
                raise
            time.sleep(0.1)
    raise AssertionError("unreachable cgroup preparation state")


if __name__ == "__main__":
    print(json.dumps(prepare(), sort_keys=True))
