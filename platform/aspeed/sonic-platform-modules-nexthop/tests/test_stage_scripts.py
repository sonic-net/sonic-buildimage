"""
Unit tests for the Nexthop boot-stage shell scripts.

watchdog-stage-marker.sh and storm-guard-clear.sh talk to hardware only through
`busybox devmem`, so each test runs the real script with a stub busybox on PATH
that records its arguments and returns a scripted read value or failure.

The register address and magic clear value live only in the marker script on
the SONiC side (hw-watchdog-mgrd writes its markers by running the script), so
the literals below are pinned here to catch an unintended change.
"""

import os
import re
import subprocess

import pytest

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
NEXTHOP_DIR = os.path.dirname(TEST_DIR)
SCRIPTS_DIR = os.path.join(NEXTHOP_DIR, "common", "scripts")
MARKER_SH = os.path.join(SCRIPTS_DIR, "watchdog-stage-marker.sh")
STORMGUARD_SH = os.path.join(SCRIPTS_DIR, "storm-guard-clear.sh")

# Pinned copies of the scripts' constants (checked below); the real ones are
# shared only with the Nexthop U-Boot.
STAGE_REG = "0x14c3704c"
MAGIC_CLEAR = "0xEA000000"
STORMGUARD_REG = "0x14C021A0"

STUB_BUSYBOX = r"""#!/bin/sh
# Stub busybox: only "devmem ADDR 32 [VALUE]" is used by the scripts.
echo "$*" >> "$STUB_CALLS"
[ "$1" = "devmem" ] || { echo "stub: unexpected applet $1" >&2; exit 99; }
if [ $# -eq 3 ]; then
    [ -n "${STUB_READ_FAIL:-}" ] && exit 1
    echo "${STUB_READ_VALUE:-0x00000000}"
    exit 0
fi
[ -n "${STUB_WRITE_FAIL:-}" ] && exit 1
[ "$4" = "${STUB_WRITE_FAIL_VALUE:-}" ] && exit 1
exit 0
"""


def _run(tmp_path, script, args, read_value=None, read_fail=False,
         write_fail=False, write_fail_value=None):
    """Run `script` with a stub busybox; return (returncode, stderr, calls)."""
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir(exist_ok=True)
    busybox = bin_dir / "busybox"
    busybox.write_text(STUB_BUSYBOX)
    busybox.chmod(0o755)
    calls = tmp_path / "calls"
    if calls.exists():
        calls.unlink()

    env = dict(os.environ,
               PATH="%s:%s" % (bin_dir, os.environ.get("PATH", "")),
               STUB_CALLS=str(calls))
    if read_value is not None:
        env["STUB_READ_VALUE"] = read_value
    if read_fail:
        env["STUB_READ_FAIL"] = "1"
    if write_fail:
        env["STUB_WRITE_FAIL"] = "1"
    if write_fail_value is not None:
        env["STUB_WRITE_FAIL_VALUE"] = write_fail_value
    result = subprocess.run([script] + list(args), env=env,
                            capture_output=True, text=True, check=False)
    lines = calls.read_text().splitlines() if calls.exists() else []
    return result.returncode, result.stderr, lines


READ = "devmem %s 32" % STAGE_REG
CLEAR = "devmem %s 32 %s" % (STAGE_REG, MAGIC_CLEAR)


def write(value):
    return "devmem %s 32 %s" % (STAGE_REG, value)


# ----------------------------------------------------------- stage marker

def test_marker_advances_when_higher(tmp_path):
    rc, _, calls = _run(tmp_path, MARKER_SH, ["0x23"], read_value="0x00000022")
    assert rc == 0
    assert calls == [READ, CLEAR, write("0x23")]


def test_marker_writes_from_uninitialized(tmp_path):
    rc, _, calls = _run(tmp_path, MARKER_SH, ["0x22"], read_value="0x00000000")
    assert rc == 0
    assert calls == [READ, CLEAR, write("0x22")]


def test_marker_skips_when_equal(tmp_path):
    rc, _, calls = _run(tmp_path, MARKER_SH, ["0x22"], read_value="0x00000022")
    assert rc == 0
    assert calls == [READ]


def test_marker_never_moves_backwards(tmp_path):
    # hw-watchdog-mgrd can stamp 0x24 (armed) before the systemd stage units
    # run; the lower-valued stage markers must not regress it.
    rc, _, calls = _run(tmp_path, MARKER_SH, ["0x22"], read_value="0x00000024")
    assert rc == 0
    assert calls == [READ]


