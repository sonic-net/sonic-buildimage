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
            }
        },
        'DHCP_RELAY': {
            'Vlan1000': {
                'dhcpv6_servers': ['fc02:2000::1']
            }
        }
    }

    def get_entry(table, key):
        return CONFIG[table][key]

    def set_entry(table, key, data):
        CONFIG[table].setdefault(key, {})
        CONFIG[table][key] = data

    cfgdb.get_entry = mock.Mock(side_effect=get_entry)
    cfgdb.set_entry = mock.Mock(side_effect=set_entry)

    yield cfgdb

