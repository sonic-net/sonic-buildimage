from unittest.mock import MagicMock, patch

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test

import sys
sys.modules["swsscommon"] = swsscommon_test

from swsscommon import swsscommon

from bgpcfgd.managers_prefix_list import PrefixListMgr

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')

def constructor():
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(TEMPLATE_PATH),
        'constants': {},
    }

    m = PrefixListMgr(common_objs, "CONFIG_DB", "PREFIX_LIST")
    m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", {"bgp_asn": "65100", "type": "SpineRouter", "subtype": "UpstreamLC"})
    
    return m

def set_handler_test(manager, key, value):
    res = manager.set_handler(key, value)
    assert res, "Returns always True"

def del_handler_test(manager, key):
    res = manager.del_handler(key)
    assert res, "Returns always True"
    
# test if the ipv4 radian configs are set correctly
@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_generate_prefix_list_config_ipv4(mocked_log_debug):
    m = constructor()
    set_handler_test(m, "ANCHOR_PREFIX|192.168.0.0/24", {})
    mocked_log_debug.assert_called_with("PrefixListMgr:: Anchor prefix 192.168.0.0/24 added to radian configuration")

# test if the ipv6 radian configs are set correctly
@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_generate_prefix_list_config_ipv6(mocked_log_debug):
    m = constructor()
    set_handler_test(m, "ANCHOR_PREFIX|fc02:100::/64", {})
    mocked_log_debug.assert_called_with("PrefixListMgr:: Anchor prefix fc02:100::/64 added to radian configuration")

# test if invalid prefix is handled correctly
@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_generate_prefix_list_config_invalid_prefix(mocked_log_warn):
    m = constructor()
    set_handler_test(m, "ANCHOR_PREFIX|invalid_prefix", {})
    mocked_log_warn.assert_called_with("PrefixListMgr:: Prefix 'invalid_prefix' format is wrong for prefix list 'ANCHOR_PREFIX'")

# test if the ipv4 radian configs are deleted correctly
@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_del_handler_ipv4(mocked_log_debug):
    m = constructor()
    set_handler_test(m, "ANCHOR_PREFIX|192.168.0.0/24", {})
    del_handler_test(m, "ANCHOR_PREFIX|192.168.0.0/24")
    mocked_log_debug.assert_called_with("PrefixListMgr:: Anchor prefix 192.168.0.0/24 removed from radian configuration")

# test if the ipv6 radian configs are deleted correctly
@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_del_handler_ipv6(mocked_log_debug):
    m = constructor()
    set_handler_test(m, "ANCHOR_PREFIX|fc02:100::/64", {})
    del_handler_test(m, "ANCHOR_PREFIX|fc02:100::/64")
    mocked_log_debug.assert_called_with("PrefixListMgr:: Anchor prefix fc02:100::/64 removed from radian configuration")

def constructor_with_constants(constants):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(TEMPLATE_PATH),
        'constants': constants,
    }
    m = PrefixListMgr(common_objs, "CONFIG_DB", "PREFIX_LIST")
    m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost",
                    {"bgp_asn": "65100", "type": "ToRRouter", "subtype": ""})
    return m

@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_dynamic_prefix_type(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "IPV4_DOWNSTREAM_PREFIXES_VNET_Vnet1001|10.0.0.0/24", {"action": "permit"})
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "ip prefix-list IPV4_DOWNSTREAM_PREFIXES_VNET_Vnet1001 permit 10.0.0.0/24" in push_call
    mocked_log_warn.assert_not_called()

