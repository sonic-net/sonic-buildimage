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

def test_set():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "10.0.0.57",
        }),
        True,
        [
            "ip route 10.1.0.0/24 10.0.0.57 tag 1",
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

@patch('bgpcfgd.managers_static_rt.log_debug')
def test_del_for_appl(mocked_log_debug):
    class MockRedisConfigDbGet:
        def __init__(self, cache=dict()):
            self.cache = cache
            self.CONFIG_DB = "CONFIG_DB"

        def get(self, db, key, field):
            if key in self.cache:
                if field in self.cache[key]["value"]:
                    return self.cache[key]["value"][field]
            return None  # return nil

    mgr = constructor()

    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "PortChannel0001",
        }),
        True,
        [
            "ip route 10.1.0.0/24 PortChannel0001 tag 1",
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

    #from "APPL_DB" instance, static route can not be uninstalled if the static route exists in config_db and "bfd"="false" (or no bfd field)
    mgr.db_name = "APPL_DB"
    cfg_db_cache = {
        "STATIC_ROUTE|10.1.0.0/24": {
            "value": {
                "advertise": "false",
                "nexthop": "PortChannel0001"
            }
        }
    }
    mgr.config_db = MockRedisConfigDbGet(cfg_db_cache)

    set_del_test(
        mgr,
        "DEL",
        ("10.1.0.0/24",),
        True,
        []
    )
    mocked_log_debug.assert_called_with("{} ignore appl_db static route deletion because of key {} exist in config_db and bfd is not true".format(mgr.db_name, "10.1.0.0/24"))

    cfg_db_cache = {
        "STATIC_ROUTE|10.1.0.0/24": {
            "value": {
                "advertise": "false",
                "bfd": "false",
                "nexthop": "PortChannel0001"
            }
        }
    }
    mgr.db_name = "APPL_DB"
    mgr.config_db = MockRedisConfigDbGet(cfg_db_cache)

    set_del_test(
        mgr,
        "DEL",
        ("10.1.0.0/24",),
        True,
        []
    )
    mocked_log_debug.assert_called_with("{} ignore appl_db static route deletion because of key {} exist in config_db and bfd is not true".format(mgr.db_name, "10.1.0.0/24"))

    #From "APPL_DB" instance, static route can be deleted if bfd field is true in config_db
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "PortChannel0001",
        }),
        True,
        [
            "ip route 10.1.0.0/24 PortChannel0001 tag 1",
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
    cfg_db_cache = {
        "STATIC_ROUTE|10.1.0.0/24": {
            "value": {
                "advertise": "false",
                "bfd": "true",
                "nexthop": "PortChannel0001"
            }
        }
    }
    mgr.db_name = "APPL_DB"
    mgr.config_db = MockRedisConfigDbGet(cfg_db_cache)
    set_del_test(
        mgr,
        "DEL",
        ("10.1.0.0/24",),
        True,
        [
            "no ip route 10.1.0.0/24 PortChannel0001 tag 1",
            "router bgp 65100",
            " address-family ipv4",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit",
            "no route-map STATIC_ROUTE_FILTER"
        ]
    )

    #From "APPL_DB" instance, static route can be deleted if the static route does not in config_db
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "PortChannel0001",
        }),
        True,
        [
            "ip route 10.1.0.0/24 PortChannel0001 tag 1",
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

    cfg_db_cache = {}
    mgr.db_name = "APPL_DB"
    mgr.config_db = MockRedisConfigDbGet(cfg_db_cache)
    set_del_test(
        mgr,
        "DEL",
        ("10.1.0.0/24",),
        True,
        [
            "no ip route 10.1.0.0/24 PortChannel0001 tag 1",
            "router bgp 65100",
            " address-family ipv4",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit",
            "no route-map STATIC_ROUTE_FILTER"
        ]
    )

def test_set_nhportchannel():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "PortChannel0001",
        }),
        True,
        [
            "ip route 10.1.0.0/24 PortChannel0001 tag 1",
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

    set_del_test(
        mgr,
        "DEL",
        ("10.1.0.0/24",),
        True,
        [
            "no ip route 10.1.0.0/24 PortChannel0001 tag 1",
            "router bgp 65100",
            " address-family ipv4",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit",
            "no route-map STATIC_ROUTE_FILTER"
        ]
    )

def test_set_several_nhportchannels():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "PortChannel0003,PortChannel0004",
        }),
        True,
        [
            "ip route 10.1.0.0/24 PortChannel0003 tag 1",
            "ip route 10.1.0.0/24 PortChannel0004 tag 1",
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

def test_set_nhvrf():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|10.1.1.0/24", {
            "nexthop": "10.0.0.57",
            "ifname": "PortChannel0001",
            "distance": "10",
            "nexthop-vrf": "nh_vrf",
            "blackhole": "false",
        }),
        True,
        [
            "ip route 10.1.1.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf tag 1",
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

def test_set_blackhole():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|10.1.2.0/24", {
            "nexthop": "10.0.0.57",
            "ifname": "PortChannel0001",
            "distance": "10",
            "nexthop-vrf": "nh_vrf",
            "blackhole": "true",
        }),
        True,
        [
            "ip route 10.1.2.0/24 blackhole 10 tag 1",
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

def test_set_vrf():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57",
            "ifname": "PortChannel0001",
            "distance": "10",
            "nexthop-vrf": "nh_vrf",
            "blackhole": "false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
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

def test_set_ipv6():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|fc00:10::/64", {
            "nexthop": "fc00::72",
            "ifname": "PortChannel0001",
            "distance": "10",
            "nexthop-vrf": "",
            "blackhole": "false",
        }),
        True,
        [
            "ipv6 route fc00:10::/64 fc00::72 PortChannel0001 10 tag 1",
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

def test_set_nh_only():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 30 nexthop-vrf default vrf vrfRED tag 1",
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

def test_set_ifname_only():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 PortChannel0002 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
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

def test_set_with_empty_ifname():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "ifname": "PortChannel0001,,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
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

def test_set_with_empty_nh():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,,",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 PortChannel0002 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
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

def test_set_del():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
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
    set_del_test(
        mgr,
        "DEL",
        ("vrfRED|10.1.3.0/24",),
        True,
        [
            "no ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "no ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "no ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
            "router bgp 65100 vrf vrfRED",
            " address-family ipv4",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit",
            "no route-map STATIC_ROUTE_FILTER"
        ]
    )
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
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

def test_set_same_route():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
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
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "40,50,60",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "no ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "no ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "no ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 40 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 50 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 60 nexthop-vrf default vrf vrfRED tag 1"
        ]
    )

