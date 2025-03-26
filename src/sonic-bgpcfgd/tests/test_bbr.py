from unittest.mock import MagicMock, patch

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from copy import deepcopy
from . import swsscommon_test

import sys
sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.managers_bbr import BBRMgr

global_constants = {
    "bgp": {
        "allow_list": {
            "enabled": True,
            "default_pl_rules": {
                "v4": [ "deny 0.0.0.0/0 le 17" ],
                "v6": [
                    "deny 0::/0 le 59",
                    "deny 0::/0 ge 65"
                ]
            }
        }
    }
}

def test_constructor():#m1, m2, m3):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }
    m = BBRMgr(common_objs, "CONFIG_DB", "BGP_BBR")
    assert not m.enabled
    assert len(m.bbr_enabled_pgs) == 0
    assert m.directory.get("CONFIG_DB", "BGP_BBR", "status") == "disabled"

@patch('bgpcfgd.managers_bbr.log_info')
def set_handler_common(key, value,
                       is_enabled, is_valid,
                       mocked_log_info):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': cfg_mgr,
        'tf': TemplateFabric(),
        'constants': global_constants,
    }
    m = BBRMgr(common_objs, "CONFIG_DB", "BGP_BBR")
    m.enabled = is_enabled
    prepare_config_return_value = (
        [
           ["vtysh", "-c", "clear bgp peer-group PEER_V4 soft in"],
           ["vtysh", "-c", "clear bgp peer-group PEER_V6 soft in"]
        ],
        [
            "PEER_V4",
            "PEER_V6"
        ]
    )
    m._BBRMgr__set_prepare_config = MagicMock(return_value = prepare_config_return_value)
    m.cfg_mgr.push_list = MagicMock(return_value = None)
    m.cfg_mgr.restart_peer_groups = MagicMock(return_value = None) # FIXME: check for input
    res = m.set_handler(key, value)
    assert res, "Returns always True"
    if not is_enabled:
        mocked_log_info.assert_called_with('BBRMgr::BBR is disabled. Drop the request')
    else:
        if is_valid:
            m._BBRMgr__set_prepare_config.assert_called_once_with(value["status"])
            m.cfg_mgr.push_list.assert_called_once_with(prepare_config_return_value[0])
            m.cfg_mgr.restart_peer_groups.assert_called_once_with(prepare_config_return_value[1])
        else:
            m._BBRMgr__set_prepare_config.assert_not_called()
            m.cfg_mgr.push_list.assert_not_called()
            m.cfg_mgr.restart_peer_groups.assert_not_called()

def test_set_handler_not_enabled_not_valid():
    set_handler_common("anything", {}, False, False)

def test_set_handler_enabled_not_valid():
    set_handler_common("anything", {}, True, False)

def test_set_handler_enabled_valid():
    set_handler_common("all", {"status": "enabled"}, True, True)

@patch('bgpcfgd.managers_bbr.log_err')
def test_del_handler(mocked_log_err):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': cfg_mgr,
        'tf': TemplateFabric(),
        'constants': global_constants,
    }
    m = BBRMgr(common_objs, "CONFIG_DB", "BGP_BBR")
    m.del_handler("anything")
    mocked_log_err.assert_called_with("The 'BGP_BBR' table shouldn't be removed from the db")

@patch('bgpcfgd.managers_bbr.log_info')
@patch('bgpcfgd.managers_bbr.log_err')
def __init_common(constants,
                  expected_log_info, expected_log_err, expected_bbr_enabled_pgs, expected_status,
                  mocked_log_err, mocked_log_info):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': constants,
    }

    m = BBRMgr(common_objs, "CONFIG_DB", "BGP_BBR")
    m._BBRMgr__init()
    assert m.bbr_enabled_pgs == expected_bbr_enabled_pgs
    assert m.directory.get("CONFIG_DB", "BGP_BBR", "status") == expected_status
    if expected_status == "enabled":
        assert m.enabled
    if expected_log_err is not None:
        mocked_log_err.assert_called_with(expected_log_err)
    if expected_log_info is not None:
        mocked_log_info.assert_called_with(expected_log_info)

def test___init_1():
    __init_common({}, None, "BBRMgr::Disabled: 'bgp' key is not found in constants", {}, "disabled")

def test___init_2():
    constants = deepcopy(global_constants)
    __init_common(constants, "BBRMgr::Disabled: no bgp.bbr.enabled in the constants", None, {}, "disabled")

