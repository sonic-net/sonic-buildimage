"""
Route-maps and prefix-lists reach zebra through mgmtd.

zebra's route-maps and prefix-lists are mgmtd-owned: zebra's own vty has no
route-map or prefix-list commands, so a line sent to it is refused, and bgpd
accepting the same line hides that. A protocol route-map naming a CONFIG_DB
route-map then found nothing in zebra and denied every route of the protocol.
These tests pin the daemon each command is sent to, and cover the set_src
leaf that lets a CONFIG_DB route-map carry zebra's 'set src'.
"""

import os
import threading

import pytest
from jinja2 import Environment, FileSystemLoader
from unittest.mock import MagicMock, NonCallableMagicMock, patch

# match_protocol value -> FRR keyword. FRR calls OSPFv3 'ospf6'.
MATCH_PROTOCOLS = [
    ('connected', 'connected'),
    ('static', 'static'),
    ('bgp', 'bgp'),
    ('ospf', 'ospf'),
    ('ospf3', 'ospf6'),
]

swsscommon_module_mock = MagicMock(ConfigDBConnector=NonCallableMagicMock)

mockmapping = {'swsscommon.swsscommon': swsscommon_module_mock}


def _vtysh_lines(command):
    """The -c arguments of an argv-list vtysh command."""
    return [command[i + 1] for i in range(len(command) - 1) if command[i] == '-c']


def _daemons_seen(table, command):
    """Drive run_vtysh_command and return (line, daemons) for every line sent."""
    from frrcfgd.frrcfgd import BgpdClientMgr

    # __init__ opens sockets to FRR, so build the instance without it and
    # supply only what run_vtysh_command touches.
    mgr = BgpdClientMgr.__new__(BgpdClientMgr)
    mgr.lock = threading.Lock()
    seen = []
    with patch.object(BgpdClientMgr, '_BgpdClientMgr__proc_command',
                      side_effect=lambda cmd, daemons: (seen.append((cmd, daemons)), (True, ''))[1]):
        assert mgr.run_vtysh_command(table, command, None)
    return seen


@patch.dict('sys.modules', **mockmapping)
def test_route_map_commands_resolve_to_mgmtd():
    """A ROUTE_MAP row's lines go to mgmtd, which owns zebra's route-maps,
    and still to bgpd and ospfd; never to zebra itself. mgmtd is last, so a
    failed send to it cannot stop the line from reaching bgpd and ospfd."""
    seen = _daemons_seen('ROUTE_MAP', ['vtysh', '-c', 'configure terminal',
                                       '-c', 'route-map RM_TEST permit 10'])
    assert ('route-map RM_TEST permit 10', ['bgpd', 'ospfd', 'mgmtd']) in seen
    assert not [d for _, d in seen if 'zebra' in d]


@patch.dict('sys.modules', **mockmapping)
def test_prefix_commands_resolve_to_mgmtd():
    """A PREFIX row's lines go to mgmtd, which owns zebra's prefix-lists,
    last in the list for the same reason as ROUTE_MAP."""
    seen = _daemons_seen('PREFIX', ['vtysh', '-c', 'configure terminal',
                                    '-c', 'ip prefix-list PL seq 10 permit 10.0.0.0/8'])
    assert ('ip prefix-list PL seq 10 permit 10.0.0.0/8',
            ['bgpd', 'ospfd', 'pimd', 'mgmtd']) in seen
    assert not [d for _, d in seen if 'zebra' in d]


@patch.dict('sys.modules', **mockmapping)
def test_protocol_route_map_resolves_to_mgmtd():
    """'ip protocol ... route-map' is zebra config, reached through mgmtd."""
    seen = _daemons_seen('PROTOCOL_ROUTE_MAP', ['vtysh', '-c', 'configure terminal',
                                                '-c', 'ip protocol bgp route-map RM_TEST'])
    assert ('ip protocol bgp route-map RM_TEST', ['mgmtd']) in seen


@patch.dict('sys.modules', **mockmapping)
def test_table_daemons_are_connected_daemons():
    """Every daemon a table's lines go to is one frrcfgd connects to, so no
    line is logged as sent to a daemon that is not connected."""
    from frrcfgd.frrcfgd import BgpdClientMgr

    for table in ('ROUTE_MAP', 'PREFIX', 'PROTOCOL_ROUTE_MAP'):
        assert set(BgpdClientMgr.TABLE_DAEMON[table]) <= set(BgpdClientMgr.ALL_DAEMONS), table


def _route_map_daemon(run_cmd, map_name='RM_TEST', seq='10', operation='permit'):
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.route_map = {map_name: {seq: operation}}
    return daemon


def _feed(daemon, run_cmd, table, key, data):
    """Drive one table handler and return (vtysh lines, daemons) per call."""
    run_cmd.reset_mock()
    hdlr = [h for t, h in daemon.table_handler_list if t == table]
    assert len(hdlr) == 1
    hdlr[0](table, key, data)
    return [(_vtysh_lines(c[0][1]), c[0][3]) for c in run_cmd.call_args_list]