@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_anchor_prefix_wrong_device(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "ANCHOR_PREFIX|192.168.0.0/24", {})
    mocked_log_warn.assert_called_with("PrefixListMgr:: Device type ToRRouter not supported for ANCHOR_PREFIX")

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_suppress_prefix_ipv4(mocked_log_debug):
    m = constructor_with_constants({})
    set_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24", {})
    mocked_log_debug.assert_called_with("PrefixListMgr:: Suppress prefix 10.0.0.0/24 added to suppress_prefix configuration")

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_suppress_prefix_ipv6(mocked_log_debug):
    m = constructor_with_constants({})
    set_handler_test(m, "SUPPRESS_PREFIX|fc00::/64", {})
    mocked_log_debug.assert_called_with("PrefixListMgr:: Suppress prefix fc00::/64 added to suppress_prefix configuration")

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_suppress_prefix_del_ipv4(mocked_log_debug):
    m = constructor_with_constants({})
    set_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24", {})
    del_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24")
    mocked_log_debug.assert_called_with("PrefixListMgr:: Suppress prefix 10.0.0.0/24 removed from suppress_prefix configuration")

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_suppress_prefix_del_ipv6(mocked_log_debug):
    m = constructor_with_constants({})
    set_handler_test(m, "SUPPRESS_PREFIX|fc00::/64", {})
    del_handler_test(m, "SUPPRESS_PREFIX|fc00::/64")
    mocked_log_debug.assert_called_with("PrefixListMgr:: Suppress prefix fc00::/64 removed from suppress_prefix configuration")

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_suppress_prefix_any_device(mocked_log_debug):
    m = constructor()
    set_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24", {})
    mocked_log_debug.assert_called_with("PrefixListMgr:: Suppress prefix 10.0.0.0/24 added to suppress_prefix configuration")

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_suppress_prefix_constants_override(mocked_log_debug):
    constants = {"bgp": {"prefix_list": {"SUPPRESS_PREFIX": {
        "ipv4_name": "CUSTOM_IPV4_PREFIX",
        "ipv6_name": "CUSTOM_IPV6_PREFIX"}}}}
    m = constructor_with_constants(constants)
    set_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24", {})
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "CUSTOM_IPV4_PREFIX" in push_call
    assert "SUPPRESS_IPV4_PREFIX" not in push_call

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_suppress_prefix_constants_override_ipv6(mocked_log_debug):
    constants = {"bgp": {"prefix_list": {"SUPPRESS_PREFIX": {
        "ipv4_name": "CUSTOM_IPV4_PREFIX",
        "ipv6_name": "CUSTOM_IPV6_PREFIX"}}}}
    m = constructor_with_constants(constants)
    set_handler_test(m, "SUPPRESS_PREFIX|fc00::/64", {})
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "CUSTOM_IPV6_PREFIX" in push_call
    assert "SUPPRESS_IPV6_PREFIX" not in push_call

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_suppress_prefix_no_constants_fallback(mocked_log_debug):
    m = constructor_with_constants({})
    set_handler_test(m, "SUPPRESS_PREFIX|10.0.0.0/24", {})
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "SUPPRESS_IPV4_PREFIX" in push_call

# test if dynamic prefix-list config with seq/ge/le is generated correctly
def test_dynamic_prefix_with_seq_ge_le():
    m = constructor_with_constants({})
    set_handler_test(
        m,
        "Vnet1001|100.64.0.0/10",
        {"action": "permit", "seq": "5", "ge": "16", "le": "24"}
    )
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "ip prefix-list Vnet1001 seq 5 permit 100.64.0.0/10 ge 16 le 24" in push_call

# test if dynamic prefix-list delete keeps stored options for exact removal
def test_dynamic_prefix_delete_uses_stored_options():
    m = constructor_with_constants({})
    set_handler_test(
        m,
        "Vnet1001|100.64.0.0/10",
        {"action": "permit", "seq": "5", "ge": "16", "le": "24"}
    )
    del_handler_test(m, "Vnet1001|100.64.0.0/10")
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "no ip prefix-list Vnet1001 seq 5 permit 100.64.0.0/10 ge 16 le 24" in push_call

# test if dynamic prefix-list generates ipv6 command correctly
def test_dynamic_prefix_ipv6():
    m = constructor_with_constants({})
    set_handler_test(m, "A_blue|fc00::/64", {"action": "permit"})
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "ipv6 prefix-list A_blue permit fc00::/64" in push_call
