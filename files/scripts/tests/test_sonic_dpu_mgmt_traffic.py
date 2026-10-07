#!/usr/bin/env python3
"""Run with sudo unshare --net; never change the caller's network namespace."""

import json
import os
from pathlib import Path
import shutil
import shlex
import subprocess
import tempfile
import time
import unittest


SCRIPT = Path(os.environ.get(
    "F068_SCRIPT", str(Path(__file__).resolve().parents[1] / "sonic-dpu-mgmt-traffic.sh")
))
BASELINE = os.environ.get("F068_BASELINE")
INTEGRATION = os.environ.get("F068_INTEGRATION") == "1"
IPTABLES = os.environ.get("F068_IPTABLES", "iptables")
IPTABLES_SAVE = os.environ.get("F068_IPTABLES_SAVE", "iptables-save")
REDIS_BIN = os.environ.get("F068_REDIS_BIN")

MOCK = r"""#!/usr/bin/python3
import json, os, sys
from pathlib import Path
tool = Path(sys.argv[0]).name
args = sys.argv[1:]
config = json.loads(Path(os.environ["F068_CONFIG"]).read_text())
with open(os.environ["F068_LOG"], "a") as stream:
    stream.write(json.dumps([tool] + args) + "\n")
if tool == "ifconfig":
    print(config.get("interface", "bridge-midplane: flags\n    inet 192.0.2.254 netmask 255.255.255.0"))
    sys.exit(config.get("interface_status", 0))
if tool == "netstat":
    print(config.get("listeners", "Proto Recv-Q Send-Q Local Address Foreign Address State"))
    sys.exit(config.get("netstat_status", 0))
if tool == "redis-cli":
    if config.get("redis_status"):
        sys.exit(config["redis_status"])
    dpus = config.get("dpus", {"dpu0": ["dpu0", "192.0.2.10"], "dpu1": ["dpu1", "192.0.2.11"]})
    if "keys" in args:
        print(config.get("keys", "\n".join("DPUS|" + dpu for dpu in reversed(dpus))))
    elif "hget" in args:
        index = args.index("hget")
        key, field = args[index + 1:index + 3]
        if key.startswith("DPUS|"):
            print(dpus.get(key[5:], ["", ""])[0])
        else:
            interface = key.split("|")[-1]
            print(next((entry[1] for entry in dpus.values() if entry[0] == interface), ""))
    sys.exit(0)
if tool == "iptables":
    state_path = Path(os.environ["F068_STATE"])
    state = json.loads(state_path.read_text())
    action = args[2]
    rule = args[:2] + args[3:]
    if action == "-C":
        if config.get("check_status"):
            print(config.get("check_message", "simulated check failure"), file=sys.stderr)
            sys.exit(config["check_status"])
        sys.exit(0 if rule in state else 1)
    if action == "-S":
        if config.get("chain_status"):
            print("simulated chain query failure", file=sys.stderr)
            sys.exit(config["chain_status"])
        sys.exit(0)
    mutations = sum(1 for line in Path(os.environ["F068_LOG"]).read_text().splitlines()
                    if json.loads(line)[0] == "iptables" and json.loads(line)[3] in ("-A", "-D"))
    if config.get("mutation_status") and mutations >= config.get("fail_at", 1):
        print("simulated mutation failure", file=sys.stderr)
        sys.exit(config["mutation_status"])
    if action == "-A":
        state.append(rule)
    elif action == "-D":
        state.remove(rule)
    state_path.write_text(json.dumps(state))
    sys.exit(0)
sys.exit("Unexpected mock command")
"""


def isolated():
    return os.geteuid() == 0 and (
        os.readlink("/proc/self/ns/net") != os.readlink("/proc/1/ns/net")
    )


