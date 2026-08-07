#!/usr/bin/env python_nos

DEFAULT_TEMP_CRIT_RECOVER_CMD = "echo 1 > /sys/s3ip/system/psu_reset"
PLATFORM_INFO_DIR = "/etc/sonic/"
BMC_PLATFORM_INFO_DIR = "/etc/device/"
DFD_BOARD_ID_PATH = "/sys/module/platform_common/parameters/dfd_my_type"
BOARD_ID_PATH = BMC_PLATFORM_INFO_DIR + ".board_id"
BOARD_AIRFLOW_PATH = BMC_PLATFORM_INFO_DIR + ".airflow"
SUB_VERSION_FILE = BMC_PLATFORM_INFO_DIR + ".subversion"
PRODUCT_NAME_PATH = BMC_PLATFORM_INFO_DIR + ".productname"
HOST_MACHINE = "/host/machine.conf"
ONIE_HOST_MACHINE = "/host/machine.conf"
PLATFORM_DEBUG_INIT_PATH = "/mnt/data/platform_debug_init.sh"
COMMON_DRIVER_CHECK_CONFIGS = [
    {
        "name": "aspeed_vhub",
        "delay": 1,
        "pre_check": {
            "gettype": "sysfs",
            "loc": "/sys/s3ip/system/bmc_ready",
            "okval": 1,
        }
    },
]
FW_UPGRADE_STARTED_FLAG = BMC_PLATFORM_INFO_DIR + ".doing_fw_upg"
WARM_UPG_FLAG = BMC_PLATFORM_INFO_DIR + ".warm_upg_flag"
WARM_UPGRADE_STARTED_FLAG = BMC_PLATFORM_INFO_DIR + ".doing_warm_upg"
AIRFLOW_RESULT_FILE = BMC_PLATFORM_INFO_DIR + ".airflow"
PLUGINS_DOCKER_STARTED_FLAG = BMC_PLATFORM_INFO_DIR + ".plugins_docker_started_flag"
BAR_INFO_PATH = "/tmp/device/.bar_info"
WARM_UPGRADE_FLAG = PLATFORM_INFO_DIR+".doing_warm_upg"
SONIC_DB_DATABASE_CONFIG_PATH = "/var/run/redis/sonic-db/database_config.json"
SONIC_STATE_DB_NAME = "STATE_DB"
BMC_PATCH_KEY = "BMC_PATCH_TABLE|bmc_patch"
BMC_PATCH_COUNTDOWN_FIELD = "countdown"
BMC_CHECK_UPDATING_ENABLE = False
avs_begin_sleep_time = 0
SET_MAC_NEED_REBOOT = True
SFF_TEMP_STORE_FILE = "/tmp/.sff_temp"
# generate_mgmt_version parameter
MGMT_VERSION_PATH = "/tmp/.platform/mgmt_version"
SYSLOG_PREFIX = "WB_PLATFORM"
SDKCHECK_PARAMS = None

# product strategy
PRODUCT_STRATEGY_DEFAULT = 0
PRODUCT_STRATEGY_1       = 1
PRODUCT_STRATEGY_2       = 2

PRODUCT_STRATEGY = PRODUCT_STRATEGY_DEFAULT
EXECUTABLE_FILE_PATH = ''
EXECUTABLE_APP_PATH = ''
CONFIG_FILE_PATH = "/usr/local/bin/"
ONIE_PLATFORM_KEY = "onie_platform"
FW_UPGRADE_APP_NAME = "firmware_upgrade"
FIRMWARE_UPGRADE_CMD = EXECUTABLE_APP_PATH + FW_UPGRADE_APP_NAME + " %s 0x%x 0x%x %s %s 0x%x"
FW_UP_BIOS_APP_NAME = "fw_up_bios"
FW_UP_BIOS_CMD = EXECUTABLE_APP_PATH + FW_UP_BIOS_APP_NAME + " -c -f %s"

S3IP_SYSFS_NAME = "s3ip"
S3IP_INVALID_VALUE = "-99999999"
S3IP_LINK_TO_EXTEND = False
S3IP_LINK_TO_SYS_SWITCH = False
SET_S3IP_ROOT_MODE = False
EXTEND_S3IP_PATH = ''

ALL_FAN_ABSENT_LOG = ""

LOAD_APP_BY_COMMON = "load_process"
UNLOAD_APP_BY_COMMON = "unload_process"
LOAD_APP_BY_SUPERVISOR = "supervisor_load_process"
UNLOAD_APP_BY_SUPERVISOR = "supervisor_unload_process"
CURRENT_LOAD_APP_WAY = LOAD_APP_BY_COMMON

