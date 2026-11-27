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
# because can’t use dotted names directly in a call, have to create a dictionary and unpack it using **:
mockmapping = {'swsscommon.swsscommon': swsscommon_module_mock}

@patch.dict('sys.modules', **mockmapping)
def test_contructor():
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.start()
    for table, hdlr in daemon.table_handler_list:
        daemon.config_db.subscribe.assert_any_call(table, hdlr)
    daemon.config_db.pubsub.psubscribe.assert_called_once()
    assert(daemon.config_db.sub_thread.is_alive() == True)
    daemon.stop()
    daemon.config_db.pubsub.punsubscribe.assert_called_once()
    assert(daemon.config_db.sub_thread.is_alive() == False)

@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_key_and_asn_validation(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    normalize_key = daemon._BGPConfigDaemon__normalize_bgp_table_key
    asn_is_valid = daemon._BGPConfigDaemon__bgp_asn_is_valid
    validate_input = daemon._BGPConfigDaemon__validate_bgp_table_input

    assert normalize_key('BGP_NEIGHBOR', 'default|FC00:10::1') == 'default|fc00:10::1'
    assert normalize_key('BGP_NEIGHBOR', 'default|1234') == 'default|1234'
    assert normalize_key('BGP_NEIGHBOR', 'default|Ethernet0') == 'default|Ethernet0'
    assert normalize_key('BGP_PEER_GROUP', 'Vrf-RED_1|PG_V4-1') == 'Vrf-RED_1|PG_V4-1'
    assert normalize_key('BGP_NEIGHBOR_AF', 'default|10.0.0.1|ipv4_unicast') == \
        'default|10.0.0.1|ipv4_unicast'
    assert normalize_key('BGP_GLOBALS_AF', 'default|l2vpn_evpn') == \
        'default|l2vpn_evpn'
    assert normalize_key('ROUTE_REDISTRIBUTE', 'default|static|bgp|ipv6') == \
        'default|static|bgp|ipv6'

    for table, key in (
            ('BGP_NEIGHBOR', None),
            ('BGP_NEIGHBOR', 'vrf name|10.0.0.1'),
            ('BGP_NEIGHBOR', 'default|peer name'),
            ('BGP_PEER_GROUP', 'default|peer' + chr(39) + 'name'),
            ('BGP_NEIGHBOR_AF', 'default|10.0.0.1'),
            ('BGP_GLOBALS', 'default|extra'),
            ('BGP_GLOBALS_AF', 'default|ipv4_unicast' + chr(39) + '-c-bad'),
            ('BGP_NEIGHBOR_AF', 'default|10.0.0.1|ipv4_multicast'),
            ('ROUTE_REDISTRIBUTE', 'default|static|bgp|l2vpn')):
        assert normalize_key(table, key) is None

    for value in (1, '4294967295'):
        assert asn_is_valid(value)
    for value in (True, 0, '4294967296', '65000x', '9' * 5000):
        assert not asn_is_valid(value)

    assert validate_input('BGP_NEIGHBOR', 'default|10.0.0.1',
                          {'name': 'Edge peer "A"',
                           'shutdown_message': 'planned maintenance'}) == \
        'default|10.0.0.1'

    daemon.metadata_asn = '65000'
    daemon.metadata_handler('DEVICE_METADATA', 'localhost',
                            {'bgp_asn': "65000' -c 'bad"})
    assert daemon.metadata_asn == '65000'

    daemon.bgp_global_handler('BGP_GLOBALS', 'vrf name', {'local_asn': '65000'})
    daemon.bgp_global_handler('BGP_GLOBALS', 'default', {'local_asn': '0'})
    daemon.bgp_global_handler('BGP_GLOBALS', 'default', {'confed_id': '0'})
    daemon.bgp_global_handler('BGP_GLOBALS', 'default',
                              {'confed_peers': ['65001', "65002' -c 'bad"]})
    daemon.bgp_neighbor_handler('BGP_NEIGHBOR', 'default|peer name', {'asn': '65001'})
    daemon.bgp_neighbor_handler('BGP_NEIGHBOR', 'default|10.0.0.1', {'asn': '65001x'})
    daemon.bgp_neighbor_handler('BGP_PEER_GROUP', 'default|PG1', {'local_asn': '0'})
    daemon.bgp_neighbor_handler('BGP_NEIGHBOR', 'default|10.0.0.1',
                                {'peer_group_name': "PG1' -c 'bad"})
    daemon.bgp_neighbor_handler('BGP_NEIGHBOR', 'default|10.0.0.1',
                                {'name': 'edge peer\nexit'})
    daemon.bgp_neighbor_handler('BGP_PEER_GROUP', 'default|PG1',
                                {'auth_password': 'secret\nexit'})
    daemon.bgp_table_handler_common('BGP_GLOBALS_LISTEN_PREFIX', 'default|10.0.0.0/24',
                                    {'peer_group': 'PG1\nbad'})
    run_cmd.assert_not_called()
    assert daemon.bgp_message.empty()
    assert not daemon.table_data_cache


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_empty_confed_peers_runtime_update_is_normalized(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    update_bgp = MagicMock()
    daemon._BGPConfigDaemon__update_bgp = update_bgp

    daemon.bgp_global_handler(
        'BGP_GLOBALS', 'default',
        {'local_asn': '65000', 'confed_peers': ['']})

    key, del_table, table, data = daemon.bgp_message.get_nowait()
    assert (key, del_table, table) == ('default', False, 'BGP_GLOBALS')
    assert data['local_asn'].data == '65000'
    assert data['confed_peers'].data == []
    update_bgp.assert_called_once()


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_unified_replay_validates_bgp_keys_and_asns(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.config_db.serialize_key.side_effect = (
        lambda key: '|'.join(key) if isinstance(key, tuple) else key)
    replay_entry = daemon._BGPConfigDaemon__replay_table_entry

    for table, key, data in (
            ('BGP_NEIGHBOR', ('vrf name', '10.0.0.1'), {'asn': '65001'}),
            ('BGP_NEIGHBOR', ('default', 'peer name'), {'asn': '65001'}),
            ('BGP_NEIGHBOR', ('default', '10.0.0.1'), {'asn': "65001' -c 'bad"}),
            ('BGP_NEIGHBOR', ('default', '10.0.0.1'),
             {'peer_group_name': "PG1' -c 'bad"}),
            ('BGP_NEIGHBOR', ('default', '10.0.0.1'),
             {'name': 'edge peer\nexit'}),
            ('BGP_GLOBALS_LISTEN_PREFIX', ('default', '10.0.0.0/24'),
             {'peer_group': 'PG1\nbad'}),
            ('BGP_GLOBALS', 'default', {'local_asn': '0'})):
        replay_entry(table, key, data)

    run_cmd.assert_not_called()
    assert daemon.bgp_message.empty()
    assert not daemon.table_data_cache

    update_bgp = MagicMock()
    daemon._BGPConfigDaemon__update_bgp = update_bgp
    replay_entry('BGP_NEIGHBOR', ('default', 'FC00:10::1'), {'asn': '65001'})
    key, del_table, table, data = daemon.bgp_message.get_nowait()
    assert (key, del_table, table) == ('default|fc00:10::1', False, 'BGP_NEIGHBOR')
    assert data['asn'].data == '65001'
    assert data['asn'].op == 1
    update_bgp.assert_called_once()

    update_bgp.reset_mock()
    replay_entry('BGP_GLOBALS', 'default',
                 {'local_asn': '65000', 'confed_peers': ['']})
    key, del_table, table, data = daemon.bgp_message.get_nowait()
    assert (key, del_table, table) == ('default', False, 'BGP_GLOBALS')
    assert data['local_asn'].data == '65000'
    assert data['confed_peers'].data == []
    update_bgp.assert_called_once()


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_dequeue_revalidates_dependent_bgp_updates(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon, CachedDataWithOp
    daemon = BGPConfigDaemon()
    daemon.config_db.serialize_key.side_effect = (
        lambda key: '|'.join(key) if isinstance(key, tuple) else key)
    daemon.config_db.get_table.return_value = {
        ('default', '10.0.0.0/24\nexit'): {'peer_group': 'PG1'},
    }
    apply_dep = daemon._BGPConfigDaemon__apply_dep_vrf_table
    update_bgp = daemon._BGPConfigDaemon__update_bgp

    apply_dep('default', 'BGP_GLOBALS_LISTEN_PREFIX',
              match=lambda data: data.get('peer_group') == 'PG1')
    changed = []
    update_bgp(changed)
    assert changed == []

    daemon.bgp_message.put((
        'default|10.0.0.1', False, 'BGP_NEIGHBOR',
        {'name': CachedDataWithOp('edge peer\nexit',
                                  CachedDataWithOp.OP_ADD)}))
    update_bgp(changed)
    assert changed == []
    run_cmd.assert_not_called()

class CmdMapTestInfo:
    data_buf = {}
    def __init__(self, table, key, data, exp_cmd, no_del = False, neg_cmd = None,
                 chk_data = None, daemons = None, ignore_tail = False):
        self.table_name = table
        self.key = key
        self.data = data
        self.vtysh_cmd = exp_cmd
        self.no_del = no_del
        self.vtysh_neg_cmd = neg_cmd
        self.chk_data = chk_data
        self.daemons = daemons
        self.ignore_tail = ignore_tail
    @classmethod
    def add_test_data(cls, test):
        assert(isinstance(test.data, dict))
        cls.data_buf.setdefault(
                test.table_name, {}).setdefault(test.key, {}).update(test.data)
    @classmethod
    def del_test_data(cls, test):
        assert(test.table_name in cls.data_buf and
               test.key in cls.data_buf[test.table_name])
        cache_data = cls.data_buf[test.table_name][test.key]
        assert(isinstance(test.data, dict))
        for k, v in test.data.items():
            assert(k in cache_data and cache_data[k] == v)
            del(cache_data[k])
    @classmethod
    def get_test_data(cls, test):
        assert(test.table_name in cls.data_buf and
               test.key in cls.data_buf[test.table_name])
        return copy.deepcopy(cls.data_buf[test.table_name][test.key])
    @staticmethod
    def compose_vtysh_cmd(cmd_list, negtive = False):
        result = ['vtysh']
        for cmd in cmd_list:
            cmd = cmd.format('no ' if negtive else '')
            result += ['-c', cmd]
        return result
    def check_running_cmd(self, mock, is_del):
        if is_del:
            vtysh_cmd = self.vtysh_cmd if self.vtysh_neg_cmd is None else self.vtysh_neg_cmd
        else:
            vtysh_cmd = self.vtysh_cmd
        if callable(vtysh_cmd):
            cmds = []
            for call in mock.call_args_list:
                assert(call[0][0] == self.table_name)
                cmds.append(call[0][1])
            vtysh_cmd(is_del, cmds, self.chk_data)
        else:
            if self.ignore_tail is None:
                mock.assert_called_with(self.table_name, self.compose_vtysh_cmd(vtysh_cmd, is_del),
                                        True, self.daemons)
            else:
                mock.assert_called_with(self.table_name, self.compose_vtysh_cmd(vtysh_cmd, is_del),
                                        True, self.daemons, self.ignore_tail)

def hdl_confed_peers_cmd(is_del, cmd_list, chk_data):
    assert(len(chk_data) >= len(cmd_list))
    if is_del:
        chk_data = list(reversed(chk_data))
    for idx, cmd in enumerate(cmd_list):
        # cmd is now a list: ['vtysh', '-c', ..., '-c', last_cmd]
        # Extract last -c value
        last_cmd = cmd[-1] if isinstance(cmd, list) else re.findall(r"-c\s+'([^']+)'\s*", cmd)[-1]
        neg_cmd = False
        if last_cmd.startswith('no '):
            neg_cmd = True
            last_cmd = last_cmd[len('no '):]
        assert(last_cmd.startswith('bgp confederation peers '))
        peer_set = set(last_cmd[len('bgp confederation peers '):].split())
        if is_del or (len(chk_data) >= 3 and idx == 0):
            assert(neg_cmd)
        else:
            assert(not neg_cmd)
        assert(peer_set == chk_data[idx])

conf_cmd = 'configure terminal'
conf_bgp_cmd = lambda vrf, asn: [conf_cmd, 'router bgp %d vrf %s' % (asn, vrf)]
conf_no_bgp_cmd = lambda vrf, asn: [conf_cmd, 'no router bgp %d%s' % (asn, '' if vrf == 'default' else ' vrf %s' % vrf)]
conf_bgp_dft_cmd = lambda vrf, asn: conf_bgp_cmd(vrf, asn) + ['no bgp default ipv4-unicast']
conf_bgp_af_cmd = lambda vrf, asn, af: conf_bgp_cmd(vrf, asn) + ['address-family %s %s' % (af, 'evpn' if af == 'l2vpn' else 'unicast')]

bgp_globals_data = [
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'local_asn': 100},
                       conf_bgp_dft_cmd('default', 100), False, conf_no_bgp_cmd('default', 100), None, None, None),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'router_id': '1.1.1.1'},
                       conf_bgp_cmd('default', 100) + ['{}bgp router-id 1.1.1.1']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'load_balance_mp_relax': 'true'},
                       conf_bgp_cmd('default', 100) + ['{}bgp bestpath as-path multipath-relax ']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'as_path_mp_as_set': 'true'},
                       conf_bgp_cmd('default', 100) + ['bgp bestpath as-path multipath-relax as-set'], False,
                       conf_bgp_cmd('default', 100) + ['bgp bestpath as-path multipath-relax ']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'always_compare_med': 'false'},
                       conf_bgp_cmd('default', 100) + ['no bgp always-compare-med']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'external_compare_router_id': 'true'},
                       conf_bgp_cmd('default', 100) + ['{}bgp bestpath compare-routerid']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'ignore_as_path_length': 'true'},
                       conf_bgp_cmd('default', 100) + ['{}bgp bestpath as-path ignore']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'graceful_restart_enable': 'true'},
                       conf_bgp_cmd('default', 100) + ['{}bgp graceful-restart']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'gr_restart_time': '10'},
                       conf_bgp_cmd('default', 100) + ['{}bgp graceful-restart restart-time 10']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'gr_stale_routes_time': '20'},
                       conf_bgp_cmd('default', 100) + ['{}bgp graceful-restart stalepath-time 20']),
        CmdMapTestInfo('BGP_GLOBALS', 'default', {'gr_preserve_fw_state': 'true'},
                       conf_bgp_cmd('default', 100) + ['{}bgp graceful-restart preserve-fw-state']),
        CmdMapTestInfo('BGP_GLOBALS_AF', 'default|ipv4_unicast', {'ebgp_route_distance': '100',
                                                                  'ibgp_route_distance': '115',
                                                                  'local_route_distance': '238'},
                       conf_bgp_af_cmd('default', 100, 'ipv4') + ['{}distance bgp 100 115 238']),
        CmdMapTestInfo('BGP_GLOBALS_AF', 'default|ipv6_unicast', {'autort': 'rfc8365-compatible'},
                       conf_bgp_af_cmd('default', 100, 'ipv6') + ['{}autort rfc8365-compatible']),
        CmdMapTestInfo('BGP_GLOBALS_AF', 'default|ipv6_unicast', {'advertise-all-vni': 'true'},
                       conf_bgp_af_cmd('default', 100, 'ipv6') + ['{}advertise-all-vni']),
        CmdMapTestInfo('BGP_GLOBALS_AF', 'default|ipv6_unicast', {'advertise-svi-ip': 'true'},
                       conf_bgp_af_cmd('default', 100, 'ipv6') + ['{}advertise-svi-ip']),
        CmdMapTestInfo('BGP_GLOBALS', 'Vrf_red', {'local_asn': 200},
                       conf_bgp_dft_cmd('Vrf_red', 200), False, conf_no_bgp_cmd('Vrf_red', 200), None, None, None),
        CmdMapTestInfo('BGP_GLOBALS', 'Vrf_red', {'med_confed': 'true'},
                       conf_bgp_cmd('Vrf_red', 200) + ['{}bgp bestpath med confed']),
        CmdMapTestInfo('BGP_GLOBALS', 'Vrf_red', {'confed_peers': ['2', '10', '5']},
                       hdl_confed_peers_cmd, True, None, [{'2', '10', '5'}]),
        CmdMapTestInfo('BGP_GLOBALS', 'Vrf_red', {'confed_peers': ['10', '8']},
                       hdl_confed_peers_cmd, False, None, [{'2', '5'}, {'8'}, {'10', '8'}]),
        CmdMapTestInfo('BGP_GLOBALS', 'Vrf_red', {'keepalive': '300', 'holdtime': '900'},
                       conf_bgp_cmd('Vrf_red', 200) + ['{}timers bgp 300 900']),
        CmdMapTestInfo('BGP_GLOBALS', 'Vrf_red', {'max_med_admin': 'true', 'max_med_admin_val': '20'},
                       conf_bgp_cmd('Vrf_red', 200) + ['{}bgp max-med administrative 20']),
        CmdMapTestInfo('BGP_GLOBALS_AF', 'Vrf_red|ipv4_unicast', {'import_vrf': 'Vrf_test'},
                       conf_bgp_af_cmd('Vrf_red', 200, 'ipv4') + ['{}import vrf Vrf_test']),
        CmdMapTestInfo('BGP_GLOBALS_AF', 'Vrf_red|ipv6_unicast', {'import_vrf_route_map': 'test_map'},
                       conf_bgp_af_cmd('Vrf_red', 200, 'ipv6') + ['{}import vrf route-map test_map']),
]

