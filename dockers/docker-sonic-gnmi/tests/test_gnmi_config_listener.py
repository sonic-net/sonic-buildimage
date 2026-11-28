import fcntl
import importlib.util
import json
import os
import subprocess
import sys
import types
from collections import deque
from http.client import HTTPException
from pathlib import Path
from unittest.mock import MagicMock, Mock, call, patch
from xmlrpc import client as xmlrpclib

import pytest


SCRIPT = Path(__file__).resolve().parents[1] / "gnmi-config-listener.py"


class Subscriber:
    def __init__(self):
        self.events = deque()

    def pop(self):
        return self.events.popleft() if self.events else ("", "", ())


@pytest.fixture
def env(monkeypatch, tmp_path):
    data = {
        ("GNMI", "gnmi"): {"port": "8080", "user_auth": "cert"},
        ("GNMI", "certs"): {
            "server_crt": "/server.crt", "server_key": "/server.key",
        },
        ("DEVICE_METADATA", "localhost"): {"type": "ToRRouter"},
    }
    subscribers = {}
    selector = Mock()
    rpc = Mock()
    rpc.getProcessInfo.return_value = {"statename": "RUNNING", "pid": 100}
    log = Mock()
    clock = [0]
    swss = Mock()
    swss.Table.side_effect = lambda db, name: types.SimpleNamespace(
        get=lambda key: (True, tuple(data.get((name, key), {}).items()))
    )

    def subscribe(db, name):
        subscribers[name] = Subscriber()
        return subscribers[name]

    swss.SubscriberStateTable.side_effect = subscribe
    swss.Select.return_value = selector
    swss.Select.OBJECT = 1
    swss.Select.TIMEOUT = 0
    swss.Select.ERROR = -1
    modules = {
        name: types.ModuleType(name)
        for name in (
            "swsscommon", "sonic_py_common", "sonic_py_common.logger",
            "supervisor", "supervisor.childutils", "supervisor.compat",
        )
    }
    modules["swsscommon"].swsscommon = swss
    modules["sonic_py_common.logger"].Logger = Mock(return_value=log)
    modules["supervisor.compat"].xmlrpclib = xmlrpclib
    proxy = MagicMock()
    proxy.__enter__.return_value = types.SimpleNamespace(supervisor=rpc)
    modules["supervisor.childutils"].getRPCInterface = Mock(
        return_value=proxy
    )
    spec = importlib.util.spec_from_file_location(
        "gnmi_config_listener", SCRIPT
    )
    module = importlib.util.module_from_spec(spec)
    with patch.dict(sys.modules, modules):
        spec.loader.exec_module(module)
    monkeypatch.setattr(module, "monotonic", lambda: clock[0])
    validate = Mock(return_value=subprocess.CompletedProcess([], 0, stdout=""))
    monkeypatch.setattr(module, "subprocess", types.SimpleNamespace(
        run=validate, PIPE=subprocess.PIPE, STDOUT=subprocess.STDOUT,
        CalledProcessError=subprocess.CalledProcessError,
    ), raising=False)
    config_file = tmp_path / "gnmi-config.json"
    monkeypatch.setattr(
        module, "PREPARED_CONFIG_FILE", str(config_file), raising=False
    )

    def acknowledge(program, wait):
        Path(str(config_file) + ".read").write_text("100\n")

    rpc.startProcess.side_effect = acknowledge
    return types.SimpleNamespace(
        listener=module.GnmiConfigListener(), data=data,
        subscribers=subscribers, selector=selector, rpc=rpc, log=log,
        clock=clock, module=module, validate=validate, config_file=config_file,
        acknowledge=acknowledge,
    )