def test_set_add_del_nh():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
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
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61,10.0.0.63",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003,PortChannel0004",
            "distance": "10,20,30,30",
            "nexthop-vrf": "nh_vrf,,default,",
            "blackhole": "false,false,false,",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.63 PortChannel0004 30 vrf vrfRED tag 1",
        ]
    )
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59",
            "ifname": "PortChannel0001,PortChannel0002",
            "distance": "10,20",
            "nexthop-vrf": "nh_vrf,",
            "blackhole": "false,false",
        }),
        True,
        [
            "no ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
            "no ip route 10.1.3.0/24 10.0.0.63 PortChannel0004 30 vrf vrfRED tag 1",
        ]
    )

def test_set_add_del_nh_ethernet():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|20.1.3.0/24", {
            "nexthop": "20.0.0.57,20.0.0.59,20.0.0.61",
            "ifname": "Ethernet4,Ethernet8,Ethernet12",
            "distance": "10,20,30",
            "nexthop-vrf": "default,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 20.1.3.0/24 20.0.0.57 Ethernet4 10 nexthop-vrf default tag 1",
            "ip route 20.1.3.0/24 20.0.0.59 Ethernet8 20 tag 1",
            "ip route 20.1.3.0/24 20.0.0.61 Ethernet12 30 nexthop-vrf default tag 1",
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
    set_del_test(
        mgr,
        "SET",
        ("default|20.1.3.0/24", {
            "nexthop": "20.0.0.57,20.0.0.59,20.0.0.61,20.0.0.63",
            "ifname": "Ethernet4,Ethernet8,Ethernet12,Ethernet16",
            "distance": "10,20,30,30",
            "nexthop-vrf": "default,,default,",
            "blackhole": "false,false,false,",
        }),
        True,
        [
            "ip route 20.1.3.0/24 20.0.0.63 Ethernet16 30 tag 1",
        ]
    )
    set_del_test(
        mgr,
        "SET",
        ("default|20.1.3.0/24", {
            "nexthop": "20.0.0.57,20.0.0.59",
            "ifname": "Ethernet4,Ethernet8",
            "distance": "10,20",
            "nexthop-vrf": "default,",
            "blackhole": "false,false",
        }),
        True,
        [
            "no ip route 20.1.3.0/24 20.0.0.61 Ethernet12 30 nexthop-vrf default tag 1",
            "no ip route 20.1.3.0/24 20.0.0.63 Ethernet16 30 tag 1",
        ]
    )