# sonic reboot cause file path
SONIC_REBOOT_CAUSE_PATH = "/host/reboot-cause/reboot-cause.txt"

REBOOT_CAUSE_NON_HARDWARE = "Non-Hardware"
REBOOT_CAUSE_THERMAL_OVERLOAD_CPU = "Thermal Overload: CPU"
REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC = "Thermal Overload: ASIC"
REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER = "Thermal Overload: Other"
REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED = "Insufficient Fan Speed"
REBOOT_CAUSE_CPU_COLD_RESET = "CPU Cold Reset"
REBOOT_CAUSE_CPU_WARM_RESET = "CPU Warm Reset"
REBOOT_CAUSE_BIOS_RESET = "BIOS"
REBOOT_CAUSE_PSU_SHUTDOWN = "PSU Shutdown"
REBOOT_CAUSE_BMC_SHUTDOWN = "BMC Shutdown"
REBOOT_CAUSE_BMC_SHUTDOWN_ON_OFF = "BMC Shutdown(off/on)"
REBOOT_CAUSE_RESET_BUTTON_SHUTDOWN = "Reset Button Shutdown"
REBOOT_CAUSE_RESET_BUTTON_COLD_SHUTDOWN = "Reset Button Cold Reboot"
REBOOT_CAUSE_POWER_LOSS = "Power Loss"
REBOOT_CAUSE_WATCHDOG = "Watchdog"
REBOOT_CAUSE_HARDWARE_OTHER = "Hardware - Other"

REBOOT_CAUSE_BMC_DUAL_BOOT_RESET = "BMC Dual Boot Switch"
REBOOT_CAUSE_BMC_SHUTDOWN_RESET = "Power Reset, The last reboot is BMC set CPU warm reset"
REBOOT_CAUSE_BMC_SHUTDOWN_CYCLE = "Power Cycle, The last reboot is BMC or SWITCH set CPU cold reset or Push power button"
REBOOT_CAUSE_BMC_SYSDOG_REBOOT_TIMEOUT = "BMC Sysdog Reboot Timeout"
REBOOT_CAUSE_BMC_SYSDOG_HANG_TIMEOUT = "BMC Sysdog Hang Timeout"

REBOOT_CAUSE_STR2INT = {
    REBOOT_CAUSE_NON_HARDWARE: 0,
    REBOOT_CAUSE_POWER_LOSS: 1,
    REBOOT_CAUSE_THERMAL_OVERLOAD_CPU: 2,
    REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC: 3,
    REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER: 4,
    REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED: 5,
    REBOOT_CAUSE_WATCHDOG: 6,
    REBOOT_CAUSE_HARDWARE_OTHER: 7,
    REBOOT_CAUSE_CPU_COLD_RESET: 8,
    REBOOT_CAUSE_CPU_WARM_RESET: 9,
    REBOOT_CAUSE_BIOS_RESET: 10,
    REBOOT_CAUSE_PSU_SHUTDOWN: 11,
    REBOOT_CAUSE_BMC_SHUTDOWN: 12,
    REBOOT_CAUSE_RESET_BUTTON_SHUTDOWN: 13,
    REBOOT_CAUSE_RESET_BUTTON_COLD_SHUTDOWN: 14,
    REBOOT_CAUSE_BMC_DUAL_BOOT_RESET: 15,
    REBOOT_CAUSE_BMC_SHUTDOWN_RESET: 16,
    REBOOT_CAUSE_BMC_SHUTDOWN_CYCLE: 17,
    REBOOT_CAUSE_BMC_SHUTDOWN_ON_OFF: 18,
    REBOOT_CAUSE_BMC_SYSDOG_REBOOT_TIMEOUT : 19,
    REBOOT_CAUSE_BMC_SYSDOG_HANG_TIMEOUT : 20,
    # adapt old product
    "BMC reboot": 7,
    "BMC powerdown": 12,
    "Other": 0,
}

