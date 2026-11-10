from unittest.mock import MagicMock, patch

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test

import sys
sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.managers_nht import NhtMgr  # noqa: E402

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')


def constructor(cloudtype=None):
    """Create an NhtMgr; optionally seed DEVICE_METADATA cloudtype."""
    directory = Directory()
    if cloudtype is not None:
        directory.put("CONFIG_DB", "DEVICE_METADATA", "localhost", {"cloudtype": cloudtype})
    common_objs = {
        'directory': directory,
        'cfg_mgr':   MagicMock(),
        'tf':        TemplateFabric(TEMPLATE_PATH),
        'constants': {},
    }
    return NhtMgr(common_objs, "CONFIG_DB", "NEXTHOP_TRACKING")


def pushed_lines(m):
    """FRR config lines pushed to cfg_mgr, without comments and blanks."""
    m.cfg_mgr.push.assert_called_once()
    raw = m.cfg_mgr.push.call_args[0][0]
    return [line.rstrip() for line in raw.split("\n") if line.strip() and not line.strip().startswith('!')]


# ---------------------------------------------------------------------------
# Explicit values, default VRF
# ---------------------------------------------------------------------------

@patch('bgpcfgd.managers_nht.log_info')
def test_set_enable_ipv4(mocked_log_info):
    m = constructor()
    assert m.set_handler("default|ipv4", {"resolve_via_default": "true"})
    assert pushed_lines(m) == ['ip nht resolve-via-default']
    mocked_log_info.assert_called_with(
        "NhtMgr: resolve_via_default=true scheduled for (vrf=default, afi=ipv4)")
    assert m.directory.get("CONFIG_DB", "NEXTHOP_TRACKING", "default|ipv4") == {"resolve_via_default": "true"}


def test_set_disable_ipv4():
    m = constructor()
    assert m.set_handler("default|ipv4", {"resolve_via_default": "false"})
    assert pushed_lines(m) == ['no ip nht resolve-via-default']


def test_set_enable_ipv6():
    m = constructor()
    assert m.set_handler("default|ipv6", {"resolve_via_default": "true"})
    assert pushed_lines(m) == ['ipv6 nht resolve-via-default']


def test_set_disable_ipv6():
    m = constructor()
    assert m.set_handler("default|ipv6", {"resolve_via_default": "false"})
    assert pushed_lines(m) == ['no ipv6 nht resolve-via-default']


def test_set_explicit_value_wins_over_public_cloudtype():
    m = constructor(cloudtype="Public")
    assert m.set_handler("default|ipv4", {"resolve_via_default": "true"})
    assert pushed_lines(m) == ['ip nht resolve-via-default']


# ---------------------------------------------------------------------------
# Absent field and delete apply the platform default
# ---------------------------------------------------------------------------

def test_set_absent_field_ipv4_platform_default():
    m = constructor()
    assert m.set_handler("default|ipv4", {"NULL": "NULL"})
    assert pushed_lines(m) == ['ip nht resolve-via-default']


def test_set_absent_field_ipv4_public_cloudtype():
    m = constructor(cloudtype="Public")
    assert m.set_handler("default|ipv4", {"NULL": "NULL"})
    assert pushed_lines(m) == ['no ip nht resolve-via-default']


def test_set_absent_field_ipv6_platform_default():
    m = constructor()
    assert m.set_handler("default|ipv6", {"NULL": "NULL"})
    assert pushed_lines(m) == ['no ipv6 nht resolve-via-default']


def test_del_ipv4_platform_default():
    m = constructor()
    m.set_handler("default|ipv4", {"resolve_via_default": "false"})
    m.cfg_mgr.push.reset_mock()
    assert m.del_handler("default|ipv4")
    assert pushed_lines(m) == ['ip nht resolve-via-default']
    assert not m.directory.path_exist("CONFIG_DB", "NEXTHOP_TRACKING", "default|ipv4")


def test_del_ipv4_public_cloudtype():
    m = constructor(cloudtype="public")
    assert m.del_handler("default|ipv4")
    assert pushed_lines(m) == ['no ip nht resolve-via-default']


def test_del_ipv6_platform_default():
    m = constructor()
    assert m.del_handler("default|ipv6")
    assert pushed_lines(m) == ['no ipv6 nht resolve-via-default']


# ---------------------------------------------------------------------------
# User VRFs: commands wrapped in 'vrf X' / 'exit-vrf'
# ---------------------------------------------------------------------------

def test_set_user_vrf_enable():
    m = constructor()
    assert m.set_handler("Vrf_blue|ipv4", {"resolve_via_default": "true"})
    assert pushed_lines(m) == ['vrf Vrf_blue', ' ip nht resolve-via-default', 'exit-vrf']


def test_set_user_vrf_disable():
    m = constructor()
    assert m.set_handler("Vrf_blue|ipv6", {"resolve_via_default": "false"})
    assert pushed_lines(m) == ['vrf Vrf_blue', ' no ipv6 nht resolve-via-default', 'exit-vrf']


def test_set_user_vrf_absent_field_enabled_regardless_of_cloudtype():
    m = constructor(cloudtype="Public")
    assert m.set_handler("Vrf_blue|ipv4", {"NULL": "NULL"})
    assert pushed_lines(m) == ['vrf Vrf_blue', ' ip nht resolve-via-default', 'exit-vrf']


def test_del_user_vrf_restores_enabled():
    m = constructor()
    m.set_handler("Vrf_blue|ipv6", {"resolve_via_default": "false"})
    m.cfg_mgr.push.reset_mock()
    assert m.del_handler("Vrf_blue|ipv6")
    assert pushed_lines(m) == ['vrf Vrf_blue', ' ipv6 nht resolve-via-default', 'exit-vrf']
    assert not m.directory.path_exist("CONFIG_DB", "NEXTHOP_TRACKING", "Vrf_blue|ipv6")


# ---------------------------------------------------------------------------
# Ignored input: bad values, bad keys
# ---------------------------------------------------------------------------

@patch('bgpcfgd.managers_nht.log_warn')
def test_set_unexpected_value_ignored(mocked_log_warn):
    m = constructor()
    assert m.set_handler("default|ipv4", {"resolve_via_default": "maybe"})
    m.cfg_mgr.push.assert_not_called()
    mocked_log_warn.assert_called_with(
        "NhtMgr: unexpected resolve_via_default value 'maybe' for key 'default|ipv4', ignoring")


@patch('bgpcfgd.managers_nht.log_err')
def test_set_invalid_keys(mocked_log_err):
    m = constructor()
    for key in ("default", "default|ipv4|extra", "|ipv4", "default|ipvX"):
        assert m.set_handler(key, {"resolve_via_default": "true"})
    m.cfg_mgr.push.assert_not_called()
    assert mocked_log_err.call_count == 4


@patch('bgpcfgd.managers_nht.log_err')
def test_del_invalid_keys(mocked_log_err):
    m = constructor()
    for key in ("default", "|ipv6", "default|ipvX"):
        assert m.del_handler(key)
    m.cfg_mgr.push.assert_not_called()
    assert mocked_log_err.call_count == 3


@patch('bgpcfgd.managers_nht.log_err')
def test_template_error_is_logged(mocked_log_err):
    m = constructor()
    m.nht_template = MagicMock()
    import jinja2
    m.nht_template.render.side_effect = jinja2.TemplateError("boom")
    assert m.set_handler("default|ipv4", {"resolve_via_default": "true"})
    m.cfg_mgr.push.assert_not_called()
    mocked_log_err.assert_called_once()
    assert not m.directory.path_exist("CONFIG_DB", "NEXTHOP_TRACKING", "default|ipv4")
