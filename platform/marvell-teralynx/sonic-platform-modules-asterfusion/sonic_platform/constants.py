#####################################################################
# Asterfusion CX-N Devices Platform Constants                       #
#                                                                   #
# Module contains an implementation of SONiC Platform Constants and #
# provides the platform information                                 #
#                                                                   #
#####################################################################

try:
    import copy as _copy
    import json as _json

    from collections import OrderedDict as _OrderedDict
    from itertools import chain as _chain, product as _product
    from pathlib import Path as _Path

    from sonic_py_common.device_info import get_path_to_platform_dir
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


################################
#             MISC             #
################################
def singleton(cls):
    _instance = {}

    def _singleton(*args, **kargs):
        if cls not in _instance:
            _instance[cls] = cls(*args, **kargs)
        return _instance[cls]

    return _singleton


def _get_pddf_support():
    pddf_module_loaded = (
        len(
            list(
                filter(
                    lambda x: "pddf" in x,
                    _Path("/proc/modules").read_text().splitlines(),
                )
            )
        )
        > 0
    )
    pddf_support_flag = (_Path(get_path_to_platform_dir()) / "pddf_support").exists()
    return pddf_module_loaded and pddf_support_flag


PDDF_SUPPORT = _get_pddf_support()
LOGGING_CONFIG_NAME = "logging.conf"
LOGGING_STACK_INFO = False
NOT_AVAILABLE = "N/A"
PDDF_DEVICE_JSON_PATH = "/usr/share/sonic/platform/pddf/pddf-device.json"
PLATFORM_INSTALL_JSON_PATH = "/etc/sonic/platform_install.json"
SYSFS_HWMON_SUBSYS_DIR = "/sys/class/hwmon"
SYSFS_MATCH_TYPE_VALUE = "value"
SYSFS_MATCH_TYPE_EXACT = "exact"
SYSFS_MATCH_TYPE_FUZZY = "fuzzy"


################################
#            HWSKU             #
################################
HWSKU_CX308P48YN = "CX308P-48Y-N"
HWSKU_CX532PN = "CX532P-N"
HWSKU_CX564PN = "CX564P-N"
HWSKU_CX664DN = "CX664D-N"
HWSKU_CX732QN = "CX732Q-N"
HWSKU_CX864EN = "CX864E-N"
HWSKU_CX864EN_128X400G = "CX864E-N_128X400G"
HWSKU_CX864EN_64X400G_32X800G = "CX864E-N_64X400G_32X800G"


################################
#           PLATFORM           #
################################
PLTFM_CX308P48YN = "x86_64-asterfusion_cx308p_48y_n-r0"
PLTFM_CX532PN = "x86_64-asterfusion_cx532p_n-r0"
PLTFM_CX564PN = "x86_64-asterfusion_cx564p_n-r0"
PLTFM_CX664DN = "x86_64-asterfusion_cx664d_n-r0"
PLTFM_CX732QN = "x86_64-asterfusion_cx732q_n-r0"
PLTFM_CX864EN = "x86_64-asterfusion_cx864e_n-r0"


################################
#         ASIC & COMe          #
################################
ASIC_TL05E00 = "TL05E00"
ASIC_TL05E01 = "TL05E01"
ASIC_TL05E02 = "TL05E02"
ASIC_TL05E03 = "TL05E03"
ASIC_TL07E01 = "TL07E01"
ASIC_TL07E02 = "TL07E02"
ASIC_TL07E03 = "TL07E03"
ASIC_TL10E02 = "TL10E02"
ASIC_TL10E04 = "TL10E04"
ASIC_FL00E01 = "FL00E01"
ASIC_FL00E02 = "FL00E02"
ASIC_FL00E03 = "FL00E03"

# All available combinations for TL:
# CX308P-48Y-N_TL05E00
# CX308P-48Y-N_TL05E01
# CX308P-48Y-N_TL05E02
# CX308P-48Y-N_TL05E03
# CX532P-N_TL05E00
# CX532P-N_TL07E01
# CX532P-N_TL07E02
# CX532P-N_TL07E03
# CX564P-N_TL07E01
# CX564P-N_TL07E02
# CX564P-N_TL07E03
# CX664D-N_TL07E01
# CX664D-N_TL07E02
# CX664D-N_TL07E03
# CX732Q-N_TL07E01
# CX732Q-N_TL07E02
# CX732Q-N_TL07E03
# CX864E-N_TL10E02
# CX864E-N_TL10E04

# All available combinations for FL:
# CX308P-48Y-N_FL00E01
# CX308P-48Y-N_FL00E02
# CX308P-48Y-N_FL00E03
# CX532P-N_FL00E02
# CX532P-N_FL00E03


################################
#            EEPROM            #
################################
EEPROM_I2C_SYSFS_PATH = "/sys/bus/i2c/devices/0-0056/eeprom"
EEPROM_CACHE_PATH = "/var/cache/sonic/decode-syseeprom/syseeprom_cache"
EEPROM_UART_FUZZY_MATCH_DIR = SYSFS_HWMON_SUBSYS_DIR
EEPROM_UART_SYSFS_DIR = "device/SYS_EEPROM"
EEPROM_UART_SYSFS_NAME_LIST = [
    "product_name",  # 0x21
    "part_number",  # 0x22
    "serial_number",  # 0x23
    "base_mac_address",  # 0x24
    "manufacture_data",  # 0x25
    "device_version",  # 0x26
    "lable_revision",  # 0x27
    "platform_name",  # 0x28
    "onie_version",  # 0x29
    "mac_address",  # 0x2A
    "manufacturer",  # 0x2B
    "country_code",  # 0x2C
    "vendor_name",  # 0x2D
    "diag_version",  # 0x2E
    "service_tag",  # 0x2F
    "switch_verdor",  # 0x30
    "main_board_version",  # 0x31
    "come_version",  # 0x32
    "ghc0_board_version",  # 0x33
    "ghc1_board_version",  # 0x34
    "eeprom_crc32",  # 0xFE
]
EEPROM_CODE_LIST = [
    b"\x21",  # Product Name
    b"\x22",  # Part Number
    b"\x23",  # Serial Number
    b"\x24",  # Base MAC Address
    b"\x25",  # Manufacture Date
    b"\x26",  # Device Version
    b"\x27",  # Label Revision
    b"\x28",  # Platform Name
    b"\x29",  # ONIE Version
    b"\x2a",  # MAC Addresses
    b"\x2b",  # Manufacturer
    b"\x2c",  # Manufacture Country
    b"\x2d",  # Vendor Name
    b"\x2e",  # Diag Version
    b"\x2f",  # Service Tag
    b"\x30",  # Switch ASIC Vendor
    b"\x31",  # Main Board Version
    b"\x32",  # COME Version
    b"\x33",  # GHC-0 Board Version
    b"\x34",  # GHC-1 Board Version
    b"\xfe",  # CRC-32
]
EEPROM_FIELD_CODE_MAP = list(zip(EEPROM_UART_SYSFS_NAME_LIST, EEPROM_CODE_LIST))

# TlvInfo Header:
#    Id String:    TlvInfo
#    Version:      1
#    Total Length: 169
# TLV Name             Code    Len    Value
# -------------------  ------  -----  ----------------------
# Product Name         0x21    11     CX532P-NT-AC
# Part Number          0x22    7      ONBP1U-N-2X32C-S-AC
# Serial Number        0x23    8      F023542A074
# Base MAC Address     0x24    6      60:EB:5A:01:14:E9
# Manufacture Date     0x25    19     24/06/2024 11:22:42
# Device Version       0x26    1      1
# Label Revision       0x27    1      0
# Platform Name        0x28    30     x86_64-asterfusion_cx532p_n-r0
# ONIE Version         0x29    10     2019.05_v1.0.6
# MAC Addresses        0x2A    2      2
# Manufacturer         0x2B    11     Asterfusion
# Manufacture Country  0x2C    2      CN
# Vendor Name          0x2D    11     Asterfusion
# Diag Version         0x2E    3      1.0
# Service Tag          0x2F    1      X
# Switch ASIC Vendor   0x30    1      Marvell                         * IGNORED CODE *
# Main Board Version   0x31    1      APNS1280N-AB1-V1.0-221000074    * IGNORED CODE *
# COME Version         0x32    1      CME5008-16GB-HH-ADV             * IGNORED CODE *
# GHC-0 Board Version  0x33    1                                      * IGNORED CODE *
# GHC-1 Board Version  0x34    1                                      * IGNORED CODE *
# CRC-32               0xFE    4      0x573024A5


COMPONENT_INFO = {}  # type: dict[str, dict[str, list[dict[str, str|dict[str, str]]]]]
FAN_SPEED_TOLERANCE = 10
FAN_INFO = (
    {}
)  # type: dict[str, dict[str, list[dict[str, str|dict[str, str|tuple[str, str]]]]]]
FAN_DRAWER_INFO = {}  # type: dict[str, dict[str, list[dict[str, str]]]]
PSU_INFO = {}  # type: dict[str, dict[str, list[dict[str, float|str|dict[str, str]]]]]
THERMAL_TEMP_HIGH = 85.0
THERMAL_TEMP_CRIT_HIGH = 105.0
THERMAL_TEMP_LOW = 0.0
THERMAL_TEMP_CRIT_LOW = 0.0
THERMAL_INFO_TEMPLATE = {
    "name": NOT_AVAILABLE,
    "tscale": 1000.0,
    "presence": {
        "type": SYSFS_MATCH_TYPE_VALUE,
        "value": "present",
        "cmp": "present",
    },
    "high": {
        "type": SYSFS_MATCH_TYPE_VALUE,
        "value": THERMAL_TEMP_HIGH,
    },
    "chigh": {
        "type": SYSFS_MATCH_TYPE_VALUE,
        "value": THERMAL_TEMP_CRIT_HIGH,
    },
    "low": {
        "type": SYSFS_MATCH_TYPE_VALUE,
        "value": THERMAL_TEMP_LOW,
    },
    "clow": {
        "type": SYSFS_MATCH_TYPE_VALUE,
        "value": THERMAL_TEMP_CRIT_LOW,
    },
}  # type: dict[str, float|str|dict[str, float|str]]]
THERMAL_INFO = (
    {}
)  # type: dict[str, dict[str, list[dict[str, float|str|dict[str, float|str]]]]]
VOLTAGE_INFO = (
    {}
)  # type: dict[str, dict[str, list[dict[str, float|str|dict[str, str]]]]]
LPMODE_UNSUPPORTED = 2
LPMODE_FAILURE = 1
LPMODE_SUCCESS = 0
LPMODE_OFF = 1
LPMODE_ON = 0
RESET_FAILURE = 1
RESET_SUCCESS = 0
ERROR_DESCRIPTION_OK = "OK"
ERROR_DESCRIPTION_UNPLUGGED = "Unplugged"
SFP_INFO = {}  # type: dict[str, dict[str, list[dict[str, str|dict[str, str]]]]]

