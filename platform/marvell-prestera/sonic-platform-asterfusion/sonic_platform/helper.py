####################################################################
# Asterfusion CX-N Devices Chassis Helper API                      #
#                                                                  #
# Module contains an implementation of SONiC Platform Base API and #
# provides the helper api                                          #
#                                                                  #
####################################################################

try:
    import copy

    from pathlib import Path

    from .constants import *
    from .logger import Logger

    from sonic_py_common.device_info import get_hwsku
    from sonic_py_common.device_info import get_platform
    from sonic_py_common.device_info import get_path_to_hwsku_dir
    from sonic_py_common.device_info import get_sonic_version_info
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


@singleton
class Helper(object):
    """Platform-specific Helper class"""

    def __init__(self):
        # type: () -> None
        self._logger = Logger()
        # Retrieve asic type
        sonic_version_info = get_sonic_version_info()
        self._asic_type = sonic_version_info.get("asic_type", None)
        if self._asic_type is None:
            self._logger.log_fatal("Failed in retrieving platform")
            raise RuntimeError("failed in retrieving asic type")
        self._logger.log_debug("Retrieved asic type <{}>".format(self._asic_type))
        # Retrieve platform
        self._platform = get_platform()
        self._logger.log_debug("Retrieved platform <{}>".format(self._platform))
        # Retrieve hwsku
        self._hwsku = get_hwsku()
        self._logger.log_debug("Retrieved hwsku <{}>".format(self._hwsku))
        # Retrieve asic (or hwsku if with various port)
        hwsku_linkpath = Path(get_path_to_hwsku_dir())
        if not hwsku_linkpath.is_symlink():
            self._logger.log_fatal("Failed in retrieving asic")
            raise RuntimeError("failed in retrieving asic")
        hwsku_dirpath = hwsku_linkpath.readlink()
        hwsku, *various_port, self._asic = hwsku_dirpath.parent.name.split("_")
        if len(various_port) > 0:
            self._hwsku = "_".join((hwsku, *various_port))
        self._logger.log_debug("Retrieved asic <{}>".format(self._asic))

    def get_asic_type(self):
        # type: () -> str
        return self._asic_type

    def get_platform(self):
        # type: () -> str
        return self._platform

    def get_hwsku(self):
        # type: () -> str
        return self._hwsku

    def get_asic(self):
        # type: () -> str
        return self._asic

    def get_peripheral_num(self):
        # type: () -> tuple[int, int, int, int, int, int, int]
        # Retrieve component info
        component_info = COMPONENT_INFO.get(self._hwsku, {}).get(self._asic, None)
        if component_info is None:
            self._logger.log_fatal("Failed in retrieving component num")
            raise RuntimeError("failed in retrieving component num")
        # Retrieve fan drawer info
        fan_drawer_info = FAN_DRAWER_INFO.get(self._hwsku, {}).get(self._asic, None)
        if fan_drawer_info is None:
            self._logger.log_fatal("Failed in retrieving fan drawer num")
            raise RuntimeError("failed in fan retrieving drawer num")
        # Retrieve fan info
        fan_info = FAN_INFO.get(self._hwsku, {}).get(self._asic, None)
        if fan_info is None:
            self._logger.log_fatal("Failed in retrieving fan num")
            raise RuntimeError("failed in retrieving fan num")
        # Retrieve psu info
        psu_info = PSU_INFO.get(self._hwsku, {}).get(self._asic, None)
        if psu_info is None:
            self._logger.log_fatal("Failed in retrieving psu num")
            raise RuntimeError("failed in retrieving psu num")
        # Retrieve thermal info
        thermal_info = THERMAL_INFO.get(self._hwsku, {}).get(self._asic, None)
        if thermal_info is None:
            self._logger.log_fatal("Failed in retrieving thermal num")
            raise RuntimeError("failed in retrieving thermal num")
        THERMAL_INFO[self._hwsku][self._asic] = (
            self.get_platform_thermal() + thermal_info
        )  # For X86 platform sensor drivers
        thermal_info = THERMAL_INFO.get(self._hwsku, {}).get(self._asic, None)
        # Retrieve voltage sensor info
        voltage_sensor_info = VOLTAGE_INFO.get(self._hwsku, {}).get(self._asic, None)
        if voltage_sensor_info is None:
            self._logger.log_fatal("Failed in retrieving voltage sensor num")
            raise RuntimeError("failed in retrieving voltage sensor num")
        # Retrieve sfp info
        sfp_info = SFP_INFO.get(self._hwsku, {}).get(self._asic, None)
        if sfp_info is None:
            self._logger.log_fatal("Failed in retrieving sfp num")
            raise RuntimeError("failed in retrieving sfp num")
        return (
            len(component_info),
            len(fan_drawer_info),
            len(fan_info),
            len(psu_info),
            len(thermal_info),
            len(voltage_sensor_info),
            len(sfp_info),
        )

    def get_platform_thermal(self):
        # type: () -> list[dict[str, float|str|dict[str, float|str]]]
        thermals = (
            {}
        )  # type: dict[tuple[str, str], dict[str, float|str|dict[str, float|str]]]
        for subdir in Path(SYSFS_HWMON_SUBSYS_DIR).glob("*"):
            for file in subdir.glob("*"):
                if file.name.startswith("temp") and file.is_file():
                    key = (subdir.name, file.name.split("_")[0].removeprefix("temp"))
                    thermal_info = thermals.setdefault(
                        key, copy.deepcopy(THERMAL_INFO_TEMPLATE)
                    )
                    if file.name.endswith("_label"):
                        thermal_info["name"] = file.read_text().strip().title()
                    elif file.name.endswith("_input"):
                        thermal_info["temp"] = {
                            "type": SYSFS_MATCH_TYPE_EXACT,
                            "path": file.as_posix(),
                        }
                        thermal_info["status"] = {
                            "type": SYSFS_MATCH_TYPE_EXACT,
                            "path": file.as_posix(),
                            "revcmp": "0",
                        }
                    elif file.name.endswith("_max"):
                        thermal_info["high"] = {
                            "type": SYSFS_MATCH_TYPE_EXACT,
                            "path": file.as_posix(),
                        }
                    elif file.name.endswith("_crit"):
                        thermal_info["chigh"] = {
                            "type": SYSFS_MATCH_TYPE_EXACT,
                            "path": file.as_posix(),
                        }
                    else:
                        self._logger.log_debug(
                            "Skipping unused sysfs at path <{}>".format(file.as_posix())
                        )
        valid_thermals = sorted(
            filter(
                lambda thermal_info: thermal_info.get("name") != NOT_AVAILABLE,
                thermals.values(),
            ),
            key=lambda thermal_info: thermal_info.get("name"),
        )
        self._logger.log_debug(
            "Found extra platform thermal(s): {}".format(
                ", ".join(
                    sorted(
                        map(
                            lambda thermal_info: thermal_info.get("name"),
                            valid_thermals,
                        )
                    )
                )
            )
        )
        return valid_thermals

    def get_sysfs_content(self, info, attr):
        # type: (dict[str, str]|dict[str, str|dict[str, str]]|dict[str, float|str|dict[str, str]]|dict[str, float|str|dict[str, float|str]]|dict[str, str|dict[str, str|tuple[str, str]]], str) -> str|bool
        attr_name = info.get("name", NOT_AVAILABLE)
        attr_value = NOT_AVAILABLE
        attr_info = info.get(attr, None)
        if attr_info is not None:
            attr_type = attr_info.get("type", NOT_AVAILABLE)
            if attr_type == SYSFS_MATCH_TYPE_VALUE:
                attr_value = attr_info.get("value", NOT_AVAILABLE)
            elif attr_type == SYSFS_MATCH_TYPE_EXACT:
                attr_path = Path(attr_info.get("path", NOT_AVAILABLE))
                if attr_path.exists():
                    attr_value = attr_path.read_text().strip()
                else:
                    self._logger.log_error(
                        "Failed in retrieving <{}> for <{}> due to exact path missing".format(
                            attr, attr_name
                        )
                    )
            elif attr_type == SYSFS_MATCH_TYPE_FUZZY:
                attr_dir = attr_info.get("dir")
                attr_file = attr_info.get("file")
                for subdir in Path(attr_dir).glob("*"):
                    attr_path = Path(attr_dir, subdir, attr_file)
                    if not attr_path.exists():
                        continue
                    attr_value = attr_path.read_text().strip()
                    break
                else:
                    self._logger.log_error(
                        "Failed in retrieving <{}> for <{}> due to fuzzy path missing".format(
                            attr, attr_name
                        )
                    )
            else:
                self._logger.log_error(
                    "Failed in retrieving <{}> for <{}> due to unsupported <{}> type <{}>".format(
                        attr, attr_name, attr, attr_type
                    )
                )
            attr_key = attr_info.get("key", None)
            if attr_key is not None:
                attr_values = filter(
                    lambda line: attr_key in line,
                    attr_value.splitlines(),
                )
                attr_delim = attr_info.get("delim", None)
                attr_trail = attr_info.get("trail", None)
                if attr_delim is not None:
                    attr_values = map(
                        lambda line: (
                            line.split(attr_delim)[1].strip()
                            if attr_delim in line
                            else NOT_AVAILABLE
                        ),
                        attr_values,
                    )
                if attr_trail is not None:
                    attr_values = map(
                        lambda line: line.removesuffix(attr_trail).strip(),
                        attr_values,
                    )
                attr_values = list(attr_values)
                if len(attr_values) == 1:
                    attr_value = attr_values[0].strip()
                else:
                    self._logger.log_error(
                        "Failed in retrieving <{}> for <{}> due to keyword match failure".format(
                            attr, attr_name
                        )
                    )
            attr_cmp = attr_info.get("cmp", None)
            attr_revcmp = attr_info.get("revcmp", None)
            compare_result = True
            if attr_cmp is not None:
                compare_result = attr_value == attr_cmp
            if attr_revcmp is not None:
                compare_result = compare_result and (attr_value != attr_cmp)
            if attr_cmp is not None or attr_revcmp is not None:
                attr_value = compare_result
                attr_choices = attr_info.get("choices", None)
                if attr_choices is not None:
                    attr_value = attr_choices[attr_value]
        self._logger.log_debug(
            "Retrieved <{}> <{}> for <{}>".format(attr, attr_value, attr_name)
        )
        return attr_value

    def trigger_via_sysfs(self, info, attr, status):
        # type: (dict[str, str]|dict[str, str|dict[str, str]]|dict[str, float|str|dict[str, str]]|dict[str, float|str|dict[str, float|str]]|dict[str, str|dict[str, str|tuple[str, str]]], str, bool) -> bool
        attr_name = info.get("name", NOT_AVAILABLE)
        attr_info = info.get(attr, None)
        if attr_info is None:
            return False
        attr_op = "on" if status else "off"
        attr_val = attr_info.get(attr_op, NOT_AVAILABLE)
        attr_attr = attr_info.get("attr", NOT_AVAILABLE)
        if attr_attr not in ("rw", "wo"):
            self._logger.log_error(
                "Failed in trigger <{}> of <{}> for <{}> due to file not writable".format(
                    attr_op, attr, attr_name
                )
            )
            return False
        if attr_val == NOT_AVAILABLE:
            self._logger.log_error(
                "Failed in trigger <{}> of <{}> for <{}> due to operation unsupported".format(
                    attr_op, attr, attr_name
                )
            )
            return False
        attr_type = attr_info.get("type", NOT_AVAILABLE)
        if attr_type == SYSFS_MATCH_TYPE_EXACT:
            attr_path = Path(attr_info.get("path", NOT_AVAILABLE))
            if not attr_path.exists():
                self._logger.log_error(
                    "Failed in trigger <{}> of <{}> for <{}> due to exact path missing".format(
                        attr_op, attr, attr_name
                    )
                )
            try:
                attr_path.write_text(attr_val)
                return True
            except Exception as err:
                self._logger.log_error(
                    "Failed in trigger <{}> of <{}> for <{}> due to <{}>".format(
                        attr_op, attr, attr_name, err
                    )
                )
        elif attr_type == SYSFS_MATCH_TYPE_FUZZY:
            attr_dir = attr_info.get("dir")
            attr_file = attr_info.get("file")
            for subdir in Path(attr_dir).glob("*"):
                attr_path = Path(attr_dir, subdir, attr_file)
                if not attr_path.exists():
                    continue
                try:
                    attr_path.write_text(attr_val)
                    return True
                except Exception as err:
                    self._logger.log_error(
                        "Failed in trigger <{}> of <{}> for <{}> due to <{}>".format(
                            attr_op, attr, attr_name, err
                        )
                    )
            else:
                self._logger.log_error(
                    "Failed in trigger <{}> of <{}> for <{}> due to fuzzy path missing".format(
                        attr_op, attr, attr_name
                    )
                )
        self._logger.log_error(
            "Failed in trigger <{}> of <{}> for <{}> due to unsupported type <{}>".format(
                attr_op, attr, attr_name, attr_type
            )
        )
        return False
