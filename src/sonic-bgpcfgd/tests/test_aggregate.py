from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_aggregate_address import AggregateAddressMgr, BGP_AGGREGATE_ADDRESS_TABLE_NAME, BGP_BBR_TABLE_NAME
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
        return self.addresses.get(key)

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
        set_del_test.push_list_called = True
        assert cmds == expected_cmds
        return True
    mgr.cfg_mgr.push_list = push_list

    if op == "SET":
        mgr.set_handler(*args)
    elif op == "DEL":
        mgr.del_handler(*args)
    else:
        assert False, "Operation is not supported"

    if expected_cmds:
        assert set_del_test.push_list_called, "cfg_mgr.push_list wasn't called"
    else:
        assert not set_del_test.push_list_called, "cfg_mgr.push_list was called"


@pytest.mark.parametrize("aggregate_prefix", ["192.168.1.1", "2ff::1/64"])
@pytest.mark.parametrize("bbr_status", [BGP_BBR_STATUS_ENABLED, BGP_BBR_STATUS_DISABLED])
@pytest.mark.parametrize("bbr_required", ["true", "false"])
@pytest.mark.parametrize("summary_only", ["true", "false"])
@pytest.mark.parametrize("as_set", ["true", "false"])
@pytest.mark.parametrize("aggregate_address_prefix_list", ["", "aggregate_PL"])
@pytest.mark.parametrize("contributing_address_prefix_list", ["", "contributing_PL"])
def test_set_with_parameters_combination(
    aggregate_prefix,
    bbr_status,
    bbr_required,
    summary_only,
    as_set,
    aggregate_address_prefix_list,
    contributing_address_prefix_list
):
    mgr = constructor(bbr_status=bbr_status)
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
        'bbr-required': bbr_required
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
    data = mgr.address_table.get(aggregate_prefix)
    assert data == expected_state
