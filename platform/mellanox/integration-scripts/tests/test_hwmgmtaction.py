#
# Copyright (c) 2023-2024 NVIDIA CORPORATION & AFFILIATES.
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

import sys
import shutil
from unittest import mock, TestCase
from pyfakefs.fake_filesystem_unittest import Patcher
sys.path.append('../')
from hwmgmt_helper import *
from hwmgmt_kernel_patches import *


NEW_NONUP_LIST = """ \
0168-TMP-mlxsw-minimal-Ignore-error-reading-SPAD-register.patch
0169-TMP-mlxsw-i2c-Prevent-transaction-execution-for-spec.patch
0172-DS-platform-mlx-platform-Add-SPI-path-for-rack-switc.patch
0174-DS-mlxsw-core_linecards-Skip-devlink-and-provisionin.patch
"""

NEW_UP_LIST = """\
0001-i2c-mlxcpld-Update-module-license.patch
0002-i2c-mlxcpld-Decrease-polling-time-for-performance-im.patch
0003-i2c-mlxcpld-Add-support-for-I2C-bus-frequency-settin.patch
0004-i2c-mux-mlxcpld-Update-module-license.patch
0005-i2c-mux-mlxcpld-Move-header-file-out-of-x86-realm.patch
0006-i2c-mux-mlxcpld-Convert-driver-to-platform-driver.patch
0007-i2c-mux-mlxcpld-Prepare-mux-selection-infrastructure.patch
0008-i2c-mux-mlxcpld-Get-rid-of-adapter-numbers-enforceme.patch
0009-i2c-mux-mlxcpld-Extend-driver-to-support-word-addres.patch
0010-i2c-mux-mlxcpld-Extend-supported-mux-number.patch
0011-i2c-mux-mlxcpld-Add-callback-to-notify-mux-creation-.patch
0099-mlxsw-core_hwmon-Fix-variable-names-for-hwmon-attrib.patch
0100-mlxsw-core_thermal-Rename-labels-according-to-naming.patch
0101-mlxsw-core_thermal-Remove-obsolete-API-for-query-res.patch
0102-mlxsw-reg-Add-mgpir_-prefix-to-MGPIR-fields-comments.patch
0103-mlxsw-core-Remove-unnecessary-asserts.patch
0104-mlxsw-reg-Extend-MTMP-register-with-new-slot-number-.patch
0105-mlxsw-reg-Extend-MTBR-register-with-new-slot-number-.patch
0106-mlxsw-reg-Extend-MCIA-register-with-new-slot-number-.patch
0107-mlxsw-reg-Extend-MCION-register-with-new-slot-number.patch
0188-i2c-mux-Add-register-map-based-mux-driver.patch
"""

TEST_SLK_COMMIT = """\
Integrate HW-MGMT 7.0030.0937 Changes
 ## Patch List
* 0002-i2c-mlxcpld-Decrease-polling-time-for-performance-im.patch : https://github.com/torvalds/linux/commit/cb9744178f33
* 0003-i2c-mlxcpld-Add-support-for-I2C-bus-frequency-settin.patch : https://github.com/torvalds/linux/commit/66b0c2846ba8
* 0005-i2c-mux-mlxcpld-Move-header-file-out-of-x86-realm.patch : https://github.com/torvalds/linux/commit/98d29c410475
* 0006-i2c-mux-mlxcpld-Convert-driver-to-platform-driver.patch : https://github.com/torvalds/linux/commit/84af1b168c50
* 0007-i2c-mux-mlxcpld-Prepare-mux-selection-infrastructure.patch : https://github.com/torvalds/linux/commit/81566938083a
* 0008-i2c-mux-mlxcpld-Get-rid-of-adapter-numbers-enforceme.patch : https://github.com/torvalds/linux/commit/cae5216387d1
* 0009-i2c-mux-mlxcpld-Extend-driver-to-support-word-addres.patch : https://github.com/torvalds/linux/commit/c52a1c5f5db5
* 0010-i2c-mux-mlxcpld-Extend-supported-mux-number.patch : https://github.com/torvalds/linux/commit/699c0506543e
* 0011-i2c-mux-mlxcpld-Add-callback-to-notify-mux-creation-.patch : https://github.com/torvalds/linux/commit/a39bd92e92b9
* 0188-i2c-mux-Add-register-map-based-mux-driver.patch : https://patchwork.ozlabs.org/project/linux-i2c/patch/20230215195322.21955-1-vadimp@nvidia.com/
"""

TEST_SB_COMMIT = """\
Integrate HW-MGMT 7.0030.0937 Changes

"""

REL_INPUTS_DIR = "platform/mellanox/integration-scripts/tests/data/"
MOCK_INPUTS_DIR = "/sonic/" + REL_INPUTS_DIR
MOCK_WRITE_FILE = MOCK_INPUTS_DIR + "test_writer_file.out"
MOCK_KCFG_DIR = MOCK_INPUTS_DIR + "/kconfig"

