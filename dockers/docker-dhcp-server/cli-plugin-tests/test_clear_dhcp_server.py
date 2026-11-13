import copy
import json
import socket
import subprocess
import sys
import threading
import time
from contextlib import contextmanager, nullcontext
from unittest import mock

import click
import pytest
from click.testing import CliRunner
import utilities_common.cli as clicommon

sys.path.append("../cli/clear/plugins/")
import clear_dhcp_server


MAC_A = "aa:bb:cc:dd:ee:01"
MAC_B = "aa:bb:cc:dd:ee:02"


@pytest.fixture
def state_events():
    subscriber = mock.Mock()
    selector = mock.Mock()
    selector.select.side_effect = AssertionError("Unexpected wait after synchronous state refresh")
    return subscriber, selector


@pytest.fixture
def server(mock_db, state_events):
    """Model live Kea leases separately from the signal-driven State DB cache."""
    mock_db.hmset("CONFIG_DB", "VLAN|Vlan100", {"vlanid": "100"})
    mock_db.hmset("CONFIG_DB", "VLAN|Vlan1000", {"vlanid": "1000"})
    mock_db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan1000", {"mode": "MATCH"})
    leases = [
        {"subnet-id": 100, "ip-address": "100.1.1.3", "hw-address": MAC_A},
        {"subnet-id": 100, "ip-address": "100.1.1.4", "hw-address": MAC_B},
        {"subnet-id": 1000, "ip-address": "192.168.0.20", "hw-address": MAC_A},
    ]
    for entry in leases:
        mock_db.hmset(
            "STATE_DB", "DHCP_SERVER_IPV4_LEASE|Vlan{}|{}".format(
                entry["subnet-id"], entry["hw-address"]),
            {"ip": entry["ip-address"], "lease_start": "100", "lease_end": "200"})
    config = {
        "subnet4": [
            {"id": 100, "subnet": "100.1.1.0/24"},
            {"id": 1000, "subnet": "192.168.0.0/21"},
        ]
    }

    def command(name, arguments=None):
        if name == "config-get":
            return {"result": 0, "arguments": {"Dhcp4": config}}
        if name == "lease4-get-all":
            # Deliberately include another subnet: the client must scope its
            # destructive operations even if a response contains extra leases.
            return {"result": 0, "arguments": {"leases": copy.deepcopy(leases)}}
        if name == "lease4-del":
            selected = [entry for entry in leases if entry["ip-address"] == arguments["ip-address"]]
        elif name == "lease4-wipe":
            assert arguments["subnet-id"] > 0
            selected = [entry for entry in leases if entry["subnet-id"] == arguments["subnet-id"]]
        else:
            raise AssertionError(name)
        for entry in selected:
            leases.remove(entry)
        return {"result": 0 if selected else 3}

    def notify(*arguments):
        assert arguments == ("supervisorctl", "signal", "USR1", "dhcp-server-ipv4:dhcpservd")
        for key in list(mock_db.keys("STATE_DB", "DHCP_SERVER_IPV4_LEASE|Vlan100|*")):
            entry = mock_db.get_all("STATE_DB", key)
            if not any(
                    lease["ip-address"] == entry["ip"] and lease["hw-address"] == key.split("|")[-1]
                    for lease in leases):
                mock_db.delete("STATE_DB", key)
        # Acknowledge the request as dhcpservd does after a completed refresh.
        request_id = mock_db.get("STATE_DB", clear_dhcp_server.LEASE_SYNC_KEY, "request_id")
        mock_db.set("STATE_DB", clear_dhcp_server.LEASE_SYNC_KEY, "completed_id", request_id)
        return ""

    db = clicommon.Db()
    db.db = mock_db
    with mock.patch.object(clear_dhcp_server.os, "geteuid", return_value=0), \
         mock.patch.object(clear_dhcp_server, "_lease_clear_lock", side_effect=nullcontext), \
         mock.patch.object(clear_dhcp_server, "_lease_state_events",
                           side_effect=lambda: nullcontext(state_events)), \
         mock.patch.object(clear_dhcp_server, "_kea_command", side_effect=command) as kea, \
         mock.patch.object(clear_dhcp_server, "_container_command", side_effect=notify) as signal:
        yield db, leases, config, kea, signal


def invoke(server, *arguments):
    """Invoke the registered command hierarchy, including feature validation."""
    return CliRunner().invoke(
        clear_dhcp_server.dhcp_server, ["ipv4", "lease", *arguments], obj=server[0])


