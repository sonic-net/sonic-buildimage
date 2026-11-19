"""
Unit tests for the boot_guard seam in sonic-program-uboot-env.sh.

bootcmd gates the boot payload on `run boot_guard`. If boot_guard cannot be
written, that gate would fail forever and the board would stop at the U-Boot
prompt, so the script must fall back to an unguarded bootcmd. When the write
succeeds the guarded bootcmd must be byte-identical to what shipped before.

The script is run for real with stub fw_setenv/fw_printenv on PATH.
"""

import os
import subprocess

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(os.path.dirname(TEST_DIR), "scripts",
                      "sonic-program-uboot-env.sh")

BOOT_PAYLOAD = ('test -n "$boot_once" && setenv do_boot_once "$boot_once" && '
                'setenv boot_once "" && saveenv && run do_boot_once; run boot_next')
GUARDED_BOOTCMD = "run print_menu; if run boot_guard; then %s; fi" % BOOT_PAYLOAD
UNGUARDED_BOOTCMD = "run print_menu; %s" % BOOT_PAYLOAD

STUB_FW_SETENV = r"""#!/bin/sh
echo "$*" >> "$STUB_CALLS"
[ "$1" = "${STUB_FAIL_VAR:-}" ] && exit 1
exit 0
"""

# The script probes `fw_printenv bootcmd` to confirm the env is readable, and
# reads a few values with -n; report an empty environment otherwise.
STUB_FW_PRINTENV = r"""#!/bin/sh
if [ "$1" = "-n" ]; then
    exit 1
fi
echo "$1="
exit 0
"""


def _run(tmp_path, fail_var=None):
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    for name, body in (("fw_setenv", STUB_FW_SETENV),
                       ("fw_printenv", STUB_FW_PRINTENV)):
        p = bin_dir / name
        p.write_text(body)
        p.chmod(0o755)
    calls = tmp_path / "calls"

    env = dict(os.environ,
               PATH="%s:%s" % (bin_dir, os.environ.get("PATH", "")),
               STUB_CALLS=str(calls),
               UBOOT_ENV_BOOT_DEVICE=str(tmp_path / "no-such-dev"),
               UBOOT_ENV_DEMO_DEV=str(tmp_path / "no-such-disk"),
               UBOOT_ENV_DEMO_PART="1",
               UBOOT_ENV_DISK_INTERFACE="mmc",
               UBOOT_ENV_IMAGE_DIR="image-test",
               BOOTCONF_DT_PATHS=str(tmp_path / "no-such-dt"))
    env.pop("UBOOT_ENV_INSTALLER_CONF", None)
    env.pop("SONIC_UBOOT_ENV_LOG_FILE", None)
    if fail_var:
        env["STUB_FAIL_VAR"] = fail_var
    # The script is installed with its mode by debian/rules; in the tree it is
    # a plain file, so run it through sh as its shebang would.
    result = subprocess.run(["sh", SCRIPT], env=env, capture_output=True,
                            text=True, check=False)
    settings = {}
    for line in calls.read_text().splitlines() if calls.exists() else []:
        name, _, value = line.partition(" ")
        settings[name] = value
    return result, settings


def test_guarded_bootcmd_when_boot_guard_written(tmp_path):
    result, settings = _run(tmp_path)
    assert result.returncode == 0, result.stderr
    assert settings["boot_guard"] == "test 1 = 1"
    assert settings["bootcmd"] == GUARDED_BOOTCMD


def test_unguarded_bootcmd_when_boot_guard_write_fails(tmp_path):
    result, settings = _run(tmp_path, fail_var="boot_guard")
    assert result.returncode == 0, result.stderr
    assert "boot_guard" not in settings["bootcmd"]
    assert settings["bootcmd"] == UNGUARDED_BOOTCMD
    assert "Failed to set boot_guard" in result.stderr


def test_bootcmd_write_order(tmp_path):
    # boot_guard must be in flash before the bootcmd that runs it.
    result, _ = _run(tmp_path)
    assert result.returncode == 0, result.stderr
    names = [l.split(" ", 1)[0]
             for l in (tmp_path / "calls").read_text().splitlines()]
    assert names.index("boot_guard") < names.index("bootcmd")