# Add admin status test cases for BGP_NEIGHBOR_AF and BGP_PEER_GROUP_AF
address_families = ['ipv4', 'ipv6', 'l2vpn']
admin_states = [
    ('true', '{}neighbor {} activate'),
    ('false', '{}no neighbor {} activate'),
    ('up', '{}neighbor {} activate'),
    ('down', '{}no neighbor {} activate')
]

def create_af_test_data(table_name):
    # Start with BGP globals setup
    test_data = [
        CmdMapTestInfo('BGP_GLOBALS', 'default',
                      {'local_asn': '100'},
                      conf_bgp_dft_cmd('default', 100),
                      ignore_tail=None)
    ]
    for af in address_families:
        af_key = f"{af}_{'evpn' if af == 'l2vpn' else 'unicast'}"
        if af == 'ipv4':
            entries = [('PG_IPV4_1', 'default')] if table_name == 'BGP_PEER_GROUP_AF' else \
                      [('10.0.0.1', 'default')]
        elif af == 'ipv6':
            entries = [('PG_IPV6_1', 'default')] if table_name == 'BGP_PEER_GROUP_AF' else \
                      [('2001:db8::1', 'default')]
        else:  # l2vpn case
            entries = [('PG_EVPN_1', 'default')] if table_name == 'BGP_PEER_GROUP_AF' else \
                      [('10.0.0.1', 'default')]

        for entry, vrf in entries:
            for status, cmd_template in admin_states:
                test_data.append(
                    CmdMapTestInfo(
                        table_name,
                        f'{vrf}|{entry}|{af_key}',
                        {'admin_status': status},
                        conf_bgp_af_cmd(vrf, 100, af) + [cmd_template.format('', entry)]
                    )
                )
    return test_data

