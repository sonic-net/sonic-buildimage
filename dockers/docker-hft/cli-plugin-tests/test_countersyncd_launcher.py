"""Tests for the countersyncd launcher and the entrypoint of the hft container."""

import json
import os
import shutil
import subprocess
import textwrap
from pathlib import Path

import pytest


DOCKER_DIR = Path(__file__).resolve().parent.parent
LAUNCHER = DOCKER_DIR / "countersyncd.sh"
DOCKER_INIT = DOCKER_DIR / "docker_init.sh"

# Excerpt of "countersyncd --help" (clap) covering every line layout it uses:
# short and long option, long option only, flags, and wrapped descriptions.
COUNTERSYNCD_HELP = """\
SONiC High Frequency Telemetry Counter Sync Daemon

Usage: countersyncd [OPTIONS]

Options:
  -e, --enable-stats
          Enable stats reporting to console
  -c, --enable-counter-db
          Enable counter database writing
  -f, --counter-db-frequency <COUNTER_DB_FREQUENCY>
          Counter database write frequency in seconds [default: 3]
  -l, --log-level <LOG_LEVEL>
          Set the logging level [default: info]
      --log-format <LOG_FORMAT>
          Set the log output format: 'simple' for level and message only, 'full' for timestamp,
          file, line, level, and message [default: full]
      --netlink-rcvbuf <NETLINK_RCVBUF>
          Netlink SO_RCVBUF size in bytes (0 = default). Use 4MB or higher if you see 'Netlink
          receive buffer full (ENOBUFS)' [default: 4194304]
  -o, --enable-otel
          Enable OpenTelemetry metrics export
      --otel-endpoint <OTEL_ENDPOINT>
          OpenTelemetry collector endpoint URL [default: http://localhost:4317]
      --otel-capacity <OTEL_CAPACITY>
          Set the SAI stats batch channel capacity for otel; must be positive, with no upper clamp
          [default: 1024]
  -h, --help
          Print help
  -V, --version
          Print version
"""

FAKE_COUNTERSYNCD = """\
#!/usr/bin/env python3
import json
import sys

if sys.argv[1:] == ["--help"]:
    sys.stdout.write({help!r})
    sys.exit(0)
print("ARGV=" + json.dumps(sys.argv[1:]))
"""


@pytest.fixture
def hft(tmp_path):
    """Run countersyncd.sh against a fake countersyncd binary."""
    binary = tmp_path / "countersyncd"
    binary.write_text(FAKE_COUNTERSYNCD.format(help=COUNTERSYNCD_HELP))
    binary.chmod(0o755)

    class Runner:
        settings_file = tmp_path / "hft.env"
        container_args_file = tmp_path / "container_args"

        def run(self, env=None, container_args=None):
            if container_args is not None:
                self.container_args_file.write_bytes(
                    b"".join(arg.encode() + b"\0" for arg in container_args))
            full_env = {
                "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                "HFT_COUNTERSYNCD_BIN": str(binary),
                "HFT_CONFIG_FILE": str(self.settings_file),
                "HFT_CONTAINER_ARGS_FILE": str(self.container_args_file),
            }
            full_env.update(env or {})
            result = subprocess.run(["bash", str(LAUNCHER)], env=full_env,
                                    capture_output=True, text=True, check=False)
            self.output = result.stdout
            self.returncode = result.returncode
            argv_lines = [line for line in result.stdout.splitlines() if line.startswith("ARGV=")]
            return json.loads(argv_lines[0][len("ARGV="):]) if argv_lines else None

        def write_settings(self, content):
            self.settings_file.write_text(textwrap.dedent(content))

    return Runner()


def test_no_option_by_default(hft):
    assert hft.run() == []


def test_environment_variables_become_options(hft):
    argv = hft.run(env={
        "COUNTERSYNCD_ENABLE_OTEL": "true",
        "COUNTERSYNCD_OTEL_ENDPOINT": "http://10.0.0.1:4317",
        "COUNTERSYNCD_ENABLE_COUNTER_DB": "True",
        "COUNTERSYNCD_NETLINK_RCVBUF": "0",
        "COUNTERSYNCD_LOG_LEVEL": "",
    })
    assert argv == [
        "--enable-counter-db",
        "--enable-otel",
        "--netlink-rcvbuf=0",
        "--otel-endpoint=http://10.0.0.1:4317",
    ]


def test_flag_can_be_turned_off(hft):
    argv = hft.run(env={
        "COUNTERSYNCD_ENABLE_OTEL": "false",
        "COUNTERSYNCD_ENABLE_COUNTER_DB": "TRUE",
    })
    assert argv == ["--enable-counter-db"]


