#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
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
from unittest import mock, TestCase
from pyfakefs.fake_filesystem_unittest import Patcher
sys.path.append('../')
from sdk_kernel_patches import *

MOCK_SLK_SERIES = """\
# Trtnetlink-catch-EOPNOTSUPP-errors.patch

# Mellanox patches for 5.10
###-> mellanox_sdk-start
###-> mellanox_sdk-end

###-> mellanox_hw_mgmt-start
0001-i2c-mlxcpld-Update-module-license.patch
###-> mellanox_hw_mgmt-end

0001-psample-Encapsulate-packet-metadata-in-a-struct.patch
0002-psample-Add-additional-metadata-attributes.patch
#####
"""

EXP_SLK_SERIES_STG1 = """\
# Trtnetlink-catch-EOPNOTSUPP-errors.patch

# Mellanox patches for 5.10
###-> mellanox_sdk-start
###-> mellanox_sdk-end

###-> mellanox_hw_mgmt-start
0001-i2c-mlxcpld-Update-module-license.patch
###-> mellanox_hw_mgmt-end

#####
"""

MOCK_SLK_SERIES_1 = """\
# Trtnetlink-catch-EOPNOTSUPP-errors.patch

# Mellanox patches for 5.10
###-> mellanox_sdk-start
0001-psample-Encapsulate-packet-metadata-in-a-struct.patch
###-> mellanox_sdk-end

###-> mellanox_hw_mgmt-start
0001-i2c-mlxcpld-Update-module-license.patch
###-> mellanox_hw_mgmt-end

#####
"""

MOCK_FINAL_SLK_SERIES = """\
# Trtnetlink-catch-EOPNOTSUPP-errors.patch

# Mellanox patches for 5.10
###-> mellanox_sdk-start
0001-psample-Encapsulate-packet-metadata-in-a-struct.patch
0002-psample-Add-additional-metadata-attributes.patch
0003-psample-define-the-macro-PSAMPLE_MD_EXTENDED_ATTR.patch
0004-new-sdk-driver-change-1.patch
0005-new-sdk-driver-change-1.patch
###-> mellanox_sdk-end

###-> mellanox_hw_mgmt-start
0001-i2c-mlxcpld-Update-module-license.patch
###-> mellanox_hw_mgmt-end

#####
"""

PTCH_LIST = ["0001-psample-Encapsulate-packet-metadata-in-a-struct.patch", \
              "0002-psample-Add-additional-metadata-attributes.patch"]

EXT_PTCH_LIST = ["0003-psample-define-the-macro-PSAMPLE_MD_EXTENDED_ATTR.patch", \
              "0004-new-sdk-driver-change-1.patch", \
              "0005-new-sdk-driver-change-1.patch"]

MOCK_SLK_VER = "5.10.104"

MOCK_README = """\
Copy the whole directory which contains this README to some folder on the machine where you plan to build the linux kernel,
then execute "./apply_patches_example.sh -d <your_linux_kernel_source_folder>".

Patch                                                                      Upstream-Commit                             From-Kernel-Version
==========================================================================================================================================
0001-psample-Encapsulate-packet-metadata-in-a-struct.patch                 a03e99d39f1943ec88f6fd3b0b9f34c20663d401    5.13
0002-psample-Add-additional-metadata-attributes.patch                      07e1a5809b595df6e125504dff6245cb2c8ed3de    5.13
0003-psample-define-the-macro-PSAMPLE_MD_EXTENDED_ATTR.patch               N/A                                         N/A
wrong-patch-name                                                           Whatevrr                                    rqrwf     vwvwvrvrv
==================================================  
"""

TEST_SLK_COMMIT = """\
Integrate SDK 4.5.1000 Kernel Patches

 ## Patch List
* 0001-psample-Encapsulate-packet-metadata-in-a-struct.patch : https://github.com/torvalds/linux/commit/a03e99d39f1943ec88f6fd3b0b9f34c20663d401
* 0002-psample-Add-additional-metadata-attributes.patch : https://github.com/torvalds/linux/commit/07e1a5809b595df6e125504dff6245cb2c8ed3de
* 0003-psample-define-the-macro-PSAMPLE_MD_EXTENDED_ATTR.patch : 
"""

LINES_READ = ""

