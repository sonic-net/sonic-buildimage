#!/usr/bin/env python3
"""
Unit tests for sensor.py divisor/multiplier scaling (F078 fix).

Verifies that eval() is gone and that divisor/multiplier correctly
replaces the old 'format' string patterns from platform config.
"""
import sys
import os
import types
import unittest

# ---------------------------------------------------------------------------
# Stub heavy dependencies
# ---------------------------------------------------------------------------
_plat_hal_pkg = types.ModuleType("plat_hal")
sys.modules.setdefault("plat_hal", _plat_hal_pkg)

_devicebase_mod = types.ModuleType("plat_hal.devicebase")


class _StubDevicebase:
    def get_value(self, conf):
        return False, None

    def get_median(self, conf, read_times):
        return False, None


_devicebase_mod.devicebase = _StubDevicebase
sys.modules["plat_hal.devicebase"] = _devicebase_mod
_plat_hal_pkg.devicebase = _devicebase_mod

_HERE = os.path.dirname(os.path.abspath(__file__))
_HAL_DIR = os.path.dirname(_HERE)
if _HAL_DIR not in sys.path:
    sys.path.insert(0, _HAL_DIR)

from sensor import sensor

# Also register sensor under plat_hal.sensor so temp.py can import it
_sensor_mod = types.ModuleType("plat_hal.sensor")
_sensor_mod.sensor = sensor
sys.modules["plat_hal.sensor"] = _sensor_mod
_plat_hal_pkg.sensor = _sensor_mod

import temp as temp_mod
from temp import temp


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_sensor(divisor=None, multiplier=None, min_val=100, max_val=50000,
                low_val=200, high_val=45000):
    conf = {
        "value": None,
        "flag": None,
        "Min": min_val,
        "Max": max_val,
        "Low": low_val,
        "High": high_val,
        "Unit": None,
        "read_times": 1,
    }
    if divisor is not None:
        conf["divisor"] = divisor
    if multiplier is not None:
        conf["multiplier"] = multiplier
    return sensor(conf)


# ---------------------------------------------------------------------------
# Tests: no eval() in sensor.py or temp.py
# ---------------------------------------------------------------------------

class TestNoEval(unittest.TestCase):
    def test_sensor_module_has_no_eval(self):
        import inspect
        import sensor as sensor_mod
        source = inspect.getsource(sensor_mod)
        self.assertNotIn("eval(", source,
            "sensor.py must not contain eval()")

    def test_temp_module_has_no_format_eval(self):
        """temp.py Value property must not use eval(self.format %)."""
        temp_path = os.path.join(_HAL_DIR, "temp.py")
        with open(temp_path) as f:
            source = f.read()
        self.assertNotIn("eval(self.format", source,
            "temp.py must not contain eval(self.format ...)")

    def test_temp_scale_method_available(self):
        """temp instance must have _scale() from sensor inheritance — no AttributeError."""
        conf = {
            "name": "test_temp",
            "temp_id": "TEMP0",
            "Temperature": {
                "value": None,
                "flag": None,
                "Min": 0,
                "Max": 80000,
                "Low": 0,
                "High": 75000,
                "Unit": None,
                "divisor": 1000,
                "read_times": 1,
            },
        }
        t = temp(conf)
        # Must not raise AttributeError
        result = t._scale("50000")
        self.assertAlmostEqual(result, 50.0, places=3)

    def test_temp_scale_no_divisor(self):
        """temp without divisor returns round(float(val), 3)."""
        conf = {
            "name": "test_temp",
            "temp_id": "TEMP0",
            "Temperature": {
                "value": None,
                "flag": None,
                "Min": 0,
                "Max": 100,
                "Low": 0,
                "High": 90,
                "Unit": None,
                "read_times": 1,
            },
        }
        t = temp(conf)
        self.assertAlmostEqual(t._scale("42.5"), 42.5, places=3)


# ---------------------------------------------------------------------------
# Tests: _scale() correctness
# ---------------------------------------------------------------------------

class TestScaling(unittest.TestCase):
    def test_no_divisor_returns_float_rounded(self):
        """No divisor: _scale returns round(float(val), 3)."""
        s = make_sensor()
        self.assertAlmostEqual(s._scale("3.200"), 3.2, places=3)
        self.assertAlmostEqual(s._scale("42"), 42.0, places=3)

    def test_divisor_1_preserves_float(self):
        """divisor=1 (replaces 'format': '%s'): preserves float precision."""
        s = make_sensor(divisor=1)
        self.assertAlmostEqual(s._scale("3.200"), 3.2, places=3)
        self.assertAlmostEqual(s._scale("3.600"), 3.6, places=3)

    def test_divisor_1000(self):
        """float(float(%s)/1000) equivalent"""
        s = make_sensor(divisor=1000)
        self.assertAlmostEqual(s._scale("5000"), 5.0, places=3)
        self.assertAlmostEqual(s._scale("1234"), 1.234, places=3)

    def test_divisor_1000000(self):
        """float(float(%s)/1000000) equivalent"""
        s = make_sensor(divisor=1000000)
        self.assertAlmostEqual(s._scale("2000000"), 2.0, places=3)

    def test_divisor_1000_multiplier_1124(self):
        """float(float(%s)/1000) * 1.124 equivalent"""
        s = make_sensor(divisor=1000, multiplier=1.124)
        expected = round(5000 / 1000 * 1.124, 3)
        self.assertAlmostEqual(s._scale("5000"), expected, places=3)

    def test_default_multiplier_is_1(self):
        s = make_sensor(divisor=1000)
        self.assertEqual(s.multiplier, 1.0)

    def test_scale_result_rounded_to_3dp(self):
        s = make_sensor(divisor=1000)
        result = s._scale("1")
        self.assertEqual(result, round(1 / 1000 * 1.0, 3))


# ---------------------------------------------------------------------------
# Tests: Min/Max/Low/High property scaling
# ---------------------------------------------------------------------------

class TestPropertyScaling(unittest.TestCase):
    def test_min_scaled(self):
        s = make_sensor(divisor=1000)
        self.assertAlmostEqual(s.Min, 0.1, places=3)  # 100/1000

    def test_max_scaled(self):
        s = make_sensor(divisor=1000)
        self.assertAlmostEqual(s.Max, 50.0, places=3)  # 50000/1000

    def test_low_scaled(self):
        s = make_sensor(divisor=1000)
        self.assertAlmostEqual(s.Low, 0.2, places=3)  # 200/1000

    def test_high_scaled(self):
        s = make_sensor(divisor=1000)
        self.assertAlmostEqual(s.High, 45.0, places=3)  # 45000/1000

    def test_divisor_1_float_min_preserved(self):
        """divisor=1 preserves float Min (fixes MAC_QSFPDD_VDD3.3V_* regression)."""
        s = make_sensor(divisor=1, min_val=3.200, max_val=3.600,
                        low_val=3.100, high_val=3.700)
        self.assertAlmostEqual(s.Min, 3.2, places=3)
        self.assertAlmostEqual(s.Max, 3.6, places=3)

    def test_no_divisor_float_min_preserved(self):
        """No divisor: float Min also preserved (round(float(val), 3))."""
        s = make_sensor(min_val=3.200, max_val=3.600)
        self.assertAlmostEqual(s.Min, 3.2, places=3)


if __name__ == "__main__":
    unittest.main()
