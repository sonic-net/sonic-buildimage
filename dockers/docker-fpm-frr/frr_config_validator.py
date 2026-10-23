#!/usr/bin/env python3

"""Check config values used as FRR command arguments before rendering."""

import json
import re
import subprocess
import sys


HOSTNAME_RE = re.compile(r"[A-Za-z0-9_](?:[A-Za-z0-9_.-]{0,251}[A-Za-z0-9_])?")


def read_device_metadata():
    result = subprocess.run(
        ["sonic-cfggen", "-d", "--var-json", "DEVICE_METADATA"],
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(result.stdout)


def validate_hostname(device_metadata):
    if not isinstance(device_metadata, dict):
        raise ValueError("DEVICE_METADATA is not a table")
    localhost = device_metadata.get("localhost")
    if not isinstance(localhost, dict):
        raise ValueError("DEVICE_METADATA localhost is not an entry")
    hostname = localhost.get("hostname")
    if not isinstance(hostname, str) or HOSTNAME_RE.fullmatch(hostname) is None:
        raise ValueError("DEVICE_METADATA localhost hostname is not valid for FRR")


def main():
    try:
        validate_hostname(read_device_metadata())
    except (OSError, json.JSONDecodeError, subprocess.CalledProcessError, ValueError) as error:
        print("FRR configuration validation failed: {}".format(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
