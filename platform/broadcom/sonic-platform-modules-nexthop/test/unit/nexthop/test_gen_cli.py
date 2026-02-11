#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import click
import textwrap
import tempfile
import os
import pytest

from click.testing import CliRunner

from nexthop import gen_cli


def test_generate_pddf_device_json_success():
    INPUT_PDDF_DEVICE_TEMPLATE = textwrap.dedent(
        """
        {
          "COMPONENT1": {
            "attr_list":
            [
              { "attr_name": "version", "get_cmd": "fpga read32 {{switchcard_fpga_bdf}} 0x0 | sed 's/^0x//'" }
            ]
          },
          "MULTIFPGAPCIE0": {
            "dev_info": {
              "device_name": "CPU_FPGA",
              "device_bdf": "{{cpu_card_fpga_bdf}}",
              "dev_attr": {}
            }
          },
          "MULTIFPGAPCIE1": {
            "dev_info": {
              "device_bdf": "{{switchcard_fpga_bdf}}"
            }
          }
        }
        """
    )
    INPUT_PCIE_VARIABLES = textwrap.dedent(
        """
        - name: "cpu_card_fpga_bdf"
          lookup_command: "echo 03 | xargs printf '0000:%s:00.0'"

        - name: "switchcard_fpga_bdf"
          lookup_command: "echo 04 | xargs printf '0000:%s:00.0'"
        """
    )
    INPUT_PLATFORM_JSON = textwrap.dedent(
        """
        {
          "chassis": {
            "name": "NH-4010-F"
            }
        }
        """
    )
    EXPECTED_PDDF_DEVICE_JSON = textwrap.dedent(
        """
        {
          "COMPONENT1": {
            "attr_list":
            [
              { "attr_name": "version", "get_cmd": "fpga read32 0000:04:00.0 0x0 | sed 's/^0x//'" }
            ]
          },
          "MULTIFPGAPCIE0": {
            "dev_info": {
              "device_name": "CPU_FPGA",
              "device_bdf": "0000:03:00.0",
              "dev_attr": {}
            }
          },
          "MULTIFPGAPCIE1": {
            "dev_info": {
              "device_bdf": "0000:04:00.0"
            }
          }
        }
        """
    )

    runner = CliRunner()
    with tempfile.TemporaryDirectory() as temp_dir:
        vars_path = os.path.join(temp_dir, "pcie-variables.yaml")
        template_path = os.path.join(temp_dir, "pddf-device.json.j2")
        platform_json_path = os.path.join(temp_dir, "platform.json")
        output_path = os.path.join(temp_dir, "pddf-device.json")

        # Given
        with open(vars_path, "w") as f:
            f.write(INPUT_PCIE_VARIABLES)
        with open(template_path, "w") as f:
            f.write(INPUT_PDDF_DEVICE_TEMPLATE)
        with open(platform_json_path, "w") as f:
            f.write(INPUT_PLATFORM_JSON)

        # When
        result = runner.invoke(
            gen_cli.pddf_device_json,
            [
                f"--template_filepath={template_path}",
                f"--vars_filepath={vars_path}",
                f"--platform_json_filepath={platform_json_path}",
                f"--output_filepath={output_path}",
            ],
        )

        # Then
        assert result.exit_code == 0
        assert os.path.exists(output_path)
        with open(output_path, "r") as f:
            generated_content = f.read()
        assert generated_content == EXPECTED_PDDF_DEVICE_JSON