@pytest.mark.parametrize("selector,remaining", [
    (["100.1.1.3"], ["100.1.1.4"]),
    (["--all"], []),
])
def test_clear_lease_scoped_to_interface(server, selector, remaining):
    """Clear both stores while preserving every other interface's leases."""
    db, leases, _, kea, signal = server
    other_keys = db.db.keys("STATE_DB", "DHCP_SERVER_IPV4_LEASE|Vlan1000|*")
    other_state = {key: copy.deepcopy(db.db.get_all("STATE_DB", key)) for key in other_keys}
    result = invoke(server, "Vlan100", *selector)
    assert result.exit_code == 0, result.output
    assert "from Kea and State DB" in result.output
    assert [entry["ip-address"] for entry in leases if entry["subnet-id"] == 100] == remaining
    assert any(entry["subnet-id"] == 1000 and entry["hw-address"] == MAC_A for entry in leases)
    assert {key: db.db.get_all("STATE_DB", key) for key in other_keys} == other_state
    assert sorted(
        db.db.get_all("STATE_DB", key)["ip"]
        for key in db.db.keys("STATE_DB", "DHCP_SERVER_IPV4_LEASE|Vlan100|*")) == remaining
    signal.assert_called()
    if selector == ["--all"]:
        assert mock.call("lease4-wipe", {"subnet-id": 100}) in kea.call_args_list
    else:
        assert [call for call in kea.call_args_list if call.args[0] == "lease4-del"] == [
            mock.call("lease4-del", {"ip-address": "100.1.1.3"}),
        ]


@pytest.mark.parametrize("mode", ["PORT", "MATCH"])
def test_clear_supports_both_modes(server, mode):
    """Lease clearing is independent of the allocation policy mode."""
    server[0].db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", {"mode": mode})
    assert invoke(server, "Vlan100", "100.1.1.3").exit_code == 0


def test_clear_ip_reacquisition_is_reported(server):
    original = server[3].side_effect

    def reacquire(name, arguments=None):
        response = original(name, arguments)
        if name == "lease4-del" and response["result"] == 0:
            server[1].append({"subnet-id": 100, "ip-address": "100.1.1.3", "hw-address": MAC_B})
        return response

    server[3].side_effect = reacquire
    result = invoke(server, "Vlan100", "100.1.1.3")

    assert result.exit_code == 1, result.output
    assert "Matching leases still exist" in result.output
    assert sum(call.args[0] == "lease4-del" for call in server[3].call_args_list) == 1


@pytest.mark.parametrize("arguments", [
    [], ["Vlan100"], ["Vlan100", "100.1.1.3", "--all"],
    ["Vlan100", "::1"], ["Vlan100", "300.1.1.1"],
])
def test_invalid_selectors_do_not_contact_server(server, arguments):
    """Reject missing, conflicting, IPv6 and malformed selectors before mutation."""
    original = copy.deepcopy(server[1])
    result = invoke(server, *arguments)
    assert result.exit_code == 2, result.output
    assert server[1] == original
    server[3].assert_not_called()
    server[4].assert_not_called()


@pytest.mark.parametrize("arguments", [
    ["Vlan100", "--mac", MAC_A],
    ["Vlan100", "100.1.1.3", "--mac", MAC_A],
    ["Vlan100", "--all", "--mac", MAC_A],
])
def test_removed_mac_selector_is_rejected(server, arguments):
    original = copy.deepcopy(server[1])

    result = invoke(server, *arguments)

    assert result.exit_code == 2, result.output
    assert "No such option: --mac" in result.output
    assert server[1] == original
    server[3].assert_not_called()
    server[4].assert_not_called()


def test_help_only_advertises_ip_and_all(server):
    result = invoke(server, "--help")

    assert result.exit_code == 0, result.output
    assert "IP_ADDRESS" in result.output
    assert "--all" in result.output
    assert "--mac" not in result.output
    server[3].assert_not_called()


def test_no_matching_lease_is_failure(server):
    """No-match must be visible to both the operator and calling scripts."""
    result = invoke(server, "Vlan100", "100.1.1.99")
    assert result.exit_code == 1
    assert "No matching Kea lease" in result.output
    assert not any(call.args[0] in ("lease4-del", "lease4-wipe") for call in server[3].call_args_list)