def drive(env, events):
    events = deque(events)

    def select(timeout):
        if any(subscriber.events for subscriber in env.subscribers.values()):
            return env.module.swsscommon.Select.OBJECT, None
        if not events:
            raise StopIteration
        if timeout == 0 and events[0][0] > env.clock[0]:
            return env.module.swsscommon.Select.TIMEOUT, None
        now, table, key, values = events.popleft()
        env.clock[0] = now
        if table is None:
            return env.module.swsscommon.Select.TIMEOUT, None
        if values is None:
            env.data.pop((table, key), None)
        else:
            env.data[table, key] = values
        env.subscribers[table].events.append((key, "SET", ()))
        return env.module.swsscommon.Select.OBJECT, None

    env.selector.select.side_effect = select
    with pytest.raises(StopIteration):
        env.listener.run()


@pytest.mark.parametrize("table,key,values", [
    ("GNMI", "gnmi", {"port": "50052", "user_auth": "cert"}),
    ("GNMI", "gnmi", {"port": "8080", "user_auth": "password"}),
    ("GNMI", "certs", {"server_crt": "/replacement.crt"}),
    ("DEVICE_METADATA", "x509", {"server_key": "/replacement.key"}),
    ("DEVICE_METADATA", "localhost", {"type": "SmartSwitchDPU"}),
    ("MGMT_VRF_CONFIG", "vrf_global", {"mgmtVrfEnabled": "true"}),
    ("GNMI", "certs", None),
])
def test_relevant_changes_restart_only_gnmi(env, table, key, values):
    drive(env, [(0, table, key, values), (1, None, None, None)])
    assert env.rpc.method_calls == [
        call.getProcessInfo("gnmi-native"),
        call.getProcessInfo("gnmi-native"),
        call.stopProcess("gnmi-native", True),
        call.startProcess("gnmi-native", True),
        call.getProcessInfo("gnmi-native"),
    ]


@pytest.mark.parametrize("table,key,values", [
    ("GNMI", "gnmi", {"port": "8080", "user_auth": "cert"}),
    ("GNMI", "unrelated", {"port": "9000"}),
    ("DEVICE_METADATA", "localhost", {"type": "ToRRouter", "hostname": "new"}),
    ("DEVICE_METADATA", "other", {"type": "SmartSwitchDPU"}),
    ("MGMT_VRF_CONFIG", "vrf_global", {"unrelated": "true"}),
])
def test_unrelated_or_unchanged_input_is_ignored(env, table, key, values):
    drive(env, [(0, table, key, values), (1, None, None, None)])
    assert env.rpc.method_calls == []


def test_initial_subscription_dump_does_not_restart(env):
    events = [
        (0, table, key, values.copy())
        for (table, key), values in env.data.items()
    ]
    drive(env, events + [(1, None, None, None)])
    assert env.rpc.method_calls == []


def test_burst_is_coalesced_without_waiting_for_quiet(env):
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50051"}),
        (0.5, "GNMI", "gnmi", {"port": "50052"}),
        (1, "GNMI", "gnmi", {"port": "50053"}),
    ])
    env.rpc.startProcess.assert_called_once_with("gnmi-native", True)
    assert env.listener.config["GNMI", "gnmi"] == {"port": "50053"}


def test_change_reverted_before_deadline_does_not_restart(env):
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (0.5, "GNMI", "gnmi", {"port": "8080", "user_auth": "cert"}),
        (1, None, None, None),
    ])
    assert env.rpc.method_calls == []


def test_change_during_restart_is_handled_after_restart_finishes(env):
    def start(program, wait):
        env.acknowledge(program, wait)
        if env.rpc.startProcess.call_count == 1:
            env.data["GNMI", "gnmi"] = {"port": "50053"}
            env.subscribers["GNMI"].events.append(("gnmi", "SET", ()))

    env.rpc.startProcess.side_effect = start
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
        (1.1, "DEVICE_METADATA", "other", {}),
        (2.1, None, None, None),
    ])
    assert env.rpc.method_calls == [
        call.getProcessInfo("gnmi-native"),
        call.getProcessInfo("gnmi-native"),
        call.stopProcess("gnmi-native", True),
        call.startProcess("gnmi-native", True),
        call.getProcessInfo("gnmi-native"),
    ] * 2
    assert env.listener.config["GNMI", "gnmi"] == {"port": "50053"}