def test_generate_pcie_yaml_success():
    INPUT_PCIE_TEMPLATE = textwrap.dedent(
        """
        - bus: '00'
          dev: '00'
          fn: '0'
          id: 14b5
          name: 'Host bridge: Advanced Micro Devices, Inc. [AMD] Family 17h-19h PCIe Root Complex (rev 01)'
        {%- if has_pci_bridge_00_01_1 == 'true' %}
        - bus: '00'
          dev: '01'
          fn: '1'
          id: 14b8
          name: 'PCI bridge: Advanced Micro Devices, Inc. [AMD] Family 17h-19h PCIe GPP Bridge'
        {%- endif %}
        {%- if has_pci_bridge_00_02_3 == 'true' %}
        - bus: '00'
          dev: '02'
          fn: '3'
          id: 14ba
          name: 'PCI bridge: Advanced Micro Devices, Inc. [AMD] Family 17h-19h PCIe GPP Bridge'
        {%- endif %}
        - bus: '{{asic_bus}}'
          dev: '00'
          fn: '0'
          id: f900
          name: 'Ethernet controller: Broadcom Inc. and subsidiaries BCM78900 Switch ASIC [Tomahawk5] (rev 11)'
        - bus: '{{cpu_card_fpga_bus}}'
          dev: '00'
          fn: '0'
          id: '7011'
          name: 'Serial controller: Xilinx Corporation 7-Series FPGA Hard PCIe block (AXI/debug)'
        - bus: '{{switchcard_fpga_bus}}'
          dev: '00'
          fn: '0'
          id: '7012'
          name: 'Serial controller: Xilinx Corporation Device 7012'
        - bus: '{{nvme_bus}}'
          dev: '00'
          fn: '0'
          id: 110b
          name: 'Non-Volatile memory controller: ATP ELECTRONICS INC Device 110b (rev 03)'
        - bus: '{{amd_soc_group_1_bus}}'
          dev: '00'
          fn: '0'
          id: 145a
          name: 'Non-Essential Instrumentation [1300]: Advanced Micro Devices, Inc. [AMD/ATI] Dummy Function (absent graphics controller)'
        - bus: '{{amd_soc_group_2_bus}}'
          dev: '00'
          fn: '0'
          id: 145a
          name: 'Non-Essential Instrumentation [1300]: Advanced Micro Devices, Inc. [AMD]'
        - bus: '{{amd_soc_group_3_bus}}'
          dev: '00'
          fn: '0'
          id: 161f
          name: 'USB controller: Advanced Micro Devices, Inc. [AMD] Rembrandt USB4 XHCI controller #8'
        """
    )
    INPUT_PCIE_VARIABLES = textwrap.dedent(
        """
        - name: "asic_bus"
          lookup_command: "echo 02"

        - name: "cpu_card_fpga_bus"
          lookup_command: "echo 03"

        - name: "switchcard_fpga_bus"
          lookup_command: "echo 04"

        - name: "nvme_bus"
          lookup_command: "echo 06"

        - name: "amd_soc_group_1_bus"
          lookup_command: "echo e5"

        - name: "amd_soc_group_2_bus"
          lookup_command: "echo e6"

        - name: "amd_soc_group_3_bus"
          lookup_command: "echo e7"

        - name: "has_pci_bridge_00_01_1"
          lookup_command: "echo true"

        - name: "has_pci_bridge_00_02_3"
          lookup_command: "echo false"
        """
    )
    INPUT_PLATFORM_JSON = textwrap.dedent(
        """
        {
          "chassis": {
            "name": "NH-4010-F"
            }
        }
        """
    )
    EXPECTED_PCIE_YAML = textwrap.dedent(
        """
        - bus: '00'
          dev: '00'
          fn: '0'
          id: 14b5
          name: 'Host bridge: Advanced Micro Devices, Inc. [AMD] Family 17h-19h PCIe Root Complex (rev 01)'
        - bus: '00'
          dev: '01'
          fn: '1'
          id: 14b8
          name: 'PCI bridge: Advanced Micro Devices, Inc. [AMD] Family 17h-19h PCIe GPP Bridge'
        - bus: '02'
          dev: '00'
          fn: '0'
          id: f900
          name: 'Ethernet controller: Broadcom Inc. and subsidiaries BCM78900 Switch ASIC [Tomahawk5] (rev 11)'
        - bus: '03'
          dev: '00'
          fn: '0'
          id: '7011'
          name: 'Serial controller: Xilinx Corporation 7-Series FPGA Hard PCIe block (AXI/debug)'
        - bus: '04'
          dev: '00'
          fn: '0'
          id: '7012'
          name: 'Serial controller: Xilinx Corporation Device 7012'
        - bus: '06'
          dev: '00'
          fn: '0'
          id: 110b
          name: 'Non-Volatile memory controller: ATP ELECTRONICS INC Device 110b (rev 03)'
        - bus: 'e5'
          dev: '00'
          fn: '0'
          id: 145a
          name: 'Non-Essential Instrumentation [1300]: Advanced Micro Devices, Inc. [AMD/ATI] Dummy Function (absent graphics controller)'
        - bus: 'e6'
          dev: '00'
          fn: '0'
          id: 145a
          name: 'Non-Essential Instrumentation [1300]: Advanced Micro Devices, Inc. [AMD]'
        - bus: 'e7'
          dev: '00'
          fn: '0'
          id: 161f
          name: 'USB controller: Advanced Micro Devices, Inc. [AMD] Rembrandt USB4 XHCI controller #8'
        """
    )

    runner = CliRunner()
    with tempfile.TemporaryDirectory() as temp_dir:
        vars_path = os.path.join(temp_dir, "pcie-variables.yaml")
        template_path = os.path.join(temp_dir, "pcie.yaml.j2")
        platform_json_path = os.path.join(temp_dir, "platform.json")
        output_path = os.path.join(temp_dir, "pcie.yaml")

        # Given
        with open(vars_path, "w") as f:
            f.write(INPUT_PCIE_VARIABLES)
        with open(template_path, "w") as f:
            f.write(INPUT_PCIE_TEMPLATE)
        with open(platform_json_path, "w") as f:
            f.write(INPUT_PLATFORM_JSON)

        # When
        result = runner.invoke(
            gen_cli.pddf_device_json,
            [
                f"--template_filepath={template_path}",
                f"--vars_filepath={vars_path}",
                f"--platform_json_filepath={platform_json_path}",
                f"--output_filepath={output_path}",
            ],
        )

        # Then
        assert result.exit_code == 0
        assert os.path.exists(output_path)
        with open(output_path, "r") as f:
            generated_content = f.read()
        assert generated_content == EXPECTED_PCIE_YAML


