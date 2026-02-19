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

DPU_TABLE = 'DPU'
DPUS_TABLE = 'DPUS'


def get_smartswitch_config(hwsku=None):
    config = {}

    if os.environ.get("CFGGEN_UNIT_TESTING") == "2":
        if hwsku == 'SSwitch-32x1000Gb':
            json_file =  os.path.join(tests_path, "data", "smartswitch", "sample_switch_platform.json")
        elif hwsku == 'SS-DPU-1x400Gb':
            json_file =  os.path.join(tests_path, "data", "smartswitch", "sample_dpu_platform.json")
        else:
            return config
    else:
        try:
            platform_path = device_info.get_path_to_platform_dir()
        except OSError:
            return config
        json_file = os.path.join(platform_path, device_info.PLATFORM_JSON_FILE)

    platform_json = portconfig.readJson(json_file)
    if not platform_json:
        return config

    if DPU_TABLE in platform_json:
        config[DPU_TABLE] = platform_json[DPU_TABLE]
    if DPUS_TABLE in platform_json:
        config[DPUS_TABLE] = platform_json[DPUS_TABLE]

    return config
