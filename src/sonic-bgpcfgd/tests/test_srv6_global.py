from unittest.mock import MagicMock
import time

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_srv6_global import SRv6GlobalCfgMgr

def constructor():
    cfg_mgr = MagicMock()

    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }

    srv6global_mgr = SRv6GlobalCfgMgr(common_objs, "CONFIG_DB", "SRV6_GLOBAL")

    return srv6global_mgr

def op_test(mgr: SRv6GlobalCfgMgr, op, args, expected_ret, expected_cmds):
    op_test.push_list_called = False
    def push_list_checker(cmds):
        op_test.push_list_called = True
        assert len(cmds) == len(expected_cmds)
        for i in range(len(expected_cmds)):
            assert cmds[i].lower() == expected_cmds[i].lower()
        return True
    mgr.cfg_mgr.push_list = push_list_checker

    if op == 'SET':
        ret = mgr.set_handler(*args)
        mgr.cfg_mgr.push_list = MagicMock()
        assert expected_ret == ret
    elif op == 'DEL':
        mgr.del_handler(*args)
        mgr.cfg_mgr.push_list = MagicMock()
    else:
        mgr.cfg_mgr.push_list = MagicMock()
        assert False, "Unexpected operation {}".format(op)

    if expected_ret and expected_cmds:
        assert op_test.push_list_called, "cfg_mgr.push_list wasn't called"
    else:
        assert not op_test.push_list_called, "cfg_mgr.push_list was called"

def test_srv6_global_encap_src_addr_set():
    srv6global_mgr = constructor()

    assert(srv6global_mgr.directory.path_exist(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr"))
    assert(srv6global_mgr.directory.get(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr") == srv6global_mgr.SRV6_ENCAP_SRC_ADDR_DEFAULTS)

    op_test(srv6global_mgr, 'SET', ("Values", {
        'encap_src_addr': 'fcbb:bbbb:1::1'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'encapsulation',
        'source-address fcbb:bbbb:1::1'
    ])
    assert(srv6global_mgr.directory.path_exist(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr"))
    assert(srv6global_mgr.directory.get(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr") == "fcbb:bbbb:1::1")

def test_srv6_global_encap_src_addr_unset():
    srv6global_mgr = constructor()

    assert(srv6global_mgr.directory.path_exist(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr"))
    assert(srv6global_mgr.directory.get(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr") == srv6global_mgr.SRV6_ENCAP_SRC_ADDR_DEFAULTS)

    op_test(srv6global_mgr, 'SET', ("Values", {
        'encap_src_addr': 'fcbb:bbbb:1::1'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'encapsulation',
        'source-address fcbb:bbbb:1::1'
    ])
    assert(srv6global_mgr.directory.path_exist(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr"))
    assert(srv6global_mgr.directory.get(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr") == "fcbb:bbbb:1::1")

    op_test(srv6global_mgr, 'SET', ("Values", {
        'encap_src_addr': '::'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'encapsulation',
        'source-address ::'
    ])
    assert(srv6global_mgr.directory.path_exist(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr"))
    assert(srv6global_mgr.directory.get(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr") == srv6global_mgr.SRV6_ENCAP_SRC_ADDR_DEFAULTS)

def test_srv6_global_del():
    srv6global_mgr = constructor()

    assert(srv6global_mgr.directory.path_exist(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr"))
    assert(srv6global_mgr.directory.get(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr") == srv6global_mgr.SRV6_ENCAP_SRC_ADDR_DEFAULTS)

    op_test(srv6global_mgr, 'SET', ("Values", {
        'encap_src_addr': 'fcbb:bbbb:1::1'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'encapsulation',
        'source-address fcbb:bbbb:1::1'
    ])
    assert(srv6global_mgr.directory.path_exist(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr"))
    assert(srv6global_mgr.directory.get(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr") == "fcbb:bbbb:1::1")

    op_test(srv6global_mgr, 'DEL', ("Values",), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'encapsulation',
        'source-address ::'
    ])
    assert(srv6global_mgr.directory.path_exist(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr"))
    assert(srv6global_mgr.directory.get(srv6global_mgr.db_name, srv6global_mgr.table_name, "encap_src_addr") == srv6global_mgr.SRV6_ENCAP_SRC_ADDR_DEFAULTS)
