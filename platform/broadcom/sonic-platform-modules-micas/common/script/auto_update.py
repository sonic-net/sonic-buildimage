#!/usr/bin/env python3

try:
    import os
    import json
    import logging
    import sys
    from sonic_py_common import device_info
    from sonic_platform.platform import Platform
except ImportError as e:
    raise ImportError(str(e) + "- required module not found") from e

PLATFORM_COMPONENTS_FILE = "platform_components.json"
CHASSIS_KEY = "chassis"
COMPONENT_KEY = "component"
FIRMWARE_KEY = "firmware"
VERSION_KEY = "version"
chassis_component_map = {}
current_chassis_component_map = {}
current_chassis = Platform().get_chassis()


def parse_component_section(section, component):
    if not isinstance(component, dict):
        logging.error("dictionary is expected: key=%s", COMPONENT_KEY)
        return False

    if not component:
        return False

    missing_key = None
    chassis_component_map[section] = {}

    for key1, value1 in component.items():
        if not isinstance(value1, dict):
            logging.error("dictionary is expected: key=%s", key1)
            return False

        if value1:
            if len(value1) < 1 or len(value1) > 3:
                logging.error("unexpected number of records: key=%s", key1)
                return False

            if FIRMWARE_KEY not in value1:
                missing_key = FIRMWARE_KEY
                break

            for key2, value2 in value1.items():
                if not isinstance(value2, str):
                    logging.error("string is expected: key=%s", key2)
                    return False

            chassis_component_map[section][key1] = value1

    if missing_key is not None:
        logging.error("\"%s\" key hasn't been found", missing_key)
        return False

    return True


def parse_chassis_section(chassis):
    if not isinstance(chassis, dict):
        logging.error("dictionary is expected: key=%s", CHASSIS_KEY)
        return False

    if not chassis:
        logging.error("dictionary is empty: key=%s", CHASSIS_KEY)
        return False

    if len(chassis) != 1:
        logging.error("unexpected number of records: key=%s", CHASSIS_KEY)
        return False

    for key, value in chassis.items():
        if not isinstance(value, dict):
            logging.error("dictionary is expected: key=%s", key)
            return False

        if not value:
            logging.error("dictionary is empty: key=%s", key)
            return False

        if COMPONENT_KEY not in value:
            logging.error("\"%s\" key hasn't been found", COMPONENT_KEY)
            return False

        if len(value) != 1:
            logging.error("unexpected number of records: key=%s", key)
            return False

        return parse_component_section(key, value[COMPONENT_KEY])

    return False


def get_platform_components_path():
    PLATFORM_COMPONENTS_PATH_TEMPLATE = "/usr/share/sonic/device/{}/{}"
    PLATFORM_COMPONENTS_FILE_PATH = PLATFORM_COMPONENTS_PATH_TEMPLATE.format(
        device_info.get_platform(), PLATFORM_COMPONENTS_FILE)
    return PLATFORM_COMPONENTS_FILE_PATH


def parse_platform_components():
    platform_components_path = get_platform_components_path()
    with open(platform_components_path) as platform_components:
        data = json.load(platform_components)

        if not isinstance(data, dict):
            logging.error("dictionary is expected: key=root")
            return False

        if not data:
            logging.error("dictionary is empty: key=root")
            return False

        if CHASSIS_KEY not in data:
            logging.error("\"%s\" key hasn't been found", CHASSIS_KEY)
            return False

        return parse_chassis_section(data[CHASSIS_KEY])


def get_current_chassis_component_map():
    chassis_name = current_chassis.get_name()
    current_chassis_component_map[chassis_name] = {}

    component_list = current_chassis.get_all_components()
    for component in component_list:
        component_name = component.get_name()
        current_chassis_component_map[chassis_name][component_name] = component

    return current_chassis_component_map


def get_upgrade_dict():
    upgrade_dict = {}
    firmware_version_current = ""
    firmware_version_available = ""

    if not parse_platform_components():
        logging.error("Reading platform_components.json i, ion exception")
        sys.exit(1)

    if not get_current_chassis_component_map():
        logging.error("Reading firmware i, ion from the driver is abnormal")
        sys.exit(1)

    chassis_name = current_chassis.get_name()
    diff_keys = set(chassis_component_map.keys()) ^ set(current_chassis_component_map.keys())
    if diff_keys:
        logging.error("%s names mismatch: keys=%s", chassis_name, str(list(diff_keys)))
        return None

    for chassis_name, component_map in current_chassis_component_map.items():
        for component_name, component in component_map.items():
            firmware_version_current = component.get_firmware_version()
            if component_name in chassis_component_map[chassis_name]:
                firmware_version_available = chassis_component_map[chassis_name][component_name][VERSION_KEY]
            else:
                logging.warning("can't find %s in %s", component_name, PLATFORM_COMPONENTS_FILE)
                break

            if not os.path.exists(chassis_component_map[chassis_name][component_name][FIRMWARE_KEY]):
                logging.error("%s does not exist", chassis_component_map[chassis_name][component_name][FIRMWARE_KEY])
                break

            if firmware_version_available != firmware_version_current:
                upgrade_dict[component_name] = chassis_component_map[chassis_name][component_name][FIRMWARE_KEY]

    return upgrade_dict


def auto_upgrade():
    upgrade_result_dict = {}
    chassis_name = current_chassis.get_name()

    upgrade_dict = get_upgrade_dict()
    if not upgrade_dict:
        logging.info("No firmware found for automatic upgrade")
        return None

    component_map = current_chassis_component_map[chassis_name]
    for value, path in upgrade_dict.items():
        status = component_map[value].install_firmware(path)
        if status:
            upgrade_result_dict[value] = "success"
            logging.info("%s Upgrade Success", value)
        else:
            upgrade_result_dict[value] = "failed"
            logging.error("%s Upgrade Failed", value)
    return upgrade_result_dict


if __name__ == '__main__':
    auto_upgrade()