# Create test data for both neighbor and peer group AF
neighbor_af_data = create_af_test_data('BGP_NEIGHBOR_AF')
peer_group_af_data = create_af_test_data('BGP_PEER_GROUP_AF')

# Create test data for neighbor shutdown
neighbor_shutdown_data = [
    # Set up BGP globals first
    CmdMapTestInfo('BGP_GLOBALS', 'default',
                  {'local_asn': '100'},
                  conf_bgp_dft_cmd('default', 100),
                  ignore_tail=None),
    # Then add neighbor shutdown configuration
    CmdMapTestInfo('BGP_NEIGHBOR', 'default|10.1.1.1',
                  {'admin_status': 'down', 'shutdown_message': 'maintenance'},
                  conf_bgp_cmd('default', 100) + ['{}neighbor 10.1.1.1 shutdown message maintenance']),
    CmdMapTestInfo('BGP_NEIGHBOR', 'default|10.1.1.2',
                  {'admin_status': 'false', 'shutdown_message': 'planned outage'},
                  conf_bgp_cmd('default', 100) + ['{}neighbor 10.1.1.2 shutdown message planned outage']),
    CmdMapTestInfo('BGP_NEIGHBOR', 'default|10.1.1.4',
                  {'admin_status': 'up'},
                  conf_bgp_cmd('default', 100) + ['{}no neighbor 10.1.1.4 shutdown']),
    CmdMapTestInfo('BGP_NEIGHBOR', 'default|10.1.1.5',
                  {'admin_status': 'true'},
                  conf_bgp_cmd('default', 100) + ['{}no neighbor 10.1.1.5 shutdown'])
]

