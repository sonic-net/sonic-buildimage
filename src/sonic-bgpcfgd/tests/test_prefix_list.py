from unittest.mock import MagicMock, patch

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test
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