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

# Excerpt of "countersyncd --help". clap lists the options in this layout when
# their descriptions are long: an option with a short name, without one, flags,
# options taking a value, and wrapped descriptions.
COUNTERSYNCD_HELP = """\
SONiC High Frequency Telemetry Counter Sync Daemon

Usage: countersyncd [OPTIONS]

Options:
  -e, --enable-stats
          Enable stats reporting to console
  -i, --stats-interval <STATS_INTERVAL>
          Stats reporting interval in seconds [default: 10]
  -c, --enable-counter-db
          Enable counter database writing
  -f, --counter-db-frequency <COUNTER_DB_FREQUENCY>
          Counter database write frequency in seconds [default: 3]
  -l, --log-level <LOG_LEVEL>
          Set the logging level [default: info]
      --log-format <LOG_FORMAT>
          Set the log output format: 'simple' for level and message only, 'full' for timestamp,
          file, line, level, and message [default: full]
      --comm-stats-interval <COMM_STATS_INTERVAL>
          Interval in seconds for logging comm stats (channel lengths). Use a shorter value (e.g.
          60) when verifying HFT processing slowness. Minimum 1 [default: 600]
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

# The layout clap uses when the descriptions are short, with the markers of a
# flag which can be repeated, of an option taking several values and of an
# option whose value is optional.
SINGLE_LINE_HELP = """\
Usage: countersyncd [OPTIONS]

Options:
  -o, --enable-otel                  Enable OpenTelemetry metrics export
  -v, --verbose...                   More logs
  -x, --otel-header <OTEL_HEADER>    Header added to OTLP requests
      --multi <MULTI>...             Several values
      --color [<COLOR>]              Optional value
  -l, --log-level <LOG_LEVEL>        Set the logging level [default: info]
  -h, --help                         Print help
"""

FAKE_COUNTERSYNCD = """\
#!/usr/bin/env python3
import json
import sys

if sys.argv[1:] == ["--help"]:
    with open({help_file!r}) as help_file:
        sys.stdout.write(help_file.read())
    sys.exit(0)
