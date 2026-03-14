import click
import pytest
import os
import copy
import json
import signal
import fcntl
import importlib.util
import psutil
import re
import utilities_common.cli as clicommon

from click.testing import CliRunner

from unittest.mock import patch, call, MagicMock, ANY
original_import_module = importlib.import_module


@pytest.fixture(scope="module")
def patch_import_module():
    # We need to mock import module because clear_dhcp_relay.py has below import
    # dhcprelay = importlib.import_module('show.plugins.dhcp-relay')
    # When install current container, sonic-application-extension would move below file to destination in switch
    # Src: dockers/docker-dhcp-relay/cli/show/plugins/show_dhcp_relay.py
    # Dst: python-package-patch/show/plugins/dhcp-relay.py
    # The dst path doesn't exist in UT env, hence we need to mock it
    fake_dhcprelay = MagicMock()

    with patch('importlib.import_module') as mock_import:
        def side_effect(name):
            if name == 'show.plugins.dhcp-relay':
                return fake_dhcprelay
            return original_import_module(name)  # fallback

        mock_import.side_effect = side_effect

        clear_dhcp_relay_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "cli", "clear", "plugins",
                                                             "clear_dhcp_relay.py"))
        spec = importlib.util.spec_from_file_location("clear_dhcp_relay", clear_dhcp_relay_path)
        clear_dhcp_relay = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(clear_dhcp_relay)
        yield clear_dhcp_relay


def test_plugin_registration(patch_import_module):
    cli = MagicMock()
    clear_dhcp_relay = patch_import_module
    clear_dhcp_relay.register(cli)


@pytest.mark.parametrize("interface", [None, "Vlan1000"])
def test_clear_dhcp_relay_ipv6_counter(interface, patch_import_module):
    clear_dhcp_relay = patch_import_module
    gotten_interfaces = ["Ethernet0, Ethernet1"]

    mock_counter = MagicMock()
    clear_dhcp_relay.dhcprelay.DHCPv6_Counter.return_value = mock_counter
    mock_counter.get_interface.return_value = gotten_interfaces
    clear_dhcp_relay.clear_dhcp_relay_ipv6_counter(interface)
    if interface:
        mock_counter.clear_table.assert_called_once_with(interface)
    else:
        calls = [call(intf) for intf in gotten_interfaces]
        mock_counter.clear_table.assert_has_calls(calls)


@pytest.mark.parametrize("test_param_result", [
    ["", True, True],
    ["Vlan1000", True, True],
    ["Vlan1000", False, False]
])
def test_is_vlan_interface_valid(patch_import_module, test_param_result):
    clear_dhcp_relay = patch_import_module
    mock_db = MagicMock()
    vlan_interface = test_param_result[0]
    mock_db.exists.return_value = test_param_result[1]
    exp_res = test_param_result[2]
    act_res = clear_dhcp_relay.is_vlan_interface_valid(vlan_interface, mock_db)
    assert act_res == exp_res, ("Result {} is unexpected with [vlan_interface: {}, exists: {}]"
                                .format(act_res, vlan_interface, test_param_result[1]))


