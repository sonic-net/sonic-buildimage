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

def test_bgp_neighbor_af():
    # The neighbor AF test cases explicitly verify delete behavior, so skip the delete
    # verification data_set_del_test (else it would try the del of 'no ' commands as well and fail)
    data_set_del_test(neighbor_af_data, skip_del=True)

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


# ---------------------------------------------------------------------------
# Feature / regression tests for the frrcfgd BGP parity gaps (sonic-buildimage#28482):
#   * 'no bgp ebgp-requires-policy' emitted per BGP instance
#   * route-map 'on-match next' / 'on-match goto' (continue-flow) clause
#   * listen-range peer-group attribute drop on delete + re-create
# ---------------------------------------------------------------------------

def _collect_vtysh_lines(run_cmd):
    """Return every vtysh '-c' payload across all g_run_command calls, in order."""
    lines = []
    for call in run_cmd.call_args_list:
        argv = call[0][1]
        if not isinstance(argv, list):
            continue
        i = 0
        while i < len(argv):
            if argv[i] == '-c' and i + 1 < len(argv):
                lines.append(argv[i + 1])
                i += 2
            else:
                i += 1
    return lines


def _make_daemon():
    from frrcfgd.frrcfgd import BGPConfigDaemon
    return BGPConfigDaemon()


def _handler(daemon, table):
    hdlrs = [h for t, h in daemon.table_handler_list if t == table]
    assert len(hdlrs) == 1, "expected exactly one handler for {}".format(table)
    return hdlrs[0]


def _seed_bgp_asn(daemon, asn='100', vrf='default'):
    _handler(daemon, 'BGP_GLOBALS')('BGP_GLOBALS', vrf, {'local_asn': asn})


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bgp_ebgp_requires_policy(run_cmd):
    """ebgp_requires_policy is field-driven (NOT an unconditional default): false ->
    'no bgp ebgp-requires-policy', true -> 'bgp ebgp-requires-policy'. The migrator sets it
    to replicate bgpcfgd's traditional behavior. See sonic-buildimage#28482."""
    daemon = _make_daemon()
    _seed_bgp_asn(daemon, '100')
    # Not set by default -> frrcfgd must not touch ebgp-requires-policy.
    assert not any('ebgp-requires-policy' in line for line in _collect_vtysh_lines(run_cmd)), \
        'ebgp-requires-policy must not be emitted unless the field is set'
    # Field false -> 'no bgp ebgp-requires-policy'
    run_cmd.reset_mock()
    _handler(daemon, 'BGP_GLOBALS')('BGP_GLOBALS', 'default',
                                    {'local_asn': '100', 'ebgp_requires_policy': 'false'})
    assert 'no bgp ebgp-requires-policy' in _collect_vtysh_lines(run_cmd), _collect_vtysh_lines(run_cmd)
    # Field true -> 'bgp ebgp-requires-policy'
    run_cmd.reset_mock()
    _handler(daemon, 'BGP_GLOBALS')('BGP_GLOBALS', 'default',
                                    {'local_asn': '100', 'ebgp_requires_policy': 'true'})
    assert 'bgp ebgp-requires-policy' in _collect_vtysh_lines(run_cmd), _collect_vtysh_lines(run_cmd)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_route_map_on_match(run_cmd):
    daemon = _make_daemon()
    rm_hdlr = _handler(daemon, 'ROUTE_MAP')
    # ON_MATCH_NEXT -> 'on-match next'
    rm_hdlr('ROUTE_MAP', 'RM_OMN|10',
            {'route_operation': 'permit', 'set_on_match_action': 'ON_MATCH_NEXT'})
    lines = _collect_vtysh_lines(run_cmd)
    assert 'route-map RM_OMN permit 10' in lines, lines
    assert any(line.strip() == 'on-match next' for line in lines), lines
    # ON_MATCH_GOTO with a target sequence -> 'on-match goto 20'
    run_cmd.reset_mock()
    rm_hdlr('ROUTE_MAP', 'RM_OMG|10',
            {'route_operation': 'permit', 'set_on_match_action': 'ON_MATCH_GOTO',
             'set_on_match_goto': '20'})
    lines = _collect_vtysh_lines(run_cmd)
    assert any(line.strip() == 'on-match goto 20' for line in lines), lines


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_peer_group_rerender_after_delete(run_cmd):
    """Regression (sonic-buildimage#28482): a listen-range peer-group re-created after a delete
    must re-render its attributes (e.g. update-source), not silently drop them because a stale
    cache row made the incremental diff resolve every field to 'no change'."""
    daemon = _make_daemon()
    # The real swsscommon ConfigDBConnector.serialize_key joins a key tuple with '|'; the test
    # mock does not, so make it realistic -- the peer-group cache-eviction path reconstructs the
    # cache key via serialize_key((vrf, pg)), and it must match the stored 'vrf|pg' key.
    daemon.config_db.serialize_key = lambda key_tuple: '|'.join(key_tuple)
    _seed_bgp_asn(daemon, '100')
    pg_hdlr = _handler(daemon, 'BGP_PEER_GROUP')
    pg_data = {'asn': '100', 'local_addr': '1.1.1.1'}

    run_cmd.reset_mock()
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', dict(pg_data))
    assert 'neighbor PG1 update-source 1.1.1.1' in _collect_vtysh_lines(run_cmd), \
        'update-source missing on initial peer-group create'

    # delete the peer-group
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', None)

    # re-create: attributes must be programmed again (pre-fix they were dropped)
    run_cmd.reset_mock()
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', dict(pg_data))
    assert 'neighbor PG1 update-source 1.1.1.1' in _collect_vtysh_lines(run_cmd), \
        'update-source dropped on peer-group re-create (cache not evicted on delete)'


