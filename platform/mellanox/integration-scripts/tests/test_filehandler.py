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
from unittest import mock
sys.path.append('../')
from helper import *

MOCK_SLK_SERIES = """\
# Trtnetlink-catch-EOPNOTSUPP-errors.patch
# Tbridge-per-port-multicast-broadcast-flood-flags.patch
#

# Mellanox patches for 5.10
###-> mellanox_sdk-start
###-> mellanox_sdk-end

###-> mellanox_hw_mgmt-start
0001-i2c-mlxcpld-Update-module-license.patch
0002-i2c-mlxcpld-Decrease-polling-time-for-performan.patch
0003-i2c-mlxcpld-Add-support-for-I2C-bus-frequency-s.patch
0004-i2c-mux-mlxcpld-Update-module-license.patch
###-> mellanox_hw_mgmt-end
"""

MOCK_SLK_KCFG = """\
CONFIG_RANDOM=rrr

[common]
CONFIG_LOG_BUF_SHIFT=20

[amd64]
CONFIG_SENSORS_DPS1900=m

###-> mellanox_amd64-start
CONFIG_OF=y
CONFIG_THERMAL_OF=y
CONFIG_DW_DMAC_PCI=y
###-> mellanox_amd64-end

[armhf]
CONFIG_EEPROM_SFF_8436=m
CONFIG_EEPROM_OPTOE=m
CONFIG_I2C_MUX_GPIO=y
"""

FINAL_MOCK_SLK_KCFG = """\
CONFIG_RANDOM=rrr

[common]
CONFIG_LOG_BUF_SHIFT=20

[amd64]
CONFIG_SENSORS_DPS1900=m

###-> mellanox_amd64-start
CONFIG_OF=y
CONFIG_THERMAL_OF=y
CONFIG_DW_DMAC_PCI=y
CONFIG_I2C_I801=m
CONFIG_PINCTRL=y
CONFIG_PINCTRL_INTEL=m
CONFIG_I2C_MUX_PCA954x=m
CONFIG_SPI_PXA2XX=m
###-> mellanox_amd64-end

[armhf]
CONFIG_EEPROM_SFF_8436=m
CONFIG_EEPROM_OPTOE=m
CONFIG_I2C_MUX_GPIO=y
"""

MOCK_SLK_EXCL = """\
[common]
CONFIG_CGROUP_NET_CLASSID
CONFIG_NET_CLS_CGROUP
CONFIG_NETFILTER_XT_MATCH_CGROUP
CONFIG_CGROUP_NET_PRIO

[amd64]
# Unset X86_PAT according to Broadcom's requirement
CONFIG_X86_PAT
CONFIG_MLXSW_PCI
###-> mellanox_amd64-start
###-> mellanox_amd64-end

[arm64]
CONFIG_SQUASHFS_DECOMP_MULTI_PERCPU
"""

FINAL_MOCK_SLK_EXCL = """\
[common]
CONFIG_CGROUP_NET_CLASSID
CONFIG_NET_CLS_CGROUP
CONFIG_NETFILTER_XT_MATCH_CGROUP
CONFIG_CGROUP_NET_PRIO

[amd64]
# Unset X86_PAT according to Broadcom's requirement
CONFIG_X86_PAT
CONFIG_MLXSW_PCI
###-> mellanox_amd64-start
CONFIG_OF
CONFIG_THERMAL_OF
###-> mellanox_amd64-end

[arm64]
CONFIG_SQUASHFS_DECOMP_MULTI_PERCPU
"""

LINES_WRITE = []
LINES_READ = []

def writer_mock(path, lines, raw=False):
    global LINES_WRITE
    if raw:
        join_with = ""
    else:
        join_with = "\n"
    print("Expected: ")
    print(join_with.join(LINES_WRITE))
    print("Recieved:")
    print(join_with.join(lines))
    assert LINES_WRITE == lines

def read_raw_mock(path):
    global LINES_READ
    return LINES_READ

class TestFilehandler:
    def test_find_markers(self):
        lines = MOCK_SLK_SERIES.split("\n")
        print(lines)
        i_start, i_end = FileHandler.find_marker_indices(lines, "mellanox_hw_mgmt")
        print(i_start, i_end)
        assert lines[i_start] == "###-> mellanox_hw_mgmt-start"
        assert lines[i_end] == "###-> mellanox_hw_mgmt-end"

        i_start, i_end = FileHandler.find_marker_indices(lines, "mellanox_sdk")
        print(i_start, i_end)
        assert lines[i_start] == "###-> mellanox_sdk-start"
        assert lines[i_end] == "###-> mellanox_sdk-end"

        i_start, i_end = FileHandler.find_marker_indices(lines, "whatevrr")
        print(i_start, i_end)
        assert i_start == -1
        assert i_end == len(lines)

        i_start, i_end = FileHandler.find_marker_indices(lines)
        print(i_start, i_end)
        assert i_start == -1
        assert i_end == len(lines)

    @mock.patch('helper.FileHandler.read_raw', side_effect=read_raw_mock)
    def test_insert_kcfg(self, mock_read_raw):
        global LINES_READ
        LINES_READ = MOCK_SLK_KCFG.splitlines(True)
        kcfg_inc_raw = FileHandler.read_raw("")
        new_opts = OrderedDict({
                "CONFIG_OF" : "y",
                "CONFIG_THERMAL_OF" : "y",
                "CONFIG_DW_DMAC_PCI" : "y",
                "CONFIG_I2C_I801" : "m",
                "CONFIG_PINCTRL" : "y",
                "CONFIG_PINCTRL_INTEL" : "m",
                "CONFIG_I2C_MUX_PCA954x" : "m",
                "CONFIG_SPI_PXA2XX" : "m"
        })
        x86_start, x86_end = FileHandler.find_marker_indices(kcfg_inc_raw, MLNX_KFG_MARKER)
        assert "###-> mellanox_amd64-start" in kcfg_inc_raw[x86_start]
        assert "###-> mellanox_amd64-end" in kcfg_inc_raw[x86_end]
        final_kcfg = FileHandler.insert_kcfg_data(kcfg_inc_raw, x86_start, x86_end, new_opts)
        assert final_kcfg == FINAL_MOCK_SLK_KCFG.splitlines(True)