@pytest.mark.parametrize("direction, type, keys, db_dhcp_type, paused_vlans", [
    (None, None, ["DHCPV4_COUNTER_TABLE:Vlan100:Ethernet1", "DHCPV4_COUNTER_TABLE_Vlan100",
                             "DHCPV4_COUNTER_TABLE_Vlan1000"], set(["Discover"]), set(["Vlan100"])),
    ("RX", "Offer", ["DHCPV4_COUNTER_TABLE:Vlan100:Ethernet1", "DHCPV4_COUNTER_TABLE_Vlan100",
                                "DHCPV4_COUNTER_TABLE_Vlan1000"], set(["Discover", "Offer"]), set(["Vlan100"])),
    ("TX", "Ack", ["DHCPV4_COUNTER_TABLE:Vlan100:Ethernet1", "DHCPV4_COUNTER_TABLE_Vlan100",
                              "DHCPV4_COUNTER_TABLE_Vlan1000"], set(["Discover", "Offer"]), set(["Vlan100"])),
    ("TX", "Ack", ["DHCPV4_COUNTER_TABLE:Vlan100:Ethernet1", "DHCPV4_COUNTER_TABLE_Vlan100",
                              "DHCPV4_COUNTER_TABLE_Vlan1000"], None, set(["Vlan100"]))
])
def test_clear_dhcpv4_db_counters(patch_import_module, direction, type, keys,
                                  db_dhcp_type, paused_vlans):
    clear_dhcp_relay = patch_import_module
    mock_db = MagicMock()
    directions = [direction] if direction else clear_dhcp_relay.SUPPORTED_DIR
    types = [type] if type else clear_dhcp_relay.SUPPORTED_DHCP_TYPE
    mock_db.keys.return_value = keys
    count_obj = {}
    if db_dhcp_type is not None:
        for current_type in db_dhcp_type:
            if current_type not in clear_dhcp_relay.SUPPORTED_DHCP_TYPE:
                continue
            # Set it to non zero before clearing
            count_obj[current_type] = "1"
        mock_db.get.return_value = json.dumps(count_obj).replace("\"", "\'")
    else:
        mock_db.get.return_value = None
    with patch.object(clear_dhcp_relay, "is_vlan_interface_valid", return_value=True):
        clear_dhcp_relay.clear_dhcpv4_db_counters(mock_db, direction, type, paused_vlans)
        calls = []
        for vlan_interface in paused_vlans:
            for key in keys:
                if not (":{}:".format(vlan_interface) in key or key.endswith(vlan_interface)):
                    continue
                for curr_dir in directions:
                    current_count_obj = copy.deepcopy(count_obj)
                    for curr_type in types:
                        current_count_obj[curr_type] = "0"
                # Expect set count to zero
                calls.append(call(ANY, key, curr_dir, json.dumps(current_count_obj).replace("\"", "\'")))
        mock_db.set.assert_has_calls(calls, any_order=True)


@pytest.mark.parametrize("sig, vlan_interface, procs", [
    (signal.SIGUSR1, "Vlan1000", []),
    (signal.SIGUSR2, "", [["dhcpmon", "5", "", ""]]),
    (signal.SIGUSR2, "", [["dhcrelay", "5", "dhcpmon -id Vlan1000", ""]]),
    (signal.SIGUSR2, "Vlan2000", [["dhcrelay", "5", "dhcpmon -id Vlan1000", ""]]),
    (100, "Vlan1000", [])
])
def test_notify_dhcpmon_processes(patch_import_module, sig, vlan_interface, procs):
    clear_dhcp_relay = patch_import_module
    with patch.object(click, "echo") as mock_echo, \
         patch.object(psutil, "process_iter") as mock_iterator, \
         patch.object(os, "kill") as mock_kill:
        mock_procs = []
        for proc in procs:
            mock_proc = MagicMock()
            mock_proc.name.return_value = proc[0]
            mock_proc.pid = proc[1]
            mock_proc.cmdline.return_value = proc[2]
            mock_procs.append(mock_proc)
        mock_iterator.return_value = mock_procs
        clear_dhcp_relay.notify_dhcpmon_processes(vlan_interface, sig)
        try:
            signal.Signals(sig)
        except ValueError:
            mock_echo.assert_called_once_with("Invalid signal, skip it")
            mock_iterator.assert_not_called()
            mock_kill.assert_not_called()
            return
        mock_kill_calls = []
        expected_notified_vlans = set()
        for proc in procs:
            if proc[3]:
                mock_kill_calls.append(call(int(proc[1]), sig))
                expected_notified_vlans.add(proc[3])
        if mock_kill_calls:
            mock_kill.assert_has_calls(mock_kill_calls)
        else:
            mock_kill.assert_not_called()


@pytest.mark.parametrize("interface", ["Vlan1000", None])
def test_clear_dhcp6relay_clear_counters(patch_import_module, interface):
    clear_dhcp_relay = patch_import_module
    runner = CliRunner()
    with patch.object(clear_dhcp_relay, "clear_dhcp_relay_ipv6_counter") as mock_clear:
        if interface:
            result = runner.invoke(clear_dhcp_relay.dhcp6relay_clear.commands["dhcp6relay_counters"],
                                   ["--interface={}".format(interface)])
        else:
            result = runner.invoke(clear_dhcp_relay.dhcp6relay_clear.commands["dhcp6relay_counters"])
        assert result.exit_code == 0
        mock_clear.assert_called_once_with(interface)


@pytest.mark.parametrize("interface", ["Vlan1000", None])
def test_clear_dhcp_relay_ipv6_counters(patch_import_module, interface):
    clear_dhcp_relay = patch_import_module
    runner = CliRunner()
    args = ["--interface={}".format(interface)] if interface else []
    with patch.object(clear_dhcp_relay, "clear_dhcp_relay_ipv6_counter") as mock_clear:
        result = runner.invoke(clear_dhcp_relay.dhcp_relay.commands["ipv6"].commands["counters"], args)
        assert result.exit_code == 0
        mock_clear.assert_called_once_with(interface)


