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

import os
import sys
from unittest import mock

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

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
