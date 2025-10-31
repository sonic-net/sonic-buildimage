import pytest
import mock_tables # lgtm [py/unused-import]
from unittest import mock

@pytest.fixture()
def mock_cfgdb():
    cfgdb = mock.Mock()
    CONFIG = {
        'VLAN': {
            'Vlan1000': {
                'dhcp_servers': ['192.0.0.1']
            },
            'Vlan200': {},
        },
        'DHCPV4_RELAY': {
            'Vlan1000': {
                'dhcpv4_servers': ['192.0.0.1']
            },
            'Vlan200': {},
        },
        'DHCP_RELAY': {
            'Vlan1000': {
                'dhcpv6_servers': ['fc02:2000::1']
            }
        },
        'DEVICE_METADATA': {
            'localhost': {
                'has_sonic_dhcpv4_relay' : "False"
            }
        },
        'VRF': {
            "default": {
                'VRF': 'default'
            },
            "VrfRED": {
                'VRF': 'VrfRED'
            },
            "VrfBLUE": {
                'VRF': 'VrfBLUE'
            }
        },
        'PORTCHANNEL_INTERFACE': {
            "PortChannel5": {},
            "PortChannel6": {},
            "PortChannel5|192.168.0.1/31": {},
            "PortChannel6|192.168.0.3/31": {}
        },
        'LOOPBACK_INTERFACE': {
            "Loopback0": {},
            "Loopback2": {},
            "Loopback3": {},
            "Loopback0|10.1.0.1/32": {},
            "Loopback2|10.1.0.1/32": {},
            "Loopback3|10.1.0.2/32": {}
        },
        'INTERFACE': {
            "Ethernet0": {},
            "Ethernet0|10.0.0.0/31": {},
            "Ethernet8": {},
            "Ethernet8|10.0.0.4/31": {},
            "Ethernet12": {},
            "Ethernet12|10.0.0.6/31": {},
        }
    }

    def get_entry(table, key):
        if table not in CONFIG or key not in CONFIG[table]:
            return {}
        return CONFIG[table][key]

    def set_entry(table, key, data):
        CONFIG[table].setdefault(key, {})

        if data is None:
            CONFIG[table].pop(key)
        else:
            CONFIG[table][key] = data

    def get_keys(table):
        return CONFIG[table].keys()

    def get_table(table):
        return CONFIG.get(table, {})

    cfgdb.get_table = mock.Mock(side_effect=get_table)
    cfgdb.get_entry = mock.Mock(side_effect=get_entry)
    cfgdb.set_entry = mock.Mock(side_effect=set_entry)
    cfgdb.get_keys = mock.Mock(side_effect=get_keys)

    yield cfgdb
