#!/usr/bin/env python3

DEFAULT_TEMP_CRIT_RECOVER_CMD = "/sbin/reboot"
BOARD_ID_PATH = "/etc/sonic/.board_id"
BOARD_AIRFLOW_PATH = "/etc/sonic/.airflow"
SUB_VERSION_FILE = "/etc/sonic/.subversion"
PRODUCT_NAME_PATH = "/etc/sonic/.productname"
HOST_MACHINE = "/host/machine.conf"
PLATFORM_DEBUG_INIT_PATH = "/mnt/data/platform_debug_init.sh"
COMMON_DRIVER_CHECK_CONFIGS = []
FW_UPGRADE_STARTED_FLAG = "/etc/sonic/.doing_fw_upg"
WARM_UPG_FLAG = "/etc/sonic/.warm_upg_flag"
WARM_UPGRADE_STARTED_FLAG = "/etc/sonic/.doing_warm_upg"
AIRFLOW_RESULT_FILE = "/etc/sonic/.airflow"
PLUGINS_DOCKER_STARTED_FLAG = "/etc/sonic/.plugins_docker_started_flag"
avs_begin_sleep_time = 30
SET_MAC_NEED_REBOOT = True
# generate_mgmt_version parameter
MGMT_VERSION_PATH = "/tmp/.platform/mgmt_version"
SYSLOG_PREFIX = "WB_PLATFORM"
SDKCHECK_PARAMS = None

LOAD_APP_BY_COMMON = "load_process"
UNLOAD_APP_BY_COMMON = "unload_process"
LOAD_APP_BY_SUPERVISOR = "supervisor_load_process"
UNLOAD_APP_BY_SUPERVISOR = "supervisor_unload_process"
CURRENT_LOAD_APP_WAY = LOAD_APP_BY_COMMON

# firmware_upgrade parameter
UPGRADE_BY_FIRMWARE_UPGRADE_COMMON = 0
UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER = 1
UPGRADE_BY_AFU = 2
UPGRADE_BY_CUSTOM = 3

SUPPORT_UPGRADE_LIST = [
    UPGRADE_BY_FIRMWARE_UPGRADE_COMMON,
    UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER,
    UPGRADE_BY_AFU,
    UPGRADE_BY_CUSTOM
]

