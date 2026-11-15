#!/usr/bin/env python


from pathlib import Path
import re

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")



class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten


class CpuThermal(ThermalBase):
    """Read coretemp sensors without depending on hwmon enumeration order.

    CPU sensors are appended to the existing PDDF board inventory. Discovery
    and reads never write sysfs; missing or invalid values remain unavailable.
    """

    HWMON_ROOT = Path("/sys/devices/platform/coretemp.0/hwmon")

    def __init__(self, label, name, position, hwmon_root=None):
        self.label = label
        self.name = name
        self.position = position
        self.hwmon_root = (
            Path(hwmon_root) if hwmon_root is not None else self.HWMON_ROOT
        )

    @staticmethod
    def _text(path):
        try:
            return path.read_text().strip()
        except (OSError, UnicodeError):
            return None

    @classmethod
    def _channels(cls, root):
        try:
            for hwmon in sorted(root.glob("hwmon*")):
                if cls._text(hwmon / "name") != "coretemp":
                    continue
                for path in hwmon.glob("temp*_label"):
                    match = re.fullmatch(r"temp([0-9]+)_label", path.name)
                    label = cls._text(path)
                    if match and label:
                        yield int(match[1]), label, hwmon
        except OSError:
            # An optional CPU sensor must not prevent board inventory startup.
            return

    @classmethod
    def discover(cls, position_offset, hwmon_root=None):
        root = Path(hwmon_root) if hwmon_root is not None else cls.HWMON_ROOT
        sensors = []
        seen = set()
        for channel, label, _ in sorted(cls._channels(root), key=lambda item: item[0]):
            match = re.fullmatch(r"(Package id|Core) ([0-9]+)", label)
            if not match or label in seen:
                continue
            kind, number = match.groups()
            if kind == "Core":
                name = "CPU_Core_{}_temp".format(number)
            else:
                name = (
                    "CPU_Package_temp"
                    if number == "0"
                    else "CPU_Package_{}_temp".format(number)
                )
            sensors.append(cls(label, name, position_offset + len(sensors) + 1, root))
            seen.add(label)
        return sensors

    def _path(self, suffix):
        # Resolve again so a driver reload or changed hwmon number cannot leave
        # the reader attached to an old path or a different sensor.
        for channel, label, hwmon in self._channels(self.hwmon_root):
            if label == self.label:
                return hwmon / "temp{}_{}".format(channel, suffix)
        return None

    def _reading(self, suffix):
        path = self._path(suffix)
        text = self._text(path) if path is not None else None
        try:
            return int(text) / 1000.0
        except (TypeError, ValueError, OverflowError):
            return None

    def get_name(self):
        return self.name

    def get_temperature(self):
        return self._reading("input")

    def get_high_threshold(self):
        return self._reading("max")

    def get_high_critical_threshold(self):
        return self._reading("crit")

    def get_presence(self):
        path = self._path("input")
        return path is not None and path.is_file()

    def get_status(self):
        return self.get_temperature() is not None

    def get_position_in_parent(self):
        return self.position

    def is_replaceable(self):
        return False

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_temp_label(self):
        return self.name

    def get_low_threshold(self):
        return None

    def get_low_critical_threshold(self):
        return None

    def set_high_threshold(self, temperature):
        return False

    def set_high_critical_threshold(self, temperature):
        return False

    def set_low_threshold(self, temperature):
        return False

    def set_low_critical_threshold(self, temperature):
        return False