def test_marker_compares_numerically_not_lexically(tmp_path):
    # 0x4 (U-Boot pre-kernel) is below 0x22 even though "0x4" > "0x22" as text.
    rc, _, calls = _run(tmp_path, MARKER_SH, ["0x22"], read_value="0x4")
    assert rc == 0
    assert calls == [READ, CLEAR, write("0x22")]


def test_marker_writes_when_read_fails(tmp_path):
    # An unreadable register must not suppress the marker: attribution is
    # best-effort, so fall through to clear-then-set.
    rc, _, calls = _run(tmp_path, MARKER_SH, ["0x22"], read_fail=True)
    assert rc == 0
    assert calls == [READ, CLEAR, write("0x22")]


def test_marker_writes_when_read_is_garbage(tmp_path):
    rc, _, calls = _run(tmp_path, MARKER_SH, ["0x22"], read_value="garbage")
    assert rc == 0
    assert calls == [READ, CLEAR, write("0x22")]


def test_marker_failed_clear_skips_set(tmp_path):
    # If the magic clear fails, setting bits on top of the stale value would
    # corrupt the marker, so the set is skipped. Reported, but exit 0: losing
    # attribution must never turn into a failed boot stage.
    rc, err, calls = _run(tmp_path, MARKER_SH, ["0x22"],
                          read_value="0x00000000", write_fail=True)
    assert rc == 0
    assert "failed to clear" in err
    assert calls == [READ, CLEAR]


def test_marker_failed_set_is_best_effort(tmp_path):
    rc, err, calls = _run(tmp_path, MARKER_SH, ["0x22"],
                          read_value="0x00000000", write_fail_value="0x22")
    assert rc == 0
    assert "failed to write marker 0x22" in err
    assert calls == [READ, CLEAR, write("0x22")]


@pytest.mark.parametrize("args", [
    [],                 # missing value
    ["0x22", "0x23"],   # too many
    ["22"],             # not hex-prefixed
    ["0x"],             # empty digits
    ["0xZZ"],           # not hex
    ["0x123456789"],    # wider than 32 bits
    ["-0x22"],
])
def test_marker_rejects_bad_arguments(tmp_path, args):
    # Previously a missing/invalid argument was evaluated as 0 and, if the
    # register read failed, the marker was magic-cleared and lost.
    rc, err, calls = _run(tmp_path, MARKER_SH, args, read_fail=True)
    assert rc == 2
    assert "usage" in err.lower()
    assert calls == []


def test_marker_accepts_uppercase_hex(tmp_path):
    rc, _, calls = _run(tmp_path, MARKER_SH, ["0X2A"], read_value="0x00000022")
    assert rc == 0
    assert calls == [READ, CLEAR, write("0X2A")]


# ------------------------------------------------------- storm-guard clear

def test_stormguard_clear_writes_zero(tmp_path):
    rc, _, calls = _run(tmp_path, STORMGUARD_SH, [])
    assert rc == 0
    assert calls == ["devmem %s 32 0" % STORMGUARD_REG]


def test_stormguard_clear_reports_failure(tmp_path):
    # Unlike the stage marker, a failed counter clear is a real problem: the
    # next WDT reset would count against the storm limit.
    rc, err, calls = _run(tmp_path, STORMGUARD_SH, [], write_fail=True)
    assert rc == 1
    assert "failed to clear" in err
    assert calls == ["devmem %s 32 0" % STORMGUARD_REG]


# ------------------------------------------------ constants are pinned

def _shell_assignment(path, name):
    text = open(path).read()
    m = re.search(r'^%s="?(0[xX][0-9a-fA-F]+)"?\s*$' % re.escape(name), text,
                  re.MULTILINE)
    assert m, "%s not found in %s" % (name, path)
    return int(m.group(1), 0)


def test_script_constants_match_pinned_values():
    # These addresses are compiled into the Nexthop U-Boot. A change here
    # must be deliberate and paired with a U-Boot change.
    assert _shell_assignment(MARKER_SH, "WDT_STAGE_SCRATCH") == int(STAGE_REG, 0)
    assert _shell_assignment(MARKER_SH, "WDT_STAGE_SCRATCH_MAGIC_CLEAR") == \
        int(MAGIC_CLEAR, 0)
    assert _shell_assignment(STORMGUARD_SH, "STORMGUARD_COUNT") == \
        int(STORMGUARD_REG, 0)


def test_scripts_use_strict_mode():
    for path in (MARKER_SH, STORMGUARD_SH):
        head = open(path).read().split("\n", 40)[:40]
        assert any(line.strip() == "set -euo pipefail" for line in head), \
            "%s does not enable strict mode" % path
