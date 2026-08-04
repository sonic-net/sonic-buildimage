import os
import sys
import portconfig
from sonic_py_common import device_info

try:
    if os.environ["CFGGEN_UNIT_TESTING"] == "2":
        modules_path = os.path.join(os.path.dirname(__file__), ".")
        tests_path = os.path.join(modules_path, "tests")
        sys.path.insert(0, modules_path)
        sys.path.insert(0, tests_path)
        import mock_tables.dbconnector
        mock_tables.dbconnector.load_namespace_config()

except KeyError:
    pass

ASIC_SENSORS_KEY = "asic_sensors"


def get_asic_sensors_config():
    config = {}
    if os.environ.get("CFGGEN_UNIT_TESTING") == "2":
        json_file =  os.path.join(tests_path, "data",  "asic_sensors", "platform.json")
    else:
        try:
            platform_path = device_info.get_path_to_platform_dir()
        except OSError:
            return config
        json_file = os.path.join(platform_path, device_info.PLATFORM_JSON_FILE)

    if not os.path.exists(json_file):
        return config
    
    platform_json = portconfig.readJson(json_file)
    if not platform_json:
        return config

    if ASIC_SENSORS_KEY in platform_json:
        config["ASIC_SENSORS"] = {"ASIC_SENSORS_POLLER_INTERVAL": {"interval": platform_json[ASIC_SENSORS_KEY]["poll_interval"]},
                                  "ASIC_SENSORS_POLLER_STATUS":  {"admin_status": platform_json[ASIC_SENSORS_KEY]["poll_admin_status"]}
                                  }
    return config
