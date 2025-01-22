import os
import sys
from unittest import mock

from swsscommon import swsscommon
swsscommon.RestartWaiter = mock.MagicMock()

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
sys.path.insert(0, modules_path)

from containercfgd import containercfgd


@containercfgd.config_handler('MockTable')
class MockHandler:
    def handle_init_data(self, init_data):
        pass

    def handle_config(self, table, key, data):
        pass


def test_handler_register():
    assert 'MockTable' in containercfgd.ContainerConfigDaemon.handlers
    assert isinstance(containercfgd.ContainerConfigDaemon.handlers['MockTable'], MockHandler)


def test_init_data_handler():
    mock_handler_cls = mock.MagicMock()
    mock_handler_instance = mock.MagicMock()
    mock_handler_instance.handle_init_data = mock.MagicMock()
    mock_handler_cls.return_value = mock_handler_instance

    containercfgd.ContainerConfigDaemon.register_handler('LoadMock', mock_handler_cls)
    daemon = containercfgd.ContainerConfigDaemon()
    daemon.init_data_handler({})
    mock_handler_instance.handle_init_data.assert_called_once()
    containercfgd.ContainerConfigDaemon.handlers.pop('LoadMock')


@mock.patch('containercfgd.containercfgd.ConfigDBConnector')
def test_run(mock_connector):
    mock_db = mock.MagicMock()
    mock_db.connect = mock.MagicMock()
    mock_db.subscribe = mock.MagicMock()
    mock_db.listen = mock.MagicMock()
    mock_connector.return_value = mock_db

    daemon = containercfgd.ContainerConfigDaemon()
    daemon.run()
    mock_db.connect.assert_called_once()
    expected = []
    for table_name, handler in containercfgd.ContainerConfigDaemon.handlers.items():
        expected.append(mock.call(table_name, handler.handle_config))
    mock_db.subscribe.assert_has_calls(expected, any_order=True)
    mock_db.listen.assert_called_once()
