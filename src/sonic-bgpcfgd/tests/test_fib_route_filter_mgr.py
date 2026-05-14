import sys
from unittest.mock import MagicMock
from . import swsscommon_test

sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_fib_route_filter import FibRouteFilterMgr, FIB_ROUTE_FILTER_TABLE


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_mgr():
    common_objs = {
        "directory": Directory(),
        "cfg_mgr":   MagicMock(),
        "tf":        TemplateFabric(),
        "constants": {},
        "state_db_conn": None,
    }
    return FibRouteFilterMgr(common_objs, "CONFIG_DB", FIB_ROUTE_FILTER_TABLE)


_DONT_CHECK = object()


def run_set(mgr, key, data, expect_cmds=_DONT_CHECK):
    mgr.cfg_mgr.push_list.reset_mock()
    result = mgr.set_handler(key, data)
    if expect_cmds is _DONT_CHECK:
        pass
    elif expect_cmds is None:
        mgr.cfg_mgr.push_list.assert_not_called()
    else:
        mgr.cfg_mgr.push_list.assert_called_once_with(expect_cmds)
    return result


def run_del(mgr, key, expect_cmds=_DONT_CHECK):
    mgr.cfg_mgr.push_list.reset_mock()
    mgr.del_handler(key)
    if expect_cmds is _DONT_CHECK:
        pass
    elif expect_cmds is None:
        mgr.cfg_mgr.push_list.assert_not_called()
    else:
        mgr.cfg_mgr.push_list.assert_called_once_with(expect_cmds)


# ---------------------------------------------------------------------------
# _build_*_cmds unit tests — lock the exact wire format
# ---------------------------------------------------------------------------

class TestBuildCmds:
    def test_set_default_vrf_ipv4(self):
        cmds = FibRouteFilterMgr._build_set_cmds("default", "ip", "bgp", "RM_FROM_BGP")
        assert cmds == ["ip protocol bgp route-map RM_FROM_BGP"]

    def test_set_default_vrf_ipv6(self):
        cmds = FibRouteFilterMgr._build_set_cmds("default", "ipv6", "ospf6", "RM_FROM_OSPF6")
        assert cmds == ["ipv6 protocol ospf6 route-map RM_FROM_OSPF6"]

    def test_set_named_vrf_wraps_in_block(self):
        cmds = FibRouteFilterMgr._build_set_cmds("Vrf_red", "ip", "static", "RM_STATIC_V4")
        assert cmds == [
            "vrf Vrf_red",
            " ip protocol static route-map RM_STATIC_V4",
            "exit-vrf",
        ]

    def test_del_default_vrf(self):
        cmds = FibRouteFilterMgr._build_del_cmds("default", "ip", "bgp")
        assert cmds == ["no ip protocol bgp"]

    def test_del_named_vrf_wraps_in_block(self):
        cmds = FibRouteFilterMgr._build_del_cmds("Vrf_red", "ipv6", "static")
        assert cmds == [
            "vrf Vrf_red",
            " no ipv6 protocol static",
            "exit-vrf",
        ]


# ---------------------------------------------------------------------------
# set_handler tests
# ---------------------------------------------------------------------------

class TestSetHandler:
    def test_default_vrf_ipv4_bgp(self):
        mgr = make_mgr()
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_FROM_BGP"},
                expect_cmds=["ip protocol bgp route-map RM_FROM_BGP"])
        assert mgr._applied["default|IPv4|bgp"] == "RM_FROM_BGP"

    def test_default_vrf_ipv6_ospf6(self):
        mgr = make_mgr()
        run_set(mgr, "default|IPv6|ospf6", {"route_map": "RM_FROM_OSPF6"},
                expect_cmds=["ipv6 protocol ospf6 route-map RM_FROM_OSPF6"])

    def test_named_vrf_wraps(self):
        mgr = make_mgr()
        run_set(mgr, "Vrf_red|IPv4|static", {"route_map": "RM_STATIC_V4"},
                expect_cmds=[
                    "vrf Vrf_red",
                    " ip protocol static route-map RM_STATIC_V4",
                    "exit-vrf",
                ])

    def test_re_set_same_route_map_is_idempotent(self):
        # FRR's `ip protocol PROTO route-map NAME` is upsert-style and the
        # network is the same on retry; skipping the vtysh write also keeps
        # bgpcfgd's commit batch from churning under no-op CONFIG_DB events
        # (e.g. ConfigDBConnector replays on reconnect).
        mgr = make_mgr()
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_FROM_BGP"})
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_FROM_BGP"},
                expect_cmds=None)

    def test_re_set_with_different_route_map_replaces(self):
        # FRR replaces the prior binding when the new command is issued; no
        # explicit `no ... route-map <prev>` is required.
        mgr = make_mgr()
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_OLD"})
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_NEW"},
                expect_cmds=["ip protocol bgp route-map RM_NEW"])
        assert mgr._applied["default|IPv4|bgp"] == "RM_NEW"

    def test_missing_route_map_is_rejected(self):
        # YANG marks route_map mandatory, but bgpcfgd still receives the raw
        # CONFIG_DB row — defend against a hand-set malformed row.
        mgr = make_mgr()
        result = run_set(mgr, "default|IPv4|bgp", {}, expect_cmds=None)
        assert result is True
        assert "default|IPv4|bgp" not in mgr._applied

    def test_malformed_key_too_few_parts_rejected(self):
        mgr = make_mgr()
        result = run_set(mgr, "default|IPv4", {"route_map": "RM"}, expect_cmds=None)
        assert result is True

    def test_malformed_key_empty_field_rejected(self):
        mgr = make_mgr()
        result = run_set(mgr, "default||bgp", {"route_map": "RM"}, expect_cmds=None)
        assert result is True

    def test_unknown_addr_family_rejected(self):
        # The YANG accepts IPv4|IPv6; defend against an out-of-schema row.
        mgr = make_mgr()
        result = run_set(mgr, "default|IPvX|bgp", {"route_map": "RM"}, expect_cmds=None)
        assert result is True

    def test_tuple_key_accepted(self):
        # Defensive: same parser used by other managers' future runner changes.
        mgr = make_mgr()
        run_set(mgr, ("default", "IPv4", "bgp"), {"route_map": "RM_FROM_BGP"},
                expect_cmds=["ip protocol bgp route-map RM_FROM_BGP"])


