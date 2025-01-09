from unittest.mock import MagicMock, patch

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

    mgr = SRv6Mgr(common_objs, "CONFIG_DB", "SRV6_MY_SID_TABLE")
    assert len(mgr.sids) == 0

    # prepare the mock SRV6_MY_LOCATORS table
    mgr.directory.put("CONFIG_DB", "SRV6_MY_LOCATORS", "loc1", { 'prefix': "FCBB:BBBB:20::"})
    assert len(mgr.directory.data["CONFIG_DB__SRV6_MY_LOCATORS"]["loc1"]) == 1

    return mgr

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

def test_uN_add():
    mgr = constructor()
    print("---------", mgr.directory.get("CONFIG_DB", "SRV6_MY_LOCATORS", "loc1"), "--------------")

    op_test(mgr, 'SET', ("loc1|FCBB:BBBB:20:F1::", {
        'action': 'uN'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator loc1',
        'prefix FCBB:BBBB:20::/48 block-len 32 node-len 16 func-bits 16',
        'sid FCBB:BBBB:20:F1::/64 behavior uN'
    ])

def test_uDT46_add_vrf1():
    mgr = constructor()

    op_test(mgr, 'SET', ("loc1|FCBB:BBBB:20:F2::", {
        'action': 'uDT46',
        'decap_vrf': 'vrf1'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator loc1',
        'prefix FCBB:BBBB:20::/48 block-len 32 node-len 16 func-bits 16',
        'sid FCBB:BBBB:20:F2::/64 behavior uDT46 vrf vrf1'
    ])

def test_uN_del():
    mgr = constructor()

    # add uN function first
    mgr.set_handler("loc1|FCBB:BBBB:20:F1::", {
        'action': 'uN'
    })

    # test the deletion
    op_test(mgr, 'DEL', ("loc1|FCBB:BBBB:20:F1::",),
            expected_ret=True, expected_cmds=[
            'segment-routing',
            'srv6',
            'locators',
            'no locator loc1'
    ])

def test_uDT46_del_vrf1():
    mgr = constructor()

    # add a uN action first to make the uDT46 action not the last function
    assert mgr.set_handler("loc1|FCBB:BBBB:20:F1::", {
        'action': 'uN'
    })

    # add the uDT46 action
    assert mgr.set_handler("loc1|FCBB:BBBB:20:F2::", {
        'action': 'uDT46',
        "decap_vrf": "vrf1"
    })

    # test the deletion of uDT46
    op_test(mgr, 'DEL', ("loc1|FCBB:BBBB:20:F2::",),
            expected_ret=True, expected_cmds=[
            'segment-routing',
            'srv6',
            'locators',
            'locator loc1',
            'no sid FCBB:BBBB:20:F2::/64 behavior uDT46 vrf vrf1'
    ])

def test_invalid_add():
    mgr = constructor()

    # test the addition of a SID with a non-existent locator
    op_test(mgr, 'SET', ("loc2|FCBB:BBBB:21:F1::", {
        'action': 'uN'
    }), expected_ret=False, expected_cmds=[])