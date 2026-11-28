#!/usr/bin/env python3
"""Reject an undersized native K3s installer workspace before preparing Make."""

import argparse
import json
import os
from pathlib import Path
import sys


REQUIRED_AVAILABLE_BYTES = 256 * 1024**3


def check(workspace, output):
    workspace = workspace.resolve()
    report = {
        "schema": 1,
        "workspace": str(workspace),
        "required_available_bytes": REQUIRED_AVAILABLE_BYTES,
        "available_bytes": None,
        "measurement": "statvfs.f_bavail * statvfs.f_frsize",
        "scope": "entry_guard_only_not_a_reservation_or_quota",
        "status": "failed",
    }
    try:
        usage = os.statvfs(workspace)
        available = usage.f_bavail * usage.f_frsize
        report["available_bytes"] = available
        if available >= REQUIRED_AVAILABLE_BYTES:
            report["status"] = "passed"
        else:
            report["error"] = "Insufficient free space for the native VS installer"
    except OSError as error:
        report["error"] = f"Cannot measure workspace free space: {error}"
    serialized = json.dumps(report, indent=2, sort_keys=True) + "\n"
    # Early rejection may precede the private helper and its artifact privacy
    # gate. Keep the safe measurement in the job log even without an upload.
    print(serialized, end="", flush=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(serialized)
    if report["status"] != "passed":
        print(f"{report['error']}; available_bytes={report['available_bytes']}, "
              f"required_available_bytes={REQUIRED_AVAILABLE_BYTES}", file=sys.stderr)
        return 1
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    return check(args.workspace, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
