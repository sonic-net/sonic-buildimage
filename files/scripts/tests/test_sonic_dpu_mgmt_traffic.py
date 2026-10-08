#!/usr/bin/env python3

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "sonic-dpu-mgmt-traffic.sh"
BASH = os.environ.get("F068_BASH", "bash")

IPTABLES_MOCK = """#!/bin/bash
{
    printf 'CALL\\037'
    printf '%s\\037' "$@"
    printf '\\n'
} >> "$IPTABLES_LOG"
[[ " $* " != *" -C "* ]]
"""


def bash_path(path):
    if os.name != "nt":
        return str(path)
    result = subprocess.run(
        [BASH, "-lc", 'cygpath -u "$1"', "test", str(path)],
        text=True, capture_output=True, timeout=10, check=True,
    )
    return result.stdout.strip()


class DpuMgmtTrafficSecurityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="f068-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.log = self.root / "iptables.jsonl"
        iptables = self.bin / "iptables"
        iptables.write_text(IPTABLES_MOCK)
        iptables.chmod(0o755)

        source = SCRIPT.read_text()
        functions = source.split("\nmgmt_iface=eth0\n", 1)[0]
        self.helpers = self.root / "helpers.sh"
        self.helpers.write_text(functions)
        self.env = dict(os.environ, IPTABLES_LOG=bash_path(self.log))

    def run_bash(self, command, *args):
        return subprocess.run(
            [BASH, "-c", 'source "$1"; PATH="$2:$PATH"; shift 2; ' + command,
             "test", bash_path(self.helpers), bash_path(self.bin), *args],
            env=self.env, text=True, capture_output=True, timeout=10,
        )

    def iptables_calls(self):
        if not self.log.exists():
            return []
        return [line.split("\x1f")[1:-1] for line in self.log.read_text().splitlines()]

    def test_script_has_valid_syntax_and_no_eval(self):
        result = subprocess.run([BASH, "-n", bash_path(SCRIPT)], text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotRegex(SCRIPT.read_text(), r"(?m)^\s*eval\b")

    def test_iptables_arguments_are_not_reparsed(self):
        values = (
            "value with spaces",
            "value; touch F068_EXECUTED",
            "$(touch F068_EXECUTED)",
            "`touch F068_EXECUTED`",
        )
        for value in values:
            with self.subTest(value=value):
                self.log.unlink(missing_ok=True)
                result = self.run_bash(
                    'add_rem_valid_iptable enable nat PREROUTING --dport "$1"',
                    value,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(self.iptables_calls(), [
                    ["-t", "nat", "-C", "PREROUTING", "--dport", value],
                    ["-t", "nat", "-A", "PREROUTING", "--dport", value],
                ])
                self.assertFalse((self.root / "F068_EXECUTED").exists())

    def test_controller_callers_preserve_argument_boundaries(self):
        result = self.run_bash(
            """
            fw_change=disable
            mgmt_iface="eth 0"
            midplane_iface="bridge;midplane"
            midplane_gateway="192.0.2.254; touch F068_EXECUTED"
            sel_dpu_names=(dpu0)
            provided_ports=("1234; touch F068_EXECUTED")
            midplane_ip_dict[dpu0]="192.0.2.10; touch F068_EXECUTED"
            ctrl_dpu_ib_forwarding enable
            """
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        calls = self.iptables_calls()
        added = [call for call in calls if "-A" in call]
        self.assertIn("eth 0", added[-1])
        self.assertTrue(any("bridge;midplane" in call for call in added))
        self.assertIn("1234; touch F068_EXECUTED", added[-1])
        self.assertIn("192.0.2.10; touch F068_EXECUTED:22", added[-1])
        self.assertIn("192.0.2.254; touch F068_EXECUTED", added[-2])
        self.assertFalse((self.root / "F068_EXECUTED").exists())

    def test_port_is_decimal_before_arithmetic(self):
        sentinel = self.root / "F068_EXECUTED"
        payload = f"a[$(touch {sentinel})0]"
        result = self.run_bash('normalize_port "$1"', payload)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(sentinel.exists())
        self.assertEqual(self.iptables_calls(), [])

    def test_decimal_ports_are_normalized(self):
        for value, expected in (("1024", "1024"), ("001024", "1024"), ("0000", "0")):
            with self.subTest(value=value):
                result = self.run_bash('normalize_port "$1"', value)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.strip(), expected)


if __name__ == "__main__":
    unittest.main(verbosity=2)
