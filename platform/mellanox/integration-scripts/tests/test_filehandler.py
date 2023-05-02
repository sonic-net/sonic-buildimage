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

###-> mellanox-start
CONFIG_OF=y
CONFIG_THERMAL_OF=y
CONFIG_DW_DMAC_PCI=y
###-> mellanox-end

[armhf]
CONFIG_EEPROM_SFF_8436=m
CONFIG_EEPROM_OPTOE=m
CONFIG_I2C_MUX_GPIO=y
"""

UPDATED_MLNX_KCFG = """\
CONFIG_OF=y
CONFIG_THERMAL_OF=y
CONFIG_DW_DMAC_PCI=y
CONFIG_I2C_I801=m
CONFIG_PINCTRL=y
CONFIG_PINCTRL_INTEL=m
CONFIG_I2C_MUX_PCA954x=m
CONFIG_SPI_PXA2XX=m
"""

FINAL_MOCK_SLK_KCFG = """\
CONFIG_RANDOM=rrr

[common]
CONFIG_LOG_BUF_SHIFT=20

[amd64]
CONFIG_SENSORS_DPS1900=m

###-> mellanox-start
CONFIG_OF=y
CONFIG_THERMAL_OF=y
CONFIG_DW_DMAC_PCI=y
CONFIG_I2C_I801=m
CONFIG_PINCTRL=y
CONFIG_PINCTRL_INTEL=m
CONFIG_I2C_MUX_PCA954x=m
CONFIG_SPI_PXA2XX=m
###-> mellanox-end

[armhf]
CONFIG_EEPROM_SFF_8436=m
CONFIG_EEPROM_OPTOE=m
CONFIG_I2C_MUX_GPIO=y
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
    def test_read_kconfig_parser(self, mock_read_raw):
        global LINES_READ
        LINES_READ = MOCK_SLK_KCFG.split("\n")
        all_cfg = FileHandler.read_kconfig_parser("")
        print(all_cfg)
        assert all_cfg['no_parent'] == ['CONFIG_RANDOM=rrr']
        assert all_cfg['common'] == ['CONFIG_LOG_BUF_SHIFT=20']
        assert all_cfg['amd64'] == ['CONFIG_SENSORS_DPS1900=m', 'CONFIG_OF=y', 'CONFIG_THERMAL_OF=y', 'CONFIG_DW_DMAC_PCI=y']
        assert all_cfg['armhf'] == ['CONFIG_EEPROM_SFF_8436=m', 'CONFIG_EEPROM_OPTOE=m', 'CONFIG_I2C_MUX_GPIO=y']
    
    @mock.patch('helper.FileHandler.write_lines', side_effect=writer_mock)
    @mock.patch('helper.FileHandler.read_raw', side_effect=read_raw_mock)
    def test_write_lines_marker(self, mock_read_raw, mock_write_lines_marker):
        global LINES_READ
        global LINES_WRITE
        LINES_READ = MOCK_SLK_KCFG.splitlines(True)
        LINES_WRITE = FINAL_MOCK_SLK_KCFG.splitlines(True)

        list_opts = KCFG.parse_opts_strs(UPDATED_MLNX_KCFG.split("\n"))
        writable_opts = KCFG.get_writable_opts(list_opts)

        FileHandler.write_lines_marker("", writable_opts, marker="mellanox")

    @mock.patch('helper.FileHandler.write_lines', side_effect=writer_mock)
    @mock.patch('helper.FileHandler.read_raw', side_effect=read_raw_mock)
    def test_read_kconfig_inclusion(self, mock_read_raw, mock_write_lines_marker):
        global LINES_READ
        LINES_READ = FINAL_MOCK_SLK_KCFG.splitlines(True)
        opts = FileHandler.read_kconfig_inclusion("")

        global LINES_WRITE
        LINES_WRITE = UPDATED_MLNX_KCFG.splitlines()
        writable_opts = KCFG.get_writable_opts(KCFG.parse_opts_strs(opts))
        print(writable_opts)
        FileHandler.write_lines("", writable_opts)