def test___init_3():
    constants = deepcopy(global_constants)
    constants["bgp"]["bbr"] = { "123" : False }
    __init_common(constants, "BBRMgr::Disabled: no bgp.bbr.enabled in the constants", None, {}, "disabled")

def test___init_4():
    constants = deepcopy(global_constants)
    constants["bgp"]["bbr"] = { "enabled" : False }
    __init_common(constants, "BBRMgr::Disabled: no bgp.bbr.enabled in the constants", None, {}, "disabled")

def test___init_5():
    constants = deepcopy(global_constants)
    constants["bgp"]["bbr"] = { "enabled" : True }
    __init_common(constants, "BBRMgr::Disabled: no BBR enabled peers", None, {}, "disabled")

def test___init_6():
    expected_bbr_entries = {
                "PEER_V4": ["ipv4"],
                "PEER_V6": ["ipv6"],
    }
    constants = deepcopy(global_constants)
    constants["bgp"]["bbr"] = { "enabled" : True }
    constants["bgp"]["peers"] = {
        "general": {
            "bbr": expected_bbr_entries,
        }
    }
    __init_common(constants, "BBRMgr::Initialized and enabled from constants. Default state: 'disabled'", None, expected_bbr_entries, "disabled")

def test___init_7():
    expected_bbr_entries = {
                "PEER_V4": ["ipv4"],
                "PEER_V6": ["ipv6"],
    }
    constants = deepcopy(global_constants)
    constants["bgp"]["bbr"] = { "enabled" : True, "default_state": "disabled" }
    constants["bgp"]["peers"] = {
        "general": {
            "bbr": expected_bbr_entries,
        }
    }
    __init_common(constants, "BBRMgr::Initialized and enabled from constants. Default state: 'disabled'", None, expected_bbr_entries, "disabled")

def test___init_8():
    expected_bbr_entries = {
                "PEER_V4": ["ipv4"],
                "PEER_V6": ["ipv6"],
    }
    constants = deepcopy(global_constants)
    constants["bgp"]["bbr"] = { "enabled" : True, "default_state": "enabled" }
    constants["bgp"]["peers"] = {
        "general": {
            "bbr": expected_bbr_entries,
        }
    }
    __init_common(constants, "BBRMgr::Initialized and enabled from constants. Default state: 'enabled'", None, expected_bbr_entries, "enabled")

@patch('bgpcfgd.managers_bbr.BBRMgr.get_bbr_status_from_config_db', return_value='disabled')
def test___init_with_config_db_overwirte_constants(mocked_get_bbr_status_from_config_db):
    expected_bbr_entries = {
        "PEER_V4": ["ipv4"],
        "PEER_V6": ["ipv6"],
    }
    constants = deepcopy(global_constants)
    constants["bgp"]["bbr"] = {"enabled": True, "default_state": "enabled"}
    constants["bgp"]["peers"] = {
        "general": {
            "bbr": expected_bbr_entries,
        }
    }

    # BBR status from config_db should be prioritized over constants
    __init_common(constants, "BBRMgr::Initialized and enabled from config_db. Default state: 'disabled'", None, expected_bbr_entries, "disabled")

@patch('bgpcfgd.managers_bbr.BBRMgr.get_bbr_status_from_config_db', return_value='enabled')
def test___init_with_config_db_no_peers(mocked_get_bbr_status_from_config_db):

    constants = deepcopy(global_constants)
    constants["bgp"]["bbr"] = {"enabled": True}

    __init_common(constants, "BBRMgr::Disabled: no BBR enabled peers", None, {}, "disabled")

@patch('bgpcfgd.managers_bbr.log_info')
def read_pgs_common(constants, expected_log_info, expected_bbr_enabled_pgs, mocked_log_info):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': cfg_mgr,
        'tf': TemplateFabric(),
        'constants': constants,
    }
    m = BBRMgr(common_objs, "CONFIG_DB", "BGP_BBR")
    res = m._BBRMgr__read_pgs()
    assert res == expected_bbr_enabled_pgs
    if expected_log_info is not None:
        mocked_log_info.assert_called_with(expected_log_info)

def test___read_pgs_no_configuration():
    read_pgs_common(global_constants, "BBRMgr::no 'peers' was found in constants", {})

def test___read_pgs_parse_configuration():
    expected_bbr_entries = {
                "PEER_V4": ["ipv4", "ipv6"],
                "PEER_V6": ["ipv6"],
    }
    constants = deepcopy(global_constants)
    constants["bgp"]["peers"] = {
        "general": {
            "bbr": expected_bbr_entries,
        },
        "dynamic": {
            "123": {
                "PEER_V8": ["ipv10", "ipv20"],
            }
        }
    }
    read_pgs_common(constants, None, expected_bbr_entries)

