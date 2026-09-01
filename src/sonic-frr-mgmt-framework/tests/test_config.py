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


def _bmp_get_table_fn(state):
    """Build a config_db.get_table side_effect backed by a mutable dict so a test
    can apply an initial BMP config, then mutate `state` and re-invoke the handler
    to exercise an incremental change/delete against the last-applied shadow."""
    def fn(table_name):
        return state.get(table_name, {})
    return fn


def _render(cmd):
    """Render an argv-list vtysh command back to its shell-string form so the
    substring assertions in some tests keep working after the argv-list
    migration. ['vtysh', '-c', 'configure terminal', ...] ->
    "vtysh -c 'configure terminal' ...". Strings pass through unchanged."""
    if not isinstance(cmd, list):
        return cmd
    out = []
    i = 0
    while i < len(cmd):
        if cmd[i] == '-c' and i + 1 < len(cmd):
            out.append("-c '%s'" % cmd[i + 1])
            i += 2
        else:
            out.append(cmd[i])
            i += 1
    return ' '.join(out)


def _bmp_inner_cmds(run_cmd):
    """Return every individual `-c <line>` config line pushed to vtysh across all
    calls, as exact strings — so assertions can distinguish `bmp monitor ...` from
    `no bmp monitor ...` (one is a substring of the other)."""
    lines = []
    for call in run_cmd.call_args_list:
        argv = call[0][1]
        i = 0
        while i < len(argv):
            if argv[i] == '-c' and i + 1 < len(argv):
                lines.append(argv[i + 1])
                i += 2
            else:
                i += 1
    return lines


def _bmp_apply_full(daemon, state):
    """Fire the per-key BMP events for a whole desired config, the way CONFIG_DB
    delivers them on first write / startup replay: global, then each target, then
    its collectors, then its afi-safis."""
    for k, v in state.get('BMP', {}).items():
        daemon.bmp_handler('BMP', k, v)
    for tkey, tval in state.get('BMP_TARGET', {}).items():
        daemon.bmp_handler('BMP_TARGET', tkey, tval)
    for ckey, cval in state.get('BMP_TARGET_COLLECTOR', {}).items():
        daemon.bmp_handler('BMP_TARGET_COLLECTOR', ckey, cval)
    for akey, aval in state.get('BMP_TARGET_AFI_SAFI', {}).items():
        daemon.bmp_handler('BMP_TARGET_AFI_SAFI', akey, aval)


