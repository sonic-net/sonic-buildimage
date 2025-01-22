import socket
import pytest
from unittest.mock import MagicMock, NonCallableMagicMock, patch

swsscommon_module_mock = MagicMock(ConfigDBConnector = NonCallableMagicMock)
# because can’t use dotted names directly in a call, have to create a dictionary and unpack it using **:
mockmapping = {'swsscommon.swsscommon': swsscommon_module_mock}

with patch.dict('sys.modules', **mockmapping):
    from frrcfgd.frrcfgd import CachedDataWithOp
    from frrcfgd.frrcfgd import BGPPeerGroup
    from frrcfgd.frrcfgd import BGPKeyMapInfo
    from frrcfgd.frrcfgd import BGPKeyMapList
    from frrcfgd.frrcfgd import get_command_cmn
    from frrcfgd.frrcfgd import CommunityList
    from frrcfgd.frrcfgd import MatchPrefix
    from frrcfgd.frrcfgd import MatchPrefixList
    from frrcfgd.frrcfgd import AggregateAddr
    from frrcfgd.frrcfgd import IpNextHop
    from frrcfgd.frrcfgd import IpNextHopSet

def test_data_with_op():
    data = CachedDataWithOp()
    assert(data.data is None)
    assert(data.op == CachedDataWithOp.OP_NONE)
    data = CachedDataWithOp(10, CachedDataWithOp.OP_ADD)
    assert(data.data == 10)
    assert(data.op == CachedDataWithOp.OP_ADD)

def test_peer_group():
    pg = BGPPeerGroup('Vrf_red')
    assert(pg.vrf == 'Vrf_red')
    assert(len(pg.ref_nbrs) == 0)

def test_command_map():
    def cmd_hdlr():
        pass
    key_map1 = BGPKeyMapInfo('test command', cmd_hdlr, None)
    assert(key_map1.daemons is None)
    assert(key_map1.run_cmd == 'test command')
    assert(key_map1.hdl_func == cmd_hdlr)
    assert(key_map1.data is None)
    key_map2 = BGPKeyMapInfo('[bgpd,ospfd]daemon command', None, 100)
    assert(key_map2.daemons == ['bgpd', 'ospfd'])
    assert(key_map2.run_cmd == 'daemon command')
    assert(key_map2.hdl_func == get_command_cmn)
    assert(key_map2.data == 100)

def get_cmd_map_list(attr_list, map_key = None):
    map_list = []
    for attr in attr_list:
        tokens = attr[0].split('|', 1)
        if len(tokens) == 2:
            key = tokens[1]
        else:
            key = None
        if map_key is not None and key is not None and map_key != key:
            continue
        cmd_hdlr = cmd_data = None
        if len(attr) >= 4:
            cmd_hdlr = attr[2]
            cmd_data = attr[3]
        elif len(attr) == 3:
            cmd_data = attr[2]
        map_list.append(BGPKeyMapInfo(attr[1], cmd_hdlr, cmd_data))
    return map_list

def test_command_map_list():
    def cmd_hdlr(num):
        pass
    map_list = [('abc', 'set attribute abc'),
                ('defg', 'system config', cmd_hdlr, 10),
                ('test', 'system test', [20, 30]),
                ('xyz', 'config test'),
                ('ip_cmd|ipv4', 'test on ipv4'),
                ('ip_cmd|ipv6', 'test on ipv6')]
    cmd_map_list = BGPKeyMapList(map_list, 'frrcfg', {'ip_cmd': 'ipv4'})
    chk_map_list = get_cmd_map_list(map_list, 'ipv4')
    for idx, cmd_map in enumerate(cmd_map_list):
        assert(chk_map_list[idx] == cmd_map[1])
    cmd_map_list = BGPKeyMapList(map_list, 'frrcfg', {'ip_cmd': 'ipv6'})
    chk_map_list = get_cmd_map_list(map_list, 'ipv6')
    for idx, cmd_map in enumerate(cmd_map_list):
        assert(chk_map_list[idx] == cmd_map[1])

