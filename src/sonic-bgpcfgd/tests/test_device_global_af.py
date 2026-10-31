import sys
import pytest
from unittest.mock import MagicMock

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test

sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.managers_device_global_af import DeviceGlobalAfMgr, BGP_DEVICE_GLOBAL_AF_TABLE
from swsscommon import swsscommon


DEFAULT_ASN = "65001"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_mgr(default_asn=DEFAULT_ASN):
    """Construct a DeviceGlobalAfMgr with mocked swsscommon and pre-populated directory."""
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   MagicMock(),
        'tf':        TemplateFabric(),
        'constants': {},
        'state_db_conn': None,
    }
    mgr = DeviceGlobalAfMgr(common_objs, "CONFIG_DB", BGP_DEVICE_GLOBAL_AF_TABLE)

    # Satisfy the bgp_asn dependency
    mgr.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME,
                      "localhost", {"bgp_asn": default_asn})
    return mgr


# Sentinel: pass as expect_cmds to skip the push_list assertion (setup steps)
_DONT_CHECK = object()


def run_set(mgr, key, data, expect_cmds=_DONT_CHECK):
    """Call set_handler and optionally assert what was pushed.

    expect_cmds=_DONT_CHECK  → don't assert (use for setup)
    expect_cmds=None         → assert push_list was NOT called
    expect_cmds=[...]        → assert push_list was called with exactly those commands
    """
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
    """Call del_handler and optionally assert what was pushed."""
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
    def test_disabled_ipv4(self):
        cmds = DeviceGlobalAfMgr._build_cmds("65001", "ipv4_unicast", "disabled")
        assert cmds == [
            "router bgp 65001",
            " address-family ipv4 unicast",
            "  no install backup-path",
            " exit-address-family",
            "exit",
        ]

    def test_pic_ipv4(self):
        cmds = DeviceGlobalAfMgr._build_cmds("65001", "ipv4_unicast", "pic")
        assert cmds == [
            "router bgp 65001",
            " address-family ipv4 unicast",
            "  install backup-path",
            " exit-address-family",
            "exit",
        ]

    def test_pic_ecmp_ipv4(self):
        cmds = DeviceGlobalAfMgr._build_cmds("65001", "ipv4_unicast", "pic-ecmp")
        assert cmds == [
            "router bgp 65001",
            " address-family ipv4 unicast",
            "  install backup-path ecmp",
            " exit-address-family",
            "exit",
        ]

    def test_pic_ipv6(self):
        cmds = DeviceGlobalAfMgr._build_cmds("65001", "ipv6_unicast", "pic")
        assert cmds == [
            "router bgp 65001",
            " address-family ipv6 unicast",
            "  install backup-path",
            " exit-address-family",
            "exit",
        ]



# ---------------------------------------------------------------------------
# set_handler tests
# ---------------------------------------------------------------------------

class TestSetHandler:
    def test_pic_ipv4(self):
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"},
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv4 unicast",
                    "  install backup-path",
                    " exit-address-family",
                    "exit",
                ])
        assert mgr._pic_state["ipv4_unicast"] == "pic"

    def test_pic_ecmp_ipv4(self):
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic-ecmp"},
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv4 unicast",
                    "  install backup-path ecmp",
                    " exit-address-family",
                    "exit",
                ])

    def test_pic_ipv6(self):
        mgr = make_mgr()
        run_set(mgr, "ipv6_unicast", {"install_backup_path": "pic"},
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv6 unicast",
                    "  install backup-path",
                    " exit-address-family",
                    "exit",
                ])

    def test_disabled_is_noop_when_already_disabled(self):
        """Default state is disabled; sending disabled must not emit commands."""
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "disabled"},
                expect_cmds=None)

    def test_no_field_treated_as_disabled(self):
        """Missing install_backup_path defaults to disabled, which is the initial state."""
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {}, expect_cmds=None)

    def test_ipv4_and_ipv6_are_independent(self):
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"})
        run_set(mgr, "ipv6_unicast", {"install_backup_path": "pic-ecmp"})
        assert mgr._pic_state["ipv4_unicast"] == "pic"
        assert mgr._pic_state["ipv6_unicast"] == "pic-ecmp"

    def test_idempotent_set(self):
        """Applying the same mode twice must only push commands once."""
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"})
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"},
                expect_cmds=None)

    def test_transition_pic_to_pic_ecmp(self):
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"})
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic-ecmp"},
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv4 unicast",
                    "  install backup-path ecmp",
                    " exit-address-family",
                    "exit",
                ])

    def test_transition_pic_ecmp_to_pic(self):
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic-ecmp"})
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"},
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv4 unicast",
                    "  install backup-path",
                    " exit-address-family",
                    "exit",
                ])

    def test_transition_pic_to_disabled(self):
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"})
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "disabled"},
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv4 unicast",
                    "  no install backup-path",
                    " exit-address-family",
                    "exit",
                ])

    def test_missing_asn_defers(self):
        """If DEVICE_METADATA has no bgp_asn the handler must return False."""
        mgr = make_mgr()
        # Overwrite with an entry that has no bgp_asn
        mgr.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME,
                          "localhost", {})
        result = mgr.set_handler("ipv4_unicast", {"install_backup_path": "pic"})
        assert result is False
        mgr.cfg_mgr.push_list.assert_not_called()

    def test_unsupported_afi_safi_is_ignored(self):
        mgr = make_mgr()
        result = run_set(mgr, "ipv4_labeled_unicast",
                         {"install_backup_path": "pic"}, expect_cmds=None)
        assert result is True

    def test_unknown_pic_mode_is_ignored(self):
        mgr = make_mgr()
        result = run_set(mgr, "ipv4_unicast",
                         {"install_backup_path": "invalid"}, expect_cmds=None)
        assert result is True


# ---------------------------------------------------------------------------
# del_handler tests
# ---------------------------------------------------------------------------

class TestDelHandler:
    def test_del_removes_pic(self):
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"})
        run_del(mgr, "ipv4_unicast",
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv4 unicast",
                    "  no install backup-path",
                    " exit-address-family",
                    "exit",
                ])
        assert "ipv4_unicast" not in mgr._pic_state

    def test_del_removes_pic_ecmp(self):
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic-ecmp"})
        run_del(mgr, "ipv4_unicast",
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv4 unicast",
                    "  no install backup-path",
                    " exit-address-family",
                    "exit",
                ])

    def test_del_on_unknown_key_is_noop(self):
        """Deleting a key never SET must not emit any commands."""
        mgr = make_mgr()
        run_del(mgr, "ipv4_unicast", expect_cmds=None)

    def test_del_ipv6(self):
        mgr = make_mgr()
        run_set(mgr, "ipv6_unicast", {"install_backup_path": "pic"})
        run_del(mgr, "ipv6_unicast",
                expect_cmds=[
                    "router bgp 65001",
                    " address-family ipv6 unicast",
                    "  no install backup-path",
                    " exit-address-family",
                    "exit",
                ])
        assert "ipv6_unicast" not in mgr._pic_state

    def test_del_clears_state_even_when_asn_missing(self):
        """del_handler must clear _pic_state even if no BGP ASN is available,
        so a later set_handler for the same mode is not silently skipped."""
        mgr = make_mgr()
        run_set(mgr, "ipv4_unicast", {"install_backup_path": "pic"})
        # Remove the ASN
        mgr.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME,
                          "localhost", {})
        run_del(mgr, "ipv4_unicast", expect_cmds=None)  # no FRR push
        assert "ipv4_unicast" not in mgr._pic_state
