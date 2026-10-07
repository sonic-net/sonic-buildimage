import importlib.util
import sys
import types
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).parents[1] / "dhcp_dos_logger.py"


class FakeLogger:
    def __init__(self, *_args, **_kwargs):
        pass

    def __getattr__(self, _name):
        return lambda *_args, **_kwargs: None


class FakeSelect:
    TIMEOUT = 0
    OBJECT = 1

    def addSelectable(self, _selectable):
        pass

    def select(self, _timeout):
        return self.OBJECT, None


def load_logger(is_multi_asic=False):
    port_tables = {
        None: {"Ethernet0": {}},
        "asic0": {"Ethernet0": {}},
        "asic1": {"Ethernet4": {}},
    }
    config_dbs = {}

    def create_config_db(*, use_unix_socket_path, namespace=None):
        config_db = mock.Mock()
        config_db.get_table.return_value = port_tables[namespace]
        config_dbs[namespace] = config_db
        return config_db

    config_db_connector = mock.Mock(side_effect=create_config_db)

    appl_db_connector = mock.Mock(return_value=mock.Mock())
    subscriber = mock.Mock()
    subscriber.pop.side_effect = [("PortInitDone", "", [])]

    swsscommon_module = types.ModuleType("swsscommon.swsscommon")
    swsscommon_module.ConfigDBConnector = config_db_connector
    sonic_db_config = mock.Mock()
    swsscommon_module.SonicDBConfig = sonic_db_config
    swsscommon_module.DBConnector = appl_db_connector
    swsscommon_module.Select = FakeSelect
    swsscommon_module.SubscriberStateTable = mock.Mock(return_value=subscriber)
    swsscommon_module.APP_PORT_TABLE_NAME = "PORT_TABLE"

    swsscommon_package = types.ModuleType("swsscommon")
    swsscommon_package.__path__ = []
    swsscommon_package.swsscommon = swsscommon_module

    logger_module = types.ModuleType("sonic_py_common.logger")
    logger_module.Logger = FakeLogger

    multi_asic_module = types.ModuleType("sonic_py_common.multi_asic")
    multi_asic_module.is_multi_asic = mock.Mock(return_value=is_multi_asic)
    multi_asic_module.get_namespace_list = mock.Mock(
        return_value=["asic0", "asic1"]
    )

    sonic_py_common = types.ModuleType("sonic_py_common")
    sonic_py_common.__path__ = []
    sonic_py_common.multi_asic = multi_asic_module

    fake_modules = {
        "sonic_py_common": sonic_py_common,
        "sonic_py_common.logger": logger_module,
        "sonic_py_common.multi_asic": multi_asic_module,
        "swsscommon": swsscommon_package,
        "swsscommon.swsscommon": swsscommon_module,
    }

    module_name = "dhcp_dos_logger_under_test"
    spec = importlib.util.spec_from_file_location(module_name, SCRIPT)
    module = importlib.util.module_from_spec(spec)
    with mock.patch.dict(sys.modules, fake_modules):
        spec.loader.exec_module(module)

    return (
        module,
        config_db_connector,
        appl_db_connector,
        sonic_db_config,
        config_dbs,
    )


def test_single_asic_redis_connections_use_unix_sockets():
    module, config_db_connector, appl_db_connector, _, _ = load_logger()

    config_db_connector.assert_called_once_with(use_unix_socket_path=True)

    module.wait_for_port_init_done()

    appl_db_connector.assert_called_once_with("APPL_DB", 0, False)


def test_multi_asic_config_db_connections_use_namespace_unix_sockets():
    module, config_db_connector, _, sonic_db_config, config_dbs = load_logger(
        is_multi_asic=True
    )

    sonic_db_config.initializeGlobalConfig.assert_called_once_with()
    assert config_db_connector.call_args_list == [
        mock.call(use_unix_socket_path=True, namespace="asic0"),
        mock.call(use_unix_socket_path=True, namespace="asic1"),
    ]

    for namespace in ("asic0", "asic1"):
        config_dbs[namespace].connect.assert_called_once_with()
        config_dbs[namespace].get_table.assert_called_once_with("PORT")

    assert module.ports_table == {"Ethernet0": {}, "Ethernet4": {}}
    assert module.drop_pkts == {"Ethernet0": 0, "Ethernet4": 0}
