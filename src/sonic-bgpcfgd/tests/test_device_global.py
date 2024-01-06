from unittest.mock import MagicMock, patch

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test
from .util import load_constants
import bgpcfgd.managers_device_global
from swsscommon import swsscommon
from copy import deepcopy

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')
BASE_PATH = os.path.abspath('../sonic-bgpcfgd/tests/data/general/peer-group.conf/')
INTERNAL_BASE_PATH = os.path.abspath('../sonic-bgpcfgd/tests/data/internal/peer-group.conf/')
global_constants = {
    "bgp":  {
        "traffic_shift_community" :"12345:12345",
        "internal_community_match_tag" : "1001"
    }
}

def constructor(check_internal=False):
    cfg_mgr = MagicMock()
    def get_text():
        text = []
        for line in cfg_mgr.changes.split('\n'):
            if line.lstrip().startswith('!'):
                continue
            text.append(line)
        text += ["     "]
        return text
    def update():
        if check_internal:
            cfg_mgr.changes = get_string_from_file("/result_chasiss_packet.conf", INTERNAL_BASE_PATH)
        else:
            cfg_mgr.changes = get_string_from_file("/result_all.conf")
    def push(cfg):
        cfg_mgr.changes += cfg + "\n"
    def get_config():
        return cfg_mgr.changes
    cfg_mgr.get_text = get_text
    cfg_mgr.update = update
    cfg_mgr.push = push
    cfg_mgr.get_config = get_config

    constants = deepcopy(global_constants)
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(TEMPLATE_PATH),
        'constants': constants
    }
    mgr = bgpcfgd.managers_device_global.DeviceGlobalCfgMgr(common_objs, "CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME)
    cfg_mgr.update()
    return mgr


@patch('bgpcfgd.managers_device_global.log_debug')
def test_isolate_device(mocked_log_info):
    m = constructor()
    res = m.set_handler("STATE", {"tsa_enabled": "true"})
    assert res, "Expect True return value for set_handler"
    mocked_log_info.assert_called_with("DeviceGlobalCfgMgr::Done")
    assert m.cfg_mgr.get_config() == get_string_from_file("/result_all_isolate.conf")

@patch('bgpcfgd.managers_device_global.log_debug')
def test_isolate_device_internal_session(mocked_log_info):
    m = constructor(check_internal=True)
    res = m.set_handler("STATE", {"tsa_enabled": "true"})
    assert res, "Expect True return value for set_handler"
    mocked_log_info.assert_called_with("DeviceGlobalCfgMgr::Done")
    assert m.cfg_mgr.get_config() == get_string_from_file("/result_chassis_packet_isolate.conf", INTERNAL_BASE_PATH)


@patch('bgpcfgd.managers_device_global.log_debug')
def test_unisolate_device(mocked_log_info):
    m = constructor()
    res = m.set_handler("STATE", {"tsa_enabled": "false"})
    assert res, "Expect True return value for set_handler"
    mocked_log_info.assert_called_with("DeviceGlobalCfgMgr::Done")
    assert m.cfg_mgr.get_config() == get_string_from_file("/result_all_unisolate.conf")

@patch('bgpcfgd.managers_device_global.log_debug')
def test_unisolate_device_internal_session(mocked_log_info):
    m = constructor(check_internal=True)
    res = m.set_handler("STATE", {"tsa_enabled": "false"})
    assert res, "Expect True return value for set_handler"
    mocked_log_info.assert_called_with("DeviceGlobalCfgMgr::Done")
    assert m.cfg_mgr.get_config() == get_string_from_file("/result_chassis_packet_unisolate.conf", INTERNAL_BASE_PATH)


def test_check_state_and_get_tsa_routemaps():
    m = constructor()
    m.set_handler("STATE", {"tsa_enabled": "true"})
    res = m.check_state_and_get_tsa_routemaps(m.cfg_mgr.get_config())
    assert res == get_string_from_file("/result_isolate.conf")

    m.set_handler("STATE", {"tsa_enabled": "false"})
    res = m.check_state_and_get_tsa_routemaps(m.cfg_mgr.get_config())
    assert res == ""

def test_get_tsa_routemaps():
    m = constructor()
    assert m.get_ts_routemaps([], m.tsa_template) == ""

    res = m.get_ts_routemaps(m.cfg_mgr.get_text(), m.tsa_template)
    expected_res = get_string_from_file("/result_isolate.conf")
    assert res == expected_res

def test_get_tsb_routemaps():
    m = constructor()
    assert m.get_ts_routemaps([], m.tsb_template) == ""

    res = m.get_ts_routemaps(m.cfg_mgr.get_text(), m.tsb_template)
    expected_res = get_string_from_file("/result_unisolate.conf")
    assert res == expected_res

def get_string_from_file(filename, base_path=BASE_PATH):
    fp = open(base_path + filename, "r")
    cfg = fp.read()
    fp.close()

    return cfg

@patch('bgpcfgd.managers_device_global.log_err')
def test_set_handler_failure_case(mocked_log_info):
    m = constructor()
    res = m.set_handler("STATE", {})
    assert res == False, "Expect False return value for invalid data passed to set_handler"
    mocked_log_info.assert_called_with("DeviceGlobalCfgMgr:: data is None")

def test_del_handler():
    m = constructor()
    res = m.del_handler("STATE")
    assert res, "Expect True return value for del_handler"

