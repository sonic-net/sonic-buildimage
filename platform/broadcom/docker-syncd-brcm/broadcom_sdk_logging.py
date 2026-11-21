#!/usr/bin/env python3
"""Capture Broadcom SDK diagnostics without changing persistent platform files.

The SDK pre-init hook enables its existing file sink before init bcm. The
monitor copies/truncates the append-mode file at 1 MiB, retaining two bounded
backups, and reports explicit scache exhaustion to supervisord's stderr/syslog.
"""

import datetime
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time

LOG_DIR = Path("/var/log/broadcom-sdk")
RUNTIME_DIR = Path("/etc/sai.d/sdk-logging")
MAX_BYTES = 1024 * 1024
BACKUPS = 2
POLL_SECONDS = 0.25
SCACHE = re.compile(
    r"Scache out of space\.\s*max=(\d{1,20}) bytes, used=(\d{1,20}) bytes, alloc_size=(\d{1,20}) bytes"
)
MODULE = re.compile(r"bcm_init failed in (\S+) ")


def report(message):
    print("Broadcom SDK diagnostics: " + message, file=sys.stderr, flush=True)


def properties(text):
    """Match the SDK/SAI last-value-wins convention, retaining source text separately."""
    values = {}
    for line in text.splitlines():
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
    return values


def override_properties(text, updates):
    # Remove existing active definitions so SDK parser precedence is irrelevant.
    lines = [
        line
        for line in text.splitlines()
        if line.lstrip().startswith("#")
        or "=" not in line
        or line.split("=", 1)[0].strip() not in updates
    ]
    return (
        "\n".join(lines)
        + "\n"
        + "".join(key + "=" + str(value) + "\n" for key, value in updates.items())
    )


def command_path(path):
    # These names are embedded in SDK command text. Do not introduce SDK-shell
    # quoting/expansion ambiguities for an unusual platform-supplied pathname.
    if not re.fullmatch(r"/[A-Za-z0-9_./:+-]+", str(path)):
        raise ValueError("unsupported SDK command pathname: " + str(path))
    return str(path)


def retain_tail(source, destination, limit=MAX_BYTES):
    with source.open("rb") as src:
        src.seek(max(0, source.stat().st_size - limit))
        data = src.read(limit)
    destination.write_bytes(data)
    destination.chmod(0o600)


def rotate(path, limit=MAX_BYTES):
    for number in range(BACKUPS, 1, -1):
        older = Path(str(path) + "." + str(number - 1))
        if older.exists():
            older.replace(str(path) + "." + str(number))
    retain_tail(path, Path(str(path) + ".1"), limit)
    # The SDK opens the file with append=yes, so copytruncate does not leave
    # a sparse hole or require the vendor process to reopen its descriptor.
    with path.open("r+b") as stream:
        stream.truncate(0)


def prepare(command, log_dir=LOG_DIR, runtime_dir=RUNTIME_DIR):
    """Return rewritten argv plus context, or unchanged argv for non-BCM profiles."""
    indices = [i + 1 for i, arg in enumerate(command[:-1]) if arg == "-p"]
    if not indices:
        return command, None
    index = indices[-1]
    profile = Path(command[index])
    profile_text = profile.read_text()
    bcm = Path(properties(profile_text).get("SAI_INIT_CONFIG_FILE", ""))
    # YAML/LTSW/DNX profiles need their own validated logging mechanism.
    if not str(bcm).endswith(".bcm"):
        return command, None
    bcm_text = bcm.read_text()
    values = properties(bcm_text)
    previous_preinit = values.get("sai_preinit_cmd_file")
    preinit_text = Path(previous_preinit).read_text() if previous_preinit else ""
    log_dir.mkdir(parents=True, exist_ok=True, mode=0o700)
    runtime_dir.mkdir(parents=True, exist_ok=True, mode=0o700)
    log = log_dir / "startup.log"
    command_path(log)
    preinit = runtime_dir / "preinit.soc"
    command_path(preinit)
    runtime_bcm = runtime_dir / "config.bcm"
    runtime_profile = runtime_dir / "startup.profile"
    # A copytruncate archive is bounded even after a previous interrupted run.
    if log.exists() and log.stat().st_size:
        rotate(log)
    log.touch(mode=0o600)
    log.chmod(0o600)
    try:
        version = subprocess.check_output(
            ["dpkg-query", "-W", "-f=${Version}", "libsaibcm"],
            text=True,
            stderr=subprocess.DEVNULL,
            timeout=5,
        ).strip()
    except (OSError, subprocess.SubprocessError):
        version = "unknown"
    context = {
        "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "source_profile": str(profile),
        "source_bcm": str(bcm),
        "stable_size": values.get("stable_size", "unspecified"),
        "sai_package": version,
        "raw_log": str(log),
    }
    with log.open("a") as stream:
        stream.write("SONiC SDK startup: " + json.dumps(context, sort_keys=True) + "\n")
    preinit.write_text(
        "log file=" + str(log) + " append=yes quiet=yes on\n"
        "debug soc common error\nversion\n" + preinit_text + "\n"
    )
    runtime_bcm.write_text(
        override_properties(bcm_text, {"sai_preinit_cmd_file": preinit})
    )
    runtime_profile.write_text(
        override_properties(profile_text, {"SAI_INIT_CONFIG_FILE": runtime_bcm})
    )
    for path in (preinit, runtime_bcm, runtime_profile):
        path.chmod(0o600)
    metadata = log_dir / "startup.json"
    metadata.write_text(json.dumps(context, indent=2) + "\n")
    metadata.chmod(0o600)
    rewritten = list(command)
    rewritten[index] = str(runtime_profile)
    return rewritten, context


