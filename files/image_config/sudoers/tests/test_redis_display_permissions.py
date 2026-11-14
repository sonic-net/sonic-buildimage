"""Check fixed Redis-backed display permissions in an opted-in disposable container."""

import json
import os
from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.fixture(scope="module", autouse=True)
def disposable_sudo_policy():
    if (os.environ.get("REDIS_SUDO_INTEGRATION") != "1"
            or os.geteuid() != 0 or not Path("/.dockerenv").is_file()):
        pytest.skip("requires an explicitly selected disposable root Docker container")
    source = Path(__file__).resolve().parents[1] / "sudoers"
    subprocess.run(["/usr/sbin/visudo", "-cf", str(source)], check=True)
    shutil.copyfile(source, "/etc/sudoers")
    os.chmod("/etc/sudoers", 0o440)
    show = Path("/usr/local/bin/show")
    show.write_text(
        "#!/usr/bin/python3 -I\n"
        "import json, os, sys\n"
        "print(json.dumps({'uid':os.geteuid(), 'args':sys.argv[1:], "
        "'pythonpath':os.environ.get('PYTHONPATH')}))\n")
    show.chmod(0o755)
    consutil = Path("/usr/local/bin/consutil")
    consutil.write_text(show.read_text())
    consutil.chmod(0o755)


def ordinary_sudo(arguments, executable="show"):
    return subprocess.run(
        ["/usr/sbin/runuser", "-u", "nobody", "--", "/usr/bin/sudo", "-n",
         "/usr/local/bin/" + executable] + arguments,
        capture_output=True, text=True,
        env=dict(os.environ, PYTHONPATH="/not-a-trusted-python-path"))


@pytest.mark.parametrize("arguments", [
    ["orchagent", "tasks"],
    ["orchagent", "tasks", "--namespace", "asic0"],
    ["orchagent", "tasks", "--namespace", "asic123"],
])
def test_named_read_runs_as_root(arguments):
    result = ordinary_sudo(arguments)
    assert result.returncode == 0, result.stderr
    output = json.loads(result.stdout)
    assert output == {"uid": 0, "args": arguments, "pythonpath": None}


@pytest.mark.parametrize("arguments", [
    ["orchagent", "clear"],
    ["orchagent", "tasks", "clear"],
    ["orchagent", "tasks", "--namespace", "all"],
    ["orchagent", "tasks", "--namespace", "asic0evil"],
    ["orchagent", "tasks", "--namespace", "asic0", "--extra"],
    ["orchagent", "tasks", "--namespace", "asic0-extra"],
    ["orchagent", "tasks", "--unexpected", "value"],
    ["orchagent", "tasks", "--namespace", "invalid-namespace"],
    ["orchagent", "tasks", ";", "id"],
    ["config", "hostname", "changed"],
])
def test_other_operations_and_extra_arguments_are_denied(arguments):
    result = ordinary_sudo(arguments)
    assert result.returncode != 0
    assert not result.stdout


def test_environment_assignment_is_denied():
    result = subprocess.run(
        ["/usr/sbin/runuser", "-u", "nobody", "--", "/usr/bin/sudo", "-n",
         "PYTHONPATH=/not-trusted", "/usr/local/bin/show", "orchagent", "tasks"],
        capture_output=True, text=True)
    assert result.returncode != 0
    assert not result.stdout


CONSOLE_COMMANDS = [["refresh-state", "0"], ["refresh-state", "123"]]
MUX_COMMANDS = [
    ["muxcable", "status"],
    ["muxcable", "status", "--json"],
    ["muxcable", "status", "--", "Ethernet0"],
    ["muxcable", "status", "--json", "--", "Ethernet0"],
    ["muxcable", "hwmode", "muxdirection"],
    ["muxcable", "hwmode", "muxdirection", "--json"],
    ["muxcable", "hwmode", "muxdirection", "--", "Ethernet0"],
    ["muxcable", "hwmode", "muxdirection", "--json", "--", "Ethernet0"],
    ["muxcable", "hwmode", "switchmode"],
    ["muxcable", "hwmode", "switchmode", "--", "Ethernet0"],
    ["muxcable", "firmware", "version", "--", "Ethernet0"],
    ["muxcable", "firmware", "version", "--active", "--", "Ethernet0"],
]
for operation in ["berinfo", "eyeinfo", "fecstatistics", "pcsstatistics"]:
    for target in ["NIC", "TORA", "TORB", "LOCAL"]:
        for options in [[], ["--json"]]:
            MUX_COMMANDS.append(["muxcable", operation] + options + ["--", "Ethernet0", target])
for operation in ["alivecablestatus", "event-log", "get-fec-anlt-speed", "health",
                  "resetcause", "operationtime", "queueinfo"]:
    for options in [[], ["--json"]]:
        MUX_COMMANDS.append(["muxcable", operation] + options + ["--", "Ethernet0"])


@pytest.mark.parametrize("executable,arguments", [
    *[("consutil", args) for args in CONSOLE_COMMANDS],
    *[("show", args) for args in MUX_COMMANDS],
])
def test_console_and_mux_queries_preserve_arguments(executable, arguments):
    result = ordinary_sudo(arguments, executable)
    assert result.returncode == 0, result.stderr
    assert json.loads(result.stdout) == {"uid": 0, "args": arguments, "pythonpath": None}


@pytest.mark.parametrize("executable,arguments", [
    ("consutil", ["refresh-state", "-1"]),
    ("consutil", ["refresh-state", "all"]),
    ("consutil", ["refresh-state", "0", "extra"]),
    ("consutil", ["refresh-state", "invalid-line"]),
    ("consutil", ["connect", "0"]),
    ("consutil", ["refresh-state", "invalid-line-number"]),
    ("show", ["muxcable", "status", "--", "Ethernet0", "extra"]),
    ("show", ["muxcable", "status", "--", "invalid interface"]),
    ("show", ["muxcable", "status", "--", "invalid interface name"]),
    ("show", ["muxcable", "berinfo", "--", "Ethernet0", "INVALID"]),
    ("show", ["muxcable", "berinfo", "--", "Ethernet0"]),
    ("show", ["muxcable", "hwmode", "switchmode", "--json"]),
    ("show", ["muxcable", "firmware", "version", "--json", "--", "Ethernet0"]),
    ("show", ["muxcable", "firmware", "activate", "--", "Ethernet0"]),
    ("show", ["muxcable", "debugdumpregisters", "Ethernet0", "all"]),
    ("show", ["muxcable", "config", "Ethernet0"]),
])
def test_other_console_and_mux_operations_are_denied(executable, arguments):
    result = ordinary_sudo(arguments, executable)
    assert result.returncode != 0
    assert not result.stdout
