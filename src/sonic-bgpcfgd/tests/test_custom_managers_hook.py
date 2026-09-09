from unittest.mock import MagicMock, patch

import pytest

from bgpcfgd.main import load_custom_managers


@patch("bgpcfgd.main.importlib.import_module")
def test_load_custom_managers_when_module_is_absent(mock_import_module):
    mock_import_module.side_effect = ModuleNotFoundError(
        name="bgpcfgd.managers_custom"
    )

    assert load_custom_managers(MagicMock()) == []


@patch("bgpcfgd.main.importlib.import_module")
def test_load_custom_managers_propagates_dependency_error(mock_import_module):
    mock_import_module.side_effect = ModuleNotFoundError(
        name="deployment_dependency"
    )

    with pytest.raises(ModuleNotFoundError):
        load_custom_managers(MagicMock())


@patch("bgpcfgd.main.importlib.import_module")
def test_load_custom_managers_returns_registered_managers(mock_import_module):
    common_objs = MagicMock()
    managers = [MagicMock(), MagicMock()]
    module = mock_import_module.return_value
    module.get_managers.return_value = managers

    assert load_custom_managers(common_objs) == managers
    mock_import_module.assert_called_once_with(
        ".managers_custom",
        "bgpcfgd",
    )
    module.get_managers.assert_called_once_with(common_objs)
