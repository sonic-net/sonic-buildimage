"""Record startup and resource evidence before a failed private Bazel server is removed."""

import json
import os
from pathlib import Path
import sys
import time


# JVM output may echo arbitrary options or environment values. Publish only
# known error phrases, never entire lines, stack traces, or the launch command.
JVM_ERRORS = (
    "Error occurred during initialization of VM", "Could not create the Java Virtual Machine",
    "Could not reserve enough space", "Native memory allocation", "Cannot allocate memory",
    "OutOfMemoryError", "unable to create native thread", "pthread_create failed",
    "Resource temporarily unavailable", "Too many open files", "No space left on device",
    "Permission denied", "Address already in use", "Unrecognized VM option",
    "SIGSEGV", "SIGBUS", "SIGILL", "SIGABRT",
)


def jvm_errors(base):
    log = base / "server/jvm.out"
    try:
        if log.parent.is_symlink() or log.is_symlink() or not log.is_file():
            return {"available": False}
        with log.open("rb") as stream:
            size = os.fstat(stream.fileno()).st_size
            stream.seek(max(0, size - 16384))
            tail = stream.read(16384)
    except OSError:
        return {"available": False}
    text = tail.decode(errors="replace").lower()
    return {"available": True, "bytes_scanned": len(tail),
            "error_phrases": [phrase for phrase in JVM_ERRORS if phrase.lower() in text]}


def read_counters(root, names):
    result = {}
    for name in names:
        try:
            result[name] = (root / name).read_text().strip()
        except OSError:
            pass
    return result


def snapshot(base, proc=Path("/proc"), mount=Path("/sys/fs/cgroup")):
    report = {"captured_epoch_seconds": time.time(), "jvm": jvm_errors(base),
              "load_average_1_5_15_min": list(os.getloadavg()),
              "host_pressure": read_counters(proc / "pressure", ("cpu", "memory", "io"))}
    group = read_counters(proc / "self", ("cgroup",)).get("cgroup", "")
    for line in group.splitlines():
        if not line.startswith("0::/"):
            continue
        relative = Path(line[4:])
        if relative.is_absolute() or ".." in relative.parts:
            break
        names = ("memory.current", "memory.max", "memory.high", "memory.peak", "memory.events",
                 "memory.swap.current", "memory.swap.max", "pids.current", "pids.max",
                 "pids.events", "cpu.max", "cpu.pressure", "memory.pressure", "io.pressure")
        # A nested slave process can see an unlimited leaf under a limited container.
        report["cgroup_v2"] = {"mount_root": read_counters(mount, names)}
        if relative.parts:
            report["cgroup_v2"]["process"] = read_counters(mount / relative, names)
        break
    try:
        disk = os.statvfs(base)
        report["output_base_disk"] = {"available_bytes": disk.f_bavail * disk.f_frsize,
                                      "available_inodes": disk.f_favail}
    except OSError:
        pass
    return report


if __name__ == "__main__":
    base, directory = map(Path, sys.argv[1:])
    (directory / "startup-diagnostics.json").write_text(json.dumps(snapshot(base), indent=2) + "\n")
