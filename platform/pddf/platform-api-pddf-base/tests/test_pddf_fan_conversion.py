import glob
import json
import os

import pytest

from sonic_platform_pddf_base.pddf_fan_conversion import (
    FanConversion,
    FanConversionError,
)


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


@pytest.mark.parametrize(
    'config,value,expected',
    [
        (conversion(multiplier=255, divisor=100), 50, 128),
        (conversion(multiplier=100, divisor=255), 127, 50),
        (conversion(multiplier=6, output_offset=10), 15, 100),
        (conversion(input_offset=-10, divisor=6), 50, 7),
        (
            conversion(
                input_offset=1,
                multiplier=625,
                pre_divide_offset=75,
                divisor=100,
            ),
            127,
            801,
        ),
        (conversion(multiplier=100, divisor=625, output_offset=-1), 50, 7),
        (
            conversion(
                multiplier=100,
                divisor=18000,
                divide_before_multiply=True,
            ),
            18000,
            100,
        ),
        (
            conversion(
                multiplier=100,
                divisor=34000,
                divide_before_multiply=True,
            ),
            18530,
            55,
        ),
    ],
)
def test_affine_conversion_profiles(config, value, expected):
    assert int(round(FanConversion(config).convert(value))) == expected


def test_all_repository_conversions_are_declarative_and_valid():
    repository_root = os.path.abspath(
        os.path.join(os.path.dirname(__file__), '..', '..', '..', '..')
    )
    plugin_paths = glob.glob(
        os.path.join(repository_root, 'device', '*', '*', 'pddf', 'pd-plugin.json')
    )
    formula_files = 0
    formula_count = 0

    for plugin_path in plugin_paths:
        with open(plugin_path) as plugin_file:
            fan_data = json.load(plugin_file).get('FAN', {})

        file_has_formula = False
        for field in ('pwm_to_duty_cycle', 'duty_cycle_to_pwm'):
            if field not in fan_data:
                continue
            file_has_formula = True
            formula_count += 1
            converter = FanConversion(fan_data[field])
            values = range(0, 101) if field == 'duty_cycle_to_pwm' else (
                0, 1, 50, 100, 127, 255, 256, 625, 18000, 34000
            )
            for value in values:
                converter.convert(value)

        formula_files += int(file_has_formula)

    assert formula_files == 27
    assert formula_count == 50


@pytest.mark.parametrize(
    'config',
    [
        None,
        'lambda value: value',
        [],
        {},
        conversion(divisor=0),
        conversion(divisor=-1),
        conversion(multiplier=True),
        conversion(multiplier=1.0),
        conversion(pre_divide_offset=1, divide_before_multiply=True),
        dict(conversion(), divide_before_multiply=1),
        dict(conversion(), force_float=1),
        dict(conversion(), operation='multiply'),
    ],
)
def test_invalid_conversion_config_is_rejected(config):
    with pytest.raises(FanConversionError):
        FanConversion(config)


@pytest.mark.parametrize('value', [None, True, '1', float('inf'), float('nan')])
def test_invalid_inputs_are_rejected(value):
    with pytest.raises(FanConversionError):
        FanConversion(conversion()).convert(value)


def test_oversized_values_are_rejected():
    with pytest.raises(FanConversionError):
        FanConversion(conversion(multiplier=10 ** 12 + 1))

    with pytest.raises(FanConversionError):
        FanConversion(conversion()).convert(10 ** 12 + 1)
