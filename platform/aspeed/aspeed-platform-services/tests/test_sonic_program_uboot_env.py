"""
Unit tests for the U-Boot env vendor hook lookup in sonic-program-uboot-env.sh.

The TFTP installer ramdisk is shared by every Aspeed board, so the script must
run only the hook in the detected platform's own device directory (next to its
installer.conf). Other boards keep the default boot_guard.
"""

import os
import subprocess

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_PATH = os.path.join(
    os.path.dirname(TEST_DIR), "scripts", "sonic-program-uboot-env.sh")
HOOK_NAME = "sonic-uboot-env-vendor-hook"
DEFAULT_BOOT_GUARD = "test 1 = 1"

STUBS = {
    # Unset variables read as missing; the script's env readability probe must succeed.
    "fw_printenv": '#!/bin/sh\n[ "$1" = "-n" ] && exit 1\nexit 0\n',
    "fw_setenv": '#!/bin/sh\nprintf "%s\\t%s\\n" "$1" "$2" >> "$FW_SETENV_CALLS"\n',
    "blkid": "#!/bin/sh\necho 0000-test\n",
}


def write_script(path, text):
    path.write_text(text)
    path.chmod(0o755)


def make_platform_dir(tmp_path, hook=None):
    """Create a platform device dir with installer.conf and an optional hook."""
    platform_dir = tmp_path / "device" / "vendor" / "arm64-vendor_board-r0"
    platform_dir.mkdir(parents=True)
    (platform_dir / "installer.conf").write_text("CONSOLE_SPEED=115200\n")
    if hook is not None:
        write_script(platform_dir / HOOK_NAME, hook)
    return platform_dir


def run_program_uboot_env(tmp_path, installer_conf):
    """Run the script with stub fw_* tools; return (exit code, fw_setenv calls, stderr)."""
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    for name, text in STUBS.items():
        write_script(bin_dir / name, text)
    calls = tmp_path / "fw_setenv.calls"

    env = dict(os.environ,
               PATH="%s:%s" % (bin_dir, os.environ.get("PATH", "")),
               FW_SETENV_CALLS=str(calls),
               BOOTCONF_DT_PATHS=str(tmp_path / "no-bootconf"),
               UBOOT_ENV_BOOT_DEVICE="/dev/mmcblk0p1",
               UBOOT_ENV_DEMO_DEV="/dev/mmcblk0",
               UBOOT_ENV_DEMO_PART="1",
               UBOOT_ENV_DISK_INTERFACE="mmc",
               UBOOT_ENV_IMAGE_DIR="image-test")
    env.pop("SONIC_UBOOT_ENV_LOG_FILE", None)
    env.pop("UBOOT_ENV_INSTALLER_CONF", None)
    if installer_conf is not None:
        env["UBOOT_ENV_INSTALLER_CONF"] = str(installer_conf)
    result = subprocess.run(["sh", SCRIPT_PATH], env=env,
                            capture_output=True, check=False)
    lines = calls.read_text().splitlines() if calls.exists() else []
    return result.returncode, [tuple(line.split("\t", 1)) for line in lines], \
        result.stderr.decode()


def boot_guard_values(calls):
    return [value for name, value in calls if name == "boot_guard"]


def test_platform_hook_runs_after_defaults(tmp_path):
    platform_dir = make_platform_dir(
        tmp_path, hook='#!/bin/sh\nfw_setenv boot_guard "board_guard"\n')
    rc, calls, _ = run_program_uboot_env(tmp_path, platform_dir / "installer.conf")
    assert rc == 0
    assert boot_guard_values(calls) == [DEFAULT_BOOT_GUARD, "board_guard"]


def test_platform_without_hook_keeps_default_boot_guard(tmp_path):
    platform_dir = make_platform_dir(tmp_path)
    rc, calls, _ = run_program_uboot_env(tmp_path, platform_dir / "installer.conf")
    assert rc == 0
    assert boot_guard_values(calls) == [DEFAULT_BOOT_GUARD]


def test_no_installer_conf_keeps_default_boot_guard(tmp_path):
    rc, calls, _ = run_program_uboot_env(tmp_path, None)
    assert rc == 0
    assert boot_guard_values(calls) == [DEFAULT_BOOT_GUARD]


def test_non_executable_hook_is_skipped_with_warning(tmp_path):
    platform_dir = make_platform_dir(
        tmp_path, hook='#!/bin/sh\nfw_setenv boot_guard "board_guard"\n')
    (platform_dir / HOOK_NAME).chmod(0o644)
    rc, calls, stderr = run_program_uboot_env(tmp_path, platform_dir / "installer.conf")
    assert rc == 0
    assert boot_guard_values(calls) == [DEFAULT_BOOT_GUARD]
    assert "WARNING: vendor U-Boot env hook %s is not executable; skipping" % (
        platform_dir / HOOK_NAME) in stderr


def test_failing_hook_is_not_fatal(tmp_path):
    platform_dir = make_platform_dir(tmp_path, hook="#!/bin/sh\nexit 3\n")
    rc, _, stderr = run_program_uboot_env(tmp_path, platform_dir / "installer.conf")
    assert rc == 0
    assert "WARNING: vendor U-Boot env hook %s failed" % (
        platform_dir / HOOK_NAME) in stderr