def write_lines_mock(path, lines, raw=False):
    # Create the dir if it doesn't exist already
    with open(MOCK_WRITE_FILE, 'w') as f:
        for line in lines:
            if raw:
                f.write(f"{line}")
            else:
                f.write(f"{line}\n")

def mock_hwmgmt_args():
    with mock.patch("sys.argv", ["hwmgmt_kernel_patches.py", "post",
                                "--patches", "/tmp",
                                "--non_up_patches", "/tmp",
                                "--config_inc_amd", MOCK_KCFG_DIR+"/new_x86.config",
                                "--config_inc_arm", MOCK_KCFG_DIR+"/new_arm.config",
                                "--config_base_amd", MOCK_KCFG_DIR+"/x86.config",
                                "--config_base_arm", MOCK_KCFG_DIR+"/arm64.config",
                                "--config_inc_down_amd", MOCK_KCFG_DIR+"/new_x86_down.config",
                                "--config_inc_down_arm", MOCK_KCFG_DIR+"/new_arm_down.config",
                                "--series", MOCK_INPUTS_DIR+"/new_series",
                                "--current_non_up_patches", MOCK_INPUTS_DIR+"/hwmgmt_nonup_patches",
                                "--kernel_version", "5.10.140",
                                "--hw_mgmt_ver", "7.0030.0937",
                                "--sb_msg", "/tmp/sb_msg.log",
                                "--slk_msg", "/tmp/slk_msg.log",
                                "--build_root", "/sonic", 
                                "--is_test"]):
        parser = create_parser()
        return parser.parse_args()

def check_file_content(path):
    list1 = FileHandler.read_raw(MOCK_WRITE_FILE)
    list2 = FileHandler.read_raw(path)
    for i in range(0, len(list1)):
        if list1[i] != list2[i]:
            print("--- {}\n--- {}".format(list1[i], list2[i]))
            return False
    return True