REBOOT_CAUSE_PATH_DIR = "/tmp/sonic/.reboot/"
# bsp reboot cause file path
REBOOT_TYPE_PATH_BASE = "/tmp/sonic/.reboot/"
REBOOT_CAUSE_PATH = REBOOT_CAUSE_PATH_DIR+".previous-reboot-cause.txt"
REBOOT_TYPE_PATH = REBOOT_TYPE_PATH_BASE + ".reboot_type"
SONIC_REBOOT_CAUSE_MATCH_LIST = [
    {
        "match_str": "reboot",
        "record_msg": "reboot",
        "reboot_cause": REBOOT_CAUSE_NON_HARDWARE,
    },
    {
        "match_str": "Kernel Panic",
        "record_msg": "Kernel Panic",
        "reboot_cause": REBOOT_CAUSE_NON_HARDWARE,
    },
]
PLATFORM_REBOOT_HISTORY_FILE = "/etc/.platform_reboot_history"

S3IP_DEBUG_LINK = False
S3IP_STANDARD_PATTERN_DEBUG_SUPPORT = True
S3IP_STANDARD_PATTERN_DEBUG_NOT_SUPPORT = False
S3IP_SENSOR_DEBUG_LINK_MAP = ["vol_sensor", "curr_sensor"]
S3IP_STANDARD_PATTERN = [
    f"/sys/{S3IP_SYSFS_NAME}/sysled/sys_led_status" ,
    f"/sys/{S3IP_SYSFS_NAME}/sysled/bmc_led_status",
    f"/sys/{S3IP_SYSFS_NAME}/sysled/fan_led_status",
    f"/sys/{S3IP_SYSFS_NAME}/sysled/psu_led_status",
    f"/sys/{S3IP_SYSFS_NAME}/sysled/id_led_status",

    f"/sys/{S3IP_SYSFS_NAME}/watchdog/identify",
    f"/sys/{S3IP_SYSFS_NAME}/watchdog/timeleft",
    f"/sys/{S3IP_SYSFS_NAME}/watchdog/timeout",

    f"/sys/{S3IP_SYSFS_NAME}/syseeprom",

    f"/sys/{S3IP_SYSFS_NAME}/psu/number",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/model_name",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/present",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/out_status",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/in_status",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/serial_number",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/part_number",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/type",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/in_curr",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/in_vol",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/in_power",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/out_max_power",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/out_curr",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/out_vol",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/out_power",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/num_temp_sensors",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/fan_speed",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/led_status",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/temp_sensor/number",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/temp*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/temp*/type",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/temp*/max",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/temp*/min",
    f"/sys/{S3IP_SYSFS_NAME}/psu/psu*/temp*/value",

    f"/sys/{S3IP_SYSFS_NAME}/transceiver/power_on",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/power_on",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/tx_fault",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/tx_disable",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/present",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/rx_los",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/reset",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/low_power_mode",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/interrupt",
    f"/sys/{S3IP_SYSFS_NAME}/transceiver/eth*/eeprom",

    f"/sys/{S3IP_SYSFS_NAME}/fpga/number",
    f"/sys/{S3IP_SYSFS_NAME}/fpga/fpga*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/fpga/fpga*/type",
    f"/sys/{S3IP_SYSFS_NAME}/fpga/fpga*/firmware_version",
    f"/sys/{S3IP_SYSFS_NAME}/fpga/fpga*/board_version",
    f"/sys/{S3IP_SYSFS_NAME}/fpga/fpga*/reg_test",

    f"/sys/{S3IP_SYSFS_NAME}/cpld/number",
    f"/sys/{S3IP_SYSFS_NAME}/cpld/cpld*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/cpld/cpld*/type",
    f"/sys/{S3IP_SYSFS_NAME}/cpld/cpld*/firmware_version",
    f"/sys/{S3IP_SYSFS_NAME}/cpld/cpld*/board_version",
    f"/sys/{S3IP_SYSFS_NAME}/cpld/cpld*/reg_test",

    f"/sys/{S3IP_SYSFS_NAME}/temp_sensor/number",
    f"/sys/{S3IP_SYSFS_NAME}/temp_sensor/temp*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/temp_sensor/temp*/type",
    f"/sys/{S3IP_SYSFS_NAME}/temp_sensor/temp*/max",
    f"/sys/{S3IP_SYSFS_NAME}/temp_sensor/temp*/min",
    f"/sys/{S3IP_SYSFS_NAME}/temp_sensor/temp*/value",

    f"/sys/{S3IP_SYSFS_NAME}/vol_sensor/number",
    f"/sys/{S3IP_SYSFS_NAME}/vol_sensor/vol*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/vol_sensor/vol*/type",
    f"/sys/{S3IP_SYSFS_NAME}/vol_sensor/vol*/max",
    f"/sys/{S3IP_SYSFS_NAME}/vol_sensor/vol*/min",
    f"/sys/{S3IP_SYSFS_NAME}/vol_sensor/vol*/range",
    f"/sys/{S3IP_SYSFS_NAME}/vol_sensor/vol*/nominal_value",
    f"/sys/{S3IP_SYSFS_NAME}/vol_sensor/vol*/value",

    f"/sys/{S3IP_SYSFS_NAME}/curr_sensor/number",
    f"/sys/{S3IP_SYSFS_NAME}/curr_sensor/curr*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/curr_sensor/curr*/type",
    f"/sys/{S3IP_SYSFS_NAME}/curr_sensor/curr*/max",
    f"/sys/{S3IP_SYSFS_NAME}/curr_sensor/curr*/min",
    f"/sys/{S3IP_SYSFS_NAME}/curr_sensor/curr*/value",

    f"/sys/{S3IP_SYSFS_NAME}/slot/number",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/model_name",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/hardware_version",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/serial_number",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/part_number",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/status",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/led_status",

    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/num_temp_sensors",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/temp*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/temp*/type",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/temp*/max",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/temp*/min",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/temp*/value",

    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/num_vol_sensors",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/vol*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/vol*/type",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/vol*/max",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/vol*/min",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/vol*/range",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/vol*/nominal_value",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/vol*/value",

    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/num_curr_sensors",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/curr*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/curr*/type",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/curr*/max",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/curr*/min",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/curr*/value",

    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/num_fpgas",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/fpga*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/fpga*/type",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/fpga*/firmware_version",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/fpga*/board_version",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/fpga*/reg_test",

    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/num_cplds",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/cpld*/alias",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/cpld*/type",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/cpld*/firmware_version",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/cpld*/board_version",
    f"/sys/{S3IP_SYSFS_NAME}/slot/slot*/cpld*/reg_test",

    f"/sys/{S3IP_SYSFS_NAME}/fan/number",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/model_name",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/serial_number",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/part_number",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/hardware_version",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/motor_number",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/direction",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/ratio",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/status",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/led_status",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/motor*/speed_min",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/motor*/speed_max",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/motor*/speed_tolerance",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/motor*/speed_target",
    f"/sys/{S3IP_SYSFS_NAME}/fan/fan*/motor*/speed",
]