# ---------------------------------------------------------------------------
# del_handler tests
# ---------------------------------------------------------------------------

class TestDelHandler:
    def test_del_default_vrf_after_set(self):
        mgr = make_mgr()
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_FROM_BGP"})
        run_del(mgr, "default|IPv4|bgp",
                expect_cmds=["no ip protocol bgp"])
        assert "default|IPv4|bgp" not in mgr._applied

    def test_del_named_vrf_wraps(self):
        mgr = make_mgr()
        run_set(mgr, "Vrf_red|IPv6|static", {"route_map": "RM_STATIC_V6"})
        run_del(mgr, "Vrf_red|IPv6|static",
                expect_cmds=[
                    "vrf Vrf_red",
                    " no ipv6 protocol static",
                    "exit-vrf",
                ])

    def test_del_untracked_key_is_noop(self):
        # Avoids spamming `no ...` for state we never set — keeps bgpcfgd
        # quiet on warmboot replays where DEL events arrive before SET.
        mgr = make_mgr()
        run_del(mgr, "default|IPv4|bgp", expect_cmds=None)

    def test_del_malformed_key_noop(self):
        mgr = make_mgr()
        run_del(mgr, "default|IPv4", expect_cmds=None)

    def test_del_unknown_addr_family_noop(self):
        mgr = make_mgr()
        run_del(mgr, "default|IPvX|bgp", expect_cmds=None)


# ---------------------------------------------------------------------------
# Full lifecycle: set -> del -> re-set must re-push (not idempotent-skip)
# ---------------------------------------------------------------------------

class TestLifecycle:
    def test_set_del_set_re_pushes(self):
        # _applied.pop on del clears the cached route_map, so the next set
        # with the same route_map must NOT short-circuit via the
        # `_applied.get(key) == route_map` guard.
        mgr = make_mgr()
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_V1"})
        run_del(mgr, "default|IPv4|bgp")
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_V1"},
                expect_cmds=["ip protocol bgp route-map RM_V1"])
        assert mgr._applied["default|IPv4|bgp"] == "RM_V1"


# ---------------------------------------------------------------------------
# Cross-row independence — each (vrf, afi, proto) is tracked separately
# ---------------------------------------------------------------------------

class TestMultipleRows:
    def test_independent_tracking_per_key(self):
        mgr = make_mgr()
        run_set(mgr, "default|IPv4|bgp", {"route_map": "RM_BGP4"})
        run_set(mgr, "default|IPv6|bgp", {"route_map": "RM_BGP6"})
        run_set(mgr, "Vrf_red|IPv4|static", {"route_map": "RM_S4"})

        assert mgr._applied == {
            "default|IPv4|bgp": "RM_BGP4",
            "default|IPv6|bgp": "RM_BGP6",
            "Vrf_red|IPv4|static": "RM_S4",
        }

        run_del(mgr, "default|IPv6|bgp",
                expect_cmds=["no ipv6 protocol bgp"])
        assert "default|IPv6|bgp" not in mgr._applied
        # Sibling rows untouched.
        assert mgr._applied["default|IPv4|bgp"] == "RM_BGP4"
        assert mgr._applied["Vrf_red|IPv4|static"] == "RM_S4"