def mock_sdk_args():
    with mock.patch("sys.argv", ["sdk_kernel_patches.py",
                                "--sonic_kernel_ver", MOCK_SLK_VER,
                                "--patches", "/tmp",
                                "--sdk_ver", "4.5.1000",
                                "--slk_msg", "/tmp/slk-commit-msg.hgsdhg",
                                "--build_root", "/sonic"]):
        parser = create_parser()
        return parser.parse_args()


def check_lists(exp, rec):
    print(" ------- Expected ----------- ")
    print("".join(exp))
    print("Size: {}".format(len(exp)))
    print(" ------- Recieved ----------- ")
    print("".join(rec))
    print("Size: {}".format(len(rec)))
    if len(exp) != len(rec):
        return False
    for i in range(0, len(exp)):
        if exp[i] != rec[i]:
            return False
    return True

def read_strip_mock(path, as_string=False):
    global LINES_READ
    return LINES_READ

class TestSDKAction(TestCase):
    def setUp(self):
        self.action = SDKAction(mock_sdk_args())
        self.action.check()
        self.path_kernel = os.path.join(KERNEL_BACKPORTS, "5.10")
        self.patcher = Patcher()
        self.patcher.setUp()

    def tearDown(self):
        Data.new_series = list()
        Data.new_patches = list()
        Data.old_series = list()
        Data.old_patches = list()
        Data.k_dir = ""
        Data.i_sdk_start = -1
        Data.i_sdk_end = -1
        self.patcher.tearDown()
        global LINES_READ 
        LINES_READ = ""

    def create_files(self, root_dir, lis):
        for file in lis:
            self.patcher.fs.create_file(os.path.join(root_dir, file))

    def test_get_kernel_dir_1(self):
        dir_ = os.path.join(self.path_kernel, MOCK_SLK_VER)
        self.patcher.fs.create_dir(os.path.join("/tmp", dir_))
        self.action.get_kernel_dir()
        assert dir_ == Data.k_dir
        
    def test_get_kernel_dir_2(self):
        dir_ = os.path.join(self.path_kernel, "5.10.99")
        self.patcher.fs.create_dir(os.path.join("/tmp", dir_))
        self.action.get_kernel_dir()
        assert dir_ == Data.k_dir

        dir_2 = os.path.join(self.path_kernel, "5.10.105")
        self.patcher.fs.create_dir(os.path.join("/tmp", dir_2))
        self.action.get_kernel_dir()
        assert dir_ == Data.k_dir

    def test_find_sdk_patches(self):
        Data.old_series = MOCK_SLK_SERIES.splitlines(True)
        self.action.refresh_markers()
        self.action.find_sdk_patches()
        assert "mellanox_sdk-start" in Data.old_series[Data.i_sdk_start]
        assert "mellanox_sdk-end" in Data.old_series[Data.i_sdk_end]
        assert not Data.old_patches

        Data.old_series = MOCK_SLK_SERIES_1.splitlines(True)
        self.action.refresh_markers()
        self.action.find_sdk_patches()
        assert "mellanox_sdk-start" in Data.old_series[Data.i_sdk_start]
        assert "mellanox_sdk-end" in Data.old_series[Data.i_sdk_end]
        assert len(Data.old_patches) == 1
        assert Data.old_patches[-1] == "0001-psample-Encapsulate-packet-metadata-in-a-struct.patch"

    def test_get_new_patches(self):
        root_dir = "/tmp/kernel_backports/5.10/5.10.27"
        self.create_files(root_dir, PTCH_LIST)
        self.action.get_kernel_dir()
        print(Data.k_dir)
        self.action.get_new_patches()
        assert len(Data.new_patches) == 2
        assert Data.new_patches[0] == "0001-psample-Encapsulate-packet-metadata-in-a-struct.patch"
        assert Data.new_patches[1] == "0002-psample-Add-additional-metadata-attributes.patch"

    def test_update_series(self):
        root_dir = "/tmp/kernel_backports/5.10/5.10.27"
        self.create_files(root_dir, PTCH_LIST)
        Data.old_series = MOCK_SLK_SERIES.splitlines(True)
        self.action.refresh_markers()
        self.action.find_sdk_patches()
        self.action.get_kernel_dir()
        self.action.get_new_patches()
        self.action.update_series()
        assert check_lists(EXP_SLK_SERIES_STG1.splitlines(True), Data.old_series)
        self.action.refresh_markers()
        assert "mellanox_sdk-start" in Data.old_series[Data.i_sdk_start]
        assert "mellanox_sdk-end" in Data.old_series[Data.i_sdk_end]

    def test_update_series_1(self):
        root_dir = "/tmp/kernel_backports/5.10/5.10.27"
        self.create_files(root_dir, PTCH_LIST)
        Data.old_series = MOCK_SLK_SERIES_1.splitlines(True)
        self.action.refresh_markers()
        self.action.find_sdk_patches()
        self.action.get_kernel_dir()
        self.action.get_new_patches()
        self.action.update_series()
        assert check_lists(EXP_SLK_SERIES_STG1.splitlines(True), Data.old_series)
        self.action.refresh_markers()
        assert "mellanox_sdk-start" in Data.old_series[Data.i_sdk_start]
        assert "mellanox_sdk-end" in Data.old_series[Data.i_sdk_end]

    def test_process_patches_1(self):
        root_dir = "/tmp/kernel_backports/5.10/5.10.27"
        self.create_files(root_dir, PTCH_LIST)
        self.create_files(root_dir, EXT_PTCH_LIST)
        Data.old_series = MOCK_SLK_SERIES.splitlines(True)
        self.action.refresh_markers()
        self.action.find_sdk_patches()
        self.action.get_kernel_dir()
        self.action.get_new_patches()
        self.action.update_series()
        self.action.refresh_markers()
        self.action.add_new_patch_series()
        assert check_lists(MOCK_FINAL_SLK_SERIES.splitlines(True), Data.new_series)

    def test_process_patches_2(self):
        root_dir = "/tmp/kernel_backports/5.10/5.10.27"
        self.create_files(root_dir, PTCH_LIST)
        self.create_files(root_dir, EXT_PTCH_LIST)
        Data.old_series = MOCK_SLK_SERIES_1.splitlines(True)
        self.action.refresh_markers()
        self.action.find_sdk_patches()
        self.action.get_kernel_dir()
        self.action.get_new_patches()
        self.action.update_series()
        self.action.refresh_markers()
        self.action.add_new_patch_series()
        assert check_lists(MOCK_FINAL_SLK_SERIES.splitlines(True), Data.new_series)

    @mock.patch('helper.FileHandler.read_strip', side_effect=read_strip_mock)
    def test_patch_table(self, mock_read_strip_mock):
        global LINES_READ 
        LINES_READ = MOCK_README.splitlines()
        table = self.action.fetch_patch_table("")
        assert "0001-psample-Encapsulate-packet-metadata-in-a-struct.patch" in table
        assert "0002-psample-Add-additional-metadata-attributes.patch" in table
        assert "0003-psample-define-the-macro-PSAMPLE_MD_EXTENDED_ATTR.patch" in table
        assert table["0001-psample-Encapsulate-packet-metadata-in-a-struct.patch"] == "a03e99d39f1943ec88f6fd3b0b9f34c20663d401"
        assert table["0002-psample-Add-additional-metadata-attributes.patch"] == "07e1a5809b595df6e125504dff6245cb2c8ed3de"
        assert table["0003-psample-define-the-macro-PSAMPLE_MD_EXTENDED_ATTR.patch"] == "N/A"
    
    @mock.patch('helper.FileHandler.read_strip', side_effect=read_strip_mock)
    def test_commit_msg(self, mock_read_strip_mock):
        global LINES_READ 
        LINES_READ = MOCK_README.splitlines()
        root_dir = "/tmp/kernel_backports/5.10/5.10.27"
        self.create_files(root_dir, PTCH_LIST)
        self.create_files(root_dir, ["0003-psample-define-the-macro-PSAMPLE_MD_EXTENDED_ATTR.patch"])
        Data.old_series = MOCK_SLK_SERIES.splitlines(True)
        self.action.refresh_markers()
        self.action.find_sdk_patches()
        self.action.get_kernel_dir()
        self.action.get_new_patches()
        table = self.action.fetch_patch_table("")
        msg = self.action.create_commit_msg(table)
        print(msg)
        assert check_lists(TEST_SLK_COMMIT.splitlines(True),  msg.splitlines(True))
