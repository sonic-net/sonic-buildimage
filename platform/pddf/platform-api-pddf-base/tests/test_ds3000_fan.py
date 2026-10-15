import importlib.util
import sys
import types
from pathlib import Path


if 'sonic_platform_base.fan_base' not in sys.modules:
    sonic_platform_base = types.ModuleType('sonic_platform_base')
    sonic_platform_base.__path__ = []
    fan_base = types.ModuleType('sonic_platform_base.fan_base')

    class FanBase(object):
        def __init__(self):
            pass

    fan_base.FanBase = FanBase
    sonic_platform_base.fan_base = fan_base
    sys.modules['sonic_platform_base'] = sonic_platform_base
    sys.modules['sonic_platform_base.fan_base'] = fan_base


from sonic_platform_pddf_base.pddf_fan_conversion import FanConversion


def conversion():
    return {
        'input_offset': 0,
        'multiplier': 255,
        'pre_divide_offset': 0,
        'divisor': 100,
        'output_offset': 0,
        'divide_before_multiply': False,
        'force_float': False,
    }


class FakeAPIHelper(object):
    writes = []

    def is_bmc_present(self):
        return False

    def lpc_setreg(self, path, register, value):
        self.writes.append((path, register, value))
        return True


def load_ds3000_fan(repository_root):
    package_name = 'ds3000_sonic_platform'
    package = types.ModuleType(package_name)
    package.__path__ = []
    sys.modules[package_name] = package

    helper = types.ModuleType(package_name + '.helper')
    helper.APIHelper = FakeAPIHelper
    sys.modules[package_name + '.helper'] = helper

    path = (
        repository_root /
        'platform/broadcom/sonic-platform-modules-cel/ds3000/pddf/'
        'sonic_platform/fan.py'
    )
    spec = importlib.util.spec_from_file_location(
        package_name + '.fan', str(path)
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_ds3000_set_speed_uses_declarative_converter():
    repository_root = Path(__file__).resolve().parents[4]
    module = load_ds3000_fan(repository_root)
    FakeAPIHelper.writes = []

    fan = module.Fan.__new__(module.Fan)
    fan.is_psu_fan = False
    fan.fan_index = 1
    fan.fantray_index = 1
    fan.plugin_data = {'FAN': {'duty_cycle_to_pwm': conversion()}}
    fan._duty_cycle_to_pwm = FanConversion(conversion())
    fan._api_helper = FakeAPIHelper()

    assert fan.set_speed(50) is True
    assert FakeAPIHelper.writes == [
        (fan.LPC_CPLD_SETREG_PATH, '0xa1b2', '0x80'),
    ]