def _daemons_for(calls, line):
    found = [daemons for lines, daemons in calls if line in lines]
    assert found, 'no call sent %r: %s' % (line, calls)
    return found[0]


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_set_src_is_sent_to_mgmtd(run_cmd):
    """set_src renders zebra's 'set src' inside the route-map, sent to mgmtd only:
    neither bgpd nor ospfd has the command."""
    daemon = _route_map_daemon(run_cmd)
    calls = _feed(daemon, run_cmd, 'ROUTE_MAP', 'RM_TEST|10',
                  {'route_operation': 'permit', 'set_src': '10.1.0.32'})
    assert ['configure terminal', 'route-map RM_TEST permit 10', 'set src 10.1.0.32'] in \
        [lines for lines, _ in calls]
    assert _daemons_for(calls, 'set src 10.1.0.32') == ['mgmtd']


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_set_src_ipv6(run_cmd):
    daemon = _route_map_daemon(run_cmd)
    calls = _feed(daemon, run_cmd, 'ROUTE_MAP', 'RM_TEST|10',
                  {'route_operation': 'permit', 'set_src': 'fc00:1::32'})
    assert _daemons_for(calls, 'set src fc00:1::32') == ['mgmtd']


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_set_src_removed_emits_no_form(run_cmd):
    daemon = _route_map_daemon(run_cmd)
    _feed(daemon, run_cmd, 'ROUTE_MAP', 'RM_TEST|10',
          {'route_operation': 'permit', 'set_src': '10.1.0.32'})
    calls = _feed(daemon, run_cmd, 'ROUTE_MAP', 'RM_TEST|10', {'route_operation': 'permit'})
    assert _daemons_for(calls, 'no set src 10.1.0.32') == ['mgmtd']


@pytest.mark.parametrize('proto, keyword', MATCH_PROTOCOLS)
@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_match_source_protocol_is_sent_to_bgpd_and_mgmtd(run_cmd, proto, keyword):
    """'match source-protocol' goes to mgmtd for zebra, and to bgpd, which has
    its own copy of the route-map: without the line there, a live edit leaves
    bgpd's copy matching every route until the next reboot."""
    daemon = _route_map_daemon(run_cmd)
    calls = _feed(daemon, run_cmd, 'ROUTE_MAP', 'RM_TEST|10',
                  {'route_operation': 'permit', 'match_protocol': proto})
    assert _daemons_for(calls, 'match source-protocol ' + keyword) == ['bgpd', 'mgmtd']


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_match_source_protocol_removed_from_bgpd_and_mgmtd(run_cmd):
    daemon = _route_map_daemon(run_cmd)
    _feed(daemon, run_cmd, 'ROUTE_MAP', 'RM_TEST|10',
          {'route_operation': 'permit', 'match_protocol': 'connected'})
    calls = _feed(daemon, run_cmd, 'ROUTE_MAP', 'RM_TEST|10', {'route_operation': 'permit'})
    assert _daemons_for(calls, 'no match source-protocol connected') == ['bgpd', 'mgmtd']


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_route_map_statement_uses_table_daemons(run_cmd):
    """The statement itself carries no tag, so it takes ROUTE_MAP's daemons."""
    daemon = _route_map_daemon(run_cmd)
    daemon.route_map = {}
    calls = _feed(daemon, run_cmd, 'ROUTE_MAP', 'RM_TEST|10', {'route_operation': 'permit'})
    assert _daemons_for(calls, 'route-map RM_TEST permit 10') is None


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_ipv6_prefix_goes_to_mgmtd(run_cmd):
    """An IPv6 prefix-list line goes to mgmtd, not zebra."""
    from frrcfgd.frrcfgd import BGPConfigDaemon

    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    _feed(daemon, run_cmd, 'PREFIX_SET', 'PFS_V6', {'mode': 'IPv6'})
    calls = _feed(daemon, run_cmd, 'PREFIX', 'PFS_V6|10|fc00::/64|exact', {'action': 'permit'})
    assert _daemons_for(calls, 'ipv6 prefix-list PFS_V6 seq 10 permit fc00::/64') == ['bgpd', 'mgmtd']


def _render_route_map(route_map_data):
    template_dir = os.path.join(os.path.dirname(__file__), '..', 'templates', 'bgpd')
    env = Environment(loader=FileSystemLoader(template_dir), trim_blocks=True)
    return env.get_template('bgpd.conf.db.route_map.j2').render(ROUTE_MAP=route_map_data)


def test_template_renders_set_src():
    """The boot render carries set_src as 'set src' in the statement body."""
    out = _render_route_map({('RM_SRC', '10'): {'route_operation': 'permit',
                                                'set_src': '10.1.0.32'}})
    assert 'route-map RM_SRC permit 10\n set src 10.1.0.32\n' in out, out


def test_template_omits_set_src_when_unset():
    out = _render_route_map({('RM_SRC', '10'): {'route_operation': 'permit'}})
    assert 'set src' not in out, out
