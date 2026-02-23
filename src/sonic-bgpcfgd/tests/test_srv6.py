from unittest.mock import MagicMock
import time

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_srv6 import SRv6Mgr

def constructor():
    cfg_mgr = MagicMock()

    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }

    loc_mgr = SRv6Mgr(common_objs, "CONFIG_DB", "SRV6_MY_LOCATORS")
    sid_mgr = SRv6Mgr(common_objs, "CONFIG_DB", "SRV6_MY_SIDS")

    return loc_mgr, sid_mgr

def op_test(mgr: SRv6Mgr, op, args, expected_ret, expected_cmds):
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

def test_locator_add():
    loc_mgr, _ = constructor()

    op_test(loc_mgr, 'SET', ("loc1", {
        'prefix': 'fcbb:bbbb:1::'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator loc1',
        'prefix fcbb:bbbb:1::/48 block-len 32 node-len 16 func-bits 16',
        'behavior usid'
    ])

    assert loc_mgr.directory.path_exist(loc_mgr.db_name, loc_mgr.table_name, "loc1")

def test_locator_del():
    loc_mgr, _ = constructor()
    loc_mgr.set_handler("loc1", {'prefix': 'fcbb:bbbb:1::'})

    op_test(loc_mgr, 'DEL', ("loc1",), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'no locator loc1'
    ])

    assert not loc_mgr.directory.path_exist(loc_mgr.db_name, loc_mgr.table_name, "loc1")

def test_uN_add():
    loc_mgr, sid_mgr = constructor()
    assert loc_mgr.set_handler("loc1", {'prefix': 'fcbb:bbbb:1::'})

    op_test(sid_mgr, 'SET', ("loc1|FCBB:BBBB:1::/48", {
        'action': 'uN'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'static-sids',
        'sid fcbb:bbbb:1::/48 locator loc1 behavior uN'
    ])

    print(loc_mgr.directory.data)
    assert sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:1::\\48")

def test_uDT46_add_vrf1():
    loc_mgr, sid_mgr = constructor()
    assert loc_mgr.set_handler("loc1", {'prefix': 'fcbb:bbbb:1::'})

    op_test(sid_mgr, 'SET', ("loc1|FCBB:BBBB:1:F2::/64", {
        'action': 'uDT46',
        'decap_vrf': 'Vrf1'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'static-sids',
        'sid fcbb:bbbb:1:f2::/64 locator loc1 behavior uDT46 vrf Vrf1'
    ])

    print(loc_mgr.directory.data)
    assert sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:1:f2::\\64")

def test_uN_del():
    loc_mgr, sid_mgr = constructor()
    assert loc_mgr.set_handler("loc1", {'prefix': 'fcbb:bbbb:1::'})

    # add uN function first
    assert sid_mgr.set_handler("loc1|FCBB:BBBB:1::/48", {
        'action': 'uN'
    })

    # test the deletion
    op_test(sid_mgr, 'DEL', ("loc1|FCBB:BBBB:1::/48",),
            expected_ret=True, expected_cmds=[
            'segment-routing',
            'srv6',
            'static-sids',
            'no sid fcbb:bbbb:1::/48 locator loc1 behavior uN'
    ])

    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:1::\\48")

def test_uDT46_del_vrf1():
    loc_mgr, sid_mgr = constructor()
    assert loc_mgr.set_handler("loc1", {'prefix': 'fcbb:bbbb:1::'})

    # add a uN action first to make the uDT46 action not the last function
    assert sid_mgr.set_handler("loc1|FCBB:BBBB:1::/48", {
        'action': 'uN'
    })

    # add the uDT46 action
    assert sid_mgr.set_handler("loc1|FCBB:BBBB:1:F2::/64", {
        'action': 'uDT46',
        "decap_vrf": "Vrf1"
    })

    # test the deletion of uDT46
    op_test(sid_mgr, 'DEL', ("loc1|FCBB:BBBB:1:F2::/64",),
            expected_ret=True, expected_cmds=[
            'segment-routing',
            'srv6',
            'static-sids',
            'no sid fcbb:bbbb:1:f2::/64 locator loc1 behavior uDT46 vrf Vrf1'
    ])

    assert sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:1::\\48")
    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:1:f2::\\64")

def test_invalid_add():
    _, sid_mgr = constructor()

    # test the addition of a SID with a non-existent locator
    op_test(sid_mgr, 'SET', ("loc2|FCBB:BBBB:21:F1::/64", {
        'action': 'uN'
    }), expected_ret=False, expected_cmds=[])

    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc2|fcbb:bbbb:21:f1::\\64")

def test_add_unmatched_sid():
    loc_mgr, sid_mgr = constructor()
    assert loc_mgr.set_handler("loc1", {'prefix': 'fcbb:bbbb:20::'})

    # test the addition of a SID with a non-matching locator
    op_test(sid_mgr, 'SET', ("loc1|FCBB:BBBB:21::/48", {
        'action': 'uN'
    }), expected_ret=False, expected_cmds=[])

    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:21::\\48")

def test_out_of_order_add():
    loc_mgr, sid_mgr = constructor()
    loc_mgr.cfg_mgr.push_list = MagicMock()
    sid_mgr.cfg_mgr.push_list = MagicMock()

    # add two sids first
    sid_mgr.handler(op='SET', key="loc1|FCBB:BBBB:20::/48", data={'action': 'uN'})
    sid_mgr.handler(op='SET', key="loc2|FCBB:BBBB:21::/48", data={'action': 'uN'})

    # verify that the sid is not added
    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:20::\\48")
    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc2|fcbb:bbbb:21::\\48")

    # add the locator loc2
    loc_mgr.handler(op='SET', key="loc2", data={'prefix': 'fcbb:bbbb:21::'})

    # verify that the sid of loc2 is programmed and the sid of loc1 is not prrogrammed
    # after locator config was added
    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:20::\\48")
    assert sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc2|fcbb:bbbb:21::\\48")

def test_out_of_order_add_wait_for_all_deps():
    loc_mgr, sid_mgr = constructor()
    sid_mgr.wait_for_all_deps = True
    loc_mgr.cfg_mgr.push_list = MagicMock()
    sid_mgr.cfg_mgr.push_list = MagicMock()

    # add two sids first
    sid_mgr.handler(op='SET', key="loc1|FCBB:BBBB:20::/48", data={'action': 'uN'})
    sid_mgr.handler(op='SET', key="loc2|FCBB:BBBB:21::/48", data={'action': 'uN'})

    # verify that the sid is not added
    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:20::\\48")
    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc2|fcbb:bbbb:21::\\48")

    # add the locator loc2
    loc_mgr.handler(op='SET', key="loc2", data={'prefix': 'fcbb:bbbb:21::'})

    # verify that neither of the sids are programmed because the manager is waiting for all dependencies
    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:20::\\48")
    assert not sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc2|fcbb:bbbb:21::\\48")

    # add the locator loc1
    loc_mgr.handler(op='SET', key="loc1", data={'prefix': 'fcbb:bbbb:20::'})

    # verify that both of the sids are programmed because all dependencies are satisfied
    assert sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc1|fcbb:bbbb:20::\\48")
    assert sid_mgr.directory.path_exist(sid_mgr.db_name, sid_mgr.table_name, "loc2|fcbb:bbbb:21::\\48")
