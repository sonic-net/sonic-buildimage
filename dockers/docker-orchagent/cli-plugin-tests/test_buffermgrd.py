import os
from pathlib import Path
import subprocess

import pytest


LAUNCHER = Path(__file__).resolve().parents[1] / "buffermgrd.sh"


@pytest.fixture
def run_buffermgrd(tmp_path, monkeypatch):
    """Run the launcher with temporary runtime paths and stub its two binaries."""
    sonic_dir = tmp_path / "sonic"
    config_dir = tmp_path / "config"
    bin_dir = tmp_path / "bin"
    for directory in (sonic_dir / "hwsku", sonic_dir / "platform", config_dir, bin_dir):
        directory.mkdir(parents=True)

    db_cli = bin_dir / "sonic-db-cli"
    db_cli.write_text('#!/usr/bin/env bash\nprintf "%s\\n" "$TEST_BUFFER_MODEL"\n')
    daemon = bin_dir / "buffermgrd"
    daemon.write_text('#!/usr/bin/env bash\nprintf "%s\\n" "$@" > "$TEST_ARGS_FILE"\n')
    db_cli.chmod(0o755)
    daemon.chmod(0o755)

    for name in ("BUFFERMGRD_ARGS", "BUFFERMGRD_PERIPHERIAL_ARGS", "BUFFERMGRD_ZERO_PROFILE_ARGS"):
        monkeypatch.delenv(name, raising=False)
    monkeypatch.setenv("PATH", str(bin_dir) + os.pathsep + os.environ["PATH"])
    args_file = tmp_path / "args"
    monkeypatch.setenv("TEST_ARGS_FILE", str(args_file))

    script = LAUNCHER.read_text()
    script = script.replace("/usr/share/sonic", str(sonic_dir))
    script = script.replace("/etc/sonic", str(config_dir))
    script = script.replace("/usr/bin/buffermgrd", str(daemon))

    def run(model="traditional"):
        monkeypatch.setenv("TEST_BUFFER_MODEL", model)
        subprocess.run(["bash"], input=script, text=True, check=True, capture_output=True)
        return args_file.read_text().splitlines()

    return run, sonic_dir, config_dir


@pytest.mark.parametrize("hwsku_table,platform_table", [
    (True, True), (True, False), (False, True), (False, False),
])
def test_lookup_precedence(run_buffermgrd, hwsku_table, platform_table):
    run, sonic_dir, _ = run_buffermgrd
    hwsku_file = sonic_dir / "hwsku" / "pg_profile_lookup.ini"
    platform_file = sonic_dir / "platform" / "pg_profile_lookup.ini"
    if hwsku_table:
        hwsku_file.write_text("HWSKU override\n")
    if platform_table:
        platform_file.write_text("platform default\n")

    expected = platform_file if platform_table and not hwsku_table else hwsku_file
    assert run() == ["-l", str(expected)]
    assert hwsku_file.exists() == hwsku_table
    assert platform_file.exists() == platform_table


@pytest.mark.parametrize("valid_target", [True, False])
def test_lookup_symlink(run_buffermgrd, valid_target):
    run, sonic_dir, _ = run_buffermgrd
    target = sonic_dir / "override.ini"
    if valid_target:
        target.write_text("HWSKU override\n")
    hwsku_file = sonic_dir / "hwsku" / "pg_profile_lookup.ini"
    hwsku_file.symlink_to(target)
    platform_file = sonic_dir / "platform" / "pg_profile_lookup.ini"
    platform_file.write_text("platform default\n")

    assert run() == ["-l", str(hwsku_file if valid_target else platform_file)]
    assert hwsku_file.is_symlink()


@pytest.mark.parametrize("model", ["", "traditional"])
def test_static_lookup_for_existing_buffer_models(run_buffermgrd, model):
    run, sonic_dir, _ = run_buffermgrd
    platform_file = sonic_dir / "platform" / "pg_profile_lookup.ini"
    platform_file.write_text("platform default\n")

    assert run(model) == ["-l", str(platform_file)]


@pytest.mark.parametrize("peripheral,zero_profiles", [
    (False, False), (False, True), (True, False), (True, True),
])
def test_dynamic_buffers_keep_existing_arguments(run_buffermgrd, peripheral, zero_profiles):
    run, sonic_dir, config_dir = run_buffermgrd
    for level in ("hwsku", "platform"):
        (sonic_dir / level / "pg_profile_lookup.ini").write_text("unused static table\n")
    expected = ["-a", str(config_dir / "asic_table.json")]
    if peripheral:
        (config_dir / "peripheral_table.json").write_text("{}")
        expected += ["-p", str(config_dir / "peripheral_table.json")]
    if zero_profiles:
        (config_dir / "zero_profiles.json").write_text("{}")
        expected += ["-z", str(config_dir / "zero_profiles.json")]

    assert run("dynamic") == expected