if PDDF_SUPPORT:
    PDDF_DEVICE_DATA = _json.loads(_Path(PDDF_DEVICE_JSON_PATH).read_text())
    _sys_sysfs_dir = _Path("/sys/bus/i2c/devices") / "{}-{:04x}".format(
        int(
            PDDF_DEVICE_DATA.get("SYS1", {})
            .get("uart", {})
            .get("topo_info", {})
            .get("parent_bus"),
            16,
        ),
        int(
            PDDF_DEVICE_DATA.get("SYS1", {})
            .get("uart", {})
            .get("topo_info", {})
            .get("dev_addr"),
            16,
        ),
    )

    ################################
    #            EEPROM            #
    ################################
    _eeprom_sysfs_dir = _Path("/sys/bus/i2c/devices") / "{}-{:04x}".format(
        int(
            PDDF_DEVICE_DATA.get("EEPROM1", {})
            .get("uart", {})
            .get("topo_info", {})
            .get("parent_bus"),
            16,
        ),
        int(
            PDDF_DEVICE_DATA.get("EEPROM1", {})
            .get("uart", {})
            .get("topo_info", {})
            .get("dev_addr"),
            16,
        ),
    )
    EEPROM_UART_FUZZY_MATCH_DIR = _eeprom_sysfs_dir.parent.as_posix()
    EEPROM_UART_SYSFS_DIR = _eeprom_sysfs_dir.name

    ################################
    #          COMPONENT           #
    ################################

    ################
    # CX308P-48Y-N #
    ################
    COMPONENT_INFO[HWSKU_CX308P48YN] = {}
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E00] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "System Management",
        },
        {
            "name": "CPLD2",
            "desc": "SFP28 Ports (Y1 - Y32) Management",
        },
        {
            "name": "CPLD3",
            "desc": "SFP28 Ports (Y33 - Y48), QSFP28 Ports (C1 - C8) Management",
        },
        {
            "name": "CPLD4",
            "desc": "Fan Management",
        },
        {
            "name": "CPLD5",
            "desc": "Power Management",
        },
    ]
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bmc_version").as_posix(),
            },
        },
        {
            "name": "CPLD1",
            "desc": "SFP28 Ports (Y33 - Y48), QSFP28 Ports (C1 - C8) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld1_version").as_posix(),
            },
        },
        {
            "name": "CPLD2",
            "desc": "SFP28 Ports (Y1 - Y32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld2_version").as_posix(),
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_FL00E01] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_FL00E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_FL00E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )

    ############
    # CX532P-N #
    ############
    COMPONENT_INFO[HWSKU_CX532PN] = {}
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL05E00] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "System Management",
        },
        {
            "name": "CPLD2",
            "desc": "QSFP28 Ports (C1 - C16) Management",
        },
        {
            "name": "CPLD3",
            "desc": "QSFP28 Ports (C17 - C32) Management",
        },
        {
            "name": "CPLD4",
            "desc": "Fan Management",
        },
        {
            "name": "CPLD5",
            "desc": "Power Management",
        },
    ]
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bmc_version").as_posix(),
            },
        },
        {
            "name": "CPLD1",
            "desc": "QSFP28 Ports (C1 - C16) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld1_version").as_posix(),
            },
        },
        {
            "name": "CPLD2",
            "desc": "SFP+ Ports (X1 - X2), QSFP28 Ports (C17 - C32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld2_version").as_posix(),
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_FL00E02] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bmc_version").as_posix(),
            },
        },
        {
            "name": "CPLD1",
            "desc": "SFP+ Ports (X1 - X2), QSFP28 Ports (C13 - C32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld1_version").as_posix(),
            },
        },
        {
            "name": "CPLD2",
            "desc": "QSFP28 Ports (C1 - C12) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld2_version").as_posix(),
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_FL00E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX532PN][ASIC_FL00E02]
    )

    ############
    # CX564P-N #
    ############
    COMPONENT_INFO[HWSKU_CX564PN] = {}
    COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bmc_version").as_posix(),
            },
        },
        {
            "name": "CPLD1",
            "desc": "QSFP28 Ports (C1 - C32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld1_version").as_posix(),
            },
        },
        {
            "name": "CPLD2",
            "desc": "SFP+ Ports (X1 - X2), QSFP28 Ports (C33 - C64) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld2_version").as_posix(),
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )

    ############
    # CX664D-N #
    ############
    COMPONENT_INFO[HWSKU_CX664DN] = {}
    COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bmc_version").as_posix(),
            },
        },
        {
            "name": "CPLD1",
            "desc": "QSFP56 Ports (D1 - D32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld1_version").as_posix(),
            },
        },
        {
            "name": "CPLD2",
            "desc": "SFP+ Ports (X1 - X2), QSFP56 Ports (D33 - D64) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld2_version").as_posix(),
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )

    ############
    # CX732Q-N #
    ############
    COMPONENT_INFO[HWSKU_CX732QN] = {}
    COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bmc_version").as_posix(),
            },
        },
        {
            "name": "CPLD1",
            "desc": "SFP+ Ports (X1 - X2), QSFPDD Ports (QC1 - QC32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld1_version").as_posix(),
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )

    ############
    # CX864E-N #
    ############
    COMPONENT_INFO[HWSKU_CX864EN] = {}
    COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bmc_version").as_posix(),
            },
        },
        {
            "name": "BOM1",
            "desc": "",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bom1_version").as_posix(),
            },
        },
        {
            "name": "BOM2",
            "desc": "",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "bom2_version").as_posix(),
            },
        },
        {
            "name": "CPLD1",
            "desc": "SFP+ Ports (X1 - X2), OSFP Ports (E1 - E64) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld1_version").as_posix(),
            },
        },
        {
            "name": "CPLD2",
            "desc": "System Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld2_version").as_posix(),
            },
        },
        {
            "name": "CPLD3",
            "desc": "System Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld3_version").as_posix(),
            },
        },
        {
            "name": "CPLD4",
            "desc": "System Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "cpld4_version").as_posix(),
            },
        },
        {
            "name": "PCB1",
            "desc": "Printed Circuit Board",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "pcb1_version").as_posix(),
            },
        },
        {
            "name": "PCB2",
            "desc": "Printed Circuit Board",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": (_sys_sysfs_dir / "pcb2_version").as_posix(),
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX864EN_128X400G] = {}
    COMPONENT_INFO[HWSKU_CX864EN_64X400G_32X800G] = {}
    COMPONENT_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    COMPONENT_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E04] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    COMPONENT_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E04] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    COMPONENT_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E04] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )

    ################################
    #         PERIPHERALS          #
    ################################
    NUM_FANTRAYS = PDDF_DEVICE_DATA.get("PLATFORM", {}).get("num_fantrays", 0)
    NUM_FANS_PERTRAY = PDDF_DEVICE_DATA.get("PLATFORM", {}).get("num_fans_pertray", 0)
    NUM_PSUS = PDDF_DEVICE_DATA.get("PLATFORM", {}).get("num_psus", 0)
    NUM_TEMPS = PDDF_DEVICE_DATA.get("PLATFORM", {}).get("num_temps", 0)
    NUM_PAYLOADS = len(
        list(filter(lambda key: key.startswith("PAYLOAD"), PDDF_DEVICE_DATA.keys()))
    )
    NUM_PORTS = PDDF_DEVICE_DATA.get("PLATFORM", {}).get("num_ports", 0)

    SFP_NAMES = {}  # type: dict[str, dict[str, list[str]]]
    ################
    # CX308P-48Y-N #
    ################
    CX308P48YN_PORT_SFP_START = 0
    CX308P48YN_PORT_QSFP_START = 48
    CX308P48YN_PORT_END = 56
    SFP_NAMES[HWSKU_CX308P48YN] = {}
    SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E00] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if CX308P48YN_PORT_SFP_START <= _sfp_index < CX308P48YN_PORT_QSFP_START:
            sfp_name = "Ethernet{}".format(_sfp_index)
        elif CX308P48YN_PORT_QSFP_START <= _sfp_index < CX308P48YN_PORT_END:
            sfp_name = "Ethernet{}".format(
                CX308P48YN_PORT_QSFP_START
                + (_sfp_index - CX308P48YN_PORT_QSFP_START) * 4
            )
        SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E00].append(sfp_name)
    SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E01] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if CX308P48YN_PORT_SFP_START <= _sfp_index < CX308P48YN_PORT_QSFP_START:
            sfp_name = "Ethernet{}".format(_sfp_index)
        elif CX308P48YN_PORT_QSFP_START <= _sfp_index < CX308P48YN_PORT_END:
            sfp_name = "Ethernet{}".format(
                CX308P48YN_PORT_QSFP_START
                + (_sfp_index - CX308P48YN_PORT_QSFP_START) * 4
            )
        SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E01].append(sfp_name)
    SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E02] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E03] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    SFP_NAMES[HWSKU_CX308P48YN][ASIC_FL00E01] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    SFP_NAMES[HWSKU_CX308P48YN][ASIC_FL00E02] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    SFP_NAMES[HWSKU_CX308P48YN][ASIC_FL00E03] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX308P48YN][ASIC_TL05E01]
    )

    ############
    # CX532P-N #
    ############
    CX532PN_PORT_QSFP_START = 0
    CX532PN_PORT_SFP_START = 32
    CX532PN_PORT_END = 34
    SFP_NAMES[HWSKU_CX532PN] = {}
    SFP_NAMES[HWSKU_CX532PN][ASIC_TL05E00] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if CX532PN_PORT_QSFP_START <= _sfp_index < CX532PN_PORT_SFP_START:
            sfp_name = "Ethernet{}".format(_sfp_index * 4)
        elif CX532PN_PORT_SFP_START <= _sfp_index < CX532PN_PORT_END:
            continue
        SFP_NAMES[HWSKU_CX532PN][ASIC_TL05E00].append(sfp_name)
    SFP_NAMES[HWSKU_CX532PN][ASIC_TL07E01] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if CX532PN_PORT_QSFP_START <= _sfp_index < CX532PN_PORT_SFP_START:
            sfp_name = "Ethernet{}".format(_sfp_index * 4)
        elif CX532PN_PORT_SFP_START <= _sfp_index < CX532PN_PORT_END:
            sfp_name = "Ethernet{}".format(CX532PN_PORT_SFP_START * 3 + _sfp_index)
        SFP_NAMES[HWSKU_CX532PN][ASIC_TL07E01].append(sfp_name)
    SFP_NAMES[HWSKU_CX532PN][ASIC_TL07E02] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX532PN][ASIC_TL07E01]
    )
    SFP_NAMES[HWSKU_CX532PN][ASIC_TL07E03] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX532PN][ASIC_TL07E01]
    )
    SFP_NAMES[HWSKU_CX532PN][ASIC_FL00E02] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX532PN][ASIC_TL07E01]
    )
    SFP_NAMES[HWSKU_CX532PN][ASIC_FL00E03] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX532PN][ASIC_TL07E01]
    )

    ############
    # CX564P-N #
    ############
    CX564PN_PORT_QSFP_START = 0
    CX564PN_PORT_SFP_START = 64
    CX564PN_PORT_END = 66
    SFP_NAMES[HWSKU_CX564PN] = {}
    SFP_NAMES[HWSKU_CX564PN][ASIC_TL07E01] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if CX564PN_PORT_QSFP_START <= _sfp_index < CX564PN_PORT_SFP_START:
            sfp_name = "Ethernet{}".format(_sfp_index * 4)
        elif CX564PN_PORT_SFP_START <= _sfp_index < CX564PN_PORT_END:
            sfp_name = "Ethernet{}".format(CX564PN_PORT_SFP_START * 3 + _sfp_index)
        SFP_NAMES[HWSKU_CX564PN][ASIC_TL07E01].append(sfp_name)
    SFP_NAMES[HWSKU_CX564PN][ASIC_TL07E02] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX564PN][ASIC_TL07E01]
    )
    SFP_NAMES[HWSKU_CX564PN][ASIC_TL07E03] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX564PN][ASIC_TL07E01]
    )

    ############
    # CX664D-N #
    ############
    CX664DN_PORT_QSFPDD_START = 0
    CX664DN_PORT_SFP_START = 64
    CX664DN_PORT_END = 66
    SFP_NAMES[HWSKU_CX664DN] = {}
    SFP_NAMES[HWSKU_CX664DN][ASIC_TL07E01] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if CX664DN_PORT_QSFPDD_START <= _sfp_index < CX664DN_PORT_SFP_START:
            sfp_name = "Ethernet{}".format(_sfp_index * 4)
        elif CX664DN_PORT_SFP_START <= _sfp_index < CX664DN_PORT_END:
            sfp_name = "Ethernet{}".format(CX664DN_PORT_SFP_START * 3 + _sfp_index)
        SFP_NAMES[HWSKU_CX664DN][ASIC_TL07E01].append(sfp_name)
    SFP_NAMES[HWSKU_CX664DN][ASIC_TL07E02] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX664DN][ASIC_TL07E01]
    )
    SFP_NAMES[HWSKU_CX664DN][ASIC_TL07E03] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX664DN][ASIC_TL07E01]
    )

    ############
    # CX732Q-N #
    ############
    CX732QN_PORT_QSFPDD_START = 0
    CX732QN_PORT_SFP_START = 32
    CX732QN_PORT_END = 34
    SFP_NAMES[HWSKU_CX732QN] = {}
    SFP_NAMES[HWSKU_CX732QN][ASIC_TL07E01] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if CX732QN_PORT_QSFPDD_START <= _sfp_index < CX732QN_PORT_SFP_START:
            sfp_name = "Ethernet{}".format(_sfp_index * 8)
        elif CX732QN_PORT_SFP_START <= _sfp_index < CX732QN_PORT_END:
            sfp_name = "Ethernet{}".format(CX732QN_PORT_SFP_START * 7 + _sfp_index)
        SFP_NAMES[HWSKU_CX732QN][ASIC_TL07E01].append(sfp_name)
    SFP_NAMES[HWSKU_CX732QN][ASIC_TL07E02] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX732QN][ASIC_TL07E01]
    )
    SFP_NAMES[HWSKU_CX732QN][ASIC_TL07E03] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX732QN][ASIC_TL07E01]
    )

    ############
    # CX864E-N #
    ############
    CX864EN_PORT_OSFP_START = 0
    CX864EN_PORT_SFP_START = 64
    CX864EN_PORT_END = 66
    SFP_NAMES[HWSKU_CX864EN] = {}
    SFP_NAMES[HWSKU_CX864EN][ASIC_TL10E02] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if CX864EN_PORT_OSFP_START <= _sfp_index < CX864EN_PORT_SFP_START:
            sfp_name = "Ethernet{}".format(_sfp_index * 8)
        elif CX864EN_PORT_SFP_START <= _sfp_index < CX864EN_PORT_END:
            sfp_name = "Ethernet{}".format(CX864EN_PORT_SFP_START * 7 + _sfp_index)
        SFP_NAMES[HWSKU_CX864EN][ASIC_TL10E02].append(sfp_name)
    CX864EN_128X400G_PORT_QSFPDD_START = 0
    CX864EN_128X400G_PORT_SFP_START = 128
    CX864EN_128X400G_PORT_END = 130
    SFP_NAMES[HWSKU_CX864EN_128X400G] = {}
    SFP_NAMES[HWSKU_CX864EN_128X400G][ASIC_TL10E02] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if (
            CX864EN_128X400G_PORT_QSFPDD_START
            <= _sfp_index
            < CX864EN_128X400G_PORT_SFP_START
        ):
            sfp_name = "Ethernet{}".format(_sfp_index * 4)
        elif CX864EN_128X400G_PORT_SFP_START <= _sfp_index < CX864EN_128X400G_PORT_END:
            sfp_name = "Ethernet{}".format(
                CX864EN_128X400G_PORT_SFP_START * 3 + _sfp_index
            )
        SFP_NAMES[HWSKU_CX864EN_128X400G][ASIC_TL10E02].append(sfp_name)
    CX864EN_64X400G_32X800G_PORT_QSFPDD_START = 0
    CX864EN_64X400G_32X800G_PORT_OSFP_START = 64
    CX864EN_64X400G_32X800G_PORT_SFP_START = 96
    CX864EN_64X400G_32X800G_PORT_END = 98
    SFP_NAMES[HWSKU_CX864EN_64X400G_32X800G] = {}
    SFP_NAMES[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02] = []
    for _sfp_index in range(NUM_PORTS):
        sfp_name = NOT_AVAILABLE
        if (
            CX864EN_64X400G_32X800G_PORT_QSFPDD_START
            <= _sfp_index
            < CX864EN_64X400G_32X800G_PORT_OSFP_START
        ):
            sfp_name = "Ethernet{}".format(_sfp_index * 4)
        elif (
            CX864EN_64X400G_32X800G_PORT_OSFP_START
            <= _sfp_index
            < CX864EN_64X400G_32X800G_PORT_SFP_START
        ):
            sfp_name = "Ethernet{}".format(
                _sfp_index * 8 - CX864EN_64X400G_32X800G_PORT_OSFP_START * 4
            )
        elif (
            CX864EN_64X400G_32X800G_PORT_SFP_START
            <= _sfp_index
            < CX864EN_64X400G_32X800G_PORT_END
        ):
            sfp_name = "Ethernet{}".format(
                CX864EN_64X400G_32X800G_PORT_OSFP_START * 3
                + (
                    CX864EN_64X400G_32X800G_PORT_SFP_START
                    - CX864EN_64X400G_32X800G_PORT_OSFP_START
                )
                * 7
                + _sfp_index
            )
        SFP_NAMES[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02].append(sfp_name)
    SFP_NAMES[HWSKU_CX864EN][ASIC_TL10E04] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX864EN][ASIC_TL10E02]
    )
    SFP_NAMES[HWSKU_CX864EN_128X400G][ASIC_TL10E04] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX864EN][ASIC_TL10E02]
    )
    SFP_NAMES[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E04] = _copy.deepcopy(
        SFP_NAMES[HWSKU_CX864EN][ASIC_TL10E02]
    )

    _fan_drawer_info = []  # type: list[dict[str, str]]
    _fan_info = []  # type: list[dict[str, str|dict[str, str|tuple[str, str]]]]
    _psu_info = []  # type: list[dict[str, float|str|dict[str, str]]]
    _thermal_info = []  # type: list[dict[str, float|str|dict[str, float|str]]]
    _voltage_info = []  # type: list[dict[str, float|str|dict[str, str]]]
    _sfp_info = []  # type: list[dict[str, str|dict[str, str]]]
    for _fan_drawer_index in range(NUM_FANTRAYS):
        _fan_drawer_info.append(
            {
                "name": "FAN DRAWER {}".format(_fan_drawer_index + 1),
            }
        )
    for _fan_drawer_index, _fan_index in _product(
        range(NUM_FANTRAYS), range(NUM_FANS_PERTRAY)
    ):
        _fan_drawer_sysfs_dir = _Path("/sys/bus/i2c/devices") / "{}-{:04x}".format(
            int(
                PDDF_DEVICE_DATA.get("FAN{}".format(_fan_drawer_index + 1), {})
                .get("uart", {})
                .get("topo_info", {})
                .get("parent_bus"),
                16,
            ),
            int(
                PDDF_DEVICE_DATA.get("FAN{}".format(_fan_drawer_index + 1), {})
                .get("uart", {})
                .get("topo_info", {})
                .get("dev_addr"),
                16,
            ),
        )
        if _fan_index % 2 == 0:
            _fan_info.append(
                {
                    "name": "FAN {}F".format(_fan_drawer_index + 1),
                    "presence": {
                        "type": SYSFS_MATCH_TYPE_EXACT,
                        "path": (_fan_drawer_sysfs_dir / "fan_presence").as_posix(),
                        "cmp": "1",
                    },
                    "status": {
                        "type": SYSFS_MATCH_TYPE_EXACT,
                        "path": (_fan_drawer_sysfs_dir / "fan_status").as_posix(),
                        "cmp": "0",
                    },
                    "speed": {
                        "type": SYSFS_MATCH_TYPE_EXACT,
                        "path": (
                            _fan_drawer_sysfs_dir / "fan_front_speed_rpm"
                        ).as_posix(),
                    },
                    "direction": {
                        "type": SYSFS_MATCH_TYPE_EXACT,
                        "path": (_fan_drawer_sysfs_dir / "fan_direction").as_posix(),
                        "cmp": "1",
                        "choices": ("Intake", "Exhaust"),
                    },
                }
            )
        else:
            _fan_info.append(
                {
                    "name": "FAN {}R".format(_fan_drawer_index + 1),
                    "presence": {
                        "type": SYSFS_MATCH_TYPE_EXACT,
                        "path": (_fan_drawer_sysfs_dir / "fan_presence").as_posix(),
                        "cmp": "1",
                    },
                    "status": {
                        "type": SYSFS_MATCH_TYPE_EXACT,
                        "path": (_fan_drawer_sysfs_dir / "fan_status").as_posix(),
                        "cmp": "0",
                    },
                    "speed": {
                        "type": SYSFS_MATCH_TYPE_EXACT,
                        "path": (
                            _fan_drawer_sysfs_dir / "fan_rear_speed_rpm"
                        ).as_posix(),
                    },
                    "direction": {
                        "type": SYSFS_MATCH_TYPE_EXACT,
                        "path": (_fan_drawer_sysfs_dir / "fan_direction").as_posix(),
                        "cmp": "1",
                        "choices": ("Intake", "Exhaust"),
                    },
                }
            )
    for _psu_index in range(NUM_PSUS):
        _psu_sysfs_dir = _Path("/sys/bus/i2c/devices") / "{}-{:04x}".format(
            int(
                PDDF_DEVICE_DATA.get("PSU{}-UART".format(_psu_index + 1), {})
                .get("uart", {})
                .get("topo_info", {})
                .get("parent_bus"),
                16,
            ),
            int(
                PDDF_DEVICE_DATA.get("PSU{}-UART".format(_psu_index + 1), {})
                .get("uart", {})
                .get("topo_info", {})
                .get("dev_addr"),
                16,
            ),
        )
        _psu_info.append(
            {
                "name": "PSU {}".format(_psu_index + 1),
                "vscale": 1000.0,
                "cscale": 1000.0,
                "pscale": 1000.0,
                "presence": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_presence").as_posix(),
                    "cmp": "0",
                },
                "status": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_status").as_posix(),
                    "cmp": "1",
                },
                "warning": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_warning").as_posix(),
                    "cmp": "0",
                },
                "vin": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_v_in").as_posix(),
                },
                "vout": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_v_out").as_posix(),
                },
                "cin": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_i_in").as_posix(),
                },
                "cout": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_i_out").as_posix(),
                },
                "pin": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_p_in").as_posix(),
                },
                "pout": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_psu_sysfs_dir / "psu_p_out").as_posix(),
                },
            }
        )
    for _thermal_index in range(NUM_TEMPS):
        _thermal_sysfs_dir = _Path("/sys/bus/i2c/devices") / "{}-{:04x}".format(
            int(
                PDDF_DEVICE_DATA.get("THERMAL{}".format(_thermal_index + 1), {})
                .get("uart", {})
                .get("topo_info", {})
                .get("parent_bus"),
                16,
            ),
            int(
                PDDF_DEVICE_DATA.get("THERMAL{}".format(_thermal_index + 1), {})
                .get("uart", {})
                .get("topo_info", {})
                .get("dev_addr"),
                16,
            ),
        )
        _thermal_sysfs_names = (
            PDDF_DEVICE_DATA.get("THERMAL{}".format(_thermal_index + 1), {})
            .get("uart", {})
            .get("attr_list", [])
        )
        if not len(_thermal_sysfs_names):
            continue
        _thermal_sysfs_name = _thermal_sysfs_names[0].get("attr_name", NOT_AVAILABLE)
        if _thermal_sysfs_name == NOT_AVAILABLE or _thermal_sysfs_name.endswith("_set"):
            continue
        _thermal_info.append(
            {
                "name": _thermal_sysfs_name.replace("_", " ").title(),
                "presence": {
                    "type": SYSFS_MATCH_TYPE_VALUE,
                    "value": "1",
                    "cmp": "1",
                },
                "status": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_thermal_sysfs_dir / _thermal_sysfs_name).as_posix(),
                    "revcmp": "0",
                },
                "temp": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_thermal_sysfs_dir / _thermal_sysfs_name).as_posix(),
                },
                "high": {
                    "type": SYSFS_MATCH_TYPE_VALUE,
                    "value": THERMAL_TEMP_HIGH,
                },
                "chigh": {
                    "type": SYSFS_MATCH_TYPE_VALUE,
                    "value": THERMAL_TEMP_CRIT_HIGH,
                },
                "low": {
                    "type": SYSFS_MATCH_TYPE_VALUE,
                    "value": THERMAL_TEMP_LOW,
                },
                "clow": {
                    "type": SYSFS_MATCH_TYPE_VALUE,
                    "value": THERMAL_TEMP_CRIT_LOW,
                },
            }
        )
    for _voltage_index in range(NUM_PAYLOADS):
        _voltage_sysfs_dir = _Path("/sys/bus/i2c/devices") / "{}-{:04x}".format(
            int(
                PDDF_DEVICE_DATA.get("PAYLOAD{}".format(_voltage_index + 1), {})
                .get("uart", {})
                .get("topo_info", {})
                .get("parent_bus"),
                16,
            ),
            int(
                PDDF_DEVICE_DATA.get("PAYLOAD{}".format(_voltage_index + 1), {})
                .get("uart", {})
                .get("topo_info", {})
                .get("dev_addr"),
                16,
            ),
        )
        _voltage_sysfs_names = (
            PDDF_DEVICE_DATA.get("PAYLOAD{}".format(_voltage_index + 1), {})
            .get("uart", {})
            .get("attr_list", [])
        )
        if not len(_voltage_sysfs_names):
            continue
        _voltage_sysfs_name = _voltage_sysfs_names[0].get("attr_name", NOT_AVAILABLE)
        if _voltage_sysfs_name == NOT_AVAILABLE or _voltage_sysfs_name.endswith("_set"):
            continue
        _voltage_info.append(
            {
                "name": _voltage_sysfs_name.replace("_", " ").title(),
                "vscale": 1000.0,
                "presence": {
                    "type": SYSFS_MATCH_TYPE_VALUE,
                    "value": "1",
                    "cmp": "1",
                },
                "status": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_voltage_sysfs_dir / _voltage_sysfs_name).as_posix(),
                    "revcmp": "0",
                },
                "vin": {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_voltage_sysfs_dir / _voltage_sysfs_name).as_posix(),
                },
            }
        )
    for _sfp_index in range(NUM_PORTS):
        _sfp_eeprom_sysfs_dir = _Path("/sys/bus/i2c/devices") / "{}-{:04x}".format(
            int(
                PDDF_DEVICE_DATA.get("PORT{}-EEPROM".format(_sfp_index + 1), {})
                .get("i2c", {})
                .get("topo_info", {})
                .get("parent_bus"),
                16,
            ),
            int(
                PDDF_DEVICE_DATA.get("PORT{}-EEPROM".format(_sfp_index + 1), {})
                .get("i2c", {})
                .get("topo_info", {})
                .get("dev_addr"),
                16,
            ),
        )
        _sfp_ctrl_sysfs_dir = _Path("/sys/bus/i2c/devices") / "{}-{:04x}".format(
            int(
                PDDF_DEVICE_DATA.get("PORT{}-CTRL".format(_sfp_index + 1), {})
                .get("i2c", {})
                .get("topo_info", {})
                .get("parent_bus"),
                16,
            ),
            int(
                PDDF_DEVICE_DATA.get("PORT{}-CTRL".format(_sfp_index + 1), {})
                .get("i2c", {})
                .get("topo_info", {})
                .get("dev_addr"),
                16,
            ),
        )
        sfp_info = {}
        sfp_info["eeprom"] = {
            "type": SYSFS_MATCH_TYPE_EXACT,
            "path": (_sfp_eeprom_sysfs_dir / "eeprom").as_posix(),
        }
        _sfp_attr_list = (
            PDDF_DEVICE_DATA.get("PORT{}-CTRL".format(_sfp_index + 1), {})
            .get("i2c", {})
            .get("attr_list", [])
        )
        for _sfp_attr in _sfp_attr_list:
            _sfp_attr_name = _sfp_attr.get("attr_name", NOT_AVAILABLE)
            if _sfp_attr_name == "xcvr_present":
                sfp_info["presence"] = {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_sfp_ctrl_sysfs_dir / "xcvr_present").as_posix(),
                    "cmp": "1",
                }
            elif _sfp_attr_name == "xcvr_reset":
                sfp_info["reset"] = {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_sfp_ctrl_sysfs_dir / "xcvr_reset").as_posix(),
                    "cmp": "1",
                    "on": "1",
                    "off": "0",
                }
            elif _sfp_attr_name == "xcvr_intr_status":
                sfp_info["intr"] = {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_sfp_ctrl_sysfs_dir / "xcvr_intr_status").as_posix(),
                    "cmp": "1",
                    "on": "1",
                    "off": "0",
                }
            elif _sfp_attr_name == "xcvr_lpmode":
                sfp_info["lpmode"] = {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_sfp_ctrl_sysfs_dir / "xcvr_lpmode").as_posix(),
                    "cmp": "1",
                    "on": "1",
                    "off": "0",
                }
            elif _sfp_attr_name == "xcvr_rxlos":
                sfp_info["rxlos"] = {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_sfp_ctrl_sysfs_dir / "xcvr_rxlos").as_posix(),
                    "cmp": "1",
                    "on": "1",
                    "off": "0",
                }
            elif _sfp_attr_name == "xcvr_txdisable":
                sfp_info["txdis"] = {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_sfp_ctrl_sysfs_dir / "xcvr_txdisable").as_posix(),
                    "cmp": "1",
                    "on": "1",
                    "off": "0",
                }
            elif _sfp_attr_name == "xcvr_txfault":
                sfp_info["txfault"] = {
                    "type": SYSFS_MATCH_TYPE_EXACT,
                    "path": (_sfp_ctrl_sysfs_dir / "xcvr_txfault").as_posix(),
                    "cmp": "1",
                    "on": "1",
                    "off": "0",
                }
        _sfp_info.append(sfp_info)

    for _hwsku in (
        HWSKU_CX308P48YN,
        HWSKU_CX532PN,
        HWSKU_CX564PN,
        HWSKU_CX664DN,
        HWSKU_CX732QN,
        HWSKU_CX864EN,
        HWSKU_CX864EN_128X400G,
        HWSKU_CX864EN_64X400G_32X800G,
    ):
        FAN_DRAWER_INFO[_hwsku] = {}
        FAN_INFO[_hwsku] = {}
        PSU_INFO[_hwsku] = {}
        THERMAL_INFO[_hwsku] = {}
        VOLTAGE_INFO[_hwsku] = {}
        SFP_INFO[_hwsku] = {}
        for _asic in (
            ASIC_TL05E00,
            ASIC_TL05E01,
            ASIC_TL05E02,
            ASIC_TL05E03,
            ASIC_TL07E01,
            ASIC_TL07E02,
            ASIC_TL07E03,
            ASIC_TL10E02,
            ASIC_TL10E04,
            ASIC_FL00E01,
            ASIC_FL00E02,
            ASIC_FL00E03,
        ):
            FAN_DRAWER_INFO[_hwsku][_asic] = _copy.deepcopy(_fan_drawer_info)
            FAN_INFO[_hwsku][_asic] = _copy.deepcopy(_fan_info)
            PSU_INFO[_hwsku][_asic] = _copy.deepcopy(_psu_info)
            THERMAL_INFO[_hwsku][_asic] = _copy.deepcopy(_thermal_info)
            VOLTAGE_INFO[_hwsku][_asic] = _copy.deepcopy(_voltage_info)
            SFP_INFO[_hwsku][_asic] = _copy.deepcopy(_sfp_info)
            for _index, _sfp_name in enumerate(
                SFP_NAMES.get(_hwsku, {}).get(_asic, {})
            ):
                SFP_INFO[_hwsku][_asic][_index]["name"] = _sfp_name
