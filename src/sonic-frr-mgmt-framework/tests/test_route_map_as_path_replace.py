"""
Unit tests for route-map set as-path replace configuration (NOS-6716).

Tests the frrcfgd handler for translating CONFIG_DB set_as_path_replace
to FRR vtysh commands.
"""

import pytest
from unittest.mock import MagicMock, NonCallableMagicMock, patch

swsscommon_module_mock = MagicMock(ConfigDBConnector=NonCallableMagicMock)
bgpcfgd_managers_bfd_mock = MagicMock()
bgpcfgd_directory_mock = MagicMock()
bgpcfgd_log_mock = MagicMock()
bgpcfgd_utils_mock = MagicMock()

mockmapping = {
    'swsscommon.swsscommon': swsscommon_module_mock,
    'bgpcfgd': MagicMock(),
    'bgpcfgd.managers_bfd': bgpcfgd_managers_bfd_mock,
    'bgpcfgd.directory': bgpcfgd_directory_mock,
    'bgpcfgd.log': bgpcfgd_log_mock,
    'bgpcfgd.utils': bgpcfgd_utils_mock,
}


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_route_map_as_path_replace_specific_asn(run_cmd):
    """Test set as-path replace with a specific old and new ASN."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.route_map = {'replace_as_path': {'100': 'permit'}}

    table = 'ROUTE_MAP'
    key = 'replace_as_path|100'
    data = {
        'route_operation': 'permit',
        'set_as_path_replace': '40001 44444',
    }

    run_cmd.reset_mock()

    hdlr = [h for t, h in daemon.table_handler_list if t == table]
    assert len(hdlr) == 1
    hdlr[0](table, key, data)

    expected_cmd = (
        "vtysh -c 'configure terminal' "
        "-c 'route-map replace_as_path permit 100' "
        "-c 'set as-path replace 40001 44444'"
    )
    calls = [call[0][1] for call in run_cmd.call_args_list]
    assert any(expected_cmd in call for call in calls), \
        f"Expected command not found. Calls: {calls}"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_route_map_as_path_replace_any(run_cmd):
    """Test set as-path replace with 'any' to replace all ASNs."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.route_map = {'replace_as_path': {'100': 'permit'}}

    table = 'ROUTE_MAP'
    key = 'replace_as_path|100'
    data = {
        'route_operation': 'permit',
        'set_as_path_replace': 'any 44444',
    }

    run_cmd.reset_mock()

    hdlr = [h for t, h in daemon.table_handler_list if t == table]
    assert len(hdlr) == 1
    hdlr[0](table, key, data)

    expected_cmd = (
        "vtysh -c 'configure terminal' "
        "-c 'route-map replace_as_path permit 100' "
        "-c 'set as-path replace any 44444'"
    )
    calls = [call[0][1] for call in run_cmd.call_args_list]
    assert any(expected_cmd in call for call in calls), \
        f"Expected command not found. Calls: {calls}"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_route_map_as_path_replace_with_other_set_actions(run_cmd):
    """Test set as-path replace combined with other set actions (e.g. ipv6 next-hop prefer-global)."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.route_map = {'replace_as_path': {'100': 'permit'}}

    table = 'ROUTE_MAP'
    key = 'replace_as_path|100'
    data = {
        'route_operation': 'permit',
        'set_as_path_replace': '40001 44444',
        'set_ipv6_next_hop_prefer_global': 'true',
    }

    run_cmd.reset_mock()

    hdlr = [h for t, h in daemon.table_handler_list if t == table]
    assert len(hdlr) == 1
    hdlr[0](table, key, data)

    calls = [call[0][1] for call in run_cmd.call_args_list]
    combined = ' '.join(calls)
    assert 'set as-path replace 40001 44444' in combined, \
        f"Expected as-path replace command not found. Calls: {calls}"
    assert 'set ipv6 next-hop prefer-global' in combined, \
        f"Expected ipv6 next-hop prefer-global command not found. Calls: {calls}"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_route_map_as_path_replace_delete(run_cmd):
    """Test deletion of set as-path replace — default handler emits 'no set as-path replace <value>'."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.route_map = {'replace_as_path': {'100': 'permit'}}

    table = 'ROUTE_MAP'
    key = 'replace_as_path|100'
    data = {
        'route_operation': 'permit',
        'set_as_path_replace': '40001 44444',
    }

    hdlr = [h for t, h in daemon.table_handler_list if t == table]
    assert len(hdlr) == 1
    hdlr[0](table, key, data)

    run_cmd.reset_mock()

    # Delete the route-map entry
    hdlr[0](table, key, {})

    expected_cmd = (
        "vtysh -c 'configure terminal' "
        "-c 'no route-map replace_as_path permit 100'"
    )
    calls = [call[0][1] for call in run_cmd.call_args_list]
    assert any(expected_cmd in call for call in calls), \
        f"Expected delete command not found. Calls: {calls}"
