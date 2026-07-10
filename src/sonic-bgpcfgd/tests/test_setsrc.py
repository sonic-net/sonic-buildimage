from unittest.mock import MagicMock, patch

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from copy import deepcopy
from . import swsscommon_test
from swsscommon import swsscommon

import sys
sys.modules["swsscommon"] = swsscommon_test

from bgpcfgd.managers_setsrc import ZebraSetSrc

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')

def constructor():
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(TEMPLATE_PATH),
        'constants': {},
    }

    m = ZebraSetSrc(common_objs, "STATE_DB", swsscommon.STATE_INTERFACE_TABLE_NAME)
    assert m.lo_ipv4 == None
    assert m.lo_ipv6 == None

    return m

@patch('bgpcfgd.managers_setsrc.log_info')
def test_set_handler(mocked_log_info):
    m = constructor()
    res = m.set_handler("Loopback0|10.1.0.32/32", {"state": "ok"})
    assert res, "Returns always True"
    mocked_log_info.assert_called_with("The 'set src' configuration with Loopback0 ip '10.1.0.32' has been scheduled to be added")

@patch('bgpcfgd.managers_setsrc.log_err')
def test_set_handler_no_slash(mocked_log_err):
    m = constructor()
    res = m.set_handler("Loopback0|10.1.0.32", {"state": "ok"})
    assert res, "Returns always True"
    mocked_log_err.assert_called_with("Wrong Loopback0 ip prefix: '10.1.0.32'")

@patch('bgpcfgd.managers_setsrc.log_info')
def test_set_handler_ipv6(mocked_log_info):
    m = constructor()
    res = m.set_handler("Loopback0|FC00:1::32/128", {"state": "ok"})
    assert res, "Returns always True"
    mocked_log_info.assert_called_with("The 'set src' configuration with Loopback0 ip 'FC00:1::32' has been scheduled to be added")

@patch('bgpcfgd.managers_setsrc.log_err')
def test_set_handler_invalid_ip(mocked_log_err):
    m = constructor()
    res = m.set_handler("Loopback0|invalid/ip", {"state": "ok"})
    assert res, "Returns always True"
    mocked_log_err.assert_called_with("Got ambiguous ip address 'invalid'")

@patch('bgpcfgd.managers_setsrc.log_warn')
def test_del_handler(mocked_log_warn):
    m = constructor()
    del_key = "Loopback0|10.1.0.32/32"
    m.del_handler(del_key)
    mocked_log_warn.assert_called_with("Delete key '%s' is not supported for 'zebra set src' templates" % del_key)