@pytest.mark.parametrize("interface,address", [
    ("Vlan100", "192.168.0.20"),
    ("Vlan1000", "100.1.1.3"),
])
def test_ip_on_another_interface_cannot_be_deleted(server, interface, address):
    original = copy.deepcopy(server[1])

    result = invoke(server, interface, address)

    assert result.exit_code == 1, result.output
    assert "No matching Kea lease" in result.output
    assert server[1] == original
    assert not any(call.args[0] in ("lease4-del", "lease4-wipe") for call in server[3].call_args_list)


@pytest.mark.parametrize("interface", ["Vlan999", "*", "Vlan100*"])
def test_unknown_interface_is_not_a_wildcard(server, interface):
    """An interface is mandatory and exact, never a glob or implicit global scope."""
    result = invoke(server, interface, "--all")
    assert result.exit_code == 1
    assert "not configured" in result.output
    server[3].assert_not_called()


@pytest.mark.parametrize("subnets", [
    [], [{"id": 0}], [{"id": 100}, {"id": 100}],
])
def test_missing_or_ambiguous_running_subnet_cannot_wipe(server, subnets):
    """Never fall back to Kea's dangerous all-subnets wipe semantics."""
    server[2]["subnet4"] = subnets
    result = invoke(server, "Vlan100", "--all")
    assert result.exit_code == 1
    assert "No unique active Kea subnet" in result.output
    assert not any(call.args[0] == "lease4-wipe" for call in server[3].call_args_list)


def test_smart_switch_uses_running_midplane_subnet(server):
    """Resolve the existing smart-switch subnet without assuming a VLAN name."""
    db, leases, config, _, _ = server
    db.db.hmset("CONFIG_DB", "DHCP_SERVER_IPV4|bridge-midplane", {"mode": "PORT"})
    db.db.hmset("CONFIG_DB", "MID_PLANE_BRIDGE|GLOBAL", {
        "bridge": "bridge-midplane", "ip_prefix": "169.254.200.254/24"})
    config["subnet4"].append({"id": 10000, "subnet": "169.254.200.0/24"})
    leases.append({"subnet-id": 10000, "ip-address": "169.254.200.1", "hw-address": MAC_A})
    result = invoke(server, "bridge-midplane", "169.254.200.1")
    assert result.exit_code == 0, result.output
    assert not any(entry["subnet-id"] == 10000 for entry in leases)


def test_feature_disabled_cannot_clear(server):
    """Respect the standard DHCP feature gate."""
    server[0].db.set("CONFIG_DB", "FEATURE|dhcp_server", "state", "disabled")
    assert invoke(server, "Vlan100", "--all").exit_code != 0
    server[3].assert_not_called()


def test_non_root_cannot_clear(server):
    """Require privileges before taking a lock or talking to Docker."""
    with mock.patch.object(clear_dhcp_server.os, "geteuid", return_value=1000):
        result = invoke(server, "Vlan100", "--all")
    assert result.exit_code == 1
    assert "sudo" in result.output
    server[3].assert_not_called()


def test_state_sync_timeout_is_not_reported_as_success(server):
    """A successful Kea deletion is not sufficient when the cache remains stale."""
    server[4].side_effect = None
    with mock.patch.object(clear_dhcp_server, "STATE_DB_TIMEOUT", 0):
        result = invoke(server, "Vlan100", "100.1.1.3")
    assert result.exit_code == 1
    assert "State DB synchronization timed out" in result.output
    assert not any(entry["ip-address"] == "100.1.1.3" for entry in server[1])


@pytest.fixture
def state_wait(state_events):
    clock = [0]
    subscriber, selector = state_events
    sync = {}
    db = mock.Mock()
    db.set.side_effect = lambda _, key, field, value: sync.__setitem__((key, field), value)
    db.get.side_effect = lambda _, key, field: sync.get((key, field))

    def timeout(milliseconds):
        clock[0] += milliseconds / 1000
        return clear_dhcp_server.swsscommon.Select.TIMEOUT, None

    selector.select.side_effect = timeout
    with mock.patch.object(clear_dhcp_server.time, "monotonic", side_effect=lambda: clock[0]), \
         mock.patch.object(clear_dhcp_server.time, "sleep", side_effect=AssertionError("Unexpected polling")), \
         mock.patch.object(clear_dhcp_server, "_container_command") as notify, \
         mock.patch.object(clear_dhcp_server, "_state_leases", return_value=[]) as leases:
        yield clock, subscriber, selector, notify, leases, db, sync


