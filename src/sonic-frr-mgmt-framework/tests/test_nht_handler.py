"""Tests for the NEXTHOP_TRACKING handler in frrcfgd.

Covers:
  - vtysh command construction for the default VRF and user VRFs, both AFIs
  - explicit resolve_via_default values
  - absent field and delete applying the platform default (cloudtype aware)
  - invalid key, AFI and value rejection
"""
from unittest.mock import MagicMock, NonCallableMagicMock, patch

swsscommon_module_mock = MagicMock(ConfigDBConnector=NonCallableMagicMock)
mockmapping = {'swsscommon.swsscommon': swsscommon_module_mock}

with patch.dict('sys.modules', **mockmapping):
    from frrcfgd.frrcfgd import BGPConfigDaemon


def _make_daemon(cloudtype=None):
    """Minimal BGPConfigDaemon for handler tests.

    __new__ bypasses __init__ (which opens CONFIG_DB and spawns threads).
    config_db is mocked so the platform-default lookup of DEVICE_METADATA
    works; cloudtype seeds what that lookup returns.
    """
    daemon = BGPConfigDaemon.__new__(BGPConfigDaemon)
    daemon._BGPConfigDaemon__run_command = MagicMock(return_value=True)
    daemon.config_db = MagicMock()
    daemon.config_db.get_entry.return_value = {} if cloudtype is None else {'cloudtype': cloudtype}
    return daemon


def _cmd(daemon):
    """The command list passed to the most recent __run_command call."""
    daemon._BGPConfigDaemon__run_command.assert_called_once()
    return daemon._BGPConfigDaemon__run_command.call_args[0][1]


def _in_vrf(cmd, vrf, line):
    """True if `line` sits between 'vrf <vrf>' and 'exit-vrf' in cmd."""
    return cmd.index('vrf {}'.format(vrf)) < cmd.index(line) < cmd.index('exit-vrf')


# ---------------------------------------------------------------------------
# Explicit values
# ---------------------------------------------------------------------------

def test_default_vrf_ipv4_enable():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", {"resolve_via_default": "true"})
    assert _cmd(daemon) == ['vtysh', '-c', 'configure terminal', '-c', 'ip nht resolve-via-default']


def test_default_vrf_ipv4_disable():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", {"resolve_via_default": "false"})
    assert _cmd(daemon) == ['vtysh', '-c', 'configure terminal', '-c', 'no ip nht resolve-via-default']


def test_default_vrf_ipv6_enable():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv6", {"resolve_via_default": "true"})
    assert _cmd(daemon) == ['vtysh', '-c', 'configure terminal', '-c', 'ipv6 nht resolve-via-default']


def test_user_vrf_ipv4_disable_ordering():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "Vrf_blue|ipv4", {"resolve_via_default": "false"})
    cmd = _cmd(daemon)
    assert cmd[:3] == ['vtysh', '-c', 'configure terminal']
    assert _in_vrf(cmd, 'Vrf_blue', 'no ip nht resolve-via-default')


def test_user_vrf_ipv6_enable_ordering():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "Vrf_blue|ipv6", {"resolve_via_default": "true"})
    assert _in_vrf(_cmd(daemon), 'Vrf_blue', 'ipv6 nht resolve-via-default')


def test_explicit_value_wins_over_public_cloudtype():
    daemon = _make_daemon(cloudtype='Public')
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", {"resolve_via_default": "true"})
    assert 'ip nht resolve-via-default' in _cmd(daemon)


# ---------------------------------------------------------------------------
# Absent field and delete apply the platform default
# ---------------------------------------------------------------------------

def test_absent_field_default_vrf_ipv4():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", {"NULL": "NULL"})
    assert _cmd(daemon)[-1] == 'ip nht resolve-via-default'


def test_absent_field_default_vrf_ipv4_public_cloudtype():
    daemon = _make_daemon(cloudtype='Public')
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", {"NULL": "NULL"})
    assert _cmd(daemon)[-1] == 'no ip nht resolve-via-default'
    daemon.config_db.get_entry.assert_called_with('DEVICE_METADATA', 'localhost')


def test_absent_field_default_vrf_ipv6():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv6", {"NULL": "NULL"})
    assert _cmd(daemon)[-1] == 'no ipv6 nht resolve-via-default'


def test_absent_field_user_vrf_enabled_regardless_of_cloudtype():
    daemon = _make_daemon(cloudtype='Public')
    daemon.nht_handler("NEXTHOP_TRACKING", "Vrf_blue|ipv6", {"NULL": "NULL"})
    assert _in_vrf(_cmd(daemon), 'Vrf_blue', 'ipv6 nht resolve-via-default')


def test_delete_default_vrf_ipv4():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", None)
    assert _cmd(daemon) == ['vtysh', '-c', 'configure terminal', '-c', 'ip nht resolve-via-default']


def test_delete_default_vrf_ipv4_public_cloudtype():
    daemon = _make_daemon(cloudtype='public')
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", None)
    assert _cmd(daemon)[-1] == 'no ip nht resolve-via-default'


def test_delete_default_vrf_ipv6():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv6", None)
    assert _cmd(daemon)[-1] == 'no ipv6 nht resolve-via-default'


def test_delete_user_vrf_restores_enabled():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "Vrf_blue|ipv4", None)
    assert _in_vrf(_cmd(daemon), 'Vrf_blue', 'ip nht resolve-via-default')


# ---------------------------------------------------------------------------
# Input validation: nothing reaches vtysh
# ---------------------------------------------------------------------------

def test_invalid_key_format_does_not_run():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default", {"resolve_via_default": "true"})
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4|extra", {"resolve_via_default": "true"})
    daemon._BGPConfigDaemon__run_command.assert_not_called()


def test_empty_vrf_name_does_not_run():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "|ipv4", {"resolve_via_default": "true"})
    daemon._BGPConfigDaemon__run_command.assert_not_called()


def test_invalid_afi_does_not_run():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipvX", {"resolve_via_default": "true"})
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipvX", None)
    daemon._BGPConfigDaemon__run_command.assert_not_called()


def test_unexpected_value_does_not_run():
    daemon = _make_daemon()
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", {"resolve_via_default": "maybe"})
    daemon._BGPConfigDaemon__run_command.assert_not_called()


def test_run_command_failure_does_not_raise():
    daemon = _make_daemon()
    daemon._BGPConfigDaemon__run_command = MagicMock(return_value=False)
    daemon.nht_handler("NEXTHOP_TRACKING", "default|ipv4", {"resolve_via_default": "true"})
    daemon._BGPConfigDaemon__run_command.assert_called_once()
