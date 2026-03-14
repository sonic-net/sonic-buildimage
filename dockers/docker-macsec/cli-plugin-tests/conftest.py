import pytest
import mock_tables # lgtm [py/unused-import]
import mock_single_asic # lgtm[py/unused-import]
from unittest import mock


@pytest.fixture()
def mock_cfgdb():
    cfgdb = mock.Mock()
    CONFIG = {
        'PORT': {
            'Ethernet0': {
                "admin_status": "up"
            }
        }
    }

    def get_entry(table, key):
        if table not in CONFIG or key not in CONFIG[table]:
            return {}
        return CONFIG[table][key]

    def set_entry(table, key, data):
        CONFIG.setdefault(table, {})
        CONFIG[table].setdefault(key, {})
        CONFIG[table][key] = data

    def get_keys(table):
        return CONFIG[table].keys()

    cfgdb.get_entry = mock.Mock(side_effect=get_entry)
    cfgdb.set_entry = mock.Mock(side_effect=set_entry)
    cfgdb.get_keys = mock.Mock(side_effect=get_keys)

    yield cfgdb
