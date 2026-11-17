#!/usr/bin/env python3
"""Check the installed shared SWSS image using the common runtime probe."""

import hashlib
import json
from pathlib import Path
import platform
import sys

import runtime_smoke


def main():
    report = json.loads(Path(sys.argv[1]).read_bytes())
    if report["status"] != "passed" or platform.machine() != "x86_64":
        raise ValueError("expected validated native AMD64 image")
    for name, item in report["native"].items():
        binary = Path("/") / name
        if hashlib.sha256(binary.read_bytes()).hexdigest() != item["sha256"]:
            raise ValueError("installed source bytes differ: " + name)
        info = binary.stat()
        if (info.st_uid, info.st_gid, info.st_mode & 0o7777) != (item["uid"], item["gid"], item["mode"]):
            raise ValueError("installed source permissions differ: " + name)
    inputs = {"architecture": "amd64", "elfs": report["elfs"], "debug_pairs": [], "policy": {
        "python_imports": ["swsscommon.swsscommon", "dash_api.acl_group_pb2", "dash_api.utils"],
        "commands": [{"argv": ["/usr/local/bin/supervisord", "--help"], "output_pattern": "nodaemon"}],
    }}
    result = runtime_smoke.probe(inputs, "runtime")
    print(json.dumps({"status": "passed" if result["passed"] else "failed",
                      "machine": result["machine"], "source_elf_count": len(report["native"]),
                      "loader_checks": [{"path": item["path"], "passed": item["passed"],
                                         "returncode": item["returncode"]} for item in result["loader_checks"]],
                      "python_imports_passed": all(item["passed"] for item in result["python_imports"]),
                      "supervisord_passed": all(item["passed"] for item in result["commands"]),
                      "failures": result["failures"]}, indent=2))
    if not result["passed"]:
        print(json.dumps(result, indent=2), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