@patch('bgpcfgd.managers_bbr.log_err')
def __set_validation_common(key, data, expected_log_err, expected_result, mocked_log_err):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': cfg_mgr,
        'tf': TemplateFabric(),
        'constants': global_constants,
    }
    m = BBRMgr(common_objs, "CONFIG_DB", "BGP_BBR")
    res = m._BBRMgr__set_validation(key, data)
    assert res == expected_result
    if expected_log_err is not None:
        mocked_log_err.assert_called_with(expected_log_err)

def test___set_validation_1():
    __set_validation_common("all1", {}, "Invalid key 'all1' for table 'BGP_BBR'. Only key value 'all' is supported", False)

def test___set_validation_2():
    __set_validation_common("all", {"stat": "enabled"}, "Invalid value '{'stat': 'enabled'}' for table 'BGP_BBR', key 'all'. Key 'status' in data is expected", False)

def test___set_validation_3():
    __set_validation_common("all", {"status": "enabled1"}, "Invalid value '{'status': 'enabled1'}' for table 'BGP_BBR', key 'all'. Only 'enabled' and 'disabled' are supported", False)

def test___set_validation_4():
    __set_validation_common("all", {"status": "enabled"}, None, True)

def test___set_validation_5():
    __set_validation_common("all", {"status": "disabled"}, None, True)

def __set_prepare_config_common(status, bbr_enabled_pgs, available_pgs, expected_cmds, bbr_applied_pgs=None):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': cfg_mgr,
        'tf': TemplateFabric(),
        'constants': global_constants,
    }
    m = BBRMgr(common_objs, "CONFIG_DB", "BGP_BBR")
    m.directory.data = {"CONFIG_DB__DEVICE_METADATA":
        {
            "localhost": {
                "bgp_asn": "65500"
            }
        }
    }
    m.bbr_enabled_pgs = bbr_enabled_pgs
    m._BBRMgr__get_available_peer_groups = MagicMock(return_value = sorted(available_pgs))
    cmds, peer_groups = m._BBRMgr__set_prepare_config(status)
    assert cmds == expected_cmds
    assert set(peer_groups) == (available_pgs if not bbr_applied_pgs else bbr_applied_pgs)

def test___set_prepare_config_enabled():
    __set_prepare_config_common("enabled", {
                "PEER_V4": ["ipv4", "ipv6"],
                "PEER_V6": ["ipv6"],
        }, {"PEER_V4", "PEER_V6"}, [
        'router bgp 65500',
        ' address-family ipv4',
        '  neighbor PEER_V4 allowas-in 1',
        " exit-address-family",
        ' address-family ipv6',
        '  neighbor PEER_V4 allowas-in 1',
        '  neighbor PEER_V6 allowas-in 1',
        " exit-address-family",
        "exit"
        ])

def test___set_prepare_config_disabled():
    __set_prepare_config_common("disabled", {
                "PEER_V4": ["ipv4", "ipv6"],
                "PEER_V6": ["ipv6"],
        }, {"PEER_V4", "PEER_V6"}, [
        'router bgp 65500',
        ' address-family ipv4',
        '  no neighbor PEER_V4 allowas-in 1',
        " exit-address-family",
        ' address-family ipv6',
        '  no neighbor PEER_V4 allowas-in 1',
        '  no neighbor PEER_V6 allowas-in 1',
        " exit-address-family",
        "exit"
    ])

def test___set_prepare_config_enabled_part():
    __set_prepare_config_common("enabled", {
                "PEER_V4": ["ipv4", "ipv6"],
                "PEER_V6": ["ipv6"],
                "PEER_V8": ["ipv4"]
        }, {"PEER_V4", "PEER_V6"}, [
        'router bgp 65500',
        ' address-family ipv4',
        '  neighbor PEER_V4 allowas-in 1',
        " exit-address-family",
        ' address-family ipv6',
        '  neighbor PEER_V4 allowas-in 1',
        '  neighbor PEER_V6 allowas-in 1',
        " exit-address-family",
        "exit"
    ])

def test___set_prepare_config_disabled_part():
    __set_prepare_config_common("disabled", {
                "PEER_V4": ["ipv4", "ipv6"],
                "PEER_V6": ["ipv6"],
                "PEER_v10": ["ipv4"],
        }, {"PEER_V4", "PEER_V6"}, [
        'router bgp 65500',
        ' address-family ipv4',
        '  no neighbor PEER_V4 allowas-in 1',
        " exit-address-family",
        ' address-family ipv6',
        '  no neighbor PEER_V4 allowas-in 1',
        '  no neighbor PEER_V6 allowas-in 1',
        " exit-address-family",
        "exit"
    ])