def test_community_list():
    for ext in [False, True]:
        comm_list = CommunityList('comm', ext)
        assert(comm_list.name == 'comm')
        assert(comm_list.is_ext == ext)
        assert(comm_list.match_action is None)
        assert(comm_list.is_std is None)
        assert(len(comm_list.mbr_list) == 0)

def test_match_prefix():
    pfx = MatchPrefix(socket.AF_INET, '1.2.3.4/16')
    assert(pfx.ip_prefix == '1.2.0.0/16')
    assert(pfx.min_len is None)
    assert(pfx.max_len is None)
    assert(pfx.action == 'permit')
    pfx = MatchPrefix(socket.AF_INET, '10.10.10.10/24', '25..29', 'deny')
    assert(pfx.ip_prefix == '10.10.10.0/24')
    assert(pfx.min_len == 25)
    assert(pfx.max_len == 29)
    assert(pfx.action == 'deny')
    pfx = MatchPrefix(socket.AF_INET, '100.0.0.0/8', '16..32')
    assert(pfx.ip_prefix == '100.0.0.0/8')
    assert(pfx.min_len == 16)
    assert(pfx.max_len is None)
    assert(pfx.action == 'permit')
    pfx = MatchPrefix(socket.AF_INET, '20.20.20.20/28', '25..30', 'deny')
    assert(pfx.ip_prefix == '20.20.20.16/28')
    assert(pfx.min_len is None)
    assert(pfx.max_len == 30)
    assert(pfx.action == 'deny')

    pfx = MatchPrefix(socket.AF_INET6, '1:2::3:4/64')
    assert(pfx.ip_prefix == '1:2::/64')
    assert(pfx.min_len is None)
    assert(pfx.max_len is None)
    assert(pfx.action == 'permit')
    pfx = MatchPrefix(socket.AF_INET6, '10:10::10:10/64', '70..100', 'deny')
    assert(pfx.ip_prefix == '10:10::/64')
    assert(pfx.min_len == 70)
    assert(pfx.max_len == 100)
    assert(pfx.action == 'deny')
    pfx = MatchPrefix(socket.AF_INET6, '1001::/16', '16..128')
    assert(pfx.ip_prefix == '1001::/16')
    assert(pfx.min_len is None)
    assert(pfx.max_len == 128)
    assert(pfx.action == 'permit')

def test_match_prefix_list():
    ipv4_pfx_attrs = [('10.1.1.1/16', '18..24', 'permit'),
                      ('20.2.2.2/24', None, 'deny'),
                      ('30.3.3.3/8', '10..32', 'permit'),
                      ('40.4.4.4/28', '20..30', 'deny')]
    ipv6_pfx_attrs = [('1000:1::1/64', '80..120', 'permit'),
                      ('2000:2::2/96', None, 'deny'),
                      ('3000:3::3/32', '40..128', 'permit'),
                      ('4000:4::4/80', '60..100', 'deny')]

    pfx_list = MatchPrefixList()
    for attr in ipv4_pfx_attrs:
        pfx_list.add_prefix(*attr)
    assert(pfx_list.af == socket.AF_INET)
    assert(len(pfx_list) == len(ipv4_pfx_attrs))
    chk_pfx_list = []
    for attr in ipv4_pfx_attrs:
        chk_pfx_list.append(MatchPrefix(socket.AF_INET, *attr))
    assert(all([x == y for x, y in zip(chk_pfx_list, pfx_list)]))

    pfx_list = MatchPrefixList()
    for attr in ipv6_pfx_attrs:
        pfx_list.add_prefix(*attr)
    assert(pfx_list.af == socket.AF_INET6)
    assert(len(pfx_list) == len(ipv6_pfx_attrs))
    chk_pfx_list = []
    for attr in ipv6_pfx_attrs:
        chk_pfx_list.append(MatchPrefix(socket.AF_INET6, *attr))
    assert(all([x == y for x, y in zip(chk_pfx_list, pfx_list)]))