@pytest.mark.parametrize("followup_states,expected_ports", [
    (["RUNNING"], ["50052", "50052"]),
    (["STARTING", "RUNNING"], ["50052", "50052"]),
    (["STARTING", "STOPPED"], ["50052"]),
    ([OSError("socket unavailable")], ["50052"]),
])
def test_revert_during_restart_preserves_followup(
        env, followup_states, expected_ports):
    states = ["RUNNING", "RUNNING", "RUNNING"]
    for state in followup_states:
        states.append(state)
        if state == "RUNNING":
            states.extend([state, state])
    env.rpc.getProcessInfo.side_effect = [
        {"statename": state, "pid": 100} if isinstance(state, str) else state
        for state in states
    ]
    launched_ports = []

    def stop(program, wait):
        if env.rpc.stopProcess.call_count == 1:
            env.data["GNMI", "gnmi"] = {"port": "50053"}
            env.subscribers["GNMI"].events.append(("gnmi", "SET", ()))

    def start(program, wait):
        env.acknowledge(program, wait)
        launched_ports.append(
            json.loads(env.config_file.read_text())["GNMI"]["gnmi"]["port"]
        )
        if env.rpc.startProcess.call_count == 1:
            env.data["GNMI", "gnmi"] = {"port": "50052"}
            env.subscribers["GNMI"].events.append(("gnmi", "SET", ()))

    env.rpc.stopProcess.side_effect = stop
    env.rpc.startProcess.side_effect = start
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
        (2, None, None, None),
        (3, None, None, None),
        (4, None, None, None),
        (5, "GNMI", "gnmi", {"port": "50052"}),
        (6, None, None, None),
    ])
    assert launched_ports == expected_ports
    assert env.rpc.getProcessInfo.call_count == len(states)


def test_noop_after_restart_queue_drains_is_ignored(env):
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
        (2, "GNMI", "gnmi", {"port": "50052"}),
        (3, None, None, None),
    ])
    env.rpc.startProcess.assert_called_once_with("gnmi-native", True)


def test_unrelated_event_queued_during_restart_is_ignored(env):
    def start(program, wait):
        env.acknowledge(program, wait)
        env.subscribers["DEVICE_METADATA"].events.append(("other", "SET", ()))

    env.rpc.startProcess.side_effect = start
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
        (2, None, None, None),
    ])
    env.rpc.startProcess.assert_called_once_with("gnmi-native", True)


@pytest.mark.parametrize("state", [
    "STOPPED", "STOPPING", "EXITED", "FATAL", "BACKOFF",
])
def test_stopped_or_failed_process_is_not_started(env, state):
    env.rpc.getProcessInfo.return_value = {"statename": state}
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
        (2, None, None, None),
    ])
    env.rpc.stopProcess.assert_not_called()
    env.rpc.startProcess.assert_not_called()


@pytest.mark.parametrize("final_state", ["RUNNING", "STOPPED"])
def test_change_while_starting_waits_without_overriding_stop(env, final_state):
    env.rpc.getProcessInfo.side_effect = [
        {"statename": "STARTING", "pid": 100},
        {"statename": final_state, "pid": 100},
    ] + ([{"statename": "RUNNING", "pid": 100}] * 2
         if final_state == "RUNNING" else [])
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
        (2, None, None, None),
    ])
    if final_state == "RUNNING":
        env.rpc.startProcess.assert_called_once_with("gnmi-native", True)
    else:
        env.rpc.startProcess.assert_not_called()


