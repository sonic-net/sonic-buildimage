import copy
import re
from unittest.mock import MagicMock, NonCallableMagicMock, call, patch

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
        # cmd is a list: ['vtysh', '-c', ..., '-c', last_cmd]
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

# Create test data for neighbor / peer-group tcp-mss
tcp_mss_data = [
    # Set up BGP globals first
    CmdMapTestInfo('BGP_GLOBALS', 'default',
                  {'local_asn': '100'},
                  conf_bgp_dft_cmd('default', 100),
                  ignore_tail=None),
    # tcp-mss on a neighbor
    CmdMapTestInfo('BGP_NEIGHBOR', 'default|10.3.3.1',
                  {'tcp_mss': '1360'},
                  conf_bgp_cmd('default', 100) + ['{}neighbor 10.3.3.1 tcp-mss 1360']),
    # tcp-mss on a peer-group
    CmdMapTestInfo('BGP_PEER_GROUP', 'default|PG1',
                  {'tcp_mss': '1360'},
                  conf_bgp_cmd('default', 100) + ['{}neighbor PG1 tcp-mss 1360'])
]

@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def data_set_del_test(test_data, run_cmd, skip_del=False):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    CmdMapTestInfo.data_buf.clear()
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

def test_bgp_tcp_mss():
    # Verify tcp-mss command generation for both BGP_NEIGHBOR and BGP_PEER_GROUP
    data_set_del_test(tcp_mss_data, skip_del=True)

bfd_strict_mode_data = [
    CmdMapTestInfo('BGP_GLOBALS', 'default',
                  {'local_asn': '100'},
                  conf_bgp_dft_cmd('default', 100),
                  ignore_tail=None),
    CmdMapTestInfo('BGP_NEIGHBOR', 'default|10.0.0.1',
                  {'bfd': 'true', 'bfd_strict_mode': 'true'},
                  conf_bgp_cmd('default', 100) + ['{}neighbor 10.0.0.1 bfd strict']),
    # Use a distinct peer-group name; PG1 may still be in CmdMapTestInfo.data_buf
    # from test_bgp_tcp_mss (skip_del=True) and would inherit stale tcp_mss.
    CmdMapTestInfo('BGP_PEER_GROUP', 'default|PG_STRICT',
                  {'bfd': 'true', 'bfd_strict_mode': 'false'},
                  conf_bgp_cmd('default', 100) + ['{}no neighbor PG_STRICT bfd strict']),
]

evpn_mh_global_data = [
    CmdMapTestInfo('EVPN_MH_GLOBAL', 'default',
                  {'redirect_off': 'true'},
                  ['configure terminal', '{}evpn mh redirect-off'],
                  ignore_tail=None),
]

evpn_mh_interface_data = [
    CmdMapTestInfo('EVPN_MH_INTERFACE', 'Ethernet120',
                  {'mh_uplink': 'true'},
                  ['configure terminal', 'interface Ethernet120', '{}evpn mh uplink'],
                  ignore_tail=None),
    CmdMapTestInfo('EVPN_MH_INTERFACE', 'PortChannel1',
                  {'bypass': 'true'},
                  ['configure terminal', 'interface PortChannel1', '{}evpn mh bypass'],
                  ignore_tail=None),
]

def test_bgp_bfd_strict_mode():
    data_set_del_test(bfd_strict_mode_data, skip_del=True)

def test_evpn_mh_global_redirect_off():
    data_set_del_test(evpn_mh_global_data)

