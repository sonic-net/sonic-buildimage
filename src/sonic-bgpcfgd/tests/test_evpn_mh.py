from unittest.mock import MagicMock

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_evpn_mh import EvpnMhEsMgr


def constructor(port_channels=None):
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   MagicMock(),
        'tf':        TemplateFabric(),
        'constants': {},
    }
    mgr = EvpnMhEsMgr(common_objs, "CONFIG_DB", "EVPN_ETHERNET_SEGMENT")
    for name, data in (port_channels or {}).items():
        mgr.directory.put("CONFIG_DB", "PORTCHANNEL", name, data)
    return mgr


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


TYPE_3 = {"type": "TYPE_3_MAC_BASED", "esi": "AUTO", "df_pref": "50000"}
PC121 = {"PortChannel121": {"system_mac": "02:00:00:00:01:21"}}


def test_type_3_segment_uses_the_port_id_and_system_mac():
    mgr = constructor(PC121)
    op_test(mgr, 'SET', ("PortChannel121", TYPE_3),
            expected_cmds=["interface PortChannel121",
                           "evpn mh es-id 121",
                           "evpn mh es-sys-mac 02:00:00:00:01:21",
                           "evpn mh es-df-pref 50000"])


def test_type_0_segment_uses_the_operator_esi():
    mgr = constructor()
    op_test(mgr, 'SET', ("PortChannel121",
                         {"type": "TYPE_0_OPERATOR_CONFIGURED",
                          "esi": "00:11:22:33:44:55:66:77:88:99",
                          "df_pref": "40000"}),
            expected_cmds=["interface PortChannel121",
                           "evpn mh es-id 00:11:22:33:44:55:66:77:88:99",
                           "evpn mh es-df-pref 40000"])


def test_missing_system_mac_defers_until_the_portchannel_arrives():
    """The system MAC comes from PORTCHANNEL, so on a restart it can arrive
    after the segment. Programming an ESI without it silently produces one the
    peer cannot match, so the segment has to wait."""
    mgr = constructor()
    assert mgr.set_handler("PortChannel121", TYPE_3) is False

    mgr.directory.put("CONFIG_DB", "PORTCHANNEL", "PortChannel121",
                      {"system_mac": "02:00:00:00:01:21"})
    op_test(mgr, 'SET', ("PortChannel121", TYPE_3),
            expected_cmds=["interface PortChannel121",
                           "evpn mh es-id 121",
                           "evpn mh es-sys-mac 02:00:00:00:01:21",
                           "evpn mh es-df-pref 50000"])


def test_a_different_portchannel_does_not_satisfy_the_system_mac():
    """The dependency is on the table, so any PortChannel makes the slot exist.
    The lookup has to be for this interface."""
    mgr = constructor({"PortChannel999": {"system_mac": "02:00:00:00:09:99"}})
    assert mgr.set_handler("PortChannel121", TYPE_3) is False


def test_df_pref_is_optional():
    mgr = constructor(PC121)
    op_test(mgr, 'SET', ("PortChannel121", {"type": "TYPE_3_MAC_BASED", "esi": "AUTO"}),
            expected_cmds=["interface PortChannel121",
                           "evpn mh es-id 121",
                           "evpn mh es-sys-mac 02:00:00:00:01:21"])


def test_reapplying_the_same_segment_is_a_no_op():
    mgr = constructor(PC121)
    op_test(mgr, 'SET', ("PortChannel121", TYPE_3),
            expected_cmds=["interface PortChannel121",
                           "evpn mh es-id 121",
                           "evpn mh es-sys-mac 02:00:00:00:01:21",
                           "evpn mh es-df-pref 50000"])
    op_test(mgr, 'SET', ("PortChannel121", TYPE_3), expected_cmds=None)


def test_changing_the_df_pref_is_pushed():
    mgr = constructor(PC121)
    op_test(mgr, 'SET', ("PortChannel121", TYPE_3),
            expected_cmds=["interface PortChannel121",
                           "evpn mh es-id 121",
                           "evpn mh es-sys-mac 02:00:00:00:01:21",
                           "evpn mh es-df-pref 50000"])
    changed = dict(TYPE_3, df_pref="40000")
    op_test(mgr, 'SET', ("PortChannel121", changed),
            expected_cmds=["interface PortChannel121",
                           "evpn mh es-id 121",
                           "evpn mh es-sys-mac 02:00:00:00:01:21",
                           "evpn mh es-df-pref 40000"])


def test_type_0_without_an_esi_configures_nothing():
    mgr = constructor()
    op_test(mgr, 'SET', ("PortChannel121",
                         {"type": "TYPE_0_OPERATOR_CONFIGURED", "esi": "AUTO"}),
            expected_cmds=None)


def test_unknown_type_configures_nothing():
    mgr = constructor(PC121)
    op_test(mgr, 'SET', ("PortChannel121", {"type": "TYPE_9", "esi": "AUTO"}),
            expected_cmds=None)


def test_deleting_the_segment_withdraws_it():
    mgr = constructor(PC121)
    op_test(mgr, 'SET', ("PortChannel121", TYPE_3),
            expected_cmds=["interface PortChannel121",
                           "evpn mh es-id 121",
                           "evpn mh es-sys-mac 02:00:00:00:01:21",
                           "evpn mh es-df-pref 50000"])
    op_test(mgr, 'DEL', ("PortChannel121",),
            expected_cmds=["interface PortChannel121",
                           "no evpn mh es-sys-mac",
                           "no evpn mh es-df-pref",
                           "no evpn mh es-id"])


def test_deleting_an_unknown_segment_is_a_no_op():
    mgr = constructor()
    op_test(mgr, 'DEL', ("PortChannel199",), expected_cmds=None)