@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def data_set_del_test(test_data, run_cmd, skip_del=False):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    data_buf = {}
    # add data in list
    for test in test_data:
        run_cmd.reset_mock()
        hdlr = [h for t, h in daemon.table_handler_list if t == test.table_name]
        assert(len(hdlr) == 1)
        CmdMapTestInfo.add_test_data(test)
        hdlr[0](test.table_name, test.key, CmdMapTestInfo.get_test_data(test))
        test.check_running_cmd(run_cmd, False)

    if skip_del:
        return

    # delete data in reverse direction
    for test in reversed(test_data):
        if test.no_del:
            continue
        run_cmd.reset_mock()
        hdlr = [h for t, h in daemon.table_handler_list if t == test.table_name]
        assert(len(hdlr) == 1)
        CmdMapTestInfo.del_test_data(test)
        hdlr[0](test.table_name, test.key, CmdMapTestInfo.get_test_data(test))
        test.check_running_cmd(run_cmd, True)

def test_bgp_globals():
    data_set_del_test(bgp_globals_data)


def _new_bgp_config_daemon():
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.bgp_asn = {'default': '65000'}
    return daemon


def _bgp_af_sid_advertise_command(af, command):
    return ['vtysh', '-c', 'configure terminal',
            '-c', 'router bgp 65000 vrf default',
            '-c', 'address-family {} unicast'.format(af),
            '-c', command]


