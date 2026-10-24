"""
Unit tests for SRV6_GLOBAL encapsulation source address configuration.

Tests the frrcfgd handler for translating CONFIG_DB SRV6_GLOBAL
to FRR vtysh commands.
"""

from unittest.mock import MagicMock, NonCallableMagicMock, patch

swsscommon_module_mock = MagicMock(ConfigDBConnector=NonCallableMagicMock)

mockmapping = {
    'swsscommon.swsscommon': swsscommon_module_mock,
    'bgpcfgd': MagicMock(),
    'bgpcfgd.managers_bfd': MagicMock(),
    'bgpcfgd.directory': MagicMock(),
    'bgpcfgd.log': MagicMock(),
    'bgpcfgd.utils': MagicMock(),
}

ENCAP_CMD_PREFIX = [
    'vtysh',
    '-c', 'configure terminal',
    '-c', 'segment-routing',
    '-c', 'srv6',
    '-c', 'encapsulation',
]


def get_handler(daemon):
    hdlr = [h for t, h in daemon.table_handler_list if t == 'SRV6_GLOBAL']
    assert len(hdlr) == 1
    return hdlr[0]


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_srv6_global_set_encap_source_address(run_cmd):
    """Test setting the global SRv6 encapsulation source address."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    run_cmd.reset_mock()

    get_handler(daemon)('SRV6_GLOBAL', 'default', {'encap_source_address': 'fc00:1::1'})

    calls = [call[0][1] for call in run_cmd.call_args_list]
    assert ENCAP_CMD_PREFIX + ['-c', 'source-address fc00:1::1'] in calls, \
        f"Expected command not found. Calls: {calls}"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_srv6_global_delete_encap_source_address(run_cmd):
    """Test deleting the SRV6_GLOBAL entry removes the encapsulation source address."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    hdlr = get_handler(daemon)
    hdlr('SRV6_GLOBAL', 'default', {'encap_source_address': 'fc00:1::1'})
    run_cmd.reset_mock()

    hdlr('SRV6_GLOBAL', 'default', None)

    calls = [call[0][1] for call in run_cmd.call_args_list]
    assert ENCAP_CMD_PREFIX + ['-c', 'no source-address'] in calls, \
        f"Expected delete command not found. Calls: {calls}"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_srv6_global_invalid_key(run_cmd):
    """Test that keys other than 'default' are rejected."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    run_cmd.reset_mock()

    get_handler(daemon)('SRV6_GLOBAL', 'other', {'encap_source_address': 'fc00:1::1'})

    calls = [call[0][1] for call in run_cmd.call_args_list]
    assert not any('source-address fc00:1::1' in call for call in calls), \
        f"Unexpected command for invalid key. Calls: {calls}"