@patch('bgpcfgd.managers_static_rt.log_debug')
def test_set_no_action(mocked_log_debug):
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|10.1.1.0/24", {
            "nexthop": "10.0.0.57",
            "ifname": "PortChannel0001",
            "blackhole": "true",
        }),
        True,
        [
            "ip route 10.1.1.0/24 blackhole tag 1",
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

    set_del_test(
        mgr,
        "SET",
        ("default|10.1.1.0/24", {
            "nexthop": "10.0.0.59",
            "ifname": "PortChannel0002",
            "blackhole": "true",
        }),
        True,
        []
    )
    mocked_log_debug.assert_called_with("CONFIG_DB Nothing to update for static route default|10.1.1.0/24")

@patch('bgpcfgd.managers_static_rt.log_debug')
def test_del_no_action(mocked_log_debug):
    mgr = constructor()
    set_del_test(
        mgr,
        "DEL",
        ("default|10.1.1.0/24",),
        True,
        []
    )
    mocked_log_debug.assert_called_with("CONFIG_DB Nothing to update for static route default|10.1.1.0/24")

def test_set_invalid_arg():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|10.1.1.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59",
            "ifname": "PortChannel0001",
        }),
        False,
        []
    )

@patch('bgpcfgd.managers_static_rt.log_err')
def test_set_invalid_blackhole(mocked_log_err):
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("default|10.1.1.0/24", {
            "nexthop": "",
            "ifname": "",
            "blackhole": "false",
        }),
        True,
        [
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
    mocked_log_err.assert_called_with("Mandatory attribute not found for nexthop")

def test_set_invalid_ipaddr():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "invalid_ipaddress",
        }),
        False,
        []
    )

def test_set_del_no_bgp_asn():
    mgr = constructor(skip_bgp_asn=True)
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
        ]
    )
    set_del_test(
        mgr,
        "DEL",
        ("vrfRED|10.1.3.0/24",),
        True,
        [
            "no ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "no ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "no ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
        ]
    )

def test_set_del_bgp_asn_change():
    mgr = constructor(skip_bgp_asn=True)
    set_del_test(
        mgr,
        "SET",
        ("vrfRED|10.1.3.0/24", {
            "nexthop": "10.0.0.57,10.0.0.59,10.0.0.61",
            "ifname": "PortChannel0001,PortChannel0002,PortChannel0003",
            "distance": "10,20,30",
            "nexthop-vrf": "nh_vrf,,default",
            "blackhole": "false,false,false",
        }),
        True,
        [
            "ip route 10.1.3.0/24 10.0.0.57 PortChannel0001 10 nexthop-vrf nh_vrf vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.59 PortChannel0002 20 vrf vrfRED tag 1",
            "ip route 10.1.3.0/24 10.0.0.61 PortChannel0003 30 nexthop-vrf default vrf vrfRED tag 1",
        ]
    )

    assert mgr.vrf_pending_redistribution == {"vrfRED"}

    expected_cmds = [
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
    mgr.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", {"bgp_asn": "65100"})

    assert not mgr.vrf_pending_redistribution

def test_set_tag_enable():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "10.0.0.57","advertise":"true"
        }),
        True,
        [
            "ip route 10.1.0.0/24 10.0.0.57 tag 1",
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

def test_set_tag_disable():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "10.0.0.57","advertise":"false"
        }),
        True,
        [
            "ip route 10.1.0.0/24 10.0.0.57 tag 2",
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

def test_set_tag_change():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "10.0.0.57","advertise":"true"
        }),
        True,
        [
            "ip route 10.1.0.0/24 10.0.0.57 tag 1",
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

    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "nexthop": "10.0.0.57","advertise":"false"
        }),
        True,
        [
            "no ip route 10.1.0.0/24 10.0.0.57 tag 1",
            "ip route 10.1.0.0/24 10.0.0.57 tag 2",
        ]
    )

def test_set_bfd_false():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "bfd": "false",
            "nexthop": "PortChannel0001",
        }),
        True,
        [
            "ip route 10.1.0.0/24 PortChannel0001 tag 1",
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

    set_del_test(
        mgr,
        "DEL",
        ("10.1.0.0/24",),
        True,
        [
            "no ip route 10.1.0.0/24 PortChannel0001 tag 1",
            "router bgp 65100",
            " address-family ipv4",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            " address-family ipv6",
            "  no redistribute static route-map STATIC_ROUTE_FILTER",
            " exit-address-family",
            "exit",
            "no route-map STATIC_ROUTE_FILTER"
        ]
    )

def test_set_bfd_true():
    mgr = constructor()
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "bfd": "false",
            "nexthop": "PortChannel0001",
        }),
        True,
        [
            "ip route 10.1.0.0/24 PortChannel0001 tag 1",
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
    #do nothing for adding smae route second time
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "bfd": "false",
            "nexthop": "PortChannel0001",
        }),
        True,
        [
        ]
    )
    #clear internal cache if bfd flag is true
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "bfd": "true",
            "nexthop": "PortChannel0001",
        }),
        True,
        [
        ]
    )

    #install the route becasue that cache was cleared above
    set_del_test(
        mgr,
        "SET",
        ("10.1.0.0/24", {
            "bfd": "false",
            "nexthop": "PortChannel0001",
        }),
        True,
        [
            "ip route 10.1.0.0/24 PortChannel0001 tag 1",
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