@pytest.mark.parametrize("method,error", [
    ("getProcessInfo", OSError("socket unavailable")),
    ("getProcessInfo", HTTPException("invalid response")),
    ("getProcessInfo", xmlrpclib.ProtocolError("local", 500, "error", {})),
    ("stopProcess", xmlrpclib.Fault(70, "NOT_RUNNING")),
    ("startProcess", xmlrpclib.Fault(50, "SPAWN_ERROR")),
])
def test_supervisor_failure_is_logged_not_retried(env, method, error):
    getattr(env.rpc, method).side_effect = error
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
        (2, "GNMI", "gnmi", {"port": "50052"}),
        (3, None, None, None),
    ])
    env.log.log_error.assert_called_once()
    assert "supervisor restart failed" in env.log.log_error.call_args[0][0]
    assert getattr(env.rpc, method).call_count == 1
    if method != "startProcess":
        env.rpc.startProcess.assert_not_called()
    assert not env.config_file.exists()
    assert not Path(str(env.config_file) + ".read").exists()


def test_new_change_after_a_rejected_restart_can_request_restart(env):
    env.rpc.stopProcess.side_effect = [xmlrpclib.Fault(70, "NOT_RUNNING"), True]
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
        (2, "GNMI", "gnmi", {"port": "50053"}),
        (3, None, None, None),
    ])
    assert env.rpc.stopProcess.call_count == 2
    env.rpc.startProcess.assert_called_once_with("gnmi-native", True)


def test_subscription_failure_is_reported_to_supervision(env):
    env.selector.select.return_value = (
        env.module.swsscommon.Select.ERROR, None
    )
    with pytest.raises(RuntimeError, match="subscription select failed"):
        env.listener.run()
    env.rpc.startProcess.assert_not_called()


def test_incomplete_edits_keep_runtime_until_a_valid_candidate_arrives(env):
    env.validate.side_effect = [
        subprocess.CalledProcessError(
            5, "gnmi-native.sh", output="TLS requires both certificate and key"
        ),
        subprocess.CalledProcessError(
            5, "gnmi-native.sh", output="TLS requires both certificate and key"
        ),
        subprocess.CompletedProcess([], 0, stdout=""),
    ]
    drive(env, [
        (0, "GNMI", "certs", {"ca_crt": "/new-ca.crt"}),
        (1, None, None, None),
        (2, "GNMI", "certs", {"ca_crt": "/new-ca.crt"}),
        (3, None, None, None),
        (4, "GNMI", "certs", {
            "ca_crt": "/new-ca.crt", "server_crt": "/new-server.crt",
        }),
        (5, None, None, None),
    ])
    env.rpc.stopProcess.assert_not_called()
    env.rpc.startProcess.assert_not_called()
    assert env.validate.call_count == 2
    assert env.log.log_warning.call_count == 2
    assert "unapplied" in env.log.log_warning.call_args[0][0]
    assert env.listener.config["GNMI", "certs"]["server_key"] == "/server.key"
    assert not env.config_file.exists()

    drive(env, [
        (6, "GNMI", "certs", {
            "ca_crt": "/new-ca.crt", "server_crt": "/new-server.crt",
            "server_key": "/new-server.key",
        }),
        (7, None, None, None),
    ])
    env.rpc.stopProcess.assert_called_once_with("gnmi-native", True)
    env.rpc.startProcess.assert_called_once_with("gnmi-native", True)
    assert not env.config_file.exists()


def test_restart_holds_the_validated_snapshot_not_a_later_db_read(env):
    validated = []

    def validate(command, **kwargs):
        validated.append(json.loads(Path(command[2]).read_text()))
        return subprocess.CompletedProcess(command, 0, stdout="")

    def stop(program, wait):
        assert env.config_file.stat().st_mode & 0o777 == 0o600
        with env.config_file.open() as snapshot:
            with pytest.raises(BlockingIOError):
                fcntl.flock(snapshot, fcntl.LOCK_EX | fcntl.LOCK_NB)
        env.data["GNMI", "gnmi"] = {"port": "50053"}

    def start(program, wait):
        env.acknowledge(program, wait)
        assert json.loads(env.config_file.read_text()) == validated[0]
        assert validated[0]["GNMI"]["gnmi"]["port"] == "50052"

    env.validate.side_effect = validate
    env.rpc.stopProcess.side_effect = stop
    env.rpc.startProcess.side_effect = start
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
    ])
    assert len(validated) == 1
    assert not env.config_file.exists()