def start_redis(root, env, processes):
    socket = root / "redis.sock"
    process = subprocess.Popen(
        [str(Path(REDIS_BIN) / "redis-server"), "--port", "0", "--unixsocket", str(socket),
         "--save", "", "--appendonly", "no", "--dir", str(root), "--logfile", str(root / "redis.log")],
        env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
    )
    processes.append(process)

    def client(*args, check=True):
        return subprocess.run([str(Path(REDIS_BIN) / "redis-cli"), "-s", str(socket), *args],
                              env=env, text=True, capture_output=True, timeout=5, check=check)

    for _ in range(100):
        if process.poll() is not None:
            raise RuntimeError("Isolated Redis failed to start: " + process.stderr.read().decode())
        response = client("PING", check=False)
        if response.returncode == 0 and response.stdout.strip() == "PONG":
            return client, (
                "#!/bin/bash\nexec " + shlex.quote(str(Path(REDIS_BIN) / "redis-cli"))
                + " -s " + shlex.quote(str(socket)) + ' "$@"\n'
            )
        time.sleep(0.02)
    raise RuntimeError("Isolated Redis did not become responsive")


def stop_processes(processes):
    for process in reversed(processes):
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        for stream in (process.stdout, process.stderr):
            if stream is not None:
                stream.close()


