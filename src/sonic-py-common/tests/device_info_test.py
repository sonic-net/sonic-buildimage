import os
import sys

# TODO: Remove this if/else block once we no longer support Python 2
if sys.version_info.major == 3:
    from unittest import mock
else:
    # Expect the 'mock' package for python 2
    # https://pypi.python.org/pypi/mock
    import mock

import pytest
import json

from sonic_py_common import device_info

from .mock_swsscommon import SonicV2Connector

# TODO: Remove this if/else block once we no longer support Python 2
if sys.version_info.major == 3:
    BUILTINS = "builtins"
else:
    BUILTINS = "__builtin__"

MACHINE_CONF_CONTENTS = """\
onie_version=2016.11-5.1.0008-9600
onie_vendor_id=33049
onie_machine_rev=0
onie_arch=x86_64
onie_config_version=1
onie_build_date="2017-04-26T11:01+0300"
onie_partition_type=gpt
onie_kernel_version=4.10.11
onie_firmware=auto
onie_switch_asic=mlnx
onie_skip_ethmgmt_macs=yes
onie_machine=mlnx_msn2700
onie_platform=x86_64-mlnx_msn2700-r0"""

EXPECTED_GET_MACHINE_INFO_RESULT = {
    'onie_arch': 'x86_64',
    'onie_skip_ethmgmt_macs': 'yes',
    'onie_platform': 'x86_64-mlnx_msn2700-r0',
    'onie_machine_rev': '0',
    'onie_version': '2016.11-5.1.0008-9600',
    'onie_machine': 'mlnx_msn2700',
    'onie_config_version': '1',
    'onie_partition_type': 'gpt',
    'onie_build_date': '"2017-04-26T11:01+0300"',
    'onie_switch_asic': 'mlnx',
    'onie_vendor_id': '33049',
    'onie_firmware': 'auto',
    'onie_kernel_version': '4.10.11'
}

SONIC_VERISON_YML = """\
---
build_version: 'test_branch.1-a8fbac59d'
debian_version: '11.4'
kernel_version: '5.10.0-18-2-amd64'
asic_type: mellanox
asic_subtype: 'mellanox'
commit_id: 'a8fbac59d'
branch: 'test_branch'
release: 'master'
libswsscommon: 1.0.0
sonic_utilities: 1.2"""

SONIC_VERISON_YML_RESULT = {
    'build_version': 'test_branch.1-a8fbac59d',
    'debian_version': '11.4',
    'kernel_version': '5.10.0-18-2-amd64',
    'asic_type': 'mellanox',
    'asic_subtype': 'mellanox',
    'commit_id': 'a8fbac59d',
    'branch': 'test_branch',
    'release': 'master',
    'libswsscommon': '1.0.0',
    'sonic_utilities': 1.2
}

