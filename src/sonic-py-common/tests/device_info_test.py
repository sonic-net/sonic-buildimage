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
                     "revision": SonicV2Connector.TEST_REV}
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

    @mock.patch("sonic_py_common.device_info.get_platform_info")
    def test_is_chassis(self, mock_platform_info):
        mock_platform_info.return_value = {"switch_type": "npu"}
        assert device_info.is_chassis() == False
        assert device_info.is_voq_chassis() == False
        assert device_info.is_packet_chassis() == False

        mock_platform_info.return_value = {"switch_type": "voq"}
        assert device_info.is_voq_chassis() == True
        assert device_info.is_packet_chassis() == False
        assert device_info.is_chassis() == True

        mock_platform_info.return_value = {"switch_type": "chassis-packet"}
        assert device_info.is_voq_chassis() == False
        assert device_info.is_packet_chassis() == True
        assert device_info.is_chassis() == True

        mock_platform_info.return_value = {}
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
        assert mock_sonic_ver.called_once()
        assert mock_machine_info.called_once()
        assert mock_hwsku.called_once()
        mock_cfg_inst.get_table.assert_called_once_with("DEVICE_METADATA")

    @classmethod
    def teardown_class(cls):
        print("TEARDOWN")
