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

    cfgdb.get_entry = mock.Mock(side_effect=get_entry)
    cfgdb.set_entry = mock.Mock(side_effect=set_entry)
    cfgdb.get_keys = mock.Mock(side_effect=get_keys)

    yield cfgdb
