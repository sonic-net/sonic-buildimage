#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import importlib
import os
import sys
from unittest import mock

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from mellanox_component_versions import main as main_module
from mellanox_component_versions.main import (
    parse_fw_version_sw,
    parse_fw_version_dpu,
    parse_compiled_components_file,
    get_platform_component_versions,
    get_current_version,
    get_component_rules
)


class TestParseFwVersion:
    FW_XML_OUTPUT = """
    <Devices>
    <Device type="Spectrum3" pciName="/dev/2">
      <Versions>
        <FW current="10.20.1111" available="N/A"/>
        <FW_Running current="10.20.2000" available="N/A"/>
      </Versions>
    </Device>
    <Device type="Spectrum3" pciName="/dev/1">
      <Versions>
        <FW current="10.20.1000" available="N/A"/>
      </Versions>
    </Device>
    <Device type="BlueField3" pciName="/dev/bf3">
      <Versions>
        <FW current="20.30.1000" available="N/A"/>
      </Versions>
    </Device>
    </Devices>
    """

    def test_parse_fw_version_sw_valid_xml(self):
        versions = parse_fw_version_sw(self.FW_XML_OUTPUT)
        assert versions == ["10.20.1000", "10.20.2000"]

    def test_parse_fw_version_dpu_valid_xml(self):
        versions = parse_fw_version_dpu(self.FW_XML_OUTPUT)
        assert len(versions) == 1
        assert versions[0] == "20.30.1000"

    def test_parse_fw_version_invalid_xml(self):
        invalid_xml = "not valid xml"
        versions = parse_fw_version_sw(invalid_xml)
        assert versions == ["N/A"]


class TestParseCompiledComponentsFile:

    @mock.patch('os.path.exists')
    @mock.patch('builtins.open', new_callable=mock.mock_open, read_data="MFT 4.20.0\nSDK 4.5.2010\nKERNEL 5.10.0\n")
    @mock.patch('mellanox_component_versions.main.get_asic_type', return_value="mellanox")
    def test_parse_compiled_components_file_exists(self, mock_asic_type, mock_file, mock_exists):
        mock_exists.return_value = True

        result = parse_compiled_components_file()
        assert result["MFT"] == "4.20.0"
        assert result["SDK"] == "4.5.2010"
        assert result["KERNEL"] == "5.10.0"
        assert "SIMX" in result

    @mock.patch('os.path.exists')
    @mock.patch('mellanox_component_versions.main.get_asic_type', return_value="mellanox")
    def test_parse_compiled_components_file_not_exists(self, mock_asic_type, mock_exists):
        mock_exists.return_value = False

        result = parse_compiled_components_file()
        # All components should be N/A
        for comp in list(get_component_rules().keys()) + ["SIMX"]:
            assert result[comp] == "N/A"

    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock.mock_open,
                read_data="MFT 4.20.0\nSDK 4.5.2010\nSAI 1.0.0\nSAI_API_HEADERS 1.14.0\nKERNEL 5.10.0\nBFSOC 1.2.3\n")
    @mock.patch('mellanox_component_versions.main.get_asic_type', return_value="bluefield")
    def test_parse_compiled_components_file_dpu_with_sai_api_headers(self, mock_asic_type, mock_file, mock_exists):
        result = parse_compiled_components_file()

        assert result["MFT"] == "4.20.0"
        assert result["SDK"] == "4.5.2010"
        assert result["SAI"] == "1.0.0"
        assert result["SAI_API_HEADERS"] == "1.14.0"
        assert result["KERNEL"] == "5.10.0"
        assert result["BFSOC"] == "1.2.3"
        assert "SIMX" in result


class TestGetPlatformComponentVersions:

    @mock.patch('mellanox_component_versions.main.get_pdp')
    def test_get_platform_component_versions_success(self, mock_get_pdp):
        mock_onie = mock.Mock()
        mock_onie.get_firmware_version.return_value = "2019.11-5.2.0020-115200"
        mock_bios = mock.Mock()
        mock_bios.get_firmware_version.return_value = "1.2.3"

        versions_map = {
            "ONIE": mock_onie,
            "BIOS": mock_bios
        }
        mock_ccm = {"chassis1": versions_map}

        mock_pdp = mock.Mock()
        mock_pdp.chassis_component_map = mock_ccm
        mock_pdp.chassis.get_name.return_value = "chassis1"
        mock_get_pdp.return_value = mock_pdp

        result = get_platform_component_versions()
        assert result["ONIE"] == "2019.11-5.2.0020-115200"
        assert result["BIOS"] == "1.2.3"


