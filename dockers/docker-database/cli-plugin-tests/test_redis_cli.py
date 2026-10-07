#!/usr/bin/env python3

import os
import pty
import subprocess
import tempfile
import unittest
from pathlib import Path


WRAPPER = Path(__file__).resolve().parents[1] / "base_image_files" / "redis-cli"
CONFIG_PATH = "/var/run/redis/sonic-db/database_config.json"
SOCKET_FILTER = (
    '.INSTANCES.redis.unix_socket_path | '
    'select(type == "string" and length > 0)'
)


class RedisCliWrapperTest(unittest.TestCase):
    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.root = Path(self.tempdir.name)
        self.mock_bin = self.root / "bin"
        self.mock_bin.mkdir()
        self.docker_argv = self.root / "docker.argv"
        self.docker_stdin = self.root / "docker.stdin"
        self.jq_argv = self.root / "jq.argv"

        self._write_executable(
            "docker",
            """#!/bin/bash
printf '%s\\0' "$@" > "$MOCK_DOCKER_ARGV"
cat > "$MOCK_DOCKER_STDIN"
exit "${MOCK_DOCKER_EXIT:-0}"
""",
        )
        self._write_executable(
            "jq",
            """#!/bin/bash
printf '%s\\0' "$@" > "$MOCK_JQ_ARGV"
if [ "${MOCK_JQ_EXIT:-0}" -ne 0 ]; then
    exit "$MOCK_JQ_EXIT"
fi
printf '%s' "${MOCK_JQ_OUTPUT-/var/run/redis/redis.sock}"
""",
        )

        self.env = os.environ.copy()
        self.env.update(
            {
                "PATH": f"{self.mock_bin}{os.pathsep}{self.env['PATH']}",
                "MOCK_DOCKER_ARGV": str(self.docker_argv),
                "MOCK_DOCKER_STDIN": str(self.docker_stdin),
                "MOCK_JQ_ARGV": str(self.jq_argv),
            }
        )

    def tearDown(self):
        self.tempdir.cleanup()

    def _write_executable(self, name, contents):
        path = self.mock_bin / name
        path.write_text(contents)
        path.chmod(0o755)

    @staticmethod
    def _read_nul_argv(path):
        raw = path.read_bytes()
        if not raw:
            return []
        return [arg.decode() for arg in raw.split(b"\0")[:-1]]

    def _run_wrapper(self, args, stdin=b"", tty_stdout=False, env=None):
        command_env = self.env.copy()
        if env:
            command_env.update(env)

        if tty_stdout:
            master_fd, slave_fd = pty.openpty()
            try:
                result = subprocess.run(
                    [str(WRAPPER), *args],
                    input=stdin,
                    stdout=slave_fd,
                    stderr=subprocess.PIPE,
                    env=command_env,
                    check=False,
                )
            finally:
                os.close(slave_fd)
                os.close(master_fd)
            return result

        return subprocess.run(
            [str(WRAPPER), *args],
            input=stdin,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=command_env,
            check=False,
        )

    def _expected_docker_argv(self, args, flags="-i", socket=None):
        expected = ["exec", flags, "database", "redis-cli"]
        if socket is not None:
            expected.extend(["-s", socket])
        expected.extend(args)
        return expected

    def test_default_injects_configured_socket_and_preserves_argv(self):
        socket = "/var/run/redis/socket path.sock"
        args = [
            "SET",
            "key with spaces",
            "",
            "line one\nline two",
            "*",
            "'quoted'",
            "",
        ]

        result = self._run_wrapper(args, env={"MOCK_JQ_OUTPUT": socket})

        self.assertEqual(result.returncode, 0, result.stderr.decode())
        self.assertEqual(
            self._read_nul_argv(self.docker_argv),
            self._expected_docker_argv(args, socket=socket),
        )
        self.assertEqual(
            self._read_nul_argv(self.jq_argv),
            ["-er", SOCKET_FILTER, CONFIG_PATH],
        )

    def test_stdin_is_preserved_byte_for_byte(self):
        stdin = b"first line\nembedded NUL:\x00:end\n"

        result = self._run_wrapper(["--pipe"], stdin=stdin)

        self.assertEqual(result.returncode, 0, result.stderr.decode())
        self.assertEqual(self.docker_stdin.read_bytes(), stdin)

    def test_non_tty_uses_interactive_only_flag(self):
        result = self._run_wrapper(["PING"])

        self.assertEqual(result.returncode, 0, result.stderr.decode())
        self.assertEqual(
            self._read_nul_argv(self.docker_argv),
            self._expected_docker_argv(
                ["PING"], socket="/var/run/redis/redis.sock"
            ),
        )

    def test_tty_stdout_adds_tty_flag(self):
        result = self._run_wrapper(["PING"], tty_stdout=True)

        self.assertEqual(result.returncode, 0, result.stderr.decode())
        self.assertEqual(
            self._read_nul_argv(self.docker_argv),
            self._expected_docker_argv(
                ["PING"], flags="-it", socket="/var/run/redis/redis.sock"
            ),
        )

    def test_missing_config_fails_closed_without_running_docker(self):
        result = self._run_wrapper(["PING"], env={"MOCK_JQ_EXIT": "4"})

        self.assertEqual(result.returncode, 1)
        self.assertIn(CONFIG_PATH.encode(), result.stderr)
        self.assertFalse(self.docker_argv.exists())

    def test_empty_configured_socket_fails_closed_without_running_docker(self):
        result = self._run_wrapper(["PING"], env={"MOCK_JQ_OUTPUT": ""})

        self.assertEqual(result.returncode, 1)
        self.assertIn(b"Configured Redis Unix socket is empty", result.stderr)
        self.assertFalse(self.docker_argv.exists())

    def test_docker_exit_status_is_preserved(self):
        result = self._run_wrapper(["PING"], env={"MOCK_DOCKER_EXIT": "23"})

        self.assertEqual(result.returncode, 23)

    def test_help_and_version_do_not_require_config(self):
        for option in ("--help", "--version"):
            with self.subTest(option=option):
                self.docker_argv.unlink(missing_ok=True)
                self.jq_argv.unlink(missing_ok=True)

                result = self._run_wrapper(
                    [option], env={"MOCK_JQ_EXIT": "99"}
                )

                self.assertEqual(result.returncode, 0, result.stderr.decode())
                self.assertEqual(
                    self._read_nul_argv(self.docker_argv),
                    self._expected_docker_argv([option]),
                )
                self.assertFalse(self.jq_argv.exists())

    def test_option_like_command_values_still_use_socket(self):
        for value in ("-h", "--tls", "--cluster", "--version"):
            with self.subTest(value=value):
                self.docker_argv.unlink(missing_ok=True)
                self.jq_argv.unlink(missing_ok=True)
                args = ["SET", "key", value]

                result = self._run_wrapper(args)

                self.assertEqual(result.returncode, 0, result.stderr.decode())
                self.assertEqual(
                    self._read_nul_argv(self.docker_argv),
                    self._expected_docker_argv(
                        args, socket="/var/run/redis/redis.sock"
                    ),
                )
                self.assertTrue(self.jq_argv.exists())

    def test_option_value_resembling_endpoint_flag_still_uses_socket(self):
        args = ["-a", "-h", "PING"]

        result = self._run_wrapper(args)

        self.assertEqual(result.returncode, 0, result.stderr.decode())
        self.assertEqual(
            self._read_nul_argv(self.docker_argv),
            self._expected_docker_argv(
                args, socket="/var/run/redis/redis.sock"
            ),
        )

    def test_explicit_endpoint_after_other_options_preserves_tcp(self):
        prefixes = (
            ["-n", "4"],
            ["-t", "1.5"],
            ["-X", "TAG"],
            ["--count", "10"],
            ["--keystats-samples", "5"],
            ["--cursor", "0"],
            ["--top", "20"],
            ["-2"],
            ["-e"],
            ["--mono"],
            ["--keystats"],
        )
        for prefix in prefixes:
            with self.subTest(prefix=prefix):
                self.docker_argv.unlink(missing_ok=True)
                self.jq_argv.unlink(missing_ok=True)
                args = [*prefix, "--raw", "-h", "redis.example", "PING"]

                result = self._run_wrapper(
                    args, env={"MOCK_JQ_EXIT": "99"}
                )

                self.assertEqual(result.returncode, 0, result.stderr.decode())
                self.assertEqual(
                    self._read_nul_argv(self.docker_argv),
                    self._expected_docker_argv(args),
                )
                self.assertFalse(self.jq_argv.exists())

    def test_conservative_opt_outs_are_exact_pass_through(self):
        cases = {
            "host": ["-h", "redis.example", "PING"],
            "port": ["-p", "6380", "PING"],
            "socket": ["-s", "/tmp/explicit.sock", "PING"],
            "uri": ["-u", "redis://redis.example:6380/4", "PING"],
            "ipv4": ["-4", "PING"],
            "ipv6": ["-6", "PING"],
            "tls": ["--tls", "PING"],
            "sni": ["--sni", "redis.example", "PING"],
            "cacert": ["--cacert", "/tmp/ca.pem", "PING"],
            "cacertdir": ["--cacertdir", "/tmp/certs", "PING"],
            "insecure": ["--insecure", "PING"],
            "cert": ["--cert", "/tmp/client.pem", "PING"],
            "key": ["--key", "/tmp/client.key", "PING"],
            "tls_ciphers": ["--tls-ciphers", "cipher-list", "PING"],
            "tls_ciphersuites": [
                "--tls-ciphersuites",
                "ciphersuite-list",
                "PING",
            ],
            "cluster_follow": ["-c", "PING"],
            "cluster_manager": [
                "--cluster",
                "info",
                "redis.example:6380",
            ],
            "replica": ["--replica"],
            "help": ["--help"],
            "version": ["--version"],
            "intrinsic_latency": ["--intrinsic-latency", "1"],
        }

        for name, args in cases.items():
            with self.subTest(name=name):
                self.docker_argv.unlink(missing_ok=True)
                self.jq_argv.unlink(missing_ok=True)

                result = self._run_wrapper(
                    args, env={"MOCK_JQ_EXIT": "99"}
                )

                self.assertEqual(result.returncode, 0, result.stderr.decode())
                self.assertEqual(
                    self._read_nul_argv(self.docker_argv),
                    self._expected_docker_argv(args),
                )
                self.assertFalse(self.jq_argv.exists())


if __name__ == "__main__":
    unittest.main()