@pytest.mark.parametrize("final_info", [
    {"statename": "STOPPED"},
    {"statename": "RUNNING", "pid": 200},
])
def test_operator_change_during_validation_is_not_overridden(env, final_info):
    env.rpc.getProcessInfo.return_value = {"statename": "RUNNING", "pid": 100}

    def validate(command, **kwargs):
        env.rpc.getProcessInfo.return_value = final_info
        return subprocess.CompletedProcess(command, 0, stdout="")

    env.validate.side_effect = validate
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
    ])
    env.rpc.stopProcess.assert_not_called()
    env.rpc.startProcess.assert_not_called()
    assert not env.config_file.exists()


def test_rejected_edit_reverted_to_running_config_needs_no_restart(env):
    original = env.data["GNMI", "certs"].copy()
    env.validate.side_effect = subprocess.CalledProcessError(
        5, "gnmi-native.sh", output="TLS requires both certificate and key"
    )
    drive(env, [
        (0, "GNMI", "certs", {"ca_crt": "/new-ca.crt"}),
        (1, None, None, None),
        (2, "GNMI", "certs", original),
        (3, None, None, None),
    ])
    env.rpc.stopProcess.assert_not_called()
    env.validate.assert_called_once()
    assert "withdrawn" in env.log.log_notice.call_args[0][0]


@pytest.fixture
def launcher(tmp_path, monkeypatch):
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    template = tmp_path / "telemetry_vars.j2"
    template.touch()
    config_file = tmp_path / "gnmi-config.json"
    args_file = tmp_path / "args.json"
    db_reads = tmp_path / "db-reads"
    source = SCRIPT.with_name("gnmi-native.sh").read_text()
    for before, after in (
        ("TELEMETRY_VARS_FILE=/usr/share/sonic/templates/telemetry_vars.j2",
         "TELEMETRY_VARS_FILE=" + str(template)),
        ("PREPARED_CONFIG_FILE=/run/gnmi-config.json",
         "PREPARED_CONFIG_FILE=" + str(config_file)),
        ("PATHS_BLACKLIST_FILE=/etc/telemetry/paths_blacklist.txt",
         "PATHS_BLACKLIST_FILE=" + str(tmp_path / "paths_blacklist.txt")),
        ("exec /usr/sbin/telemetry ${TELEMETRY_ARGS}",
         "exec telemetry ${TELEMETRY_ARGS}"),
    ):
        assert source.count(before) == 1
        source = source.replace(before, after)
    script = tmp_path / "gnmi-native.sh"
    script.write_text(source)
    script.chmod(0o700)

    tools = {
        "sonic-cfggen": """
if "-j" in sys.argv:
    if os.environ.get("GNMI_TEST_UNLINK_ON_CFGGEN"):
        Path(os.environ["GNMI_TEST_UNLINK_ON_CFGGEN"]).unlink(missing_ok=True)
    config = json.loads(Path(sys.argv[sys.argv.index("-j") + 1]).read_text())
else:
    with open(os.environ["GNMI_TEST_DB_READS"], "a") as log:
        log.write("cfggen\\n")
    config = json.loads(os.environ["GNMI_TEST_LIVE_CONFIG"])
print(json.dumps({
    "gnmi": config.get("GNMI", {}).get("gnmi", ""),
    "certs": config.get("GNMI", {}).get("certs", ""),
    "x509": config.get("DEVICE_METADATA", {}).get("x509", ""),
}))
""",
        "sonic-db-cli": """
with open(os.environ["GNMI_TEST_DB_READS"], "a") as log:
    log.write("db-cli\\n")
config = json.loads(os.environ["GNMI_TEST_LIVE_CONFIG"])
table, key = sys.argv[-2].split("|")
print(config.get(table, {}).get(key, {}).get(sys.argv[-1], ""))
""",
        "flock": """
assert sys.argv[1:5] == ["-s", "-n", "-E", "75"]
assert sys.argv[-1] == "true"
with open(sys.argv[-2], "a") as lock:
    try:
        fcntl.flock(lock, fcntl.LOCK_SH | fcntl.LOCK_NB)
    except BlockingIOError:
        sys.exit(75)
""",
        "telemetry": """
Path(os.environ["GNMI_TEST_ARGS_FILE"]).write_text(json.dumps(sys.argv[1:]))
""",
    }
    for name, body in tools.items():
        tool = bin_dir / name
        tool.write_text(
            "#!" + sys.executable + "\n"
            "import fcntl, json, os, sys\nfrom pathlib import Path\n" + body
        )
        tool.chmod(0o700)
    monkeypatch.setenv("PATH", str(bin_dir) + os.pathsep + os.environ["PATH"])
    monkeypatch.setenv("GNMI_TEST_ARGS_FILE", str(args_file))
    monkeypatch.setenv("GNMI_TEST_DB_READS", str(db_reads))
    monkeypatch.setenv("GNMI_TEST_LIVE_CONFIG", json.dumps({
        "GNMI": {"gnmi": {"port": "9090"}},
        "DEVICE_METADATA": {}, "MGMT_VRF_CONFIG": {},
    }))
    monkeypatch.delenv("GNMI_LISTENER_MODE", raising=False)

    def run(*arguments):
        return subprocess.run(
            ["bash", str(script), *arguments], text=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )

    return types.SimpleNamespace(
        script=script, config_file=config_file, args_file=args_file,
        db_reads=db_reads, run=run,
    )


