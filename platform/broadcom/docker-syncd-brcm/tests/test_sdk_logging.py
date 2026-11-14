"""Hardware-free tests for SDK config instrumentation and failure diagnostics."""

import importlib.util
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
from unittest.mock import patch

import jinja2
import pytest

SOURCE = Path(__file__).resolve().parents[1]
ROOT = SOURCE.parents[2]
spec = importlib.util.spec_from_file_location(
    "sdk_logging", SOURCE / "broadcom_sdk_logging.py"
)
sdk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sdk)
ERROR = "0:_bcm_esw_scache_ptr_get: Scache out of space.max=89128960 bytes, used=88887228 bytes, alloc_size=458756 bytes\n"
MODULE = "0:_bcm_modules_init: bcm_init failed in mim (No resources for operation)\n"


@pytest.fixture
def config(tmp_path):
    vendor = tmp_path / "vendor.soc"
    vendor.write_text("# retain vendor setup\nm0 load 0 0x0 firmware.bin\n")
    bcm = tmp_path / "source.bcm"
    bcm.write_text(
        "# SDK config\nstable_size=0x5500000\nportmap_1=1:25\nsai_preinit_cmd_file="
        + str(vendor)
        + "\n"
    )
    profile = tmp_path / "sai.profile"
    profile.write_text("SAI_INIT_CONFIG_FILE=" + str(bcm) + "\nOTHER_SETTING=keep\n")
    return tmp_path, profile, bcm, vendor


def prepare(config):
    root, profile, _, _ = config
    command = [
        "/usr/bin/dsserve",
        "/usr/bin/syncd",
        "--diag",
        "-u",
        "-p",
        str(profile),
        "-s",
    ]
    with patch.object(sdk.subprocess, "check_output", return_value="15.2.test"):
        return sdk.prepare(command, root / "logs", root / "runtime")


def test_runtime_copies_preserve_platform_files_and_command_arguments(config):
    root, profile, bcm, vendor = config
    originals = {p: p.read_bytes() for p in (profile, bcm, vendor)}
    command, context = prepare(config)
    assert command[:5] == ["/usr/bin/dsserve", "/usr/bin/syncd", "--diag", "-u", "-p"]
    assert command[-1] == "-s"
    runtime_profile = Path(command[5])
    runtime_bcm = Path(
        sdk.properties(runtime_profile.read_text())["SAI_INIT_CONFIG_FILE"]
    )
    runtime_preinit = Path(
        sdk.properties(runtime_bcm.read_text())["sai_preinit_cmd_file"]
    )
    assert runtime_preinit.read_text().endswith(vendor.read_text() + "\n")
    assert runtime_preinit.read_text().startswith(
        "log file=" + str(root / "logs/startup.log")
    )
    assert "append=yes" in runtime_preinit.read_text()
    assert "\nversion\n" in runtime_preinit.read_text()
    assert runtime_bcm.read_text().count("\nsai_preinit_cmd_file=") == 1
    assert sdk.properties(runtime_bcm.read_text())["stable_size"] == "0x5500000"
    assert sdk.properties(runtime_bcm.read_text())["portmap_1"] == "1:25"
    assert {p: p.read_bytes() for p in originals} == originals
    assert context["source_bcm"] == str(bcm)
    assert context["sai_package"] == "15.2.test"
    assert runtime_profile.stat().st_mode & 0o777 == 0o600
    assert (root / "logs/startup.json").stat().st_mode & 0o777 == 0o600


def test_profile_without_preinit_and_unknown_package_version(config):
    root, profile, bcm, _ = config
    bcm.write_text("stable_size=0x8000000\n")
    with patch.object(
        sdk.subprocess, "check_output", side_effect=OSError("not installed")
    ):
        _, context = sdk.prepare(
            ["/usr/bin/syncd", "-p", str(profile)], root / "logs", root / "runtime"
        )
    assert context["sai_package"] == "unknown"
    assert "version" in (root / "runtime/preinit.soc").read_text()


@pytest.mark.parametrize(
    "command", [["/usr/bin/syncd"], ["/usr/bin/syncd", "-p", "profile.yml"]]
)
def test_unsupported_profiles_are_not_rewritten(tmp_path, command):
    if "profile.yml" in command:
        profile = tmp_path / "profile.yml"
        profile.write_text("SAI_INIT_CONFIG_FILE=/vendor/config.yml\n")
        command[-1] = str(profile)
    assert sdk.prepare(command, tmp_path / "logs", tmp_path / "runtime") == (
        command,
        None,
    )
    assert not (tmp_path / "logs").exists()