def test___set_prepare_config_enabled_multiple_peers():
    __set_prepare_config_common("enabled", {
                "PEER_V4": ["ipv4"],
                "PEER_V6": ["ipv6"],
        }, {"PEER_V4", "PEER_V4_DEPLOYMENT_ID_0", "PEER_V4_DEPLOYMENT_ID_1", "PEER_V6", "PEER_V6_DEPLOYMENT_ID_0", "PEER_V6_DEPLOYMENT_ID_1", "PEER_INVALID"},
        [
             'router bgp 65500',
        ' address-family ipv4',
        '  neighbor PEER_V4 allowas-in 1',
        '  neighbor PEER_V4_DEPLOYMENT_ID_0 allowas-in 1',
        '  neighbor PEER_V4_DEPLOYMENT_ID_1 allowas-in 1',
        " exit-address-family",
        ' address-family ipv6',
        '  neighbor PEER_V6 allowas-in 1',
        '  neighbor PEER_V6_DEPLOYMENT_ID_0 allowas-in 1',
        '  neighbor PEER_V6_DEPLOYMENT_ID_1 allowas-in 1',
        " exit-address-family",
        "exit"
        ],
        {"PEER_V4", "PEER_V4_DEPLOYMENT_ID_0", "PEER_V4_DEPLOYMENT_ID_1", "PEER_V6", "PEER_V6_DEPLOYMENT_ID_0", "PEER_V6_DEPLOYMENT_ID_1"})

def test___set_prepare_config_disabled_multiple_peers():
    __set_prepare_config_common("disabled", {
                "PEER_V4": ["ipv4"],
                "PEER_V6": ["ipv6"],
        }, {"PEER_V4", "PEER_V4_DEPLOYMENT_ID_0", "PEER_V4_DEPLOYMENT_ID_1", "PEER_V6", "PEER_V6_DEPLOYMENT_ID_0", "PEER_V6_DEPLOYMENT_ID_1", "PEER_INVALID"},
        [
             'router bgp 65500',
        ' address-family ipv4',
        '  no neighbor PEER_V4 allowas-in 1',
        '  no neighbor PEER_V4_DEPLOYMENT_ID_0 allowas-in 1',
        '  no neighbor PEER_V4_DEPLOYMENT_ID_1 allowas-in 1',
        " exit-address-family",
        ' address-family ipv6',
        '  no neighbor PEER_V6 allowas-in 1',
        '  no neighbor PEER_V6_DEPLOYMENT_ID_0 allowas-in 1',
        '  no neighbor PEER_V6_DEPLOYMENT_ID_1 allowas-in 1',
        " exit-address-family",
        "exit"
        ],
        {"PEER_V4", "PEER_V4_DEPLOYMENT_ID_0", "PEER_V4_DEPLOYMENT_ID_1", "PEER_V6", "PEER_V6_DEPLOYMENT_ID_0", "PEER_V6_DEPLOYMENT_ID_1"})

def test__get_available_peer_groups():
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': cfg_mgr,
        'tf': TemplateFabric(),
        'constants': global_constants,
    }
    m = BBRMgr(common_objs, "CONFIG_DB", "BGP_BBR")
    m.cfg_mgr.get_text = MagicMock(return_value=[
        '  neighbor PEER_V4 peer-group',
        '  neighbor PEER_V6 peer-group',
        '  address-family ipv4',
        '    neighbor PEER_V4 allowas-in 1',
        '    neighbor PEER_V4 soft-reconfiguration inbound',
        '    neighbor PEER_V4 route-map FROM_BGP_PEER_V4 in',
        '    neighbor PEER_V4 route-map TO_BGP_PEER_V4 out',
        '  exit-address-family',
        '  address-family ipv6',
        '    neighbor PEER_V6 allowas-in 1',
        '    neighbor PEER_V6 soft-reconfiguration inbound',
        '    neighbor PEER_V6 route-map FROM_BGP_PEER_V6 in',
        '    neighbor PEER_V6 route-map TO_BGP_PEER_V6 out',
        '  exit-address-family',
        '     ',
    ])
    res = m._BBRMgr__get_available_peer_groups()
    assert res == {"PEER_V4", "PEER_V6"}
