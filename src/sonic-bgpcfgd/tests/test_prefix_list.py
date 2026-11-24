from unittest.mock import MagicMock, patch

import os
import pytest
from . import swsscommon_test

import sys
sys.modules["swsscommon"] = swsscommon_test
sys.modules["swsscommon.swsscommon"] = swsscommon_test.swsscommon

from swsscommon import swsscommon

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
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

# test if dynamic prefix-list missing action logs warning and skips config push
@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_dynamic_prefix_missing_action_logs_warn_and_skips_push(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "Vnet1001|100.64.0.0/10", {})
    mocked_log_warn.assert_called_with(
        "PrefixListMgr:: Mandatory field 'action' is not defined for prefix list 'Vnet1001'"
    )
    m.cfg_mgr.push.assert_not_called()

# test if dynamic prefix-list empty action logs warning and skips config push
@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_dynamic_prefix_empty_action_logs_warn_and_skips_push(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "Vnet1001|100.64.0.0/10", {"action": ""})
    mocked_log_warn.assert_called_with(
        "PrefixListMgr:: Mandatory field 'action' is not defined for prefix list 'Vnet1001'"
    )
    m.cfg_mgr.push.assert_not_called()

# test if dynamic prefix-list delete falls back to delete-by-name when cache is empty
@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_dynamic_prefix_delete_empty_cache_uses_delete_by_name_fallback(mocked_log_warn):
    m = constructor_with_constants({})
    del_handler_test(m, "Vnet1001|100.64.0.0/10")
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "no ip prefix-list Vnet1001" in push_call
    mocked_log_warn.assert_called_with(
        "PrefixListMgr:: Missing cached fields for delete of prefix list 'Vnet1001'; issued best-effort delete-by-name"
    )

def get_pushed_cmds(manager):
    return [c[0][0] for c in manager.cfg_mgr.push.call_args_list]