class Capture:
    def __init__(self, context, limit=MAX_BYTES):
        self.context = context
        self.path = Path(context["raw_log"])
        self.limit = limit
        self.offset = 0
        self.partial = b""
        self.scache_seen = False
        self.module_seen = False

    def line(self, line):
        match = SCACHE.search(line)
        if match:
            if self.scache_seen:
                return
            maximum, used, requested = map(int, match.groups())
            message = (
                "stable-cache exhausted: maximum={0} used={1} requested={2} available={3}; "
                "BCM={4} stable_size={5} SAI={6}; retained log={7}"
            ).format(
                maximum,
                used,
                requested,
                maximum - used,
                self.context["source_bcm"],
                self.context["stable_size"],
                self.context["sai_package"],
                self.path,
            )
            self.scache_seen = True
        elif self.scache_seen and not self.module_seen and MODULE.search(line):
            self.module_seen = True
            message = "initialization failed in SDK module " + MODULE.search(line)[1]
        else:
            return
        report(message)

    def consume(self, chunk):
        lines = (self.partial + chunk).split(b"\n")
        self.partial = lines.pop()[-8192:]
        for line in lines:
            self.line(line.decode(errors="replace").rstrip("\r"))

    def poll(self):
        with self.path.open("rb") as stream:
            if self.path.stat().st_size < self.offset:
                self.offset = 0
                self.partial = b""
            stream.seek(self.offset)
            # Bound work per poll so a noisy SDK cannot starve child/signal handling.
            for _ in range(16):
                chunk = stream.read(65536)
                if not chunk:
                    break
                self.offset += len(chunk)
                self.consume(chunk)
            size = self.path.stat().st_size
            if size >= self.limit:
                # Inspect the retained tail as well when a burst exceeded this
                # poll's read budget. Do not join lines across the skipped gap.
                if size > self.offset:
                    stream.seek(max(self.offset, size - self.limit))
                    self.partial = b""
                    self.consume(stream.read(self.limit))
                rotate(self.path, self.limit)
                self.offset = 0
                self.partial = b""


def run(command, log_dir=LOG_DIR, runtime_dir=RUNTIME_DIR):
    try:
        rewritten, context = prepare(command, log_dir, runtime_dir)
    except (OSError, ValueError) as error:
        report("capture unavailable; starting syncd unchanged: " + str(error))
        rewritten, context = command, None
    if context is None:
        os.execv(command[0], command)
    report(
        "capturing SDK output in "
        + context["raw_log"]
        + "; BCM="
        + context["source_bcm"]
    )
    child = subprocess.Popen(rewritten)
    previous = {}

    def forward(signum, frame):
        try:
            child.send_signal(signum)
        except ProcessLookupError:
            pass

    for sig in (signal.SIGTERM, signal.SIGINT):
        previous[sig] = signal.signal(sig, forward)
    capture = Capture(context)
    capture_error = False
    try:
        while True:
            try:
                capture.poll()
                capture_error = False
            except OSError as error:
                if not capture_error:
                    report("capture I/O failed; syncd remains running: " + str(error))
                capture_error = True
            status = child.poll()
            if status is not None:
                # A final read catches a short-lived initialization failure.
                try:
                    capture.poll()
                    if capture.partial:
                        capture.line(capture.partial.decode(errors="replace"))
                except OSError:
                    pass
                return status if status >= 0 else 128 - status
            time.sleep(POLL_SECONDS)
    finally:
        for sig, handler in previous.items():
            signal.signal(sig, handler)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit("Usage: broadcom_sdk_logging.py COMMAND [ARGS...]")
    raise SystemExit(run(sys.argv[1:]))
