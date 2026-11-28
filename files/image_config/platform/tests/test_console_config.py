import os
from pathlib import Path
import re
import subprocess

import pytest


REPO_ROOT = Path(__file__).resolve().parents[4]
RC_LOCAL = REPO_ROOT / "files" / "image_config" / "platform" / "rc.local"
GOLDFINCH = REPO_ROOT / "device" / "arista" / "arm64-arista_goldfinch-r0"
BAUD_FILE = "serial-getty@.service.d/10-sonic-console-baud.conf"
FLOW_FILE = (
    "serial-getty@ttyS12.service.d/20-sonic-console-flow-control.conf"
)
EXECSTARTS = [
    (
        r"ExecStart=-/sbin/agetty -o '-p -- \\u' --keep-baud "
        "115200,57600,38400,9600 - $TERM"
    ),
    (
        r"ExecStart=-/sbin/agetty -o '-- \\u' --noreset --noclear "
        "--keep-baud 115200,38400,9600 - ${TERM}"
    ),
    (
        r"ExecStart=-/usr/sbin/agetty -o '-p -- \\u' "
        "115200,38400,9600 - $TERM"
    ),
]


def console_function(source):
    start = source.index("program_console_speed()\n")
    end = source.index("\nrequest_dpu_midplane_ip()", start)
    return source[start:end]


def run_console_config(tmp_path, execstart=EXECSTARTS[0],
                       cmdline="console=ttyS12,115200n8", config=None,
                       source=None, platform="test-platform"):
    systemd_dir = tmp_path / "systemd"
    systemd_dir.mkdir(exist_ok=True)
    device_dir = tmp_path / "device"
    platform_dir = device_dir / platform
    platform_dir.mkdir(parents=True, exist_ok=True)
    config_path = platform_dir / "platform_env.conf"
    if config is None:
        config_path.unlink(missing_ok=True)
    else:
        config_path.write_text(config)
    unit = tmp_path / "serial-getty@.service"
    unit.write_text("[Service]\n" + execstart + "\nTTYReset=yes\n")
    cmdline_path = tmp_path / "cmdline"
    cmdline_path.write_text(cmdline)
    calls = tmp_path / "systemctl.calls"
    function = console_function(RC_LOCAL.read_text() if source is None else source)
    function = function.replace("/proc/cmdline", str(cmdline_path))
    function = function.replace(
        "/lib/systemd/system/serial-getty@.service", str(unit))
    function = function.replace("/etc/systemd/system/", str(systemd_dir) + "/")
    function = function.replace("/usr/share/sonic/device/", str(device_dir) + "/")
    script = (
        'systemctl() { printf "%s\\n" "$*" >> "$SYSTEMCTL_CALLS"; }\n'
        + function + "\nprogram_console_speed\n"
    )
    env = dict(os.environ, platform=platform, SYSTEMCTL_CALLS=str(calls),
               console_hardware_flow_control="ttyS99")
    result = subprocess.run(["sh", "-c", script], env=env, text=True,
                            capture_output=True, check=True)
    files = {
        str(path.relative_to(systemd_dir)): path.read_text()
        for path in systemd_dir.rglob("*.conf")
    }
    return files, calls.read_text().splitlines(), result.stdout


def dropin(execstart):
    return "[Service]\nExecStart=\n" + execstart + "\n"


def baud_execstart(execstart, baud):
    return re.sub(
        r"--keep-baud [0-9][0-9,]*", baud,
        re.sub(r"u' [0-9][0-9,]* ", "u' " + baud + " ", execstart))


@pytest.mark.parametrize("execstart", EXECSTARTS)
@pytest.mark.parametrize("config", [None, "", "switch_bmc=1\n",
                                   "console_hardware_flow_control=\n"])
def test_default_getty_behavior_is_unchanged(tmp_path, execstart, config):
    files, calls, output = run_console_config(
        tmp_path, execstart=execstart, config=config)
    assert files == {BAUD_FILE: dropin(baud_execstart(execstart, "115200"))}
    assert calls == ["daemon-reload"]
    assert output == ""


@pytest.mark.parametrize("execstart", EXECSTARTS)
@pytest.mark.parametrize("cmdline, baud", [
    ("console=ttyS12,115200n8", "115200"),
    ("console=ttyS12,9600n8", "9600"),
    ("console=ttyAMA0,57600n8", "57600"),
    ("console=ttyS12,57600n8 console=ttyS12,115200n8", "57600"),
    ("console=tty0", "9600"),
    ("", "9600"),
])
def test_opt_in_preserves_baud_and_login_options(tmp_path, execstart,
                                               cmdline, baud):
    files, calls, output = run_console_config(
        tmp_path, execstart=execstart, cmdline=cmdline,
        config="console_hardware_flow_control=ttyS12\n")
    expected = baud_execstart(execstart, baud)
    assert files == {
        BAUD_FILE: dropin(expected),
        FLOW_FILE: dropin(expected.replace("agetty ", "agetty --flow-control ", 1)),
    }
    assert calls == ["daemon-reload"]
    assert output == ""


def test_existing_flow_control_option_is_not_duplicated(tmp_path):
    execstart = EXECSTARTS[0].replace("agetty ", "agetty --flow-control ", 1)
    files, _, _ = run_console_config(
        tmp_path, execstart=execstart,
        config="console_hardware_flow_control=ttyS12\n")
    assert files[FLOW_FILE].count("--flow-control") == 1
    assert files[FLOW_FILE] == files[BAUD_FILE]