def _fake_config_db(daemon, db):
    """Back the daemon's CONFIG_DB reads with ``{table: {key_tuple: row}}``."""
    daemon.config_db.serialize_key = lambda key_tuple: '|'.join(key_tuple)
    daemon.config_db.get_table = lambda table: copy.deepcopy(db.get(table, {}))
    daemon.config_db.get_entry = lambda table, key: copy.deepcopy(db.get(table, {}).get(key, {}))


class _FakeBgpd:
    """Fails 'neighbor <peer> peer-group <pg>' as bgpd does when <pg> does not exist,
    or when neither the peer nor the group has a remote-as."""
    def __init__(self):
        self.groups = set()
        self.remote_as = set()

    def __call__(self, table, argv, *args, **kwargs):
        cmd = argv[-1].strip() if isinstance(argv, list) else ''
        words = cmd.split()
        if len(words) == 3 and words[0] == 'neighbor' and words[2] == 'peer-group':
            self.groups.add(words[1])
        elif len(words) == 4 and words[0] == 'neighbor' and words[2] == 'peer-group':
            return words[3] in self.groups and bool({words[1], words[3]} & self.remote_as)
        elif len(words) == 4 and words[0] == 'neighbor' and words[2] == 'remote-as':
            self.remote_as.add(words[1])
        return True


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_neighbor_af_cache_cleared_on_delete(run_cmd):
    """Deleting a neighbor must evict every BGP_NEIGHBOR_AF cache row, l2vpn_evpn included."""
    daemon = _make_daemon()
    _fake_config_db(daemon, {})
    _seed_bgp_asn(daemon, '100')
    nbr_hdlr = _handler(daemon, 'BGP_NEIGHBOR')
    af_hdlr = _handler(daemon, 'BGP_NEIGHBOR_AF')
    nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', {'asn': '200'})
    af_hdlr('BGP_NEIGHBOR_AF', 'default|10.0.0.1|l2vpn_evpn', {'admin_status': 'true'})
    nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', None)
    assert not [k for k in daemon.table_data_cache if '10.0.0.1' in k], daemon.table_data_cache

    run_cmd.reset_mock()
    nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', {'asn': '200'})
    af_hdlr('BGP_NEIGHBOR_AF', 'default|10.0.0.1|l2vpn_evpn', {'admin_status': 'true'})
    assert 'neighbor 10.0.0.1 activate' in _collect_vtysh_lines(run_cmd)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_peer_type_readd_reapplies_neighbor_af(run_cmd):
    """A neighbor re-created via peer_type (no asn) must re-apply its BGP_NEIGHBOR_AF rows."""
    daemon = _make_daemon()
    _fake_config_db(daemon, {'BGP_NEIGHBOR_AF': {
        ('default', '10.0.0.1', 'ipv4_unicast'): {'admin_status': 'true'}}})
    _seed_bgp_asn(daemon, '100')
    nbr_hdlr = _handler(daemon, 'BGP_NEIGHBOR')
    nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', {'peer_type': 'external'})
    nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', None)

    run_cmd.reset_mock()
    nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', {'peer_type': 'external'})
    lines = _collect_vtysh_lines(run_cmd)
    assert 'neighbor 10.0.0.1 remote-as external' in lines, lines
    assert 'neighbor 10.0.0.1 activate' in lines, lines


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_remote_as_swap_recreates(run_cmd):
    """Swapping asn/peer_type/peer_group_name makes bgpd drop and re-create the peer, so the
    whole row and its BGP_NEIGHBOR_AF rows must be rendered again."""
    for old, new in [({'peer_type': 'external'}, {'peer_group_name': 'PG1'}),
                     ({'asn': '300'}, {'peer_group_name': 'PG1'}),
                     ({'asn': '300'}, {'peer_type': 'external'})]:
        run_cmd.reset_mock()
        daemon = _make_daemon()
        old_row, new_row = dict(old, local_addr='1.1.1.1'), dict(new, local_addr='1.1.1.1')
        db = {'BGP_NEIGHBOR': {('default', '10.0.0.1'): old_row},
              'BGP_NEIGHBOR_AF': {('default', '10.0.0.1', 'ipv4_unicast'): {'admin_status': 'true'}}}
        _fake_config_db(daemon, db)
        _seed_bgp_asn(daemon, '100')
        _handler(daemon, 'BGP_PEER_GROUP')('BGP_PEER_GROUP', 'default|PG1', {'asn': '200'})
        nbr_hdlr = _handler(daemon, 'BGP_NEIGHBOR')
        nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', dict(old_row))

        run_cmd.reset_mock()
        db['BGP_NEIGHBOR'][('default', '10.0.0.1')] = new_row
        nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', dict(new_row))
        lines = _collect_vtysh_lines(run_cmd)
        if 'peer_type' in new:
            assert 'neighbor 10.0.0.1 remote-as external' in lines, (old, lines)
        else:
            assert 'neighbor 10.0.0.1 peer-group PG1' in lines, (old, lines)
            assert '10.0.0.1' in daemon.bgp_peer_group['default']['PG1'].ref_nbrs, old
        assert 'neighbor 10.0.0.1 update-source 1.1.1.1' in lines, (old, lines)
        assert 'neighbor 10.0.0.1 activate' in lines, (old, lines)
        assert daemon.table_data_cache['BGP_NEIGHBOR&&default|10.0.0.1'] == new_row, old


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_peer_group_remote_as_swap_recreates(run_cmd):
    """A peer-group swapping asn for peer_type is re-rendered, with its members."""
    daemon = _make_daemon()
    db = {'BGP_PEER_GROUP': {('default', 'PG1'): {'asn': '200', 'local_addr': '1.1.1.1'}},
          'BGP_NEIGHBOR': {('default', '10.0.0.1'): {'peer_group_name': 'PG1'}}}
    _fake_config_db(daemon, db)
    _seed_bgp_asn(daemon, '100')
    pg_hdlr = _handler(daemon, 'BGP_PEER_GROUP')
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', {'asn': '200', 'local_addr': '1.1.1.1'})

    run_cmd.reset_mock()
    db['BGP_PEER_GROUP'][('default', 'PG1')] = {'peer_type': 'external', 'local_addr': '1.1.1.1'}
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', {'peer_type': 'external', 'local_addr': '1.1.1.1'})
    lines = _collect_vtysh_lines(run_cmd)
    assert 'neighbor PG1 remote-as external' in lines, lines
    assert 'neighbor PG1 update-source 1.1.1.1' in lines, lines
    assert 'neighbor 10.0.0.1 peer-group PG1' in lines, lines
    assert '10.0.0.1' in daemon.bgp_peer_group['default']['PG1'].ref_nbrs


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_peer_group_peer_type_update_reapplies_neighbors(run_cmd):
    """Giving an existing peer-group a peer_type must re-bind neighbors bgpd rejected
    while the group had no remote-as."""
    run_cmd.side_effect = _FakeBgpd()
    daemon = _make_daemon()
    db = {'BGP_NEIGHBOR': {('default', '10.0.0.1'): {'peer_group_name': 'PG1'}}}
    _fake_config_db(daemon, db)
    _seed_bgp_asn(daemon, '100')
    pg_hdlr = _handler(daemon, 'BGP_PEER_GROUP')
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', {'local_addr': '1.1.1.1'})
    _handler(daemon, 'BGP_NEIGHBOR')('BGP_NEIGHBOR', 'default|10.0.0.1', {'peer_group_name': 'PG1'})

    run_cmd.reset_mock()
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', {'local_addr': '1.1.1.1', 'peer_type': 'external'})
    assert 'neighbor 10.0.0.1 peer-group PG1' in _collect_vtysh_lines(run_cmd)
    assert '10.0.0.1' in daemon.bgp_peer_group['default']['PG1'].ref_nbrs


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_peer_group_readd_reapplies_af(run_cmd):
    """Re-adding a peer-group must re-apply its BGP_PEER_GROUP_AF rows still in CONFIG_DB."""
    daemon = _make_daemon()
    _fake_config_db(daemon, {'BGP_PEER_GROUP_AF': {
        ('default', 'PG1', 'ipv4_unicast'): {'admin_status': 'true'}}})
    _seed_bgp_asn(daemon, '100')
    pg_hdlr = _handler(daemon, 'BGP_PEER_GROUP')
    af_hdlr = _handler(daemon, 'BGP_PEER_GROUP_AF')
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', {'asn': '200'})
    af_hdlr('BGP_PEER_GROUP_AF', 'default|PG1|ipv4_unicast', {'admin_status': 'true'})
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', None)

    run_cmd.reset_mock()
    pg_hdlr('BGP_PEER_GROUP', 'default|PG1', {'asn': '200'})
    assert 'neighbor PG1 activate' in _collect_vtysh_lines(run_cmd)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_neighbor_before_peer_group(run_cmd):
    """``config load`` delivers BGP_NEIGHBOR before BGP_PEER_GROUP. A neighbor bgpd rejected
    because its group did not exist yet must be bound once the group is created, whether the
    group carries asn, peer_type, or no remote-as at all."""
    for pg_data, nbr_data in [({'asn': '200'}, {'peer_group_name': 'PG1'}),
                              ({'peer_type': 'external'}, {'peer_group_name': 'PG1'}),
                              ({'local_addr': '1.1.1.1'}, {'asn': '200', 'peer_group_name': 'PG1'})]:
        run_cmd.reset_mock()
        run_cmd.side_effect = _FakeBgpd()
        daemon = _make_daemon()
        db = {'BGP_NEIGHBOR': {('default', '10.0.0.1'): nbr_data},
              'BGP_NEIGHBOR_AF': {('default', '10.0.0.1', 'ipv4_unicast'): {'admin_status': 'true'}}}
        _fake_config_db(daemon, db)
        _seed_bgp_asn(daemon, '100')
        _handler(daemon, 'BGP_NEIGHBOR')('BGP_NEIGHBOR', 'default|10.0.0.1', dict(nbr_data))
        _handler(daemon, 'BGP_PEER_GROUP')('BGP_PEER_GROUP', 'default|PG1', dict(pg_data))
        lines = _collect_vtysh_lines(run_cmd)
        assert lines.count('neighbor 10.0.0.1 peer-group PG1') == 2, (pg_data, lines)
        assert 'neighbor 10.0.0.1 activate' in lines, (pg_data, lines)
        assert '10.0.0.1' in daemon.bgp_peer_group['default']['PG1'].ref_nbrs, pg_data


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_device_metadata_zebra_knobs(run_cmd):
    """The device-wide zebra knobs bgpcfgd renders must apply at runtime, not only via the
    boot-time template render -- a CONFIG_DB write should reach FRR without a config reload.

    Each is opt-in: absent emits nothing and leaves FRR's own default (180s retention for
    nexthop groups), so upgrading changes no existing installation. See
    sonic-buildimage#28482."""
    daemon = _make_daemon()
    md_hdlr = _handler(daemon, 'DEVICE_METADATA')

    # Nothing set -> frrcfgd must not touch either knob.
    md_hdlr('DEVICE_METADATA', 'localhost', {'bgp_asn': '100'})
    lines = _collect_vtysh_lines(run_cmd)
    assert not any('nexthop-group keep' in line for line in lines), lines
    assert not any('nexthop kernel' in line for line in lines), lines

    # nexthop_group_keep_time -> 'zebra nexthop-group keep <N>'
    run_cmd.reset_mock()
    md_hdlr('DEVICE_METADATA', 'localhost',
            {'bgp_asn': '100', 'nexthop_group_keep_time': '1'})
    lines = _collect_vtysh_lines(run_cmd)
    assert any(line.strip() == 'zebra nexthop-group keep 1' for line in lines), lines

    # zebra_nexthop enabled/disabled -> the enable line and its 'no' form
    run_cmd.reset_mock()
    md_hdlr('DEVICE_METADATA', 'localhost',
            {'bgp_asn': '100', 'zebra_nexthop': 'enabled'})
    lines = _collect_vtysh_lines(run_cmd)
    assert any(line.strip() == 'zebra nexthop kernel enable' for line in lines), lines

    run_cmd.reset_mock()
    md_hdlr('DEVICE_METADATA', 'localhost',
            {'bgp_asn': '100', 'zebra_nexthop': 'disabled'})
    lines = _collect_vtysh_lines(run_cmd)
    assert any(line.strip() == 'no zebra nexthop kernel enable' for line in lines), lines


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_device_metadata_suppress_fib_pending(run_cmd):
    """suppress-fib-pending is a 'router bgp <asn>' sub-command, so it is emitted with an
    explicit router-bgp prefix rather than through device_metadata_key_map (which has no ASN
    context and would emit at top level, where FRR rejects it).

    DEVICE_METADATA stays the single source of truth: it is what `config
    suppress-fib-pending` writes and what route_check.py reads."""
    daemon = _make_daemon()
    md_hdlr = _handler(daemon, 'DEVICE_METADATA')

    # Absent -> nothing emitted.
    md_hdlr('DEVICE_METADATA', 'localhost', {'bgp_asn': '100'})
    assert not any('suppress-fib-pending' in line for line in _collect_vtysh_lines(run_cmd))

    # enabled -> the line, framed inside 'router bgp <asn>'
    run_cmd.reset_mock()
    md_hdlr('DEVICE_METADATA', 'localhost',
            {'bgp_asn': '100', 'suppress-fib-pending': 'enabled'})
    lines = _collect_vtysh_lines(run_cmd)
    assert 'router bgp 100' in lines, lines
    assert any(line.strip() == 'bgp suppress-fib-pending' for line in lines), lines

    # disabled -> the 'no' form
    run_cmd.reset_mock()
    md_hdlr('DEVICE_METADATA', 'localhost',
            {'bgp_asn': '100', 'suppress-fib-pending': 'disabled'})
    lines = _collect_vtysh_lines(run_cmd)
    assert any(line.strip() == 'no bgp suppress-fib-pending' for line in lines), lines

    # No ASN known yet -> defer rather than emit a malformed 'router bgp None'.
    run_cmd.reset_mock()
    daemon.metadata_asn = None
    md_hdlr('DEVICE_METADATA', 'localhost', {'suppress-fib-pending': 'enabled'})
    lines = _collect_vtysh_lines(run_cmd)
    assert not any('suppress-fib-pending' in line for line in lines), lines


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