def test_real_launcher_rejects_partial_edits_then_activates_snapshot(
        env, launcher, monkeypatch):
    monkeypatch.setattr(env.module, "subprocess", subprocess)
    monkeypatch.setattr(env.module, "LAUNCHER", str(launcher.script))
    env.data["GNMI", "gnmi"] = {
        "port": "50052", "user_auth": "password", "vrf": "mgmt",
    }
    env.data["DEVICE_METADATA", "localhost"]["subtype"] = "SmartSwitch"
    env.data["MGMT_VRF_CONFIG", "vrf_global"] = {"mgmtVrfEnabled": "true"}

    def stop(program, wait):
        env.data["GNMI", "gnmi"] = {"port": "9090", "user_auth": "none"}
        monkeypatch.setenv(
            "GNMI_TEST_UNLINK_ON_CFGGEN", str(launcher.config_file)
        )

    def start(program, wait):
        result = launcher.run("start")
        assert result.returncode == 0, result.stdout
        args = json.loads(launcher.args_file.read_text())
        assert args[args.index("--port") + 1] == "50052"
        assert args[args.index("--server_key") + 1] == "/new.key"
        assert args[args.index("--client_auth") + 1] == "password"
        assert args[args.index("--vrf") + 1] == "mgmt"
        assert args[args.index("--gnmi_vrf") + 1] == "mgmt"
        assert "-zmq_port=8100" in args
        assert not launcher.db_reads.exists()
        env.rpc.getProcessInfo.return_value = {
            "statename": "RUNNING",
            "pid": int(Path(str(launcher.config_file) + ".read").read_text()),
        }

    env.rpc.stopProcess.side_effect = stop
    env.rpc.startProcess.side_effect = start
    drive(env, [
        (0, "GNMI", "certs", {"ca_crt": "/new-ca.crt"}),
        (1, None, None, None),
        (2, "GNMI", "certs", {
            "ca_crt": "/new-ca.crt", "server_crt": "/new.crt",
        }),
        (3, None, None, None),
    ])
    env.rpc.stopProcess.assert_not_called()
    assert not launcher.args_file.exists()
    assert "requires both server_crt and server_key" in (
        env.log.log_warning.call_args[0][0]
    )
    drive(env, [
        (4, "GNMI", "certs", {
            "ca_crt": "/new-ca.crt", "server_crt": "/new.crt",
            "server_key": "/new.key",
        }),
        (5, None, None, None),
    ])
    env.rpc.startProcess.assert_called_once_with("gnmi-native", True)
    assert not launcher.config_file.exists()


