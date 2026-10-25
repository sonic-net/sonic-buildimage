#!/usr/bin/env python3
"""Best-effort, private phase receipt for the final VS image Make recipe.

Only ``end`` publishes a fixed-schema summary. Children, including sudo -E
children, append to the same caller-owned inode; no build output is captured.
Every expected transition must be present before a receipt can be published.
"""
import fcntl
import json
import os
import stat
import sys
import tempfile
import time
from contextlib import contextmanager


ENV = "SONIC_IMAGE_TIMING_FILE"
PREFIX = "sonic-image-timing-"
MARKER = "[ image phases ] [ target/sonic-vs.img.gz ] "
MAX_BYTES = 16384
MAX_SEGMENTS = 64
PHASES = {"assembly", "boot-checks", "compression"}
CONTEXTS = {"recipe", "filesystem", "installer", "bios", "uefi", "4-asic", "6-asic"}
FILESYSTEM = [
    ("assembly", "recipe"),
    ("assembly", "filesystem"),
    ("compression", "filesystem"),
    ("assembly", "filesystem"),
    ("compression", "filesystem"),
    ("assembly", "recipe"),
    ("assembly", "installer"),
]


def image_events(context):
    return [("assembly", context), ("boot-checks", context),
            ("assembly", context), ("compression", context), ("assembly", "recipe")]


SINGLE = FILESYSTEM + image_events("bios") + image_events("uefi")
MULTI = SINGLE + image_events("4-asic") + image_events("6-asic")


def now_us():
    # Quantize endpoints, then subtract: every emitted segment telescopes exactly.
    return time.monotonic_ns() // 1000


@contextmanager
def receipt(path):
    if os.path.dirname(path) != "/tmp" or not os.path.basename(path).startswith(PREFIX):
        raise ValueError("not a timing receipt")
    fd = os.open(path, os.O_RDWR | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if (not stat.S_ISREG(info.st_mode) or stat.S_IMODE(info.st_mode) != 0o600
                or info.st_nlink != 1 or info.st_size > MAX_BYTES
                or os.geteuid() not in (0, info.st_uid)):
            raise ValueError("unsafe timing receipt")
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        with os.fdopen(fd, "r+", encoding="ascii") as stream:
            fd = None
            yield stream, info
    finally:
        if fd is not None:
            os.close(fd)


def remove_receipt(path, info):
    current = os.lstat(path)
    if (current.st_dev, current.st_ino) == (info.st_dev, info.st_ino):
        os.unlink(path)


def read_events(stream):
    events = [json.loads(line) for line in stream.read(MAX_BYTES + 1).splitlines()]
    if not 1 <= len(events) <= MAX_SEGMENTS:
        raise ValueError("invalid event count")
    previous = -1
    for event in events:
        if (not isinstance(event, list) or len(event) != 3
                or type(event[0]) is not int or not previous <= event[0] <= 2**63 - 1
                or event[1] not in PHASES or event[2] not in CONTEXTS):
            raise ValueError("invalid event")
        previous = event[0]
    return events


def expected(events, complete=False):
    labels = [(event[1], event[2]) for event in events]
    return any(labels == (pattern if complete else pattern[:len(labels)])
               for pattern in (SINGLE, MULTI))


def begin():
    fd, path = tempfile.mkstemp(prefix=PREFIX, dir="/tmp")
    try:
        with os.fdopen(fd, "w", encoding="ascii") as stream:
            stream.write(json.dumps([now_us(), "assembly", "recipe"], separators=(",", ":")) + "\n")
        return path
    except Exception:
        os.unlink(path)
        raise


def switch(path, phase, context):
    with receipt(path) as (stream, _):
        events = read_events(stream)
        event = [now_us(), phase, context]
        if (len(events) >= MAX_SEGMENTS or phase not in PHASES or context not in CONTEXTS
                or event[0] < events[-1][0] or not expected(events + [event])):
            # A missed/duplicate transition invalidates this receipt permanently.
            stream.seek(0, os.SEEK_END)
            stream.write("invalid\n")
            return
        stream.seek(0, os.SEEK_END)
        stream.write(json.dumps(event, separators=(",", ":")) + "\n")


def end(path):
    with receipt(path) as (stream, info):
        try:
            events = read_events(stream)
            finish = now_us()
            if not expected(events, complete=True) or finish < events[-1][0]:
                return None
            start = events[0][0]
            ends = [event[0] for event in events[1:]] + [finish]
            value = {
                "version": 1,
                "duration_seconds": (finish - start) / 1_000_000,
                "segments": [
                    {"phase": phase, "context": context,
                     "offset_seconds": (timestamp - start) / 1_000_000,
                     "duration_seconds": (stop - timestamp) / 1_000_000}
                    for (timestamp, phase, context), stop in zip(events, ends)
                ],
            }
            encoded = json.dumps(value, separators=(",", ":"), allow_nan=False)
            return MARKER + encoded if len(encoded) <= MAX_BYTES else None
        finally:
            remove_receipt(path, info)


def main(argv):
    # Telemetry must never change the build's exit status or expose diagnostics.
    try:
        if argv == ["begin"]:
            print(begin())
            return
        path = os.environ.get(ENV)
        if not path:
            return
        if len(argv) == 3 and argv[0] == "switch":
            switch(path, argv[1], argv[2])
        elif argv == ["end"]:
            value = end(path)
            if value:
                print(value)
        elif argv == ["discard"]:
            with receipt(path) as (_, info):
                remove_receipt(path, info)
    except Exception:
        pass


if __name__ == "__main__":
    main(sys.argv[1:])
