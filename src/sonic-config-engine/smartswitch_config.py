import os
import sys
import portconfig

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

DPUS_TABLE  = 'DPUS'


def get_smartswitch_config(hwsku=None, platform=None):
    hwsku_json_file = portconfig.get_hwsku_file_name(hwsku, platform)

    if os.environ.get("CFGGEN_UNIT_TESTING") == "2" and hwsku == 'SSwitch-32x1000Gb':
        hwsku_json_file = os.path.join(tests_path, "data", "smartswitch", hwsku, "hwsku.json")

    if not hwsku_json_file:
        return {}

    hwsku_dict = portconfig.readJson(hwsku_json_file)
    if not hwsku_dict:
        raise Exception("hwsku_dict is none")

    config = {}

    if DPUS_TABLE in hwsku_dict:
        config[DPUS_TABLE] = hwsku_dict[DPUS_TABLE]

    return config