@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_named_entry_ipv4(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|pl10",
                     {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10", "ge": "17", "le": "17"})
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "ip prefix-list TEST_PL seq 10 permit 10.10.0.0/16 ge 17 le 17" in push_call
    mocked_log_warn.assert_not_called()

def test_named_entry_ipv6():
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|v6_10", {"prefix": "fc00:10::/48", "action": "permit", "seq": "10", "le": "64"})
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "ipv6 prefix-list TEST_PL seq 10 permit fc00:10::/48 le 64" in push_call

def test_named_entry_prefix_normalized():
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|pl10", {"prefix": "10.10.1.1/16", "action": "permit"})
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert "ip prefix-list TEST_PL permit 10.10.0.0/16" in push_call

def test_named_entries_same_prefix_coexist():
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|pl10",
                     {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10", "ge": "17", "le": "17"})
    set_handler_test(m, "TEST_PL|pl20",
                     {"prefix": "10.10.0.0/16", "action": "permit", "seq": "20", "ge": "24", "le": "24"})
    cmds = get_pushed_cmds(m)
    assert cmds == [
        "\nip prefix-list TEST_PL seq 10 permit 10.10.0.0/16 ge 17 le 17",
        "\nip prefix-list TEST_PL seq 20 permit 10.10.0.0/16 ge 24 le 24",
    ]
    cache = m.directory.get_slot("CONFIG_DB", "PREFIX_LIST")
    assert "TEST_PL|pl10" in cache and "TEST_PL|pl20" in cache

def test_named_entry_delete_uses_cache():
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|pl10",
                     {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10", "ge": "17", "le": "17"})
    set_handler_test(m, "TEST_PL|pl20",
                     {"prefix": "10.10.0.0/16", "action": "permit", "seq": "20", "ge": "24", "le": "24"})
    del_handler_test(m, "TEST_PL|pl10")
    push_call = m.cfg_mgr.push.call_args[0][0]
    assert push_call == "\nno ip prefix-list TEST_PL seq 10 permit 10.10.0.0/16 ge 17 le 17"
    cache = m.directory.get_slot("CONFIG_DB", "PREFIX_LIST")
    assert "TEST_PL|pl10" not in cache
    assert "TEST_PL|pl20" in cache

@pytest.mark.parametrize("entry_name", ["pl99", "123", "192.0.2.1", "2001:db8::1"])
@pytest.mark.parametrize("cached_data", [None, {"action": "permit"}])
@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_named_entry_delete_without_cached_prefix_is_skipped(mocked_log_warn, entry_name, cached_data):
    m = constructor_with_constants({})
    if cached_data is not None:
        m.directory.put("CONFIG_DB", "PREFIX_LIST", "TEST_PL|" + entry_name, cached_data)
    del_handler_test(m, "TEST_PL|" + entry_name)
    m.cfg_mgr.push.assert_not_called()
    mocked_log_warn.assert_called_with(
        "PrefixListMgr:: No cached entry for '%s' in prefix list 'TEST_PL'; skip delete" % entry_name
    )

def test_named_entry_update_replaces_old_line():
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|pl10",
                     {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10", "ge": "17", "le": "17"})
    set_handler_test(m, "TEST_PL|pl10",
                     {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10", "ge": "17", "le": "28"})
    cmds = get_pushed_cmds(m)
    assert cmds == [
        "\nip prefix-list TEST_PL seq 10 permit 10.10.0.0/16 ge 17 le 17",
        "\nno ip prefix-list TEST_PL seq 10 permit 10.10.0.0/16 ge 17 le 17",
        "\nip prefix-list TEST_PL seq 10 permit 10.10.0.0/16 ge 17 le 28",
    ]
    cached = m.directory.get_slot("CONFIG_DB", "PREFIX_LIST")["TEST_PL|pl10"]
    assert cached["le"] == "28"

def test_named_entry_update_changes_prefix():
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|e1", {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10"})
    set_handler_test(m, "TEST_PL|e1", {"prefix": "fc00:10::/48", "action": "permit", "seq": "10"})
    cmds = get_pushed_cmds(m)
    assert cmds[1:] == [
        "\nno ip prefix-list TEST_PL seq 10 permit 10.10.0.0/16",
        "\nipv6 prefix-list TEST_PL seq 10 permit fc00:10::/48",
    ]

def test_named_entry_same_data_no_removal():
    m = constructor_with_constants({})
    data = {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10"}
    set_handler_test(m, "TEST_PL|pl10", dict(data))
    set_handler_test(m, "TEST_PL|pl10", dict(data))
    assert not any(c.startswith("\nno ") for c in get_pushed_cmds(m))

@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_named_entry_invalid_update_keeps_old_entry(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|pl10", {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10"})
    set_handler_test(m, "TEST_PL|pl10", {"prefix": "10.10.0.0/16", "seq": "10"})
    mocked_log_warn.assert_called_with(
        "PrefixListMgr:: Mandatory field 'action' is not defined for prefix list 'TEST_PL'"
    )
    assert get_pushed_cmds(m) == ["\nip prefix-list TEST_PL seq 10 permit 10.10.0.0/16"]
    del_handler_test(m, "TEST_PL|pl10")
    assert m.cfg_mgr.push.call_args[0][0] == "\nno ip prefix-list TEST_PL seq 10 permit 10.10.0.0/16"

def test_legacy_entry_update_replaces_old_line():
    m = constructor_with_constants({})
    set_handler_test(m, "Vnet1001|100.64.0.0/10", {"action": "permit", "seq": "5", "ge": "16", "le": "24"})
    set_handler_test(m, "Vnet1001|100.64.0.0/10", {"action": "permit", "seq": "5", "ge": "16", "le": "20"})
    cmds = get_pushed_cmds(m)
    assert cmds[1:] == [
        "\nno ip prefix-list Vnet1001 seq 5 permit 100.64.0.0/10 ge 16 le 24",
        "\nip prefix-list Vnet1001 seq 5 permit 100.64.0.0/10 ge 16 le 20",
    ]

@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_named_entry_name_with_slash_is_rejected(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|10.10.0.0/16", {"prefix": "10.10.0.0/16", "action": "permit"})
    m.cfg_mgr.push.assert_not_called()
    mocked_log_warn.assert_called_with(
        "PrefixListMgr:: Entry name '10.10.0.0/16' is invalid for prefix list 'TEST_PL': when the 'prefix' field "
        "is set, the key must be a name without '/' or '|'"
    )

@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_named_entry_invalid_prefix_field(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "TEST_PL|pl10", {"prefix": "not_a_prefix", "action": "permit"})
    m.cfg_mgr.push.assert_not_called()
    mocked_log_warn.assert_called_with(
        "PrefixListMgr:: Prefix 'not_a_prefix' format is wrong for prefix list 'TEST_PL'"
    )

@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_empty_prefix_field_uses_legacy_key(mocked_log_warn):
    m = constructor_with_constants({})
    set_handler_test(m, "Vnet1001|100.64.0.0/10", {"prefix": "", "action": "permit"})
    assert m.cfg_mgr.push.call_args[0][0] == "\nip prefix-list Vnet1001 permit 100.64.0.0/10"
    mocked_log_warn.assert_not_called()

@pytest.mark.parametrize("partial_cache", [False, True])
@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_delete_by_name_fallback_skipped_when_list_has_entries(mocked_log_warn, partial_cache):
    m = constructor_with_constants({})
    set_handler_test(m, "Vnet1001|pl10", {"prefix": "10.10.0.0/16", "action": "permit", "seq": "10"})
    if partial_cache:
        m.directory.put("CONFIG_DB", "PREFIX_LIST", "Vnet1001|100.64.0.0/10", {"prefix": "100.64.0.0/10"})
    del_handler_test(m, "Vnet1001|100.64.0.0/10")
    assert get_pushed_cmds(m) == ["\nip prefix-list Vnet1001 seq 10 permit 10.10.0.0/16"]
    mocked_log_warn.assert_called_with(
        "PrefixListMgr:: Missing cached fields for '100.64.0.0/10'; prefix list 'Vnet1001' still has other entries, "
        "skip best-effort delete-by-name"
    )

def test_delete_by_name_fallback_other_family_only():
    m = constructor_with_constants({})
    set_handler_test(m, "Vnet1001|v6_10", {"prefix": "fc00:10::/48", "action": "permit", "seq": "10"})
    del_handler_test(m, "Vnet1001|100.64.0.0/10")
    assert m.cfg_mgr.push.call_args[0][0] == "\nno ip prefix-list Vnet1001"

def test_set_handler_does_not_mutate_input():
    m = constructor_with_constants({})
    data = {"prefix": "10.10.0.0/16", "action": "permit"}
    set_handler_test(m, "TEST_PL|pl10", data)
    assert data == {"prefix": "10.10.0.0/16", "action": "permit"}

@patch('bgpcfgd.managers_prefix_list.log_debug')
def test_named_entry_static_type(mocked_log_debug):
    m = constructor_with_constants({})
    set_handler_test(m, "SUPPRESS_PREFIX|sp1", {"prefix": "10.0.0.0/24"})
    mocked_log_debug.assert_called_with("PrefixListMgr:: Suppress prefix 10.0.0.0/24 added to suppress_prefix configuration")
    del_handler_test(m, "SUPPRESS_PREFIX|sp1")
    mocked_log_debug.assert_called_with("PrefixListMgr:: Suppress prefix 10.0.0.0/24 removed from suppress_prefix configuration")
    assert "SUPPRESS_PREFIX|sp1" not in m.directory.get_slot("CONFIG_DB", "PREFIX_LIST")

@pytest.mark.parametrize("prefix_type", ["SUPPRESS_PREFIX", "ANCHOR_PREFIX"])
@pytest.mark.parametrize("new_prefix, ipv", [("10.1.0.0/24", "ip"), ("fc00::/64", "ipv6")])
def test_static_named_entry_update_replaces_old_configuration(prefix_type, new_prefix, ipv):
    m = constructor()
    key = prefix_type + "|sp1"
    set_handler_test(m, key, {"prefix": "10.0.0.0/24"})
    m.cfg_mgr.push.reset_mock()
    m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost",
                    {"bgp_asn": "65200", "type": "SpineRouter", "subtype": "UpstreamLC"})

    set_handler_test(m, key, {"prefix": new_prefix})
    cmds = get_pushed_cmds(m)
    assert len(cmds) == 2
    old_name = "SUPPRESS_IPV4_PREFIX" if prefix_type == "SUPPRESS_PREFIX" else "ANCHOR_CONTRIBUTING_ROUTES"
    new_name = "SUPPRESS_IPV6_PREFIX" if prefix_type == "SUPPRESS_PREFIX" and ipv == "ipv6" else old_name
    assert cmds[0].startswith("\nno ip prefix-list %s permit 10.0.0.0/24" % old_name)
    assert cmds[1].startswith("\n%s prefix-list %s permit %s" % (ipv, new_name, new_prefix))
    if prefix_type == "ANCHOR_PREFIX":
        assert "no aggregate-address 10.0.0.0/24 route-map TAG_ANCHOR_COMMUNITY" in cmds[0]
        assert "router bgp 65100" in cmds[0]
        assert "router bgp 65200" in cmds[1]
        assert "aggregate-address %s route-map TAG_ANCHOR_COMMUNITY" % new_prefix in cmds[1]
    assert m.directory.get_slot("CONFIG_DB", "PREFIX_LIST")[key]["prefix"] == new_prefix

    del_handler_test(m, key)
    delete_cmd = get_pushed_cmds(m)[-1]
    assert delete_cmd.startswith("\nno %s prefix-list %s permit %s" % (ipv, new_name, new_prefix))
    if prefix_type == "ANCHOR_PREFIX":
        assert "no aggregate-address %s route-map TAG_ANCHOR_COMMUNITY" % new_prefix in delete_cmd

@pytest.mark.parametrize("prefix_type", ["SUPPRESS_PREFIX", "ANCHOR_PREFIX"])
@patch('bgpcfgd.managers_prefix_list.log_warn')
def test_static_named_entry_invalid_metadata_update_keeps_old_configuration(mocked_log_warn, prefix_type):
    m = constructor()
    key = prefix_type + "|sp1"
    set_handler_test(m, key, {"prefix": "10.0.0.0/24"})
    previous = dict(m.directory.get_slot("CONFIG_DB", "PREFIX_LIST")[key])
    m.cfg_mgr.push.reset_mock()
    m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost",
                    {"type": "SpineRouter", "subtype": "UpstreamLC"})

    set_handler_test(m, key, {"prefix": "10.1.0.0/24"})
    m.cfg_mgr.push.assert_not_called()
    assert m.directory.get_slot("CONFIG_DB", "PREFIX_LIST")[key] == previous
    mocked_log_warn.assert_called_with("PrefixListMgr:: Missing metadata key: 'bgp_asn'")

@pytest.mark.parametrize("prefix_type", ["SUPPRESS_PREFIX", "ANCHOR_PREFIX"])
def test_static_named_entry_unchanged_command_does_not_remove_configuration(prefix_type):
    m = constructor()
    key = prefix_type + "|sp1"
    set_handler_test(m, key, {"prefix": "10.0.0.0/24"})
    m.cfg_mgr.push.reset_mock()

    set_handler_test(m, key, {"prefix": "10.0.0.0/24", "action": "permit", "seq": "20"})
    cmds = get_pushed_cmds(m)
    assert len(cmds) == 1
    assert cmds[0].startswith("\nip prefix-list ")

@pytest.mark.parametrize("prefix_type", ["SUPPRESS_PREFIX", "ANCHOR_PREFIX"])
def test_static_named_entry_update_preserves_configuration_shared_by_other_entry(prefix_type):
    m = constructor()
    key = prefix_type + "|sp1"
    other_key = prefix_type + "|sp2"
    set_handler_test(m, key, {"prefix": "10.0.0.0/24"})
    set_handler_test(m, other_key, {"prefix": "10.0.0.0/24"})
    m.cfg_mgr.push.reset_mock()

    set_handler_test(m, key, {"prefix": "10.1.0.0/24"})
    cmds = get_pushed_cmds(m)
    assert len(cmds) == 1
    assert cmds[0].startswith("\nip prefix-list ")
    assert "permit 10.1.0.0/24" in cmds[0]
    assert m.directory.get_slot("CONFIG_DB", "PREFIX_LIST")[other_key]["prefix"] == "10.0.0.0/24"
