import importlib.machinery
import importlib.util
from pathlib import Path
from unittest.mock import MagicMock, patch


SCRIPT = Path(__file__).resolve().parents[1] / "container_checker"
loader = importlib.machinery.SourceFileLoader("container_checker", str(SCRIPT))
spec = importlib.util.spec_from_loader(loader.name, loader)
assert spec is not None
container_checker = importlib.util.module_from_spec(spec)
loader.exec_module(container_checker)


@patch.object(container_checker.device_info, "is_smartswitch", return_value=False)
@patch.object(container_checker.device_info, "is_disaggregated_chassis", return_value=False)
@patch.object(container_checker.device_info, "is_supervisor", return_value=False)
@patch.object(container_checker.multi_asic, "is_multi_asic", return_value=False)
@patch.object(container_checker.multi_asic, "get_asic_presence_list", return_value=[])
@patch.object(container_checker.swsscommon, "ConfigDBConnector")
def test_host_service_is_not_expected_as_container(
    mock_config_db,
    _mock_presence,
    _mock_multi_asic,
    _mock_supervisor,
    _mock_disaggregated,
    _mock_smartswitch,
):
    connector = MagicMock()
    connector.get_table.return_value = {
        "dldd": {"state": "enabled", "runtime_type": "host"},
        "snmp": {"state": "enabled"},
    }
    mock_config_db.return_value = connector

    expected, always = container_checker.get_expected_running_containers()

    assert expected == {"snmp"}
    assert always == set()
