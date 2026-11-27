import copy
import re
from unittest.mock import MagicMock, NonCallableMagicMock, patch


def mock_is_vrf_name_valid(name):
    return (isinstance(name, str) and name not in ('.', '..') and
            re.fullmatch(r'[A-Za-z0-9_.][A-Za-z0-9_.-]{0,14}', name) is not None)


def mock_is_interface_name_valid(name):
    return isinstance(name, str) and 0 < len(name) < 16


swsscommon_module_mock = MagicMock(
    ConfigDBConnector=NonCallableMagicMock,
    isInterfaceNameValid=mock_is_interface_name_valid,
    isVrfNameValid=mock_is_vrf_name_valid)
mockmapping = {'swsscommon.swsscommon': swsscommon_module_mock}

with patch.dict('sys.modules', **mockmapping):
    import frrcfgd.frrcfgd as frrcfgd_module
    from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector

# CONFIG_DB rows as the tables hold them
CONFIG = {
    'BGP_GLOBALS': {
        'default': {'local_asn': '65100', 'router_id': '10.1.0.1'},
    },
    'BGP_NEIGHBOR': {
        ('default', '10.100.0.1'): {'asn': '65200', 'admin_status': 'up'},
    },
    'STATIC_ROUTE': {
        ('Vrf_peer', '10.201.0.0/24'): {'nexthop': '172.16.24.2', 'ifname': 'Ethernet24',
                                        'nexthop-vrf': 'default'},
    },
}


def serialize_key(key):
    return '|'.join(key) if isinstance(key, tuple) else key


def make_daemon(mode, run_cmd, template_restore=None):
    tables = copy.deepcopy(CONFIG)
    metadata = {'docker_routing_config_mode': mode,
                'frr_mgmt_framework_config': 'true'}
    if template_restore is not None:
        metadata['use_template_render_for_restore'] = template_restore
    tables['DEVICE_METADATA'] = {'localhost': metadata}

    def get_entry(self, table, key):
        return copy.deepcopy(tables.get(table, {}).get(key, {}))

    def get_table(self, table):
        return copy.deepcopy(tables.get(table, {}))

    run_cmd.return_value = True
    with patch.object(ExtConfigDBConnector, 'get_entry', get_entry, create=True), \
            patch.object(ExtConfigDBConnector, 'get_table', get_table, create=True), \
            patch.object(ExtConfigDBConnector, 'serialize_key', staticmethod(serialize_key), create=True), \
            patch.object(frrcfgd_module, 'g_run_command', run_cmd):
        return BGPConfigDaemon()


def commands(run_cmd):
    # whitespace-normalised, the command templates leave double blanks for empty args;
    # a command is either a string or an argv list
    cmds = []
    for call in run_cmd.call_args_list:
        cmd = call[0][1]
        if not isinstance(cmd, str):
            cmd = ' '.join(str(part) for part in cmd)
        cmds.append(' '.join(cmd.split()))
    return cmds


def test_split_unified_replays_bgp_and_static_routes_at_startup():
    run_cmd = MagicMock()
    daemon = make_daemon('split-unified', run_cmd)
    cmds = commands(run_cmd)
    assert any('router bgp 65100' in c for c in cmds), cmds
    assert any('neighbor 10.100.0.1 remote-as 65200' in c for c in cmds), cmds
    # STATIC_ROUTE is diff-based: it is only programmed if the in-memory view starts empty
    assert any('ip route 10.201.0.0/24 172.16.24.2 Ethernet24' in c and 'Vrf_peer' in c for c in cmds), cmds
    # the view of FRR is rebuilt by the replay
    assert daemon.bgp_asn.get('default') == '65100'
    assert '10.201.0.0/24' in daemon.static_route_list.get('Vrf_peer', {})
    assert 'STATIC_ROUTE&&Vrf_peer|10.201.0.0/24' in daemon.table_data_cache


def test_separated_mode_does_not_replay():
    run_cmd = MagicMock()
    daemon = make_daemon('separated', run_cmd)
    assert commands(run_cmd) == []
    # the view is primed from CONFIG_DB, as before
    assert daemon.bgp_asn.get('default') == '65100'
    assert '10.201.0.0/24' in daemon.static_route_list.get('Vrf_peer', {})


def test_unified_mode_with_template_restore_does_not_replay():
    run_cmd = MagicMock()
    daemon = make_daemon('unified', run_cmd)
    # use_template_render_for_restore defaults to 'true': frr.conf was rendered
    # from the CONFIG_DB templates, so no replay is needed
    assert commands(run_cmd) == []
    assert daemon.bgp_asn.get('default') == '65100'


def test_unified_mode_without_template_restore_still_replays():
    run_cmd = MagicMock()
    daemon = make_daemon('unified', run_cmd, template_restore='false')
    cmds = commands(run_cmd)
    assert any('router bgp 65100' in c for c in cmds), cmds
    assert daemon.bgp_asn.get('default') == '65100'