def test_clear_dhcp_relay_ipv4_counters_no_root(patch_import_module):
    clear_dhcp_relay = patch_import_module
    runner = CliRunner()
    with patch.object(os, "geteuid", return_value=1), \
         patch.object(click, "get_current_context") as mock_get_ctx:
        mock_ctx = MagicMock()
        mock_get_ctx.return_value = mock_ctx
        result = runner.invoke(clear_dhcp_relay.dhcp_relay.commands["ipv4"].commands["counters"])
        assert result.exit_code == 0
        mock_ctx.fail.assert_called_once_with("Clear DHCPv4 counter need to be run with sudo permission")


def test_clear_dhcp_relay_ipv4_counters_locked(patch_import_module):
    clear_dhcp_relay = patch_import_module
    runner = CliRunner()
    with patch.object(os, "geteuid", return_value=0), \
         patch.object(clear_dhcp_relay, "open"), \
         patch.object(fcntl, "flock", side_effect=BlockingIOError) as mock_flock, \
         patch.object(clear_dhcp_relay, "notify_dhcpmon_processes"), \
         patch.object(clear_dhcp_relay, "clear_dhcpv4_db_counters"), \
         patch.object(click, "get_current_context") as mock_get_ctx:
        mock_ctx = MagicMock()
        mock_get_ctx.return_value = mock_ctx
        result = runner.invoke(clear_dhcp_relay.dhcp_relay.commands["ipv4"].commands["counters"])
        assert result.exit_code != 0
        mock_ctx.fail.assert_called_once_with("Cannot lock {}".format(clear_dhcp_relay.DHCPV4_CLEAR_COUNTER_LOCK_FILE) + 
                                              ", seems another user is clearing DHCPv4 relay counter simultaneous")
        mock_flock.assert_has_calls([
            call(ANY, fcntl.LOCK_EX | fcntl.LOCK_NB),
            call(ANY, fcntl.LOCK_UN)
        ])


def test_clear_dhcp_relay_ipv4_counters_vlan_invalid(patch_import_module):
    clear_dhcp_relay = patch_import_module
    runner = CliRunner()
    with patch.object(os, "geteuid", return_value=0), \
         patch.object(clear_dhcp_relay, "is_vlan_interface_valid", return_value=False), \
         patch.object(click, "get_current_context") as mock_get_ctx:
        mock_ctx = MagicMock()
        mock_get_ctx.return_value = mock_ctx
        result = runner.invoke(clear_dhcp_relay.dhcp_relay.commands["ipv4"].commands["counters"], "--interface=Vlan1000")
        assert result.exit_code == 0
        mock_ctx.fail.assert_called_once_with("Vlan1000 doesn't exist")


@pytest.mark.parametrize("interface", ["", "Vlan1000"])
@pytest.mark.parametrize("dir", [None, "RX", "TX"])
@pytest.mark.parametrize("type", [None, "Discover"])
@pytest.mark.parametrize("failed_vlans", [{}, {"Vlan1000": set(["RX", "TX"])}])
def test_clear_dhcp_relay_ipv4_counters(patch_import_module, interface, dir, type, failed_vlans):
    clear_dhcp_relay = patch_import_module
    runner = CliRunner()
    args = []
    if interface != "":
        args.append("--interface={}".format(interface))
    if dir is not None:
        args.append("--dir={}".format(dir))
    if type is not None:
        args.append("--type={}".format(type))
    paused_vlans = set()
    with patch.object(os, "geteuid", return_value=0), \
         patch.object(clear_dhcp_relay, "open"), \
         patch.object(fcntl, "flock") as mock_flock, \
         patch.object(clear_dhcp_relay, "notify_dhcpmon_processes", return_value=set()) as mock_notify, \
         patch.object(clear_dhcp_relay, "get_paused_vlans", return_value=paused_vlans), \
         patch.object(clear_dhcp_relay, "clear_dhcpv4_db_counters") as mock_clear, \
         patch.object(clear_dhcp_relay, "is_vlan_interface_valid", return_value=True), \
         patch.object(clear_dhcp_relay, "get_failed_vlans", return_value=failed_vlans), \
         patch.object(clear_dhcp_relay, "clear_writing_state_db_flag") as mock_clear_writing_state_db_flag, \
         patch.object(click, "get_current_context") as mock_get_ctx:
        mock_ctx = MagicMock()
        mock_get_ctx.return_value = mock_ctx
        result = runner.invoke(clear_dhcp_relay.dhcp_relay.commands["ipv4"].commands["counters"], args)
        assert result.exit_code == 0
        mock_flock.assert_has_calls([
            call(ANY, fcntl.LOCK_EX | fcntl.LOCK_NB),
            call(ANY, fcntl.LOCK_UN)
        ])
        mock_notify.assert_has_calls([
            call(interface, signal.SIGUSR1),
            call(interface, signal.SIGUSR2)
        ])
        mock_clear.assert_called_once_with(ANY, dir, type, paused_vlans)
        if failed_vlans:
            mock_ctx.fail.assert_called_once()
        mock_clear_writing_state_db_flag.assert_has_calls([
            call(ANY),
            call(ANY)
        ])