def test_explicit_failure_is_reported_across_partial_reads(config, capsys):
    _, context = prepare(config)
    capture = sdk.Capture(context)
    path = Path(context["raw_log"])
    with path.open("a") as stream:
        stream.write(ERROR[:80])
    capture.poll()
    assert "exhausted" not in capsys.readouterr().err
    with path.open("a") as stream:
        stream.write(ERROR[80:] + MODULE)
    capture.poll()
    output = capsys.readouterr().err
    assert "stable-cache exhausted" in output
    assert "maximum=89128960" in output and "available=241732" in output
    assert "requested=458756" in output and "SDK module mim" in output
    assert "stable_size=0x5500000" in output
    with path.open("a") as stream:
        stream.write(ERROR + MODULE)
    capture.poll()
    assert capsys.readouterr().err == ""


def test_generic_init_failure_is_not_misclassified(config, capsys):
    _, context = prepare(config)
    capture = sdk.Capture(context)
    with Path(context["raw_log"]).open("a") as stream:
        stream.write("init bcm failed, rc=-1\nSAI_STATUS_FAILURE\n" + MODULE)
    capture.poll()
    assert capsys.readouterr().err == ""


def test_copytruncate_retains_bounded_backups_and_append_writer(config):
    _, context = prepare(config)
    path = Path(context["raw_log"])
    capture = sdk.Capture(context, limit=1024)
    for _ in range(5):
        with path.open("ab") as writer:
            writer.write(b"x" * 2048 + b"\n")
            writer.flush()
            capture.poll()
            writer.write(b"next\n")
        assert path.read_bytes() == b"next\n"
    backups = list(path.parent.glob("startup.log.*"))
    assert len(backups) == 2
    assert all(p.stat().st_size <= 1024 for p in backups)
    assert capture.partial == b""


def test_startup_rotates_prior_capture_without_losing_context(config):
    root, _, _, _ = config
    prepare(config)
    path = root / "logs/startup.log"
    path.write_text("previous failure evidence\n")
    prepare(config)
    assert Path(str(path) + ".1").read_text() == "previous failure evidence\n"
    assert "source_bcm" in path.read_text()


def test_unreadable_vendor_preinit_fails_open(config, capsys):
    root, profile, _, vendor = config
    vendor.unlink()
    original = ["/usr/bin/syncd", "-p", str(profile)]
    with patch.object(sdk.os, "execv", side_effect=SystemExit(0)) as execute:
        with pytest.raises(SystemExit):
            sdk.run(original, root / "logs", root / "runtime")
    execute.assert_called_once_with(original[0], original)
    assert "starting syncd unchanged" in capsys.readouterr().err


@pytest.mark.parametrize("status", [0, 2])
def test_real_child_exit_and_final_sdk_error_are_preserved(config, capsys, status):
    root, profile, _, _ = config
    child = root / "fake_syncd.py"
    child.write_text("""import pathlib,sys
p=pathlib.Path(sys.argv[sys.argv.index('-p')+1])
def props(t): return dict(l.split('=',1) for l in t.splitlines() if '=' in l and not l.startswith('#'))
b=pathlib.Path(props(p.read_text())['SAI_INIT_CONFIG_FILE'])
s=pathlib.Path(props(b.read_text())['sai_preinit_cmd_file'])
log=s.read_text().splitlines()[0].split('file=',1)[1].split()[0]
with open(log,'a') as f: f.write(%r)
sys.exit(%d)
""" % (ERROR + MODULE, status))
    with patch.object(sdk.subprocess, "check_output", return_value="test-sai"):
        assert (
            sdk.run(
                [sys.executable, str(child), "-p", str(profile)],
                root / "logs",
                root / "runtime",
            )
            == status
        )
    assert "stable-cache exhausted" in capsys.readouterr().err


def test_capture_io_failure_does_not_stop_child(config, capsys):
    root, profile, _, _ = config
    with patch.object(sdk.Capture, "poll", side_effect=OSError("disk full")):
        assert (
            sdk.run(
                [sys.executable, "-c", "import sys;sys.exit(0)", "-p", str(profile)],
                root / "logs",
                root / "runtime",
            )
            == 0
        )
    assert "syncd remains running" in capsys.readouterr().err