@pytest.mark.parametrize("tty", [
    "1", "ttyS", "ttyAMA", "ttyS12x", "ttyUSB0", "/dev/ttyS12",
    "ttyS12/../../other", "ttyS12 ttyS13", "ttyS12\nttyS13",
])
def test_invalid_uart_is_reported_without_enabling_flow_control(tmp_path, tty):
    files, _, output = run_console_config(
        tmp_path, config="console_hardware_flow_control='%s'\n" % tty)
    assert set(files) == {BAUD_FILE}
    assert "invalid console_hardware_flow_control" in output


@pytest.mark.parametrize("tty", ["ttyS0", "ttyS12", "ttyAMA0"])
def test_only_the_selected_getty_instance_is_overridden(tmp_path, tty):
    files, _, _ = run_console_config(
        tmp_path, config="console_hardware_flow_control=%s\n" % tty)
    selected = (
        "serial-getty@%s.service.d/20-sonic-console-flow-control.conf" % tty
    )
    assert set(files) == {BAUD_FILE, selected}
    assert "--flow-control" not in files[BAUD_FILE]


def test_regeneration_removes_only_sonic_owned_stale_overrides(tmp_path):
    systemd_dir = tmp_path / "systemd"
    old_dir = systemd_dir / "serial-getty@ttyS13.service.d"
    old_dir.mkdir(parents=True)
    (old_dir / "20-sonic-console-flow-control.conf").write_text("stale\n")
    (old_dir / "override.conf").write_text("administrator\n")
    files, _, _ = run_console_config(
        tmp_path, config="console_hardware_flow_control=ttyS12\n")
    assert "serial-getty@ttyS13.service.d/20-sonic-console-flow-control.conf" \
        not in files
    assert files["serial-getty@ttyS13.service.d/override.conf"] == "administrator\n"
    repeated, calls, _ = run_console_config(
        tmp_path, config="console_hardware_flow_control=ttyS12\n")
    assert repeated == files
    assert calls == ["daemon-reload", "daemon-reload"]
    opted_out, _, _ = run_console_config(tmp_path)
    assert set(opted_out) == {
        BAUD_FILE, "serial-getty@ttyS13.service.d/override.conf",
    }


@pytest.mark.parametrize("execstart, warning", [
    ("", "no ExecStart"),
    ("ExecStart=-/sbin/agetty --noclear - $TERM", "could not place"),
])
def test_unrecognized_packaged_unit_removes_stale_generated_files(
        tmp_path, execstart, warning):
    run_console_config(tmp_path, config="console_hardware_flow_control=ttyS12\n")
    files, _, output = run_console_config(
        tmp_path, execstart=execstart,
        config="console_hardware_flow_control=ttyS12\n")
    assert files == {}
    assert warning in output


def test_unrecognized_getty_command_keeps_baud_only_override(tmp_path):
    execstart = EXECSTARTS[0].replace("/sbin/agetty", "/sbin/other-getty")
    files, _, output = run_console_config(
        tmp_path, execstart=execstart,
        config="console_hardware_flow_control=ttyS12\n")
    assert files == {BAUD_FILE: dropin(baud_execstart(execstart, "115200"))}
    assert "could not enable hardware flow control" in output


def test_failing_platform_environment_is_reported_without_enabling_flow_control(
        tmp_path):
    files, _, output = run_console_config(
        tmp_path, config="console_hardware_flow_control=ttyS12\nfalse\n")
    assert set(files) == {BAUD_FILE}
    assert "could not read platform environment" in output


def test_platform_is_resolved_before_console_configuration_on_every_boot():
    source = RC_LOCAL.read_text()
    machine = source.index("\n. /host/machine.conf\n")
    platform = source.index('\nplatform=""\n', machine)
    console = source.index("\nprogram_console_speed\n", platform)
    first_boot = source.index("\nif [ -f $FIRST_BOOT_FILE ]; then", console)
    assert machine < platform < console < first_boot
    assert "platform=$aboot_platform" in source[platform:console]
    assert "platform=$onie_platform" in source[platform:console]


def test_goldfinch_is_the_only_platform_opt_in(tmp_path):
    configs = list((REPO_ROOT / "device").rglob("platform_env.conf"))
    enabled = [
        path for path in configs
        if re.search(r"^console_hardware_flow_control=.+$", path.read_text(),
                     re.MULTILINE)
    ]
    assert enabled == [GOLDFINCH / "platform_env.conf"]
    installer = (GOLDFINCH / "installer.conf").read_text()
    assert re.search(r"^CONSOLE_DEV=12$", installer, re.MULTILINE)
    assert re.search(r"^CONSOLE_SPEED=115200$", installer, re.MULTILINE)
    files, _, _ = run_console_config(
        tmp_path, config=enabled[0].read_text(),
        platform="arm64-arista_goldfinch-r0")
    assert set(files) == {BAUD_FILE, FLOW_FILE}


@pytest.mark.parametrize("tty", ["ttyS0", "ttyS12", "ttyAMA0"])
def test_generated_instance_dropin_is_secureboot_allowlisted(tmp_path, tty):
    allowlist = (
        REPO_ROOT / "files" / "image_config" / "secureboot"
        / "allowlist_paths.conf"
    )
    patterns = tmp_path / "allowlist.patterns"
    prefix = str(tmp_path / "rw") + "/"
    patterns.write_text("".join(
        prefix + pattern + "$\n"
        for pattern in allowlist.read_text().splitlines() if pattern
    ))
    generated = prefix + (
        "etc/systemd/system/serial-getty@%s.service.d/"
        "20-sonic-console-flow-control.conf\n" % tty
    )
    result = subprocess.run(
        ["grep", "-q", "-f", str(patterns)], input=generated,
        text=True, check=False)
    assert result.returncode == 0
    administrator = generated.replace(
        "20-sonic-console-flow-control.conf", "override.conf")
    result = subprocess.run(
        ["grep", "-q", "-f", str(patterns)], input=administrator,
        text=True, check=False)
    assert result.returncode == 1
