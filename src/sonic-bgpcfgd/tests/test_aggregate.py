from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_aggregate_address import AggregateAddressMgr, BGP_AGGREGATE_ADDRESS_TABLE_NAME, BGP_BBR_TABLE_NAME
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
        import pdb; pdb.set_trace() 
        set_del_test.push_list_called = True
        assert cmds in expected_cmds
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


def test_set_with_bbr_required():
    mgr = constructor(bbr_status=BGP_BBR_STATUS_DISABLED)
    prefix = '192.168.0.0/24'
    bbr_required = 'true'
    attr = (
        ('bbr-required', bbr_required),
        ('summary-only', 'false')
    )
    expected_state = {
        'aggregate-address-prefix-list': '',
        'contributing-address-prefix-list': '',
        'state': 'inactive',
        'bbr-required': bbr_required
    }
    set_del_test(
        mgr,
        "SET",
        (prefix, attr)
    )
    assert [prefix] == mgr.address_table.getKeys()
    data = mgr.address_table.get(prefix)
    assert data == expected_state


def test_set_without_bbr_required():
    mgr = constructor(bbr_status=BGP_BBR_STATUS_DISABLED)
    prefix = '192.168.0.1/24'
    bbr_required = 'false'
    attr = (
        ('bbr-required', bbr_required),
        ('summary-only', 'false')
    )
    expected_state = {
        'aggregate-address-prefix-list': '',
        'contributing-address-prefix-list': '',
        'state': 'active',
        'bbr-required': bbr_required
    }
    expected_cmds = [[
        'router bgp 65001',
        'address-family ipv4',
        'aggregate-address 192.168.0.1/24 ',
        'exit-address-family',
        'exit'
    ]]
    set_del_test(
        mgr,
        "SET",
        (prefix, attr),
        expected_cmds
    )
    assert [prefix] == mgr.address_table.getKeys()
    data = mgr.address_table.get(prefix)
    assert data == expected_state