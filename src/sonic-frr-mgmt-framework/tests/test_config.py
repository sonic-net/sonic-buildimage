import copy
import re
from unittest.mock import MagicMock, NonCallableMagicMock, patch

swsscommon_module_mock = MagicMock(ConfigDBConnector = NonCallableMagicMock)
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
        cmdline = 'vtysh'
        for cmd in cmd_list:
            cmd = cmd.format('no ' if negtive else '')
            cmdline += " -c '%s'" % cmd
        return cmdline
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
        last_cmd = re.findall(r"-c\s+'([^']+)'\s*", cmd)[-1]
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
def test_bmp_single_target_single_collector(run_cmd):
    """Test BMP with single target and single collector."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return BMP configuration with separate tables
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {
                'sonic-bmp': {
                    'mirror': 'false',
                    'stats-interval': '2000'
                }
            }
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {
                ('sonic-bmp', '192.168.1.100', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                }
            }
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {
                ('sonic-bmp', 'ipv4_unicast'): {
                    'adj-rib-in-pre': 'true',
                    'adj-rib-in-post': 'false',
                    'loc-rib': 'false'
                }
            }
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change
    daemon.bmp_handler('BMP_TARGET', 'sonic-bmp', {})

    # Verify commands were called
    assert run_cmd.call_count >= 2

    # Find the command that configures the target
    commands = [call[0][1] for call in run_cmd.call_args_list]
    config_cmd = None
    for cmd in commands:
        if 'bmp targets sonic-bmp' in cmd and 'bmp connect' in cmd:
            config_cmd = cmd
            break

    assert config_cmd is not None
    assert 'router bgp 65000' in config_cmd
    assert 'bmp mirror buffer-limit 4294967214' in commands[1]
    assert 'bmp targets sonic-bmp' in config_cmd
    assert 'bmp connect 192.168.1.100 port 5000 min-retry 30000 max-retry 720000' in config_cmd
    assert 'bmp stats interval 2000' in config_cmd

    # Check AFI/SAFI monitoring command
    monitor_cmd = None
    for cmd in commands:
        if 'bmp monitor ipv4 unicast pre-policy' in cmd:
            monitor_cmd = cmd
            break
    assert monitor_cmd is not None


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_multiple_collectors(run_cmd):
    """Test BMP with single target and multiple collectors."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return BMP configuration with multiple collectors
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {'sonic-bmp': {}}
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {
                ('sonic-bmp', '192.168.1.100', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                },
                ('sonic-bmp', '192.168.1.101', '5001'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                }
            }
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {
                ('sonic-bmp', 'ipv4_unicast'): {
                    'adj-rib-in-pre': 'true',
                    'adj-rib-in-post': 'false',
                    'loc-rib': 'false'
                }
            }
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('sonic-bmp', '192.168.1.101', '5001'), {})

    # Verify commands were called
    assert run_cmd.call_count >= 2

    # Find the command that configures the target with collectors
    commands = [call[0][1] for call in run_cmd.call_args_list]
    config_cmd = None
    for cmd in commands:
        if 'bmp targets sonic-bmp' in cmd and 'bmp connect' in cmd:
            config_cmd = cmd
            break

    assert config_cmd is not None, "Could not find command with 'bmp targets sonic-bmp' and 'bmp connect'"
    # Both collectors should be configured
    assert 'bmp connect 192.168.1.100 port 5000' in config_cmd, f"Collector 1 not found in: {config_cmd}"
    assert 'bmp connect 192.168.1.101 port 5001' in config_cmd, f"Collector 2 not found in: {config_cmd}"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_multiple_policies(run_cmd):
    """Test BMP with multiple policies enabled simultaneously (pre-policy + post-policy)."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return BMP configuration with multiple policies
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {'sonic-bmp': {}}
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {
                ('sonic-bmp', '192.168.1.100', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                }
            }
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {
                ('sonic-bmp', 'ipv4_unicast'): {
                    'adj-rib-in-pre': 'true',
                    'adj-rib-in-post': 'true',  # Both enabled!
                    'loc-rib': 'false'
                }
            }
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change
    daemon.bmp_handler('BMP_TARGET_AFI_SAFI', ('sonic-bmp', 'ipv4_unicast'), {})

    # Verify commands were called
    commands = [call[0][1] for call in run_cmd.call_args_list]

    # Both pre-policy and post-policy should be configured
    pre_policy_found = False
    post_policy_found = False
    for cmd in commands:
        if 'bmp monitor ipv4 unicast pre-policy' in cmd:
            pre_policy_found = True
        if 'bmp monitor ipv4 unicast post-policy' in cmd:
            post_policy_found = True

    assert pre_policy_found, "pre-policy command not found"
    assert post_policy_found, "post-policy command not found"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_all_afi_safi_types(run_cmd):
    """Test BMP with all 7 AFI/SAFI types."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return BMP configuration with all AFI/SAFI types
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {'sonic-bmp': {}}
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {
                ('sonic-bmp', '192.168.1.100', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                }
            }
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {
                ('sonic-bmp', 'ipv4_unicast'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                },
                ('sonic-bmp', 'ipv6_unicast'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                },
                ('sonic-bmp', 'ipv4_multicast'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                },
                ('sonic-bmp', 'ipv6_multicast'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                },
                ('sonic-bmp', 'l2vpn_evpn'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                },
                ('sonic-bmp', 'ipv4_vpn'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                },
                ('sonic-bmp', 'ipv6_vpn'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                }
            }
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change
    daemon.bmp_handler('BMP_TARGET', 'sonic-bmp', {})

    # Verify commands were called
    commands = [call[0][1] for call in run_cmd.call_args_list]

    # Check all AFI/SAFI types are configured
    expected_monitors = [
        'bmp monitor ipv4 unicast pre-policy',
        'bmp monitor ipv6 unicast pre-policy',
        'bmp monitor ipv4 multicast pre-policy',
        'bmp monitor ipv6 multicast pre-policy',
        'bmp monitor l2vpn evpn pre-policy',
        'bmp monitor ipv4 vpn pre-policy',
        'bmp monitor ipv6 vpn pre-policy'
    ]

    for expected in expected_monitors:
        found = any(expected in cmd for cmd in commands)
        assert found, f"Expected monitor command not found: {expected}"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_multiple_targets(run_cmd):
    """Test BMP with multiple targets."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return BMP configuration with multiple targets
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {
                'target1': {},
                'target2': {}
            }
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {
                ('target1', '192.168.1.100', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                },
                ('target2', '192.168.1.101', '5001'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                }
            }
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {
                ('target1', 'ipv4_unicast'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                },
                ('target2', 'ipv6_unicast'): {
                    'adj-rib-in-pre': 'false', 'adj-rib-in-post': 'true', 'loc-rib': 'false'
                }
            }
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change
    daemon.bmp_handler('BMP_TARGET', 'target1', {})

    # Verify commands were called
    commands = [call[0][1] for call in run_cmd.call_args_list]

    # Both targets should be configured
    target1_found = any('bmp targets target1' in cmd for cmd in commands)
    target2_found = any('bmp targets target2' in cmd for cmd in commands)

    assert target1_found, "target1 not found in commands"
    assert target2_found, "target2 not found in commands"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_multiple_vrfs(run_cmd):
    """Test BMP configuration is applied to all VRFs."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up multiple VRFs with BGP ASN
    daemon.bgp_asn = {
        'default': 65000,
        'Vrf_red': 65001,
        'Vrf_blue': 65002
    }

    # Mock config_db.get_table to return BMP configuration
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {'sonic-bmp': {}}
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {
                ('sonic-bmp', '192.168.1.100', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                }
            }
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {
                ('sonic-bmp', 'ipv4_unicast'): {
                    'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'
                }
            }
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change
    daemon.bmp_handler('BMP_TARGET', 'sonic-bmp', {})

    # Verify commands were called for all VRFs
    commands = [call[0][1] for call in run_cmd.call_args_list]

    # Check that each VRF got configured
    default_vrf_found = any('router bgp 65000' in cmd and 'bmp targets sonic-bmp' in cmd for cmd in commands)
    red_vrf_found = any('router bgp 65001 vrf Vrf_red' in cmd and 'bmp targets sonic-bmp' in cmd for cmd in commands)
    blue_vrf_found = any('router bgp 65002 vrf Vrf_blue' in cmd and 'bmp targets sonic-bmp' in cmd for cmd in commands)

    assert default_vrf_found, "Default VRF not configured"
    assert red_vrf_found, "Vrf_red not configured"
    assert blue_vrf_found, "Vrf_blue not configured"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_multiple_afi_safi_per_target(run_cmd):
    """Test BMP with multiple AFI/SAFI configurations per target with different policies."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return BMP configuration with multiple AFI/SAFI
    # Each AFI/SAFI has different policy combinations
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {
                'production': {
                    'mirror': 'true',
                    'stats-interval': '10000'
                }
            }
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {
                ('production', '192.168.1.100', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                },
                ('production', '192.168.1.101', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                }
            }
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {
                # IPv4 unicast: pre-policy only
                ('production', 'ipv4_unicast'): {
                    'adj-rib-in-pre': 'true',
                    'adj-rib-in-post': 'false',
                    'loc-rib': 'false'
                },
                # IPv6 unicast: post-policy only
                ('production', 'ipv6_unicast'): {
                    'adj-rib-in-pre': 'false',
                    'adj-rib-in-post': 'true',
                    'loc-rib': 'false'
                },
                # L2VPN EVPN: both pre-policy and post-policy
                ('production', 'l2vpn_evpn'): {
                    'adj-rib-in-pre': 'true',
                    'adj-rib-in-post': 'true',
                    'loc-rib': 'false'
                },
                # IPv4 VPN: loc-rib only
                ('production', 'ipv4_vpn'): {
                    'adj-rib-in-pre': 'false',
                    'adj-rib-in-post': 'false',
                    'loc-rib': 'true'
                }
            }
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change
    daemon.bmp_handler('BMP_TARGET', 'production', {})

    # Verify commands were called
    commands = [call[0][1] for call in run_cmd.call_args_list]

    # Find the target configuration command (not the "no bmp targets" removal command)
    config_cmd = None
    for cmd in commands:
        if 'bmp targets production' in cmd and 'no bmp targets' not in cmd:
            config_cmd = cmd
            break

    assert config_cmd is not None, "Could not find BMP target configuration command"

    # Verify multiple collectors
    assert 'bmp connect 192.168.1.100 port 5000' in config_cmd
    assert 'bmp connect 192.168.1.101 port 5000' in config_cmd

    # Verify mirror and stats-interval
    assert 'bmp mirror' in config_cmd
    assert 'bmp stats interval 10000' in config_cmd

    # Verify different policies for different AFI/SAFI
    # Note: AFI/SAFI monitoring commands are sent separately
    all_commands = ' '.join(commands)
    assert 'bmp monitor ipv4 unicast pre-policy' in all_commands
    assert 'bmp monitor ipv6 unicast post-policy' in all_commands
    assert 'bmp monitor l2vpn evpn pre-policy' in all_commands
    assert 'bmp monitor l2vpn evpn post-policy' in all_commands
    assert 'bmp monitor ipv4 vpn loc-rib' in all_commands

    # Verify that wrong combinations are NOT present
    assert 'bmp monitor ipv4 unicast post-policy' not in all_commands
    assert 'bmp monitor ipv6 unicast pre-policy' not in all_commands
    assert 'bmp monitor ipv4 vpn pre-policy' not in all_commands


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_no_config_uses_default_sonic_bmp_target(run_cmd):
    """Test backward compatibility: when no BMP config is present, create default 'sonic-bmp' target."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return EMPTY BMP configuration
    daemon.config_db.get_table = MagicMock(return_value={})

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change (e.g., triggered by BGP config change)
    daemon.bmp_handler('BMP', 'table', {})

    # Verify commands were called
    commands = [call[0][1] for call in run_cmd.call_args_list]

    # Should have default "sonic-bmp" target configuration for backward compatibility
    # Default: 127.0.0.1:5000, stats_interval 1000ms, ipv4_unicast pre-policy, ipv6_unicast pre-policy
    all_commands = ' '.join(commands)

    # Verify buffer limit is set
    assert any('bmp mirror buffer-limit 4294967214' in cmd for cmd in commands)

    # Verify default target "sonic-bmp" is created
    assert 'bmp targets sonic-bmp' in all_commands

    # Verify default collector 127.0.0.1:5000
    assert 'bmp connect 127.0.0.1 port 5000' in all_commands

    # Verify default stats interval (1000ms)
    assert 'bmp stats interval 1000' in all_commands

    # Verify default monitoring policies (pre-policy for IPv4 and IPv6 unicast)
    assert 'bmp monitor ipv4 unicast pre-policy' in all_commands
    assert 'bmp monitor ipv6 unicast pre-policy' in all_commands

    # Verify no other policies are configured
    assert 'post-policy' not in all_commands
    assert 'loc-rib' not in all_commands


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_with_no_collectors_or_afi_safi(run_cmd):
    """Test BMP target with no collectors or AFI/SAFI configs (edge case)."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return BMP configuration with target but no collectors/afi-safi
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {
                'empty-target': {
                    'mirror': 'false',
                    'stats-interval': '5000'
                }
            }
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {}  # No collectors
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {}  # No afi-safi entries
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate BMP configuration change
    daemon.bmp_handler('BMP_TARGET', 'empty-target', {})

    # Verify commands were called
    commands = [call[0][1] for call in run_cmd.call_args_list]

    # Should still create the target even without collectors/afi-safi
    config_cmd = None
    for cmd in commands:
        if 'bmp targets empty-target' in cmd and 'no bmp targets' not in cmd:
            config_cmd = cmd
            break

    assert config_cmd is not None, "Target should be created even without collectors/afi-safi"
    assert 'bmp stats interval 5000' in config_cmd
    # Mirror should not be present (default is false)
    assert 'bmp mirror' not in config_cmd


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_deletion(run_cmd):
    """Test BMP target deletion from CONFIG_DB triggers default target creation."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return EMPTY BMP configuration (simulating all targets deleted)
    daemon.config_db.get_table = MagicMock(return_value={})

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate deletion of a BMP target (data=None indicates deletion)
    daemon.bmp_handler('BMP_TARGET', 'production', None)

    # Verify that the deletion command was issued
    commands = [call[0][1] for call in run_cmd.call_args_list]
    all_commands = ' '.join(commands)

    # Should first remove the target
    assert 'no bmp targets production' in all_commands

    # Should then create the default sonic-bmp target (backward compatibility)
    assert 'bmp targets sonic-bmp' in all_commands
    assert '127.0.0.1 port 5000' in all_commands


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_deletion_with_vrf(run_cmd):
    """Test BMP target deletion with VRF triggers default target creation."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN with VRF
    daemon.bgp_asn = {'Vrf1': 65100}

    # Mock config_db.get_table to return EMPTY BMP configuration (simulating all targets deleted)
    daemon.config_db.get_table = MagicMock(return_value={})

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate deletion of a BMP target in VRF
    daemon.bmp_handler('BMP_TARGET', 'test-target', None)

    # Verify that the deletion command was issued
    commands = [call[0][1] for call in run_cmd.call_args_list]
    all_commands = ' '.join(commands)

    # Should first remove the target in the VRF context
    assert 'no bmp targets test-target' in all_commands
    assert 'router bgp 65100 vrf Vrf1' in all_commands

    # Should then create the default sonic-bmp target (backward compatibility)
    assert 'bmp targets sonic-bmp' in all_commands
    assert '127.0.0.1 port 5000' in all_commands


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_collector_deletion_rebuilds_target(run_cmd):
    """Test that deleting a collector rebuilds the entire target configuration."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # Set up BGP ASN
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return BMP configuration with one collector remaining
    def mock_get_table(table_name):
        if table_name == 'BMP':
            return {'global': {'mirror-buffer-limit': '4294967214'}}
        elif table_name == 'BMP_TARGET':
            return {
                'production': {
                    'mirror': 'false',
                    'stats-interval': '2000'
                }
            }
        elif table_name == 'BMP_TARGET_COLLECTOR':
            return {
                # Second collector was deleted, only one remains
                ('production', '192.168.1.100', '5000'): {
                    'min-retry': '30000',
                    'max-retry': '720000'
                }
            }
        elif table_name == 'BMP_TARGET_AFI_SAFI':
            return {
                ('production', 'ipv4_unicast'): {
                    'adj-rib-in-pre': 'true',
                    'adj-rib-in-post': 'false',
                    'loc-rib': 'false'
                }
            }
        return {}

    daemon.config_db.get_table = MagicMock(side_effect=mock_get_table)

    # Reset mock to clear any calls from daemon initialization
    run_cmd.reset_mock()

    # Simulate deletion of a collector (data=None)
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('production', '192.168.1.101', '5000'), None)

    # Verify commands were called
    commands = [call[0][1] for call in run_cmd.call_args_list]
    all_commands = ' '.join(commands)

    # Should remove and re-add the target with updated configuration
    assert 'no bmp targets production' in all_commands or 'no bmp targets sonic-bmp' in all_commands
    assert 'bmp targets production' in all_commands

    # Should have only one collector now
    assert '192.168.1.100 port 5000' in all_commands
    # The deleted collector should not be present
    assert '192.168.1.101' not in all_commands


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_deletion_triggers_default_target(run_cmd):
    """Test that deleting all BMP targets triggers creation of default sonic-bmp target (backward compatibility)."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.bgp_asn = {'default': 65000}

    # Mock config_db.get_table to return EMPTY BMP configuration (simulating all targets deleted)
    daemon.config_db.get_table = MagicMock(return_value={})

    run_cmd.reset_mock()

    # Simulate deletion of the last BMP target - this should trigger creation of default sonic-bmp target
    daemon.bmp_handler('BMP_TARGET', 'custom-target', None)

    # Verify commands were called
    commands = [call[0][1] for call in run_cmd.call_args_list]
    all_commands = ' '.join(commands)

    # Should first remove the custom target
    assert 'no bmp targets custom-target' in all_commands

    # Should then create the default sonic-bmp target (backward compatibility)
    assert 'bmp targets sonic-bmp' in all_commands
    assert '127.0.0.1 port 5000' in all_commands
    assert 'bmp stats interval 1000' in all_commands
    assert 'bmp monitor ipv4 unicast pre-policy' in all_commands
    assert 'bmp monitor ipv6 unicast pre-policy' in all_commands
