import importlib.util
import sys
import types
from pathlib import Path

import pytest

from sonic_platform_pddf_base.pddf_fan_conversion import FanConversion


def conversion(input_offset=0, multiplier=1, pre_divide_offset=0, divisor=1,
               output_offset=0, divide_before_multiply=False,
               force_float=False):
    return {
        'input_offset': input_offset,
        'multiplier': multiplier,
        'pre_divide_offset': pre_divide_offset,
        'divisor': divisor,
        'output_offset': output_offset,
        'divide_before_multiply': divide_before_multiply,
        'force_float': force_float,
    }


class FakePddfParse(object):
    def __init__(self, pwm_path):
        self.pwm_path = pwm_path

    def get_path(self, device, attribute):
        return self.pwm_path


def load_fanutil(repository_root, relative_path, module_name):
    pddfparse = types.ModuleType('pddfparse')
    pddfparse.PddfParse = object
    sys.modules['pddfparse'] = pddfparse

    sonic_fan = types.ModuleType('sonic_fan')
    sonic_fan.__path__ = []
    fan_base = types.ModuleType('sonic_fan.fan_base')

    class FanBase(object):
        def __init__(self):
            pass

    fan_base.FanBase = FanBase
    sonic_fan.fan_base = fan_base
    sys.modules['sonic_fan'] = sonic_fan
    sys.modules['sonic_fan.fan_base'] = fan_base

    fanutil_path = repository_root / relative_path
    spec = importlib.util.spec_from_file_location(module_name, str(fanutil_path))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.mark.parametrize(
    'relative_path,module_name',
    [
        ('device/common/pddf/plugins/fanutil.py', 'pddf_common_fanutil'),
        (
            'device/ragile/x86_64-ragile_ra-b6910-64c-r0/plugins/fanutil.py',
            'pddf_ragile_fanutil',
        ),
    ],
)
def test_legacy_fanutil_uses_shared_converter(tmp_path, relative_path, module_name):
    repository_root = Path(__file__).resolve().parents[4]
    fanutil = load_fanutil(repository_root, relative_path, module_name)
    pwm_path = tmp_path / 'fan1_pwm'
    fanutil.plugin_data = {
        'FAN': {
            'duty_cycle_to_pwm': conversion(multiplier=255, divisor=100),
        },
    }
    fanutil.pddf_obj = FakePddfParse(str(pwm_path))

    fan = fanutil.FanUtil.__new__(fanutil.FanUtil)
    fan.num_fans = 1
    fan._duty_cycle_to_pwm = FanConversion(
        fanutil.plugin_data['FAN']['duty_cycle_to_pwm']
    )

    assert fan.set_speed(50) is True
    assert pwm_path.read_text() == '128'
