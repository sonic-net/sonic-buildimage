import sys
import shutil
from unittest import mock, TestCase
sys.path.append('../')
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
"""

TEST_SLK_COMMIT = """\
Intgerate HW-MGMT 7.0030.0937 Changes
 ## Patch List
* 0002-i2c-mlxcpld-Decrease-polling-time-for-performance-im.patch : https://github.com/gregkh/linux/commit/cb9744178f33
* 0003-i2c-mlxcpld-Add-support-for-I2C-bus-frequency-settin.patch : https://github.com/gregkh/linux/commit/66b0c2846ba8
* 0005-i2c-mux-mlxcpld-Move-header-file-out-of-x86-realm.patch : https://github.com/gregkh/linux/commit/98d29c410475
* 0006-i2c-mux-mlxcpld-Convert-driver-to-platform-driver.patch : https://github.com/gregkh/linux/commit/84af1b168c50
* 0007-i2c-mux-mlxcpld-Prepare-mux-selection-infrastructure.patch : https://github.com/gregkh/linux/commit/81566938083a
* 0008-i2c-mux-mlxcpld-Get-rid-of-adapter-numbers-enforceme.patch : https://github.com/gregkh/linux/commit/cae5216387d1
* 0009-i2c-mux-mlxcpld-Extend-driver-to-support-word-addres.patch : https://github.com/gregkh/linux/commit/c52a1c5f5db5
* 0010-i2c-mux-mlxcpld-Extend-supported-mux-number.patch : https://github.com/gregkh/linux/commit/699c0506543e
* 0011-i2c-mux-mlxcpld-Add-callback-to-notify-mux-creation-.patch : https://github.com/gregkh/linux/commit/a39bd92e92b9

"""


TEST_SB_COMMIT = """\
Intgerate HW-MGMT 7.0030.0937 Changes

"""

REL_INPUTS_DIR = "platform/mellanox/integration-scripts/tests/data/"
MOCK_INPUTS_DIR = "/sonic/" + REL_INPUTS_DIR
MOCK_WRITE_FILE = MOCK_INPUTS_DIR + "test_writer_file.out"

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
                                "--config_inclusion", MOCK_INPUTS_DIR+"/new_kconfig",
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
@mock.patch('helper.SLK_KCONFIG', REL_INPUTS_DIR+"kconfig-inclusion")
class TestHwMgmtPostAction(TestCase):
    def setUp(self):
        self.action = HwMgmtAction.get(mock_hwmgmt_args())
        self.action.read_data()
        # Populate the new_up, new_non_up list
        Data.new_up = NEW_UP_LIST.splitlines()
        Data.new_non_up = NEW_NONUP_LIST.splitlines()
        Data.old_series = FileHandler.read_raw(MOCK_INPUTS_DIR+"/series")
        all_kcfg = FileHandler.read_kconfig_parser(MOCK_INPUTS_DIR+"/kconfig-inclusions")
        Data.current_kcfg = []
        for hdr in HDRS:
            Data.current_kcfg.extend(all_kcfg.get(hdr, []))
        Data.current_kcfg = KCFG.parse_opts_strs(Data.current_kcfg)
        Data.kcfg_exclude = FileHandler.read_raw(MOCK_INPUTS_DIR+"/kconfig-exclusions")

    def tearDown(self):
        try:
            os.remove(MOCK_WRITE_FILE)
        except:
            pass

    def test_find_mlnx_hw_mgmt_markers(self):
        self.action.find_mlnx_hw_mgmt_markers()
        print(Data.i_mlnx_start, Data.i_mlnx_end)
        assert Data.old_series[Data.i_mlnx_start].strip() == "###-> mellanox_hw_mgmt-start"
        assert Data.old_series[Data.i_mlnx_end].strip() == "###-> mellanox_hw_mgmt-end"
    
    def test_check_kconfig_conflicts(self):
        # Add a line to create conflict
        print(Data.current_kcfg)
        Data.updated_kcfg.append(["CONFIG_EEPROM_OPTOE", "n"])
        self.action.find_mlnx_hw_mgmt_markers()
        assert self.action.check_kconfig_conflicts() == True

        # Add a duplicate option
        Data.updated_kcfg.pop(-1)
        Data.updated_kcfg.append(["CONFIG_EEPROM_OPTOE", "m"])
        assert self.action.check_kconfig_conflicts() == False

        # Check with no conflicts or duplicates
        Data.updated_kcfg.pop(-1)
        assert self.action.check_kconfig_conflicts() == False

    @mock.patch('helper.FileHandler.write_lines', side_effect=write_lines_mock)
    def test_write_final_slk_series(self, mock_write_lines):
        self.action.find_mlnx_hw_mgmt_markers()
        assert not self.action.check_kconfig_conflicts()
        self.action.write_final_slk_series()
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/series")

    def test_write_kconfig_inclusion(self):
        self.action.find_mlnx_hw_mgmt_markers()
        assert not self.action.check_kconfig_conflicts()
        print(Data.updated_kcfg)
        shutil.copy(MOCK_INPUTS_DIR+"/kconfig-inclusions", MOCK_WRITE_FILE)
        FileHandler.write_lines_marker(MOCK_WRITE_FILE, KCFG.get_writable_opts(Data.updated_kcfg), MLNX_KFG_MARKER)
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/kconfig-inclusions")
    
    @mock.patch('helper.FileHandler.write_lines', side_effect=write_lines_mock)
    def test_handle_exclusions(self, mock_write_lines):
        self.action.find_mlnx_hw_mgmt_markers()
        self.action.handle_exclusions()
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/kconfig-exclusions")
    
    @mock.patch('helper.FileHandler.write_lines', side_effect=write_lines_mock)
    def test_write_series_diff(self, mock_write_lines):
        self.action.find_mlnx_hw_mgmt_markers()
        self.action.write_final_slk_series()
        self.action.construct_series_with_non_up()
        self.action.write_series_diff()
        assert check_file_content(MOCK_INPUTS_DIR+"expected_data/series.patch")

    def test_commit_msg(self):
        table = load_patch_table(MOCK_INPUTS_DIR, "5.10.140")
        sb, slk = self.action.create_commit_msg(table)
        print(slk)
        print(TEST_SLK_COMMIT)
        assert slk.split() == TEST_SLK_COMMIT.split()
        assert sb.split() == TEST_SB_COMMIT.split()

