import sys
from unittest.mock import MagicMock
from . import swsscommon_test

sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_bgp_globals import BgpGlobalsMgr, BGP_GLOBALS_TABLE
from swsscommon import swsscommon


DEFAULT_ASN = "65001"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_mgr(default_asn=DEFAULT_ASN):
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   MagicMock(),
        'tf':        TemplateFabric(),
        'constants': {},
        'state_db_conn': None,
    }
    mgr = BgpGlobalsMgr(common_objs, "CONFIG_DB", BGP_GLOBALS_TABLE)
    mgr.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME,
                      "localhost", {"bgp_asn": default_asn})
    return mgr


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
# _build_cmds unit tests
# ---------------------------------------------------------------------------

class TestBuildCmds:
    def test_enable(self):
        cmds = BgpGlobalsMgr._build_llgr_enable_cmds("65001", "3600")
        assert cmds == [
            "router bgp 65001",
            " bgp long-lived-graceful-restart stale-time 3600",
            "exit",
        ]

    def test_disable(self):
        cmds = BgpGlobalsMgr._build_llgr_disable_cmds("65001")
        assert cmds == [
            "router bgp 65001",
            " no bgp long-lived-graceful-restart stale-time",
            "exit",
        ]


# ---------------------------------------------------------------------------
# set_handler tests
# ---------------------------------------------------------------------------

class TestSetHandler:
    def test_enable_with_stale_time(self):
        mgr = make_mgr()
        run_set(mgr, "default", {"llgr_stale_time": "3600"},
                expect_cmds=[
                    "router bgp 65001",
                    " bgp long-lived-graceful-restart stale-time 3600",
                    "exit",
                ])
        assert mgr._llgr_active is True

    def test_no_stale_time_sends_disable(self):
        # Always emit the no command so a crash between HDEL and the vtysh write
        # does not leave FRR with LLGR after bgpcfgd restarts.
        mgr = make_mgr()
        run_set(mgr, "default", {},
                expect_cmds=[
                    "router bgp 65001",
                    " no bgp long-lived-graceful-restart stale-time",
                    "exit",
                ])
        assert mgr._llgr_active is False

    def test_disable_after_enable(self):
        mgr = make_mgr()
        run_set(mgr, "default", {"llgr_stale_time": "3600"})
        run_set(mgr, "default", {},
                expect_cmds=[
                    "router bgp 65001",
                    " no bgp long-lived-graceful-restart stale-time",
                    "exit",
                ])
        assert mgr._llgr_active is False

    def test_repeated_disable_is_idempotent(self):
        mgr = make_mgr()
        run_set(mgr, "default", {},
                expect_cmds=[
                    "router bgp 65001",
                    " no bgp long-lived-graceful-restart stale-time",
                    "exit",
                ])

    def test_update_stale_time(self):
        mgr = make_mgr()
        run_set(mgr, "default", {"llgr_stale_time": "3600"})
        run_set(mgr, "default", {"llgr_stale_time": "7200"},
                expect_cmds=[
                    "router bgp 65001",
                    " bgp long-lived-graceful-restart stale-time 7200",
                    "exit",
                ])

    def test_zero_stale_time_enables_llgr(self):
        # "0" is a non-empty string so the old `if stale_time:` check would have
        # silently fallen through to the disable path. `if stale_time is not None:`
        # correctly treats it as an enable request.
        mgr = make_mgr()
        run_set(mgr, "default", {"llgr_stale_time": "0"},
                expect_cmds=[
                    "router bgp 65001",
                    " bgp long-lived-graceful-restart stale-time 0",
                    "exit",
                ])
        assert mgr._llgr_active is True

    def test_non_default_vrf_is_ignored(self):
        mgr = make_mgr()
        result = run_set(mgr, "test_vrf", {"llgr_stale_time": "3600"}, expect_cmds=None)
        assert result is True

    def test_missing_asn_defers(self):
        mgr = make_mgr()
        mgr.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME,
                          "localhost", {})
        result = run_set(mgr, "default", {"llgr_stale_time": "3600"}, expect_cmds=None)
        assert result is False


# ---------------------------------------------------------------------------
# del_handler tests
# ---------------------------------------------------------------------------

class TestDelHandler:
    def test_del_when_active_pushes_disable(self):
        mgr = make_mgr()
        run_set(mgr, "default", {"llgr_stale_time": "3600"})
        run_del(mgr, "default",
                expect_cmds=[
                    "router bgp 65001",
                    " no bgp long-lived-graceful-restart stale-time",
                    "exit",
                ])
        assert mgr._llgr_active is False

    def test_del_when_inactive_is_noop(self):
        mgr = make_mgr()
        run_del(mgr, "default", expect_cmds=None)

    def test_del_non_default_vrf_is_noop(self):
        mgr = make_mgr()
        run_set(mgr, "default", {"llgr_stale_time": "3600"})
        run_del(mgr, "test_vrf", expect_cmds=None)
        assert mgr._llgr_active is True

    def test_del_clears_state_even_when_asn_missing(self):
        mgr = make_mgr()
        run_set(mgr, "default", {"llgr_stale_time": "3600"})
        mgr.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME,
                          "localhost", {})
        run_del(mgr, "default", expect_cmds=None)
        assert mgr._llgr_active is False