class TestGetCurrentVersion:

    @mock.patch('mellanox_component_versions.main.process_rule')
    @mock.patch('mellanox_component_versions.main.get_asic_type', return_value="mellanox")
    def test_get_current_version_success(self, mock_asic_type, mock_process):
        mock_process.return_value = (True, "1.2.3")

        unique, version = get_current_version("MFT")
        assert unique == True
        assert version == "1.2.3"

    @mock.patch('mellanox_component_versions.main.process_rule')
    @mock.patch('mellanox_component_versions.main.get_asic_type', return_value="mellanox")
    def test_get_current_version_exception(self, mock_asic_type, mock_process):
        mock_process.side_effect = Exception("Test error")

        unique, version = get_current_version("MFT")
        assert unique == True
        assert version == "N/A"


class TestBmcPlatform:
    """aspeed (SONiC BMC): sonic_platform there is
    sonic-platform-modules-nvidia-bmc, which has no device_data module."""

    def test_main_imports_without_sonic_platform(self):
        with mock.patch.dict(sys.modules, {'sonic_platform': None,
                                           'sonic_platform.device_data': None}):
            importlib.reload(main_module)

    def test_get_component_rules_returns_bmc_rules_on_aspeed(self):
        with mock.patch.object(main_module, 'get_asic_type', return_value='aspeed'):
            main_module.get_component_rules.cache_clear()
            rules = main_module.get_component_rules()
        main_module.get_component_rules.cache_clear()

        assert set(rules) == {"HW_MANAGEMENT", "KERNEL"}

    def test_hw_management_queries_the_bmc_only_package(self):
        captured = self._run_bmc_rule("HW_MANAGEMENT", "1.mlnx.7.0070.1013")
        assert "hw-management-bmc" in captured["cmd"]

    def test_hw_management_version_strips_the_mlnx_prefix(self):
        captured = self._run_bmc_rule("HW_MANAGEMENT", "1.mlnx.7.0070.1013")
        assert captured["version"] == "7.0070.1013"

    def test_kernel_version_strips_the_flavour_suffix(self):
        captured = self._run_bmc_rule("KERNEL", "6.1.0-29-2-arm64")
        assert captured["version"] == "6.1.0-29-2"

    @staticmethod
    def _run_bmc_rule(component, command_output):
        """Drive one BMC rule end to end with the shelled-out command stubbed."""
        captured = {}

        def fake_run(cmd, **kwargs):
            captured["cmd"] = " ".join(cmd)
            return mock.Mock(stdout=command_output)

        with mock.patch.object(main_module, 'get_asic_type', return_value='aspeed'), \
                mock.patch.object(main_module.subprocess, 'run', side_effect=fake_run):
            main_module.get_component_rules.cache_clear()
            unique, version = main_module.get_current_version(component)
        main_module.get_component_rules.cache_clear()

        assert unique is True
        captured["version"] = version
        return captured

    def test_main_does_not_query_fwutil_on_bmc(self):
        with mock.patch.object(main_module, 'get_pdp') as mock_get_pdp:
            self._run_main_on_bmc()
        mock_get_pdp.assert_not_called()

    def test_main_omits_platform_component_rows_on_bmc(self):
        output = self._run_main_on_bmc()
        for component in main_module.UNAVAILABLE_PLATFORM_VERSIONS:
            assert component not in output

    def test_main_reports_the_bmc_components(self):
        output = self._run_main_on_bmc()
        assert "HW_MANAGEMENT" in output
        assert "KERNEL" in output

    @staticmethod
    def _run_main_on_bmc(uid=0):
        """Run main() as it runs on the BMC and return what it printed."""
        with mock.patch.object(main_module.os, 'getuid', return_value=uid), \
                mock.patch.object(main_module, 'COMPONENT_VERSIONS_FILE',
                                  '/nonexistent/component-versions'), \
                mock.patch.object(main_module, 'get_asic_type', return_value='aspeed'), \
                mock.patch.object(main_module.subprocess, 'run',
                                  return_value=mock.Mock(stdout="1.mlnx.7.0070.1013")), \
                mock.patch('builtins.print') as mock_print:
            main_module.get_component_rules.cache_clear()
            main_module.main()
        main_module.get_component_rules.cache_clear()
        return "\n".join(str(call.args[0]) for call in mock_print.call_args_list if call.args)

    def test_main_does_not_require_root_on_bmc(self):
        """The BMC rules only read a deb version and uname -r."""
        output = self._run_main_on_bmc(uid=1000)
        assert "HW_MANAGEMENT" in output

    def test_main_still_requires_root_on_switch_platforms(self):
        """Guard: relaxing root on the BMC must not relax it for switches."""
        with mock.patch.object(main_module.os, 'getuid', return_value=1000), \
                mock.patch.object(main_module, 'get_asic_type', return_value='mellanox'), \
                mock.patch('builtins.print') as mock_print:
            main_module.get_component_rules.cache_clear()
            main_module.main()
        main_module.get_component_rules.cache_clear()

        output = "\n".join(str(call.args[0]) for call in mock_print.call_args_list if call.args)
        assert "Root privileges are required" in output