def test_container_template_limits_mount_to_broadcom_syncd():
    env = jinja2.Environment(undefined=jinja2.ChainableUndefined)
    template = env.from_string(
        (ROOT / "files/build_templates/docker_image_ctl.j2").read_text()
    )
    for asic, container, wanted in [
        ("broadcom", "syncd", True),
        ("broadcom", "swss", False),
        ("mellanox", "syncd", False),
        ("vs", "syncd", False),
    ]:
        rendered = template.render(
            sonic_asic_platform=asic,
            docker_container_name=container,
            docker_image_run_opt="",
            docker_image_name="docker-syncd-brcm",
            install_debug_image="n",
        )
        assert ("/var/log/broadcom-sdk/${DEV:-default}" in rendered) is wanted
        result = subprocess.run(
            ["bash", "-n"], input=rendered, text=True, capture_output=True
        )
        assert result.returncode == 0, result.stderr


def test_error_at_end_of_large_burst_is_not_lost(config, capsys):
    _, context = prepare(config)
    path = Path(context["raw_log"])
    with path.open("ab") as stream:
        stream.write(b"x\n" * sdk.MAX_BYTES + ERROR.encode() + MODULE.encode())
    capture = sdk.Capture(context)
    capture.poll()
    assert "stable-cache exhausted" in capsys.readouterr().err
    assert path.stat().st_size == 0
    assert Path(str(path) + ".1").stat().st_size == sdk.MAX_BYTES


def test_oversized_numbers_do_not_crash_capture(config, capsys):
    _, context = prepare(config)
    capture = sdk.Capture(context)
    capture.line(
        "Scache out of space.max="
        + "9" * 5000
        + " bytes, used=1 bytes, alloc_size=2 bytes"
    )
    assert capsys.readouterr().err == ""


@pytest.mark.parametrize(
    "image",
    ["docker-syncd-brcm-dnx", "docker-syncd-brcm-legacy-th", "docker-syncd-mlnx"],
)
def test_other_syncd_images_do_not_gain_capture_mount(image):
    env = jinja2.Environment(undefined=jinja2.ChainableUndefined)
    rendered = env.from_string(
        (ROOT / "files/build_templates/docker_image_ctl.j2").read_text()
    ).render(
        sonic_asic_platform="broadcom",
        docker_container_name="syncd",
        docker_image_name=image,
        docker_image_run_opt="",
    )
    assert "BRCM_SDK_LOG_MOUNT" not in rendered


def test_duplicate_active_keys_are_replaced_without_changing_comments():
    text = "# stable_size=old\nkey=first\nother=retain\n key=second\n"
    assert (
        sdk.override_properties(text, {"key": "new"})
        == "# stable_size=old\nother=retain\nkey=new\n"
    )


def test_sigterm_is_forwarded_and_child_exit_code_preserved(config):
    root, profile, _, _ = config
    ready = root / "ready"
    child = root / "signal_child.py"
    child.write_text(
        "import signal,sys,time,pathlib\nsignal.signal(signal.SIGTERM,lambda *args:sys.exit(23))\npathlib.Path(sys.argv[1]).touch()\nwhile True: time.sleep(0.01)\n"
    )
    code = 'import importlib.util,sys;from pathlib import Path;s=importlib.util.spec_from_file_location("sdk",sys.argv[1]);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);sys.exit(m.run([sys.executable,sys.argv[2],sys.argv[3],"-p",sys.argv[4]],Path(sys.argv[5]),Path(sys.argv[6])))'
    process = subprocess.Popen(
        [
            sys.executable,
            "-c",
            code,
            str(SOURCE / "broadcom_sdk_logging.py"),
            str(child),
            str(ready),
            str(profile),
            str(root / "logs"),
            str(root / "runtime"),
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        start_new_session=True,
    )
    try:
        deadline = time.monotonic() + 5
        while not ready.exists() and time.monotonic() < deadline:
            if process.poll() is not None:
                raise AssertionError(process.communicate())
            time.sleep(0.01)
        assert ready.exists()
        process.send_signal(signal.SIGTERM)
        process.communicate(timeout=5)
        assert process.returncode == 23
    finally:
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