@pytest.mark.parametrize("mock_get_result", ["done", ""])
def test_get_failed_vlans(patch_import_module, mock_get_result):
    clear_dhcp_relay = patch_import_module
    mock_db = MagicMock()
    mock_db.get.return_value = mock_get_result
    failed_vlans = clear_dhcp_relay.get_failed_vlans(mock_db, set(["Vlan1000", "Vlan2000"]))
    if mock_get_result == "done":
        assert failed_vlans == {}
    else:
        assert failed_vlans == {"Vlan1000": set(["RX", "TX"]), "Vlan2000": set(["RX", "TX"])}


def test_clear_writing_state_db_flag(patch_import_module):
    clear_dhcp_relay = patch_import_module
    mock_db = MagicMock()
    keys = ["key1", "key2", "key3"]
    mock_db.keys.return_value = keys
    clear_dhcp_relay.clear_writing_state_db_flag(mock_db)
    mock_db.delete.assert_has_calls([call(ANY, key) for key in keys])


@pytest.mark.parametrize("is_write_db_paused, vlans, result", [
    (True, set(["Vlan1000", "Vlan2000"]), set(["Vlan1000", "Vlan2000"])),
    (False,set( ["Vlan1000", "Vlan2000"]), set()),
    (True, set(), set()),
    (False, set(), set())
])
def test_get_paused_vlans(patch_import_module, is_write_db_paused, vlans, result):
    clear_dhcp_relay = patch_import_module
    with patch.object(clear_dhcp_relay, "is_write_db_paused", return_value=is_write_db_paused):
        paused_vlans = clear_dhcp_relay.get_paused_vlans(MagicMock(), "table_name", vlans)
    assert paused_vlans == result


def test_is_write_db_paused(patch_import_module):
    clear_dhcp_relay = patch_import_module
    mock_db = MagicMock()
    clear_dhcp_relay.is_write_db_paused(mock_db, "table_name", "vlan_interface")
    mock_db.get.assert_called_once_with(ANY, "table_name|vlan_interface", "pause_write_to_db")

@pytest.mark.parametrize("direction, pkt_type, interface", [
    (None, None, None),
    ("TX", "DISCOVER", "Vlan1000"),
    ("RX", "OFFER", None),
])
def test_clear_dhcp_relay_ipv4_vlan_counter(patch_import_module, direction, pkt_type, interface):
    clear_dhcp_relay = patch_import_module
    mock_counter = MagicMock()
    clear_dhcp_relay.dhcprelay.DHCPv4_Counter.return_value = mock_counter
    clear_dhcp_relay.clear_dhcp_relay_ipv4_vlan_counter(direction, pkt_type, interface)
    mock_counter.clear_table.assert_called_once_with(direction, pkt_type, interface)

def test_dhcp4relay_clear_vlan_counters_command(patch_import_module):
    clear_dhcp_relay = patch_import_module
    runner = CliRunner()
    # Patch the actual clear function to verify call
    with patch.object(clear_dhcp_relay, "clear_dhcp_relay_ipv4_vlan_counter") as mock_clear:
        result = runner.invoke(clear_dhcp_relay.dhcp4relay_clear.commands['dhcp4relay-vlan-counters'], ['-d', 'TX', 'Vlan1000'])
        assert result.exit_code == 0

def test_register_adds_dhcp4relay_clear_vlan_counters(patch_import_module):
    clear_dhcp_relay = patch_import_module
    cli = MagicMock()
    clear_dhcp_relay.register(cli)
    # Check that dhcp4relay_clear_vlan_counters was registered as a command
    cli.add_command.assert_any_call(clear_dhcp_relay.dhcp4relay_clear_vlan_counters)