def _make_bmp_daemon(run_cmd, state, bgp_asn=None):
    from frrcfgd.frrcfgd import BGPConfigDaemon
    run_cmd.return_value = True
    daemon = BGPConfigDaemon()
    daemon.bgp_asn = bgp_asn if bgp_asn is not None else {'default': 65000}
    daemon.config_db.get_table = MagicMock(side_effect=_bmp_get_table_fn(state))
    run_cmd.reset_mock()
    return daemon


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_global_sets_buffer_limit(run_cmd):
    """A BMP|global change pushes only the mirror buffer-limit (per VRF)."""
    state = {'BMP': {'global': {'mirror-buffer-limit': '1000000000'}},
             'BMP_TARGET': {'t1': {}}}
    daemon = _make_bmp_daemon(run_cmd, state)
    daemon.bmp_handler('BMP', 'global', state['BMP']['global'])

    lines = _bmp_inner_cmds(run_cmd)
    assert 'bmp mirror buffer-limit 1000000000' in lines
    assert not any(l.startswith('no bmp targets') for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_create_sets_stats_and_mirror(run_cmd):
    """A BMP_TARGET set creates the target and applies its own attributes only
    (mirror + stats); collectors/monitors arrive as their own events."""
    state = {'BMP_TARGET': {'production': {'mirror': 'true', 'stats-interval': '2000'}}}
    daemon = _make_bmp_daemon(run_cmd, state)
    daemon.bmp_handler('BMP_TARGET', 'production', state['BMP_TARGET']['production'])

    lines = _bmp_inner_cmds(run_cmd)
    assert 'bmp targets production' in lines
    assert 'bmp stats interval 2000' in lines
    assert 'bmp mirror' in lines
    # No collector/monitor churn from a target-attribute change.
    assert not any(l.startswith('bmp connect') for l in lines)
    assert not any(l.startswith('no bmp targets') for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_collector_add(run_cmd):
    """A BMP_TARGET_COLLECTOR set connects exactly that collector under its target,
    preserving the source-interface clause."""
    state = {'BMP_TARGET': {'production': {}},
             'BMP_TARGET_COLLECTOR': {
                 ('production', '192.168.1.100', '5000'): {
                     'min-retry': '30000', 'max-retry': '720000', 'source-interface': 'Loopback0'}}}
    daemon = _make_bmp_daemon(run_cmd, state)
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('production', '192.168.1.100', '5000'),
                       state['BMP_TARGET_COLLECTOR'][('production', '192.168.1.100', '5000')])

    lines = _bmp_inner_cmds(run_cmd)
    assert 'bmp targets production' in lines
    assert 'bmp connect 192.168.1.100 port 5000 min-retry 30000 max-retry 720000 source-interface Loopback0' in lines
    assert not any(l.startswith('no bmp') for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_afi_safi_policies_exact(run_cmd):
    """A BMP_TARGET_AFI_SAFI set re-asserts the row's desired state, one line per
    policy: `bmp monitor` for enabled, `no bmp monitor` for disabled (idempotent in
    FRR). It only ever touches this target's own afi/safi — never other targets or
    any collector connection."""
    state = {'BMP_TARGET': {'production': {}},
             'BMP_TARGET_AFI_SAFI': {
                 ('production', 'ipv4_unicast'): {
                     'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'true'}}}
    daemon = _make_bmp_daemon(run_cmd, state)
    daemon.bmp_handler('BMP_TARGET_AFI_SAFI', ('production', 'ipv4_unicast'),
                       state['BMP_TARGET_AFI_SAFI'][('production', 'ipv4_unicast')])

    lines = _bmp_inner_cmds(run_cmd)
    assert 'bmp monitor ipv4 unicast pre-policy' in lines
    assert 'bmp monitor ipv4 unicast loc-rib' in lines
    # post-policy is disabled -> a `no bmp monitor` (idempotent) but never an enable.
    assert 'no bmp monitor ipv4 unicast post-policy' in lines
    assert 'bmp monitor ipv4 unicast post-policy' not in lines
    # never a session-tearing command
    assert not any(l.startswith('no bmp targets') for l in lines)
    assert not any(l.startswith('no bmp connect') for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_afi_safi_disable_only_affects_that_target(run_cmd):
    """Disabling one policy on a target emits its `no bmp monitor` and never tears
    the target down or touches any other target."""
    state = {
        'BMP_TARGET': {'t1': {}, 't2': {}},
        'BMP_TARGET_AFI_SAFI': {
            ('t1', 'ipv4_unicast'): {'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'true', 'loc-rib': 'false'},
            ('t2', 'ipv4_unicast'): {'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    # Disable post-policy on t1 only.
    run_cmd.reset_mock()
    state['BMP_TARGET_AFI_SAFI'][('t1', 'ipv4_unicast')]['adj-rib-in-post'] = 'false'
    daemon.bmp_handler('BMP_TARGET_AFI_SAFI', ('t1', 'ipv4_unicast'),
                       state['BMP_TARGET_AFI_SAFI'][('t1', 'ipv4_unicast')])

    lines = _bmp_inner_cmds(run_cmd)
    rendered = ' '.join(_render(call[0][1]) for call in run_cmd.call_args_list)
    # post-policy is disabled; pre-policy stays enabled (re-asserted, a FRR no-op).
    assert 'no bmp monitor ipv4 unicast post-policy' in lines
    assert 'bmp monitor ipv4 unicast post-policy' not in lines
    # no session teardown, and the other target is never referenced
    assert not any(l.startswith('no bmp targets') for l in lines)
    assert not any(l.startswith('no bmp connect') for l in lines)
    assert 't2' not in rendered


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_all_afi_safi_types(run_cmd):
    """All 7 AFI/SAFI types map to the right FRR afi/safi keywords."""
    afi_safis = {
        ('production', name): {'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'}
        for name in ['ipv4_unicast', 'ipv6_unicast', 'ipv4_multicast', 'ipv6_multicast',
                     'l2vpn_evpn', 'ipv4_vpn', 'ipv6_vpn']
    }
    state = {'BMP_TARGET': {'production': {}}, 'BMP_TARGET_AFI_SAFI': afi_safis}
    daemon = _make_bmp_daemon(run_cmd, state)
    for akey, aval in afi_safis.items():
        daemon.bmp_handler('BMP_TARGET_AFI_SAFI', akey, aval)

    lines = _bmp_inner_cmds(run_cmd)
    for expected in ['bmp monitor ipv4 unicast pre-policy',
                     'bmp monitor ipv6 unicast pre-policy',
                     'bmp monitor ipv4 multicast pre-policy',
                     'bmp monitor ipv6 multicast pre-policy',
                     'bmp monitor l2vpn evpn pre-policy',
                     'bmp monitor ipv4 vpn pre-policy',
                     'bmp monitor ipv6 vpn pre-policy']:
        assert expected in lines, "missing {}".format(expected)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_applied_to_all_vrfs(run_cmd):
    """A single BMP change is applied to every VRF that has a BGP instance."""
    state = {'BMP_TARGET': {'sonic-bmp': {'stats-interval': '2000'}}}
    daemon = _make_bmp_daemon(run_cmd, state,
                              bgp_asn={'default': 65000, 'Vrf_red': 65001, 'Vrf_blue': 65002})
    daemon.bmp_handler('BMP_TARGET', 'sonic-bmp', state['BMP_TARGET']['sonic-bmp'])

    rendered = [_render(call[0][1]) for call in run_cmd.call_args_list]
    assert any('router bgp 65000' in c and 'bmp targets sonic-bmp' in c for c in rendered)
    assert any('router bgp 65001 vrf Vrf_red' in c and 'bmp targets sonic-bmp' in c for c in rendered)
    assert any('router bgp 65002 vrf Vrf_blue' in c and 'bmp targets sonic-bmp' in c for c in rendered)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_full_apply_via_per_key_events(run_cmd):
    """Applying a complete two-target config through the natural per-key event
    stream produces every expected line and, crucially, NEVER tears a target down
    — even though multiple targets/collectors are configured."""
    state = {
        'BMP': {'global': {'mirror-buffer-limit': '4294967214'}},
        'BMP_TARGET': {'production': {'mirror': 'false', 'stats-interval': '2000'},
                       'troubleshooting': {'mirror': 'true', 'stats-interval': '500'}},
        'BMP_TARGET_COLLECTOR': {
            ('production', '192.168.1.100', '5000'): {'min-retry': '30000', 'max-retry': '720000'},
            ('troubleshooting', '10.0.0.1', '6000'): {
                'min-retry': '20000', 'max-retry': '600000', 'source-interface': 'Loopback0'},
        },
        'BMP_TARGET_AFI_SAFI': {
            ('production', 'ipv4_unicast'): {'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'},
            ('troubleshooting', 'l2vpn_evpn'): {'adj-rib-in-pre': 'false', 'adj-rib-in-post': 'false', 'loc-rib': 'true'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    lines = _bmp_inner_cmds(run_cmd)
    assert 'bmp mirror buffer-limit 4294967214' in lines
    assert 'bmp targets production' in lines
    assert 'bmp targets troubleshooting' in lines
    assert 'bmp connect 192.168.1.100 port 5000 min-retry 30000 max-retry 720000' in lines
    assert 'bmp connect 10.0.0.1 port 6000 min-retry 20000 max-retry 600000 source-interface Loopback0' in lines
    assert 'bmp stats interval 500' in lines
    assert 'bmp monitor ipv4 unicast pre-policy' in lines
    assert 'bmp monitor l2vpn evpn loc-rib' in lines
    # A fresh apply must not delete/tear down anything.
    assert not any(l.startswith('no bmp targets') for l in lines)
    assert not any(l.startswith('no bmp connect') for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_no_config_uses_default_sonic_bmp_target(run_cmd):
    """Backward compatibility: a BMP event with no BMP_TARGET rows (re)creates the
    default sonic-bmp target, matching the startup template."""
    state = {}  # every get_table returns {}
    daemon = _make_bmp_daemon(run_cmd, state)
    daemon.bmp_handler('BMP', 'global', {})

    lines = _bmp_inner_cmds(run_cmd)
    assert 'bmp mirror buffer-limit 4294967214' in lines
    assert 'bmp targets sonic-bmp' in lines
    assert 'bmp connect 127.0.0.1 port 5000 min-retry 10000 max-retry 15000' in lines
    assert 'bmp stats interval 1000' in lines
    assert 'bmp monitor ipv4 unicast pre-policy' in lines
    assert 'bmp monitor ipv6 unicast pre-policy' in lines
    assert not any('source-interface' in l for l in lines)
    assert not any('post-policy' in l for l in lines)
    assert not any('loc-rib' in l for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_with_no_collectors_or_afi_safi(run_cmd):
    """A target with no collectors/afi-safi is still created with its stats."""
    state = {'BMP_TARGET': {'empty-target': {'mirror': 'false', 'stats-interval': '5000'}}}
    daemon = _make_bmp_daemon(run_cmd, state)
    daemon.bmp_handler('BMP_TARGET', 'empty-target', state['BMP_TARGET']['empty-target'])

    lines = _bmp_inner_cmds(run_cmd)
    assert 'bmp targets empty-target' in lines
    assert 'bmp stats interval 5000' in lines
    # mirror is false -> `no bmp mirror` (idempotent), never the enable form
    assert 'bmp mirror' not in lines
    assert 'no bmp mirror' in lines


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_deletion(run_cmd):
    """Deleting the only target removes exactly that target and falls back to the
    default sonic-bmp target — nothing else is touched."""
    state = {
        'BMP': {'global': {'mirror-buffer-limit': '4294967214'}},
        'BMP_TARGET': {'production': {'stats-interval': '2000'}},
        'BMP_TARGET_COLLECTOR': {('production', '192.168.1.100', '5000'): {'min-retry': '30000', 'max-retry': '720000'}},
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    run_cmd.reset_mock()
    state['BMP_TARGET'] = {}
    state['BMP_TARGET_COLLECTOR'] = {}
    daemon.bmp_handler('BMP_TARGET', 'production', None)

    lines = _bmp_inner_cmds(run_cmd)
    assert 'no bmp targets production' in lines
    assert 'bmp targets sonic-bmp' in lines
    assert 'bmp connect 127.0.0.1 port 5000 min-retry 10000 max-retry 15000' in lines


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_deletion_with_vrf(run_cmd):
    """Target deletion in a non-default VRF is scoped to that VRF's context."""
    state = {'BMP_TARGET': {'test-target': {'stats-interval': '2000'}},
             'BMP_TARGET_COLLECTOR': {('test-target', '192.168.1.100', '5000'): {'min-retry': '30000', 'max-retry': '720000'}}}
    daemon = _make_bmp_daemon(run_cmd, state, bgp_asn={'Vrf1': 65100})
    _bmp_apply_full(daemon, state)

    run_cmd.reset_mock()
    state['BMP_TARGET'] = {}
    state['BMP_TARGET_COLLECTOR'] = {}
    daemon.bmp_handler('BMP_TARGET', 'test-target', None)

    rendered = [_render(call[0][1]) for call in run_cmd.call_args_list]
    assert any('router bgp 65100 vrf Vrf1' in c and 'no bmp targets test-target' in c for c in rendered)
    assert any('bmp targets sonic-bmp' in c for c in rendered)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_collector_deletion_removes_only_that_collector(run_cmd):
    """Deleting one collector removes ONLY that collector's connection and never
    tears down the target (which would reset every other collector)."""
    state = {
        'BMP_TARGET': {'production': {'stats-interval': '2000'}},
        'BMP_TARGET_COLLECTOR': {
            ('production', '192.168.1.100', '5000'): {'min-retry': '30000', 'max-retry': '720000', 'source-interface': 'Loopback0'},
            ('production', '192.168.1.101', '5000'): {'min-retry': '30000', 'max-retry': '720000'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    run_cmd.reset_mock()
    del state['BMP_TARGET_COLLECTOR'][('production', '192.168.1.101', '5000')]
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('production', '192.168.1.101', '5000'), None)

    lines = _bmp_inner_cmds(run_cmd)
    assert not any(l.startswith('no bmp targets') for l in lines)
    assert 'no bmp connect 192.168.1.101 port 5000' in lines
    # the surviving collector's connection is never re-issued
    assert not any(l.startswith('bmp connect 192.168.1.100') for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_collector_deletion_preserves_source_interface(run_cmd):
    """The `no bmp connect` for a deleted collector must repeat its source-interface
    so FRR can match and remove the right connection."""
    state = {
        'BMP_TARGET': {'production': {}},
        'BMP_TARGET_COLLECTOR': {
            ('production', '192.168.1.100', '5000'): {'min-retry': '30000', 'max-retry': '720000', 'source-interface': 'Loopback5'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    run_cmd.reset_mock()
    del state['BMP_TARGET_COLLECTOR'][('production', '192.168.1.100', '5000')]
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('production', '192.168.1.100', '5000'), None)

    lines = _bmp_inner_cmds(run_cmd)
    assert 'no bmp connect 192.168.1.100 port 5000 source-interface Loopback5' in lines


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_collector_srcif_updated_then_deleted(run_cmd):
    """After a collector's source-interface is updated, a later delete must negate
    with the NEW source-interface (the memory is overwritten on the update event),
    not the one it was first created with."""
    state = {
        'BMP_TARGET': {'production': {}},
        'BMP_TARGET_COLLECTOR': {
            ('production', '192.168.1.100', '5000'): {
                'min-retry': '30000', 'max-retry': '720000', 'source-interface': 'Loopback0'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    # Update the collector's source-interface Loopback0 -> Loopback1.
    state['BMP_TARGET_COLLECTOR'][('production', '192.168.1.100', '5000')]['source-interface'] = 'Loopback1'
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('production', '192.168.1.100', '5000'),
                       state['BMP_TARGET_COLLECTOR'][('production', '192.168.1.100', '5000')])

    # Now delete it; the `no bmp connect` must carry Loopback1.
    run_cmd.reset_mock()
    del state['BMP_TARGET_COLLECTOR'][('production', '192.168.1.100', '5000')]
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('production', '192.168.1.100', '5000'), None)

    lines = _bmp_inner_cmds(run_cmd)
    assert 'no bmp connect 192.168.1.100 port 5000 source-interface Loopback1' in lines
    assert not any('Loopback0' in l for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_target_deletion_triggers_default_target(run_cmd):
    """Deleting the last custom target falls back to the default sonic-bmp target."""
    state = {'BMP_TARGET': {'custom-target': {'stats-interval': '2000'}},
             'BMP_TARGET_COLLECTOR': {('custom-target', '192.168.1.100', '5000'): {'min-retry': '30000', 'max-retry': '720000'}}}
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    run_cmd.reset_mock()
    state['BMP_TARGET'] = {}
    state['BMP_TARGET_COLLECTOR'] = {}
    daemon.bmp_handler('BMP_TARGET', 'custom-target', None)

    lines = _bmp_inner_cmds(run_cmd)
    assert 'no bmp targets custom-target' in lines
    assert 'bmp targets sonic-bmp' in lines
    assert 'bmp connect 127.0.0.1 port 5000 min-retry 10000 max-retry 15000' in lines
    assert 'bmp stats interval 1000' in lines
    assert 'bmp monitor ipv4 unicast pre-policy' in lines
    assert 'bmp monitor ipv6 unicast pre-policy' in lines


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_afi_safi_change_does_not_reset_other_sessions(run_cmd):
    """Core regression: enabling a monitoring option on ONE target pushes that
    target's `bmp monitor` change and NEVER emits `no bmp targets` / `no bmp
    connect`, so no collector session (on this or any other target) drops. The
    unmodified target is not mentioned at all."""
    state = {
        'BMP': {'global': {'mirror-buffer-limit': '4294967214'}},
        'BMP_TARGET': {'ts101-openbmp': {'stats-interval': '2000'},
                       'ts102-openbmp': {'stats-interval': '2000'}},
        'BMP_TARGET_COLLECTOR': {
            ('ts101-openbmp', '10.0.0.1', '5000'): {'min-retry': '30000', 'max-retry': '720000'},
            ('ts102-openbmp', '10.0.0.2', '5000'): {'min-retry': '30000', 'max-retry': '720000'},
        },
        'BMP_TARGET_AFI_SAFI': {
            ('ts101-openbmp', 'ipv4_unicast'): {'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'},
            ('ts102-openbmp', 'ipv4_unicast'): {'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    # The ticket's trigger: enable adj-rib-in-post on ts101-openbmp only.
    run_cmd.reset_mock()
    state['BMP_TARGET_AFI_SAFI'][('ts101-openbmp', 'ipv4_unicast')]['adj-rib-in-post'] = 'true'
    daemon.bmp_handler('BMP_TARGET_AFI_SAFI', ('ts101-openbmp', 'ipv4_unicast'),
                       state['BMP_TARGET_AFI_SAFI'][('ts101-openbmp', 'ipv4_unicast')])

    lines = _bmp_inner_cmds(run_cmd)
    rendered = ' '.join(_render(call[0][1]) for call in run_cmd.call_args_list)

    # The core invariant: no session-tearing command is ever emitted, so no
    # collector session is dropped on any target.
    assert not any(l.startswith('no bmp targets') for l in lines), \
        "config change must not tear down any BMP target: {}".format(lines)
    assert not any(l.startswith('no bmp connect') for l in lines), \
        "config change must not drop any collector connection: {}".format(lines)
    # The untouched target is never mentioned in any way.
    assert 'ts102-openbmp' not in rendered, "an unmodified target must not be reconfigured"
    # The new monitor is enabled on the changed target (and only monitor lines for
    # this one afi/safi are touched).
    assert 'bmp monitor ipv4 unicast post-policy' in lines
    assert all(('ipv4 unicast' in l) for l in lines if 'bmp monitor' in l)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_collector_add_does_not_reset_existing(run_cmd):
    """Adding a collector emits only the new `bmp connect`; the existing collector
    is neither re-issued nor dropped, and the target is not torn down."""
    state = {
        'BMP_TARGET': {'production': {'stats-interval': '2000'}},
        'BMP_TARGET_COLLECTOR': {
            ('production', '192.168.1.100', '5000'): {'min-retry': '30000', 'max-retry': '720000'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    run_cmd.reset_mock()
    state['BMP_TARGET_COLLECTOR'][('production', '192.168.1.101', '5001')] = {
        'min-retry': '30000', 'max-retry': '720000'}
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('production', '192.168.1.101', '5001'),
                       state['BMP_TARGET_COLLECTOR'][('production', '192.168.1.101', '5001')])

    lines = _bmp_inner_cmds(run_cmd)
    assert not any(l.startswith('no bmp targets') for l in lines)
    assert not any(l.startswith('no bmp connect') for l in lines)
    assert 'bmp connect 192.168.1.101 port 5001 min-retry 30000 max-retry 720000' in lines
    assert not any(l.startswith('bmp connect 192.168.1.100') for l in lines)


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_stale_collector_delete_after_target_delete_no_ghost(run_cmd):
    """A BMP_TARGET_COLLECTOR delete that arrives AFTER the target itself was
    deleted must be a no-op — it must not resurrect the target via the
    create-or-enter `bmp targets <t>`."""
    state = {
        'BMP_TARGET': {'production': {}},
        'BMP_TARGET_COLLECTOR': {
            ('production', '192.168.1.100', '5000'): {
                'min-retry': '30000', 'max-retry': '720000', 'source-interface': 'Loopback0'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    # Target and its collector both removed from CONFIG_DB; target delete fires first.
    state['BMP_TARGET'] = {}
    state['BMP_TARGET_COLLECTOR'] = {}
    daemon.bmp_handler('BMP_TARGET', 'production', None)

    # Now a stale collector delete for the already-removed target arrives.
    run_cmd.reset_mock()
    daemon.bmp_handler('BMP_TARGET_COLLECTOR', ('production', '192.168.1.100', '5000'), None)

    lines = _bmp_inner_cmds(run_cmd)
    # Nothing at all should be pushed: no ghost `bmp targets`, no `no bmp connect`.
    assert lines == [], lines


@patch.dict('sys.modules', **mockmapping)
@patch('frrcfgd.frrcfgd.g_run_command')
def test_bmp_stale_afi_safi_delete_after_target_delete_no_ghost(run_cmd):
    """A BMP_TARGET_AFI_SAFI delete that arrives AFTER the target was deleted must
    be a no-op, not resurrect the target with an empty monitor stanza."""
    state = {
        'BMP_TARGET': {'production': {}},
        'BMP_TARGET_AFI_SAFI': {
            ('production', 'ipv4_unicast'): {'adj-rib-in-pre': 'true', 'adj-rib-in-post': 'false', 'loc-rib': 'false'},
        },
    }
    daemon = _make_bmp_daemon(run_cmd, state)
    _bmp_apply_full(daemon, state)

    state['BMP_TARGET'] = {}
    state['BMP_TARGET_AFI_SAFI'] = {}
    daemon.bmp_handler('BMP_TARGET', 'production', None)

    run_cmd.reset_mock()
    daemon.bmp_handler('BMP_TARGET_AFI_SAFI', ('production', 'ipv4_unicast'), None)

    lines = _bmp_inner_cmds(run_cmd)
    assert lines == [], lines




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