def test_evpn_mh_interface():
    data_set_del_test(evpn_mh_interface_data)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_evpn_mh_interface_row_delete_preserves_failed_attrs(run_cmd):
    """Row delete must keep programmed attrs when vtysh removal fails."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = False
    daemon = BGPConfigDaemon()
    daemon.evpn_mh_intf_map = {'Ethernet120': {'mh_uplink': True, 'bypass': True}}
    mh_hdlr = [h for t, h in daemon.table_handler_list if t == 'EVPN_MH_INTERFACE'][0]

    mh_hdlr('EVPN_MH_INTERFACE', 'Ethernet120', None)

    assert daemon.evpn_mh_intf_map == {'Ethernet120': {'mh_uplink': True, 'bypass': True}}
    assert run_cmd.call_count == 2


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_evpn_mh_interface_row_delete_clears_untracked_attrs(run_cmd):
    """Row delete must remove boot-programmed attrs not tracked in evpn_mh_intf_map."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    mh_hdlr = [h for t, h in daemon.table_handler_list if t == 'EVPN_MH_INTERFACE'][0]

    mh_hdlr('EVPN_MH_INTERFACE', 'Ethernet220', None)

    assert run_cmd.call_count == 2
    cmds = _get_vtysh_commands(run_cmd)
    assert any('no evpn mh uplink' in c for c in cmds)
    assert any('no evpn mh bypass' in c for c in cmds)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_evpn_mh_global_redirect_off_hdel_from_cache(run_cmd):
    """HDEL redirect_off must clear FRR when cache had it but the flag was not set."""
    from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.evpn_mh_redirect_off = False
    table_key = ExtConfigDBConnector.get_table_key('EVPN_MH_GLOBAL', 'default')
    daemon.table_data_cache[table_key] = {'redirect_off': 'true'}
    mh_hdlr = [h for t, h in daemon.table_handler_list if t == 'EVPN_MH_GLOBAL'][0]

    mh_hdlr('EVPN_MH_GLOBAL', 'default', {})

    run_cmd.assert_called_with(
        'EVPN_MH_GLOBAL',
        CmdMapTestInfo.compose_vtysh_cmd(['configure terminal', 'no evpn mh redirect-off']),
        True, None)
    assert daemon.evpn_mh_redirect_off is False


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_unified_mode_evpn_mh_global_timer_cache_reset_enables_replay(run_cmd):
    """Unified-mode replay: EVPN_MH_GLOBAL table_data_cache must be evicted.

    __init__ pre-populates table_data_cache from CONFIG_DB, so replay would
    otherwise mark timer fields as OP_NONE and skip pushing into empty mgmtd.
    """
    from frrcfgd.frrcfgd import BGPConfigDaemon, ExtConfigDBConnector
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    mh_hdlr = [h for t, h in daemon.table_handler_list if t == 'EVPN_MH_GLOBAL'][0]
    table_key = ExtConfigDBConnector.get_table_key('EVPN_MH_GLOBAL', 'default')
    timer_data = {'startup_delay': '120'}

    daemon.table_data_cache[table_key] = dict(timer_data)
    run_cmd.reset_mock()
    mh_hdlr('EVPN_MH_GLOBAL', 'default', timer_data)
    assert not run_cmd.called, \
        "With pre-populated table_data_cache, timer replay should be skipped (OP_NONE)"

    del daemon.table_data_cache[table_key]
    run_cmd.reset_mock()
    mh_hdlr('EVPN_MH_GLOBAL', 'default', timer_data)
    cmds = _get_vtysh_commands(run_cmd)
    assert any('evpn mh startup-delay 120' in c for c in cmds), \
        "After cache eviction, timer must be pushed to FRR; cmds={}".format(cmds)


# ---------------------------------------------------------------------------
# Tests for the peer-group ordering fix (auto-create + unified-mode cache reset)
# ---------------------------------------------------------------------------