def acknowledge(sync, request_id=None):
    key = clear_dhcp_server.LEASE_SYNC_KEY
    sync[(key, "completed_id")] = request_id or sync[(key, "request_id")]


def test_state_sync_waits_for_acknowledgement_event(state_wait):
    """Block for the daemon's acknowledgement instead of polling Redis."""
    clock, subscriber, selector, notify, leases, db, sync = state_wait
    notify.side_effect = lambda *args: (clear_dhcp_server.LEASE_SYNC_KEY, "request_id") in sync or pytest.fail(
        "Refresh requested before the request ID was published")

    def update(milliseconds):
        assert milliseconds == 8000
        clock[0] = 5
        acknowledge(sync)
        return clear_dhcp_server.swsscommon.Select.OBJECT, subscriber

    selector.select.side_effect = update
    clear_dhcp_server._sync_state_db(db, "Vlan100", None, subscriber, selector)

    notify.assert_called_once()
    leases.assert_called_once_with(db, "Vlan100", None)
    assert clock[0] == 5


def test_state_sync_retries_a_missed_notification(state_wait):
    """Retry a coalesced or ignored signal without flooding the daemon."""
    clock, subscriber, selector, notify, _, db, sync = state_wait
    notify.side_effect = lambda *args: acknowledge(sync) if notify.call_count == 2 else None
    clear_dhcp_server._sync_state_db(db, "Vlan100", None, subscriber, selector)

    selector.select.assert_called_once_with(8000)
    assert notify.call_count == 2
    assert clock[0] == 8


def test_state_sync_empty_cache_needs_current_acknowledgement(state_wait):
    """An empty cache or an older refresh's acknowledgement is not completion."""
    clock, subscriber, selector, notify, leases, db, sync = state_wait
    acknowledge(sync, "older-request")

    def events(milliseconds):
        clock[0] += 1
        if selector.select.call_count == 2:
            acknowledge(sync)
        return clear_dhcp_server.swsscommon.Select.OBJECT, subscriber

    selector.select.side_effect = events
    clear_dhcp_server._sync_state_db(db, "Vlan100", "100.1.1.3", subscriber, selector)

    assert selector.select.call_count == 2
    leases.assert_called_once()
    notify.assert_called_once()


def test_state_sync_reports_leases_remaining_after_acknowledgement(state_wait):
    _, subscriber, selector, notify, leases, db, sync = state_wait
    leases.return_value = ["DHCP_SERVER_IPV4_LEASE|Vlan100|" + MAC_B]
    notify.side_effect = lambda *args: acknowledge(sync)

    with pytest.raises(click.ClickException, match="Matching leases still exist in State DB"):
        clear_dhcp_server._sync_state_db(db, "Vlan100", "100.1.1.3", subscriber, selector)
    selector.select.assert_not_called()


@pytest.mark.parametrize("updated", [False, True])
def test_state_sync_checks_acknowledgement_at_deadline(state_wait, updated):
    clock, subscriber, selector, notify, _, db, sync = state_wait

    def timeout(milliseconds):
        clock[0] += milliseconds / 1000
        if updated and clock[0] >= 20:
            acknowledge(sync)
        return clear_dhcp_server.swsscommon.Select.TIMEOUT, None

    selector.select.side_effect = timeout
    if updated:
        clear_dhcp_server._sync_state_db(db, "Vlan100", None, subscriber, selector)
    else:
        with pytest.raises(click.ClickException, match="synchronization timed out"):
            clear_dhcp_server._sync_state_db(db, "Vlan100", None, subscriber, selector)

    assert selector.select.call_args_list == [mock.call(8000), mock.call(8000), mock.call(4000)]
    assert notify.call_count == 3


def test_state_sync_select_error_is_explicit(state_wait):
    _, subscriber, selector, _, _, db, _ = state_wait
    selector.select.side_effect = None
    selector.select.return_value = clear_dhcp_server.swsscommon.Select.ERROR, None

    with pytest.raises(click.ClickException, match="select\\(\\) failed"):
        clear_dhcp_server._sync_state_db(db, "Vlan100", None, subscriber, selector)


