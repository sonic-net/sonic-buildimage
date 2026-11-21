"""Exercise the production PMBus callbacks without accessing hardware."""

import json
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
import unittest
from unittest.mock import Mock

from platform_test_utils import SOURCE, BASE, DEVICE, definitions


class As5835Psu(unittest.TestCase):
    def test_only_selected_psu_fan_comparisons_are_excluded(self):
        config = json.loads(
            (
                SOURCE / (DEVICE + "as5835_54x-r0/system_health_monitoring_config.json")
            ).read_text()
        )
        self.assertEqual(
            config["devices_to_ignore"],
            ["asic", "psu.temperature", "PSU-1 FAN-1.speed", "PSU-2 FAN-1.speed",
             "PSU-1 FAN-1.direction", "PSU-2 FAN-1.direction"],
        )
        from test_as5835_pddf import load_classes, mapped_api
        classes = load_classes()
        api, plugin = mapped_api({})
        instance = classes['Fan'](0, 0, api, plugin)
        instance.pddf_obj.get_attr_name_output = Mock(return_value={'mode': 'i2c', 'status': '40'})
        self.assertEqual(instance.get_target_speed(), 40)
        instance.is_psu_fan = True
        with self.assertRaises(NotImplementedError):
            instance.get_target_speed()

    def test_voltage_callbacks_use_model_specific_format(self):
        compiler = shutil.which("cc")
        self.assertIsNotNone(compiler, "C compiler required for PMBus regression")
        source = (SOURCE / (BASE + "as5835-54x/modules/ym2651y.c")).read_text()

        def block(pattern):
            match = re.search(pattern, source)
            self.assertIsNotNone(match, pattern)
            start = match.start()
            opening = source.index("{", match.start())
            depth = 1
            end = opening + 1
            while depth:
                depth += (source[end] == "{") - (source[end] == "}")
                end += 1
            return source[start:end]

        pieces = [
            block(r"enum chips \{"),
            block(r"enum ym2651y_sysfs_attributes \{"),
            block(r"struct ym2651y_data \{"),
        ]
        harness = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
typedef uint8_t u8;
typedef uint16_t u16;
struct device { int unused; };
struct mutex { int unused; };
struct i2c_client { int unused; };
struct device_attribute { int unused; };
struct sensor_device_attribute {
    struct device_attribute dev_attr;
    int index;
    ssize_t (*show)(struct device *, struct device_attribute *, char *);
};
#define to_sensor_dev_attr(p) ((struct sensor_device_attribute *)(p))
#define S_IRUGO 0
#define SENSOR_DEVICE_ATTR(name, mode, callback, store, number) \
    struct sensor_device_attribute sensor_dev_attr_##name = {{0}, number, callback}
"""
        harness += "\n".join(piece + ";" for piece in pieces)
        harness += r"""
static struct ym2651y_data fixture;
static struct ym2651y_data *ym2651y_update_device(struct device *dev) { return &fixture; }
static struct i2c_client *to_i2c_client(struct device *dev) { return NULL; }
static struct ym2651y_data *i2c_get_clientdata(struct i2c_client *client) { return &fixture; }
"""
        for function in (
            "two_complement_to_int",
            "show_linear",
            "show_vout_by_mode",
            "show_vout",
        ):
            harness += (
                block(r"static (?:int|ssize_t) " + function + r"\([^;]*?\)\s*\{") + "\n"
            )
        for attribute in ("psu_v_out", "psu_mfr_vout_min", "psu_mfr_vout_max"):
            harness += (
                re.search(
                    r"static SENSOR_DEVICE_ATTR\(" + attribute + r",[^;]+;", source
                )[0]
                + "\n"
            )
        harness += r"""
static int read_attr(struct sensor_device_attribute *attr) {
    char result[64];
    ssize_t length = attr->show(NULL, &attr->dev_attr, result);
    return length ? atoi(result) : -1;
}
int main(void) {
    fixture.valid = 1;
    fixture.vout_mode = 0x17; /* signed exponent -9 */
    fixture.v_out = 12 * 512;
    fixture.mfr_vout_min = 11 * 512;
    fixture.mfr_vout_max = 13 * 512;
    for (int chip = YM2401; chip <= YM1401A; chip++) {
        if (chip == YM2851) continue;
        fixture.chip = chip;
        assert(read_attr(&sensor_dev_attr_psu_v_out) == 12000);
        assert(read_attr(&sensor_dev_attr_psu_mfr_vout_min) == 11000);
        assert(read_attr(&sensor_dev_attr_psu_mfr_vout_max) == 13000);
    }
    fixture.valid = 0;
    assert(read_attr(&sensor_dev_attr_psu_mfr_vout_min) == -1);
    fixture.valid = 1;
    fixture.v_out = 12;
    fixture.mfr_vout_min = 11;
    fixture.mfr_vout_max = 13;
    for (int chip = YM2651; chip <= YPEB1200AM; chip++) {
        if (chip == YM2401 || chip == YM1401A) continue;
        fixture.chip = chip;
        assert(read_attr(&sensor_dev_attr_psu_v_out) == 12000);
        assert(read_attr(&sensor_dev_attr_psu_mfr_vout_min) == 11000);
        assert(read_attr(&sensor_dev_attr_psu_mfr_vout_max) == 13000);
    }
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / "test.c").write_text(harness)
            subprocess.run(
                [compiler, "-std=c99", str(path / "test.c"), "-o", str(path / "test")],
                check=True,
                capture_output=True,
                text=True,
            )
            subprocess.run([str(path / "test")], check=True, capture_output=True)
