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


def _make_frf_daemon(run_cmd, initial_table=None):
    """Helper: build a BGPConfigDaemon and seed fib_route_filter_state
    from initial_table (a dict of key -> {'route_map': ...}), then reset
    run_cmd so the caller sees only the vtysh it triggers.

    The daemon's own __init__ runs against a mocked config_db that returns
    empty tables, so the state dict starts empty; we reseed it afterwards
    to simulate pre-existing rows without touching the init path.
    """
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()

    initial_table = initial_table or {}
    daemon.fib_route_filter_state = {}
    for k, v in initial_table.items():
        norm_key = '|'.join(k) if isinstance(k, tuple) else str(k)
        rm = v.get('route_map') if isinstance(v, dict) else None
        if rm:
            daemon.fib_route_filter_state[norm_key] = rm
    run_cmd.reset_mock()
    return daemon


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_set_default_vrf_ipv4(run_cmd):
    """SET on default VRF, ipv4, bgp emits 'ip protocol bgp route-map <NAME>'."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_FROM_BGP'})
    assert run_cmd.call_count == 1
    cmd = run_cmd.call_args[0][1]
    assert "configure terminal" in cmd
    assert "ip protocol bgp route-map RM_FROM_BGP" in cmd
    assert "vrf " not in cmd  # default VRF: no 'vrf …' wrapping in the vtysh
    assert daemon.fib_route_filter_state['default|IPv4|bgp'] == 'RM_FROM_BGP'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_set_default_vrf_ipv6_ospf6(run_cmd):
    """IPv6 renders the 'ipv6' keyword instead of 'ip'."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv6|ospf6', {'route_map': 'RM_V6'})
    cmd = run_cmd.call_args[0][1]
    assert "ipv6 protocol ospf6 route-map RM_V6" in cmd
    assert "ip protocol" not in cmd  # should NOT emit ipv4 form
    assert daemon.fib_route_filter_state['default|IPv6|ospf6'] == 'RM_V6'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_set_named_vrf_wraps_in_vrf_block(run_cmd):
    """Non-default VRF wraps the line in 'vrf <N> / ... / exit-vrf'."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'Vrf_red|IPv4|static', {'route_map': 'RM_STATIC'})
    cmd = run_cmd.call_args[0][1]
    assert "vrf Vrf_red" in cmd
    assert "ip protocol static route-map RM_STATIC" in cmd
    assert "exit-vrf" in cmd


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_set_with_tuple_key(run_cmd):
    """swsscommon may deliver compound keys as tuples; handler must accept both."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', ('default', 'IPv4', 'connected'),
        {'route_map': 'RM_CONN'})
    cmd = run_cmd.call_args[0][1]
    assert "ip protocol connected route-map RM_CONN" in cmd
    assert daemon.fib_route_filter_state['default|IPv4|connected'] == 'RM_CONN'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_idempotent_set_skipped(run_cmd):
    """Applying the same route-map twice must emit vtysh only once."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_A'})
    assert run_cmd.call_count == 1
    run_cmd.reset_mock()
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_A'})
    assert run_cmd.call_count == 0


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_update_route_map(run_cmd):
    """Updating to a different route-map emits a fresh binding."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_A'})
    run_cmd.reset_mock()
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_B'})
    assert run_cmd.call_count == 1
    cmd = run_cmd.call_args[0][1]
    assert "ip protocol bgp route-map RM_B" in cmd
    assert "RM_A" not in cmd
    assert daemon.fib_route_filter_state['default|IPv4|bgp'] == 'RM_B'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_delete_emits_no_form(run_cmd):
    """Deletion emits 'no ip protocol <PROTO> route-map <NAME>' using stored RM."""
    daemon = _make_frf_daemon(
        run_cmd,
        initial_table={('default', 'IPv4', 'bgp'): {'route_map': 'RM_FROM_BGP'}},
    )
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', None)
    assert run_cmd.call_count == 1
    cmd = run_cmd.call_args[0][1]
    assert "no ip protocol bgp route-map RM_FROM_BGP" in cmd
    assert 'default|IPv4|bgp' not in daemon.fib_route_filter_state


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_delete_named_vrf(run_cmd):
    """Deletion of a named-VRF row wraps the 'no' in 'vrf <N> / exit-vrf'."""
    daemon = _make_frf_daemon(
        run_cmd,
        initial_table={('Vrf_red', 'IPv4', 'static'): {'route_map': 'RM_STATIC'}},
    )
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'Vrf_red|IPv4|static', None)
    cmd = run_cmd.call_args[0][1]
    assert "vrf Vrf_red" in cmd
    assert "no ip protocol static route-map RM_STATIC" in cmd
    assert "exit-vrf" in cmd


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_delete_untracked_is_noop(run_cmd):
    """Delete on a key never set is a silent noop — no vtysh push."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', None)
    assert run_cmd.call_count == 0


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_malformed_key_is_ignored(run_cmd):
    """Keys with the wrong number of components must be rejected without a push."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|ipv4', {'route_map': 'RM'})
    assert run_cmd.call_count == 0


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_unsupported_afi_is_ignored(run_cmd):
    """addr_family outside IPv4/IPv6 must not produce a vtysh command."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPvX|bgp', {'route_map': 'RM'})
    assert run_cmd.call_count == 0


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_missing_route_map_is_ignored(run_cmd):
    """A SET with no route_map field is rejected — YANG should have blocked
    this, but the daemon must not push a broken vtysh command if it slips through."""
    daemon = _make_frf_daemon(run_cmd)
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {})
    assert run_cmd.call_count == 0


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_registered_in_table_handler_list(run_cmd):
    """Sanity check: FIB_ROUTE_FILTER is wired into table_handler_list so
    CONFIG_DB subscription routes to our handler."""
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    mapped = dict(daemon.table_handler_list)
    assert 'FIB_ROUTE_FILTER' in mapped
    assert mapped['FIB_ROUTE_FILTER'] == daemon.fib_route_filter_handler


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_set_state_not_updated_on_failure(run_cmd):
    """If vtysh fails, the handler must NOT record the new route-map in
    fib_route_filter_state — otherwise a retry would be silently
    short-circuited by the idempotent `prev_rm == route_map` check."""
    daemon = _make_frf_daemon(run_cmd)
    run_cmd.return_value = False  # simulate transient FRR failure
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_A'})
    assert run_cmd.call_count == 1  # command was attempted
    assert 'default|IPv4|bgp' not in daemon.fib_route_filter_state


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_failed_set_is_retryable(run_cmd):
    """After a failed SET, a retry with the same data must re-emit the vtysh
    (not be short-circuited as 'already applied'), and a successful retry
    must then advance state."""
    daemon = _make_frf_daemon(run_cmd)
    # First attempt fails.
    run_cmd.return_value = False
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_A'})
    assert 'default|IPv4|bgp' not in daemon.fib_route_filter_state
    # Retry succeeds.
    run_cmd.return_value = True
    run_cmd.reset_mock()
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_A'})
    assert run_cmd.call_count == 1  # not skipped as idempotent
    assert daemon.fib_route_filter_state['default|IPv4|bgp'] == 'RM_A'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_delete_state_not_updated_on_failure(run_cmd):
    """If the `no ... route-map` vtysh fails, state must still reflect the
    old route-map so a retry can emit the same 'no' form."""
    daemon = _make_frf_daemon(
        run_cmd,
        initial_table={('default', 'IPv4', 'bgp'): {'route_map': 'RM_A'}},
    )
    run_cmd.return_value = False
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', None)
    assert run_cmd.call_count == 1
    # State still shows RM_A so retry can emit `no ... route-map RM_A`.
    assert daemon.fib_route_filter_state['default|IPv4|bgp'] == 'RM_A'


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_frf_update_failure_preserves_prior_route_map(run_cmd):
    """If an update from RM_A -> RM_B fails, state must still show RM_A
    (matching what zebra actually has), not RM_B."""
    daemon = _make_frf_daemon(
        run_cmd,
        initial_table={('default', 'IPv4', 'bgp'): {'route_map': 'RM_A'}},
    )
    run_cmd.return_value = False
    daemon.fib_route_filter_handler(
        'FIB_ROUTE_FILTER', 'default|IPv4|bgp', {'route_map': 'RM_B'})
    assert daemon.fib_route_filter_state['default|IPv4|bgp'] == 'RM_A'