@pytest.mark.parametrize("fail", [False, True])
def test_lease_state_subscription_cleanup(fail):
    subscriber, selector = mock.Mock(), mock.Mock()
    with mock.patch.object(clear_dhcp_server.swsscommon, "DBConnector") as connector, \
         mock.patch.object(clear_dhcp_server.swsscommon, "SubscriberStateTable", return_value=subscriber) as table, \
         mock.patch.object(clear_dhcp_server.swsscommon, "Select", return_value=selector):
        if fail:
            with pytest.raises(click.ClickException, match="Cannot monitor DHCP lease state: disconnected"):
                with clear_dhcp_server._lease_state_events():
                    raise RuntimeError("disconnected")
        else:
            with clear_dhcp_server._lease_state_events() as events:
                assert events == (subscriber, selector)
        table.assert_called_once_with(connector.return_value, "DHCP_SERVER_IPV4_LEASE_SYNC")
        selector.addSelectable.assert_called_once_with(subscriber)
        selector.removeSelectable.assert_called_once_with(subscriber)


def test_subscription_setup_failure_is_explicit():
    with mock.patch.object(clear_dhcp_server.swsscommon, "DBConnector", side_effect=RuntimeError("unavailable")):
        with pytest.raises(click.ClickException, match="Cannot monitor DHCP lease state: unavailable"):
            with clear_dhcp_server._lease_state_events():
                pytest.fail("Subscription unexpectedly succeeded")


def test_subscription_precedes_lease_deletion(server, state_events):
    active = [False]
    original = server[3].side_effect

    @contextmanager
    def subscribe():
        active[0] = True
        try:
            yield state_events
        finally:
            active[0] = False

    def command(name, arguments=None):
        if name == "lease4-del":
            assert active[0]
        return original(name, arguments)

    server[3].side_effect = command
    with mock.patch.object(clear_dhcp_server, "_lease_state_events", side_effect=subscribe):
        result = invoke(server, "Vlan100", "100.1.1.3")

    assert result.exit_code == 0, result.output
    assert not active[0]


def test_subscription_failure_cannot_delete(server):
    with mock.patch.object(clear_dhcp_server, "_lease_state_events",
                           side_effect=click.ClickException("Cannot monitor DHCP lease state")):
        result = invoke(server, "Vlan100", "100.1.1.3")

    assert result.exit_code == 1
    assert "Cannot monitor DHCP lease state" in result.output
    assert not any(call.args[0] in ("lease4-del", "lease4-wipe") for call in server[3].call_args_list)


def test_ip_delete_failure_does_not_request_state_refresh(server):
    original = server[3].side_effect

    def fail_delete(name, arguments=None):
        if name == "lease4-del":
            raise click.ClickException("simulated delete failure")
        return original(name, arguments)

    server[3].side_effect = fail_delete
    result = invoke(server, "Vlan100", "100.1.1.3")

    assert result.exit_code == 1
    assert "simulated delete failure" in result.output
    server[4].assert_not_called()
    assert any(entry["subnet-id"] == 100 and entry["hw-address"] == MAC_A for entry in server[1])


def test_clear_does_not_depend_on_cached_lease(server):
    """An absent State DB row must not prevent deleting a real Kea lease."""
    server[0].db.delete("STATE_DB", "DHCP_SERVER_IPV4_LEASE|Vlan100|" + MAC_A)
    result = invoke(server, "Vlan100", "100.1.1.3")
    assert result.exit_code == 0, result.output
    assert not any(entry["ip-address"] == "100.1.1.3" for entry in server[1])


def test_no_server_lease_refreshes_stale_cache_but_returns_failure(server):
    """Clean an orphaned cache entry without claiming a server lease was deleted."""
    server[1][:] = [entry for entry in server[1] if entry["ip-address"] != "100.1.1.3"]
    result = invoke(server, "Vlan100", "100.1.1.3")
    assert result.exit_code == 1
    assert "No matching Kea lease" in result.output
    assert not server[0].db.exists("STATE_DB", "DHCP_SERVER_IPV4_LEASE|Vlan100|" + MAC_A)


def test_empty_interface_all_is_failure(server):
    """An empty interface must not become a successful global wipe."""
    server[1][:] = [entry for entry in server[1] if entry["subnet-id"] != 100]
    result = invoke(server, "Vlan100", "--all")
    assert result.exit_code == 1
    assert not any(call.args[0] == "lease4-wipe" for call in server[3].call_args_list)


def test_synchronizer_failure_is_not_success(server):
    """Report failure if the daemon cannot be notified after server deletion."""
    server[4].side_effect = click.ClickException("dhcpservd is unavailable")
    result = invoke(server, "Vlan100", "100.1.1.3")
    assert result.exit_code == 1
    assert "dhcpservd is unavailable" in result.output