print("ARGV=" + json.dumps(sys.argv[1:]))
"""


def nul_separated(args):
    return b"".join(arg.encode() + b"\0" for arg in args)


@pytest.fixture
def hft(tmp_path):
    """Run countersyncd.sh against a fake countersyncd binary."""
    help_file = tmp_path / "help.txt"
    help_file.write_text(COUNTERSYNCD_HELP)
    binary = tmp_path / "countersyncd"
    binary.write_text(FAKE_COUNTERSYNCD.format(help_file=str(help_file)))
    binary.chmod(0o755)

    class Runner:
        settings_file = tmp_path / "hft.env"
        container_args_file = tmp_path / "container_args"

        def run(self, env=None, container_args=None, unset=()):
            if container_args is not None:
                self.container_args_file.write_bytes(nul_separated(container_args))
            full_env = {
                "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                "HFT_COUNTERSYNCD_BIN": str(binary),
                "HFT_CONFIG_FILE": str(self.settings_file),
                "HFT_CONTAINER_ARGS_FILE": str(self.container_args_file),
            }
            full_env.update(env or {})
            for name in unset:
                full_env.pop(name, None)
            result = subprocess.run(["bash", str(LAUNCHER)], env=full_env,
                                    capture_output=True, text=True, check=False)
            self.output = result.stdout + result.stderr
            self.returncode = result.returncode
            argv_lines = [line for line in result.stdout.splitlines() if line.startswith("ARGV=")]
            return json.loads(argv_lines[0][len("ARGV="):]) if argv_lines else None

        def write_settings(self, content):
            self.settings_file.write_text(textwrap.dedent(content))

        def set_help(self, text):
            help_file.write_text(text)

    return Runner()


def test_no_option_by_default(hft):
    assert hft.run() == []
    assert hft.returncode == 0


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


def test_flag_with_invalid_value_is_ignored(hft):
    assert hft.run(env={"COUNTERSYNCD_ENABLE_OTEL": "yes"}) == []
    assert "Ignoring COUNTERSYNCD_ENABLE_OTEL: --enable-otel is a flag, expected true or false" in hft.output


def test_settings_file_is_overridden_by_environment(hft):
    hft.write_settings("""\
        # comment

        COUNTERSYNCD_OTEL_ENDPOINT=http://file:4317
          export COUNTERSYNCD_LOG_LEVEL = "debug"
        COUNTERSYNCD_LOG_FORMAT='simple'
        COUNTERSYNCD_ENABLE_OTEL=true
        not a setting
        OTHER_VARIABLE=1
        HFT_COUNTERSYNCD_ARGS=--otel-capacity 2048""")
    argv = hft.run(env={
        "COUNTERSYNCD_OTEL_ENDPOINT": "http://env:4317",
        "COUNTERSYNCD_ENABLE_OTEL": "false",
    })
    assert argv == [
        "--log-format=simple",
        "--log-level=debug",
        "--otel-endpoint=http://env:4317",
        "--otel-capacity=2048",
    ]
    assert "Ignoring line 7 of {}: expected <NAME>=<value>".format(hft.settings_file) in hft.output
    assert "Ignoring line 8 of {}: only COUNTERSYNCD_<NAME> and HFT_COUNTERSYNCD_ARGS".format(
        hft.settings_file) in hft.output


@pytest.mark.skipif(os.path.exists("/var/run/hft/container_args"),
                    reason="the default container arguments file exists on this host")
def test_settings_file_cannot_change_the_container_variables(hft, tmp_path):
    other_args_file = tmp_path / "other_args"
    other_args_file.write_bytes(nul_separated(["--log-level=warn"]))
    hft.write_settings("""\
        HFT_CONTAINER_ARGS_FILE={}
        HFT_COUNTERSYNCD_BIN=/bin/false
        HFT_CONFIG_FILE=/nonexistent
        """.format(other_args_file))
    # HFT_CONTAINER_ARGS_FILE is not in the environment, but must not be taken
    # from the settings file either
    assert hft.run(unset=["HFT_CONTAINER_ARGS_FILE"]) == []
    for lineno in (1, 2, 3):
        assert "Ignoring line {} of {}: only COUNTERSYNCD_<NAME> and HFT_COUNTERSYNCD_ARGS".format(
            lineno, hft.settings_file) in hft.output


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
    assert "Ignoring COUNTERSYNCD_: no option name" in hft.output


def test_sources_are_merged_by_option(hft):
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
        "--comm-stats-interval=60",
        "--enable-otel",
        "--otel-endpoint=http://container:4317",
        "a b",
    ]
    assert ("Ignoring COUNTERSYNCD_LOG_LEVEL: overridden by --log-level in HFT_COUNTERSYNCD_ARGS"
            in hft.output)
    assert ("Ignoring COUNTERSYNCD_OTEL_ENDPOINT: overridden by --otel-endpoint in the container arguments"
            in hft.output)


def test_container_arguments_override_hft_countersyncd_args(hft):
    argv = hft.run(env={"HFT_COUNTERSYNCD_ARGS": "--log-level=info --enable-otel"},
                   container_args=["--log-level", "debug"])
    assert argv == ["--enable-otel", "--log-level=debug"]
    assert ("Ignoring --log-level in HFT_COUNTERSYNCD_ARGS: overridden by --log-level in the container arguments"
            in hft.output)


@pytest.mark.parametrize("container_args,expected", [
    # The variables give --enable-otel and --log-level=warn
    (["-o"], ["--log-level=warn", "--enable-otel"]),
    (["-l", "debug"], ["--enable-otel", "--log-level=debug"]),
    (["-ldebug"], ["--enable-otel", "--log-level=debug"]),
    (["-l=debug"], ["--enable-otel", "--log-level=debug"]),
    (["-ol", "debug"], ["--enable-otel", "--log-level=debug"]),
    (["-eo"], ["--log-level=warn", "--enable-stats", "--enable-otel"]),
    (["-cf", "7"], ["--enable-otel", "--log-level=warn", "--enable-counter-db", "--counter-db-frequency=7"]),
])
def test_short_options_override_variables(hft, container_args, expected):
    argv = hft.run(env={"COUNTERSYNCD_ENABLE_OTEL": "true", "COUNTERSYNCD_LOG_LEVEL": "warn"},
                   container_args=container_args)
    assert argv == expected


def test_options_repeated_in_one_source_are_kept(hft):
    # Within a source, arguments are passed as given: options which can be
    # repeated keep all their occurrences, others are rejected by countersyncd.
    argv = hft.run(env={"HFT_COUNTERSYNCD_ARGS": "-l info --log-level=debug"})
    assert argv == ["--log-level=info", "--log-level=debug"]


def test_values_are_kept_whole(hft):
    argv = hft.run(container_args=["--otel-endpoint", "http://a b:4317/x=1", "--log-format=-", "-l", "-"])
    assert argv == ["--otel-endpoint=http://a b:4317/x=1", "--log-format=-", "--log-level=-"]


def test_missing_value_is_left_for_countersyncd(hft):
    assert hft.run(container_args=["--otel-endpoint"]) == ["--otel-endpoint"]
    assert hft.run(container_args=["-l"]) == ["--log-level"]
    # As for countersyncd, an argument which looks like an option is not a value
    argv = hft.run(env={"COUNTERSYNCD_ENABLE_OTEL": "true"},
                   container_args=["--otel-endpoint", "-o", "-l", "--enable-stats"])
    assert argv == ["--otel-endpoint", "--enable-otel", "--log-level", "--enable-stats"]


def test_other_arguments_are_passed_unchanged(hft):
    argv = hft.run(env={"COUNTERSYNCD_ENABLE_OTEL": "true"},
                   container_args=["--no-such-option", "x", "-z", "--enable-otel=yes", "--", "-o", "--log-level"])
    assert argv == ["--enable-otel", "--no-such-option", "x", "-z", "--enable-otel=yes", "--", "-o", "--log-level"]


def test_single_line_help_layout(hft):
    hft.set_help(SINGLE_LINE_HELP)
    argv = hft.run(
        env={
            "COUNTERSYNCD_ENABLE_OTEL": "true",
            "COUNTERSYNCD_VERBOSE": "true",
            "COUNTERSYNCD_OTEL_HEADER": "a=1",
            "COUNTERSYNCD_MULTI": "z",
            "COUNTERSYNCD_COLOR": "never",
            "COUNTERSYNCD_LOG_LEVEL": "warn",
        },
        container_args=["-vv", "-x", "b=2", "-x", "c=3", "--multi", "m1", "m2", "--color", "always", "-l", "debug"],
    )
    assert argv == [
        "--enable-otel",
        "--verbose", "--verbose",
        "--otel-header=b=2", "--otel-header=c=3",
        "--multi", "m1", "m2",
        "--color", "always",
        "--log-level=debug",
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


def run_docker_init(tmp_path, args):
    """Run docker_init.sh, with the exec of supervisord replaced by a dump of the environment."""
    args_file = tmp_path / "run" / "container_args"
    # HFT_CONTAINER_ARGS_FILE is set, but not exported
    script = 'HFT_CONTAINER_ARGS_FILE="{}"; exec() {{ env; }}; source "$0" "$@"'.format(args_file)
    result = subprocess.run(["bash", "-c", script, str(DOCKER_INIT)] + args,
                            env={"PATH": os.environ.get("PATH", "/usr/bin:/bin")},
                            capture_output=True, text=True, check=True)
    return args_file, result.stdout.splitlines()


@pytest.mark.parametrize("args", [[], ["--otel-endpoint", "http://x:4317", "a b", ""]])
def test_docker_init_saves_container_arguments(tmp_path, args):
    args_file, _ = run_docker_init(tmp_path, args)
    assert args_file.read_bytes() == nul_separated(args)


def test_docker_init_exports_the_container_args_file(tmp_path):
    args_file, environment = run_docker_init(tmp_path, ["-o"])
    assert "HFT_CONTAINER_ARGS_FILE={}".format(args_file) in environment


def test_countersyncd_is_a_critical_process():
    critical = (DOCKER_DIR / "critical_processes").read_text().split()
    assert "program:countersyncd" in critical


@pytest.mark.skipif(shutil.which("bash") is None, reason="bash is required")
def test_scripts_are_valid_bash():
    for script in ("countersyncd.sh", "docker_init.sh", "start.sh", "readiness_probe_hook"):
        subprocess.run(["bash", "-n", str(DOCKER_DIR / script)], check=True)