def _get_vtysh_commands(run_cmd_mock):
    """Return searchable vtysh config strings from all g_run_command calls."""
    out = []
    for call in run_cmd_mock.call_args_list:
        cmd = call[0][1]
        if isinstance(cmd, list):
            parts = []
            i = 1
            while i < len(cmd):
                if cmd[i] == '-c' and i + 1 < len(cmd):
                    parts.append(cmd[i + 1])
                    i += 2
                else:
                    i += 1
            out.append(' '.join(parts))
        else:
            out.append(cmd)
    return out


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_pg_auto_create_when_neighbor_before_pg(run_cmd):
    """BGP_NEIGHBOR arrives before BGP_PEER_GROUP (natsorted replay order).

    frrcfgd must auto-create the peer-group in FRR before assigning the
    neighbor to it, so that 'neighbor X peer-group PG' never references an
    unknown peer-group.
    """
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    # local_asn is needed for vtysh commands to be built correctly
    daemon.bgp_asn['default'] = 100
    # PEER_V4 exists in CONFIG_DB but has NOT been sent to FRR yet
    daemon.config_db.get_keys.return_value = [('default', 'PEER_V4')]

    nbr_hdlr = [h for t, h in daemon.table_handler_list if t == 'BGP_NEIGHBOR'][0]
    run_cmd.reset_mock()

    # Simulate: BGP_NEIGHBOR event arrives while bgp_peer_group cache is empty
    nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', {'peer_group_name': 'PEER_V4'})

    cmds = _get_vtysh_commands(run_cmd)

    # Auto-create command must have been sent
    pg_create_idx = next((i for i, c in enumerate(cmds) if 'neighbor PEER_V4 peer-group' in c
                          and 'peer-group PEER_V4' not in c.split('neighbor PEER_V4 peer-group')[0]), None)
    pg_assign_idx = next((i for i, c in enumerate(cmds) if "peer-group PEER_V4" in c
                          and '10.0.0.1' in c), None)

    assert pg_create_idx is not None, \
        "peer-group auto-create 'neighbor PEER_V4 peer-group' was not sent to FRR; cmds={}".format(cmds)
    assert pg_assign_idx is not None, \
        "neighbor-to-peer-group assignment 'neighbor 10.0.0.1 peer-group PEER_V4' not sent; cmds={}".format(cmds)
    assert pg_create_idx < pg_assign_idx, \
        "peer-group must be created before neighbor is assigned; cmds={}".format(cmds)

    # Cache must be updated so subsequent events don't double-create
    assert 'PEER_V4' in daemon.bgp_peer_group.get('default', {}), \
        "PEER_V4 should be tracked in bgp_peer_group cache after auto-create"


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_pg_auto_create_skipped_for_nonexistent_pg(run_cmd):
    """BGP_NEIGHBOR references a peer-group that does not exist in CONFIG_DB.

    The auto-create guard must NOT create a ghost peer-group in FRR.
    frrcfgd still forwards the 'neighbor X peer-group PG' attribute command
    to FRR (which real FRR rejects, since PG was never defined), but the key
    protection is that the 'neighbor PG peer-group' definition command is
    never sent, so FRR stays clean.
    """
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    daemon.bgp_asn['default'] = 100
    # GHOST_PG is not present in CONFIG_DB at all
    daemon.config_db.get_keys.return_value = []

    nbr_hdlr = [h for t, h in daemon.table_handler_list if t == 'BGP_NEIGHBOR'][0]
    run_cmd.reset_mock()

    nbr_hdlr('BGP_NEIGHBOR', 'default|10.0.0.1', {'peer_group_name': 'GHOST_PG'})

    cmds = _get_vtysh_commands(run_cmd)

    # The peer-group definition ('neighbor GHOST_PG peer-group') must NOT be sent.
    # This is the critical guard: without it frrcfgd would silently create a
    # peer-group in FRR with no attributes (no remote-as, etc.) so sessions
    # could never establish.
    assert not any(c.endswith("'neighbor GHOST_PG peer-group'") for c in cmds), \
        "ghost peer-group definition must NOT be sent to FRR; cmds={}".format(cmds)

    # The peer-group must not be tracked in the cache either
    assert 'GHOST_PG' not in daemon.bgp_peer_group.get('default', {}), \
        "GHOST_PG must not appear in bgp_peer_group cache"