def test_settings_file_is_overridden_by_environment(hft):
    hft.write_settings("""\
        # comment

        COUNTERSYNCD_OTEL_ENDPOINT=http://file:4317
          export COUNTERSYNCD_LOG_LEVEL = "debug"
        COUNTERSYNCD_LOG_FORMAT='simple'
        COUNTERSYNCD_ENABLE_OTEL=false
        not a setting
        OTHER_VARIABLE=1
        HFT_COUNTERSYNCD_ARGS=--otel-capacity 2048""")
    argv = hft.run(env={"COUNTERSYNCD_OTEL_ENDPOINT": "http://env:4317"})
    assert argv == [
        "--log-format=simple",
        "--log-level=debug",
        "--otel-endpoint=http://env:4317",
        "--otel-capacity",
        "2048",
    ]
    assert "Ignoring line 7 of" in hft.output
    assert "Ignoring line 8 of" in hft.output


def test_values_from_settings_file_are_not_expanded(hft):
    hft.write_settings("COUNTERSYNCD_OTEL_ENDPOINT=$(touch /tmp/never)${HOME}\n")
    assert hft.run() == ["--otel-endpoint=$(touch /tmp/never)${HOME}"]


def test_unknown_variables_are_ignored(hft):
    argv = hft.run(env={
        # Kubernetes service links of a Service named "countersyncd"
        "COUNTERSYNCD_PORT": "tcp://10.0.0.1:4317",
        "COUNTERSYNCD_SERVICE_HOST": "10.0.0.1",
        "COUNTERSYNCD_OTEL_ENDPIONT": "http://typo:4317",
        "COUNTERSYNCD_HELP": "true",
        "COUNTERSYNCD_": "true",
    })
    assert argv == []
    assert "Ignoring COUNTERSYNCD_PORT: countersyncd has no --port option" in hft.output
    assert "Ignoring COUNTERSYNCD_OTEL_ENDPIONT" in hft.output
    assert "Ignoring COUNTERSYNCD_HELP" in hft.output


def test_raw_arguments_take_precedence(hft):
    argv = hft.run(
        env={
            "COUNTERSYNCD_ENABLE_OTEL": "true",
            "COUNTERSYNCD_OTEL_ENDPOINT": "http://env:4317",
            "COUNTERSYNCD_LOG_LEVEL": "warn",
            "COUNTERSYNCD_ENABLE_COUNTER_DB": "true",
            "HFT_COUNTERSYNCD_ARGS": " --log-level=debug\t--comm-stats-interval 60 ",
        },
        container_args=["--enable-otel", "--otel-endpoint", "http://container:4317", "a b"],
    )
    assert argv == [
        "--enable-counter-db",
        "--log-level=debug",
        "--comm-stats-interval",
        "60",
        "--enable-otel",
        "--otel-endpoint",
        "http://container:4317",
        "a b",
    ]


def test_empty_container_arguments(hft):
    assert hft.run(container_args=[]) == []


def test_credentials_are_not_logged(hft):
    argv = hft.run(container_args=[
        "--otel-endpoint=http://user:pass@collector:4317",
        "--otel-auth-token=abc",
        "--otel-header", "Authorization=Bearer abc",
    ])
    assert argv == [
        "--otel-endpoint=http://user:pass@collector:4317",
        "--otel-auth-token=abc",
        "--otel-header", "Authorization=Bearer abc",
    ]
    start_line = next(line for line in hft.output.splitlines() if "Starting" in line)
    assert "pass" not in start_line
    assert "abc" not in start_line
    assert "--otel-endpoint=http://<hidden>@collector:4317" in start_line


def test_missing_settings_file_is_reported_when_explicit(hft):
    hft.run()
    assert "is not readable" in hft.output


@pytest.mark.skipif(os.path.exists("/etc/sonic/hft/hft.env"),
                    reason="the default settings file exists on this host")
def test_missing_default_settings_file_is_silent(hft):
    assert hft.run(env={"HFT_CONFIG_FILE": ""}) == []
    assert "is not readable" not in hft.output


def test_missing_binary(hft, tmp_path):
    assert hft.run(env={"HFT_COUNTERSYNCD_BIN": str(tmp_path / "missing")}) is None
    assert hft.returncode == 1


@pytest.mark.skipif(os.path.exists("/usr/local/bin/supervisord"),
                    reason="docker_init.sh would start supervisord")
@pytest.mark.parametrize("args", [[], ["--otel-endpoint", "http://x:4317", "a b", ""]])
def test_docker_init_saves_container_arguments(tmp_path, args):
    args_file = tmp_path / "run" / "container_args"
    subprocess.run(["bash", str(DOCKER_INIT)] + args,
                   env={"PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                        "HFT_CONTAINER_ARGS_FILE": str(args_file)},
                   capture_output=True, check=False)
    assert args_file.read_bytes() == b"".join(arg.encode() + b"\0" for arg in args)


def test_countersyncd_is_a_critical_process():
    critical = (DOCKER_DIR / "critical_processes").read_text().split()
    assert "program:countersyncd" in critical


@pytest.mark.skipif(shutil.which("bash") is None, reason="bash is required")
def test_scripts_are_valid_bash():
    for script in ("countersyncd.sh", "docker_init.sh", "start.sh", "readiness_probe_hook"):
        subprocess.run(["bash", "-n", str(DOCKER_DIR / script)], check=True)