def test_match_prefix_list_fail():
    pfx_list = MatchPrefixList()
    pfx_list.add_prefix('1.2.3.0/24')
    with pytest.raises(ValueError):
        pfx_list.add_prefix('1::/64')

def test_aggregate_address():
    addr = AggregateAddr()
    assert(addr.as_set == False)
    assert(addr.summary_only == False)

def test_ip_nexthop():
    for af in [socket.AF_INET, socket.AF_INET6]:
        nh = IpNextHop(af, None, None, None, 'Loopback0', None, None, None)
        assert(nh.af == af)
        assert(nh.blackhole == 'false')
        assert(nh.ip == ('0.0.0.0' if af == socket.AF_INET else '::'))
        assert(nh.track == 0)
        assert(nh.interface == 'Loopback0')
        assert(nh.tag == 0)
        assert(nh.distance == 0)
        assert(nh.nh_vrf == '')
        arg_list = nh.get_arg_list()
        assert(arg_list == ['false', '', 'Loopback0'] + [''] * 4)
        nh = IpNextHop(af, 'true', '1.1.1.1' if af == socket.AF_INET else '1::1',
                       1, 'Ethernet0', 100, 2, 'default')
        assert(nh.blackhole == 'true')
        assert(nh.ip == ('0.0.0.0' if af == socket.AF_INET else '::'))
        assert(nh.track == 1)
        assert(nh.interface == '')
        assert(nh.tag == 100)
        assert(nh.distance == 2)
        assert(nh.nh_vrf == '')
        arg_list = nh.get_arg_list()
        assert(arg_list == ['true', '', '', '1', '100', '2', ''])
    nh = IpNextHop(socket.AF_INET, 'false', '1.2.3.4', 5, 'Ethernet1', 2345, 3, 'Vrf_red')
    assert(nh.af == socket.AF_INET)
    assert(nh.blackhole == 'false')
    assert(nh.ip == '1.2.3.4')
    assert(nh.track == 5)
    assert(nh.interface == 'Ethernet1')
    assert(nh.tag == 2345)
    assert(nh.distance == 3)
    assert(nh.nh_vrf == 'Vrf_red')
    arg_list = nh.get_arg_list()
    assert(arg_list == ['false', '1.2.3.4', 'Ethernet1', '5', '2345', '3', 'Vrf_red'])
    nh = IpNextHop(socket.AF_INET6, 'false', '1001:1::2002', 6, 'Ethernet2', 9000, 4, 'Vrf_blue')
    assert(nh.af == socket.AF_INET6)
    assert(nh.blackhole == 'false')
    assert(nh.ip == '1001:1::2002')
    assert(nh.track == 6)
    assert(nh.interface == 'Ethernet2')
    assert(nh.tag == 9000)
    assert(nh.distance == 4)
    assert(nh.nh_vrf == 'Vrf_blue')
    arg_list = nh.get_arg_list()
    assert(arg_list == ['false', '1001:1::2002', 'Ethernet2', '6', '9000', '4', 'Vrf_blue'])

def test_nexthop_set():
    for af in [socket.AF_INET, socket.AF_INET6]:
        nh_set = IpNextHopSet(af)
        bkh_list = ['false', 'false', 'false', 'true']
        ip_list = ['1.1.1.1', '2.2.2.2', '3.3.3.3', None]
        ip6_list = ['1::1', '2::2', '3::3', None]
        intf_list = [None, 'Vlan0', 'Loopback1', None]
        tag_list = [1000, 2000, 3000, 4000]
        vrf_list = ['default', 'Vrf_red', 'Vrf_blue', None]
        nh_set = IpNextHopSet(af, bkh_list, ip_list if af == socket.AF_INET else ip6_list, None,
                              intf_list, tag_list, None, vrf_list)
        test_set = set()
        for idx in range(len(ip_list)):
            test_set.add(IpNextHop(af, bkh_list[idx], ip_list[idx] if af == socket.AF_INET else ip6_list[idx],
                                   None, intf_list[idx], tag_list[idx], None, vrf_list[idx]))
        assert(nh_set == test_set)
