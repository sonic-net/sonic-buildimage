from unittest.mock import MagicMock, patch, call

import os
from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test
from swsscommon import swsscommon

from bgpcfgd.managers_as_path import AsPathMgr

TEMPLATE_PATH = os.path.abspath('../../dockers/docker-fpm-frr/frr')


def constructor():
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(TEMPLATE_PATH),
        'constants': {},
    }

    m = AsPathMgr(common_objs, "CONFIG_DB", "DEVICE_METADATA")
    m.directory.put("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", {"bgp_asn": "65100", "type": "SpineRouter", "subtype": "UpstreamLC"})

    return m


def set_handler_test(manager, key, value):
    res = manager.set_handler(key, value)
    assert res, "Returns always True"


def del_handler_test(manager, key):
    res = manager.del_handler(key)
    assert res, "Returns always True"


# test if T2_GROUP_ASNS has been cleared
def test_metadata_without_asns():
    m = constructor()
    set_handler_test(m, "localhost", {"bgp_asn": "65100", "type": "SpineRouter",
                                      "subtype": "UpstreamLC"})
    assert m.cfg_mgr.push.call_count == 0, "push func is called, which is unexpected"


# test if T2_GROUP_ASNS has been added
def test_metadata_with_asns():
    m = constructor()
    set_handler_test(m, "localhost",
                     {"bgp_asn": "65100", "type": "SpineRouter",
                      "subtype": "UpstreamLC", "t2_group_asns": "64120,64121"})
    m.cfg_mgr.push.assert_has_calls([
        call("bgp as-path access-list T2_GROUP_ASNS permit _64120_"),
        call("bgp as-path access-list T2_GROUP_ASNS permit _64121_")
    ])


# test if T2_GROUP_ASNS has been updated
def test_metadata_with_asns_update():
    m = constructor()
    m.cfg_mgr.get_text = MagicMock(return_value=["bgp as-path access-list T2_GROUP_ASNS seq 5 permit _64128_"])
    set_handler_test(m, "localhost",
                     {"bgp_asn": "65100", "type": "SpineRouter",
                      "subtype": "UpstreamLC", "t2_group_asns": "64120,64121"})
    m.cfg_mgr.push.assert_has_calls([
        call("no bgp as-path access-list T2_GROUP_ASNS seq 5 permit _64128_"),
        call("bgp as-path access-list T2_GROUP_ASNS permit _64120_"),
        call("bgp as-path access-list T2_GROUP_ASNS permit _64121_")
    ])


# test if T2_GROUP_ASNS has been cleared
def test_del_handler():
    m = constructor()
    set_handler_test(m, "localhost",
                     {"bgp_asn": "65100", "type": "SpineRouter",
                      "subtype": "UpstreamLC", "t2_group_asns": "64120,64121"})
    del_handler_test(m, "localhost")
    m.cfg_mgr.push.assert_has_calls([
        call("bgp as-path access-list T2_GROUP_ASNS permit _64120_"),
        call("bgp as-path access-list T2_GROUP_ASNS permit _64121_"),
        call("no bgp as-path access-list T2_GROUP_ASNS")
    ])
