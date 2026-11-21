from unittest.mock import MagicMock, patch

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test

import sys
sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.managers_prefix_list import PrefixListMgr

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')


def constructor():
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': MagicMock(),
        'tf': TemplateFabric(TEMPLATE_PATH),
        'constants': {},
    }
    return PrefixListMgr(common_objs, "CONFIG_DB", "PREFIX_LIST")


def set_handler_test(manager, key, value=None):
    res = manager.set_handler(key, value or {})
    assert res, "Returns always True"


def del_handler_test(manager, key):
    res = manager.del_handler(key)
    assert res, "Returns always True"


def test_add_ipv4_prefix_list():
    m = constructor()
    set_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24")

    generated_config = m.cfg_mgr.push.call_args[0][0]
    assert "ip prefix-list SUPPRESS_PREFIX_V4 permit 10.0.0.0/24" in generated_config


def test_add_ipv6_prefix_list():
    m = constructor()
    set_handler_test(m, "SUPPRESS_PREFIX|fc00::/64")

    generated_config = m.cfg_mgr.push.call_args[0][0]
    assert "ipv6 prefix-list SUPPRESS_PREFIX_V6 permit fc00::/64" in generated_config


def test_delete_ipv4_prefix_list():
    m = constructor()
    set_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24")
    del_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24")

    generated_config = m.cfg_mgr.push.call_args[0][0]
    assert "no ip prefix-list SUPPRESS_PREFIX_V4 permit 10.0.0.0/24" in generated_config


def test_delete_ipv6_prefix_list():
    m = constructor()
    set_handler_test(m, "SUPPRESS_PREFIX|fc00::/64")
    del_handler_test(m, "SUPPRESS_PREFIX|fc00::/64")

    generated_config = m.cfg_mgr.push.call_args[0][0]
    assert "no ipv6 prefix-list SUPPRESS_PREFIX_V6 permit fc00::/64" in generated_config


def test_generic_prefix_list_name():
    m = constructor()
    set_handler_test(m, "CUSTOM-LIST|192.0.2.0/24")

    generated_config = m.cfg_mgr.push.call_args[0][0]
    assert "ip prefix-list CUSTOM-LIST_V4 permit 192.0.2.0/24" in generated_config


def test_prefix_list_with_optional_fields():
    m = constructor()
    set_handler_test(
        m,
        "CUSTOM-LIST|192.0.2.0/24",
        {"action": "deny", "seq": "10", "ge": "25", "le": "32"},
    )

    generated_config = m.cfg_mgr.push.call_args[0][0]
    assert "ip prefix-list CUSTOM-LIST_V4 seq 10 deny 192.0.2.0/24 ge 25 le 32" in generated_config


def test_delete_uses_stored_optional_fields():
    m = constructor()
    set_handler_test(
        m,
        "CUSTOM-LIST|192.0.2.0/24",
        {"action": "deny", "seq": "10", "ge": "25", "le": "32"},
    )
    del_handler_test(m, "CUSTOM-LIST|192.0.2.0/24")

    generated_config = m.cfg_mgr.push.call_args[0][0]
    assert "no ip prefix-list CUSTOM-LIST_V4 seq 10 deny 192.0.2.0/24 ge 25 le 32" in generated_config


@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_delete_without_cached_fields_uses_name_fallback(mocked_log_warn):
    m = constructor()
    del_handler_test(m, "CUSTOM-LIST|192.0.2.0/24")

    generated_config = m.cfg_mgr.push.call_args[0][0]
    assert "no ip prefix-list CUSTOM-LIST_V4" in generated_config
    mocked_log_warn.assert_called_once_with(
        "PrefixListMgr:: Missing cached fields for delete of prefix list 'CUSTOM-LIST_V4'; "
        "issued best-effort delete-by-name"
    )


@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_invalid_prefix(mocked_log_warn):
    m = constructor()
    set_handler_test(m, "SUPPRESS_PREFIX|invalid_prefix")

    mocked_log_warn.assert_called_once_with(
        "PrefixListMgr:: Prefix 'invalid_prefix' format is wrong for prefix list 'SUPPRESS_PREFIX'"
    )
    m.cfg_mgr.push.assert_not_called()


@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_invalid_prefix_list_name(mocked_log_warn):
    m = constructor()
    set_handler_test(m, "INVALID NAME|10.0.0.0/24")

    mocked_log_warn.assert_called_once_with(
        "PrefixListMgr:: Prefix list name 'INVALID NAME' is invalid"
    )
    m.cfg_mgr.push.assert_not_called()


@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_add_log(mocked_log_debug):
    m = constructor()
    set_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24")

    mocked_log_debug.assert_called_with(
        "PrefixListMgr:: Prefix 10.0.0.0/24 added to SUPPRESS_PREFIX_V4 configuration"
    )


@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_delete_log(mocked_log_debug):
    m = constructor()
    set_handler_test(m, "SUPPRESS_PREFIX|fc00::/64")
    del_handler_test(m, "SUPPRESS_PREFIX|fc00::/64")

    mocked_log_debug.assert_called_with(
        "PrefixListMgr:: Prefix fc00::/64 removed from SUPPRESS_PREFIX_V6 configuration"
    )