def test_generate_pddf_device_json_skipped_when_default_paths_not_found():
    if os.path.exists(
        gen_cli.DEFAULT_PDDF_DEVICE_JSON_TEMPLATE_FILEPATH
    ) or os.path.exists(
        gen_cli.DEFAULT_PCIE_VARS_FILEPATH
    ) or os.path.exists(
        gen_cli.DEFAULT_PLATFORM_JSON_FILEPATH
    ):
        pytest.skip("Default template, vars, or platform.json file exists. Skipping test.")
    runner = CliRunner()

    # Given
    result = runner.invoke(gen_cli.pddf_device_json)

    # Then
    assert result.exit_code == 0


def test_generate_pcie_yaml_skipped_when_default_files_not_found():
    if os.path.exists(
        gen_cli.DEFAULT_PCIE_YAML_TEMPLATE_FILEPATH
    ) or os.path.exists(
        gen_cli.DEFAULT_PCIE_VARS_FILEPATH
    ) or os.path.exists(
        gen_cli.DEFAULT_PLATFORM_JSON_FILEPATH
    ):
        pytest.skip("Default template, vars, or platform.json file exists. Skipping test.")
    runner = CliRunner()

    # Given
    result = runner.invoke(gen_cli.pcie_yaml)

    # Then
    assert result.exit_code == 0


def test_generate_pddf_device_json_raises_when_user_input_template_not_found():
    runner = CliRunner()
    with tempfile.TemporaryDirectory() as temp_dir:
        template_path = os.path.join(temp_dir, "non-existent-pddf-device.json.j2")
        output_path = os.path.join(temp_dir, "pddf-device.json")

        # When
        result = runner.invoke(
            gen_cli.pddf_device_json,
            [
                f"--template_filepath={template_path}",
                f"--output_filepath={output_path}",
            ],
        )

        # Then
        assert result.exit_code == click.BadParameter.exit_code
        assert not os.path.exists(output_path)


def test_generate_pddf_device_json_raises_when_user_input_vars_not_found():
    runner = CliRunner()
    with tempfile.TemporaryDirectory() as temp_dir:
        vars_path = os.path.join(temp_dir, "non-existent-pcie-variables.yaml")
        output_path = os.path.join(temp_dir, "pddf-device.json")

        # When
        result = runner.invoke(
            gen_cli.pddf_device_json,
            [
                f"--vars_filepath={vars_path}",
                f"--output_filepath={output_path}",
            ],
        )

        # Then
        assert result.exit_code == click.BadParameter.exit_code
        assert not os.path.exists(output_path)


def test_generate_pddf_device_json_raises_when_user_input_platform_json_not_found():
    runner = CliRunner()
    with tempfile.TemporaryDirectory() as temp_dir:
        platform_json_path = os.path.join(temp_dir, "non-existent-platform.json")
        output_path = os.path.join(temp_dir, "pddf-device.json")

        # When
        result = runner.invoke(
            gen_cli.pddf_device_json,
            [
                f"--platform_json_filepath={platform_json_path}",
                f"--output_filepath={output_path}",
            ],
        )

        # Then
        assert result.exit_code == click.BadParameter.exit_code
        assert not os.path.exists(output_path)


def test_generate_pcie_yaml_raises_when_user_input_template_not_found():
    runner = CliRunner()
    with tempfile.TemporaryDirectory() as temp_dir:
        template_path = os.path.join(temp_dir, "non-existent-pcie.yaml.j2")
        output_path = os.path.join(temp_dir, "pcie.yaml")

        # When
        result = runner.invoke(
            gen_cli.pcie_yaml,
            [
                f"--template_filepath={template_path}",
                f"--output_filepath={output_path}",
            ],
        )

        # Then
        assert result.exit_code == click.BadParameter.exit_code
        assert not os.path.exists(output_path)


def test_generate_pcie_yaml_raises_when_user_input_vars_not_found():
    runner = CliRunner()
    with tempfile.TemporaryDirectory() as temp_dir:
        vars_path = os.path.join(temp_dir, "non-existent-pcie-variables.yaml")
        output_path = os.path.join(temp_dir, "pcie.yaml")

        # When
        result = runner.invoke(
            gen_cli.pcie_yaml,
            [
                f"--vars_filepath={vars_path}",
                f"--output_filepath={output_path}",
            ],
        )

        # Then
        assert result.exit_code == click.BadParameter.exit_code
        assert not os.path.exists(output_path)
