# LX2160ARDB temperature sensors, found by hwmon name (hwmon numbering is
# not stable): the SoC's on-die TMU sites (qoriq thermal zones) and the two
# SA56004 board sensors at I2C 0x4c/0x4d (local and remote channels).
# Names must match platform.json.
import glob
import os

from sonic_platform_base.thermal_base import ThermalBase

# name, hwmon name, I2C address (None: unique hwmon name), temp channel
THERMALS = [
    ("SoC cluster 2/3", "cluster2_3_thermal", None, 1),
    ("SoC cluster 4", "cluster4_thermal", None, 1),
    ("SoC cluster 6/7", "cluster6_7_thermal", None, 1),
    ("SoC CCN", "ccn_thermal", None, 1),
    ("SoC DCE", "dce_thermal", None, 1),
    ("SoC WRIOP", "wriop_thermal", None, 1),
    ("SoC DDR ctrl 5", "ddr_ctrl5_thermal", None, 1),
    ("Board 0x4c local", "sa56004", "004c", 1),
    ("Board 0x4c remote", "sa56004", "004c", 2),
    ("Board 0x4d local", "sa56004", "004d", 1),
    ("Board 0x4d remote", "sa56004", "004d", 2),
]

# LX2160A junction limits (thermal zones expose no thresholds in hwmon)
SOC_HIGH = 95.0
SOC_CRIT = 105.0


def _read(path):
    try:
        with open(path) as f:
            return f.read().strip()
    except OSError:
        return None


def _find_hwmon(name, addr):
    for h in sorted(glob.glob("/sys/class/hwmon/hwmon*")):
        if _read(os.path.join(h, "name")) != name:
            continue
        if addr is None or os.path.realpath(os.path.join(h, "device")).endswith("-" + addr):
            return h
    return None


class Thermal(ThermalBase):
    def __init__(self, index):
        ThermalBase.__init__(self)
        self.index = index
        self.name, self._hwmon_name, self._addr, self._chan = THERMALS[index]
        self._hwmon = None
        self.minimum_thermal = None
        self.maximum_thermal = None

    def _attr(self, attr):
        if self._hwmon is None:
            self._hwmon = _find_hwmon(self._hwmon_name, self._addr)
            if self._hwmon is None:
                return None
        v = _read(os.path.join(self._hwmon, "temp%d_%s" % (self._chan, attr)))
        try:
            return int(v) / 1000.0
        except (TypeError, ValueError):
            return None

    def get_name(self):
        return self.name

    def get_presence(self):
        return self.get_temperature() is not None

    def get_model(self):
        return "LX2160A TMU" if self._addr is None else "SA56004"

    def get_serial(self):
        return "N/A"

    def get_status(self):
        return self.get_presence()

    def is_replaceable(self):
        return False

    def get_position_in_parent(self):
        return self.index + 1

    def get_temperature(self):
        t = self._attr("input")
        if t is not None:
            self.minimum_thermal = t if self.minimum_thermal is None else min(self.minimum_thermal, t)
            self.maximum_thermal = t if self.maximum_thermal is None else max(self.maximum_thermal, t)
        return t

    def get_high_threshold(self):
        t = self._attr("max")
        return t if t is not None else SOC_HIGH

    def get_high_critical_threshold(self):
        t = self._attr("crit")
        return t if t is not None else SOC_CRIT

    def get_low_threshold(self):
        return "N/A"

    def get_low_critical_threshold(self):
        return "N/A"

    def get_minimum_recorded(self):
        return self.minimum_thermal

    def get_maximum_recorded(self):
        return self.maximum_thermal