class MockTests(unittest.TestCase):
    def setUp(self):
        if not isolated():
            self.fail("Run tests as root in a private network namespace (sudo unshare --net).")
        self.temp = tempfile.TemporaryDirectory(prefix="f068-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.root.chmod(0o755)
        self.script = self.root / "product.sh"
        shutil.copyfile(SCRIPT, self.script)
        self.script.chmod(0o755)
        self.config = {}
        self.config_path = self.root / "config.json"
        self.log = self.root / "calls.jsonl"
        self.state = self.root / "state.json"
        self.state.write_text("[]")
        self.bin = self.root / "bin"
        self.bin.mkdir()
        for tool in ("iptables", "redis-cli", "ifconfig", "netstat"):
            path = self.bin / tool
            path.write_text(MOCK)
            path.chmod(0o755)
        self.env = dict(os.environ, PATH=f"{self.bin}:/usr/sbin:/usr/bin:/sbin:/bin",
                        F068_CONFIG=str(self.config_path), F068_LOG=str(self.log),
                        F068_STATE=str(self.state))

    def run_script(self, *args, script=None, user=None):
        self.config_path.write_text(json.dumps(self.config))
        return subprocess.run(["bash", str(script or self.script), *args], env=self.env,
                              text=True, capture_output=True, timeout=15, user=user,
                              group=user, extra_groups=[] if user is not None else None)

    def inbound(self, *extra, operation="-e", dpus="dpu0", ports="1234"):
        return self.run_script("inbound", operation, "--dpus", dpus, "--ports", ports,
                               "--nofwctrl", *extra)

    def calls(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def firewall(self):
        return [call[1:] for call in self.calls() if call[0] == "iptables"]

    def reset(self):
        self.log.unlink(missing_ok=True)
        self.state.write_text("[]")

    def assert_rejected(self, result):
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("Error:", result.stderr)
        self.assertEqual(self.firewall(), [], "Invalid input caused a firewall operation")

    def helper(self, *args, prefix=""):
        code = self.script.read_text()
        if '\nmgmt_iface=eth0\n' in code and 'main(){' not in code:
            code = code.split('\nmgmt_iface=eth0\n')[0]
        helpers = self.root / "helpers.sh"
        helpers.write_text(code)
        self.config_path.write_text(json.dumps(self.config))
        return subprocess.run(
            ["bash", "-c", 'source "$1"; shift; ' + prefix + 'add_rem_valid_iptable "$@"',
             "test", str(helpers), *args], env=self.env, text=True,
            capture_output=True, timeout=15,
        )

    def test_syntax_and_no_eval(self):
        result = subprocess.run(["bash", "-n", str(self.script)], capture_output=True)
        self.assertEqual(result.returncode, 0)
        self.assertNotRegex(self.script.read_text(), r"(?m)^\s*eval\b")

    def test_helper_idempotence_and_exact_arguments(self):
        rule = ("nat", "PREROUTING", "-p", "tcp", "--dport", "1234",
                "-j", "DNAT", "--to-destination", "192.0.2.10:22")
        for operation, expected in (("enable", ["-C", "-A"]), ("enable", ["-C"]),
                                    ("disable", ["-C", "-D"]), ("disable", ["-C"])):
            self.log.unlink(missing_ok=True)
            result = self.helper(operation, *rule)
            self.assertEqual(result.returncode, 0, result.stderr)
            calls = self.firewall()
            self.assertEqual([args[2] for args in calls], expected)
            for args in calls:
                self.assertEqual(args[:2] + args[3:], ["-t", *rule])

    def test_helper_keeps_shell_syntax_as_one_argument(self):
        for option in ("--dport", "-d", "--to-destination", "--to-source", "-i", "-o"):
            for value in ("value with spaces", "value; printf F068_EXECUTED",
                          "$(printf F068_EXECUTED)", "`printf F068_EXECUTED`", "*"):
                with self.subTest(option=option, value=value):
                    self.reset()
                    result = self.helper("enable", "nat", "PREROUTING", option, value)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertNotIn("F068_EXECUTED", result.stdout)
                    self.assertEqual(self.firewall()[-1][-2:], [option, value])

    def test_helper_invalid_operation_and_chain(self):
        for args in (("bogus", "nat", "PREROUTING"), ("enable", "bad", "FORWARD"),
                     ("enable", "filter", "PREROUTING")):
            self.reset()
            self.assert_rejected(self.helper(*args))

    def test_valid_inbound_rules(self):
        result = self.inbound()
        self.assertEqual(result.returncode, 0, result.stderr)
        rules = json.loads(self.state.read_text())
        self.assertEqual(rules, [
            ["-t", "filter", "FORWARD", "-i", "bridge-midplane", "-o", "eth0", "-p", "tcp",
             "-m", "conntrack", "--ctstate", "RELATED,ESTABLISHED", "-j", "ACCEPT"],
            ["-t", "filter", "FORWARD", "-i", "eth0", "-o", "bridge-midplane", "-p", "tcp",
             "--dport", "22", "-j", "ACCEPT"],
            ["-t", "nat", "POSTROUTING", "-p", "tcp", "-d", "192.0.2.10", "--dport", "22",
             "-j", "SNAT", "--to-source", "192.0.2.254"],
            ["-t", "nat", "PREROUTING", "-i", "eth0", "-p", "tcp", "--dport", "1234",
             "-j", "DNAT", "--to-destination", "192.0.2.10:22"],
        ])

    def test_all_dpus_sorted_and_explicit_order_preserved(self):
        for dpus, expected in (("all", ["192.0.2.10:22", "192.0.2.11:22"]),
                               ("dpu1,dpu0", ["192.0.2.11:22", "192.0.2.10:22"])):
            self.reset()
            result = self.inbound(dpus=dpus, ports="9090,9091")
            self.assertEqual(result.returncode, 0, result.stderr)
            targets = [args[-1] for args in self.firewall() if args[2] == "-A" and "-j" in args
                       and "DNAT" in args]
            self.assertEqual(targets, expected)

    def test_controllers_idempotent_both_directions(self):
        for direction in ("inbound", "outbound"):
            self.reset()
            args = ["--dpus", "dpu0", "--ports", "1234"] if direction == "inbound" else []
            size = 4 if direction == "inbound" else 3
            for operation, expected in (("-e", size), ("-e", size), ("-d", 0), ("-d", 0)):
                result = self.run_script(direction, operation, *args, "--nofwctrl")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(len(json.loads(self.state.read_text())), expected)

    def test_decimal_port_boundaries(self):
        for value, expected in (("1024", "1024"), ("65535", "65535"),
                                ("01024", "1024"), ("02000", "2000"), ("000001234", "1234"),
                                ("0" * 100 + "1234", "1234")):
            self.reset()
            result = self.inbound(ports=value)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn(expected, self.firewall()[-1])

    def test_bad_ports_have_no_effect(self):
        for value in ("0", "1023", "-1", "65536", "9" * 100, "", "12x", "1234+1", "1e4",
                      "0x500", "2#10000", "1234:2345", "1234; printf F068_EXECUTED",
                      "1234$(printf F068_EXECUTED)", "1234`printf F068_EXECUTED`",
                      " 1234", "1234 ", "1234\n", ",1234", "1234,", "1234,,2345"):
            with self.subTest(value=value):
                self.reset()
                self.assert_rejected(self.inbound(ports=value))

    def test_occupied_port_and_netstat_failure(self):
        for config in ({"listeners": "tcp 0 0 0.0.0.0:1234 0.0.0.0:* LISTEN"},
                       {"netstat_status": 1}):
            self.config = config
            for operation in ("-e", "-d"):
                self.reset()
                self.assert_rejected(self.inbound(operation=operation))

    def test_occupancy_does_not_match_port_prefix(self):
        self.config = {"listeners": "tcp 0 0 0.0.0.0:12345 0.0.0.0:* LISTEN"}
        self.assertEqual(self.inbound().returncode, 0)

    def test_bad_dpu_lists(self):
        for dpus, ports in (("", "1234"), ("unknown", "1234"), ("dpu0,", "1234"),
                            (",dpu0", "1234"), ("dpu0,,dpu1", "1234,2345"),
                            ("dpu0,dpu1", "1234"), ("dpu0;printf F068_EXECUTED", "1234")):
            self.reset()
            self.assert_rejected(self.inbound(dpus=dpus, ports=ports))

    def test_bad_database_address(self):
        for value in ("", "256.0.0.1", "192.0.2", "192.0.2.10/24", "::1", "192.00.2.10",
                      "192.0.2.10,192.0.2.11", "192.0.2.10\n192.0.2.11", "192.0.2.10 ",
                      "192.0.2.10; printf F068_EXECUTED", "$(printf F068_EXECUTED)",
                      "192.0.2.10 --jump ACCEPT", "ERR simulated failure"):
            self.reset()
            self.config = {"dpus": {"dpu0": ["dpu0", value]}}
            self.assert_rejected(self.inbound())

    def test_ipv4_octet_boundaries(self):
        for address in ("0.0.0.0", "255.255.255.255", "1.2.3.4", "192.0.2.10"):
            self.reset()
            self.config = {"dpus": {"dpu0": ["dpu0", address]}}
            result = self.inbound()
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(self.firewall()[-1][-1], address + ":22")

    def test_later_dpu_validation_precedes_all_rules(self):
        self.config = {"dpus": {"dpu0": ["dpu0", "192.0.2.10"], "dpu1": ["dpu1", "invalid"]}}
        self.assert_rejected(self.inbound(dpus="all", ports="1234,2345"))

    def test_database_failures_and_bad_interfaces(self):
        for config in ({"redis_status": 1}, {"keys": ""}, {"keys": "ERR simulated failure"},
                       {"dpus": {"dpu0": ["", "192.0.2.10"]}},
                       {"dpus": {"dpu0": ["x" * 16, "192.0.2.10"]}},
                       {"dpus": {"dpu0": ["dpu0;printf", "192.0.2.10"]}}):
            self.reset()
            self.config = config
            self.assert_rejected(self.inbound())

    def test_literal_database_query(self):
        self.assertEqual(self.inbound().returncode, 0)
        queries = [args for args in self.calls() if args[0] == "redis-cli"]
        self.assertEqual(queries[0][-2:], ["keys", "DPUS|*"])
        self.assertTrue(all(args[1:4] == ["--raw", "-n", "4"] for args in queries))

    def test_interface_and_gateway_failures(self):
        for config in ({"interface_status": 1}, {"interface": "no address"},
                       {"interface": " inet 192.0.2.254\n inet 192.0.2.253"},
                       {"interface": " inet 192.0.2.254;printf"}):
            self.reset()
            self.config = config
            self.assert_rejected(self.inbound())

    def test_legacy_ifconfig_address_format(self):
        self.config = {"interface": " inet addr:192.0.2.254 Bcast:192.0.2.255"}
        self.assertEqual(self.inbound().returncode, 0)

    def test_argument_errors(self):
        for args in ((), ("invalid", "-e"), ("outbound",), ("outbound", "-e", "-d"),
                     ("inbound", "-e", "--dpus"), ("inbound", "-e", "--ports"),
                     ("outbound", "-e", "--dpus", "dpu0"), ("outbound", "-e", "--mgmt-iface", "eth0"),
                     ("outbound", "-e", "--", "unexpected")):
            self.reset()
            self.assert_rejected(self.run_script(*args))

    def test_root_guard(self):
        result = self.run_script("outbound", "-e", "--nofwctrl", user=65534)
        self.assert_rejected(result)
        self.assertEqual(self.calls(), [])

    def test_query_and_mutation_errors_propagate(self):
        for direction in ("inbound", "outbound"):
            for config in ({"check_status": 1}, {"check_status": 2}, {"check_status": 3}, {"check_status": 4},
                           {"check_status": 127}, {"mutation_status": 2}):
                self.reset()
                self.config = config
                args = ["--dpus", "dpu0", "--ports", "1234"] if direction == "inbound" else []
                result = self.run_script(direction, "-e", *args, "--nofwctrl")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Error:", result.stderr)
                self.assertNotIn("Enabled DPU management", result.stdout)
                self.assertEqual(len(self.firewall()), 2 if "mutation_status" in config else 1)

    def test_unexpected_status_one_is_not_a_disable_noop(self):
        self.config = {"check_status": 1}
        result = self.inbound(operation="-d")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Cannot check", result.stderr)
        self.assertEqual(len(self.firewall()), 1)
        self.assertNotIn("Disabled DPU management", result.stdout)

    def test_legacy_ambiguous_check_requires_valid_chain(self):
        for operation in ("enable", "disable"):
            for status in (0, 1, 3):
                self.reset()
                self.config = {"check_status": 1, "chain_status": status,
                               "check_message": "iptables: No chain/target/match by that name."}
                result = self.helper(operation, "nat", "PREROUTING", "-p", "tcp")
                expected = ["-C", "-S"]
                if status == 0 and operation == "enable":
                    expected.append("-A")
                self.assertEqual([args[2] for args in self.firewall()], expected)
                self.assertEqual(result.returncode, status)
                if status:
                    self.assertIn("Cannot check", result.stderr)

    def test_failure_at_every_rule_stops_enable_and_disable(self):
        for direction, count in (("inbound", 4), ("outbound", 3)):
            args = ["--dpus", "dpu0", "--ports", "1234"] if direction == "inbound" else []
            for operation in ("-e", "-d"):
                for index in range(1, count + 1):
                    with self.subTest(direction=direction, operation=operation, rule=index):
                        self.reset()
                        self.config = {}
                        if operation == "-d":
                            self.assertEqual(self.run_script(
                                direction, "-e", *args, "--nofwctrl").returncode, 0)
                            self.log.unlink()
                        self.config = {"mutation_status": 2, "fail_at": index}
                        result = self.run_script(direction, operation, *args, "--nofwctrl")
                        self.assertNotEqual(result.returncode, 0)
                        self.assertIn("Error:", result.stderr)
                        self.assertEqual(len(self.firewall()), index * 2)
                        state = json.loads(self.state.read_text())
                        self.assertEqual(len(state), index - 1 if operation == "-e" else count - index + 1)

    def test_long_options_and_terminator(self):
        self.assertEqual(self.run_script("inbound", "--enable", "--dpus", "dpu0",
                                        "--ports", "1234", "--nofwctrl", "--").returncode, 0)
        self.assertEqual(self.run_script("inbound", "--disable", "--dpus", "dpu0",
                                        "--ports", "1234", "--nofwctrl").returncode, 0)
        self.assertEqual(json.loads(self.state.read_text()), [])

    def test_sourcing_has_no_side_effects(self):
        self.config_path.write_text("{}")
        result = subprocess.run(["bash", "-c", 'source "$1"', "test", str(self.script)],
                                env=self.env, text=True, capture_output=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.calls(), [])
        self.assertEqual(result.stdout, "")

    def test_invalid_input_leaves_forwarding_unchanged(self):
        path = Path("/proc/sys/net/ipv4/ip_forward")
        for value in ("0", "1"):
            path.write_text(value + "\n")
            self.reset()
            result = self.run_script("inbound", "-d", "--dpus", "dpu0", "--ports", "invalid")
            self.assert_rejected(result)
            self.assertEqual(path.read_text().strip(), value)

    def test_forwarding_write_failure_propagates(self):
        result = self.helper(
            "enable", "nat", "PREROUTING", "-p", "tcp",
            prefix='mgmt_iface=f068-missing; fw_change=enable; control_forwarding enable || exit $?; ',
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Cannot update", result.stderr)
        self.assertEqual(self.firewall(), [])

    @unittest.skipUnless(BASELINE, "Set F068_BASELINE for original-rule equivalence.")
    def test_valid_rules_match_original_for_all_paths(self):
        for direction in ("inbound", "outbound"):
            for operation in ("-e", "-d"):
                args = ["--dpus", "all", "--ports", "1234,2345"] if direction == "inbound" else []
                self.reset()
                if operation == "-d":
                    self.run_script(direction, "-e", *args, "--nofwctrl")
                initial = self.state.read_text()
                self.log.unlink(missing_ok=True)
                fixed = self.run_script(direction, operation, *args, "--nofwctrl")
                expected = self.firewall()
                self.state.write_text(initial)
                self.log.unlink(missing_ok=True)
                old = self.run_script(direction, operation, *args, "--nofwctrl", script=BASELINE)
                self.assertEqual(fixed.returncode, 0, fixed.stderr)
                self.assertEqual(old.returncode, 0, old.stderr)
                self.assertEqual(self.firewall(), expected)


@unittest.skipUnless(REDIS_BIN, "Set F068_REDIS_BIN to test a real isolated Redis server.")
class RedisTests(unittest.TestCase):
    def setUp(self):
        self.assertTrue(isolated(), "Redis tests require a private network namespace.")
        self.temp = tempfile.TemporaryDirectory(prefix="f068-redis-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.processes = []
        self.addCleanup(stop_processes, self.processes)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.log = self.root / "calls.jsonl"
        self.state = self.root / "state.json"
        self.state.write_text("[]")
        config = self.root / "config.json"
        config.write_text("{}")
        self.env = dict(os.environ, PATH=f"{self.bin}:/usr/sbin:/usr/bin:/sbin:/bin",
                        F068_CONFIG=str(config), F068_LOG=str(self.log), F068_STATE=str(self.state))
        self.client, wrapper = start_redis(self.root, self.env, self.processes)
        for command in ("iptables", "ifconfig", "netstat", "redis-cli"):
            path = self.bin / command
            path.write_text(wrapper if command == "redis-cli" else MOCK)
            path.chmod(0o755)
        self.client("-n", "4", "HSET", "DPUS|dpu0", "midplane_interface", "dpu0")
        self.set_address("192.0.2.10")

    def set_address(self, address):
        self.client("-n", "4", "HSET", "DHCP_SERVER_IPV4_PORT|bridge-midplane|dpu0", "ips@", address)

    def run_script(self, operation="-e"):
        return subprocess.run(
            ["bash", str(SCRIPT), "inbound", operation, "--dpus", "dpu0", "--ports", "001234",
             "--nofwctrl"], env=self.env, text=True, capture_output=True, timeout=15,
        )

    def assert_rejected(self, result):
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Error:", result.stderr)
        self.assertEqual(json.loads(self.state.read_text()), [])
        calls = self.log.read_text().splitlines() if self.log.exists() else []
        self.assertFalse(any(json.loads(line)[0] == "iptables" for line in calls))

    def test_real_scalar_reads_enable_and_disable(self):
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(self.state.read_text())[-1][-1], "192.0.2.10:22")
        self.assertEqual(self.run_script("-d").returncode, 0)
        self.assertEqual(json.loads(self.state.read_text()), [])

    def test_real_multivalue_and_literal_text_rejected(self):
        for address in ("192.0.2.10,192.0.2.11", "192.0.2.10; printf F068_EXECUTED",
                        "$(printf F068_EXECUTED)", ""):
            self.log.unlink(missing_ok=True)
            self.set_address(address)
            self.assert_rejected(self.run_script())

    def test_real_redis_error_returns_failure(self):
        self.client("-n", "4", "DEL", "DPUS|dpu0")
        self.client("-n", "4", "SET", "DPUS|dpu0", "not-a-hash")
        result = self.run_script()
        self.assert_rejected(result)
        self.assertIn("Invalid or missing midplane interface", result.stderr)
        response = self.client("--raw", "-n", "4", "HGET", "DPUS|dpu0",
                               "midplane_interface", check=False)
        self.assertEqual(response.returncode, 0)
        self.assertIn("WRONGTYPE", response.stdout)
        self.client("-n", "4", "DEL", "DPUS|dpu0")
        self.client("-n", "4", "HSET", "DPUS|dpu0", "midplane_interface", "dpu0")
        self.client("-n", "4", "DEL", "DHCP_SERVER_IPV4_PORT|bridge-midplane|dpu0")
        self.client("-n", "4", "SET", "DHCP_SERVER_IPV4_PORT|bridge-midplane|dpu0", "not-a-hash")
        result = self.run_script()
        self.assert_rejected(result)
        self.assertIn("requires exactly one canonical IPv4", result.stderr)

    def test_real_redis_acl_error_rejected(self):
        self.client("ACL", "SETUSER", "default", "-keys")
        result = self.run_script()
        self.assert_rejected(result)
        self.assertIn("Invalid DPU key in CONFIG_DB", result.stderr)

    def test_real_redis_connection_failure(self):
        stop_processes(self.processes)
        self.processes.clear()
        result = self.run_script()
        self.assert_rejected(result)
        self.assertIn("Cannot read CONFIG_DB keys", result.stderr)


@unittest.skipUnless(INTEGRATION, "Set F068_INTEGRATION=1 for real isolated forwarding.")
class IntegrationTests(unittest.TestCase):
    def test_real_nat_forwarding_and_idempotence(self):
        self.assertTrue(isolated(), "Real firewall tests require a private network namespace.")
        with tempfile.TemporaryDirectory(prefix="f068-integration-") as directory:
            root = Path(directory)
            processes = []
            devices = []
            try:
                def run(*args, check=True, **kwargs):
                    args = list(args)
                    if args[0] == "iptables":
                        args[0] = IPTABLES
                    elif args[0] == "iptables-save":
                        args[0] = IPTABLES_SAVE
                    return subprocess.run(args, text=True, capture_output=True, timeout=15,
                                          check=check, **kwargs)

                def peer():
                    process = subprocess.Popen(
                        ["unshare", "--net", "sleep", "300"], stdout=subprocess.DEVNULL,
                        stderr=subprocess.DEVNULL,
                    )
                    processes.append(process)
                    for _ in range(100):
                        if (process.poll() is None and os.readlink(f"/proc/{process.pid}/ns/net")
                                != os.readlink("/proc/self/ns/net")):
                            return process.pid
                        time.sleep(0.02)
                    self.fail("Peer namespace did not start")

                client, dpu = peer(), peer()

                def ns(pid, *args, **kwargs):
                    return run("nsenter", "-t", str(pid), "-n", *args, **kwargs)

                run("ip", "link", "set", "lo", "up")
                for name, pid, host_ip, peer_ip in (
                    ("eth0", client, "198.51.100.1/24", "198.51.100.2/24"),
                    ("bridge-midplane", dpu, "192.0.2.254/24", "192.0.2.10/24"),
                ):
                    run("ip", "link", "add", name, "type", "veth", "peer", "name", "peer0")
                    devices.append(name)
                    run("ip", "link", "set", "peer0", "netns", str(pid))
                    run("ip", "addr", "add", host_ip, "dev", name)
                    run("ip", "link", "set", name, "up")
                    ns(pid, "ip", "addr", "add", peer_ip, "dev", "peer0")
                    ns(pid, "ip", "link", "set", "peer0", "up")
                    ns(pid, "ip", "link", "set", "lo", "up")
                run("iptables", "-P", "FORWARD", "DROP")
                Path("/proc/sys/net/ipv4/ip_forward").write_text("0\n")
                server_code = (
                    "import socketserver; "
                    "exec('class H(socketserver.BaseRequestHandler):\\n"
                    " def handle(self):\\n"
                    "  self.request.sendall((self.client_address[0]+\"|\""
                    "+self.request.recv(64).decode()).encode())\\n'); "
                    "s=socketserver.ThreadingTCPServer(('0.0.0.0',int(__import__('sys').argv[1])),H); "
                    "print('READY',flush=True); s.serve_forever()"
                )
                for pid, port in ((dpu, 22), (client, 8080)):
                    process = subprocess.Popen(
                        ["nsenter", "-t", str(pid), "-n", "python3", "-c", server_code, str(port)],
                        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    )
                    processes.append(process)
                    self.assertEqual(process.stdout.readline().strip(), "READY")
                fakebin = root / "bin"
                fakebin.mkdir()
                (fakebin / "redis-cli").write_text(
                    "#!/bin/bash\n"
                    "case \"$*\" in\n"
                    " *keys*) printf 'DPUS|dpu0\\n' ;;\n"
                    " *midplane_interface*) printf 'dpu0\\n' ;;\n"
                    " *ips@*) printf '192.0.2.10\\n' ;;\n"
                    " *) exit 1 ;;\n"
                    "esac\n"
                )
                (fakebin / "ifconfig").write_text(
                    "#!/bin/bash\n"
                    "ip -4 -o addr show dev \"$1\" | awk '{split($4,a,\"/\"); print \" inet \" a[1]}'\n"
                )
                (fakebin / "netstat").write_text("#!/bin/bash\nprintf 'Proto Local Address\\n'\n")
                if IPTABLES != "iptables":
                    (fakebin / "iptables").write_text(
                        "#!/bin/bash\nexec " + shlex.quote(IPTABLES) + ' "$@"\n'
                    )
                for path in fakebin.iterdir():
                    path.chmod(0o755)
                env = dict(os.environ, PATH=f"{fakebin}:/usr/sbin:/usr/bin:/sbin:/bin")
                if REDIS_BIN:
                    redis_client, wrapper = start_redis(root, env, processes)
                    redis_client("-n", "4", "HSET", "DPUS|dpu0", "midplane_interface", "dpu0")
                    redis_client("-n", "4", "HSET", "DHCP_SERVER_IPV4_PORT|bridge-midplane|dpu0",
                                 "ips@", "192.0.2.10")
                    (fakebin / "redis-cli").write_text(wrapper)

                def script(*args):
                    return run("bash", str(SCRIPT), *args, env=env)

                def snapshot():
                    return "\n".join(line for line in run("iptables-save").stdout.splitlines()
                                     if not line.startswith("#"))

                connect_code = (
                    "import socket,sys; s=socket.create_connection((sys.argv[1],int(sys.argv[2])),1);"
                    "s.settimeout(1); s.sendall(b'F068'); print(s.recv(64).decode())"
                )
                inbound = ("inbound", "-e", "--dpus", "dpu0", "--ports", "9090")
                script(*inbound)
                forwarding = [Path("/proc/sys/net/ipv4/ip_forward"),
                              Path("/proc/sys/net/ipv4/conf/eth0/forwarding")]
                expected_forwarding = [path.read_text() for path in forwarding]
                self.assertEqual(expected_forwarding, ["1\n", "1\n"])
                before = snapshot()
                script(*inbound)
                self.assertEqual(snapshot(), before)
                reply = ns(client, "python3", "-c", connect_code, "198.51.100.1", "9090").stdout
                self.assertEqual(reply.strip(), "192.0.2.254|F068", "Bridge SNAT reply path changed")
                self.assertEqual(ns(dpu, "ip", "route", "show", "default").stdout, "")
                ns(dpu, "ip", "route", "add", "default", "via", "192.0.2.254")
                script("outbound", "-e", "--nofwctrl")
                before = snapshot()
                script("outbound", "-e", "--nofwctrl")
                self.assertEqual(snapshot(), before)
                reply = ns(dpu, "python3", "-c", connect_code, "198.51.100.2", "8080").stdout
                self.assertEqual(reply.strip(), "198.51.100.1|F068", "Outbound masquerade changed")
                script("outbound", "-d", "--nofwctrl")
                self.assertEqual([path.read_text() for path in forwarding], expected_forwarding)
                self.assertEqual(ns(client, "python3", "-c", connect_code,
                                    "198.51.100.1", "9090").stdout.strip(), "192.0.2.254|F068")
                script("inbound", "-d", "--dpus", "dpu0", "--ports", "9090")
                self.assertEqual([path.read_text() for path in forwarding], ["0\n", "0\n"])
                self.assertNotEqual(ns(client, "python3", "-c", connect_code,
                                       "198.51.100.1", "9090", check=False).returncode, 0)
                before = snapshot()
                script("inbound", "-d", "--dpus", "dpu0", "--ports", "9090")
                script("outbound", "-d", "--nofwctrl")
                self.assertEqual(snapshot(), before)
                self.assertEqual(run("iptables", "-t", "nat", "-S").stdout.count("\n"), 4)
                self.assertEqual(run("iptables", "-S").stdout.count("\n"), 3)
            finally:
                stop_processes(processes)
                for device in devices:
                    subprocess.run(["ip", "link", "del", device], capture_output=True, check=False)


if __name__ == "__main__":
    unittest.main(verbosity=2)
