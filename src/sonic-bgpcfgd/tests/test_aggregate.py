from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from bgpcfgd.managers_aggregate_address import AggregateAddressMgr, BGP_AGGREGATE_ADDRESS_TABLE_NAME, BGP_BBR_TABLE_NAME
from bgpcfgd.managers_aggregate_address import generate_prefix_list_commands, validate_prefix
from jinja2 import Environment, FileSystemLoader
import os
import pytest
from swsscommon import swsscommon
from unittest.mock import MagicMock, patch


CONFIG_DB_NAME = "CONFIG_DB"
BGP_BBR_TABLE_NAME = "BGP_BBR"
BGP_BBR_STATUS_KEY = "status"
BGP_BBR_STATUS_ENABLED = "enabled"
BGP_BBR_STATUS_DISABLED = "disabled"
TEMPLATE_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__),
    '../../../dockers/docker-fpm-frr/frr/bgpd'
))


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


@pytest.mark.parametrize("aggregate_prefix", ["192.168.1.0/24", "2ff::/64"])
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
    # Check if the entry is currently active; only active entries trigger FRR removal
    _, current_data = mgr.address_table.get(aggregate_prefix)
    is_active = current_data and current_data.get('state') == 'active'

    except_cmds = [
        'router bgp 65001',
        'address-family ' + ('ipv4' if '.' in aggregate_prefix else 'ipv6'),
        'no aggregate-address ' + aggregate_prefix,
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
        except_cmds if is_active else None
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
        'no aggregate-address ' + aggregate_prefix,
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


@pytest.mark.parametrize("prefix,expected", [
    ("10.100.0.0/16", True),
    ("10.100.1.0/24", True),
    ("192.168.0.0/24", True),
    ("2001:db8::/32", True),
    ("0.0.0.0/0", True),
    ("::/0", True),
    ("10.100.0.1/24", False),   # host bits set
    ("10.100.1.0/23", False),   # host bits set
    ("192.168.1.1/24", False),  # host bits set
    ("2001:db8::1/32", False),  # host bits set
])
def test_validate_prefix(prefix, expected):
    net, reason = validate_prefix(prefix)
    if expected:
        assert net is not None
        assert reason is None
    else:
        assert net is None
        assert reason is not None


@pytest.mark.parametrize("bad_prefix", [
    "10.100.0.1/24",
    "10.100.1.0/23",
])
def test_host_bits_set_rejected(bad_prefix):
    """address_set_handler must return False for prefixes with host bits set."""
    mgr = constructor(bbr_status=BGP_BBR_STATUS_ENABLED)
    attr = (
        ('bbr-required', 'false'),
        ('summary-only', 'false'),
        ('as-set', 'false'),
        ('aggregate-address-prefix-list', ''),
        ('contributing-address-prefix-list', ''),
    )
    # push_list must NOT be called
    set_del_test(mgr, "SET", (bad_prefix, attr), None)
    # State should be inactive
    _, data = mgr.address_table.get(bad_prefix)
    assert data["state"] == "inactive"


@pytest.mark.parametrize("bad_prefix", [
    "10.100.0.1/24",
    "10.100.1.0/23",
])
def test_inactive_entry_skips_frr_removal(bad_prefix):
    """del_handler must skip FRR removal for inactive entries and clean up STATE_DB."""
    mgr = constructor(bbr_status=BGP_BBR_STATUS_ENABLED)
    attr = (
        ('bbr-required', 'false'),
        ('summary-only', 'false'),
        ('as-set', 'false'),
        ('aggregate-address-prefix-list', ''),
        ('contributing-address-prefix-list', ''),
    )
    # Set invalid prefix -> state = inactive
    set_del_test(mgr, "SET", (bad_prefix, attr), None)
    assert mgr.address_table.get(bad_prefix)[1]["state"] == "inactive"

    # Del should NOT push any commands to FRR
    set_del_test(mgr, "DEL", (bad_prefix,), None)

    # STATE_DB entry should be cleaned up
    assert bad_prefix not in mgr.address_table.getKeys()


def _is_ipv4(value):
    try:
        return ipaddress.ip_network(value, strict=False).version == 4
    except ValueError:
        return False


def _is_ipv6(value):
    try:
        return ipaddress.ip_network(value, strict=False).version == 6
    except ValueError:
        return False


def _ip_network(value):
    try:
        return ipaddress.ip_network(value, strict=False).network_address
    except ValueError:
        return ''


def _render_bootstrap_aggregate_config():
    env = Environment(loader=FileSystemLoader(TEMPLATE_PATH))
    env.filters['ipv4'] = _is_ipv4
    env.filters['ipv6'] = _is_ipv6
    env.filters['ip_network'] = _ip_network
    template = env.get_template('bgpd.aggregate.conf.j2')

    return template.render(
        DEVICE_METADATA={'localhost': {'bgp_asn': '65000'}},
        constants={
            'bgp': {
                'bbr': {'enabled': True, 'default_state': 'enabled'},
                'peers': {'general': {'bbr': {'TIER0_V4': ['ipv4']}}},
            },
        },
        BGP_BBR={'all': {'status': 'enabled'}},
        BGP_AGGREGATE_ADDRESS={
            '10.0.0.0/24': {
                'bbr-required': 'true',
                'summary-only': 'true',
                'as-set': 'false',
                'aggregate-address-prefix-list': 'AGG',
                'contributing-address-prefix-list': 'CON',
            },
            '2001:db8::/64': {
                'bbr-required': 'true',
                'summary-only': 'true',
                'as-set': 'false',
                'aggregate-address-prefix-list': 'AGG_V6',
                'contributing-address-prefix-list': 'CON_V6',
            },
        },
    )


def _config_lines(output):
    return [
        line.strip()
        for line in output.splitlines()
        if line.strip() and not line.strip().startswith('!')
    ]


def _manager_prefix_list_commands(is_remove=False):
    commands = []
    commands.extend(generate_prefix_list_commands('AGG', '10.0.0.0/24', True, False, is_remove))
    commands.extend(generate_prefix_list_commands('CON', '10.0.0.0/24', True, True, is_remove))
    commands.extend(generate_prefix_list_commands('AGG_V6', '2001:db8::/64', False, False, is_remove))
    commands.extend(generate_prefix_list_commands('CON_V6', '2001:db8::/64', False, True, is_remove))
    return commands


def test_bootstrap_manager_reconciliation_and_delete_use_same_prefix_list_rules():
    bootstrap_lines = set(_config_lines(_render_bootstrap_aggregate_config()))
    manager_add_commands = _manager_prefix_list_commands(is_remove=False)
    manager_delete_commands = _manager_prefix_list_commands(is_remove=True)

    assert set(manager_add_commands).issubset(bootstrap_lines)
    assert all(' seq ' not in command for command in manager_add_commands)
    assert all(' seq ' not in command for command in manager_delete_commands)
    assert manager_delete_commands == ["no %s" % command for command in manager_add_commands]
