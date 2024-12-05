from unittest.mock import MagicMock, patch

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_srv6 import SRv6Mgr
from swsscommon import swsscommon

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

    return mgr

def op_test(mgr: SRv6Mgr, op, args, expected_ret, expected_cmds):
    op_test.push_list_called = False
    def push_list_checker(cmds):
        op_test.push_list_called = True
        assert len(cmds) == len(expected_cmds)
        for i in range(len(expected_cmds)):
            assert cmds[i] == expected_cmds[i]
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

    if expected_cmds:
        assert op_test.push_list_called, "cfg_mgr.push_list wasn't called"
    else:
        assert not op_test.push_list_called, "cfg_mgr.push_list was called"

def test_uN_add():
    mgr = constructor()

    op_test(mgr, 'SET', ("FCBB:BBBB:20::", {
        'action': 'uN'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator FCBB:BBBB:20:: block-len 32 node-len 16',
        'prefix FCBB:BBBB:20::/48'
    ])

def test_uDT46_add_Vrf1():
    mgr = constructor()

    _old_exists = swsscommon.SonicV2Connector().exists
    swsscommon.SonicV2Connector().exists = lambda x: True
    op_test(mgr, 'SET', ("FCBB:BBBB:20:F1::", {
        'action': 'uDT46',
        'vrf': 'Vrf1'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator FCBB:BBBB:20:: block-len 32 node-len 16',
        'prefix FCBB:BBBB:20::/48',
        'opcode ::F1 uDT46 vrf Vrf1'
    ])
    swsscommon.SonicV2Connector().exists = _old_exists

def test_uDT46_add_default_vrf():
    mgr = constructor()

    _old_exists = swsscommon.SonicV2Connector().exists
    swsscommon.SonicV2Connector().exists = lambda x: True
    op_test(mgr, 'SET', ("FCBB:BBBB:20:F2::", {
        'action': 'uDT46'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator FCBB:BBBB:20:: block-len 32 node-len 16',
        'prefix FCBB:BBBB:20::/48',
        'opcode ::F2 uDT46'
    ])
    swsscommon.SonicV2Connector().exists = _old_exists

def test_uA_add():
    mgr = constructor()

    op_test(mgr, 'SET', ("FCBB:BBBB:20:F3::", {
        'action': 'uA',
        'adj': ["FCBB:BBBB:10::1",  "FCBB:BBBB:10::2"]
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator FCBB:BBBB:20:: block-len 32 node-len 16',
        'prefix FCBB:BBBB:20::/48',
        'opcode ::F3 uA FCBB:BBBB:10::1 FCBB:BBBB:10::2'
    ])

def test_uN_del():
    mgr = constructor()

    # add uN function first
    mgr.set_handler("FCBB:BBBB:20::", {
        'action': 'uN'
    })

    # test the deletion
    op_test(mgr, 'DEL', ("FCBB:BBBB:20::", {
        'action': 'uN'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'no locator FCBB:BBBB:20:: block-len 32 node-len 16'
    ])

def test_uDT46_del():
    mgr = constructor()

    # add a uN action first to make the uDT46 action not the last function
    mgr.set_handler("FCBB:BBBB:20::", {
        'action': 'uN'
    })

    # add the uDT46 action
    mgr.set_handler("FCBB:BBBB:20:F1::", {
        'action': 'uDT46',
        'vrf': 'Vrf1'
    })

    # test the deletion of uDT46
    _old_exists = swsscommon.SonicV2Connector().exists
    swsscommon.SonicV2Connector().exists = lambda x: True
    op_test(mgr, 'DEL', ("FCBB:BBBB:20:F1::", {
        'action': 'uDT46',
        'vrf': 'Vrf1'
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator FCBB:BBBB:20:: block-len 32 node-len 16',
        'prefix FCBB:BBBB:20::/48',
        'opcode ::F1 uDT46 vrf Vrf1'
    ])
    swsscommon.SonicV2Connector().exists = _old_exists

def test_uA_del():
    mgr = constructor()

    # add a uN action first to make the uA action not the last function
    mgr.set_handler("FCBB:BBBB:20::", {
        'action': 'uN'
    })

    # add the uA action
    mgr.set_handler("FCBB:BBBB:20:F3::", {
        'action': 'uA',
        'adj': ["FCBB:BBBB:10::1",  "FCBB:BBBB:10::2"]
    })

    # test the deletion of uA
    op_test(mgr, 'DEL', ("FCBB:BBBB:20:F3::", {
        'action': 'uA',
        'adj': ["FCBB:BBBB:10::1",  "FCBB:BBBB:10::2"]
    }), expected_ret=True, expected_cmds=[
        'segment-routing',
        'srv6',
        'locators',
        'locator FCBB:BBBB:20:: block-len 32 node-len 16',
        'prefix FCBB:BBBB:20::/48',
        'no opcode ::F3 uA FCBB:BBBB:10::1 FCBB:BBBB:10::2'
    ])