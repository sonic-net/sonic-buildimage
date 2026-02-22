from unittest.mock import MagicMock, patch

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_static_rt import StaticRouteMgr
from collections import Counter
from swsscommon import swsscommon

def constructor(skip_bgp_asn=False):
    cfg_mgr = MagicMock()

    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }

    mgr = StaticRouteMgr(common_objs, "CONFIG_DB", "STATIC_ROUTE")
    if not skip_bgp_asn:
        mgr.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", {"bgp_asn": "65100"})
    assert len(mgr.static_routes) == 0

    return mgr

def set_del_test(mgr, op, args, expected_ret, expected_cmds):
    set_del_test.push_list_called = False
    def push_list(cmds):
        set_del_test.push_list_called = True
        assert Counter(cmds) == Counter(expected_cmds) # check if commands are expected (regardless of the order)
        max_del_idx = -1
        min_set_idx = len(cmds)
        for idx in range(len(cmds)):
            if cmds[idx].startswith('no ip') and idx > max_del_idx:
                max_del_idx = idx
            if cmds[idx].startswith('ip') and idx < min_set_idx:
                min_set_idx = idx
        assert max_del_idx < min_set_idx, "DEL command comes after SET command" # DEL commands should be done first
        return True
    mgr.cfg_mgr.push_list = push_list

    if op == "SET":
        ret = mgr.set_handler(*args)
        assert ret == expected_ret
    elif op == "DEL":
        mgr.del_handler(*args)
    else:
        assert False, "Wrong operation"

    if expected_cmds:
        assert set_del_test.push_list_called, "cfg_mgr.push_list wasn't called"
    else:
        assert not set_del_test.push_list_called, "cfg_mgr.push_list was called"

def test_set_ipv4_with_sidlist():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|10.1.3.0/24", {
            "sidlist": "fcbb:bbbb:1:2:3:4:5:6",
            "ifname": "Ethernet8",
        }),
        True,
        [
            "ip route 10.1.3.0/24 Ethernet8 segments fcbb:bbbb:1:2:3:4:5:6 tag 1",
            "route-map STATIC_ROUTE_FILTER permit 10",
            " match tag 1",
            "router bgp 65100",
            " address-family ipv4",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit"
        ]
    )

def test_set_ipv4_with_sidlist_multiple_sids():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|10.1.3.0/24", {
            "sidlist": "fcbb:bbbb:1:2:3:4:5:6,fcbb:bbbb:9:10::",
            "ifname": "Ethernet8",
        }),
        True,
        [
            "ip route 10.1.3.0/24 Ethernet8 segments fcbb:bbbb:1:2:3:4:5:6/fcbb:bbbb:9:10:: tag 1",
            "route-map STATIC_ROUTE_FILTER permit 10",
            " match tag 1",
            "router bgp 65100",
            " address-family ipv4",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit"
        ]
    )

def test_set_ipv4_with_sidlist_non_default_vrf():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "sidlist": "fcbb:bbbb:1:2:3:4:5:6,fcbb:bbbb:9:10::",
            "ifname": "Ethernet8",
        }),
        True,
        [
            "ip route 10.1.3.0/24 Ethernet8 segments fcbb:bbbb:1:2:3:4:5:6/fcbb:bbbb:9:10:: vrf vrfRED tag 1",
            "route-map STATIC_ROUTE_FILTER permit 10",
            " match tag 1",
            "router bgp 65100 vrf vrfRED",
            " address-family ipv4",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit"
        ]
    )

def test_set_ipv4_with_sidlist_ecmp():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "sidlist": "fcbb:bbbb:1:2:3:4:5:6,fcbb:bbbb:9:10::|fcbb:bbbb:11:12:13:14:15:16,fcbb:bbbb:19:1a::",
            "ifname": "Ethernet8,Ethernet16",
        }),
        True,
        [
            "ip route 10.1.3.0/24 Ethernet8 segments fcbb:bbbb:1:2:3:4:5:6/fcbb:bbbb:9:10:: vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 Ethernet16 segments fcbb:bbbb:11:12:13:14:15:16/fcbb:bbbb:19:1a:: vrf vrfRED tag 1",
            "route-map STATIC_ROUTE_FILTER permit 10",
            " match tag 1",
            "router bgp 65100 vrf vrfRED",
            " address-family ipv4",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit"
        ]
    )

def test_set_ipv6_with_sidlist():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|2001:db8:1:1::/64", {
            "sidlist": "fcbb:bbbb:1:2:3:4:5:6",
            "ifname": "Ethernet8",
        }),
        True,
        [
            "ipv6 route 2001:db8:1:1::/64 Ethernet8 segments fcbb:bbbb:1:2:3:4:5:6 tag 1",
            "route-map STATIC_ROUTE_FILTER permit 10",
            " match tag 1",
            "router bgp 65100",
            " address-family ipv4",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit"
        ]
    )

def test_set_ipv6_with_sidlist_multiple_sids():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|2001:db8:1:1::/64", {
            "sidlist": "fcbb:bbbb:1:2:3:4:5:6,fcbb:bbbb:9:10::",
            "ifname": "Ethernet8",
        }),
        True,
        [
            "ipv6 route 2001:db8:1:1::/64 Ethernet8 segments fcbb:bbbb:1:2:3:4:5:6/fcbb:bbbb:9:10:: tag 1",
            "route-map STATIC_ROUTE_FILTER permit 10",
            " match tag 1",
            "router bgp 65100",
            " address-family ipv4",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit"
        ]
    )

def test_set_ipv6_with_sidlist_non_default_vrf():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|2001:db8:1:1::/64", {
            "sidlist": "fcbb:bbbb:1:2:3:4:5:6,fcbb:bbbb:9:10::",
            "ifname": "Ethernet8",
        }),
        True,
        [
            "ipv6 route 2001:db8:1:1::/64 Ethernet8 segments fcbb:bbbb:1:2:3:4:5:6/fcbb:bbbb:9:10:: vrf vrfRED tag 1",
            "route-map STATIC_ROUTE_FILTER permit 10",
            " match tag 1",
            "router bgp 65100 vrf vrfRED",
            " address-family ipv4",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit"
        ]
    )

def test_set_ipv6_with_sidlist_ecmp():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|2001:db8:1:1::/64", {
            "sidlist": "fcbb:bbbb:1:2:3:4:5:6,fcbb:bbbb:9:10::|fcbb:bbbb:11:12:13:14:15:16,fcbb:bbbb:19:1a::",
            "ifname": "Ethernet8,Ethernet16",
        }),
        True,
        [
            "ipv6 route 2001:db8:1:1::/64 Ethernet8 segments fcbb:bbbb:1:2:3:4:5:6/fcbb:bbbb:9:10:: vrf vrfRED tag 1",
            "ipv6 route 2001:db8:1:1::/64 Ethernet16 segments fcbb:bbbb:11:12:13:14:15:16/fcbb:bbbb:19:1a:: vrf vrfRED tag 1",
            "route-map STATIC_ROUTE_FILTER permit 10",
            " match tag 1",
            "router bgp 65100 vrf vrfRED",
            " address-family ipv4",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit"
        ]
    )