class TestDeviceInfo(object):
    @pytest.fixture(scope="class", autouse=True)
    def sanitize_environment(self):
        # Clear environment variables, in case a variable is set in the test
        # environment (e.g., PLATFORM) which could modify the behavior of sonic-py-common
        with mock.patch.dict(os.environ, {}, clear=True):
            yield

    def test_get_machine_info(self):
        with mock.patch("os.path.isfile") as mock_isfile:
            mock_isfile.return_value = True
            open_mocked = mock.mock_open(read_data=MACHINE_CONF_CONTENTS)
            with mock.patch("{}.open".format(BUILTINS), open_mocked):
                result = device_info.get_machine_info()
                assert result == EXPECTED_GET_MACHINE_INFO_RESULT
                open_mocked.assert_called_once_with("/host/machine.conf")

    def test_get_platform(self):
        with mock.patch("sonic_py_common.device_info.get_machine_info") as get_machine_info_mocked:
            get_machine_info_mocked.return_value = EXPECTED_GET_MACHINE_INFO_RESULT
            result = device_info.get_platform()
            assert result == "x86_64-mlnx_msn2700-r0"

    def test_get_chassis_info(self):
        with mock.patch("sonic_py_common.device_info.SonicV2Connector", new=SonicV2Connector):
            result = device_info.get_chassis_info()
            truth = {"serial": SonicV2Connector.TEST_SERIAL,
                     "model": SonicV2Connector.TEST_MODEL,
                     "revision": SonicV2Connector.TEST_REV,
                     "switch_host_serial": SonicV2Connector.TEST_SWITCH_HOST_SERIAL}
            assert result == truth

    @mock.patch("os.path.isfile")
    def test_get_sonic_version(self, mock_isfile):
        mock_isfile.return_value = True
        open_mocked = mock.mock_open(read_data=SONIC_VERISON_YML)
        with mock.patch("{}.open".format(BUILTINS), open_mocked):
            for _ in range(0,5):
                assert device_info.get_sonic_version_info() == SONIC_VERISON_YML_RESULT
            # Assert the file was read only once
            open_mocked.assert_called_once_with(device_info.SONIC_VERSION_YAML_PATH)

    @mock.patch("sonic_py_common.device_info.is_chassis_config_absent")
    @mock.patch("sonic_py_common.device_info.get_platform_info")
    @mock.patch("sonic_py_common.device_info.is_disaggregated_chassis")
    def test_is_chassis(self, mock_is_disaggregated_chassis, mock_platform_info, mock_is_chassis_config_absent):
        mock_platform_info.return_value = {"switch_type": "npu"}
        mock_is_disaggregated_chassis.return_value = False
        mock_is_chassis_config_absent.return_value = False
        assert device_info.is_chassis() == False
        assert device_info.is_voq_chassis() == False
        assert device_info.is_packet_chassis() == False

        mock_platform_info.return_value = {"switch_type": "voq"}
        mock_is_disaggregated_chassis.return_value = False
        mock_is_chassis_config_absent.return_value = False
        assert device_info.is_voq_chassis() == True
        assert device_info.is_packet_chassis() == False
        assert device_info.is_chassis() == True

        mock_platform_info.return_value = {"switch_type": "voq"}
        mock_is_disaggregated_chassis.return_value = True
        mock_is_chassis_config_absent.return_value = False
        assert device_info.is_voq_chassis() == True
        assert device_info.is_packet_chassis() == False
        assert device_info.is_chassis() == False

        mock_platform_info.return_value = {"switch_type": "voq"}
        mock_is_disaggregated_chassis.return_value = False
        mock_is_chassis_config_absent.return_value = True
        assert device_info.is_voq_chassis() == False
        assert device_info.is_packet_chassis() == False
        assert device_info.is_chassis() == False

        mock_platform_info.return_value = {"switch_type": "chassis-packet"}
        mock_is_disaggregated_chassis.return_value = False
        mock_is_chassis_config_absent.return_value = False
        assert device_info.is_voq_chassis() == False
        assert device_info.is_packet_chassis() == True
        assert device_info.is_chassis() == True

        mock_platform_info.return_value = {"switch_type": "dummy-sup",
                                           "asic_type": "vs"}
        mock_is_disaggregated_chassis.return_value = False
        mock_is_chassis_config_absent.return_value = False
        assert device_info.is_voq_chassis() == False
        assert device_info.is_packet_chassis() == False
        assert device_info.is_virtual_chassis() == True
        assert device_info.is_chassis() == True

        mock_platform_info.return_value = {}
        mock_is_disaggregated_chassis.return_value = False
        mock_is_chassis_config_absent.return_value = False
        assert device_info.is_voq_chassis() == False
        assert device_info.is_packet_chassis() == False
        assert device_info.is_chassis() == False

    @mock.patch("sonic_py_common.device_info.ConfigDBConnector", autospec=True)
    @mock.patch("sonic_py_common.device_info.get_sonic_version_info")
    @mock.patch("sonic_py_common.device_info.get_machine_info")
    @mock.patch("sonic_py_common.device_info.get_hwsku")
    def test_get_platform_info(self, mock_hwsku, mock_machine_info, mock_sonic_ver, mock_cfg_db):
        mock_cfg_inst = mock_cfg_db.return_value
        mock_cfg_inst.get_table.return_value = {"localhost": {"switch_type": "npu"}}
        mock_sonic_ver.return_value = SONIC_VERISON_YML_RESULT
        mock_machine_info.return_value = {"onie_platform" : "x86_64-mlnx_msn2700-r0"}
        mock_hwsku.return_value = "Mellanox-SN2700"
        for _ in range(0,5):
            hw_info_dict = device_info.get_platform_info()
            assert hw_info_dict["asic_type"] == "mellanox"
            assert hw_info_dict["platform"] == "x86_64-mlnx_msn2700-r0"
            assert hw_info_dict["hwsku"] == "Mellanox-SN2700"
            assert hw_info_dict["switch_type"] == "npu"
        mock_sonic_ver.assert_called_once()
        # TODO(trixie): Figure out why this is failing
        # mock_machine_info.assert_called_once()
        mock_hwsku.assert_called_once()
        mock_cfg_inst.get_table.assert_called_once_with("DEVICE_METADATA")

    @mock.patch("os.path.isfile")
    @mock.patch("{}.open".format(BUILTINS))
    @mock.patch("sonic_py_common.device_info.get_path_to_platform_dir")
    @mock.patch("sonic_py_common.device_info.get_platform")
    def test_get_platform_json_data(self, mock_get_platform, mock_get_path_to_platform_dir, mock_open, mock_isfile):
        mock_get_platform.return_value = "x86_64-mlnx_msn2700-r0"
        mock_get_path_to_platform_dir.return_value = "/usr/share/sonic/device"
        mock_isfile.return_value = True
        platform_json_data = {
            "chassis": {
                "name": "MSN2700"
            }
        }
        open_mocked = mock.mock_open(read_data=json.dumps(platform_json_data))
        mock_open.side_effect = open_mocked

        result = device_info.get_platform_json_data()
        assert result == platform_json_data
        assert mock_open.call_count == 1

        # Test case where platform is None
        mock_get_platform.return_value = None
        result = device_info.get_platform_json_data()
        assert result is None

        # Test case where platform path is None
        mock_get_platform.return_value = "x86_64-mlnx_msn2700-r0"
        mock_get_path_to_platform_dir.return_value = None
        result = device_info.get_platform_json_data()
        assert result is None

        # Test case where platform.json file does not exist
        mock_get_path_to_platform_dir.return_value = "/usr/share/sonic/device"
        mock_isfile.return_value = False
        result = device_info.get_platform_json_data()
        assert result is None

        # Test case where JSON decoding fails
        mock_isfile.return_value = True
        open_mocked = mock.mock_open(read_data="invalid json")
        mock_open.side_effect = open_mocked
        result = device_info.get_platform_json_data()
        assert result is None

    @mock.patch("sonic_py_common.device_info.get_platform_json_data")
    @mock.patch("sonic_py_common.device_info.get_platform")
    def test_is_smartswitch(self, mock_get_platform, mock_get_platform_json_data):
        # Test case where platform is None
        mock_get_platform.return_value = None
        assert device_info.is_smartswitch() == False

        # Test case where platform.json data is None
        mock_get_platform.return_value="x86_64-mlnx_msn2700-r0"
        mock_get_platform_json_data.return_value=None
        assert device_info.is_smartswitch() == False

        # Test case where platform.json data does not contain "DPUS"
        mock_get_platform_json_data.return_value={}
        assert device_info.is_smartswitch() == False

        # Test case where platform.json data contains "DPUS"
        mock_get_platform_json_data.return_value={"DPUS": {}}
        assert device_info.is_smartswitch() == True

    @mock.patch("sonic_py_common.device_info.get_platform_json_data")
    @mock.patch("sonic_py_common.device_info.is_smartswitch")
    @mock.patch("sonic_py_common.device_info.get_platform")
    def test_is_dpu(self, mock_get_platform, mock_is_smartswitch, mock_get_platform_json_data):
        # Test case where platform is None
        mock_get_platform.return_value=None
        assert device_info.is_dpu() == False

        # Test case where platform is not a smart switch
        mock_get_platform.return_value="x86_64-mlnx_msn2700-r0"
        mock_is_smartswitch.return_value=False
        assert device_info.is_dpu() == False

        # Test case where platform is a smart switch but no DPU data in platform.json
        mock_is_smartswitch.return_value=True
        mock_get_platform_json_data.return_value={}
        assert device_info.is_dpu() == False

        # Test case where platform is a smart switch and DPU data is present in platform.json
        mock_get_platform_json_data.return_value={"DPU": {}}
        assert device_info.is_dpu() == True

        # Test case where platform is a smart switch and DPU data is present in platform.json
        mock_get_platform_json_data.return_value={"DPUS": {}}
        assert device_info.is_dpu() == False

    @mock.patch("sonic_py_common.device_info.get_platform_json_data")
    @mock.patch("sonic_py_common.device_info.get_platform")
    def test_get_dpu_info(self, mock_get_platform, mock_get_platform_json_data):
        # Test case where platform is None
        mock_get_platform.return_value = None
        assert device_info.get_dpu_info() == {}

        # Test case where platform.json data is None
        mock_get_platform.return_value = "x86_64-mlnx_msn2700-r0"
        mock_get_platform_json_data.return_value = None
        assert device_info.get_dpu_info() == {}

        # Test case where platform.json data does not contain "DPUS" or "DPU"
        mock_get_platform_json_data.return_value = {}
        assert device_info.get_dpu_info() == {}

        # Test case where platform.json data contains "DPUS"
        mock_get_platform_json_data.return_value = {"DPUS": {"dpu0": {}, "dpu1": {}}}
        assert device_info.get_dpu_info() == {"dpu0": {}, "dpu1": {}}

        # Test case where platform.json data contains "DPU"
        mock_get_platform_json_data.return_value = {"DPU": {"dpu0": {}}}
        assert device_info.get_dpu_info() == {"dpu0": {}}

        # Test case where platform.json data does not contain "DPU" or "DPUS"
        mock_get_platform_json_data.return_value = {"chassis": {}}
        assert device_info.get_dpu_info() == {}

    @mock.patch("sonic_py_common.device_info.get_platform_json_data")
    @mock.patch("sonic_py_common.device_info.is_dpu")
    @mock.patch("sonic_py_common.device_info.get_platform")
    def test_get_num_dpus(self, mock_get_platform, mock_is_dpu, mock_get_platform_json_data):
        # Test case where platform is None
        mock_get_platform.return_value = None
        assert device_info.get_num_dpus() == 0

        # Test case where platform is a DPU
        mock_get_platform.return_value = "x86_64-mlnx_msn2700-r0"
        mock_is_dpu.return_value = True
        assert device_info.get_num_dpus() == 0

        # Test case where platform.json data is None
        mock_is_dpu.return_value = False
        mock_get_platform_json_data.return_value = None
        assert device_info.get_num_dpus() == 0

        # Test case where platform.json data does not contain DPUs
        mock_get_platform_json_data.return_value = {}
        assert device_info.get_num_dpus() == 0

        # Test case where platform.json data contains DPUs
        mock_get_platform_json_data.return_value = {"DPUS": {"dpu0": {}, "dpu1": {}}}
        assert device_info.get_num_dpus() == 2

    @mock.patch("sonic_py_common.device_info.get_platform_json_data")
    @mock.patch("sonic_py_common.device_info.is_dpu")
    @mock.patch("sonic_py_common.device_info.get_platform")
    def test_get_dpu_list(self, mock_get_platform, mock_is_dpu, mock_get_platform_json_data):
        # Test case where platform is None
        mock_get_platform.return_value = None
        assert device_info.get_dpu_list() == []

        # Test case where platform is a DPU
        mock_get_platform.return_value = "x86_64-mlnx_msn2700-r0"
        mock_is_dpu.return_value = True
        assert device_info.get_dpu_list() == []

        # Test case where platform.json data is None
        mock_is_dpu.return_value = False
        mock_get_platform_json_data.return_value = None
        assert device_info.get_dpu_list() == []

        # Test case where platform.json data does not contain DPUs
        mock_get_platform_json_data.return_value = {}
        assert device_info.get_dpu_list() == []

        # Test case where platform.json data contains DPUs
        mock_get_platform_json_data.return_value = {"DPUS": {"dpu0": {}, "dpu1": {}}}
        assert device_info.get_dpu_list() == ["dpu0", "dpu1"]

    # ------------------------------------------------------------------
    # Tests for BMC platform detection APIs
    # ------------------------------------------------------------------

    PLATFORM_ENV_BMC_CONTENTS = """\
switch_bmc=1
liquid_cooled=true
"""

    PLATFORM_ENV_HOST_CONTENTS = """\
switch_host=1
liquid_cooled=true
"""

    @mock.patch("sonic_py_common.device_info.get_platform_env_conf_file_path")
    def test_is_switch_bmc(self, mock_get_env_path):
        # platform_env.conf not found
        mock_get_env_path.return_value = None
        assert device_info.is_switch_bmc() is False

        open_bmc = mock.mock_open(read_data=self.PLATFORM_ENV_BMC_CONTENTS)
        with mock.patch("{}.open".format(BUILTINS), open_bmc):
            mock_get_env_path.return_value = "/usr/share/sonic/platform/platform_env.conf"
            assert device_info.is_switch_bmc() is True

        open_host = mock.mock_open(read_data=self.PLATFORM_ENV_HOST_CONTENTS)
        with mock.patch("{}.open".format(BUILTINS), open_host):
            assert device_info.is_switch_bmc() is False

    @mock.patch("sonic_py_common.device_info.get_platform_env_conf_file_path")
    def test_is_switch_host(self, mock_get_env_path):
        # platform_env.conf not found
        mock_get_env_path.return_value = None
        assert device_info.is_switch_host() is False

        open_host = mock.mock_open(read_data=self.PLATFORM_ENV_HOST_CONTENTS)
        with mock.patch("{}.open".format(BUILTINS), open_host):
            mock_get_env_path.return_value = "/usr/share/sonic/platform/platform_env.conf"
            assert device_info.is_switch_host() is True

        open_bmc = mock.mock_open(read_data=self.PLATFORM_ENV_BMC_CONTENTS)
        with mock.patch("{}.open".format(BUILTINS), open_bmc):
            assert device_info.is_switch_host() is False

    @mock.patch("os.path.exists")
    @mock.patch("sonic_py_common.device_info.get_path_to_platform_dir")
    def test_get_bmc_data(self, mock_platform_dir, mock_exists):
        BMC_GLOBAL = '{"bmc_if_name": "bmc0", "bmc_if_addr": "169.254.100.2", ' \
                     '"bmc_addr": "169.254.100.1", "bmc_net_mask": "255.255.255.252"}'
        BMC_PLATFORM = '{"bmc_if_name": "usb0", "bmc_if_addr": "169.254.0.2", ' \
                       '"bmc_addr": "169.254.0.1", "bmc_net_mask": "255.255.255.252"}'

        mock_platform_dir.return_value = "/usr/share/sonic/platform"

        # Platform bmc.json absent, global bmc.json present – global is returned
        def exists_global(path):
            return path == device_info.GLOBAL_BMC_DATA_FILE

        mock_exists.side_effect = exists_global
        with mock.patch("{}.open".format(BUILTINS),
                        mock.mock_open(read_data=BMC_GLOBAL)):
            result = device_info.get_bmc_data()
        assert result is not None
        assert result["bmc_addr"] == "169.254.100.1"
        assert result["bmc_if_name"] == "bmc0"

        # Platform bmc.json present – platform takes precedence over global
        def exists_both(path):
            return path in (device_info.GLOBAL_BMC_DATA_FILE,
                            "/usr/share/sonic/platform/bmc.json")

        mock_exists.side_effect = exists_both
        # open() returns platform data for platform path, global data for global path
        def open_side_effect(path, *args, **kwargs):
            data = BMC_PLATFORM if "platform" in path else BMC_GLOBAL
            return mock.mock_open(read_data=data)()

        with mock.patch("{}.open".format(BUILTINS), side_effect=open_side_effect):
            result = device_info.get_bmc_data()
        assert result is not None
        assert result["bmc_addr"] == "169.254.0.1", "Platform bmc.json must take precedence"
        assert result["bmc_if_name"] == "usb0"

        # Platform bmc.json present, global absent – platform is returned
        def exists_platform(path):
            return path == "/usr/share/sonic/platform/bmc.json"

        mock_exists.side_effect = exists_platform
        with mock.patch("{}.open".format(BUILTINS),
                        mock.mock_open(read_data=BMC_PLATFORM)):
            result = device_info.get_bmc_data()
        assert result is not None
        assert result["bmc_addr"] == "169.254.0.1"
        assert result["bmc_if_name"] == "usb0"

        # Neither file present
        mock_exists.side_effect = lambda p: False
        result = device_info.get_bmc_data()
        assert result is None

    @mock.patch("sonic_py_common.device_info.get_bmc_data")
    def test_get_bmc_address(self, mock_bmc_data):
        BMC_DATA = {
            "bmc_if_name": "bmc0",
            "bmc_if_addr": "169.254.100.2",
            "bmc_addr": "169.254.100.1",
            "bmc_net_mask": "255.255.255.252"
        }

        mock_bmc_data.return_value = None
        assert device_info.get_bmc_address() is None

        mock_bmc_data.return_value = BMC_DATA
        assert device_info.get_bmc_address() == "169.254.100.1"

    @mock.patch("sonic_py_common.device_info.get_bmc_data")
    def test_get_switch_host_address(self, mock_bmc_data):
        BMC_DATA = {
            "bmc_if_name": "bmc0",
            "bmc_if_addr": "169.254.100.2",
            "bmc_addr": "169.254.100.1",
            "bmc_net_mask": "255.255.255.252"
        }

        mock_bmc_data.return_value = None
        assert device_info.get_switch_host_address() is None

        mock_bmc_data.return_value = BMC_DATA
        assert device_info.get_switch_host_address() == "169.254.100.2"

    @classmethod
    def teardown_class(cls):
        print("TEARDOWN")