@mock.patch('helper.SLK_PATCH_LOC', REL_INPUTS_DIR)
@mock.patch('helper.SLK_SERIES', REL_INPUTS_DIR+"series")
@mock.patch('hwmgmt_helper.SLK_KCONFIG', REL_INPUTS_DIR+"common_kconfig")
@mock.patch('hwmgmt_helper.SLK_KCONFIG_AMD64', REL_INPUTS_DIR+"amd64_kconfig")
@mock.patch('hwmgmt_helper.SLK_KCONFIG_ARM64', REL_INPUTS_DIR+"arm64-mellanox_kconfig")
class TestHwMgmtPostAction(TestCase):
    def setUp(self):
        self.action = HwMgmtAction.get(mock_hwmgmt_args())
        self.action.read_data()
        self.kcfgaction = KConfigTask(mock_hwmgmt_args())
        self.kcfgaction.read_data()
        # Populate the new_up, new_non_up list
        Data.new_up = NEW_UP_LIST.splitlines()
        Data.new_non_up = NEW_NONUP_LIST.splitlines()
        Data.old_series = FileHandler.read_raw(MOCK_INPUTS_DIR+"/series")

    def tearDown(self):
        KCFGData.x86_incl.clear()
        KCFGData.arm_incl.clear()
        KCFGData.x86_excl.clear()
        KCFGData.arm_excl.clear()
        KCFGData.x86_down.clear()
        KCFGData.arm_down.clear()
        KCFGData.noarch_incl.clear()
        KCFGData.noarch_excl.clear()
        KCFGData.noarch_down.clear()

    def test_find_mlnx_hw_mgmt_markers(self):
        self.action.find_mlnx_hw_mgmt_markers()
        print(Data.i_mlnx_start, Data.i_mlnx_end)
        assert Data.old_series[Data.i_mlnx_start].strip() == "###-> mellanox_hw_mgmt-start"
        assert Data.old_series[Data.i_mlnx_end].strip() == "###-> mellanox_hw_mgmt-end"

    @mock.patch('helper.FileHandler.write_lines', side_effect=write_lines_mock)
    def test_write_final_slk_series(self, mock_write_lines):
        self.action.find_mlnx_hw_mgmt_markers()
        self.action.write_final_slk_series()
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/series")
    
    @mock.patch('helper.FileHandler.write_lines', side_effect=write_lines_mock)
    def test_write_final_diff(self, mock_write_lines):
        self.action.find_mlnx_hw_mgmt_markers()
        self.action.write_final_slk_series()
        self.action.construct_series_with_non_up()
        series_diff = self.action.get_series_diff()
        kcfg_diff = self._get_kcfg_diff()
        final_diff = self.action.get_merged_diff(series_diff, kcfg_diff)
        self.action.write_non_up_diff(series_diff, kcfg_diff)
        print("".join(final_diff))
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/external-changes.patch")

    def test_commit_msg(self):
        self.action.find_mlnx_hw_mgmt_markers()
        root_dir = "/sonic/" + PATCH_TABLE_LOC + PATCHWORK_LOC.format("5.10")
        content = "patchwork_link: https://patchwork.ozlabs.org/project/linux-i2c/patch/20230215195322.21955-1-vadimp@nvidia.com/\n"
        file = "0188-i2c-mux-Add-register-map-based-mux-driver.patch.txt"
        table = load_patch_table(MOCK_INPUTS_DIR, "5.10")
        with Patcher() as patcher:
            patcher.fs.create_file(os.path.join(root_dir, file), contents=content)
            sb, slk = self.action.create_commit_msg(table)
            print(slk)
            print(TEST_SLK_COMMIT)
            assert slk.split() == TEST_SLK_COMMIT.split()
            assert sb.split() == TEST_SB_COMMIT.split()

    def _parse_inc_excl(self):
        KCFGData.x86_incl, KCFGData.x86_excl = self.kcfgaction.parse_inc_exc(KCFGData.x86_base, KCFGData.x86_updated)
        KCFGData.arm_incl, KCFGData.arm_excl = self.kcfgaction.parse_inc_exc(KCFGData.arm_base, KCFGData.arm_updated)

    def _parse_noarch_inc_excl(self):
        self._parse_inc_excl()
        self.kcfgaction.parse_noarch_inc_exc()
    
    def _get_kcfg_upstream(self):
        self._parse_noarch_inc_excl()
        return self.kcfgaction.get_upstream_kconfig()

    def _get_kcfg_diff(self):
        common, amd64, arm64 = self._get_kcfg_upstream()
        return self.kcfgaction.get_downstream_kconfig_diff(common, amd64, arm64)

    def test_parse_inc_excl(self):
        self._parse_inc_excl()
        test_x86_incl = OrderedDict({
            "CONFIG_THERMAL" : "y",
            "CONFIG_THERMAL_OF" : "y",
            "CONFIG_PINCTRL" : "y",
            "CONFIG_DW_DMAC_PCI" : "y",
            "CONFIG_TI_ADS1015" : "m",
            "CONFIG_I2C_DESIGNWARE_CORE" : "m",
            "CONFIG_I2C_DESIGNWARE_PCI" : "m"
        })
        test_arm_incl = OrderedDict({
            "CONFIG_THERMAL" : "y",
            "CONFIG_THERMAL_OF" : "y",
            "CONFIG_MELLANOX_PLATFORM" : "y",
            "CONFIG_THERMAL_WRITABLE_TRIPS" : "y",
            "CONFIG_PMBUS" : "m",
            "CONFIG_SENSORS_PMBUS" : "m",
            "CONFIG_HWMON" : "y",
            "CONFIG_OF" : "y",
            "CONFIG_THERMAL_NETLINK" : "y"
        })
        test_x86_excl = OrderedDict({"CONFIG_I2C_DESIGNWARE_BAYTRAIL" : "y"})
        assert KCFGData.x86_incl == test_x86_incl
        assert KCFGData.x86_excl == test_x86_excl
        assert KCFGData.arm_incl == test_arm_incl

    def test_parse_inc_excl_noarch(self):
        self._parse_noarch_inc_excl()
        test_x86_incl = OrderedDict({
            "CONFIG_PINCTRL" : "y",
            "CONFIG_DW_DMAC_PCI" : "y",
            "CONFIG_TI_ADS1015" : "m",
            "CONFIG_I2C_DESIGNWARE_CORE" : "m",
            "CONFIG_I2C_DESIGNWARE_PCI" : "m"
        })
        test_arm_incl = OrderedDict({
            "CONFIG_MELLANOX_PLATFORM" : "y",
            "CONFIG_THERMAL_WRITABLE_TRIPS" : "y",
            "CONFIG_PMBUS" : "m",
            "CONFIG_SENSORS_PMBUS" : "m",
            "CONFIG_HWMON" : "y",
            "CONFIG_OF" : "y",
            "CONFIG_THERMAL_NETLINK" : "y"
        })
        test_noarch_incl = OrderedDict({
            "CONFIG_THERMAL" : "y",
            "CONFIG_THERMAL_OF" : "y"
        })
        assert KCFGData.x86_incl == test_x86_incl
        assert KCFGData.noarch_incl == test_noarch_incl
        assert KCFGData.arm_incl == test_arm_incl

    @mock.patch('helper.FileHandler.write_lines', side_effect=write_lines_mock)
    def test_kcfg_incl_file(self, mock_write_lines_mock):
        common, amd64, arm64 = self._get_kcfg_upstream()
        FileHandler.write_lines("", common, True)
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/common_kconfig")
        FileHandler.write_lines("", amd64, True)
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/amd64_kconfig")
        FileHandler.write_lines("", arm64, True)
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/arm64-mellanox_kconfig")
