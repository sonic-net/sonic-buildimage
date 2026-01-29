#!/usr/bin/env python3

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Test script for nh_reboot_cause utility.
This script sets up the necessary mocks and imports to test the CLI tool.
"""

import importlib
import os
import pytest
import textwrap
import sys

from click.testing import CliRunner
from unittest.mock import Mock, patch

# Prevent Python from writing .pyc files during test imports
# This avoids __pycache__ directories in common/utils/ that interfere with builds
sys.dont_write_bytecode = True


@pytest.fixture
def dpm_logger_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from sonic_platform import dpm_logger

    yield dpm_logger


@pytest.fixture
def nh_reboot_cause_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    # For files without .py extension, we need to use SourceFileLoader explicitly
    TEST_DIR = os.path.dirname(os.path.realpath(__file__))
    nh_reboot_cause_path = os.path.join(TEST_DIR, "../../../common/utils/nh_reboot_cause")
    loader = importlib.machinery.SourceFileLoader("nh_reboot_cause", nh_reboot_cause_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    nh_reboot_cause_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(nh_reboot_cause_module)

    yield nh_reboot_cause_module


def create_data_v1_sw_reboot(dpm_logger_module):
    return dpm_logger_module.DataV1(
        gen_time="2025-10-02 23:24:32 UTC",
        schema_version=1,
        causes=[
            dpm_logger_module.CauseV1(
                source="SW",
                timestamp="2025-10-02 23:22:06 UTC",
                cause="reboot",
                description="n/a",
            ),
            dpm_logger_module.CauseV1(
                source="cpu_card",
                timestamp="2025-10-02 23:22:56 UTC",
                cause="FPGA_PWR_CYC_REQ",
                description="FPGA requested power cycle",
            ),
        ],
        dpms=[
            dpm_logger_module.DpmV1(
                name="cpu_card",
                type="adm1266",
                records=[
                    {
                        "timestamp": "2025-10-02 23:22:56 UTC",
                        "dpm_name": "cpu_card",
                        "power_fault_cause": "FPGA_PWR_CYC_REQ (FPGA requested power cycle), under_voltage: VH1(POS12V), over_voltage: n/a",
                        "uid": "12345",
                        "powerup_counter": "1000",
                        "gpio_in_[7:4,9:8,R,R,R,3:1]": "0b100000000000 [GPI7]",
                        "pdio_in_[16:1]": "0b0000000000000010 [PDI2]",
                        "vh_over_voltage_[4:1]": "0b0000",
                        "vh_under_voltage_[4:1]": "0b0001 [VH1(POS12V)]",
                        "raw": textwrap.dedent(
                            """\
                            01 02 03 04 05 06 07 08
                            11 12 13 14 15 16 17 18
                            21 22 23 24 25 26 27 28
                            31 32 33 34 35 36 37 38
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ab"""
                        ),
                    }
                ],
            ),
            dpm_logger_module.DpmV1(
                name="switch_card",
                type="adm1266",
                records=[
                    {
                        "timestamp": "2025-10-02 22:10:10 UTC",
                        "dpm_name": "switch_card",
                        "power_fault_cause": "n/a, under_voltage: n/a, over_voltage: n/a",
                        "uid": "23455",
                        "powerup_counter": "2000",
                        "gpio_in_[7:4,9:8,R,R,R,3:1]": "0b000000000000",
                        "pdio_in_[16:1]": "0b0000000000000000",
                        "vh_over_voltage_[4:1]": "0b0000",
                        "vh_under_voltage_[4:1]": "0b0000",
                        "raw": textwrap.dedent(
                            """\
                            01 02 03 04 05 06 07 08
                            11 12 13 14 15 16 17 18
                            21 22 23 24 25 26 27 28
                            31 32 33 34 35 36 37 38
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ab"""
                        ),
                    },
                    {
                        "timestamp": "2025-10-02 23:22:56 UTC",
                        "dpm_name": "switch_card",
                        "power_fault_cause": "CPU_PWR_CYC_REQ (CPU card commanded power cycle), under_voltage: VH1(12V), over_voltage: n/a",
                        "uid": "23456",
                        "powerup_counter": "2000",
                        "gpio_in_[7:4,9:8,R,R,R,3:1]": "0b000000000000",
                        "pdio_in_[16:1]": "0b0000000100000000 [PDI9]",
                        "vh_over_voltage_[4:1]": "0b0000",
                        "vh_under_voltage_[4:1]": "0b0001 [VH1(12V)]",
                        "raw": textwrap.dedent(
                            """\
                            01 02 03 04 05 06 07 08
                            11 12 13 14 15 16 17 18
                            21 22 23 24 25 26 27 28
                            31 32 33 34 35 36 37 38
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ab"""
                        ),
                    },
                    {
                        "timestamp": "37.123456s after power-on",
                        "dpm_name": "switch_card",
                        "power_fault_cause": "n/a, under_voltage: n/a, over_voltage: n/a",
                        "uid": "23457",
                        "powerup_counter": "2001",
                        "gpio_in_[7:4,9:8,R,R,R,3:1]": "0b000000000000",
                        "pdio_in_[16:1]": "0b0000000000000000",
                        "vh_over_voltage_[4:1]": "0b0000",
                        "vh_under_voltage_[4:1]": "0b0000",
                        "raw": textwrap.dedent(
                            """\
                            01 02 03 04 05 06 07 08
                            11 12 13 14 15 16 17 18
                            21 22 23 24 25 26 27 28
                            31 32 33 34 35 36 37 38
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ab"""
                        ),
                    },
                ],
            ),
        ],
    )


def create_data_v1_hw_watchdog(dpm_logger_module):
    return dpm_logger_module.DataV1(
        gen_time="2025-10-03 05:13:55 UTC",
        schema_version=1,
        causes=[
            dpm_logger_module.CauseV1(
                source="switch_card",
                timestamp="2025-10-03 05:12:39 UTC",
                cause="WATCHDOG",
                description="FPGA watchdog expired",
            ),
        ],
        dpms=[
            dpm_logger_module.DpmV1(
                name="cpu_card",
                type="adm1266",
                records=[
                    {
                        "timestamp": "2025-10-03 05:12:38 UTC",
                        "dpm_name": "cpu_card",
                        "power_fault_cause": "n/a, under_voltage: VH1(POS12V), over_voltage: n/a",
                        "uid": "12346",
                        "powerup_counter": "1001",
                        "gpio_in_[7:4,9:8,R,R,R,3:1]": "0b000000000000",
                        "pdio_in_[16:1]": "0b0000000000000000",
                        "vh_over_voltage_[4:1]": "0b0000",
                        "vh_under_voltage_[4:1]": "0b0001 [VH1(POS12V)]",
                        "raw": textwrap.dedent(
                            """\
                            01 02 03 04 05 06 07 08
                            11 12 13 14 15 16 17 18
                            21 22 23 24 25 26 27 28
                            31 32 33 34 35 36 37 38
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ab"""
                        ),
                    }
                ],
            ),
            dpm_logger_module.DpmV1(
                name="switch_card",
                type="adm1266",
                records=[
                    {
                        "timestamp": "2025-10-03 05:12:39 UTC",
                        "dpm_name": "switch_card",
                        "power_fault_cause": "WATCHDOG (FPGA watchdog expired), under_voltage: VH1(12V), over_voltage: n/a",
                        "uid": "23458",
                        "powerup_counter": "2001",
                        "gpio_in_[7:4,9:8,R,R,R,3:1]": "0b000000000000",
                        "pdio_in_[16:1]": "0b0000000000001000 [PDI4]",
                        "vh_over_voltage_[4:1]": "0b0000",
                        "vh_under_voltage_[4:1]": "0b0001 [VH1(12V)]",
                        "raw": textwrap.dedent(
                            """\
                            01 02 03 04 05 06 07 08
                            11 12 13 14 15 16 17 18
                            21 22 23 24 25 26 27 28
                            31 32 33 34 35 36 37 38
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ff
                            ff ff ff ff ff ff ff ab"""
                        ),
                    },
                ],
            ),
        ],
    )


def test_show_current(dpm_logger_module, nh_reboot_cause_module):
    # Given
    DATA_V1 = create_data_v1_sw_reboot(dpm_logger_module)
    with patch.object(
        nh_reboot_cause_module, "DpmLogger", return_value=Mock(load=Mock(return_value=DATA_V1))
    ):
        # When
        result = CliRunner().invoke(nh_reboot_cause_module.reboot_cause)

        # Then
        assert result.exit_code == 0
        assert result.output == textwrap.dedent(
            """\
            === Captured at 2025-10-02 23:24:32 UTC ===
            [2025-10-02 23:22:56 UTC] cause: FPGA_PWR_CYC_REQ; description: FPGA requested power cycle; source: cpu_card
            [2025-10-02 23:22:06 UTC] cause: reboot; description: n/a; source: SW
            """
        )


def test_show_current_verbosity_1(dpm_logger_module, nh_reboot_cause_module):
    # Given
    DATA_V1 = create_data_v1_sw_reboot(dpm_logger_module)
    with patch.object(
        nh_reboot_cause_module, "DpmLogger", return_value=Mock(load=Mock(return_value=DATA_V1))
    ):
        # When
        result = CliRunner().invoke(nh_reboot_cause_module.reboot_cause, ["-v"])

        # Then
        assert result.exit_code == 0
        assert result.output == textwrap.dedent(
            """\
            === Captured at 2025-10-02 23:24:32 UTC ===
            [2025-10-02 23:22:56 UTC] cause: FPGA_PWR_CYC_REQ; description: FPGA requested power cycle; source: cpu_card
            [2025-10-02 23:22:06 UTC] cause: reboot; description: n/a; source: SW
             DPM records:
              cpu_card:powerup_1000:uid_12345          timestamp: 2025-10-02 23:22:56 UTC
                                                       power_fault_cause: FPGA_PWR_CYC_REQ (FPGA requested power cycle), under_voltage: VH1(POS12V), over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b100000000000 [GPI7]
                                                       pdio_in_[16:1]: 0b0000000000000010 [PDI2]

              switch_card:powerup_2001:uid_23457       timestamp: 37.123456s after power-on
                                                       power_fault_cause: n/a, under_voltage: n/a, over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000

              switch_card:powerup_2000:uid_23456       timestamp: 2025-10-02 23:22:56 UTC
                                                       power_fault_cause: CPU_PWR_CYC_REQ (CPU card commanded power cycle), under_voltage: VH1(12V), over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000100000000 [PDI9]

              switch_card:powerup_2000:uid_23455       timestamp: 2025-10-02 22:10:10 UTC
                                                       power_fault_cause: n/a, under_voltage: n/a, over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000

            """
        )


def test_show_current_verbosity_2(dpm_logger_module, nh_reboot_cause_module):
    # Given
    DATA_V1 = create_data_v1_sw_reboot(dpm_logger_module)
    with patch.object(
        nh_reboot_cause_module, "DpmLogger", return_value=Mock(load=Mock(return_value=DATA_V1))
    ):
        # When
        result = CliRunner().invoke(nh_reboot_cause_module.reboot_cause, ["-vv"])

        # Then
        assert result.exit_code == 0
        assert result.output == textwrap.dedent(
            """\
            === Captured at 2025-10-02 23:24:32 UTC ===
            [2025-10-02 23:22:56 UTC] cause: FPGA_PWR_CYC_REQ; description: FPGA requested power cycle; source: cpu_card
            [2025-10-02 23:22:06 UTC] cause: reboot; description: n/a; source: SW
             DPM records:
              cpu_card:powerup_1000:uid_12345          timestamp: 2025-10-02 23:22:56 UTC
                                                       dpm_name: cpu_card
                                                       power_fault_cause: FPGA_PWR_CYC_REQ (FPGA requested power cycle), under_voltage: VH1(POS12V), over_voltage: n/a
                                                       uid: 12345
                                                       powerup_counter: 1000
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b100000000000 [GPI7]
                                                       pdio_in_[16:1]: 0b0000000000000010 [PDI2]
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0001 [VH1(POS12V)]
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab

              switch_card:powerup_2001:uid_23457       timestamp: 37.123456s after power-on
                                                       dpm_name: switch_card
                                                       power_fault_cause: n/a, under_voltage: n/a, over_voltage: n/a
                                                       uid: 23457
                                                       powerup_counter: 2001
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0000
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab

              switch_card:powerup_2000:uid_23456       timestamp: 2025-10-02 23:22:56 UTC
                                                       dpm_name: switch_card
                                                       power_fault_cause: CPU_PWR_CYC_REQ (CPU card commanded power cycle), under_voltage: VH1(12V), over_voltage: n/a
                                                       uid: 23456
                                                       powerup_counter: 2000
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000100000000 [PDI9]
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0001 [VH1(12V)]
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab

              switch_card:powerup_2000:uid_23455       timestamp: 2025-10-02 22:10:10 UTC
                                                       dpm_name: switch_card
                                                       power_fault_cause: n/a, under_voltage: n/a, over_voltage: n/a
                                                       uid: 23455
                                                       powerup_counter: 2000
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0000
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab

            """
        )


def test_show_history(dpm_logger_module, nh_reboot_cause_module):
    # Given
    DATA_V1 = [
        create_data_v1_sw_reboot(dpm_logger_module),
        create_data_v1_hw_watchdog(dpm_logger_module),
    ]
    with patch.object(
        nh_reboot_cause_module, "DpmLogger", return_value=Mock(load_all=Mock(return_value=DATA_V1))
    ):
        # When
        result = CliRunner().invoke(nh_reboot_cause_module.reboot_cause, ["history"])

        # Then
        assert result.exit_code == 0
        assert result.output == textwrap.dedent(
            """\
            === Captured at 2025-10-03 05:13:55 UTC ===
            [2025-10-03 05:12:39 UTC] cause: WATCHDOG; description: FPGA watchdog expired; source: switch_card
            === Captured at 2025-10-02 23:24:32 UTC ===
            [2025-10-02 23:22:56 UTC] cause: FPGA_PWR_CYC_REQ; description: FPGA requested power cycle; source: cpu_card
            [2025-10-02 23:22:06 UTC] cause: reboot; description: n/a; source: SW
            """
        )


def test_show_history_verbosity_1(dpm_logger_module, nh_reboot_cause_module):
    # Given
    DATA_V1 = [
        create_data_v1_sw_reboot(dpm_logger_module),
        create_data_v1_hw_watchdog(dpm_logger_module),
    ]
    with patch.object(
        nh_reboot_cause_module, "DpmLogger", return_value=Mock(load_all=Mock(return_value=DATA_V1))
    ):
        # When
        result = CliRunner().invoke(nh_reboot_cause_module.reboot_cause, ["history", "-v"])

        # Then
        assert result.exit_code == 0
        assert result.output == textwrap.dedent(
            """\
            === Captured at 2025-10-03 05:13:55 UTC ===
            [2025-10-03 05:12:39 UTC] cause: WATCHDOG; description: FPGA watchdog expired; source: switch_card
             DPM records:
              cpu_card:powerup_1001:uid_12346          timestamp: 2025-10-03 05:12:38 UTC
                                                       power_fault_cause: n/a, under_voltage: VH1(POS12V), over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000
            
              switch_card:powerup_2001:uid_23458       timestamp: 2025-10-03 05:12:39 UTC
                                                       power_fault_cause: WATCHDOG (FPGA watchdog expired), under_voltage: VH1(12V), over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000001000 [PDI4]
            
            === Captured at 2025-10-02 23:24:32 UTC ===
            [2025-10-02 23:22:56 UTC] cause: FPGA_PWR_CYC_REQ; description: FPGA requested power cycle; source: cpu_card
            [2025-10-02 23:22:06 UTC] cause: reboot; description: n/a; source: SW
             DPM records:
              cpu_card:powerup_1000:uid_12345          timestamp: 2025-10-02 23:22:56 UTC
                                                       power_fault_cause: FPGA_PWR_CYC_REQ (FPGA requested power cycle), under_voltage: VH1(POS12V), over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b100000000000 [GPI7]
                                                       pdio_in_[16:1]: 0b0000000000000010 [PDI2]
            
              switch_card:powerup_2001:uid_23457       timestamp: 37.123456s after power-on
                                                       power_fault_cause: n/a, under_voltage: n/a, over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000
            
              switch_card:powerup_2000:uid_23456       timestamp: 2025-10-02 23:22:56 UTC
                                                       power_fault_cause: CPU_PWR_CYC_REQ (CPU card commanded power cycle), under_voltage: VH1(12V), over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000100000000 [PDI9]
            
              switch_card:powerup_2000:uid_23455       timestamp: 2025-10-02 22:10:10 UTC
                                                       power_fault_cause: n/a, under_voltage: n/a, over_voltage: n/a
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000

            """
        )


def test_show_history_verbosity_2(dpm_logger_module, nh_reboot_cause_module):
    # Given
    DATA_V1 = [
        create_data_v1_sw_reboot(dpm_logger_module),
        create_data_v1_hw_watchdog(dpm_logger_module),
    ]
    with patch.object(
        nh_reboot_cause_module, "DpmLogger", return_value=Mock(load_all=Mock(return_value=DATA_V1))
    ):
        # When
        result = CliRunner().invoke(nh_reboot_cause_module.reboot_cause, ["history", "-vv"])

        # Then
        assert result.exit_code == 0
        assert result.output == textwrap.dedent(
            """\
            === Captured at 2025-10-03 05:13:55 UTC ===
            [2025-10-03 05:12:39 UTC] cause: WATCHDOG; description: FPGA watchdog expired; source: switch_card
             DPM records:
              cpu_card:powerup_1001:uid_12346          timestamp: 2025-10-03 05:12:38 UTC
                                                       dpm_name: cpu_card
                                                       power_fault_cause: n/a, under_voltage: VH1(POS12V), over_voltage: n/a
                                                       uid: 12346
                                                       powerup_counter: 1001
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0001 [VH1(POS12V)]
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab
            
              switch_card:powerup_2001:uid_23458       timestamp: 2025-10-03 05:12:39 UTC
                                                       dpm_name: switch_card
                                                       power_fault_cause: WATCHDOG (FPGA watchdog expired), under_voltage: VH1(12V), over_voltage: n/a
                                                       uid: 23458
                                                       powerup_counter: 2001
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000001000 [PDI4]
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0001 [VH1(12V)]
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab
            
            === Captured at 2025-10-02 23:24:32 UTC ===
            [2025-10-02 23:22:56 UTC] cause: FPGA_PWR_CYC_REQ; description: FPGA requested power cycle; source: cpu_card
            [2025-10-02 23:22:06 UTC] cause: reboot; description: n/a; source: SW
             DPM records:
              cpu_card:powerup_1000:uid_12345          timestamp: 2025-10-02 23:22:56 UTC
                                                       dpm_name: cpu_card
                                                       power_fault_cause: FPGA_PWR_CYC_REQ (FPGA requested power cycle), under_voltage: VH1(POS12V), over_voltage: n/a
                                                       uid: 12345
                                                       powerup_counter: 1000
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b100000000000 [GPI7]
                                                       pdio_in_[16:1]: 0b0000000000000010 [PDI2]
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0001 [VH1(POS12V)]
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab
            
              switch_card:powerup_2001:uid_23457       timestamp: 37.123456s after power-on
                                                       dpm_name: switch_card
                                                       power_fault_cause: n/a, under_voltage: n/a, over_voltage: n/a
                                                       uid: 23457
                                                       powerup_counter: 2001
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0000
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab
            
              switch_card:powerup_2000:uid_23456       timestamp: 2025-10-02 23:22:56 UTC
                                                       dpm_name: switch_card
                                                       power_fault_cause: CPU_PWR_CYC_REQ (CPU card commanded power cycle), under_voltage: VH1(12V), over_voltage: n/a
                                                       uid: 23456
                                                       powerup_counter: 2000
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000100000000 [PDI9]
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0001 [VH1(12V)]
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab
            
              switch_card:powerup_2000:uid_23455       timestamp: 2025-10-02 22:10:10 UTC
                                                       dpm_name: switch_card
                                                       power_fault_cause: n/a, under_voltage: n/a, over_voltage: n/a
                                                       uid: 23455
                                                       powerup_counter: 2000
                                                       gpio_in_[7:4,9:8,R,R,R,3:1]: 0b000000000000
                                                       pdio_in_[16:1]: 0b0000000000000000
                                                       vh_over_voltage_[4:1]: 0b0000
                                                       vh_under_voltage_[4:1]: 0b0000
                                                       raw: 01 02 03 04 05 06 07 08
                                                            11 12 13 14 15 16 17 18
                                                            21 22 23 24 25 26 27 28
                                                            31 32 33 34 35 36 37 38
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ff
                                                            ff ff ff ff ff ff ff ab

            """
        )