else:
    ################################
    #          COMPONENT           #
    ################################
    # Updating firmware is not supported yet!

    ################
    # CX308P-48Y-N #
    ################
    COMPONENT_INFO[HWSKU_CX308P48YN] = {}
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E00] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "System Management",
        },
        {
            "name": "CPLD2",
            "desc": "SFP28 Ports (Y1 - Y32) Management",
        },
        {
            "name": "CPLD3",
            "desc": "SFP28 Ports (Y33 - Y48), QSFP28 Ports (C1 - C8) Management",
        },
        {
            "name": "CPLD4",
            "desc": "Fan Management",
        },
        {
            "name": "CPLD5",
            "desc": "Power Management",
        },
    ]
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bmc_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "SFP28 Ports (Y33 - Y48), QSFP28 Ports (C1 - C8) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD1",
                "delim": ":",
            },
        },
        {
            "name": "CPLD2",
            "desc": "SFP28 Ports (Y1 - Y32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD2",
                "delim": ":",
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_FL00E01] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_FL00E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_FL00E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )

    ############
    # CX532P-N #
    ############
    COMPONENT_INFO[HWSKU_CX532PN] = {}
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL05E00] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "System Management",
        },
        {
            "name": "CPLD2",
            "desc": "QSFP28 Ports (C1 - C16) Management",
        },
        {
            "name": "CPLD3",
            "desc": "QSFP28 Ports (C17 - C32) Management",
        },
        {
            "name": "CPLD4",
            "desc": "Fan Management",
        },
        {
            "name": "CPLD5",
            "desc": "Power Management",
        },
    ]
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bmc_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "QSFP28 Ports (C1 - C16) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD1",
                "delim": ":",
            },
        },
        {
            "name": "CPLD2",
            "desc": "SFP+ Ports (X1 - X2), QSFP28 Ports (C17 - C32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD2",
                "delim": ":",
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_FL00E02] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bmc_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "SFP+ Ports (X1 - X2), QSFP28 Ports (C13 - C32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD1",
                "delim": ":",
            },
        },
        {
            "name": "CPLD2",
            "desc": "QSFP28 Ports (C1 - C12) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD2",
                "delim": ":",
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX532PN][ASIC_FL00E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX532PN][ASIC_FL00E02]
    )

    ############
    # CX564P-N #
    ############
    COMPONENT_INFO[HWSKU_CX564PN] = {}
    COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bmc_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "QSFP28 Ports (C1 - C32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD1",
                "delim": ":",
            },
        },
        {
            "name": "CPLD2",
            "desc": "SFP+ Ports (X1 - X2), QSFP28 Ports (C33 - C64) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD2",
                "delim": ":",
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )

    ############
    # CX664D-N #
    ############
    COMPONENT_INFO[HWSKU_CX664DN] = {}
    COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bmc_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "QSFP56 Ports (D1 - D32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD1",
                "delim": ":",
            },
        },
        {
            "name": "CPLD2",
            "desc": "SFP+ Ports (X1 - X2), QSFP56 Ports (D33 - D64) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD2",
                "delim": ":",
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )

    ############
    # CX732Q-N #
    ############
    COMPONENT_INFO[HWSKU_CX732QN] = {}
    COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E01] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bmc_version",
            },
        },
        {
            "name": "CPLD1",
            "desc": "SFP+ Ports (X1 - X2), QSFPDD Ports (QC1 - QC32) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD1",
                "delim": ":",
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )
    COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E03] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )

    ############
    # CX864E-N #
    ############
    COMPONENT_INFO[HWSKU_CX864EN] = {}
    COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02] = [
        {
            "name": "BIOS",
            "desc": "Basic Input/Output System",
            "version": {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": "/sys/class/dmi/id/bios_version",
            },
        },
        {
            "name": "BMC",
            "desc": "Baseboard Management Controller",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bmc_version",
            },
        },
        {
            "name": "BOM1",
            "desc": "",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bom_version",
                "key": "BOM1",
                "delim": ":",
            },
        },
        {
            "name": "BOM2",
            "desc": "",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/bom_version",
                "key": "BOM2",
                "delim": ":",
            },
        },
        {
            "name": "CPLD1",
            "desc": "SFP+ Ports (X1 - X2), OSFP Ports (E1 - E64) Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD1",
                "delim": ":",
            },
        },
        {
            "name": "CPLD2",
            "desc": "System Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD2",
                "delim": ":",
            },
        },
        {
            "name": "CPLD3",
            "desc": "System Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD3",
                "delim": ":",
            },
        },
        {
            "name": "CPLD4",
            "desc": "System Management",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/cpld_version",
                "key": "CPLD4",
                "delim": ":",
            },
        },
        {
            "name": "PCB1",
            "desc": "Printed Circuit Board",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/pcb_version",
                "key": "PCB1",
                "delim": ":",
            },
        },
        {
            "name": "PCB2",
            "desc": "Printed Circuit Board",
            "version": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/SYS_INFO/pcb_version",
                "key": "PCB2",
                "delim": ":",
            },
        },
    ]
    COMPONENT_INFO[HWSKU_CX864EN_128X400G] = {}
    COMPONENT_INFO[HWSKU_CX864EN_64X400G_32X800G] = {}
    COMPONENT_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    COMPONENT_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E04] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    COMPONENT_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E04] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    COMPONENT_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E04] = _copy.deepcopy(
        COMPONENT_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )

    ################################
    #             FAN              #
    ################################

    ################
    # CX308P-48Y-N #
    ################
    FAN_INFO[HWSKU_CX308P48YN] = {}
    FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E00] = [
        {
            "name": "FAN 1F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_speed_rpm",
                "key": "FanModule1 Front",
                "delim": ":",
                "trail": "RPM",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 1R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_speed_rpm",
                "key": "FanModule1 Rear",
                "delim": ":",
                "trail": "RPM",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 2F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_speed_rpm",
                "key": "FanModule2 Front",
                "delim": ":",
                "trail": "RPM",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 2R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_speed_rpm",
                "key": "FanModule2 Rear",
                "delim": ":",
                "trail": "RPM",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 3F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_speed_rpm",
                "key": "FanModule3 Front",
                "delim": ":",
                "trail": "RPM",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 3R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_speed_rpm",
                "key": "FanModule3 Rear",
                "delim": ":",
                "trail": "RPM",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 4F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_speed_rpm",
                "key": "FanModule4 Front",
                "delim": ":",
                "trail": "RPM",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 4R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan_speed_rpm",
                "key": "FanModule4 Rear",
                "delim": ":",
                "trail": "RPM",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
    ]
    FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E01] = [
        {
            "name": "FAN 1F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan1_speed_rpm",
                "key": "FanModule1 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 1R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan1_speed_rpm",
                "key": "FanModule1 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan2_speed_rpm",
                "key": "FanModule2 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan2_speed_rpm",
                "key": "FanModule2 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan3_speed_rpm",
                "key": "FanModule3 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan3_speed_rpm",
                "key": "FanModule3 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan4_speed_rpm",
                "key": "FanModule4 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan4_speed_rpm",
                "key": "FanModule4 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
    ]
    FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E03] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    FAN_INFO[HWSKU_CX308P48YN][ASIC_FL00E01] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    FAN_INFO[HWSKU_CX308P48YN][ASIC_FL00E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    FAN_INFO[HWSKU_CX308P48YN][ASIC_FL00E03] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    FAN_DRAWER_INFO[HWSKU_CX308P48YN] = {}
    FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E00] = [
        {
            "name": "FAN DRAWER 1",
        },
        {
            "name": "FAN DRAWER 2",
        },
        {
            "name": "FAN DRAWER 3",
        },
        {
            "name": "FAN DRAWER 4",
        },
    ]
    FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E01] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E03] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_FL00E01] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_FL00E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_FL00E03] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX308P48YN][ASIC_TL05E00]
    )

    ############
    # CX532P-N #
    ############
    FAN_INFO[HWSKU_CX532PN] = {}
    FAN_INFO[HWSKU_CX532PN][ASIC_TL05E00] = [
        {
            "name": "FAN 1F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_front_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 1R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_rear_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 2F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_front_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 2R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_rear_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 3F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_front_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 3R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_rear_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 4F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_front_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 4R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_rear_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 5F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_front_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
        {
            "name": "FAN 5R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_present",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_stat",
                "cmp": "1",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_rear_rpm",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "exhaust",
            },
        },
    ]
    FAN_INFO[HWSKU_CX532PN][ASIC_TL07E01] = [
        {
            "name": "FAN 1F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_speed_rpm",
                "key": "FanModule1 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 1R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_speed_rpm",
                "key": "FanModule1 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_speed_rpm",
                "key": "FanModule2 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_speed_rpm",
                "key": "FanModule2 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_speed_rpm",
                "key": "FanModule3 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_speed_rpm",
                "key": "FanModule3 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_speed_rpm",
                "key": "FanModule4 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_speed_rpm",
                "key": "FanModule4 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
        {
            "name": "FAN 5F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_present",
                "key": "Fan 5",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_status",
                "key": "Fan 5",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_speed_rpm",
                "key": "FanModule5 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_direction",
                "key": "Fan 5",
                "delim": "is",
            },
        },
        {
            "name": "FAN 5R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_present",
                "key": "Fan 5",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_status",
                "key": "Fan 5",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_speed_rpm",
                "key": "FanModule5 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_FAN/fan5_direction",
                "key": "Fan 5",
                "delim": "is",
            },
        },
    ]
    FAN_INFO[HWSKU_CX532PN][ASIC_TL07E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    FAN_INFO[HWSKU_CX532PN][ASIC_TL07E03] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    FAN_INFO[HWSKU_CX532PN][ASIC_FL00E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    FAN_INFO[HWSKU_CX532PN][ASIC_FL00E03] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    FAN_DRAWER_INFO[HWSKU_CX532PN] = {}
    FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL05E00] = [
        {
            "name": "FAN DRAWER 1",
        },
        {
            "name": "FAN DRAWER 2",
        },
        {
            "name": "FAN DRAWER 3",
        },
        {
            "name": "FAN DRAWER 4",
        },
        {
            "name": "FAN DRAWER 5",
        },
    ]
    FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL07E01] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL07E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL07E03] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_FL00E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL05E00]
    )
    FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_FL00E03] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX532PN][ASIC_TL05E00]
    )

    ############
    # CX564P-N #
    ############
    FAN_INFO[HWSKU_CX564PN] = {}
    FAN_INFO[HWSKU_CX564PN][ASIC_TL07E01] = [
        {
            "name": "FAN 1",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan1_speed_rpm",
                "key": "FanModule1 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan2_speed_rpm",
                "key": "FanModule2 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan3_speed_rpm",
                "key": "FanModule3 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan4_speed_rpm",
                "key": "FanModule4 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
    ]
    FAN_INFO[HWSKU_CX564PN][ASIC_TL07E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )
    FAN_INFO[HWSKU_CX564PN][ASIC_TL07E03] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )
    FAN_DRAWER_INFO[HWSKU_CX564PN] = {}
    FAN_DRAWER_INFO[HWSKU_CX564PN][ASIC_TL07E01] = [
        {
            "name": "FAN DRAWER 1",
        },
        {
            "name": "FAN DRAWER 2",
        },
        {
            "name": "FAN DRAWER 3",
        },
        {
            "name": "FAN DRAWER 4",
        },
    ]
    FAN_DRAWER_INFO[HWSKU_CX564PN][ASIC_TL07E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )
    FAN_DRAWER_INFO[HWSKU_CX564PN][ASIC_TL07E03] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )

    ############
    # CX664D-N #
    ############
    FAN_INFO[HWSKU_CX664DN] = {}
    FAN_INFO[HWSKU_CX664DN][ASIC_TL07E01] = [
        {
            "name": "FAN 1",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan1_speed_rpm",
                "key": "FanModule1 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan2_speed_rpm",
                "key": "FanModule2 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan3_speed_rpm",
                "key": "FanModule3 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan4_speed_rpm",
                "key": "FanModule4 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
    ]
    FAN_INFO[HWSKU_CX664DN][ASIC_TL07E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )
    FAN_INFO[HWSKU_CX664DN][ASIC_TL07E03] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )
    FAN_DRAWER_INFO[HWSKU_CX664DN] = {}
    FAN_DRAWER_INFO[HWSKU_CX664DN][ASIC_TL07E01] = [
        {
            "name": "FAN DRAWER 1",
        },
        {
            "name": "FAN DRAWER 2",
        },
        {
            "name": "FAN DRAWER 3",
        },
        {
            "name": "FAN DRAWER 4",
        },
    ]
    FAN_DRAWER_INFO[HWSKU_CX664DN][ASIC_TL07E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )
    FAN_DRAWER_INFO[HWSKU_CX664DN][ASIC_TL07E03] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )

    ############
    # CX732Q-N #
    ############
    FAN_INFO[HWSKU_CX732QN] = {}
    FAN_INFO[HWSKU_CX732QN][ASIC_TL07E01] = [
        {
            "name": "FAN 1F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan1_speed_rpm",
                "key": "FanModule1 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 1R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan1_speed_rpm",
                "key": "FanModule1 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan2_speed_rpm",
                "key": "FanModule2 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan2_speed_rpm",
                "key": "FanModule2 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan3_speed_rpm",
                "key": "FanModule3 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan3_speed_rpm",
                "key": "FanModule3 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan4_speed_rpm",
                "key": "FanModule4 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan4_speed_rpm",
                "key": "FanModule4 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
        {
            "name": "FAN 5F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan5_present",
                "key": "Fan 5",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan5_status",
                "key": "Fan 5",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan5_speed_rpm",
                "key": "FanModule5 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan5_direction",
                "key": "Fan 5",
                "delim": "is",
            },
        },
        {
            "name": "FAN 5R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan5_present",
                "key": "Fan 5",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan5_status",
                "key": "Fan 5",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan5_speed_rpm",
                "key": "FanModule5 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan5_direction",
                "key": "Fan 5",
                "delim": "is",
            },
        },
        {
            "name": "FAN 6F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan6_present",
                "key": "Fan 6",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan6_status",
                "key": "Fan 6",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan6_speed_rpm",
                "key": "FanModule6 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan6_direction",
                "key": "Fan 6",
                "delim": "is",
            },
        },
        {
            "name": "FAN 6R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan6_present",
                "key": "Fan 6",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan6_status",
                "key": "Fan 6",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan6_speed_rpm",
                "key": "FanModule6 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_FAN/fan6_direction",
                "key": "Fan 6",
                "delim": "is",
            },
        },
    ]
    FAN_INFO[HWSKU_CX732QN][ASIC_TL07E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )
    FAN_INFO[HWSKU_CX732QN][ASIC_TL07E03] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )
    FAN_DRAWER_INFO[HWSKU_CX732QN] = {}
    FAN_DRAWER_INFO[HWSKU_CX732QN][ASIC_TL07E01] = [
        {
            "name": "FAN DRAWER 1",
        },
        {
            "name": "FAN DRAWER 2",
        },
        {
            "name": "FAN DRAWER 3",
        },
        {
            "name": "FAN DRAWER 4",
        },
        {
            "name": "FAN DRAWER 5",
        },
        {
            "name": "FAN DRAWER 6",
        },
    ]
    FAN_DRAWER_INFO[HWSKU_CX732QN][ASIC_TL07E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )
    FAN_DRAWER_INFO[HWSKU_CX732QN][ASIC_TL07E03] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )

    ############
    # CX864E-N #
    ############
    FAN_INFO[HWSKU_CX864EN] = {}
    FAN_INFO[HWSKU_CX864EN_128X400G] = {}
    FAN_INFO[HWSKU_CX864EN_64X400G_32X800G] = {}
    FAN_INFO[HWSKU_CX864EN][ASIC_TL10E02] = [
        {
            "name": "FAN 1F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan1_speed_rpm",
                "key": "FanModule1 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 1R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan1_present",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan1_status",
                "key": "Fan 1",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan1_speed_rpm",
                "key": "FanModule1 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan1_direction",
                "key": "Fan 1",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan2_speed_rpm",
                "key": "FanModule2 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 2R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan2_present",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan2_status",
                "key": "Fan 2",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan2_speed_rpm",
                "key": "FanModule2 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan2_direction",
                "key": "Fan 2",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan3_speed_rpm",
                "key": "FanModule3 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 3R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan3_present",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan3_status",
                "key": "Fan 3",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan3_speed_rpm",
                "key": "FanModule3 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan3_direction",
                "key": "Fan 3",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4F",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan4_speed_rpm",
                "key": "FanModule4 Front",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
        {
            "name": "FAN 4R",
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan4_present",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan4_status",
                "key": "Fan 4",
                "delim": "is",
                "cmp": "Good",
            },
            "speed": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan4_speed_rpm",
                "key": "FanModule4 Rear",
                "delim": ":",
            },
            "direction": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_FAN/fan4_direction",
                "key": "Fan 4",
                "delim": "is",
            },
        },
    ]
    FAN_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_INFO[HWSKU_CX864EN][ASIC_TL10E04] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E04] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E04] = _copy.deepcopy(
        FAN_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_DRAWER_INFO[HWSKU_CX864EN] = {}
    FAN_DRAWER_INFO[HWSKU_CX864EN_128X400G] = {}
    FAN_DRAWER_INFO[HWSKU_CX864EN_64X400G_32X800G] = {}
    FAN_DRAWER_INFO[HWSKU_CX864EN][ASIC_TL10E02] = [
        {
            "name": "FAN DRAWER 1",
        },
        {
            "name": "FAN DRAWER 2",
        },
        {
            "name": "FAN DRAWER 3",
        },
        {
            "name": "FAN DRAWER 4",
        },
    ]
    FAN_DRAWER_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_DRAWER_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_DRAWER_INFO[HWSKU_CX864EN][ASIC_TL10E04] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_DRAWER_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E04] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    FAN_DRAWER_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E04] = _copy.deepcopy(
        FAN_DRAWER_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )

    ################################
    #             PSU              #
    ################################

    ################
    # CX308P-48Y-N #
    ################
    PSU_INFO[HWSKU_CX308P48YN] = {}
    PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E00] = [
        {
            "name": "PSU 1",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "pscale": 1000000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_present",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_status",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "not warning",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_1",
                "key": "PSU 1 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_1",
                "key": "PSU 1 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_1",
                "key": "PSU 1 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_1",
                "key": "PSU 1 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_1",
                "key": "PSU 1 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_1",
                "key": "PSU 1 POUT",
                "delim": "is",
            },
        },
        {
            "name": "PSU 2",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "pscale": 1000000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_present",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_status",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "not warning",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_2",
                "key": "PSU 2 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_2",
                "key": "PSU 2 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_2",
                "key": "PSU 2 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_2",
                "key": "PSU 2 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_2",
                "key": "PSU 2 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_module_2",
                "key": "PSU 2 POUT",
                "delim": "is",
            },
        },
    ]
    PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E01] = [
        {
            "name": "PSU 1",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_present",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_status",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_warning",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu1_power",
                "key": "PSU 1 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu1_power",
                "key": "PSU 1 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu1_power",
                "key": "PSU 1 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu1_power",
                "key": "PSU 1 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu1_power",
                "key": "PSU 1 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu1_power",
                "key": "PSU 1 POUT",
                "delim": "is",
            },
        },
        {
            "name": "PSU 2",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_present",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_status",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu_warning",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu2_power",
                "key": "PSU 2 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu2_power",
                "key": "PSU 2 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu2_power",
                "key": "PSU 2 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu2_power",
                "key": "PSU 2 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu2_power",
                "key": "PSU 2 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_PSU/psu2_power",
                "key": "PSU 2 POUT",
                "delim": "is",
            },
        },
    ]
    PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E03] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    PSU_INFO[HWSKU_CX308P48YN][ASIC_FL00E01] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    PSU_INFO[HWSKU_CX308P48YN][ASIC_FL00E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    PSU_INFO[HWSKU_CX308P48YN][ASIC_FL00E03] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )

    ############
    # CX532P-N #
    ############
    PSU_INFO[HWSKU_CX532PN] = {}
    PSU_INFO[HWSKU_CX532PN][ASIC_TL05E00] = [
        {
            "name": "PSU 1",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "pscale": 1000000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_good",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_good",
                "cmp": "1",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_good",
                "cmp": "0",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_vin",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_vout",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_iin",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_iout",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_pin",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu1_pout",
            },
        },
        {
            "name": "PSU 2",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "pscale": 1000000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_good",
                "cmp": "1",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_good",
                "cmp": "1",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_good",
                "cmp": "0",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_vin",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_vout",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_iin",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_iout",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_pin",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_POWER/psu2_pout",
            },
        },
    ]
    PSU_INFO[HWSKU_CX532PN][ASIC_TL07E01] = [
        {
            "name": "PSU 1",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu_present",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu_status",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu_warning",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu1_power",
                "key": "PSU 1 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu1_power",
                "key": "PSU 1 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu1_power",
                "key": "PSU 1 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu1_power",
                "key": "PSU 1 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu1_power",
                "key": "PSU 1 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu1_power",
                "key": "PSU 1 POUT",
                "delim": "is",
            },
        },
        {
            "name": "PSU 2",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu_present",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu_status",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu_warning",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu2_power",
                "key": "PSU 2 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu2_power",
                "key": "PSU 2 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu2_power",
                "key": "PSU 2 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu2_power",
                "key": "PSU 2 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu2_power",
                "key": "PSU 2 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_PSU/psu2_power",
                "key": "PSU 2 POUT",
                "delim": "is",
            },
        },
    ]
    PSU_INFO[HWSKU_CX532PN][ASIC_TL07E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    PSU_INFO[HWSKU_CX532PN][ASIC_TL07E03] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    PSU_INFO[HWSKU_CX532PN][ASIC_FL00E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    PSU_INFO[HWSKU_CX532PN][ASIC_FL00E03] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )

    ############
    # CX564P-N #
    ############
    PSU_INFO[HWSKU_CX564PN] = {}
    PSU_INFO[HWSKU_CX564PN][ASIC_TL07E01] = [
        {
            "name": "PSU 1",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu_present",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu_status",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu_warning",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu1_power",
                "key": "PSU 1 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu1_power",
                "key": "PSU 1 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu1_power",
                "key": "PSU 1 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu1_power",
                "key": "PSU 1 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu1_power",
                "key": "PSU 1 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu1_power",
                "key": "PSU 1 POUT",
                "delim": "is",
            },
        },
        {
            "name": "PSU 2",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu_present",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu_status",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu_warning",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu2_power",
                "key": "PSU 2 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu2_power",
                "key": "PSU 2 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu2_power",
                "key": "PSU 2 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu2_power",
                "key": "PSU 2 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu2_power",
                "key": "PSU 2 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_PSU/psu2_power",
                "key": "PSU 2 POUT",
                "delim": "is",
            },
        },
    ]
    PSU_INFO[HWSKU_CX564PN][ASIC_TL07E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )
    PSU_INFO[HWSKU_CX564PN][ASIC_TL07E03] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )

    ############
    # CX664D-N #
    ############
    PSU_INFO[HWSKU_CX664DN] = {}
    PSU_INFO[HWSKU_CX664DN][ASIC_TL07E01] = [
        {
            "name": "PSU 1",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu_present",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu_status",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu_warning",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu1_power",
                "key": "PSU 1 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu1_power",
                "key": "PSU 1 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu1_power",
                "key": "PSU 1 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu1_power",
                "key": "PSU 1 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu1_power",
                "key": "PSU 1 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu1_power",
                "key": "PSU 1 POUT",
                "delim": "is",
            },
        },
        {
            "name": "PSU 2",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu_present",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu_status",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu_warning",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu2_power",
                "key": "PSU 2 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu2_power",
                "key": "PSU 2 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu2_power",
                "key": "PSU 2 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu2_power",
                "key": "PSU 2 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu2_power",
                "key": "PSU 2 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_PSU/psu2_power",
                "key": "PSU 2 POUT",
                "delim": "is",
            },
        },
    ]
    PSU_INFO[HWSKU_CX664DN][ASIC_TL07E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )
    PSU_INFO[HWSKU_CX664DN][ASIC_TL07E03] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )

    ############
    # CX732Q-N #
    ############
    PSU_INFO[HWSKU_CX732QN] = {}
    PSU_INFO[HWSKU_CX732QN][ASIC_TL07E01] = [
        {
            "name": "PSU 1",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu_present",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu_status",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu_warning",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu1_power",
                "key": "PSU 1 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu1_power",
                "key": "PSU 1 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu1_power",
                "key": "PSU 1 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu1_power",
                "key": "PSU 1 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu1_power",
                "key": "PSU 1 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu1_power",
                "key": "PSU 1 POUT",
                "delim": "is",
            },
        },
        {
            "name": "PSU 2",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu_present",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu_status",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu_warning",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu2_power",
                "key": "PSU 2 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu2_power",
                "key": "PSU 2 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu2_power",
                "key": "PSU 2 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu2_power",
                "key": "PSU 2 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu2_power",
                "key": "PSU 2 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_PSU/psu2_power",
                "key": "PSU 2 POUT",
                "delim": "is",
            },
        },
    ]
    PSU_INFO[HWSKU_CX732QN][ASIC_TL07E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )
    PSU_INFO[HWSKU_CX732QN][ASIC_TL07E03] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )

    ############
    # CX864E-N #
    ############
    PSU_INFO[HWSKU_CX864EN] = {}
    PSU_INFO[HWSKU_CX864EN_128X400G] = {}
    PSU_INFO[HWSKU_CX864EN_64X400G_32X800G] = {}
    PSU_INFO[HWSKU_CX864EN][ASIC_TL10E02] = [
        {
            "name": "PSU 1",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "pscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu_present",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu_status",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu_warning",
                "key": "PSU 1",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu1_power",
                "key": "PSU 1 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu1_power",
                "key": "PSU 1 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu1_power",
                "key": "PSU 1 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu1_power",
                "key": "PSU 1 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu1_power",
                "key": "PSU 1 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu1_power",
                "key": "PSU 1 POUT",
                "delim": "is",
            },
        },
        {
            "name": "PSU 2",
            "vscale": 1000.0,
            "cscale": 1000.0,
            "pscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu_present",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu_status",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "power Good",
            },
            "warning": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu_warning",
                "key": "PSU 2",
                "delim": "is",
                "cmp": "warning",
            },
            "vin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu2_power",
                "key": "PSU 2 VIN",
                "delim": "is",
            },
            "vout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu2_power",
                "key": "PSU 2 VOUT",
                "delim": "is",
            },
            "cin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu2_power",
                "key": "PSU 2 IIN",
                "delim": "is",
            },
            "cout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu2_power",
                "key": "PSU 2 IOUT",
                "delim": "is",
            },
            "pin": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu2_power",
                "key": "PSU 2 PIN",
                "delim": "is",
            },
            "pout": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_PSU/psu2_power",
                "key": "PSU 2 POUT",
                "delim": "is",
            },
        },
    ]
    PSU_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    PSU_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    PSU_INFO[HWSKU_CX864EN][ASIC_TL10E04] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    PSU_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E04] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    PSU_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E04] = _copy.deepcopy(
        PSU_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )

    ################################
    #           THERMAL            #
    ################################

    ################
    # CX308P-48Y-N #
    ################
    THERMAL_INFO[HWSKU_CX308P48YN] = {}
    THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E00] = [
        {
            "name": "BMC Sensor 1",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_Sensor/sensor_status",
                "key": "Sensor 1",
                "delim": "is",
                "cmp": "OK",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_BMC/bmc_sersor_1",
                "key": "Sensor 1",
                "delim": "is",
                "trail": "degrees (C)",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "BMC Sensor 2",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_Sensor/sensor_status",
                "key": "Sensor 2",
                "delim": "is",
                "cmp": "OK",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_BMC/bmc_sersor_2",
                "key": "Sensor 2",
                "delim": "is",
                "trail": "degrees (C)",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "BMC Sensor 3",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_Sensor/sensor_status",
                "key": "Sensor 3",
                "delim": "is",
                "cmp": "OK",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_BMC/bmc_sersor_3",
                "key": "Sensor 3",
                "delim": "is",
                "trail": "degrees (C)",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "BMC Sensor 4",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_Sensor/sensor_status",
                "key": "Sensor 4",
                "delim": "is",
                "cmp": "OK",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_BMC/bmc_sersor_4",
                "key": "Sensor 4",
                "delim": "is",
                "trail": "degrees (C)",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E01] = [
        {
            "name": "Mainboard Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_THERMAL/mainboard_left",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_THERMAL/mainboard_left",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Mainboard Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_THERMAL/mainboard_right",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_THERMAL/mainboard_right",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_THERMAL/fan_board_left",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_THERMAL/fan_board_left",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_THERMAL/fan_board_right",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_THERMAL/fan_board_right",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E02] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E03] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    THERMAL_INFO[HWSKU_CX308P48YN][ASIC_FL00E01] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    THERMAL_INFO[HWSKU_CX308P48YN][ASIC_FL00E02] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    THERMAL_INFO[HWSKU_CX308P48YN][ASIC_FL00E03] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )

    ############
    # CX532P-N #
    ############
    THERMAL_INFO[HWSKU_CX532PN] = {}
    THERMAL_INFO[HWSKU_CX532PN][ASIC_TL05E00] = [
        {
            "name": "Left Bottom Back",
            "tscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_b",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_b",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_b_max",
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_b_crit",
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_b_min",
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_b_lcrit",
            },
        },
        {
            "name": "Left Bottom Front",
            "tscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_f",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_f",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_f_max",
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_f_crit",
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_f_min",
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_b_f_lcrit",
            },
        },
        {
            "name": "Left Top Back",
            "tscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_b",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_b",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_b_max",
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_b_crit",
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_b_min",
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_b_lcrit",
            },
        },
        {
            "name": "Left Top Front",
            "tscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_f",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_f",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_f_max",
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_f_crit",
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_f_min",
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_l_t_f_lcrit",
            },
        },
        {
            "name": "Right Bottom Back",
            "tscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_b",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_b",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_b_max",
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_b_crit",
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_b_min",
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_b_lcrit",
            },
        },
        {
            "name": "Right Bottom Front",
            "tscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_f",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_f",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_f_max",
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_f_crit",
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_f_min",
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_b_f_lcrit",
            },
        },
        {
            "name": "Right Top Back",
            "tscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_b",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_b",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_b_max",
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_b_crit",
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_b_min",
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_b_lcrit",
            },
        },
        {
            "name": "Right Top Front",
            "tscale": 1000.0,
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_f",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_f",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_f_max",
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_f_crit",
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_f_min",
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp_r_t_f_lcrit",
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX532PN][ASIC_TL07E01] = [
        {
            "name": "Mainboard Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/cpu_l_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/cpu_l_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Mainboard Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/cpu_r_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/cpu_r_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/fan_1_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/fan_1_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/fan_2_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/fan_2_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Innovium Ambient",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Innovium Junction",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/main_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/main_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX532PN][ASIC_TL07E02] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    THERMAL_INFO[HWSKU_CX532PN][ASIC_TL07E03] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    THERMAL_INFO[HWSKU_CX532PN][ASIC_FL00E02] = [
        {
            "name": "Mainboard Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/cpu_l_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/cpu_l_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Mainboard Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/cpu_r_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/cpu_r_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/fan_1_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/fan_1_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/fan_2_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/fan_2_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Marvell Ambient",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_THERMAL/temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX532PN][ASIC_FL00E03] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )

    ############
    # CX564P-N #
    ############
    THERMAL_INFO[HWSKU_CX564PN] = {}
    THERMAL_INFO[HWSKU_CX564PN][ASIC_TL07E01] = [
        {
            "name": "Mainboard Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/cpu_l_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/cpu_l_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Mainboard Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/cpu_r_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/cpu_r_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/fan_1_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/fan_1_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/fan_2_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/fan_2_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Innovium Junction",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/main_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_THERMAL/main_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX564PN][ASIC_TL07E02] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )
    THERMAL_INFO[HWSKU_CX564PN][ASIC_TL07E03] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )

    ############
    # CX664D-N #
    ############
    THERMAL_INFO[HWSKU_CX664DN] = {}
    THERMAL_INFO[HWSKU_CX664DN][ASIC_TL07E01] = [
        {
            "name": "Mainboard Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/cpu_l_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/cpu_l_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Mainboard Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/cpu_r_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/cpu_r_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/fan_1_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/fan_1_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/fan_2_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/fan_2_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Innovium Junction",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/main_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_THERMAL/main_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX664DN][ASIC_TL07E02] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )
    THERMAL_INFO[HWSKU_CX664DN][ASIC_TL07E03] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )

    ############
    # CX732Q-N #
    ############
    THERMAL_INFO[HWSKU_CX732QN] = {}
    THERMAL_INFO[HWSKU_CX732QN][ASIC_TL07E01] = [
        {
            "name": "Mainboard Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/cpu_l_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/cpu_l_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Mainboard Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/cpu_r_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/cpu_r_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/fan_1_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/fan_1_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/fan_2_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/fan_2_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Innovium Junction",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/main_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_THERMAL/main_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX732QN][ASIC_TL07E02] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )
    THERMAL_INFO[HWSKU_CX732QN][ASIC_TL07E03] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )

    ############
    # CX864E-N #
    ############
    THERMAL_INFO[HWSKU_CX864EN] = {}
    THERMAL_INFO[HWSKU_CX864EN_128X400G] = {}
    THERMAL_INFO[HWSKU_CX864EN_64X400G_32X800G] = {}
    THERMAL_INFO[HWSKU_CX864EN][ASIC_TL10E02] = [
        {
            "name": "Mainboard Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/cpu_l_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/cpu_l_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Mainboard Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/cpu_r_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/cpu_r_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Left",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/fan_1_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/fan_1_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Fan Board Right",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/fan_2_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/fan_2_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
        {
            "name": "Innovium Junction",
            "presence": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": "present",
                "cmp": "present",
            },
            "status": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/main_temp",
                "revcmp": "0",
            },
            "temp": {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_THERMAL/main_temp",
            },
            "high": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_HIGH,
            },
            "chigh": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_HIGH,
            },
            "low": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_LOW,
            },
            "clow": {
                "type": SYSFS_MATCH_TYPE_VALUE,
                "value": THERMAL_TEMP_CRIT_LOW,
            },
        },
    ]
    THERMAL_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E02] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    THERMAL_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    THERMAL_INFO[HWSKU_CX864EN][ASIC_TL10E04] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    THERMAL_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E04] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    THERMAL_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E04] = _copy.deepcopy(
        THERMAL_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )

    ################################
    #            SENSOR            #
    ################################

    ################
    # CX308P-48Y-N #
    ################
    VOLTAGE_INFO[HWSKU_CX308P48YN] = {}
    VOLTAGE_INFO[HWSKU_CX308P48YN][ASIC_TL05E00] = []
    VOLTAGE_INFO[HWSKU_CX308P48YN][ASIC_TL05E01] = []
    VOLTAGE_INFO[HWSKU_CX308P48YN][ASIC_TL05E02] = []
    VOLTAGE_INFO[HWSKU_CX308P48YN][ASIC_TL05E03] = []
    VOLTAGE_INFO[HWSKU_CX308P48YN][ASIC_FL00E01] = []
    VOLTAGE_INFO[HWSKU_CX308P48YN][ASIC_FL00E02] = []
    VOLTAGE_INFO[HWSKU_CX308P48YN][ASIC_FL00E03] = []

    ############
    # CX532P-N #
    ############
    VOLTAGE_INFO[HWSKU_CX532PN] = {}
    VOLTAGE_INFO[HWSKU_CX532PN][ASIC_TL05E00] = []
    VOLTAGE_INFO[HWSKU_CX532PN][ASIC_TL07E01] = []
    VOLTAGE_INFO[HWSKU_CX532PN][ASIC_TL07E02] = []
    VOLTAGE_INFO[HWSKU_CX532PN][ASIC_TL07E03] = []
    VOLTAGE_INFO[HWSKU_CX532PN][ASIC_FL00E02] = []
    VOLTAGE_INFO[HWSKU_CX532PN][ASIC_FL00E03] = []

    ############
    # CX564P-N #
    ############
    VOLTAGE_INFO[HWSKU_CX564PN] = {}
    VOLTAGE_INFO[HWSKU_CX564PN][ASIC_TL07E01] = []
    VOLTAGE_INFO[HWSKU_CX564PN][ASIC_TL07E02] = []
    VOLTAGE_INFO[HWSKU_CX564PN][ASIC_TL07E03] = []

    ############
    # CX664D-N #
    ############
    VOLTAGE_INFO[HWSKU_CX664DN] = {}
    VOLTAGE_INFO[HWSKU_CX664DN][ASIC_TL07E01] = []
    VOLTAGE_INFO[HWSKU_CX664DN][ASIC_TL07E02] = []
    VOLTAGE_INFO[HWSKU_CX664DN][ASIC_TL07E03] = []

    ############
    # CX732Q-N #
    ############
    VOLTAGE_INFO[HWSKU_CX732QN] = {}
    VOLTAGE_INFO[HWSKU_CX732QN][ASIC_TL07E01] = []
    VOLTAGE_INFO[HWSKU_CX732QN][ASIC_TL07E02] = []
    VOLTAGE_INFO[HWSKU_CX732QN][ASIC_TL07E03] = []

    ############
    # CX864E-N #
    ############
    VOLTAGE_INFO[HWSKU_CX864EN] = {}
    VOLTAGE_INFO[HWSKU_CX864EN][ASIC_TL10E02] = []
    VOLTAGE_INFO[HWSKU_CX864EN][ASIC_TL10E04] = []

    ################################
    #             SFP              #
    ################################
    PLATFORM_INSTALL_DATA = _json.loads(
        _Path(PLATFORM_INSTALL_JSON_PATH).read_text(), object_pairs_hook=_OrderedDict
    )
    SFP_EEPROM_PATHS = []  # type: list[str]
    for platform_install_info in PLATFORM_INSTALL_DATA:
        for key in platform_install_info.keys():
            if "SFP-G" in key:
                SFP_EEPROM_PATHS = list(
                    _chain.from_iterable(
                        map(
                            lambda sfp_info: map(
                                lambda path: _Path(path, "eeprom").as_posix(),
                                sfp_info.get("paths", []),
                            ),
                            platform_install_info.values(),
                        )
                    )
                )

    ################
    # CX308P-48Y-N #
    ################
    CX308P48YN_PORT_SFP_START = 0
    CX308P48YN_PORT_QSFP_START = 48
    CX308P48YN_PORT_END = 56
    SFP_INFO[HWSKU_CX308P48YN] = {}
    SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E00] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if CX308P48YN_PORT_SFP_START <= _sfp_index < CX308P48YN_PORT_QSFP_START:
            sfp_info["name"] = "Ethernet{}".format(_sfp_index)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_SFP/SFP_present",
                "key": "SFP {:02d}".format(_sfp_index + 1),
                "delim": "is",
                "cmp": "present",
            }
            sfp_info["txstat"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_SFP/SFP_tx_stat",
                "key": "SFP {:02d}".format(_sfp_index + 1),
                "delim": "SFP {:02d}".format(_sfp_index + 1),
                "cmp": "Enable TX",
            }
            sfp_info["rxstat"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_SFP/SFP_rx_loss",
                "key": "SFP {:02d}".format(_sfp_index + 1),
                "delim": "SFP {:02d}".format(_sfp_index + 1),
                "cmp": "signal detected",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_SFP/SFP_tx_ctrl_{}".format(_sfp_index + 1),
                "on": "0",
                "off": "1",
            }
        elif CX308P48YN_PORT_QSFP_START <= _sfp_index < CX308P48YN_PORT_END:
            sfp_info["name"] = "Ethernet{}".format(
                CX308P48YN_PORT_QSFP_START
                + (_sfp_index - CX308P48YN_PORT_QSFP_START) * 4
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_QSFP/QSFP_present",
                "key": "QSFP {:02d}".format(
                    _sfp_index + 1 - CX308P48YN_PORT_QSFP_START
                ),
                "delim": "is",
                "cmp": "present",
            }
            sfp_info["lpstat"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_QSFP/QSFP_low_power_all",
                "key": "QSFP {:02d}".format(
                    _sfp_index + 1 - CX308P48YN_PORT_QSFP_START
                ),
                "delim": ":",
                "cmp": "ON",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["lpmode"] = {
                "attr": "rw",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_QSFP/QSFP_low_power_{}".format(
                    _sfp_index + 1 - CX308P48YN_PORT_QSFP_START
                ),
                "on": "1",
                "off": "0",
            }
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_QSFP/QSFP_reset",
                "on": str(_sfp_index + 1 - CX308P48YN_PORT_QSFP_START),
                "off": "0",
            }
        SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E00].append(sfp_info)
    SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E01] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if CX308P48YN_PORT_SFP_START <= _sfp_index < CX308P48YN_PORT_QSFP_START:
            sfp_info["name"] = "Ethernet{}".format(_sfp_index)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_SFP/sfp{}_present".format(_sfp_index + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_SFP/sfp{}_tx_disable".format(_sfp_index + 1),
                "on": "1",
                "off": "0",
            }
        elif CX308P48YN_PORT_QSFP_START <= _sfp_index < CX308P48YN_PORT_END:
            sfp_info["name"] = "Ethernet{}".format(
                CX308P48YN_PORT_QSFP_START
                + (_sfp_index - CX308P48YN_PORT_QSFP_START) * 4
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_QSFP/qsfp{}_present".format(
                    _sfp_index + 1 - CX308P48YN_PORT_QSFP_START
                ),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX308P_QSFP/qsfp{}_reset".format(
                    _sfp_index + 1 - CX308P48YN_PORT_QSFP_START
                ),
                "on": "1",
                "off": "0",
            }
        SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E01].append(sfp_info)
    SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E02] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E03] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    SFP_INFO[HWSKU_CX308P48YN][ASIC_FL00E01] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    SFP_INFO[HWSKU_CX308P48YN][ASIC_FL00E02] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )
    SFP_INFO[HWSKU_CX308P48YN][ASIC_FL00E03] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX308P48YN][ASIC_TL05E01]
    )

    ############
    # CX532P-N #
    ############
    CX532PN_PORT_QSFP_START = 0
    CX532PN_PORT_SFP_START = 32
    CX532PN_PORT_END = 34
    SFP_INFO[HWSKU_CX532PN] = {}
    SFP_INFO[HWSKU_CX532PN][ASIC_TL05E00] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if CX532PN_PORT_QSFP_START <= _sfp_index < CX532PN_PORT_SFP_START:
            sfp_info["name"] = "Ethernet{}".format(_sfp_index * 4)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_QSFP/qsfp{}_present".format(_sfp_index + 1),
                "cmp": "1",
            }
            sfp_info["lpstat"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_QSFP/qsfp{}_low_power".format(_sfp_index + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["lpmode"] = {
                "attr": "rw",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_QSFP/qsfp{}_low_power".format(_sfp_index + 1),
                "on": "1",
                "off": "0",
            }
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_QSFP/qsfp{}_reset".format(_sfp_index + 1),
                "on": "1",
                "off": "0",
            }
        elif CX532PN_PORT_SFP_START <= _sfp_index < CX532PN_PORT_END:
            continue
        SFP_INFO[HWSKU_CX532PN][ASIC_TL05E00].append(sfp_info)
    SFP_INFO[HWSKU_CX532PN][ASIC_TL07E01] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if CX532PN_PORT_QSFP_START <= _sfp_index < CX532PN_PORT_SFP_START:
            sfp_info["name"] = "Ethernet{}".format(_sfp_index * 4)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_QSFP/qsfp{}_present".format(_sfp_index + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_QSFP/qsfp{}_reset".format(_sfp_index + 1),
                "on": "1",
                "off": "0",
            }
        elif CX532PN_PORT_SFP_START <= _sfp_index < CX532PN_PORT_END:
            sfp_info["name"] = "Ethernet{}".format(
                CX532PN_PORT_SFP_START * 3 + _sfp_index
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_QSFP/sfp{}_present".format(
                    _sfp_index + 1 - CX532PN_PORT_SFP_START
                ),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX532P_QSFP/sfp{}_tx_disable".format(
                    _sfp_index + 1 - CX532PN_PORT_SFP_START
                ),
                "on": "1",
                "off": "0",
            }
        SFP_INFO[HWSKU_CX532PN][ASIC_TL07E01].append(sfp_info)
    SFP_INFO[HWSKU_CX532PN][ASIC_TL07E02] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    SFP_INFO[HWSKU_CX532PN][ASIC_TL07E03] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    SFP_INFO[HWSKU_CX532PN][ASIC_FL00E02] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )
    SFP_INFO[HWSKU_CX532PN][ASIC_FL00E03] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX532PN][ASIC_TL07E01]
    )

    ############
    # CX564P-N #
    ############
    CX564PN_PORT_QSFP_START = 0
    CX564PN_PORT_SFP_START = 64
    CX564PN_PORT_END = 66
    SFP_INFO[HWSKU_CX564PN] = {}
    SFP_INFO[HWSKU_CX564PN][ASIC_TL07E01] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if CX564PN_PORT_QSFP_START <= _sfp_index < CX564PN_PORT_SFP_START:
            sfp_info["name"] = "Ethernet{}".format(_sfp_index * 4)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_QSFP/qsfp{}_present".format(_sfp_index + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_QSFP/qsfp{}_reset".format(_sfp_index + 1),
                "on": "1",
                "off": "0",
            }
        elif CX564PN_PORT_SFP_START <= _sfp_index < CX564PN_PORT_END:
            sfp_info["name"] = "Ethernet{}".format(
                CX564PN_PORT_SFP_START * 3 + _sfp_index
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_QSFP/sfp{}_present".format(
                    _sfp_index + 1 - CX564PN_PORT_SFP_START
                ),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX564P_QSFP/sfp{}_tx_disable".format(
                    _sfp_index + 1 - CX564PN_PORT_SFP_START
                ),
                "on": "1",
                "off": "0",
            }
        SFP_INFO[HWSKU_CX564PN][ASIC_TL07E01].append(sfp_info)
    SFP_INFO[HWSKU_CX564PN][ASIC_TL07E02] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )
    SFP_INFO[HWSKU_CX564PN][ASIC_TL07E03] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX564PN][ASIC_TL07E01]
    )

    ############
    # CX664D-N #
    ############
    CX664DN_PORT_QSFPDD_START = 0
    CX664DN_PORT_SFP_START = 64
    CX664DN_PORT_END = 66
    SFP_INFO[HWSKU_CX664DN] = {}
    SFP_INFO[HWSKU_CX664DN][ASIC_TL07E01] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if CX664DN_PORT_QSFPDD_START <= _sfp_index < CX664DN_PORT_SFP_START:
            sfp_info["name"] = "Ethernet{}".format(_sfp_index * 4)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_QSFP/qsfp{}_present".format(_sfp_index + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_QSFP/qsfp{}_reset".format(_sfp_index + 1),
                "on": "1",
                "off": "0",
            }
        elif CX664DN_PORT_SFP_START <= _sfp_index < CX664DN_PORT_END:
            sfp_info["name"] = "Ethernet{}".format(
                CX664DN_PORT_SFP_START * 3 + _sfp_index
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_QSFP/sfp{}_present".format(
                    _sfp_index + 1 - CX664DN_PORT_SFP_START
                ),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX664D_QSFP/sfp{}_tx_disable".format(
                    _sfp_index + 1 - CX664DN_PORT_SFP_START
                ),
                "on": "1",
                "off": "0",
            }
        SFP_INFO[HWSKU_CX664DN][ASIC_TL07E01].append(sfp_info)
    SFP_INFO[HWSKU_CX664DN][ASIC_TL07E02] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )
    SFP_INFO[HWSKU_CX664DN][ASIC_TL07E03] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX664DN][ASIC_TL07E01]
    )

    ############
    # CX732Q-N #
    ############
    CX732QN_PORT_QSFPDD_START = 0
    CX732QN_PORT_SFP_START = 32
    CX732QN_PORT_END = 34
    SFP_INFO[HWSKU_CX732QN] = {}
    SFP_INFO[HWSKU_CX732QN][ASIC_TL07E01] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if CX732QN_PORT_QSFPDD_START <= _sfp_index < CX732QN_PORT_SFP_START:
            sfp_info["name"] = "Ethernet{}".format(_sfp_index * 8)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_QSFP/qsfp{}_present".format(_sfp_index + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_QSFP/qsfp{}_reset".format(_sfp_index + 1),
                "on": "1",
                "off": "0",
            }
        elif CX732QN_PORT_SFP_START <= _sfp_index < CX732QN_PORT_END:
            sfp_info["name"] = "Ethernet{}".format(
                CX732QN_PORT_SFP_START * 7 + _sfp_index
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_QSFP/sfp{}_present".format(
                    _sfp_index + 1 - CX732QN_PORT_SFP_START
                ),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX732Q_QSFP/sfp{}_tx_disable".format(
                    _sfp_index + 1 - CX732QN_PORT_SFP_START
                ),
                "on": "1",
                "off": "0",
            }
        SFP_INFO[HWSKU_CX732QN][ASIC_TL07E01].append(sfp_info)
    SFP_INFO[HWSKU_CX732QN][ASIC_TL07E02] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )
    SFP_INFO[HWSKU_CX732QN][ASIC_TL07E03] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX732QN][ASIC_TL07E01]
    )

    ############
    # CX864E-N #
    ############
    CX864EN_PORT_OSFP_START = 0
    CX864EN_PORT_SFP_START = 64
    CX864EN_PORT_END = 66
    SFP_INFO[HWSKU_CX864EN] = {}
    SFP_INFO[HWSKU_CX864EN][ASIC_TL10E02] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if CX864EN_PORT_OSFP_START <= _sfp_index < CX864EN_PORT_SFP_START:
            sfp_info["name"] = "Ethernet{}".format(_sfp_index * 8)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/qsfp{}_present".format(_sfp_index + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/qsfp{}_reset".format(_sfp_index + 1),
                "on": "1",
                "off": "0",
            }
        elif CX864EN_PORT_SFP_START <= _sfp_index < CX864EN_PORT_END:
            sfp_info["name"] = "Ethernet{}".format(
                CX864EN_PORT_SFP_START * 7 + _sfp_index
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/sfp{}_present".format(
                    _sfp_index + 1 - CX864EN_PORT_SFP_START
                ),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/sfp{}_tx_disable".format(
                    _sfp_index + 1 - CX864EN_PORT_SFP_START
                ),
                "on": "1",
                "off": "0",
            }
        SFP_INFO[HWSKU_CX864EN][ASIC_TL10E02].append(sfp_info)
    CX864EN_128X400G_PORT_QSFPDD_START = 0
    CX864EN_128X400G_PORT_SFP_START = 128
    CX864EN_128X400G_PORT_END = 130
    SFP_INFO[HWSKU_CX864EN_128X400G] = {}
    SFP_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E02] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if (
            CX864EN_128X400G_PORT_QSFPDD_START
            <= _sfp_index
            < CX864EN_128X400G_PORT_SFP_START
        ):
            sfp_info["name"] = "Ethernet{}".format(_sfp_index * 4)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/qsfp{}_present".format(_sfp_index // 2 + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/qsfp{}_reset".format(_sfp_index // 2 + 1),
                "on": "1",
                "off": "0",
            }
        elif CX864EN_128X400G_PORT_SFP_START <= _sfp_index < CX864EN_128X400G_PORT_END:
            sfp_info["name"] = "Ethernet{}".format(
                CX864EN_128X400G_PORT_SFP_START * 3 + _sfp_index
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/sfp{}_present".format(
                    _sfp_index + 1 - CX864EN_128X400G_PORT_SFP_START
                ),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/sfp{}_tx_disable".format(
                    _sfp_index + 1 - CX864EN_128X400G_PORT_SFP_START
                ),
                "on": "1",
                "off": "0",
            }
        SFP_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E02].append(sfp_info)
    CX864EN_64X400G_32X800G_PORT_QSFPDD_START = 0
    CX864EN_64X400G_32X800G_PORT_OSFP_START = 64
    CX864EN_64X400G_32X800G_PORT_SFP_START = 96
    CX864EN_64X400G_32X800G_PORT_END = 98
    SFP_INFO[HWSKU_CX864EN_64X400G_32X800G] = {}
    SFP_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02] = []
    for _sfp_index, sfp_eeprom_path in enumerate(SFP_EEPROM_PATHS):
        sfp_info = {}  # type: dict[str, str|dict[str, str]]
        if (
            CX864EN_64X400G_32X800G_PORT_QSFPDD_START
            <= _sfp_index
            < CX864EN_64X400G_32X800G_PORT_OSFP_START
        ):
            sfp_info["name"] = "Ethernet{}".format(_sfp_index * 4)
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/qsfp{}_present".format(_sfp_index // 2 + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/qsfp{}_reset".format(_sfp_index // 2 + 1),
                "on": "1",
                "off": "0",
            }
        elif (
            CX864EN_64X400G_32X800G_PORT_OSFP_START
            <= _sfp_index
            < CX864EN_64X400G_32X800G_PORT_SFP_START
        ):
            sfp_info["name"] = "Ethernet{}".format(
                _sfp_index * 8 - CX864EN_64X400G_32X800G_PORT_OSFP_START * 4
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/qsfp{}_present".format(_sfp_index - 32 + 1),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["reset"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/qsfp{}_reset".format(_sfp_index - 32 + 1),
                "on": "1",
                "off": "0",
            }
        elif (
            CX864EN_64X400G_32X800G_PORT_SFP_START
            <= _sfp_index
            < CX864EN_64X400G_32X800G_PORT_END
        ):
            sfp_info["name"] = "Ethernet{}".format(
                CX864EN_64X400G_32X800G_PORT_OSFP_START * 3
                + (
                    CX864EN_64X400G_32X800G_PORT_SFP_START
                    - CX864EN_64X400G_32X800G_PORT_OSFP_START
                )
                * 7
                + _sfp_index
            )
            sfp_info["presence"] = {
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/sfp{}_present".format(
                    _sfp_index + 1 - CX864EN_64X400G_32X800G_PORT_SFP_START
                ),
                "cmp": "1",
            }
            sfp_info["eeprom"] = {
                "type": SYSFS_MATCH_TYPE_EXACT,
                "path": sfp_eeprom_path,
            }
            # Writables
            sfp_info["txdis"] = {
                "attr": "wo",
                "type": SYSFS_MATCH_TYPE_FUZZY,
                "dir": SYSFS_HWMON_SUBSYS_DIR,
                "file": "device/CX864E_QSFP/sfp{}_tx_disable".format(
                    _sfp_index + 1 - CX864EN_64X400G_32X800G_PORT_SFP_START
                ),
                "on": "1",
                "off": "0",
            }
        SFP_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E02].append(sfp_info)
    SFP_INFO[HWSKU_CX864EN][ASIC_TL10E04] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    SFP_INFO[HWSKU_CX864EN_128X400G][ASIC_TL10E04] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
    SFP_INFO[HWSKU_CX864EN_64X400G_32X800G][ASIC_TL10E04] = _copy.deepcopy(
        SFP_INFO[HWSKU_CX864EN][ASIC_TL10E02]
    )
