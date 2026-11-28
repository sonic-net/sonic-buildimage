import os
from unittest.mock import MagicMock

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_bgp_srv6_sid_advertise import BGPSrv6SidAdvertiseMgr
from swsscommon import swsscommon


def make_manager():
    directory = Directory()
    directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", {"bgp_asn": "65000"})
    cfg_mgr = MagicMock()
    common_objs = {
        "directory": directory,
        "cfg_mgr": cfg_mgr,
        "tf": TemplateFabric(os.path.abspath("../../dockers/docker-fpm-frr/frr")),
        "constants": {},
    }
    mgr = BGPSrv6SidAdvertiseMgr(common_objs, "CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME)
    return mgr, cfg_mgr


def test_unfiltered_ipv4_sid_advertise_uses_dt46():
    mgr, cfg_mgr = make_manager()

    mgr.set_handler("SRV6", {"bgp_srv6_ipv4_sid_advertise": "true"})

    commands = cfg_mgr.push.call_args[0][0]
    assert "  sid export auto behavior dt46" in commands
    assert "route-map" not in commands
    assert all("behavior dt46" in line for line in commands.splitlines() if "sid export" in line)


def test_prefix_list_ipv6_sid_advertise_uses_family_route_map_and_dt46():
    mgr, cfg_mgr = make_manager()

    mgr.set_handler("SRV6", {"bgp_srv6_ipv6_sid_advertise": "true"})
    mgr.set_handler("SRV6", {"bgp_srv6_ipv6_sid_prefix_list": "SRV6_V6_ROUTES"})

    commands = cfg_mgr.push.call_args[0][0]
    assert "route-map BGP_SRV6_ADVERTISE_IPV6 permit 10" in commands
    assert " match ipv6 address prefix-list SRV6_V6_ROUTES" in commands
    assert "  sid export auto behavior dt46 route-map BGP_SRV6_ADVERTISE_IPV6" in commands


def test_disable_cleans_sid_advertise_and_owned_route_map():
    mgr, cfg_mgr = make_manager()
    mgr.set_handler("SRV6", {
        "bgp_srv6_ipv4_sid_advertise": "true",
        "bgp_srv6_ipv4_sid_prefix_list": "SRV6_V4_ROUTES",
    })

    mgr.set_handler("SRV6", {"bgp_srv6_ipv4_sid_advertise": "false"})

    commands = cfg_mgr.push.call_args[0][0]
    assert "  no sid export auto behavior dt46" in commands
    assert "no route-map BGP_SRV6_ADVERTISE_IPV4" in commands
    assert "sid export auto behavior dt46 route-map" not in commands


def test_delete_removes_advertisement_even_when_unfiltered():
    mgr, cfg_mgr = make_manager()
    mgr.set_handler("SRV6", {"bgp_srv6_ipv4_sid_advertise": "true"})

    calls_before_delete = len(cfg_mgr.push.call_args_list)
    mgr.del_handler("SRV6")

    deletion_commands = "\n".join(call.args[0] for call in cfg_mgr.push.call_args_list[calls_before_delete:])
    assert deletion_commands.count("no sid export auto behavior dt46") == 2


def test_unrelated_fields_and_non_default_keys_are_ignored():
    mgr, cfg_mgr = make_manager()

    mgr.set_handler("SRV6", {"bgp_srv6_locator": "MAIN"})
    mgr.set_handler("bmc", {"bgp_srv6_ipv4_sid_advertise": "true"})

    cfg_mgr.push.assert_not_called()


def test_prefix_list_ipv4_uses_ipv4_route_map_match_and_dt46():
    mgr, cfg_mgr = make_manager()

    mgr.set_handler("SRV6", {
        "bgp_srv6_ipv4_sid_advertise": "true",
        "bgp_srv6_ipv4_sid_prefix_list": "SRV6_V4_ROUTES",
    })

    commands = cfg_mgr.push.call_args[0][0]
    assert "route-map BGP_SRV6_ADVERTISE_IPV4 permit 10" in commands
    assert "match ip address prefix-list SRV6_V4_ROUTES" in commands
    assert "sid export auto behavior dt46 route-map BGP_SRV6_ADVERTISE_IPV4" in commands


def test_bgp_asn_change_reapplies_active_advertisement():
    mgr, cfg_mgr = make_manager()
    mgr.set_handler("SRV6", {
        "bgp_srv6_ipv4_sid_advertise": "true",
        "bgp_srv6_ipv4_sid_prefix_list": "SRV6_V4_ROUTES",
    })

    mgr.directory.put(
        "CONFIG_DB",
        swsscommon.CFG_DEVICE_METADATA_TABLE_NAME,
        "localhost",
        {"bgp_asn": "65001"},
    )

    commands = cfg_mgr.push.call_args[0][0]
    assert "router bgp 65001" in commands
    assert "sid export auto behavior dt46 route-map BGP_SRV6_ADVERTISE_IPV4" in commands


def test_invalid_advertisement_and_prefix_list_are_ignored():
    mgr, cfg_mgr = make_manager()

    mgr.set_handler("SRV6", {"bgp_srv6_ipv4_sid_advertise": "enabled"})
    mgr.set_handler("SRV6", {
        "bgp_srv6_ipv4_sid_advertise": "true",
        "bgp_srv6_ipv4_sid_prefix_list": "PL_IPV4\nexit",
    })

    cfg_mgr.push.assert_not_called()
    assert mgr.config == {}


def test_delete_cleans_route_maps_for_both_address_families():
    mgr, cfg_mgr = make_manager()
    mgr.set_handler("SRV6", {
        "bgp_srv6_ipv4_sid_advertise": "true",
        "bgp_srv6_ipv4_sid_prefix_list": "SRV6_V4_ROUTES",
        "bgp_srv6_ipv6_sid_advertise": "true",
        "bgp_srv6_ipv6_sid_prefix_list": "SRV6_V6_ROUTES",
    })
    calls_before_delete = len(cfg_mgr.push.call_args_list)

    mgr.del_handler("SRV6")

    deletion_commands = "\n".join(call.args[0] for call in cfg_mgr.push.call_args_list[calls_before_delete:])
    assert deletion_commands.count("no sid export auto behavior dt46") == 2
    assert "no route-map BGP_SRV6_ADVERTISE_IPV4" in deletion_commands
    assert "no route-map BGP_SRV6_ADVERTISE_IPV6" in deletion_commands
