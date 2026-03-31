from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_aggregate_address import AggregateAddressMgr, BGP_AGGREGATE_ADDRESS_TABLE_NAME, BGP_BBR_TABLE_NAME
from bgpcfgd.managers_aggregate_address import generate_aggregate_address_commands, COMMON_TRUE_STRING, COMMON_FALSE_STRING
import pytest
from swsscommon import swsscommon
from unittest.mock import MagicMock, patch


CONFIG_DB_NAME = "CONFIG_DB"
BGP_BBR_TABLE_NAME = "BGP_BBR"
BGP_BBR_STATUS_KEY = "status"
BGP_BBR_STATUS_ENABLED = "enabled"
BGP_BBR_STATUS_DISABLED = "disabled"


class MockAddressTable(object):
    def __init__(self, db_name, table_name):
        self.table_name = table_name
        self.addresses = {}

    def getKeys(self):
        return list(self.addresses.keys())

    def get(self, key):
        return (True, self.addresses.get(key))

    def delete(self, key):
        self.addresses.pop(key, None)

    def hset(self, hash, key, value):
        if hash not in self.addresses:
            self.addresses[hash] = {}
        self.addresses[hash][key] = value


@patch('swsscommon.swsscommon.Table')
def constructor(mock_table, bbr_status):
    mock_table = MockAddressTable
    cfg_mgr = MagicMock()

    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
        'state_db_conn': None
    }

    mgr = AggregateAddressMgr(common_objs, CONFIG_DB_NAME, BGP_AGGREGATE_ADDRESS_TABLE_NAME)
    mgr.address_table = mock_table("", BGP_AGGREGATE_ADDRESS_TABLE_NAME)
    mgr.directory.put(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY, bbr_status)
    mgr.directory.put(CONFIG_DB_NAME, swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost", {"bgp_asn": "65001"})

    return mgr


def set_del_test(mgr, op, args, expected_cmds=None):
    set_del_test.push_list_called = False
    def push_list(cmds):
        if cmds != expected_cmds:
            import pdb; pdb.set_trace()
        set_del_test.push_list_called = True
        assert cmds == expected_cmds
        return True
    mgr.cfg_mgr.push_list = push_list

    if op == "SET":
        mgr.set_handler(*args)
    elif op == "DEL":
        mgr.del_handler(*args)
    elif op == "SWITCH":
        return # Only change the expected commands is enough for this operation
    else:
        assert False, "Operation is not supported"

    if expected_cmds:
        assert set_del_test.push_list_called, "cfg_mgr.push_list wasn't called"
    else:
        assert not set_del_test.push_list_called, "cfg_mgr.push_list was called"


@pytest.mark.parametrize("aggregate_prefix", ["192.168.1.1", "2ff::1/64"])
@pytest.mark.parametrize("bbr_status", [BGP_BBR_STATUS_ENABLED, BGP_BBR_STATUS_DISABLED])
@pytest.mark.parametrize("bbr_required", ["true", "false"])
@pytest.mark.parametrize("switch_bbr_state", [False, True])
@pytest.mark.parametrize("summary_only", ["true", "false"])
@pytest.mark.parametrize("as_set", ["true", "false"])
@pytest.mark.parametrize("aggregate_address_prefix_list", ["", "aggregate_PL"])
@pytest.mark.parametrize("contributing_address_prefix_list", ["", "contributing_PL"])
def test_all_parameters_combination(
    aggregate_prefix,
    bbr_status,
    bbr_required,
    switch_bbr_state,
    summary_only,
    as_set,
    aggregate_address_prefix_list,
    contributing_address_prefix_list
):
    """
    Test all combinations of parameters

    There are steps in this test:
        1. Set address: simulate when we add an aggregate address into config DB
        2: BBR state switch: simulate when BBR state is changed in config DB
        3. Del address: simulate when we delete an aggregate address from config DB
    
    When checking the results, we check that:
        - The commands sent to the config manager are correct
        - The address is added/removed from the address table
        - The address data in the state DB is correct, for example, if BBR is required, the state should be active, otherwise inactive
    """

    mgr = constructor(bbr_status=bbr_status)
    # 1. Set address
    __set_handler_validate(
        mgr,
        aggregate_prefix,
        bbr_status,
        bbr_required,
        summary_only,
        as_set,
        aggregate_address_prefix_list,
        contributing_address_prefix_list
    )

    # 2. BBR state switch
    if switch_bbr_state:
        __switch_bbr_state(
            mgr,
            aggregate_prefix,
            bbr_required,
            aggregate_address_prefix_list,
            contributing_address_prefix_list,
            summary_only,
            as_set
        )

    # 3. Del address
    __del_handler_validate(
        mgr,
        aggregate_prefix,
        bbr_status,
        bbr_required,
        summary_only,
        as_set,
        aggregate_address_prefix_list,
        contributing_address_prefix_list
    )


def __set_handler_validate(
    mgr,
    aggregate_prefix,
    bbr_status,
    bbr_required,
    summary_only,
    as_set,
    aggregate_address_prefix_list,
    contributing_address_prefix_list
):
    attr = (
        ('bbr-required', bbr_required),
        ('summary-only', summary_only),
        ('as-set', as_set),
        ('aggregate-address-prefix-list', aggregate_address_prefix_list),
        ('contributing-address-prefix-list', contributing_address_prefix_list)
    )
    expecting_active_state = True if bbr_required == 'false' or bbr_status == BGP_BBR_STATUS_ENABLED else False
    expected_state = {
        'aggregate-address-prefix-list': aggregate_address_prefix_list,
        'contributing-address-prefix-list': contributing_address_prefix_list,
        'state': 'active' if expecting_active_state else 'inactive',
        'bbr-required': bbr_required,
        'summary-only': summary_only,
        'as-set': as_set
    }
    except_cmds = [
        'router bgp 65001',
        'address-family ' + ('ipv4' if '.' in aggregate_prefix else 'ipv6'),
        'aggregate-address ' + aggregate_prefix + (' summary-only' if summary_only == 'true' else '') + (' as-set' if as_set == 'true' else ''),
        'exit-address-family',
        'exit'
    ]
    if aggregate_address_prefix_list:
        except_cmds.append(('ip' if '.' in aggregate_prefix else 'ipv6') + ' prefix-list %s permit %s' % (aggregate_address_prefix_list, aggregate_prefix))
    if contributing_address_prefix_list:
        except_cmds.append(('ip' if '.' in aggregate_prefix else 'ipv6') + ' prefix-list %s permit %s' % (contributing_address_prefix_list, aggregate_prefix) + " le" + (" 32" if '.' in aggregate_prefix else " 128"))
    set_del_test(
        mgr,
        "SET",
        (aggregate_prefix, attr),
        except_cmds if expecting_active_state else None
    )
    assert [aggregate_prefix] == mgr.address_table.getKeys()
    _, data = mgr.address_table.get(aggregate_prefix)
    assert data == expected_state


def __del_handler_validate(
    mgr,
    aggregate_prefix,
    bbr_status,
    bbr_required,
    summary_only,
    as_set,
    aggregate_address_prefix_list,
    contributing_address_prefix_list
):
    except_cmds = [
        'router bgp 65001',
        'address-family ' + ('ipv4' if '.' in aggregate_prefix else 'ipv6'),
        'no aggregate-address ' + aggregate_prefix + (' summary-only' if summary_only == 'true' else '') + (' as-set' if as_set == 'true' else ''),
        'exit-address-family',
        'exit'
    ]
    if aggregate_address_prefix_list:
        except_cmds.append('no ' + ('ip' if '.' in aggregate_prefix else 'ipv6') + ' prefix-list %s permit %s' % (aggregate_address_prefix_list, aggregate_prefix))
    if contributing_address_prefix_list:
        except_cmds.append('no ' + ('ip' if '.' in aggregate_prefix else 'ipv6') + ' prefix-list %s permit %s' % (contributing_address_prefix_list, aggregate_prefix) + " le" + (" 32" if '.' in aggregate_prefix else " 128"))
    set_del_test(
        mgr,
        "DEL",
        (aggregate_prefix,),
        except_cmds
    )
    assert aggregate_prefix not in mgr.address_table.getKeys()
    assert not mgr.address_table.get(aggregate_prefix)[1], "Address should be removed from the table"


def __switch_bbr_state(
    mgr,
    aggregate_prefix,
    bbr_required,
    aggregate_address_prefix_list,
    contributing_address_prefix_list,
    summary_only,
    as_set
):
    new_bbr_status = BGP_BBR_STATUS_DISABLED if bbr_required == BGP_BBR_STATUS_ENABLED else BGP_BBR_STATUS_ENABLED
    expecting_active_state = True if bbr_required == 'false' or new_bbr_status == BGP_BBR_STATUS_ENABLED else False
    expected_state = {
        'aggregate-address-prefix-list': aggregate_address_prefix_list,
        'contributing-address-prefix-list': contributing_address_prefix_list,
        'state': 'active' if expecting_active_state else 'inactive',
        'bbr-required': bbr_required,
        'summary-only': summary_only,
        'as-set': as_set
    }
    set_cmds = [
        'router bgp 65001',
        'address-family ' + ('ipv4' if '.' in aggregate_prefix else 'ipv6'),
        'aggregate-address ' + aggregate_prefix + (' summary-only' if summary_only == 'true' else '') + (' as-set' if as_set == 'true' else ''),
        'exit-address-family',
        'exit'
    ]
    if aggregate_address_prefix_list:
        set_cmds.append(('ip' if '.' in aggregate_prefix else 'ipv6') + ' prefix-list %s permit %s' % (aggregate_address_prefix_list, aggregate_prefix))
    if contributing_address_prefix_list:
        set_cmds.append(('ip' if '.' in aggregate_prefix else 'ipv6') + ' prefix-list %s permit %s' % (contributing_address_prefix_list, aggregate_prefix) + " le" + (" 32" if '.' in aggregate_prefix else " 128"))
    del_cmds = [
        'router bgp 65001',
        'address-family ' + ('ipv4' if '.' in aggregate_prefix else 'ipv6'),
        'no aggregate-address ' + aggregate_prefix + (' summary-only' if summary_only == 'true' else '') + (' as-set' if as_set == 'true' else ''),
        'exit-address-family',
        'exit'
    ]
    if aggregate_address_prefix_list:
        del_cmds.append('no ' + ('ip' if '.' in aggregate_prefix else 'ipv6') + ' prefix-list %s permit %s' % (aggregate_address_prefix_list, aggregate_prefix))
    if contributing_address_prefix_list:
        del_cmds.append('no ' + ('ip' if '.' in aggregate_prefix else 'ipv6') + ' prefix-list %s permit %s' % (contributing_address_prefix_list, aggregate_prefix) + " le" + (" 32" if '.' in aggregate_prefix else " 128"))
    set_del_test(
        mgr,
        "SWITCH",
        None,
        set_cmds if expecting_active_state else del_cmds
    )
    mgr.directory.put(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY, new_bbr_status)
    assert [aggregate_prefix] == mgr.address_table.getKeys()
    _, data = mgr.address_table.get(aggregate_prefix)
    assert data == expected_state


class TestGenerateAggregateAddressRemovalCommands:
    """Tests that verify the removal command includes summary-only/as-set flags.

    This is the regression test for the bug where 'no aggregate-address <prefix>'
    was generated without the flags, causing FRR to silently ignore the removal
    when the running config had 'aggregate-address <prefix> summary-only'.
    """

    def test_remove_with_summary_only(self):
        cmds = generate_aggregate_address_commands(
            asn="65001", prefix="10.100.0.0/16", is_v4=True,
            is_remove=True, summary_only=COMMON_TRUE_STRING, as_set=COMMON_FALSE_STRING
        )
        assert "no aggregate-address 10.100.0.0/16 summary-only" in cmds

    def test_remove_with_as_set(self):
        cmds = generate_aggregate_address_commands(
            asn="65001", prefix="10.100.0.0/16", is_v4=True,
            is_remove=True, summary_only=COMMON_FALSE_STRING, as_set=COMMON_TRUE_STRING
        )
        assert "no aggregate-address 10.100.0.0/16 as-set" in cmds

    def test_remove_with_summary_only_and_as_set(self):
        cmds = generate_aggregate_address_commands(
            asn="65001", prefix="10.100.0.0/16", is_v4=True,
            is_remove=True, summary_only=COMMON_TRUE_STRING, as_set=COMMON_TRUE_STRING
        )
        assert "no aggregate-address 10.100.0.0/16 summary-only as-set" in cmds

    def test_remove_without_flags(self):
        cmds = generate_aggregate_address_commands(
            asn="65001", prefix="10.100.0.0/16", is_v4=True,
            is_remove=True, summary_only=COMMON_FALSE_STRING, as_set=COMMON_FALSE_STRING
        )
        assert "no aggregate-address 10.100.0.0/16" in cmds

    def test_remove_ipv6_with_summary_only(self):
        cmds = generate_aggregate_address_commands(
            asn="65001", prefix="2001:db8::/32", is_v4=False,
            is_remove=True, summary_only=COMMON_TRUE_STRING, as_set=COMMON_FALSE_STRING
        )
        assert "no aggregate-address 2001:db8::/32 summary-only" in cmds
        assert "address-family ipv6" in cmds

    def test_add_with_summary_only_still_works(self):
        cmds = generate_aggregate_address_commands(
            asn="65001", prefix="10.100.0.0/16", is_v4=True,
            is_remove=False, summary_only=COMMON_TRUE_STRING, as_set=COMMON_FALSE_STRING
        )
        assert "aggregate-address 10.100.0.0/16 summary-only" in cmds


class TestAddressDelHandlerPassesFlags:
    """Tests that address_del_handler passes summary-only/as-set from state DB."""

    def test_del_handler_with_summary_only_in_state(self):
        """Reproduce the exact bug: add with summary-only=true, then delete.
        The removal command must include 'summary-only'."""
        mgr = constructor(bbr_status=BGP_BBR_STATUS_ENABLED)

        # Set address with summary-only=true
        attr = (
            ('bbr-required', 'false'),
            ('summary-only', 'true'),
            ('as-set', 'false'),
            ('aggregate-address-prefix-list', ''),
            ('contributing-address-prefix-list', '')
        )
        mgr.set_handler("10.100.0.0/16", attr)

        # Verify state DB has the flags
        _, state_data = mgr.address_table.get("10.100.0.0/16")
        assert state_data['summary-only'] == 'true'

        # Now delete — the removal cmd must include summary-only
        push_list_calls = []
        mgr.cfg_mgr.push_list = lambda cmds: push_list_calls.append(cmds) or True
        mgr.del_handler("10.100.0.0/16")

        assert len(push_list_calls) == 1
        assert "no aggregate-address 10.100.0.0/16 summary-only" in push_list_calls[0]

    def test_bbr_disable_removes_with_summary_only(self):
        """When BBR is disabled, on_bbr_change must generate removal with summary-only."""
        mgr = constructor(bbr_status=BGP_BBR_STATUS_ENABLED)

        # Set address with bbr-required=true and summary-only=true
        attr = (
            ('bbr-required', 'true'),
            ('summary-only', 'true'),
            ('as-set', 'false'),
            ('aggregate-address-prefix-list', ''),
            ('contributing-address-prefix-list', '')
        )
        mgr.set_handler("10.100.0.0/16", attr)

        # Now switch BBR to disabled
        push_list_calls = []
        mgr.cfg_mgr.push_list = lambda cmds: push_list_calls.append(cmds) or True
        mgr.directory.put(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY, BGP_BBR_STATUS_DISABLED)

        assert len(push_list_calls) == 1
        assert "no aggregate-address 10.100.0.0/16 summary-only" in push_list_calls[0]

    def test_bbr_disable_removes_with_as_set(self):
        """When BBR is disabled, on_bbr_change must generate removal with as-set."""
        mgr = constructor(bbr_status=BGP_BBR_STATUS_ENABLED)

        attr = (
            ('bbr-required', 'true'),
            ('summary-only', 'false'),
            ('as-set', 'true'),
            ('aggregate-address-prefix-list', ''),
            ('contributing-address-prefix-list', '')
        )
        mgr.set_handler("10.100.0.0/16", attr)

        push_list_calls = []
        mgr.cfg_mgr.push_list = lambda cmds: push_list_calls.append(cmds) or True
        mgr.directory.put(CONFIG_DB_NAME, BGP_BBR_TABLE_NAME, BGP_BBR_STATUS_KEY, BGP_BBR_STATUS_DISABLED)

        assert len(push_list_calls) == 1
        assert "no aggregate-address 10.100.0.0/16 as-set" in push_list_calls[0]