CUSTOM_DEFINED_S3IP_PATTERN = []

CUSTOM_DEFINE_EXTEND_S3IP_PATTERN = []

CUSTOM_ONE2ONE_S3IP_DEBUG_MAP = []

CUSTOM_S3IP_PRESENT_DEBUG_BASE_PATH = "/sys_switch/debug/present_on/"
CUSTOM_SENSOR_S3IP_DEBUG_BATH_PATH = "/sys_switch/debug/threshold/"
CUSTOM_SENSOR_S3IP_DEBUG_MAP = []

S3IP_PRESENT_DEBUG_LINK_MAP = []

# firmware_upgrade parameter
UPGRADE_BY_FIRMWARE_UPGRADE_COMMON = 0
UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER = 1
UPGRADE_BY_AFU = 2
UPGRADE_BY_CUSTOM = 3
UPGRADE_BY_HDPARM = 4
UPGRADE_BY_VR_SCRIPT = 5
UPGRADE_BY_NVME = 6
UPGRADE_BY_NVMUPDATE = 7
UPGRADE_BY_CX7_SCRIPT = 8
UPGRADE_BY_FW_UP_BIOS = 9


NVME_SSD_UPDATE_WAY_INVAILD = -1
NVME_SSD_UPDATE_WAY_INNODISK = 0
NVME_SSD_UPDATE_WAY_LIST = [
    NVME_SSD_UPDATE_WAY_INNODISK,
]

SUPPORT_UPGRADE_LIST = [
    UPGRADE_BY_FIRMWARE_UPGRADE_COMMON,
    UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER,
    UPGRADE_BY_AFU,
    UPGRADE_BY_CUSTOM,
    UPGRADE_BY_HDPARM,
    UPGRADE_BY_VR_SCRIPT,
    UPGRADE_BY_NVME,
    UPGRADE_BY_NVMUPDATE,
    UPGRADE_BY_CX7_SCRIPT,
    UPGRADE_BY_FW_UP_BIOS
]

SET_ETH_MAC_REDHAT_CMD = 0
SET_ETH_MAC_CFG_TYPE = [
    SET_ETH_MAC_REDHAT_CMD,
]

SERVICE_LIST = [
    "platform_base.service",
    "platform_driver.service",
    "platform_process.service",
    "set-eth-mac.service",
    "platform_driver_late.service",
    "platform_driver_sh.service"
]
