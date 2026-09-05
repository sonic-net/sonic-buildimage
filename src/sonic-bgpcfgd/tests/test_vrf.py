from unittest.mock import MagicMock

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_vrf import VrfMgr


def constructor():
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   MagicMock(),
        'tf':        TemplateFabric(),
        'constants': {},
    }
    return VrfMgr(common_objs, "CONFIG_DB", "VRF")


def op_test(mgr, op, args, expected_cmds):
    op_test.pushed = None

    def push_list_checker(cmds):
        op_test.pushed = cmds
        return True
    mgr.cfg_mgr.push_list = push_list_checker

    if op == 'SET':
        assert mgr.set_handler(*args) is True
    else:
        mgr.del_handler(*args)
    mgr.cfg_mgr.push_list = MagicMock()

    assert op_test.pushed == expected_cmds


def test_vni_is_bound_to_the_vrf():
    mgr = constructor()
    op_test(mgr, 'SET', ("Vrf1", {"vni": "10200"}),
            expected_cmds=["vrf Vrf1", "vni 10200"])


def test_vrf_without_a_vni_configures_nothing():
    mgr = constructor()
    op_test(mgr, 'SET', ("Vrf1", {"NULL": "NULL"}), expected_cmds=None)


def test_changing_the_vni_withdraws_the_previous_one():
    mgr = constructor()
    op_test(mgr, 'SET', ("Vrf1", {"vni": "10200"}),
            expected_cmds=["vrf Vrf1", "vni 10200"])
    op_test(mgr, 'SET', ("Vrf1", {"vni": "10300"}),
            expected_cmds=["vrf Vrf1", "no vni 10200", "vni 10300"])


def test_reapplying_the_same_vni_is_a_no_op():
    mgr = constructor()
    op_test(mgr, 'SET', ("Vrf1", {"vni": "10200"}),
            expected_cmds=["vrf Vrf1", "vni 10200"])
    op_test(mgr, 'SET', ("Vrf1", {"vni": "10200"}), expected_cmds=None)


def test_vni_zero_withdraws_the_mapping():
    mgr = constructor()
    op_test(mgr, 'SET', ("Vrf1", {"vni": "10200"}),
            expected_cmds=["vrf Vrf1", "vni 10200"])
    op_test(mgr, 'SET', ("Vrf1", {"vni": "0"}),
            expected_cmds=["vrf Vrf1", "no vni 10200"])


def test_deleting_the_vrf_withdraws_the_vni():
    mgr = constructor()
    op_test(mgr, 'SET', ("Vrf1", {"vni": "10200"}),
            expected_cmds=["vrf Vrf1", "vni 10200"])
    op_test(mgr, 'DEL', ("Vrf1",), expected_cmds=["vrf Vrf1", "no vni 10200"])


def test_deleting_a_vrf_that_had_no_vni_configures_nothing():
    mgr = constructor()
    op_test(mgr, 'DEL', ("Vrf1",), expected_cmds=None)


def test_a_non_numeric_vni_is_rejected():
    mgr = constructor()
    op_test(mgr, 'SET', ("Vrf1", {"vni": "notanumber"}), expected_cmds=None)