def _apply_bgp_af_config(daemon, run_cmd, af, data):
    run_cmd.reset_mock()
    daemon.bgp_af_handler('BGP_GLOBALS_AF', 'default|{}_unicast'.format(af), data)
    return [call.args[1] for call in run_cmd.call_args_list]


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_ipv4_without_route_map(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {'srv6_sid_advertise': 'true'}) == [
        _bgp_af_sid_advertise_command('ipv4', 'sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == {'srv6_sid_advertise': 'true'}


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_ipv4_with_route_map(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    data = {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'SID_ADVERTISE_MAP_V4'}

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', data.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv4', 'sid export auto behavior dt46 route-map SID_ADVERTISE_MAP_V4')]
    assert daemon.table_data_cache[table_key] == data


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_ipv6_without_route_map(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv6_unicast'

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv6', {'srv6_sid_advertise': 'true'}) == [
        _bgp_af_sid_advertise_command('ipv6', 'sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == {'srv6_sid_advertise': 'true'}


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_ipv6_with_route_map(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv6_unicast'
    data = {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'SID_ADVERTISE_MAP_V6'}

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv6', data.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv6', 'sid export auto behavior dt46 route-map SID_ADVERTISE_MAP_V6')]
    assert daemon.table_data_cache[table_key] == data


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_ipv4_disabled(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {
        'srv6_sid_advertise': 'false'}) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == {'srv6_sid_advertise': 'false'}


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_ipv6_disabled(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv6_unicast'

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv6', {
        'srv6_sid_advertise': 'false'}) == [
        _bgp_af_sid_advertise_command('ipv6', 'no sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == {'srv6_sid_advertise': 'false'}


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_enable_disable_update(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    config = {'srv6_sid_advertise': 'true',
              'srv6_sid_advertise_route_map': 'SID_ADVERTISE_MAP'}

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', config.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv4', 'sid export auto behavior dt46 route-map SID_ADVERTISE_MAP')]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {
        'srv6_sid_advertise': 'false',
        'srv6_sid_advertise_route_map': 'SID_ADVERTISE_MAP'}) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == {
        'srv6_sid_advertise': 'false',
        'srv6_sid_advertise_route_map': 'SID_ADVERTISE_MAP'}

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', config.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv4', 'sid export auto behavior dt46 route-map SID_ADVERTISE_MAP')]
    assert daemon.table_data_cache[table_key] == config


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_disable_failure(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    original_config = {'srv6_sid_advertise': 'true'}
    _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_config.copy())

    run_cmd.side_effect = [False]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {
        'srv6_sid_advertise': 'false'}) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == original_config


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_route_map_removal_while_disabled(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    disabled_config = {'srv6_sid_advertise': 'false',
                       'srv6_sid_advertise_route_map': 'SID_ADVERTISE_MAP'}

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', disabled_config.copy()) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46')]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {
        'srv6_sid_advertise': 'false'}) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == {'srv6_sid_advertise': 'false'}


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_route_map_update_removal_and_restore(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    original_config = {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'ORIGINAL_MAP'}

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_config.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv4', 'sid export auto behavior dt46 route-map ORIGINAL_MAP')]
    assert daemon.table_data_cache[table_key] == original_config

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_config.copy()) == []
    assert daemon.table_data_cache[table_key] == original_config

    updated_config = {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'UPDATED_MAP'}
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', updated_config.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv4', 'sid export auto behavior dt46 route-map UPDATED_MAP')]
    assert daemon.table_data_cache[table_key] == updated_config

    base_config = {'srv6_sid_advertise': 'true'}
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', base_config.copy()) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46'),
        _bgp_af_sid_advertise_command('ipv4', 'sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == base_config

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_config.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv4', 'sid export auto behavior dt46 route-map ORIGINAL_MAP')]
    assert daemon.table_data_cache[table_key] == original_config


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_delete_and_restore(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key_v4 = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    table_key_v6 = 'BGP_GLOBALS_AF&&default|ipv6_unicast'
    original_v4 = {'srv6_sid_advertise': 'true'}
    original_v6 = {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'ORIGINAL_MAP_V6'}

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_v4.copy()) == [
        _bgp_af_sid_advertise_command('ipv4', 'sid export auto behavior dt46')]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv6', original_v6.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv6', 'sid export auto behavior dt46 route-map ORIGINAL_MAP_V6')]

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', None) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46')]
    assert table_key_v4 not in daemon.table_data_cache
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv6', None) == [
        _bgp_af_sid_advertise_command('ipv6', 'no sid export auto behavior dt46')]
    assert table_key_v6 not in daemon.table_data_cache

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_v4.copy()) == [
        _bgp_af_sid_advertise_command('ipv4', 'sid export auto behavior dt46')]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv6', original_v6.copy()) == [
        _bgp_af_sid_advertise_command(
            'ipv6', 'sid export auto behavior dt46 route-map ORIGINAL_MAP_V6')]
    assert daemon.table_data_cache[table_key_v4] == original_v4
    assert daemon.table_data_cache[table_key_v6] == original_v6


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_initial_set_failure(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    run_cmd.side_effect = [False]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {'srv6_sid_advertise': 'true'}) == [
        _bgp_af_sid_advertise_command('ipv4', 'sid export auto behavior dt46')]
    assert table_key not in daemon.table_data_cache


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_route_map_update_failure(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    original_config = {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'ORIGINAL_MAP'}
    _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_config.copy())

    run_cmd.side_effect = [False]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {
            'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'UPDATED_MAP'}) == [
        _bgp_af_sid_advertise_command(
            'ipv4', 'sid export auto behavior dt46 route-map UPDATED_MAP')]
    assert daemon.table_data_cache[table_key] == original_config


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_unset_failure(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    original_config = {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'ORIGINAL_MAP'}
    _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_config.copy())

    run_cmd.side_effect = [False]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {'srv6_sid_advertise': 'true'}) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == original_config


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_reapply_failure(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    _apply_bgp_af_config(
        daemon, run_cmd, 'ipv4', {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'ORIGINAL_MAP'})

    run_cmd.side_effect = [True, False]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {'srv6_sid_advertise': 'true'}) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46'),
        _bgp_af_sid_advertise_command('ipv4', 'sid export auto behavior dt46')]
    assert table_key not in daemon.table_data_cache


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_delete_failure(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'
    original_config = {'srv6_sid_advertise': 'true', 'srv6_sid_advertise_route_map': 'ORIGINAL_MAP'}
    _apply_bgp_af_config(daemon, run_cmd, 'ipv4', original_config.copy())

    run_cmd.side_effect = [False]
    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', None) == [
        _bgp_af_sid_advertise_command('ipv4', 'no sid export auto behavior dt46')]
    assert daemon.table_data_cache[table_key] == original_config


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_rejects_invalid_boolean(run_cmd):
    daemon = _new_bgp_config_daemon()
    table_key = 'BGP_GLOBALS_AF&&default|ipv4_unicast'

    assert _apply_bgp_af_config(daemon, run_cmd, 'ipv4', {
        'srv6_sid_advertise': 'auto'}) == []
    run_cmd.assert_not_called()
    assert table_key not in daemon.table_data_cache


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_af_sid_advertise_rejects_unsupported_context(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.bgp_asn = {'default': '65000', 'Vrf_red': '65001'}

    daemon.bgp_af_handler(
        'BGP_GLOBALS_AF', 'Vrf_red|ipv4_unicast', {'srv6_sid_advertise': 'true'})
    run_cmd.assert_not_called()

    daemon.bgp_af_handler(
        'BGP_GLOBALS_AF', 'default|l2vpn_evpn', {'srv6_sid_advertise': 'true'})
    run_cmd.assert_not_called()

    daemon.bgp_af_handler(
        'BGP_GLOBALS_AF', 'default|ipv4_unicast',
        {'srv6_sid_advertise_route_map': 'ORPHAN_MAP'})
    run_cmd.assert_not_called()


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_globals_srv6_locator_set_update_delete(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.bgp_asn = {'default': '65000'}
    daemon.table_data_cache = {}

    def locator_command(command):
        cmd = ['vtysh', '-c', 'configure terminal',
               '-c', 'router bgp 65000 vrf default',
               '-c', 'segment-routing srv6']
        if command.startswith('locator '):
            cmd += ['-c', 'encap-behavior H_Encaps_Red']
        return cmd + ['-c', command]

    daemon.bgp_global_handler('BGP_GLOBALS', 'default', {'srv6_locator': 'loc1'})
    run_cmd.assert_called_once_with(
        'BGP_GLOBALS', locator_command('locator loc1'), True, None)
    table_key = 'BGP_GLOBALS&&default'
    assert daemon.table_data_cache[table_key]['srv6_locator'] == 'loc1'

    run_cmd.reset_mock()
    daemon.bgp_global_handler('BGP_GLOBALS', 'default', {'srv6_locator': 'loc2'})
    assert [call.args[1] for call in run_cmd.call_args_list] == [
        locator_command('no locator loc1'), locator_command('locator loc2')]
    assert daemon.table_data_cache[table_key]['srv6_locator'] == 'loc2'

    run_cmd.reset_mock()
    run_cmd.side_effect = [True, False]
    daemon.bgp_global_handler('BGP_GLOBALS', 'default', {'srv6_locator': 'loc3'})
    assert [call.args[1] for call in run_cmd.call_args_list] == [
        locator_command('no locator loc2'), locator_command('locator loc3')]
    assert 'srv6_locator' not in daemon.table_data_cache.get(table_key, {})
    run_cmd.side_effect = None

    run_cmd.reset_mock()
    daemon.bgp_global_handler('BGP_GLOBALS', 'default', {'srv6_locator': 'loc4'})
    assert run_cmd.call_args.args[1] == locator_command('locator loc4')
    assert daemon.table_data_cache[table_key]['srv6_locator'] == 'loc4'

    run_cmd.reset_mock()
    daemon.bgp_global_handler('BGP_GLOBALS', 'default', {})
    run_cmd.assert_called_once_with(
        'BGP_GLOBALS', locator_command('no locator loc4'), True, None)
    assert 'srv6_locator' not in daemon.table_data_cache.get(table_key, {})


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_srv6_my_locator_format_usid_f3216(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()

    daemon.bgp_table_handler_common(
        'SRV6_MY_LOCATORS', 'MAIN',
        {'prefix': 'fcbb:bbbb:1::', 'format': 'usid-f3216'})

    cmd = ['vtysh', '-c', 'configure terminal',
           '-c', 'segment-routing', '-c', 'srv6', '-c', 'locators',
           '-c', 'locator MAIN', '-c', 'prefix fcbb:bbbb:1::/48',
           '-c', 'format usid-f3216']
    run_cmd.assert_called_once_with('SRV6_MY_LOCATORS', cmd, True, None)
    assert daemon.table_data_cache['SRV6_MY_LOCATORS&&MAIN'] == {
        'prefix': 'fcbb:bbbb:1::', 'format': 'usid-f3216'}


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_srv6_my_locator_rejects_unsupported_format(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()

    daemon.bgp_table_handler_common(
        'SRV6_MY_LOCATORS', 'MAIN',
        {'format': 'unknown', 'prefix': 'fcbb:bbbb:1::'})

    run_cmd.assert_not_called()


def test_bgp_neighbor_af():
    # The neighbor AF test cases explicitly verify delete behavior, so skip the delete
    # verification data_set_del_test (else it would try the del of 'no ' commands as well and fail)
    data_set_del_test(neighbor_af_data, skip_del=True)


def _assert_bgp_af_encapsulation_type_lifecycle(run_cmd, table, peer, af):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.metadata_asn = '65000'
    key = 'default|{}|{}_unicast'.format(peer, af)
    cmd_prefix = ['vtysh', '-c', 'configure terminal',
                  '-c', 'router bgp 65000 vrf default',
                  '-c', 'address-family {} unicast'.format(af)]

    daemon.bgp_table_handler_common(
        table, key, {'encapsulation_type': 'srv6'})
    assert [call.args[1] for call in run_cmd.call_args_list] == [
        cmd_prefix + ['-c', 'neighbor {} encapsulation-srv6'.format(peer)]]
    table_key = '{}&&{}'.format(table, key)
    assert daemon.table_data_cache[table_key]['encapsulation_type'] == 'srv6'

    run_cmd.reset_mock()
    daemon.bgp_table_handler_common(
        table, key, {'encapsulation_type': 'srv6-relax'})
    assert [call.args[1] for call in run_cmd.call_args_list] == [
        cmd_prefix + ['-c', 'no neighbor {} encapsulation-srv6'.format(peer)],
        cmd_prefix + ['-c', 'neighbor {} encapsulation-srv6-relax'.format(peer)]]
    assert daemon.table_data_cache[table_key]['encapsulation_type'] == 'srv6-relax'

    run_cmd.reset_mock()
    daemon.bgp_table_handler_common(table, key, {})
    assert [call.args[1] for call in run_cmd.call_args_list] == [
        cmd_prefix + ['-c', 'no neighbor {} encapsulation-srv6-relax'.format(peer)]]
    assert 'encapsulation_type' not in daemon.table_data_cache.get(table_key, {})


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_neighbor_af_encapsulation_type_ipv4(run_cmd):
    _assert_bgp_af_encapsulation_type_lifecycle(
        run_cmd, 'BGP_NEIGHBOR_AF', '10.0.0.1', 'ipv4')


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_neighbor_af_encapsulation_type_ipv6(run_cmd):
    _assert_bgp_af_encapsulation_type_lifecycle(
        run_cmd, 'BGP_NEIGHBOR_AF', '2001:db8::1', 'ipv6')


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_peer_group_af_encapsulation_type_ipv4(run_cmd):
    _assert_bgp_af_encapsulation_type_lifecycle(
        run_cmd, 'BGP_PEER_GROUP_AF', 'PG1', 'ipv4')


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_peer_group_af_encapsulation_type_ipv6(run_cmd):
    _assert_bgp_af_encapsulation_type_lifecycle(
        run_cmd, 'BGP_PEER_GROUP_AF', 'PG1', 'ipv6')


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_neighbor_af_encapsulation_type_update_failure(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.metadata_asn = '65000'
    key = 'default|10.0.0.1|ipv4_unicast'
    cmd_prefix = ['vtysh', '-c', 'configure terminal',
                  '-c', 'router bgp 65000 vrf default',
                  '-c', 'address-family ipv4 unicast']
    table_key = 'BGP_NEIGHBOR_AF&&{}'.format(key)

    daemon.bgp_table_handler_common(
        'BGP_NEIGHBOR_AF', key, {'encapsulation_type': 'srv6'})
    assert daemon.table_data_cache[table_key]['encapsulation_type'] == 'srv6'

    run_cmd.reset_mock()
    run_cmd.side_effect = [False]
    daemon.bgp_table_handler_common(
        'BGP_NEIGHBOR_AF', key, {'encapsulation_type': 'srv6-relax'})
    assert [call.args[1] for call in run_cmd.call_args_list] == [
        cmd_prefix + ['-c', 'no neighbor 10.0.0.1 encapsulation-srv6']]
    assert daemon.table_data_cache[table_key]['encapsulation_type'] == 'srv6'

    run_cmd.reset_mock()
    run_cmd.side_effect = [True, False]
    daemon.bgp_table_handler_common(
        'BGP_NEIGHBOR_AF', key, {'encapsulation_type': 'srv6-relax'})
    assert [call.args[1] for call in run_cmd.call_args_list] == [
        cmd_prefix + ['-c', 'no neighbor 10.0.0.1 encapsulation-srv6'],
        cmd_prefix + ['-c', 'neighbor 10.0.0.1 encapsulation-srv6-relax']]
    assert 'encapsulation_type' not in daemon.table_data_cache.get(table_key, {})


def test_bgp_peer_group_af():
    # The peer group AF test cases explicitly verify delete behavior, so skip the delete
    # verification data_set_del_test (else it would try the del of 'no ' commands as well and fail)
    data_set_del_test(peer_group_af_data, skip_del=True)

def test_bgp_neighbor_shutdown():
    # The neighbor shutdown msg test cases explicitly verify delete behavior, so skip the delete
    # verification data_set_del_test (else it would try the del of 'no ' commands as well and fail)
    data_set_del_test(neighbor_shutdown_data, skip_del=True)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_neighbor_description_injection(run_cmd):
    """Regression test: shell metacharacters in BGP_NEIGHBOR description must be
    passed as a literal vtysh argument, not interpreted by a shell."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()

    # Seed BGP_GLOBALS to set local ASN (reuse existing test data)
    globals_seed = bgp_globals_data[0]  # local_asn = 100
    CmdMapTestInfo.add_test_data(globals_seed)
    bgp_globals_hdlr = [h for t, h in daemon.table_handler_list if t == 'BGP_GLOBALS'][0]
    bgp_globals_hdlr('BGP_GLOBALS', globals_seed.key, CmdMapTestInfo.get_test_data(globals_seed))

    # Now test BGP_NEIGHBOR description with injection payload
    injection_payload = "'; id #"
    run_cmd.reset_mock()
    nbr_test = CmdMapTestInfo(
        'BGP_NEIGHBOR', 'default|10.0.0.1',
        {'name': injection_payload},
        conf_bgp_cmd('default', 100) + [
            'neighbor 10.0.0.1 description {}'.format(injection_payload)
        ]
    )
    CmdMapTestInfo.add_test_data(nbr_test)
    nbr_hdlr = [h for t, h in daemon.table_handler_list if t == 'BGP_NEIGHBOR'][0]
    nbr_hdlr('BGP_NEIGHBOR', nbr_test.key, CmdMapTestInfo.get_test_data(nbr_test))

    # Verify g_run_command was called with a list (shell=False path)
    assert run_cmd.called, "g_run_command was not called for BGP_NEIGHBOR description"
    for call in run_cmd.call_args_list:
        cmd = call[0][1]
        assert isinstance(cmd, list), \
            "command must be a list (shell=False), got string: {}".format(cmd)
        if any('description' in arg for arg in cmd):
            assert any(injection_payload in arg for arg in cmd), \
                "injection payload not found as literal arg: {}".format(cmd)


def test_bgp_confederation_template():
    """Verify bgpd.conf.db.j2 renders 'bgp confederation identifier/peers'
    from BGP_GLOBALS, and emits nothing when confed is not configured."""
    from jinja2 import Environment, FileSystemLoader
    import os

    template_dir = os.path.join(os.path.dirname(__file__), '..', 'templates', 'bgpd')
    env = Environment(loader=FileSystemLoader(template_dir))
    env.filters['ipv4'] = lambda v: False
    env.filters['ipv6'] = lambda v: False

    template = env.get_template('bgpd.conf.db.j2')

    # Confederation in the default VRF and a non-default VRF. confed is a
    # per-instance knob (FRR installs it under BGP_NODE, which is shared by
    # 'router bgp <asn>' and 'router bgp <asn> vrf <vrf>'), and frrcfgd applies
    # it per VRF, so the template must render it for non-default VRFs too.
    result = template.render(
        BGP_GLOBALS={'default': {'local_asn': '65100',
                                 'confed_id': '100',
                                 'confed_peers': ['65101', '65102']},
                     'Vrf_red': {'local_asn': '65200',
                                 'confed_id': '200',
                                 'confed_peers': ['65201']}}
    )
    assert 'router bgp 65100' in result
    assert 'bgp confederation identifier 100' in result
    assert 'bgp confederation peers 65101 65102' in result
    # The non-default VRF confed lines are emitted under its own instance.
    vrf_section = result[result.index('router bgp 65200 vrf Vrf_red'):]
    assert 'bgp confederation identifier 200' in vrf_section
    assert 'bgp confederation peers 65201' in vrf_section

    # confed_id only, no peers -> identifier renders, no stray peers line.
    result = template.render(
        BGP_GLOBALS={'default': {'local_asn': '65100', 'confed_id': '100'}}
    )
    assert 'bgp confederation identifier 100' in result
    assert 'bgp confederation peers' not in result

    # Empty confed_peers list -> the |length > 0 guard suppresses the line.
    result = template.render(
        BGP_GLOBALS={'default': {'local_asn': '65100', 'confed_peers': []}}
    )
    assert 'bgp confederation peers' not in result

    # No confederation config -> no confederation lines emitted.
    result = template.render(
        BGP_GLOBALS={'default': {'local_asn': '65100'}}
    )
    assert 'bgp confederation' not in result


def test_bgp_globals_af_sid_advertise_template():
    """Verify enabled default IPv4/IPv6 SID advertisement renders with its optional route-map."""
    from jinja2 import Environment, FileSystemLoader
    import os

    template_dir = os.path.join(os.path.dirname(__file__), '..', 'templates', 'bgpd')
    env = Environment(loader=FileSystemLoader(template_dir))
    env.filters['ipv4'] = lambda v: False
    env.filters['ipv6'] = lambda v: False
    template = env.get_template('bgpd.conf.db.j2')

    result = template.render(
        BGP_GLOBALS={'default': {'local_asn': '65100'},
                     'Vrf_red': {'local_asn': '65101'}},
        BGP_GLOBALS_AF={
            ('default', 'ipv4_unicast'):
                {'srv6_sid_advertise': 'true'},
            ('default', 'ipv6_unicast'):
                {'srv6_sid_advertise': 'true',
                 'srv6_sid_advertise_route_map': 'SID_ADVERTISE_MAP'},
            ('default', 'l2vpn_evpn'):
                {'srv6_sid_advertise': 'true'},
            ('Vrf_red', 'ipv4_unicast'):
                {'srv6_sid_advertise': 'true'}})

    ipv4_af = result.split('address-family ipv4 unicast', 1)[1].split(
        'exit-address-family', 1)[0]
    ipv6_af = result.split('address-family ipv6 unicast', 1)[1].split(
        'exit-address-family', 1)[0]
    assert 'sid export auto behavior dt46' in ipv4_af
    assert 'sid export auto behavior dt46 route-map SID_ADVERTISE_MAP' in ipv6_af
    assert 'route-map' not in ipv4_af
    assert result.count('sid export auto behavior dt46') == 2

    disabled_result = template.render(
        BGP_GLOBALS={'default': {'local_asn': '65100'}},
        BGP_GLOBALS_AF={
            ('default', 'ipv4_unicast'):
                {'srv6_sid_advertise': 'false'},
            ('default', 'ipv6_unicast'):
                {'srv6_sid_advertise': 'false',
                 'srv6_sid_advertise_route_map': 'SID_ADVERTISE_MAP'}})
    assert 'sid export auto behavior dt46' not in disabled_result


def test_bgp_neighbor_af_encapsulation_type_template():
    """Verify encapsulation type renders under its neighbor's address family."""
    from jinja2 import Environment, FileSystemLoader
    import os

    template_dir = os.path.join(os.path.dirname(__file__), '..', 'templates', 'bgpd')
    env = Environment(loader=FileSystemLoader(template_dir))
    env.filters['ipv4'] = lambda v: False
    env.filters['ipv6'] = lambda v: False
    template = env.get_template('bgpd.conf.db.j2')

    result = template.render(
        BGP_GLOBALS={'default': {'local_asn': '65100'}},
        BGP_NEIGHBOR_AF={
            ('default', '10.0.0.1', 'ipv4_unicast'):
                {'encapsulation_type': 'srv6'},
            ('default', '2001:db8::1', 'ipv6_unicast'):
                {'encapsulation_type': 'srv6-relax'}},
        BGP_PEER_GROUP_AF={
            ('default', 'PG1', 'ipv6_unicast'):
                {'encapsulation_type': 'srv6'}})

    ipv4_af = result.split('address-family ipv4 unicast', 1)[1].split(
        'exit-address-family', 1)[0]
    ipv6_af = result.split('address-family ipv6 unicast', 1)[1].split(
        'exit-address-family', 1)[0]
    assert 'neighbor 10.0.0.1 encapsulation-srv6' in ipv4_af
    assert 'neighbor 2001:db8::1 encapsulation-srv6-relax' not in ipv4_af
    assert 'neighbor 2001:db8::1 encapsulation-srv6-relax' in ipv6_af
    assert 'neighbor PG1 encapsulation-srv6' in ipv6_af
    assert 'neighbor 10.0.0.1 encapsulation-srv6' not in ipv6_af


def test_bgp_globals_srv6_locator_template():
    """Verify each BGP instance renders its configured SRv6 locator and reduced encapsulation."""
    from jinja2 import Environment, FileSystemLoader
    import os

    template_dir = os.path.join(os.path.dirname(__file__), '..', 'templates', 'bgpd')
    env = Environment(loader=FileSystemLoader(template_dir))
    env.filters['ipv4'] = lambda v: False
    env.filters['ipv6'] = lambda v: False
    template = env.get_template('bgpd.conf.db.j2')

    result = template.render(
        BGP_GLOBALS={
            'default': {'local_asn': '65100', 'srv6_locator': 'LOC_DEFAULT'},
            'Vrf_red': {'local_asn': '65101', 'srv6_locator': 'LOC_RED'}})
    default_bgp = result.split('router bgp 65100', 1)[1].split(
        'router bgp 65101', 1)[0]
    red_bgp = result.split('router bgp 65101', 1)[1]
    assert 'segment-routing srv6' in default_bgp
    assert 'encap-behavior H_Encaps_Red' in default_bgp
    assert 'locator LOC_DEFAULT' in default_bgp
    assert 'locator LOC_DEFAULT\n exit' in default_bgp
    assert 'segment-routing srv6' in red_bgp
    assert 'encap-behavior H_Encaps_Red' in red_bgp
    assert 'locator LOC_RED' in red_bgp

    result = template.render(BGP_GLOBALS={'default': {'local_asn': '65100'}})
    assert 'segment-routing srv6' not in result