def _vrf_vni_vtysh_cmd(vrf, vni=None, remove_vni=None):
  cmds = ['configure terminal', 'vrf {}'.format(vrf)]
  if remove_vni is not None:
    cmds.append('no vni {}'.format(remove_vni))
  if vni is not None:
    cmds.append('vni {}'.format(vni))
  cmds.append('exit-vrf')
  return CmdMapTestInfo.compose_vtysh_cmd(cmds)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_add(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.vrf_vni_map = {}
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    vrf_hdlr('VRF', 'Vrf200', {'vni': '5200'})

    run_cmd.assert_called_once_with('VRF', _vrf_vni_vtysh_cmd('Vrf200', vni='5200'), True, None)
    assert daemon.vrf_vni_map['Vrf200'] == '5200'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_replace(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.vrf_vni_map = {'Vrf200': '200'}
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    vrf_hdlr('VRF', 'Vrf200', {'vni': '100'})

    # Replace is issued as two separately-tracked commands so a partial failure
    # cannot desync FRR from the cache.
    assert run_cmd.call_args_list == [
        call('VRF', _vrf_vni_vtysh_cmd('Vrf200', remove_vni='200'), True, None),
        call('VRF', _vrf_vni_vtysh_cmd('Vrf200', vni='100'), True, None),
    ]
    assert daemon.vrf_vni_map['Vrf200'] == '100'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_replace_add_fails_clears_cache(run_cmd):
    """Replace where the remove succeeds but the add fails must not leave the
    cache on the old VNI (FRR has already removed it); the entry is cleared so a
    later revert to the old VNI is retried as a fresh add instead of skipped."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.vrf_vni_map = {'Vrf200': '200'}
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    # remove old succeeds, add new fails
    run_cmd.side_effect = [True, False]
    vrf_hdlr('VRF', 'Vrf200', {'vni': '100'})
    assert 'Vrf200' not in daemon.vrf_vni_map

    # revert CONFIG_DB to old VNI: cache is empty so it is retried as a fresh add
    run_cmd.reset_mock(side_effect=True)
    run_cmd.return_value = True
    vrf_hdlr('VRF', 'Vrf200', {'vni': '200'})
    run_cmd.assert_called_once_with('VRF', _vrf_vni_vtysh_cmd('Vrf200', vni='200'), True, None)
    assert daemon.vrf_vni_map['Vrf200'] == '200'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_zero_hdel_and_delete(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    daemon.vrf_vni_map = {'Vrf200': '5200'}
    vrf_hdlr('VRF', 'Vrf200', {'vni': '0'})
    run_cmd.assert_called_with('VRF', _vrf_vni_vtysh_cmd('Vrf200', remove_vni='5200'), True, None)
    assert 'Vrf200' not in daemon.vrf_vni_map

    run_cmd.reset_mock()
    daemon.vrf_vni_map = {'Vrf200': '5200'}
    vrf_hdlr('VRF', 'Vrf200', {})
    run_cmd.assert_called_with('VRF', _vrf_vni_vtysh_cmd('Vrf200', remove_vni='5200'), True, None)
    assert 'Vrf200' not in daemon.vrf_vni_map

    run_cmd.reset_mock()
    daemon.vrf_vni_map = {'Vrf200': '5200'}
    vrf_hdlr('VRF', 'Vrf200', None)
    run_cmd.assert_called_with('VRF', _vrf_vni_vtysh_cmd('Vrf200', remove_vni='5200'), True, None)
    assert 'Vrf200' not in daemon.vrf_vni_map


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_zero_to_active_skips_invalid_remove(run_cmd):
    """vni=0 in cache must not emit 'no vni 0' when transitioning to a real VNI."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.vrf_vni_map = {'Vrf200': '0'}
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    vrf_hdlr('VRF', 'Vrf200', {'vni': '5200'})

    run_cmd.assert_called_once_with(
        'VRF', _vrf_vni_vtysh_cmd('Vrf200', vni='5200'), True, None)
    assert daemon.vrf_vni_map['Vrf200'] == '5200'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_idempotent_when_unchanged(run_cmd):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.vrf_vni_map = {'Vrf200': '5200'}
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    vrf_hdlr('VRF', 'Vrf200', {'vni': '5200'})

    run_cmd.assert_not_called()


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_failed_add_not_cached_and_retried(run_cmd):
    """A failed add must not be cached, so a same-VNI retry re-issues the command."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.vrf_vni_map = {}
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    run_cmd.return_value = False
    vrf_hdlr('VRF', 'Vrf200', {'vni': '5200'})
    assert 'Vrf200' not in daemon.vrf_vni_map

    run_cmd.reset_mock()
    run_cmd.return_value = True
    vrf_hdlr('VRF', 'Vrf200', {'vni': '5200'})
    run_cmd.assert_called_once_with('VRF', _vrf_vni_vtysh_cmd('Vrf200', vni='5200'), True, None)
    assert daemon.vrf_vni_map['Vrf200'] == '5200'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_failed_replace_keeps_old(run_cmd):
    """A failed replace must keep the old VNI cached for retry."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.vrf_vni_map = {'Vrf200': '200'}
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    run_cmd.return_value = False
    vrf_hdlr('VRF', 'Vrf200', {'vni': '100'})
    assert daemon.vrf_vni_map['Vrf200'] == '200'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_vrf_vni_failed_delete_keeps_vni(run_cmd):
    """A failed delete must keep the VNI cached since FRR still has it programmed."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    daemon = BGPConfigDaemon()
    daemon.vrf_vni_map = {'Vrf200': '5200'}
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    run_cmd.return_value = False
    vrf_hdlr('VRF', 'Vrf200', None)
    assert daemon.vrf_vni_map['Vrf200'] == '5200'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_unified_mode_vrf_vni_cache_reset_enables_replay(run_cmd):
    """Unified-mode replay: vrf_vni_map must be reset before VRF replay.

    __init__ pre-populates vrf_vni_map from CONFIG_DB, so replay would otherwise
    treat every VNI as already programmed and skip pushing to mgmtd.
    """
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    vrf_hdlr = [h for t, h in daemon.table_handler_list if t == 'VRF'][0]

    daemon.vrf_vni_map = {'Vrf200': '5200'}
    run_cmd.reset_mock()
    vrf_hdlr('VRF', 'Vrf200', {'vni': '5200'})
    assert not run_cmd.called, \
        "With pre-populated cache (no reset), replay should skip unchanged VNI"

    daemon.vrf_vni_map = {}
    run_cmd.reset_mock()
    vrf_hdlr('VRF', 'Vrf200', {'vni': '5200'})
    run_cmd.assert_called_once_with('VRF', _vrf_vni_vtysh_cmd('Vrf200', vni='5200'), True, None)
    assert daemon.vrf_vni_map['Vrf200'] == '5200'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_unified_mode_cache_reset_enables_pg_creation(run_cmd):
    """Unified-mode replay: bgp_peer_group cache must be reset before replay.

    In unified mode bgpd starts empty, but __init__ pre-populates
    bgp_peer_group from CONFIG_DB for ref_nbrs tracking.  Without the reset
    the BGP_PEER_GROUP creation guard (line ~2898) sees the peer-group as
    already present and skips sending 'neighbor PG peer-group' to vtysh,
    leaving FRR with no peer-group config.

    This test verifies the two states explicitly:
      A) cache pre-populated  → creation skipped  (bug scenario)
      B) cache cleared        → creation issued   (fixed scenario)
    """
    from frrcfgd.frrcfgd import BGPConfigDaemon, BGPPeerGroup
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    daemon.bgp_asn['default'] = 100
    pg_hdlr = [h for t, h in daemon.table_handler_list if t == 'BGP_PEER_GROUP'][0]

    # --- Part A: bug scenario (cache pre-populated, no reset) ---
    # Simulate __init__ pre-populating bgp_peer_group from CONFIG_DB
    daemon.bgp_peer_group.setdefault('default', {})['PEER_V4'] = BGPPeerGroup('default')

    run_cmd.reset_mock()
    pg_hdlr('BGP_PEER_GROUP', 'default|PEER_V4', {})
    cmds_with_prepopulated_cache = _get_vtysh_commands(run_cmd)

    assert not any('neighbor PEER_V4 peer-group' in c for c in cmds_with_prepopulated_cache), \
        ("With pre-populated cache (no reset), creation should be skipped "
         "(demonstrates the bug); cmds={}".format(cmds_with_prepopulated_cache))

    # --- Part B: fixed scenario (cache reset, as done in unified-mode __init__) ---
    daemon.bgp_peer_group = {}   # this is the fix at line 2445

    run_cmd.reset_mock()
    pg_hdlr('BGP_PEER_GROUP', 'default|PEER_V4', {})
    cmds_after_reset = _get_vtysh_commands(run_cmd)

    assert any('neighbor PEER_V4 peer-group' in c for c in cmds_after_reset), \
        ("After cache reset, 'neighbor PEER_V4 peer-group' must be sent to FRR; "
         "cmds={}".format(cmds_after_reset))
    assert 'PEER_V4' in daemon.bgp_peer_group.get('default', {}), \
        "PEER_V4 must be tracked in bgp_peer_group cache after creation"