def test_locked_snapshot_is_reusable_but_abandoned_handoff_is_ignored(launcher):
    snapshot = {
        "GNMI": {"gnmi": {"port": "50052"}},
        "DEVICE_METADATA": {}, "MGMT_VRF_CONFIG": {},
    }
    launcher.config_file.write_text(json.dumps(snapshot))
    with launcher.config_file.open() as candidate:
        fcntl.flock(candidate, fcntl.LOCK_EX)
        for _ in range(2):
            result = launcher.run()
            assert result.returncode == 0, result.stdout
            args = json.loads(launcher.args_file.read_text())
            assert args[args.index("--port") + 1] == "50052"
            assert "--noTLS" in args
            assert args[args.index("--client_auth") + 1] == "cert"
            assert not launcher.db_reads.exists()
    result = launcher.run()
    assert result.returncode == 0, result.stdout
    args = json.loads(launcher.args_file.read_text())
    assert args[args.index("--port") + 1] == "9090"
    assert launcher.db_reads.exists()


@pytest.mark.parametrize("table,key", [
    ("GNMI", "certs"), ("DEVICE_METADATA", "x509"),
])
def test_launcher_check_reuses_tls_rejection_without_exec(
        launcher, table, key):
    snapshot = {
        "GNMI": {"gnmi": {"port": "50052"}},
        "DEVICE_METADATA": {}, "MGMT_VRF_CONFIG": {},
    }
    snapshot[table][key] = {"server_crt": "/server.crt"}
    launcher.config_file.write_text(json.dumps(snapshot))
    result = launcher.run("--check-config", str(launcher.config_file))
    assert result.returncode == 5
    assert "requires both server_crt and server_key" in result.stdout
    assert not launcher.args_file.exists()
    assert not launcher.db_reads.exists()


@pytest.mark.parametrize("arguments", [
    ("--check-config",), ("--check-config", "config.json", "extra"),
])
def test_malformed_check_request_cannot_launch_native(launcher, arguments):
    result = launcher.run(*arguments)
    assert result.returncode == 2
    assert not launcher.args_file.exists()


def test_handoff_waits_for_the_current_process_not_an_earlier_attempt(
        env, monkeypatch):
    received = Path(str(env.config_file) + ".read")
    env.rpc.startProcess.side_effect = lambda *args: received.write_text("99\n")
    wait = Mock(side_effect=lambda seconds: received.write_text("100\n"))
    monkeypatch.setattr(env.module, "sleep", wait)
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
    ])
    wait.assert_called_once_with(0.1)
    env.rpc.startProcess.assert_called_once_with("gnmi-native", True)
    assert not env.config_file.exists()
    assert not received.exists()


@pytest.mark.parametrize("final_info", [
    {"statename": "STOPPED", "pid": 0},
    {"statename": "RUNNING", "pid": 200},
])
def test_handoff_stops_waiting_if_the_process_is_stopped_or_replaced(
        env, monkeypatch, final_info):
    received = Path(str(env.config_file) + ".read")
    env.rpc.startProcess.side_effect = lambda *args: received.write_text("99\n")
    env.rpc.getProcessInfo.side_effect = [
        {"statename": "RUNNING", "pid": 100},
        {"statename": "RUNNING", "pid": 100},
        {"statename": "RUNNING", "pid": 100},
        final_info,
    ]
    monkeypatch.setattr(env.module, "sleep", Mock())
    drive(env, [
        (0, "GNMI", "gnmi", {"port": "50052"}),
        (1, None, None, None),
    ])
    env.rpc.startProcess.assert_called_once_with("gnmi-native", True)
    env.log.log_error.assert_called_once()
    assert not env.config_file.exists()
    assert not received.exists()
