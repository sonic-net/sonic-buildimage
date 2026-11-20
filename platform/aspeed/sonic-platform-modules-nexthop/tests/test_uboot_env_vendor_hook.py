"""
Unit tests for the Nexthop U-Boot env vendor hook.

sonic-program-uboot-env.sh runs the executable hook found in the detected
platform's device directory, so the hook ships there and sets boot_guard to
the Nexthop storm-guard U-Boot command.
"""

import os
import subprocess

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(TEST_DIR))))
HOOK_PATH = os.path.join(
    REPO_ROOT, "device", "nexthop", "arm64-nexthop_b27-r0",
    "sonic-uboot-env-vendor-hook")


def run_hook(tmp_path):
    """Run the hook with a stub fw_setenv; return (exit code, fw_setenv calls)."""
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    calls = tmp_path / "fw_setenv.calls"
    fw_setenv = bin_dir / "fw_setenv"
    fw_setenv.write_text('#!/bin/sh\necho "$*" >> "%s"\n' % calls)
    fw_setenv.chmod(0o755)

    env = dict(os.environ, PATH="%s:%s" % (bin_dir, os.environ.get("PATH", "")))
    env.pop("SONIC_UBOOT_ENV_LOG_FILE", None)
    result = subprocess.run([HOOK_PATH], env=env, capture_output=True, check=False)
    lines = calls.read_text().splitlines() if calls.exists() else []
    return result.returncode, lines


def test_hook_is_executable():
    assert os.access(HOOK_PATH, os.X_OK)


def test_hook_sets_stormguard(tmp_path):
    rc, calls = run_hook(tmp_path)
    assert rc == 0
    assert calls == ["boot_guard nh_stormguard"]  # SpellCheck-ignoreWord: stormguard
