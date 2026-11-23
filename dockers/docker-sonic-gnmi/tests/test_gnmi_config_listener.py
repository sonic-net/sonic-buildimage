import importlib.util
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
def env(monkeypatch):
    data = {
        ("GNMI", "gnmi"): {"port": "8080", "user_auth": "cert"},
        ("GNMI", "certs"): {"server_crt": "/server.crt"},
        ("DEVICE_METADATA", "localhost"): {"type": "ToRRouter"},
    }
    subscribers = {}
    selector = Mock()
    rpc = Mock()
    rpc.getProcessInfo.return_value = {"statename": "RUNNING"}
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
    return types.SimpleNamespace(
        listener=module.GnmiConfigListener(), data=data,
        subscribers=subscribers, selector=selector, rpc=rpc, log=log,
        clock=clock, module=module,
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
        call.stopProcess("gnmi-native", True),
        call.startProcess("gnmi-native", True),
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
        call.stopProcess("gnmi-native", True),
        call.startProcess("gnmi-native", True),
    ] * 2
    assert env.listener.config["GNMI", "gnmi"] == {"port": "50053"}


@pytest.mark.parametrize("followup_states,expected_ports", [
    (["RUNNING"], ["50053", "50052"]),
    (["STARTING", "RUNNING"], ["50053", "50052"]),
    (["STARTING", "STOPPED"], ["50053"]),
    ([OSError("socket unavailable")], ["50053"]),
])
def test_revert_during_restart_preserves_followup(
        env, followup_states, expected_ports):
    env.rpc.getProcessInfo.side_effect = [
        {"statename": "RUNNING"},
        *({"statename": state} if isinstance(state, str) else state
          for state in followup_states),
    ]
    launched_ports = []

    def stop(program, wait):
        if env.rpc.stopProcess.call_count == 1:
            env.data["GNMI", "gnmi"] = {"port": "50053"}
            env.subscribers["GNMI"].events.append(("gnmi", "SET", ()))

    def start(program, wait):
        launched_ports.append(env.data["GNMI", "gnmi"]["port"])
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
    assert env.rpc.getProcessInfo.call_count == len(followup_states) + 1


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
        {"statename": "STARTING"}, {"statename": final_state},
    ]
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