def test_malformed_lease_list_cannot_delete(server):
    """Reject an incomplete lease response before executing destructive commands."""
    original = server[3].side_effect

    def malformed(name, arguments=None):
        if name == "lease4-get-all":
            return {"result": 0, "arguments": {"leases": [{"ip-address": "100.1.1.3"}]}}
        return original(name, arguments)

    server[3].side_effect = malformed
    result = invoke(server, "Vlan100", "--all")
    assert result.exit_code == 1
    assert "Invalid lease list" in result.output
    assert not any(call.args[0] == "lease4-wipe" for call in server[3].call_args_list)


@pytest.mark.parametrize("output,message", [
    ("not JSON", "Invalid JSON"),
    ("[]", "Invalid command response"),
    ('{"result": false}', "Invalid command response"),
    ('{"result": 1, "text": "unsupported command"}', "unsupported command"),
])
def test_kea_protocol_errors_are_explicit(output, message):
    """Malformed or unsuccessful replies must never look like a successful clear."""
    with mock.patch.object(clear_dhcp_server, "_container_command", return_value=output):
        with pytest.raises(click.ClickException, match=message):
            clear_dhcp_server._kea_command("lease4-get-all", {"subnets": [100]})


@pytest.mark.parametrize("error", [
    FileNotFoundError("docker missing"),
    subprocess.TimeoutExpired("docker", 15),
    subprocess.CalledProcessError(1, "docker", stderr="container unavailable"),
])
def test_container_failure_is_explicit(error):
    """Propagate missing, stopped and unresponsive runtime failures."""
    with mock.patch.object(clear_dhcp_server.subprocess, "run", side_effect=error):
        with pytest.raises(click.ClickException):
            clear_dhcp_server._container_command("python3", "-c", "pass")


def test_kea_transport_uses_argument_vector():
    """Pass JSON as data, not an interpolated shell command."""
    result = subprocess.CompletedProcess([], 0, stdout='{"result": 3}', stderr="")
    with mock.patch.object(clear_dhcp_server.subprocess, "run", return_value=result) as run:
        assert clear_dhcp_server._kea_command("lease4-get-all", {"subnets": [100]}) == {"result": 3}
    arguments = run.call_args.args[0]
    assert arguments[:5] == ["docker", "exec", "dhcp_server", "python3", "-c"]
    assert json.loads(arguments[-1]) == {
        "command": "lease4-get-all", "arguments": {"subnets": [100]}}
    assert "shell" not in run.call_args.kwargs


def test_plugin_registration():
    """Expose the formerly unregistered clear command through the plugin loader."""
    cli = mock.Mock()
    clear_dhcp_server.register(cli)
    cli.add_command.assert_called_once_with(clear_dhcp_server.dhcp_server)


def test_parallel_clear_is_rejected(tmp_path):
    """Serialize clear operations instead of racing their lease/cache snapshots."""
    with mock.patch.object(clear_dhcp_server, "LEASE_CLEAR_LOCK", str(tmp_path / "lease.lock")):
        with clear_dhcp_server._lease_clear_lock():
            with pytest.raises(click.ClickException, match="in progress"):
                with clear_dhcp_server._lease_clear_lock():
                    pytest.fail("A second clear acquired the lock")


def test_control_socket_reads_fragmented_utf8_json(tmp_path):
    """Read a complete response even when a socket splits JSON and UTF-8 bytes."""
    path = str(tmp_path / "kea.sock")
    expected = {"result": 0, "text": "lease \u00e9"}
    payload = json.dumps(expected, ensure_ascii=False).encode("utf-8")
    split = payload.index(b"\xc3") + 1
    requests = []
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as listener:
        listener.bind(path)
        listener.listen(1)
        listener.settimeout(5)

        def respond():
            with listener.accept()[0] as connection:
                connection.settimeout(5)
                requests.append(json.loads(connection.recv(4096)))
                connection.sendall(payload[:split])
                time.sleep(0.02)
                connection.sendall(payload[split:])

        thread = threading.Thread(target=respond)
        thread.start()
        try:
            result = subprocess.run(
                [sys.executable, "-c", clear_dhcp_server.KEA_SOCKET_CLIENT, path,
                 json.dumps({"command": "list-commands"})],
                capture_output=True, text=True, check=True, timeout=5)
        finally:
            thread.join(timeout=5)
    assert not thread.is_alive()
    assert json.loads(result.stdout) == expected
    assert requests == [{"command": "list-commands"}]
