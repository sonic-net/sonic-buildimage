import json
import os
import runpy
import sys
from unittest import mock

import pytest

from sonic_py_common import device_info
from gearbox_sensors_config import get_gearbox_sensors_config


@pytest.fixture
def platform_path(tmp_path):
    platform_json = {
        "gearbox_sensors": {
            "poll_interval": "60",
            "poll_admin_status": "enable"
        }
    }
    (tmp_path / "platform.json").write_text(json.dumps(platform_json))
    return str(tmp_path)


@pytest.mark.parametrize("platform_json", [None, {}, {"asic_sensors": {}}])
def test_platform_without_gearbox_sensors(monkeypatch, platform_json):
    monkeypatch.setattr(device_info, "get_platform_json_data", lambda: platform_json)
    assert get_gearbox_sensors_config() == {}


def test_platform_directory_missing():
    with mock.patch.object(device_info, "get_platform_json_data", side_effect=OSError):
        assert get_gearbox_sensors_config() == {}


@pytest.mark.parametrize("interval, admin_status", [("60", "enable"), ("120", "disable")])
def test_gearbox_polling_settings(monkeypatch, interval, admin_status):
    monkeypatch.setattr(device_info, "get_platform_json_data", lambda: {
        "gearbox_sensors": {
            "poll_interval": interval,
            "poll_admin_status": admin_status
        }
    })
    assert get_gearbox_sensors_config() == {
        "GEARBOX_SENSORS": {
            "GEARBOX_SENSORS_POLLER_INTERVAL": {"interval": interval},
            "GEARBOX_SENSORS_POLLER_STATUS": {"admin_status": admin_status}
        }
    }


@pytest.mark.parametrize("multi_asic, namespace, platform_info, expected", [
    (False, None, True, True),
    (True, None, True, False),
    (True, "asic0", True, True),
    (True, "asic1", True, True),
    (False, None, False, False),
    (True, "asic0", False, False)
])
def test_cfggen_gearbox_sensors(monkeypatch, capsys, platform_path,
                               multi_asic, namespace, platform_info, expected):
    monkeypatch.delenv("CFGGEN_UNIT_TESTING", raising=False)
    monkeypatch.delenv("NAMESPACE_ID", raising=False)
    monkeypatch.setattr(device_info, "get_platform", lambda: "x86_64-test-platform-r0")
    monkeypatch.setattr(device_info, "get_path_to_platform_dir", lambda: platform_path)
    monkeypatch.setattr(device_info, "get_system_mac", lambda **kwargs: "00:11:22:33:44:55")
    cfggen_file = os.path.join(os.path.dirname(__file__), "../sonic-cfggen")
    main = runpy.run_path(cfggen_file)["main"]
    asic_config = {"ASIC_SENSORS": {
        "ASIC_SENSORS_POLLER_INTERVAL": {"interval": "10"},
        "ASIC_SENSORS_POLLER_STATUS": {"admin_status": "enable"}
    }}
    arguments = [cfggen_file, "--print-data"]
    if platform_info:
        arguments.append("-H")
    if namespace:
        arguments.extend(["-n", namespace])
    monkeypatch.setattr(sys, "argv", arguments)
    with mock.patch.dict(main.__globals__, {
        "is_multi_asic": lambda: multi_asic,
        "get_asic_id_from_name": lambda name: int(name[4:]),
        "get_asic_device_id": lambda asic_id: str(asic_id),
        "get_asic_sub_role": lambda asic_id: "FrontEnd",
        "get_asic_sensors_config": lambda: asic_config
    }):
        main()
    config = json.loads(capsys.readouterr().out)
    if expected:
        assert config["GEARBOX_SENSORS"] == {
            "GEARBOX_SENSORS_POLLER_INTERVAL": {"interval": "60"},
            "GEARBOX_SENSORS_POLLER_STATUS": {"admin_status": "enable"}
        }
        assert config["ASIC_SENSORS"] == asic_config["ASIC_SENSORS"]
    else:
        assert "GEARBOX_SENSORS" not in config
        assert "ASIC_SENSORS" not in config
