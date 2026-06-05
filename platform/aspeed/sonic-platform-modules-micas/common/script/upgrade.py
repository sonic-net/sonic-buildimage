#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import sys
import os
import time
import syslog
import signal
import click
import traceback
import logging
import re
import tempfile
from platform_util import AliasedGroup, CONTEXT_SETTINGS, get_value, set_value, exec_os_cmd, exec_os_cmd_log, write_sysfs, check_value, setup_logger, BSP_COMMON_LOG_DIR
from platform_util import parse_file_head, do_fw_upg_raw_file_generate, MAX_HEADER_SIZE, get_vr_chip_id
from platform_config import UPGRADE_SUMMARY, WARM_UPGRADE_PARAM, WARM_UPGRADE_STARTED_FLAG, FW_UPGRADE_STARTED_FLAG
from warm_upgrade import WarmBasePlatform
from wbutil.baseutil import get_board_id
from time import monotonic as _time
import shutil
from public.platform_common_config import UPGRADE_BY_FIRMWARE_UPGRADE_COMMON, UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER, UPGRADE_BY_AFU, UPGRADE_BY_CUSTOM, SUPPORT_UPGRADE_LIST, UPGRADE_BY_HDPARM, UPGRADE_BY_NVME, UPGRADE_BY_FW_UP_BIOS
from public.platform_common_config import UPGRADE_BY_VR_SCRIPT, UPGRADE_BY_NVMUPDATE, EXECUTABLE_FILE_PATH, EXECUTABLE_APP_PATH, FIRMWARE_UPGRADE_CMD, UPGRADE_BY_CX7_SCRIPT, FW_UPGRADE_APP_NAME, NVME_SSD_UPDATE_WAY_INVAILD, FW_UP_BIOS_CMD
from psu_upgrade import PsuUpgrade


############################# Error code defined #############################
ERR_FW_CHECK_CPLD_UPGRADE = -601    # "Failed to check the device CPLD information"
ERR_FW_CHECK_FPGA_UPGRADE = -602    # "Failed to check the device FPGA information"
ERR_FW_MATCH_CPLD_UPGRADE = -603    # "Not found upgrade CPLD file."
ERR_FW_MATCH_FPGA_UPGRADE = -604    # "Not found upgrade FPGA file."
ERR_FW_SAMEVER_CPLD_UPGRADE = -605    # "The CPLD version in device is same"
ERR_FW_SAMEVER_FPGA_UPGRADE = -606    # "The FPGA version in device is same"
ERR_FW_DO_CPLD_UPGRADE = -607    # "Doing upgrade CPLD is failed."
ERR_FW_DO_FPGA_UPGRADE = -608    # "Doing upgrade FPGA is failed."
ERR_FW_UPGRADE = -609    # "Failed to upgrade firmware"
FIRMWARE_PROGRAM_EXEC_ERR = -610    # "Firmware program run error!"
ERR_FW_FILE_FOUND = -701    # "Failed to find upgrade file"
ERR_FW_HEAD_PARSE = -702    # "Failed to parse upgrade firmware head info"
ERR_FW_CONFIG_FOUND = -703    # "Failed to find config item"
ERR_FW_NOSUPPORT_HOT = -704    # "No support hot upgrade"
ERR_FW_CHECK_SIZE = -705    # "Failed to check file size"
ERR_FW_DEVICE_ACCESS = -706    # "Failed to access device"
ERR_FW_NO_FILE_SUCCESS = -707    # "No files were successfully upgraded"
ERR_FW_CARD_ABSENT = -708    # "The subcard not present"
ERR_FW_HEAD_CHECK = -709    # "Failed to check head info"
ERR_FW_FOOL_PROOF = -710    # "Failed to fool proof verification"
ERR_FW_RAISE_EXCEPTION = -711    # Code raise exception
ERR_FW_INVALID_PARAM = -712    # Invalid parameter
ERR_FW_UNZIP_FAILED = -713    # Unzip firmware failed
ERR_FW_MULTI_TYPE_NAME = -714    # Multiple matches found for type
ERR_FW_NO_TYPE_FOUND = -715    # No type found
ERR_FW_MULTI_CHAIN_CHECK = -716    # Multiple chain check error

FIRMWARE_SUCCESS = 0
CHECK_OK = 0

FIlE_WITH_HEADER = 1
FIlE_WITHOUT_HEADER = 2

UPGRADE_FILE_DIR = "/tmp/firmware/"
UPGRADE_ONIE_TOOL = EXECUTABLE_FILE_PATH + "sonic_update_onie.sh"
DEBUG_FILE = "/etc/.upgrade_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "upgrade_debug.log"
logger = setup_logger(LOG_FILE)

COLD_UPGRADE = 1
WARM_UPGRADE = 2
TEST_UPGRADE = 3
BMC_UPGRADE = 4
BIOS_UPGRADE = 5
ONIE_UPGRADE = 6

CHANEL_LPC = "lpc"
CHANEL_USB = "usb"
CHANEL_REDFISH = "redfish"

BMC_UPGRADE_SUPPORT_CHANEL_LIST = [CHANEL_LPC, CHANEL_USB, CHANEL_REDFISH]

CHIP_MASTER = "0"
CHIP_SLAVE = "1"
CHIP_BOTH = "2"
BMC_UPGRADE_SUPPORT_CHIP_LIST = [CHIP_MASTER, CHIP_SLAVE, CHIP_BOTH]

ERASE_PART = "part"
ERASE_FULL = "full"
BMC_UPGRADE_SUPPORT_ERASE_TYPE_LIST = [ERASE_PART, ERASE_FULL]
ERASE_FULL_CODE_STR = "0x01"

BMC_MASTER = "master"
BMC_SLAVE = "slave"
BMC_BOTH = "both"

LPC_BMC_FLASH_RW_ENABLE = "/sys/logic_dev/lpc_bmc/flash_rw_enable"
LPC_BMC_FLASH_ERASE_FULL = "/sys/logic_dev/lpc_bmc/flash_erase_full"
LPC_BMC_RESET = "/sys/logic_dev/lpc_bmc/reset_bmc"
LPC_BMC_MASTER_FLASH_DEV = "/dev/bmc_flash_master"
LPC_BMC_SLAVE_FLASH_DEV = "/dev/bmc_flash_slave"
LPC_BMC_FLASH_PRE_WR_SIZE = 0x20000

MAIN_CARD = 0
CHILDREN_CARD = 1

SUPPORT_FILE_TYPE = ("VME", "SYSFS", "SPI-LOGIC-DEV", "MTD", "ISC", "JBI", "VME-I2C", "SYSFS-XDPE132", "BASH")

FW_NAME_CLICK_HELP = "Specify the name of firmware.\n"
FW_REFRESH_NAME_CLICK_HELP = "Specify the name to refresh firmware.\n"
FW_REFRESH_CONFIG = {}
FW_REFRESH_CONFIG_ERRMSG = ""
FW_REFRESH_ALL = "ALL"
ERASE_MAX_SIZE_BYTES = 64 * 1024 * 1024
BIOS_CONFIG_STR = "BIOS"
#Flag indicating whether user configuration should be preserved during BIOS upgrade
COLD_UPGRADE_KEEP_USER_CONFIG = False

REDFISH_UPGRADE_BMC_FILE = "/tmp/bmc-new.tar"
REDFISH_UPGRADE_BMC_CMD = '''
curl -k -u root:root -H "Content-Type:multipart/form-data" -X POST \
-F UpdateParameters="{\\\"Targets\\\":[\\\"/redfish/v1/UpdateService/FirmwareInventory/BMC\\\"],\
\\\"@Redfish.OperationApplyTime\\\":\\\"Immediate\\\", \\\"Oem\\\":{\\\"Public\\\":{\\\"ConfigToDefaults\\\":%s, \
\\\"DualImage\\\":%s, \\\"ForceUpgrade\\\":true, \\\"CheckSameVersion\\\":false}}};type=application/json" \
-F "UpdateFile=@%s;type=application/octet-stream" \
https://240.1.1.2/redfish/v1/UpdateService/update-multipart
'''

AFU_UPGRADE_CMD = EXECUTABLE_FILE_PATH + "afulnx_64 %s /P /B /N /L /K /RLC:E"
HDPARM_UPGRADE_CMD = "hdparm --yes-i-know-what-i-am-doing --please-destroy-my-drive --fwdownload %s /dev/%s"
VR_UPGRADE_CMD = EXECUTABLE_FILE_PATH + "vr_fw_upg.py upgrade %s %d 0x%02x"
NVME_SSD_UPGRADE_CMD = EXECUTABLE_FILE_PATH + "nvme_upgrade.py %d %s %s"
NVMUPDATE_UPGRADE_CMD = EXECUTABLE_FILE_PATH + "nvmupdate_upgrade.py %s %s %s"
RFU_UPGRADE_CMD = EXECUTABLE_FILE_PATH + "RFU %s /P /B /N /K /RLCE"
WARM_UPGRADE_CMD = EXECUTABLE_FILE_PATH + "warm_upgrade.py %s 0x%x 0x%x %s %s %s"
FIRMWARE_UPGRADE_TEST_CMD = EXECUTABLE_APP_PATH + FW_UPGRADE_APP_NAME + " test %s 0x%x 0x%x %s %s 0x%x"
CX7_UPGRADE_CMD = "cx7_upgrade.py %s"

GET_DISK_DEVICE_MODEL_CMD = "smartctl -a /dev/%s | grep 'Device Model'"
hard_disk_firmware_check = [
    "DSM28-480DT1NCAQFP-HLTX",
]

def is_upgrade_debug_mode():
    return os.path.exists(DEBUG_FILE)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def upgradewarninglog(s):
    logger.warning(s)

def upgradecriticallog(s):
    logger.critical(s)

def upgradeerror(s):
    logger.error(s)

def upgradedebuglog(s):
    logger.debug(s)

def upgrade_print_and_debug_log(s):
    # s = s.decode('utf-8').encode('gb2312')
    print(s)
    logger.info(s)

def signal_init():
    signal.signal(signal.SIGINT, signal.SIG_IGN)  # ignore ctrl+c signal
    signal.signal(signal.SIGTERM, signal.SIG_IGN)  # ignore kill signal
    signal.signal(signal.SIGTSTP, signal.SIG_IGN)  # ignore ctrl+z signal


def update_filetype_and_chain(config):
    new_file_type = config.get("new_file_type")
    new_chain = config.get("new_chain")
    if new_file_type is None or new_chain is None:
        return False, new_file_type, new_chain
    return True, new_file_type, new_chain


def parse_erase_size(size):
    """
    Parse size string to bytes.
    Supported examples: 1048576, 1k, 1m, 1g, 1kb, 1mb, 1gb.
    """
    if size is None:
        return 1024 * 1024

    size_str = str(size).strip().lower()
    if len(size_str) == 0:
        raise ValueError("size is empty")

    match = re.match(r"^(\d+)([kmg]?b?)$", size_str)
    if not match:
        raise ValueError("invalid size format: %s" % size)

    value = int(match.group(1), 10)
    unit = match.group(2)

    if unit in ("", "b"):
        factor = 1
    elif unit in ("k", "kb"):
        factor = 1024
    elif unit in ("m", "mb"):
        factor = 1024 * 1024
    elif unit in ("g", "gb"):
        factor = 1024 * 1024 * 1024
    else:
        raise ValueError("invalid size unit: %s" % unit)

    size_bytes = value * factor
    if size_bytes <= 0:
        raise ValueError("size must be greater than 0")
    if size_bytes > ERASE_MAX_SIZE_BYTES:
        raise ValueError("size exceeds max limit: %d bytes (64MB)" % ERASE_MAX_SIZE_BYTES)
    return size_bytes


def generate_ff_file(file_path, size_bytes, chunk_size=1024 * 1024):
    """Generate a file filled with 0xFF bytes."""
    if size_bytes <= 0:
        raise ValueError("size_bytes must be greater than 0")

    ff_chunk = b'\xff' * min(chunk_size, size_bytes)
    remaining = size_bytes
    with open(file_path, 'wb') as fp:
        while remaining > 0:
            write_size = min(len(ff_chunk), remaining)
            fp.write(ff_chunk[:write_size])
            remaining -= write_size


def update_firmware_refresh_config(name, slot_name, subtype, filetype, chain, chain_conf):
    '''
        FW_REFRESH_CONFIG format:
        FW_REFRESH_CONFIG = {
            "slot0": {
                "CPU_CPLD": {"subtype": 0xabcd, "filetype": 0, "chain": 1, "chain_conf": chain_conf}
                "BASE_CPLD": {"subtype": 0xabcd, "filetype": 0, "chain": 2, "chain_conf": chain_conf}
            },
            "slot1": {
                "LC_CPLD": {"subtype": 0xabcd, "filetype": 0xdcba, "chain": 1, "chain_conf": chain_conf}
            }
        }
    '''
    try:
        global FW_REFRESH_CONFIG
        global FW_REFRESH_CONFIG_ERRMSG
        name = name.upper()
        chain_num = int(chain.split('chain')[1])
        slot_config = FW_REFRESH_CONFIG.get(slot_name)
        if slot_config is None:
            tmp_dict = {name: {"subtype": subtype, "filetype": filetype, "chain": chain_num, "chain_conf": chain_conf}}
            FW_REFRESH_CONFIG[slot_name] = tmp_dict
            return True
        # slot config is not None
        # The name already exists
        if name in slot_config:
            # If filetype, chain, chain_conf are equal, it means that they are the same configuration, skip
            tmp_filetype = slot_config[name]["filetype"]
            tmp_chain_num = slot_config[name]["chain"]
            tmp_chain_conf = slot_config[name]["chain_conf"]
            if tmp_filetype == filetype and tmp_chain_num == chain_num and tmp_chain_conf == chain_conf:
                upgradedebuglog("%s filetype: %s, chain: %s, name: %s, already exists, skip" % (slot_name, filetype, chain_num, name))
                return False
            # if filetype or chain are not equal, it means that there are different configurations with the same name
            errmsg = ("ERR: %s refresh config repeated, please check WARM_UPGRADE_PARAM, %s, filetype: %s, chain: %s\n" %
                (name, slot_name, filetype, chain))
            upgradeerror(errmsg)
            FW_REFRESH_CONFIG_ERRMSG += errmsg
            return False
        tmp_dict = {name: {"subtype": subtype, "filetype": filetype, "chain": chain_num, "chain_conf": chain_conf}}
        FW_REFRESH_CONFIG[slot_name].update(tmp_dict)
        return True
    except Exception as e:
        errmsg = "ERR: %s chain config error,slot: %s, filetype: %s, chain: %s, errmsg: %s\n" % (name, slot_name, filetype, chain, str(e))
        upgradeerror(errmsg)
        FW_REFRESH_CONFIG_ERRMSG += errmsg
        return False


def help_get_firmware_refresh_name(slot, filetype, chain):
    slot_name = "slot%d" % slot
    slot_config = WARM_UPGRADE_PARAM.get(slot_name)
    if not isinstance(slot_config, dict):
        return None

    subtype = slot_config.get("subtype", 0)

    file_type_conf = slot_config.get(filetype)
    if not isinstance(file_type_conf, dict):
        return None

    chain_conf_list = file_type_conf.get(chain)
    if not isinstance(chain_conf_list, list):
        return None

    fw_name = ""
    for chain_conf in chain_conf_list:
        name = chain_conf.get('name')
        if isinstance(name, str):
            status = update_firmware_refresh_config(name, slot_name, subtype, filetype, chain, chain_conf)
            if status is True:
                fw_name += name + ", "
    return fw_name.rstrip(", ")

def help_get_cold_upg_fw_name(chain_conf):
    name = chain_conf.get('name')
    return name

def help_get_warm_upg_fw_name(slot, filetype, chain, chain_conf):
    name = ""
    is_support_warm_upg = chain_conf.get("is_support_warm_upg", 0)
    if is_support_warm_upg != 1:
        return name
    upgrade_way = chain_conf.get("upgrade_way", UPGRADE_BY_FIRMWARE_UPGRADE_COMMON)
    if upgrade_way == UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER:
        status, new_file_type, new_chain = update_filetype_and_chain(chain_conf)
        if status is False:
            upgradeerror("new_file_type or new_chain not define, new_file_type: %s, new_chain: %s" % (new_file_type, new_chain))
            return name
        upgradedebuglog("help_get_warm_upg_fw_name, new_file_type: %s, new_chain: %s" % (new_file_type, new_chain))
        filetype = new_file_type
        chain = "chain%s" % new_chain
    name = help_get_firmware_refresh_name(slot, filetype, chain)
    return name


def help_get_firmware_name_by_single_chain(slot, filetype, chain, chain_conf):
    fw_name = chain_conf.get("name")
    upgradedebuglog("Help get firmware name, slot: %s, filetype: %s, chain: %s, name: %s" %
        (slot, filetype, chain, fw_name))
    cold_upg_name = help_get_cold_upg_fw_name(chain_conf)
    warm_upg_name = help_get_warm_upg_fw_name(slot, filetype, chain, chain_conf)
    upgradedebuglog("cold_upg_name: %s, warm_upg_name: %s" % (cold_upg_name, warm_upg_name))
    return cold_upg_name, warm_upg_name


def help_get_firmware_name_by_chain(slot, filetype, chain, chain_conf):
    chain_cold_upg_name = ""
    chain_warm_upg_name = ""

    cold_upg_name = ""
    warm_upg_name = ""
    if isinstance(chain_conf, dict):
        cold_upg_name, warm_upg_name = help_get_firmware_name_by_single_chain(slot, filetype, chain, chain_conf)
    elif isinstance(chain_conf, list):
        for chain_conf_item in chain_conf:
            tmp_cold_upg_name, tmp_warm_upg_name = help_get_firmware_name_by_single_chain(slot, filetype, chain, chain_conf_item)
            if isinstance(tmp_cold_upg_name, str) and len(tmp_cold_upg_name) != 0:
                cold_upg_name += tmp_cold_upg_name + ", "
            if isinstance(tmp_warm_upg_name, str) and len(tmp_warm_upg_name) != 0:
                warm_upg_name += tmp_warm_upg_name + ", "
        cold_upg_name = cold_upg_name.rstrip(", ")
        warm_upg_name = warm_upg_name.rstrip(", ")

    if isinstance(cold_upg_name, str) and len(cold_upg_name) != 0:
        chain_cold_upg_name += cold_upg_name + ", "
    if isinstance(warm_upg_name, str) and len(warm_upg_name) != 0:
        chain_warm_upg_name += warm_upg_name + ", "
    return chain_cold_upg_name, chain_warm_upg_name


def help_get_filetype_firmware_name(slot, slot_config, filetype):
    filetype_cold_upg_name = ""
    filetype_warm_upg_name = ""

    file_type_conf = slot_config.get(filetype, {})
    for chain, chain_conf in file_type_conf.items():
        chain_cold_upg_name, chain_warm_upg_name = help_get_firmware_name_by_chain(slot, filetype, chain, chain_conf)
        if isinstance(chain_cold_upg_name, str) and len(chain_cold_upg_name) != 0:
            filetype_cold_upg_name += chain_cold_upg_name
        if isinstance(chain_warm_upg_name, str) and len(chain_warm_upg_name) != 0:
            filetype_warm_upg_name += chain_warm_upg_name
    return filetype_cold_upg_name, filetype_warm_upg_name

def help_get_slot_firmware_name(slot):
    slot_cold_upg_name = ""
    slot_warm_upg_name = ""
    slot_config =  UPGRADE_SUMMARY.get("slot%d" % slot, {})
    for filetype in SUPPORT_FILE_TYPE:
        filetype_cold_upg_name, filetype_warm_upg_name = help_get_filetype_firmware_name(slot, slot_config, filetype)
        if isinstance(filetype_cold_upg_name, str) and len(filetype_cold_upg_name) != 0:
            slot_cold_upg_name += filetype_cold_upg_name
        if isinstance(filetype_warm_upg_name, str) and len(filetype_warm_upg_name) != 0:
            slot_warm_upg_name += filetype_warm_upg_name

    if len(slot_cold_upg_name) != 0:
        slot_cold_upg_name = ("slot%d: %s" % (slot, slot_cold_upg_name)).rstrip(", ") + "\n"
    if len(slot_warm_upg_name) != 0:
        slot_warm_upg_name = ("slot%d: %s" % (slot, slot_warm_upg_name)).rstrip(", ") + "\n"
    return slot_cold_upg_name, slot_warm_upg_name


def help_get_firmware_name():
    cold_upg_name = ""
    warm_upg_name = ""
    try:
        debug_init()
        max_slot_num = UPGRADE_SUMMARY.get("max_slot_num", 0)
        for slot in range(0, max_slot_num + 1):
            slot_cold_upg_name, slot_warm_upg_name = help_get_slot_firmware_name(slot)
            if len(slot_cold_upg_name) != 0:
                cold_upg_name += slot_cold_upg_name
            if len(slot_warm_upg_name) != 0:
                warm_upg_name += slot_warm_upg_name
    except Exception as e:
        errmsg = "ERR: help_get_firmware_name raise exception, errmsg: %s\n" % str(e)
        upgradeerror(errmsg)
        cold_upg_name = ""
        warm_upg_name = ""
    return cold_upg_name, warm_upg_name

def check_hard_disk_protection(device_model):
    for protected_model in hard_disk_firmware_check:
        if protected_model in device_model:
            return True, protected_model
    return False, None


cold_upg_name, warm_upg_name = help_get_firmware_name()
FW_NAME_CLICK_HELP += cold_upg_name
FW_REFRESH_NAME_CLICK_HELP += warm_upg_name


class BasePlatform():

    def __init__(self):
        self.upgrade_param = UPGRADE_SUMMARY.copy()
        ret, val_tmp = get_board_id()
        if ret is False:
            self.devtype = None
        else:
            try:
                e2_id = int(val_tmp, base=16)
                self.devtype = e2_id
            except ValueError as e:
                self.devtype = None

        self.max_slot_num = self.upgrade_param.get("max_slot_num", 0)
        self.fw_sync_cfg = self.upgrade_param.get("fw_sync", None)
        self.head_info_config = {}
        self.slot_config = {}
        self.subtype = None
        self.chain = None
        self.chain_list = []
        self.filetype = None
        self.e2_devtype_config = self.upgrade_param.get('e2_devtype', {})
        self.DEVTYPE = None
        self.SUBTYPE = '0'
        self.TYPE = None
        self.CHAIN = None
        self.CHIPNAME = None
        self.VERSION = None
        self.FILETYPE = None
        self.CRC = None
        self.SUBTYPE_LIST = None
        self.need_header_flag = True

    def save_and_set_value(self, cfg_list):
        for config in cfg_list:
            ret, val = get_value(config)
            if ret:
                config["save_value"] = val
            else:
                upgradeerror(val)
                return False, "get save value fail"

            set_val = config.get("set_value", None)
            if set_val is None:
                log = "save_and_set_value lack of set_val config"
                upgradeerror(log)
                return log

            gettype = config.get("gettype", None)
            set_cmd = config.get("set_cmd", None)
            if gettype == "cmd":
                if set_cmd is None:
                    log = "save_and_set_value lack of set_cmd config"
                    upgradeerror(log)
                    return False, log
                config["cmd"] = set_cmd % set_val
                upgradedebuglog("save_and_set_value modify set cmd to %s" % config["cmd"])
            else:
                config["value"] = set_val
                upgradedebuglog("save_and_set_value modify set val to %s" % config["value"])

            ret, log = set_value(config)
            if ret is False:
                upgradeerror(log)
                return False, log
        return True, "save and set value success"

    def recover_save_value(self, cfg_list):
        total_err = 0
        for config in cfg_list:
            upgradedebuglog("config: %s, recover save value" % config)
            val = config.get("save_value", None)
            if val is None:
                upgradeerror("recover_save_value lack of save_value config")
                total_err -= 1
                continue
            gettype = config.get("gettype", None)
            set_cmd = config.get("set_cmd", None)
            if gettype == "cmd":
                config["cmd"] = set_cmd % val
                upgradedebuglog("recover_save_value modify set cmd to %s" % config["cmd"])
            else:
                config["value"] = val
                upgradedebuglog("recover_save_value modify set val to %s" % config["value"])

            ret, log = set_value(config)
            if ret is False:
                upgradeerror("recover save value write failed, log: %s" % log)
                total_err -= 1
            else:
                upgradedebuglog("recover save value success")
        if total_err < 0:
            return False, "recover save value failed"
        return True, "recover save value success"

    def check_slot_present(self, slot_present_config):
        presentbit = slot_present_config.get('presentbit')
        ret, value = get_value(slot_present_config)
        if ret is False:
            return "NOT OK"
        if isinstance(value, str):
            val_t = int(value, 16)
        else:
            val_t = value
        if presentbit:
            val_t = (val_t & (1 << presentbit)) >> presentbit
        if val_t != slot_present_config.get('okval'):
            status = "ABSENT"
        else:
            status = "PRESENT"
        return status

    def linecard_present_check(self, slot_present_config):
        present_status = self.check_slot_present(slot_present_config)
        if present_status == "NOT OK":
            return ERR_FW_DEVICE_ACCESS, "get slot present status failed."
        if present_status == "ABSENT":
            return ERR_FW_CARD_ABSENT, "slot absent"
        return CHECK_OK, "slot present"

    def subprocess_warm_upgrade(self, config, file, main_type, sub_type, slot, filetype, chain):
        dev_name = config.get("name", None)
        status, output = self.subprocess_firmware_upgrade(config, file, main_type, sub_type, slot)
        if status is False:
            upgradeerror("%s warm upgrade failed" % dev_name)
            return False, output
        command = WARM_UPGRADE_CMD % (file, main_type, sub_type, slot, filetype, chain)
        upgradedebuglog("warm upgrade cmd: %s" % command)
        if is_upgrade_debug_mode():
            status, output = exec_os_cmd_log(command)
        else:
            status, output = exec_os_cmd(command)
        if status:
            upgradeerror("%s warm upgrade failed" % dev_name)
            return False, output
        upgradedebuglog("%s warm upgrade success" % dev_name)
        return True, "upgrade success"

    def upgrade_set_value(self, config):
        record_ignore_result_flag = config.get("ex_ignore_rec", 0)
        ret, log = set_value(config)
        if ret is False and record_ignore_result_flag == 1:
            upgradeerror(log)
            return True, log

        return ret, log

    def do_fw_upg_cmd_once(self, dev_name, cmd_config):
        support_operation_list = ["set_value", "check_value", "backup_value"]
        operation = cmd_config.get("operation", "set_value")
        if operation not in support_operation_list:
            log = "%s do cmd: %s failed, invalid operation type: %s" % (dev_name, cmd_config, operation)
            upgradeerror(log)
            return False, log

        # set value
        if operation == "set_value":
            ret, log = self.upgrade_set_value(cmd_config)
            if ret is False:
                log = "%s do cmd: %s set_value failed, msg: %s" % (dev_name, cmd_config, log)
                upgradeerror(log)
                return False, log
            log = "%s do cmd: %s set_value success, msg: %s" % (dev_name, cmd_config, log)
            upgradedebuglog(log)
            return True, log
        elif operation == "backup_value":
            if cmd_config.get("value") is None:
                ret, val = get_value(cmd_config)
                if ret is False:
                    log = "%s do cmd: %s get_backup_value failed, msg: %s" % (dev_name, cmd_config, val)
                    upgradeerror(log)
                    return False, log
                cmd_config["value"] = val
                log = "%s do cmd: %s get_backup_value success, val: %s" % (dev_name, cmd_config, val)
                upgradedebuglog(log)
                return True, log
            else:
                ret, log = set_value(cmd_config)
                if ret is False:
                    log = "%s do cmd: %s set_backup_value failed, msg: %s" % (dev_name, cmd_config, log)
                    upgradeerror(log)
                    return False, log
                log = "%s do cmd: %s set_backup_value success, msg: %s" % (dev_name, cmd_config, log)
                upgradedebuglog(log)
                return True, log
        else:
            # check value
            ret, log = check_value(cmd_config)
            if ret is False:
                log = "%s do cmd: %s check_value failed, msg: %s" % (dev_name, cmd_config, log)
                upgradeerror(log)
                return False, log
            log = "%s do cmd: %s check_value success, msg: %s" % (dev_name, cmd_config, log)
            upgradedebuglog(log)
            return True, log

    def do_fw_upg_init_cmd(self, dev_name, init_cmd_list):
        # pre operation
        try:
            for init_cmd_config in init_cmd_list:
                status, log = self.do_fw_upg_cmd_once(dev_name, init_cmd_config)
                if status is False:
                    upgradeerror("%s do_fw_upg_init_cmd %s failed" % (dev_name, init_cmd_config))
                    return False, log
            msg = "%s firmware init cmd all set success" % dev_name
            upgradedebuglog(msg)
            return True, msg
        except Exception as e:
            return False, str(e)

    def do_fw_upg_finish_cmd(self, dev_name, finish_cmd_list):
        # end operation
        ret = 0
        for finish_cmd_config in finish_cmd_list:
            status, log = self.do_fw_upg_cmd_once(dev_name, finish_cmd_config)
            if status is False:
                upgradeerror("%s do_fw_upg_finish_cmd %s failed" % (dev_name, finish_cmd_config))
                ret = -1
        if ret != 0:
            msg = "%s firmware finish cmd exec failed" % dev_name
            upgradeerror(msg)
            return False, msg
        msg = "%s firmware finish cmd all set success" % dev_name
        upgradedebuglog(msg)
        return True, msg

    def do_fw_upg_finish_cmd_update(self, dev_name, init_cmd_list, finish_cmd_list):
        try:
            msg = "%s raw finish_cmd_list: %s" % (dev_name, finish_cmd_list)
            upgradedebuglog(msg)
            msg = "%s init_cmd_list: %s" % (dev_name, init_cmd_list)
            upgradedebuglog(msg)

            init_cmd_name_map = {}
            for init_cmd_config in init_cmd_list:
                if init_cmd_config.get("operation", "set_value") == "backup_value":
                    init_cmd_name = init_cmd_config.get("name")
                    if init_cmd_name:
                        init_cmd_name_map[init_cmd_name] = init_cmd_config

            for i in range(len(finish_cmd_list)):
                if finish_cmd_list[i].get("operation", "set_value") == "backup_value":
                    finish_cmd_name = finish_cmd_list[i].get("name")
                    if finish_cmd_name and init_cmd_name_map.get(finish_cmd_name):
                        finish_cmd_list[i] = init_cmd_name_map.get(finish_cmd_name)

            msg = "%s upgraded finish_cmd_list: %s" % (dev_name, finish_cmd_list)
            upgradedebuglog(msg)
            msg = "%s firmware upgrade finish cmd success" % dev_name
            upgradedebuglog(msg)
            return True, msg
        except Exception as e:
            upgradeerror(str(e))
            return False, str(e)

    def upgrade_raw_file_generate(self, file, head_info_config, raw_file_name = None):
        raw_file_tmp = None
        if self.need_header_flag:
            ret, raw_file_tmp = do_fw_upg_raw_file_generate(file, head_info_config, raw_file_name)
            if ret is False:
                return False, "generate raw file failed, reason: %s" % (raw_file_tmp)
        else:
            dir_name = os.path.dirname(file)
            file_name = os.path.basename(file)
            raw_file_tmp = os.path.join(dir_name, f"raw_{file_name}")
            try:
                shutil.copy(file, raw_file_tmp)
            except Exception as e:
                return False, f"Failed to copy file: {str(e)}"

        return True, raw_file_tmp

    def upgrade_cmd_generate(self, config, file, main_type, sub_type, slot, filetype, chain):
        upgrade_way = config.get("upgrade_way", UPGRADE_BY_FIRMWARE_UPGRADE_COMMON)
        raw_file = None

        if upgrade_way not in SUPPORT_UPGRADE_LIST:
            return False, "unsupport upgrade way: %s" % upgrade_way, None

        if upgrade_way == UPGRADE_BY_FIRMWARE_UPGRADE_COMMON:
            command = FIRMWARE_UPGRADE_CMD % (file, main_type, sub_type, slot, filetype, chain)
            return True, command, None
        elif upgrade_way == UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER:
            status, new_file_type, new_chain = update_filetype_and_chain(config)
            if status is False:
                return False, "upgrade way: %s new_file_type or new_chain not define, new_file_type: %s, new_chain: %s" % (upgrade_way, new_file_type, new_chain), None
            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = FIRMWARE_UPGRADE_CMD % (raw_file, main_type, sub_type, slot, new_file_type, new_chain)
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_AFU:
            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = AFU_UPGRADE_CMD % raw_file
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_CUSTOM:
            upgrade_cmd = config.get("upgrade_cmd")
            if upgrade_cmd is None:
                return False, "upgrade way: %s upgrade_cmd not define, upgrade_cmd: %s" % (upgrade_way, upgrade_cmd), None
            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = upgrade_cmd % raw_file
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_HDPARM:
            dev_name_str = config.get("dev_name", "sda")
            get_disk_model_cmd = GET_DISK_DEVICE_MODEL_CMD % dev_name_str
            status, val = exec_os_cmd(get_disk_model_cmd)
            if status:
                return False, "get disk device model error: %s" % (val), None

            firmware_check, firmware_check_name = check_hard_disk_protection(val)
            if firmware_check:
                if firmware_check_name != self.head_info_config.get('CHIPNAME', "NA"):
                    return False, "Firmware and device do not match. (firmware_check_name:%s, chip name %s)" % (firmware_check_name, self.head_info_config.get('CHIPNAME', "NA")), None
                else:
                    upgradedebuglog("match device model %s" % firmware_check_name)

            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = HDPARM_UPGRADE_CMD % (raw_file, dev_name_str)
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_VR_SCRIPT:
            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            bus = config.get("bus")
            addr = config.get("addr")
            command = VR_UPGRADE_CMD % (raw_file, bus, addr)
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_NVME:
            dev_name_str = config.get("dev_name", "nvme0n1")
            nvme_update_way = config.get("nvme_update_way", NVME_SSD_UPDATE_WAY_INVAILD)
            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = NVME_SSD_UPGRADE_CMD % (nvme_update_way, dev_name_str, raw_file)
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_NVMUPDATE:
            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            dev_name_str = config.get("name", "NA")
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = NVMUPDATE_UPGRADE_CMD % (dev_name_str, self.head_info_config.get('VERSION', "NA"), raw_file)
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_CX7_SCRIPT:
            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = CX7_UPGRADE_CMD % raw_file
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_FW_UP_BIOS:
            upgrade_cmd = FW_UP_BIOS_CMD % (file)
            #if COLD_UPGRADE_KEEP_USER_CONFIG is True, add -u to the command line, otherwise use the original command line
            if COLD_UPGRADE_KEEP_USER_CONFIG:
                command = upgrade_cmd + " -u"
            else:
                command = upgrade_cmd
            return True, command, None
        else:
            return False, "unsupport upgrade way: %s" % upgrade_way, None

    def upgrade_test_cmd_generate(self, config, file, main_type, sub_type, slot, filetype, chain):
        upgrade_way = config.get("upgrade_way", UPGRADE_BY_FIRMWARE_UPGRADE_COMMON)
        raw_file = None

        if upgrade_way not in SUPPORT_UPGRADE_LIST:
            return False, "test unsupport upgrade way: %s" % upgrade_way, None

        if upgrade_way == UPGRADE_BY_FIRMWARE_UPGRADE_COMMON:
            command = FIRMWARE_UPGRADE_TEST_CMD % (file, main_type, sub_type, slot, filetype, chain)
            return True, command, None
        elif upgrade_way == UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER:
            status, new_file_type, new_chain = update_filetype_and_chain(config)
            if status is False:
                return False, "test upgrade way: %s new_file_type or new_chain not define, new_file_type: %s, new_chain: %s" % (upgrade_way, new_file_type, new_chain), None
            ret, raw_file = self.upgrade_raw_file_generate(file, self.head_info_config)
            if ret is False:
                return False, "test upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = FIRMWARE_UPGRADE_TEST_CMD % (raw_file, main_type, sub_type, slot, new_file_type, new_chain)
            return True, command, raw_file

        else:
            return False, "unsupport upgrade way: %s" % upgrade_way, None

    def remove_raw_file(self, raw_file):
        if raw_file:
            if os.path.exists(raw_file):
                os.remove(raw_file)
                upgradedebuglog(f"Removed raw file: {raw_file}")

    def subprocess_firmware_upgrade(self, config, file, main_type, sub_type, slot, filetype, chain):
        dev_name = config.get("name", None)
        init_cmd_list = config.get("init_cmd", [])
        finish_cmd_list = config.get("finish_cmd", [])
        try:
            ret, command, raw_file = self.upgrade_cmd_generate(config, file, main_type, sub_type, slot, filetype, chain)
            if ret is False:
                upgradeerror("%s firmware upgrade failed, msg: %s" % (dev_name, command))
                return False, command

            ret, log = self.do_fw_upg_init_cmd(dev_name, init_cmd_list)
            self.do_fw_upg_finish_cmd_update(dev_name, init_cmd_list, finish_cmd_list)
            if ret is False:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, log
            time.sleep(0.5)  # delay 0.5s after execute init_cmd

            upgradedebuglog("firmware upgrade cmd: %s" % command)
            if is_upgrade_debug_mode():
                status, output = exec_os_cmd_log(command)
            else:
                status, output = exec_os_cmd(command)
            if status:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                upgradeerror("%s firmware upgrade failed, msg: %s" % (dev_name, output))
                return False, output
            upgradedebuglog("%s firmware upgrade success" % dev_name)

            ret, log = self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            if ret is False:
                return False, log
            return True, "upgrade success"
        except Exception as e:
            self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            return False, str(e)
        finally:
            self.remove_raw_file(raw_file)

    def subprocess_test_upgrade(self, config, file, main_type, sub_type, slot, filetype, chain):
        dev_name = config.get("name", None)
        init_cmd_list = config.get("init_cmd", [])
        finish_cmd_list = config.get("finish_cmd", [])
        try:
            ret, command, raw_file = self.upgrade_test_cmd_generate(config, file, main_type, sub_type, slot, filetype, chain)
            if ret is False:
                upgradeerror("%s firmware upgrade failed, msg: %s" % (dev_name, command))
                return False, command

            ret, log = self.do_fw_upg_init_cmd(dev_name, init_cmd_list)
            self.do_fw_upg_finish_cmd_update(dev_name, init_cmd_list, finish_cmd_list)
            if ret is False:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, log
            time.sleep(0.5)  # delay 0.5s after execute init_cmd
            upgradedebuglog("firmware upgrade cmd: %s" % command)
            if is_upgrade_debug_mode():
                status, output = exec_os_cmd_log(command)
            else:
                status, output = exec_os_cmd(command)
            if status:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                upgradeerror("%s test upgrade failed, msg: %s" % (dev_name, output))
                return False, output
            upgradedebuglog("%s test upgrade success" % dev_name)
            ret, log = self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            if ret is False:
                return False, log
            return True, "upgrade success"
        except Exception as e:
            self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            return False, str(e)
        finally:
            self.remove_raw_file(raw_file)

    def upgrade_other_bmc_access_check(self, bmc_access_config):
        access_success_code = bmc_access_config.get("success", None)
        access_fail_code = bmc_access_config.get("fail", None)
        access_runing_code = bmc_access_config.get("runing", None)
        access_timeout = bmc_access_config.get("timeout", 1200)
        access_interval = bmc_access_config.get("interval", 20)
        access_check_delay = bmc_access_config.get("check_delay", 5)

        if access_success_code is None or access_fail_code is None or access_runing_code is None:
            return False, "upgrade_other_bmc_access_check: config err"

        time.sleep(access_check_delay)
        start_time = _time()
        while True:
            ret, val = get_value(bmc_access_config)
            if ret == False:
                upgradeerror("upgrade_other_bmc_access_check: get bmc access status fail")
                return ret, val
            upgradedebuglog("upgrade_other_bmc_access_check: access_code %s" % val)

            if val == access_success_code:
                log = "upgrade_other_bmc_access_check: upgrade other bmc success"
                upgrade_print_and_debug_log(log)
                return True, log
            elif val == access_runing_code:
                log = "upgrade_other_bmc_access_check: upgrading other bmc, please wait"
                upgradedebuglog(log)
            elif val == access_fail_code:
                log = "upgrade_other_bmc_access_check: upgrade other bmc fail, access_code: %s" % val
                return False, log
            else:
                log = "upgrade_other_bmc_access_check: unknow access_code: %s" % val
                return False, log

            delta_time = _time() - start_time
            if delta_time >= access_timeout or delta_time < 0:
                log = "upgrade_other_bmc_access_check: upgrade other bmc timeout"
                return False, log
            time.sleep(access_interval)

    def upgrade_current_bmc_access_check(self, bmc_access_config):
        access_timeout = bmc_access_config.get("timeout", 1200)
        access_interval = bmc_access_config.get("interval", 20)
        access_check_delay = bmc_access_config.get("check_delay", 5)

        time.sleep(access_check_delay)
        start_time = _time()
        while True:
            ret, val = exec_os_cmd(bmc_access_config.get("cmd", None))
            if not ret and len(val) > 0:
                return True, val
            upgradedebuglog("upgrade_current_bmc_access_check: upgrading current bmc, please wait")
            delta_time = _time() - start_time
            if delta_time >= access_timeout or delta_time < 0:
                return False, ("upgrade_current_bmc_access_check: timeout")
            time.sleep(access_interval)

    def upgrade_current_bmc(self, upgrade_bmc_cmd, upgrade_bmc, get_bmc_version_config):
        if upgrade_bmc_cmd is None:
            log = "upgrade_current_bmc: upgrade_bmc_cmd err, please check"
            return False, log
        #get bmc version first
        upgrade_print_and_debug_log("upgrade_current_bmc: start upgrade current bmc")
        if get_bmc_version_config is not None:
            ret, current_bmc_version = exec_os_cmd(get_bmc_version_config.get("cmd", None))
            if ret or len(current_bmc_version) == 0:
                return False, "upgrade_current_bmc: get bmc version fail"
            upgrade_print_and_debug_log("upgrade_current_bmc: current bmc version: %s" % current_bmc_version)
        #start upgrade
        ret, val = exec_os_cmd(upgrade_bmc_cmd)
        if ret:
            return False, "upgrade_current_bmc: exec upgrade cmd fail"
        upgrade_print_and_debug_log("upgrade_current_bmc: BMC upgrading, will reboot now, please wait")
        ret, upgraded_bmc_version = self.upgrade_current_bmc_access_check(get_bmc_version_config)
        if ret is True:
            upgrade_print_and_debug_log("upgrade_current_bmc: upgrade %s bmc success , before upgrade version: %s ,after upgrade version: %s" % (upgrade_bmc, current_bmc_version, upgraded_bmc_version))
        return ret, upgraded_bmc_version

    def upgrade_other_bmc(self, upgrade_bmc_cmd, upgrade_bmc, access_check_config):
        if upgrade_bmc_cmd is None:
            log = "upgrade_other_bmc: upgrade_bmc_cmd err, please check"
            return False, log
        upgrade_print_and_debug_log("upgrade_other_bmc: start upgrade other bmc")
        ret, val = exec_os_cmd(upgrade_bmc_cmd)
        if ret:
            return False, "upgrade_other_bmc: exec upgrade cmd fail"
        upgrade_print_and_debug_log("upgrade_other_bmc: other bmc is upgrading, please wait")
        ret, log = self.upgrade_other_bmc_access_check(access_check_config)
        if ret is True:
            upgrade_print_and_debug_log("upgrade_other_bmc: upgrade %s bmc success, you can switch to other bmc check" % upgrade_bmc)
        return ret, log

    def upgrade_bmc_by_usb(self, config, current_bmc, chip_select, erase_type):
        upgrade_bmc_config = config.get("upgrade_bmc_config", None)
        access_check_config = config.get("access_check_config", None)
        get_current_bmc_version = config.get("get_current_bmc_version", None)

        if upgrade_bmc_config is None or access_check_config is None or get_current_bmc_version is None:
            return False, "upgrade_bmc_by_usb config err, please check"

        if upgrade_bmc_config.get("cmd", None) is None:
            return False, ("upgrade_bmc_by_usb: upgrade bmc cmd err, please check")
        erase_param = upgrade_bmc_config.get(erase_type, ERASE_FULL_CODE_STR)

        # Since the upgrade of the current blade will cause a reboot,
        # it is not possible to determine whether the other blade has completed the upgrade through ipmitool.
        # Therefore, upgrading both blades is changed to executing the command twice.
        if chip_select == CHIP_MASTER:
            upgrade_bmc = BMC_MASTER
            chip_param = upgrade_bmc_config.get(upgrade_bmc, CHIP_MASTER)
            upgrade_bmc_cmd = upgrade_bmc_config["cmd"] % (chip_param, erase_param)
            upgradedebuglog("upgrade_bmc_by_usb: upgrade cmd is: %s" % upgrade_bmc_cmd)
        elif chip_select == CHIP_SLAVE:
            upgrade_bmc = BMC_SLAVE
            chip_param = upgrade_bmc_config.get(upgrade_bmc, CHIP_SLAVE)
            upgrade_bmc_cmd = upgrade_bmc_config["cmd"] % (chip_param, erase_param)
            upgradedebuglog("upgrade_bmc_by_usb: upgrade cmd is: %s" % upgrade_bmc_cmd)
        else:
            upgrade_bmc = BMC_BOTH
            if current_bmc == CHIP_MASTER:
                upgrade_other_bmc = BMC_SLAVE
                other_chip_param = upgrade_bmc_config.get(upgrade_other_bmc, CHIP_SLAVE)
                upgrade_other_bmc_cmd = upgrade_bmc_config["cmd"] % (other_chip_param, erase_param)
                upgrade_current_bmc = BMC_MASTER
                current_chip_param = upgrade_bmc_config.get(upgrade_current_bmc, CHIP_MASTER)
                upgrade_current_bmc_cmd = upgrade_bmc_config["cmd"] % (current_chip_param, erase_param)
                upgradedebuglog("upgrade_bmc_by_usb: upgrade_current_bmc_cmd is: %s" % upgrade_current_bmc_cmd)
                upgradedebuglog("upgrade_bmc_by_usb: upgrade_other_bmc_cmd is: %s" % upgrade_other_bmc_cmd)
            else:
                upgrade_other_bmc = BMC_MASTER
                other_chip_param = upgrade_bmc_config.get(upgrade_other_bmc, CHIP_MASTER)
                upgrade_other_bmc_cmd = upgrade_bmc_config["cmd"] % (other_chip_param, erase_param)
                upgrade_current_bmc = BMC_SLAVE
                current_chip_param = upgrade_bmc_config.get(upgrade_current_bmc, CHIP_SLAVE)
                upgrade_current_bmc_cmd = upgrade_bmc_config["cmd"] % (current_chip_param, erase_param)
                upgradedebuglog("upgrade_bmc_by_usb: upgrade_current_bmc_cmd is: %s" % upgrade_current_bmc_cmd)
                upgradedebuglog("upgrade_bmc_by_usb: upgrade_other_bmc_cmd is: %s" % upgrade_other_bmc_cmd)

        if chip_select == CHIP_BOTH:
            upgrade_print_and_debug_log("upgrade_bmc_by_usb: start upgrade both bmc")
            upgrade_print_and_debug_log("upgrade_bmc_by_usb: first upgrade other bmc")
            ret, log = self.upgrade_other_bmc(upgrade_other_bmc_cmd, upgrade_other_bmc, access_check_config)
            if ret is False:
                upgradeerror("upgrade_bmc_by_usb: upgrade other bmc fail")
                return ret, log
            ret, log = self.upgrade_current_bmc(upgrade_current_bmc_cmd, upgrade_current_bmc, get_current_bmc_version)
            if ret is False:
                upgradeerror("upgrade_bmc_by_usb: upgrade current bmc fail")
            else:
                upgrade_print_and_debug_log("upgrade_bmc_by_usb: upgrade both bmc success")
            return ret, log
        elif chip_select == current_bmc:
            return self.upgrade_current_bmc(upgrade_bmc_cmd, upgrade_bmc, get_current_bmc_version)
        else:
            return self.upgrade_other_bmc(upgrade_bmc_cmd, upgrade_bmc, access_check_config)

    def bmc_upgrade_by_usb(self, config, file, chip_select, erase_type):
        try:
            init_cmd_list = config.get("init_cmd", [])
            finish_cmd_list = config.get("finish_cmd", [])
            dev_name = config.get("name", "BMC")

            if erase_type not in BMC_UPGRADE_SUPPORT_ERASE_TYPE_LIST :
                return False, ("bmc_upgrade_by_usb: erase_type input err, please check")

            get_usb_dev_name_cmd = config.get("get_usb_dev_name", None)
            get_current_bmc_cmd = config.get("get_current_bmc", None)
            check_image_md5_config = config.get("check_image_md5", None)

            if get_usb_dev_name_cmd is None or get_current_bmc_cmd is None:
                log = "bmc_upgrade_by_usb: %s upgrade config error, please check" % dev_name
                return False, log

            # get current bmc first
            ret, current_bmc_str = exec_os_cmd(get_current_bmc_cmd)
            if ret or len(current_bmc_str) == 0:
                return False, ("bmc_upgrade_by_usb: get current bmc fail, please check bmc status or wait bmc ready")
            upgrade_print_and_debug_log("bmc_upgrade_by_usb: get current bmc: %s" % current_bmc_str)
            if BMC_MASTER in current_bmc_str:
                current_bmc = CHIP_MASTER
            elif BMC_SLAVE in current_bmc_str:
                current_bmc = CHIP_SLAVE
            else:
                return False, ("bmc_upgrade_by_usb: get unkonw current bmc res: %s" % current_bmc_str)

            # pre operation
            for init_cmd_config in init_cmd_list:
                ret, log = set_value(init_cmd_config)
                if ret == False:
                    return False, log
            upgradedebuglog("%s bmc init cmd all set success" % dev_name)
            time.sleep(0.5)  # delay 0.5s after execute init_cmd

            # get usb dev name
            ret, usb_dev_name = exec_os_cmd(get_usb_dev_name_cmd)
            if ret:
                log = "bmc_upgrade_by_usb: %s get usb dev name failed" % dev_name
                upgradeerror(log)
                return False, usb_dev_name
            upgradedebuglog("bmc_upgrade_by_usb: get usb_dev_name success, name: %s" % usb_dev_name)

            # copy upgrade bin to USB dev
            usb_dev_path = "/dev/%s" % usb_dev_name.strip()
            upgradedebuglog("bmc_upgrade_by_usb: usb_dev_name path: %s" % usb_dev_path)
            shutil.copyfile(file, usb_dev_path)

            # check md5
            if check_image_md5_config is not None:
                md5 = calculate_md5(file)
                split_md5 = ' '.join([("0x%s" % md5[i:i+2]) for i in range(0, len(md5), 2)])
                upgradedebuglog("bmc_upgrade_by_usb: split_md5 : %s" % split_md5)
                check_image_md5_config['cmd'] = check_image_md5_config['cmd'] % split_md5
                upgradedebuglog("bmc_upgrade_by_usb: md5 sum check cmd %s" % check_image_md5_config['cmd'])
                ret, log = set_value(check_image_md5_config)
                if ret == False:
                    upgradeerror("bmc_upgrade_by_usb: %s check md5sum fail, file name %s" % (dev_name, file))
                    return False, log
                upgradedebuglog("bmc_upgrade_by_usb: md5 sum check pass")
            return self.upgrade_bmc_by_usb(config, current_bmc, chip_select, erase_type)
        except Exception as e:
            return False, str(e)
        finally:
            # end operation
            for finish_cmd_config in finish_cmd_list:
                ret, log = set_value(finish_cmd_config)
                if ret == False:
                    upgradeerror("%s bmc finish cmd exec failed" % dev_name)
                    upgradeerror(log)
            upgradedebuglog("%s bmc finish cmd all set success" % dev_name)

    def write_to_char_device(self, char_device_path, file_path, chunk_size=1024):
        """
        Write data from a file to a character device and display progress.

        Parameters:
        - char_device_path: Path to the character device (e.g., /dev/bmc_flash_master).
        - file_path: Path to the file to be written (e.g., /tmp/image-bmc0).
        - chunk_size: Size of each chunk to write (default is 1024 bytes).
        """
        try:
            upgrade_print_and_debug_log(f'start Write file : {file_path} to char dev : {char_device_path}.')
            with open(char_device_path, 'wb') as char_device:
                with open(file_path, 'rb') as input_file:
                    file_size = os.path.getsize(file_path)
                    bytes_written = 0

                    print(f'Progress: 0% ({bytes_written}/{file_size} bytes)', end='\r')
                    while True:
                        data = input_file.read(chunk_size)
                        if not data:
                            break

                        char_device.write(data)
                        bytes_written += len(data)

                        progress = (bytes_written / file_size) * 100
                        print(f'Progress: {progress:.2f}% ({bytes_written}/{file_size} bytes)', end='\r')

            upgrade_print_and_debug_log('\nWrite operation completed successfully.')
            log = f"Write file : {file_path} to char dev : {char_device_path} success"
            return True, log

        except FileNotFoundError:
            log = f"Error: The file {file_path} or device {char_device_path} was not found."
        except PermissionError:
            log = f"Error: Permission denied when accessing {char_device_path}."
        except Exception as e:
            log = f"An error occurred: {e}"
        return False, log

    def bmc_upgrade_by_lpc(self, config, file, chip_select, erase_type):
        try:
            init_cmd_list = config.get("init_cmd", [])
            finish_cmd_list = config.get("finish_cmd", [])
            dev_name = config.get("name", "BMC")

            if erase_type not in BMC_UPGRADE_SUPPORT_ERASE_TYPE_LIST :
                return False, ("bmc_upgrade_by_lpc: erase_type input err, please check")

            # pre operation
            for init_cmd_config in init_cmd_list:
                ret, log = set_value(init_cmd_config)
                if ret == False:
                    return False, log
            upgradedebuglog("%s bmc init cmd all set success" % dev_name)
            time.sleep(0.5)  # delay 0.5s after execute init_cmd

            if erase_type == ERASE_FULL:
                ret, log = write_sysfs(LPC_BMC_FLASH_ERASE_FULL, "1")
            else:
                ret, log = write_sysfs(LPC_BMC_FLASH_ERASE_FULL, "0")
            if ret == False:
                return False, log

            # enable bmc flash write
            ret, log = write_sysfs(LPC_BMC_FLASH_RW_ENABLE, "1")
            if ret == False:
                return False, log

            if chip_select == CHIP_MASTER:
                ret, log = self.write_to_char_device(LPC_BMC_MASTER_FLASH_DEV, file, LPC_BMC_FLASH_PRE_WR_SIZE)
                if ret == False:
                    return False, "lpc upgrade bmc master flash fail, reason: %s" % log
            elif chip_select == CHIP_SLAVE:
                ret, log = self.write_to_char_device(LPC_BMC_SLAVE_FLASH_DEV, file, LPC_BMC_FLASH_PRE_WR_SIZE)
                if ret == False:
                    return False, "lpc upgrade bmc slave flash fail, reason: %s" % log
            else:
                ret, log = self.write_to_char_device(LPC_BMC_MASTER_FLASH_DEV, file, LPC_BMC_FLASH_PRE_WR_SIZE)
                if ret == False:
                    return False, "lpc upgrade bmc master flash fail, reason: %s" % log
                ret, log = self.write_to_char_device(LPC_BMC_SLAVE_FLASH_DEV, file, LPC_BMC_FLASH_PRE_WR_SIZE)
                if ret == False:
                    return False, "lpc upgrade bmc slave flash fail, reason: %s" % log

            if chip_select == CHIP_SLAVE:
                ret, log = write_sysfs(LPC_BMC_RESET, "0")
            else:
                ret, log = write_sysfs(LPC_BMC_RESET, "1")

            if ret == False:
                return False, log

            upgrade_print_and_debug_log('bmc upgrade finish, will be reboot.')

            # disable bmc flash write
            ret, log = write_sysfs(LPC_BMC_FLASH_RW_ENABLE, "0")
            if ret == False:
                return False, log

            ret, log = write_sysfs(LPC_BMC_FLASH_ERASE_FULL, "0")
            if ret == False:
                return False, log

            upgradedebuglog("%s bmc upgrade success" % dev_name)
            return True, "upgrade success"
        except Exception as e:
            return False, str(e)
        finally:
            # end operation
            for finish_cmd_config in finish_cmd_list:
                ret, log = set_value(finish_cmd_config)
                if ret == False:
                    upgradeerror("%s bmc finish cmd exec failed" % dev_name)
                    upgradeerror(log)
            upgradedebuglog("%s bmc finish cmd all set success" % dev_name)

    def bmc_upgrade_by_redfish(self, config, file, chip_select, erase_type):
        try:
            init_cmd_list = config.get("init_cmd", [])
            finish_cmd_list = config.get("finish_cmd", [])
            dev_name = config.get("name", "BMC")
            duallmage = "false"
            config_erase = "false"

            # pre operation
            for init_cmd_config in init_cmd_list:
                ret, log = set_value(init_cmd_config)
                if ret == False:
                    return False, log
            upgradedebuglog("%s bmc init cmd all set success" % dev_name)
            time.sleep(0.5)  # delay 0.5s after execute init_cmd

            if chip_select == CHIP_BOTH:
                duallmage = "true"

            tar_command = "cp -f %s %s" % (file, REDFISH_UPGRADE_BMC_FILE)
            status, output = exec_os_cmd(tar_command)
            if status:
                upgradeerror("%s firmware file tar failed, msg: %s" % (dev_name, output))
                return False, output

            command = REDFISH_UPGRADE_BMC_CMD % (config_erase, duallmage, REDFISH_UPGRADE_BMC_FILE)
            upgradedebuglog("bmc_upgrade_by_redfish upgrade cmd: %s" % command)
            if is_upgrade_debug_mode():
                status, output = exec_os_cmd_log(command)
            else:
                status, output = exec_os_cmd(command)
            if status:
                upgradeerror("%s firmware upgrade failed, msg: %s" % (dev_name, output))
                return False, output

            upgradedebuglog("%s bmc upgrade success" % dev_name)
            return True, "upgrade success"
        except Exception as e:
            return False, str(e)
        finally:
            # end operation
            self.remove_raw_file(REDFISH_UPGRADE_BMC_FILE)
            for finish_cmd_config in finish_cmd_list:
                ret, log = set_value(finish_cmd_config)
                if ret == False:
                    upgradeerror("%s bmc finish cmd exec failed" % dev_name)
                    upgradeerror(log)
            upgradedebuglog("%s bmc finish cmd all set success" % dev_name)

    def subprocess_bmc_upgrade(self, config, file, chip_select, chanel, erase_type):
        try:
            save_set_reg_list = []
            dev_name = "BMC"

            default_chanel =  config.get("default_chanel", None)
            if chanel is None:
                if default_chanel is None:
                    chanel = CHANEL_LPC
                    up_bmc_config = config
                else:
                    chanel = default_chanel
                    up_bmc_config = config.get(chanel, None)
            else:
                if default_chanel is None:
                    up_bmc_config = config
                else:
                    up_bmc_config = config.get(chanel, None)

            if chip_select not in BMC_UPGRADE_SUPPORT_CHIP_LIST or chanel not in BMC_UPGRADE_SUPPORT_CHANEL_LIST:
                return False, ("chip select or chanel input err, please check")

            if up_bmc_config is None:
                return False, "can not get %s chanel config" % chanel

            save_set_reg_list = up_bmc_config.get("save_set_reg", [])
            dev_name = up_bmc_config.get("name", None)

            # save and set reg
            ret, log = self.save_and_set_value(save_set_reg_list)
            if ret == False:
                upgradeerror(log)
                return False, log
            upgradedebuglog("%s save and set cmd all set success" % dev_name)
            time.sleep(0.5)  # delay 0.5s after execute save and set reg

            if chanel == CHANEL_LPC:
                upgrade_print_and_debug_log("upgrade bmc by lpc")
                return self.bmc_upgrade_by_lpc(up_bmc_config, file, chip_select, erase_type)
            elif chanel == CHANEL_REDFISH:
                upgrade_print_and_debug_log("upgrade bmc by redfish")
                return self.bmc_upgrade_by_redfish(up_bmc_config, file, chip_select, erase_type)
            else:
                upgrade_print_and_debug_log("upgrade bmc by usb")
                return self.bmc_upgrade_by_usb(up_bmc_config, file, chip_select, erase_type)

        except Exception as e:
            return False, str(e)
        finally:
            # end operation
            for save_set_reg_config in save_set_reg_list:
                ret, log = self.recover_save_value(save_set_reg_config)
                if ret == False:
                    upgradeerror("%s recover save value failed" % dev_name)
                    upgradeerror(log)
            upgradedebuglog("%s bmc recover save value success" % dev_name)

    def subprocess_bios_upgrade(self, config, file):
        try:
            dev_name = config.get("name", None)
            command = RFU_UPGRADE_CMD % (file)
            bios_config = self.upgrade_param.get(BIOS_CONFIG_STR, {})
            init_cmd_list = bios_config.get("init_cmd", [])
            finish_cmd_list = bios_config.get("finish_cmd", [])
            ret, log = self.do_fw_upg_init_cmd(dev_name, init_cmd_list)
            self.do_fw_upg_finish_cmd_update(dev_name, init_cmd_list, finish_cmd_list)
            if ret is False:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, log
            time.sleep(0.5)  # delay 0.5s after execute init_cmd

            upgradedebuglog("RFU upgrade cmd: %s" % command)
            status, output = exec_os_cmd_log(command)
            if status:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                upgradeerror("%s upgrade failed" % dev_name)
                return False, output

            ret, log = self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            if ret is False:
                return False, log

            upgradewarninglog("%s upgrade success" % dev_name)
            return True, "upgrade success"

        except Exception as e:
            self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            return False, str(e)

    def subprocess_onie_upgrade(self, config, file):
        try:
            dev_name = config.get("name", None)

            command = "%s %s" % (UPGRADE_ONIE_TOOL, file)
            upgradedebuglog("ONIE upgrade cmd: %s" % command)
            status, output = exec_os_cmd_log(command)
            if status:
                upgradeerror("%s upgrade failed" % dev_name)
                return False, output
            upgradewarninglog("%s upgrade success" % dev_name)
            return True, "upgrade success"

        except Exception as e:
            return False, str(e)

    # The main card ID and sub-card ID in the header of the upgrade file are verified and obtained by checksum with the configuration file and device e2
    # param  :e2_card_id_config:read e2 card id config in config file
    #          header_card_id:main or children card id in upgrade file
    #          card_flag: 0:main card 1:children card
    def card_id_check(self, e2_card_id_config, header_card_id, card_flag):
        try:
            upgradedebuglog("do card_id_check")
            if header_card_id is None :
                return False, ("header_card_id get failed")
            # children card requier config how to get e2 id, need revised to use the universal method in the future.
            if card_flag == CHILDREN_CARD and len(e2_card_id_config) == 0:
                return False, ("children card id config config err")

            # if config file define how to get e2 card id, use it, if not use universal method to get
            upgradedebuglog("card_id_check get e2 card id")
            if len(e2_card_id_config) != 0:
                ret, e2_id = get_value(e2_card_id_config)
                if ret is False:
                    msg = "e2_card_id_config:%s get failed. log: %s" % (e2_card_id_config, e2_id)
                    upgradeerror(msg)
                    return False, msg
                upgradedebuglog("e2 get id:%s success" % e2_id)
            else:
                ret, val_tmp = get_board_id()
                if ret is False:
                    msg = "get_board_id failed, log: %s" % val_tmp
                    upgradeerror(msg)
                    return False, msg
                e2_id = int(val_tmp, base = 16)
            upgradedebuglog("card_id_check get e2 card id success. e2_id: 0x%x" % e2_id)

            #set upgrade file header card id to list
            # if DEVTYPE=0x1,0x2  ID_LIST=[0x1, 0x2]
            ID_LIST = header_card_id.split(',')
            ID_LIST = [ int(tmp, base=16) for tmp in ID_LIST ]

            upgradedebuglog("check_id_list: %s" % ID_LIST)
            if e2_id not in ID_LIST:
                msg = "e2_devtype:%s not match check_id_list:%s" % (e2_id, ID_LIST)
                upgradedebuglog(msg)
                return False, msg
            upgradedebuglog("card_type_check get card id:%s success" % e2_id)
            return True, e2_id

        except Exception as e:
            log = "card id check exception happend. reason:%s" % str(e)
            return False, log

    def find_chaininfo_by_firmware_name(self, slot_config, firmware_name):
        try:
            self.chain_list = []
            results = []
            all_names = []
            for filetype in SUPPORT_FILE_TYPE:
                file_type_conf = slot_config.get(filetype, {})
                for chain, chain_conf in file_type_conf.items():
                    if isinstance(chain_conf, dict):
                        name = chain_conf.get('name')
                        if isinstance(name, str):
                            all_names.append(name)
                            if name.lower() == firmware_name.lower():
                                results.append({"filetype": filetype, "chain": chain})
                    elif isinstance(chain_conf, list):
                        for chain_conf_item in chain_conf:
                            name = chain_conf_item.get('name')
                            if isinstance(name, str):
                                all_names.append(name)
                                if name.lower() == firmware_name.lower():
                                    results.append({"filetype": filetype, "chain": chain})
            if len(results) == 1:
                upgradedebuglog(f"Found chain for name '{firmware_name}' at filetype: {results[0]['filetype']}, chain: {results[0]['chain']}")
                self.filetype = results[0]['filetype']
                chain = int(results[0]['chain'].split('chain')[1])
                self.chain_list.append(chain)
                return CHECK_OK, results
            if len(results) > 1:
                msg = f"Multiple matches found for name '{firmware_name}': {results}"
                upgradeerror(msg)
                return ERR_FW_MULTI_TYPE_NAME, msg
            upgradeerror(f"Firmware '{firmware_name}' not found")
            available_names = "\n".join(sorted(set(all_names)))
            upgradeerror(f"Available names: \n{available_names}")
            return ERR_FW_NO_TYPE_FOUND, f"Available names: \n{available_names}"
        except Exception as e:
            msg = traceback.format_exc()
            upgradeerror(msg)
            return ERR_FW_RAISE_EXCEPTION, str(e)

    def file_head_param_check(self, head_info_config, slot_config):
        try:
            self.DEVTYPE = head_info_config.get('DEVTYPE', None)
            self.SUBTYPE = head_info_config.get('SUBTYPE', None)
            self.TYPE = head_info_config.get('TYPE', None)
            self.CHAIN = head_info_config.get('CHAIN', None)
            self.CHIPNAME = head_info_config.get('CHIPNAME', None)
            self.VERSION = head_info_config.get('VERSION', None)
            self.FILETYPE = head_info_config.get('FILETYPE', None)
            self.CRC = head_info_config.get('CRC', None)

            ret, e2_card_id = self.card_id_check(self.e2_devtype_config, self.DEVTYPE, MAIN_CARD)
            if ret != True:
                return ERR_FW_HEAD_CHECK, e2_card_id

            if self.SUBTYPE is not None:
                e2_subtype_config = slot_config.get("e2_subtype", {})
                ret, sub_e2_card_id = self.card_id_check(e2_subtype_config, self.SUBTYPE, CHILDREN_CARD)
                if ret != True:
                    return ERR_FW_HEAD_CHECK, sub_e2_card_id
                self.subtype = sub_e2_card_id

            if len(self.CHAIN) == 0 or len(self.FILETYPE) == 0:
                return ERR_FW_HEAD_CHECK, ("CHAIN:%s, FILETYPE:%s get failed" % (self.CHAIN, self.FILETYPE))

            chain_list = self.CHAIN.split(',')
            self.chain_list = [ int(tmp, base=10) for tmp in chain_list ]
            #set self.devtype to upgrade cmd use
            self.devtype = e2_card_id
            self.filetype = self.FILETYPE
            upgradedebuglog("file head param: devtype:0x%x, subtype:0x%x, chain_list:%s, filetype:%s"
                            % (self.devtype, self.subtype, self.chain_list, self.filetype))
            return CHECK_OK, "SUCCESS"
        except Exception as e:
            return ERR_FW_RAISE_EXCEPTION, str(e)

    def is_file_header_present(self, file):
        """
        Checks if the specified firmware file contains 'FILEHEADER'.
        """
        try:
            with open(file, 'r', errors='ignore') as fd:
                # Read the entire file content
                rdbuf = fd.read(MAX_HEADER_SIZE)
                # Check if 'FILEHEADER' is present in the file content
                if 'FILEHEADER' in rdbuf:
                    return CHECK_OK, FIlE_WITH_HEADER
                else:
                    return CHECK_OK, FIlE_WITHOUT_HEADER
        except Exception as e:
            msg = "parse %s head failed, msg: %s" % (file, str(e))
            upgradeerror(msg)
            return ERR_FW_RAISE_EXCEPTION, msg

    def get_file_size_k(self, file):
        fsize = os.path.getsize(file)
        fsize = fsize / float(1024)
        return round(fsize, 2)

    def get_device_model(self, conf):
        ret, val = get_value(conf)
        if ret is False:
            msg = "get device model failed, msg: %s" % val
            return False, msg
        decode_val = conf.get("decode")
        if decode_val is None:
            return True, val
        for k, v in decode_val.items():
            if val == v:
                return True, k
        msg = "device model decode error, val: %s" % val
        return False, msg

    def upgrade_fool_proofing(self, conf):
        try:
            status, dev_model = self.get_device_model(conf)
            if status is False:
                msg = "upgrade fool proofing get device model failed, msg: %s" % dev_model
                upgradeerror(msg)
                return False, msg
            upgradedebuglog("get device model success, device model: %s" % dev_model)
            if dev_model != self.VERSION:
                msg = "upgrade fool proofing not ok, device model: %s, upgrade file version: %s" % (
                    dev_model, self.VERSION)
                upgradedebuglog(msg)
                return True, False
            msg = "upgrade fool proofing ok, device model: %s, upgrade file version: %s" % (dev_model, self.VERSION)
            upgradedebuglog(msg)
            return True, True
        except Exception as e:
            upgradeerror(str(e))
            return False, str(e)

    def upgrade_vr_fool_proofing(self, bus, addr):
        try:
            ret, chip_id = get_vr_chip_id(bus, addr)
            if ret is False:
                msg = "get_vr_chip_id failed, msg: %s" % chip_id
                upgradeerror(msg)
                return False, msg
            upgradedebuglog("get_vr_chip_id success, chip_id: 0x%x" % chip_id)

            chip_id_list = self.CHIPNAME.split(',')
            chip_id_list = [ int(tmp, base=16) for tmp in chip_id_list ]
            if chip_id not in chip_id_list:
                msg = "vr upgrade fool proofing not ok, chip_id: %s, chip_id_list: %s" % (chip_id, chip_id_list)
                upgradedebuglog(msg)
                return True, False
            msg = "vr upgrade fool proofing ok, chip_id: %s, chip_id_list: %s" % (chip_id, chip_id_list)
            upgradedebuglog(msg)
            return True, True
        except Exception as e:
            upgradeerror(str(e))
            return False, str(e)

    def upgrading(self, config, file, devtype, subtype, slot, option_flag, chanel, erase_type, filetype, chain):
        dev_name = config.get("name", None)
        if option_flag == COLD_UPGRADE:
            status, output = self.subprocess_firmware_upgrade(config, file, devtype, subtype, slot, filetype, chain)
        elif option_flag == WARM_UPGRADE:
            status, output = self.subprocess_warm_upgrade(config, file, devtype, subtype, slot, filetype, chain)
        elif option_flag == TEST_UPGRADE:
            status, output = self.subprocess_test_upgrade(config, file, devtype, subtype, slot, filetype, chain)
        elif option_flag == BMC_UPGRADE:
            status, output = self.subprocess_bmc_upgrade(config, file, slot, chanel, erase_type)
        elif option_flag == BIOS_UPGRADE:
            status, output = self.subprocess_bios_upgrade(config, file)
        elif option_flag == ONIE_UPGRADE:
            status, output = self.subprocess_onie_upgrade(config, file)
        else:
            log = "%s set error option flag" % dev_name
            upgradeerror(log)
            return False, log

        if status is False:
            upgradeerror("%s upgrade failed" % dev_name)
            return False, output
        upgradedebuglog("%s upgrade success" % dev_name)
        return True, "upgrade success"

    def initial_check_fool_proofing(self,   chain_config):
        fool_proofing = chain_config.get("fool_proofing")
        if fool_proofing is not None:
            upgradedebuglog("do fool proofing check...")
            status, fool_proofing_status = self.upgrade_fool_proofing(fool_proofing)
            if status is False:
                msg = "upgrade fool proofing check failed, msg: %s" % fool_proofing_status
                upgradedebuglog(msg)
                return ERR_FW_DEVICE_ACCESS, msg
            if fool_proofing_status is False:
                msg = "upgrade fool proofing check not ok"
                upgradedebuglog(msg)
                return ERR_FW_FOOL_PROOF, msg
            upgradedebuglog("do fool proofing check ok")

        vr_fool_proofing = chain_config.get("vr_fool_proofing", 0)
        upgrade_way = chain_config.get("upgrade_way", UPGRADE_BY_FIRMWARE_UPGRADE_COMMON)
        if (vr_fool_proofing == 1) or (self.filetype == "BASH" and upgrade_way == UPGRADE_BY_VR_SCRIPT):
            upgradedebuglog("do VR fool proofing check...")
            bus = chain_config.get("bus")
            addr = chain_config.get("addr")
            if bus is None or addr is None:
                msg = "VR fool proofing is needed but bus or addr is None, bus: %s, addr: %s" % (bus, addr)
                upgradeerror(msg)
                return ERR_FW_CONFIG_FOUND, msg

            status, vr_fool_proofing_status = self.upgrade_vr_fool_proofing(bus, addr)
            if status is False:
                msg = "VR upgrade fool proofing check failed, msg: %s" % vr_fool_proofing_status
                upgradedebuglog(msg)
                return ERR_FW_DEVICE_ACCESS, msg
            if vr_fool_proofing_status is False:
                msg = "VR upgrade fool proofing check not ok"
                upgradedebuglog(msg)
                return ERR_FW_FOOL_PROOF, msg
            upgradedebuglog("VR fool proofing check ok")

        msg = "initial_check_fool_proofing success"
        upgradedebuglog(msg)
        return CHECK_OK, msg

    def initial_check_chain(self, file, upg_type, chain_config):
        ret, msg = self.initial_check_fool_proofing(chain_config)
        if ret != CHECK_OK:
            return ret, msg

        if upg_type == WARM_UPGRADE:
            upgradedebuglog("do support warm upgrade check...")
            if chain_config.get("is_support_warm_upg", 0) != 1:
                msg = "chain config not support warm upgrade"
                upgradedebuglog(msg)
                return ERR_FW_NOSUPPORT_HOT, msg
            upgradedebuglog("chain config support warm upgrade check ok")

        filesizecheck = chain_config.get("filesizecheck", 0)
        if filesizecheck != 0:
            upgradedebuglog("do file size check...")
            file_size = self.get_file_size_k(file)
            if file_size > filesizecheck:
                msg = "filesizecheck failed: file size: %s exceed check size: %s" % (file_size, filesizecheck)
                upgradedebuglog(msg)
                return ERR_FW_CHECK_SIZE, msg
            msg = "filesizecheck ok, file size: %s, check size: %s" % (file_size, filesizecheck)
            upgradedebuglog(msg)

        msg = "initial_check_chain success"
        upgradedebuglog(msg)
        return CHECK_OK, msg

    # update file_list through parse chain_config and params
    def initial_check_chain_update_flie_list(self, file, slot, chip_select, upg_type, chain_config, file_list, firmware_name):
        if isinstance(chain_config, dict):
            # chain_config is dictionary chip_select must None
            if chip_select is not None:
                msg = ("Invalid chip_select: %s. file: %s, slot: %s, filetype: %s, chain: %s config is dictionary, but chip_select is not None" %
                    (chip_select, file, slot, self.filetype, self.chain))
                upgradedebuglog(msg)
                return ERR_FW_INVALID_PARAM, msg
            ret, log = self.initial_check_chain(file, upg_type, chain_config)
            if ret != CHECK_OK:
                msg = ("initial_check_chain failed, file: %s, slot: %s, filetype: %s, chain: %s, log: %s" %
                    (file, slot, self.filetype, self.chain, log))
                upgradedebuglog(msg)
                return ret, msg
            file_instance = FileUpg(chain_config, file, self.devtype, self.subtype, slot, self.filetype, self.chain, upg_type)
            file_list.append(file_instance)
            msg = ("initial_check_chain success, file: %s, slot: %s, filetype: %s, chain: %s, append to file_list" %
                    (file, slot, self.filetype, self.chain))
            upgradedebuglog(msg)
            return CHECK_OK, msg

        # chain_config is list and firmware_name is not None
        if firmware_name is not None:
            for index, chain_item in enumerate(chain_config):
                if firmware_name.lower() == chain_item.get("name", "").lower():
                    ret, log = self.initial_check_chain(file, upg_type, chain_item)
                    if ret != CHECK_OK:
                        msg = ("initial_check_chain failed, file: %s, slot: %s, filetype: %s, chain: %s, log: %s" %
                            (file, slot, self.filetype, self.chain, log))
                        upgradedebuglog(msg)
                        return ret, msg
                    file_instance = FileUpg(chain_item, file, self.devtype,
                                        self.subtype, slot, self.filetype, self.chain, upg_type, index, chain_item.get("chip_select"))
                    file_list.append(file_instance)
                    msg = ("initial_check_chain success, file: %s, slot: %s, filetype: %s, chain: %s, index: %d, chip_select: %s, append to file_list" %
                        (file, slot, self.filetype, self.chain, index, chip_select))
                    upgradedebuglog(msg)
                    return CHECK_OK, msg
            msg = ("Can't find firmware_name: %s in chain config, file: %s, slot: %s, filetype: %s, chain: %s" %
                    (firmware_name, file, slot, self.filetype, self.chain))
            upgradedebuglog(msg)
            return ERR_FW_CONFIG_FOUND, msg

        # chain_config is list and chip_select is None, traverse all chains
        if chip_select is None:
            for index, chain_item in enumerate(chain_config):
                if chain_item.get("traverse_skip", 0) == 1:
                    upgradedebuglog("file: %s, slot: %s, filetype: %s, chain: %s, index: %d, chip_select: %s, skip to traverse" %
                        (file, slot, self.filetype, self.chain, index, chain_item.get("chip_select")))
                    continue
                ret, log = self.initial_check_chain(file, upg_type, chain_item)
                if ret != CHECK_OK:
                    msg = ("initial_check_chain failed, file: %s, slot: %s, filetype: %s, chain: %s, index: %d, log: %s" %
                        (file, slot, self.filetype, self.chain, index, log))
                    upgradedebuglog(msg)
                    accept_error = (ERR_FW_CARD_ABSENT, ERR_FW_HEAD_CHECK, ERR_FW_FOOL_PROOF)
                    if ret in accept_error:
                        msg = ("file: %s, slot: %s, filetype: %s, chain: %s, index: %d, initial check ret: %d, acceptable error." %
                            (file, slot, self.filetype, self.chain, index, ret))
                        upgradedebuglog(msg)
                        continue
                    return ret, log
                file_instance = FileUpg(chain_item, file, self.devtype,
                                    self.subtype, slot, self.filetype, self.chain, upg_type, index, chain_item.get("chip_select"))
                file_list.append(file_instance)
                upgradedebuglog("initial_check_chain success, file: %s, slot: %s, filetype: %s, chain: %s, index: %d, chip_select: %s, append to file_list" %
                    (file, slot, self.filetype, self.chain, index, chain_item.get("chip_select")))
            msg = ("All chains init success, file: %s, slot: %s, filetype: %s, chain: %s" %
                (file, slot, self.filetype, self.chain))
            upgradedebuglog(msg)
            return CHECK_OK, msg

        # chain_config is list and chip_select is not None, get chip_select chain config
        for index, chain_item in enumerate(chain_config):
            if chip_select == chain_item.get("chip_select"):
                ret, log = self.initial_check_chain(file, upg_type, chain_item)
                if ret != CHECK_OK:
                    msg = ("initial_check_chain failed, file: %s, slot: %s, filetype: %s, chain: %s, log: %s" %
                        (file, slot, self.filetype, self.chain, log))
                    upgradedebuglog(msg)
                    return ret, msg
                file_instance = FileUpg(chain_item, file, self.devtype,
                                    self.subtype, slot, self.filetype, self.chain, upg_type, index, chain_item.get("chip_select"))
                file_list.append(file_instance)
                msg = ("initial_check_chain success, file: %s, slot: %s, filetype: %s, chain: %s, index: %d, chip_select: %s, append to file_list" %
                    (file, slot, self.filetype, self.chain, index, chip_select))
                upgradedebuglog(msg)
                return CHECK_OK, msg
        msg = ("Can't find chip_select: %s in chain config, file: %s, slot: %s, filetype: %s, chain: %s" %
                (chip_select, file, slot, self.filetype, self.chain))
        upgradedebuglog(msg)
        return ERR_FW_CONFIG_FOUND, msg

    def initial_check_slot_present(self, slot_name):
        upgradedebuglog("do %s present check..." % slot_name)
        slot_config = self.upgrade_param.get(slot_name, {})
        slot_present_config = slot_config.get("present", {})
        if len(slot_present_config) != 0:
            ret, log = self.linecard_present_check(slot_present_config)
            if ret != CHECK_OK:
                msg = "check %s present error, msg: %s" % (slot_name, log)
                upgradedebuglog(msg)
                return ret, msg
            msg = "%s present check ok" % slot_name
            upgradedebuglog(msg)
            return CHECK_OK, msg
        msg = "%s don't need present check" % slot_name
        upgradedebuglog(msg)
        return CHECK_OK, msg

    def initial_check_refresh(self, firmware_name, slot, file_list):
        upgradedebuglog("initial_check_refresh, firmware_name: %s, slot: %d" % (firmware_name, slot))
        slot_name = "slot%d" % slot
        ret, msg = self.initial_check_slot_present(slot_name)
        if ret != CHECK_OK:
            return ret, msg

        slot_config = FW_REFRESH_CONFIG.get(slot_name)
        if slot_config is None:
            msg = "Invalid slot: %d" % slot
            return ERR_FW_CONFIG_FOUND, msg

        devtype = self.devtype
        if firmware_name not in slot_config:
            msg = "firmware_name: %s, not found in %s\n" % (firmware_name, slot_name)
            upgradedebuglog(msg.rstrip("\n"))
            return ERR_FW_NO_TYPE_FOUND, msg

        msg = "initial_check_refresh specify the firmware name: %s" % firmware_name
        upgradedebuglog(msg)
        subtype = slot_config[firmware_name]["subtype"]
        filetype = slot_config[firmware_name]["filetype"]
        chain = slot_config[firmware_name]["chain"]
        chain_conf = slot_config[firmware_name]["chain_conf"]
        refresh_instance = FileUpg(chain_conf, None, devtype, subtype, slot, filetype, chain, WARM_UPGRADE, None, None, chain_conf)
        file_list.append(refresh_instance)
        msg = ("%s refresh initial check ok, slot: %d, devtype: 0x%x, subtype: 0x%x, filetype: %s, chain: %d" %
            (firmware_name, slot, devtype, subtype, filetype, chain))
        upgradedebuglog(msg)
        return CHECK_OK, msg

    def initial_check(self, file, slot, upg_type, chip_select, file_list, firmware_name, specify_chain):
        try:
            upgradedebuglog("BasePlatform initial_check, file: %s, slot: %s, upg_type: %s, chip_select: %s, firmware_name: %s, specify_chain: %s" %
                            (file, slot, upg_type, chip_select, firmware_name, specify_chain))

            '''
            upgradedebuglog("do file exist check...")
            if not os.path.isfile(file):
                msg = "%s not found" % file
                upgradedebuglog(msg)
                return ERR_FW_FILE_FOUND, msg
            upgradedebuglog("file exist check ok")
            '''
            slot_name = "slot%d" % slot
            slot_config = self.upgrade_param.get(slot_name, {})
            ret, msg = self.initial_check_slot_present(slot_name)
            if ret != CHECK_OK:
                return ret, msg

            if firmware_name is None:
                upgradedebuglog("do file head parse...")
                self.subtype = slot_config.get("subtype", 0)
                ret, log = parse_file_head(file, self.head_info_config)
                if ret is False:
                    upgradeerror(log)
                    return ret, log
                else:
                    upgradedebuglog(log)
                upgradedebuglog("file head parse success")

                upgradedebuglog("do file head check...")
                ret, log = self.file_head_param_check(self.head_info_config, slot_config)
                if ret != CHECK_OK:
                    msg = "file: %s, head check failed, msg: %s" % (file, log)
                    upgradedebuglog(msg)
                    return ret, msg
                upgradedebuglog("file head check ok")
            else:
                self.subtype = slot_config.get("subtype", 0)
                ret, results = self.find_chaininfo_by_firmware_name(slot_config, firmware_name)
                if ret != CHECK_OK:
                    msg = "file: %s, firmware name find failed, msg: %s" % (file, results)
                    upgradedebuglog(msg)
                    return ret, msg
                self.need_header_flag = False
                ret, log = self.is_file_header_present(file)
                if ret != CHECK_OK:
                    return ret, msg
                if log == FIlE_WITH_HEADER:
                    msg = "It is not supported when firmware_name is specified and %s with header." % file
                    upgradeerror(msg)
                    return ERR_FW_INVALID_PARAM, msg

            upgradedebuglog("get upgrade chain config...")
            filetype_config = slot_config.get(self.filetype, {})
            if len(filetype_config) == 0:
                msg = "file: %s filetype: %s no support" % (file, self.filetype)
                upgradedebuglog(msg)
                return ERR_FW_CONFIG_FOUND, msg

            if specify_chain is not None:
                if specify_chain not in self.chain_list:
                    msg = "Unsupport chain: %d, support chain list: %s" % (specify_chain, self.chain_list)
                    upgradeerror(msg)
                    return ERR_FW_INVALID_PARAM, msg
                # specify_chain to upgrade
                msg = "specify_chain to upgrade, chain_list: %s, specify_chain: %s" % (self.chain_list, specify_chain)
                upgradedebuglog(msg)
                self.chain_list = [specify_chain]

            error_chain = []
            for chain in self.chain_list:
                self.chain = chain
                chain_num = "chain%s" % self.chain
                chain_config = filetype_config.get(chain_num, {})
                if not isinstance(chain_config, dict) and not isinstance(chain_config, list):
                    msg = ("Invalid chain config: %s, file: %s, slot: %s, filetype: %s, chain: %s" %
                        (chain_config, file, slot, self.filetype, self.chain))
                    upgradeerror(msg)
                    error_chain.append(chain)
                    continue

                if len(chain_config) == 0:
                    msg = ("Chain config is empty, file: %s, slot: %s, filetype: %s, chain: %s" %
                        (file, slot, self.filetype, self.chain))
                    upgradeerror(msg)
                    error_chain.append(chain)
                    continue

                upgradedebuglog("get %s filetype: %s %s config success" % (slot_name, self.filetype, chain_num))

                ret, msg = self.initial_check_chain_update_flie_list(file, slot, chip_select, upg_type, chain_config, file_list, firmware_name)
                if ret != CHECK_OK:
                    upgradeerror("%s filetype: %s %s initial_check failed, msg: %s" % (slot_name, self.filetype, chain_num, msg))
                    if ret == ERR_FW_FOOL_PROOF:
                        upgradedebuglog("%s filetype: %s %s initial_check acceptable error" % (slot_name, self.filetype, chain_num))
                        continue
                    error_chain.append(chain)
            if len(error_chain) != 0:
                msg = ("%s filetype: %s multiple chain check error, chain_list: %s, error chain: %s" %
                    (slot_name, self.filetype, self.chain_list, error_chain))
                upgradeerror(msg)
                return ERR_FW_MULTI_CHAIN_CHECK, msg
            msg = "%s filetype: %s multiple chain check ok, chain_list: %s" % (slot_name, self.filetype, self.chain_list)
            upgradedebuglog(msg)
            return CHECK_OK, msg
        except Exception as e:
            msg = traceback.format_exc()
            upgradeerror(msg)
            return ERR_FW_RAISE_EXCEPTION, str(e)

    def do_upgrade_test(self, file, slot, upg_type, success_chain, fail_chain, chip_select):
        try:
            file_test_list = []
            ret, log = self.initial_check(file, slot, upg_type, chip_select, file_test_list, None, None)
            if ret != CHECK_OK:
                return ret, log

            if len(file_test_list) == 0:
                msg = "file_test_list is empty, file: %s, slot: %s" % (file, slot)
                upgradeerror(msg)
                return ERR_FW_NO_FILE_SUCCESS, msg

            # start upgrading
            err_cnt = 0
            msg = ""
            upgradedebuglog("start upgrading")
            for file_test in file_test_list:
                ret, log = self.upgrading(file_test.config, file, self.devtype, self.subtype, slot, upg_type,
                    None, None, file_test.filetype, file_test.chain)
                if ret is False:
                    fail_chain.append(file_test.chain)
                    log = ("Upgrade test failed, file: %s, slot: %s, filetype: %s, chain: %s, index: %s, log: %s\n" %
                        (file, slot, file_test.filetype, file_test.chain, file_test.chain_index, log))
                    upgradeerror(log)
                    err_cnt += 1
                    msg += log
                else:
                    success_chain.append(file_test.chain)
                    upgradedebuglog("Upgrade success, file: %s, slot: %s, filetype: %s, chain: %s, index: %s" %
                        (file, slot, file_test.filetype, file_test.chain, file_test.chain_index))
            if err_cnt != 0:
                return ERR_FW_UPGRADE, msg
            return FIRMWARE_SUCCESS, "SUCCESS"
        except Exception as e:
            return ERR_FW_RAISE_EXCEPTION, str(e)

    def do_pre_check(self, conf):
        ret, val = get_value(conf)
        if ret is False:
            msg = "pre check get value failed, msg: %s" % val
            return False, msg
        ok_val = conf.get("ok_val")
        if val == ok_val:
            msg = "pre check success, ok_val: %s, get value: %s" % (ok_val, val)
            return True, msg
        msg = "pre check failed, ok_val: %s, get value: %s" % (ok_val, val)
        return False, msg

    def generate_test_device_list(self, slot_config, slot_name, slot_index, device):
        device_list = []

        # slot present check
        slot_present_config = slot_config.get("present", {})
        if len(slot_present_config) != 0:
            ret, log = self.linecard_present_check(slot_present_config)
            if ret == ERR_FW_CARD_ABSENT:
                msg = "%s not present" % slot_name
                upgradedebuglog(msg)
                return ret, msg
            elif ret != CHECK_OK:
                msg = "check %s present error, msg: %s" % (slot_name, log)
                upgradedebuglog(msg)
                return ret, msg
            upgradedebuglog("%s present" % slot_name)

        test_config = slot_config.get("TEST", {})
        if len(test_config) == 0:
            upgradedebuglog("%s test config no found" % slot_name)
            return CHECK_OK, []

        tmp_device_list = test_config.get(device, [])
        if len(tmp_device_list) == 0:
            upgradedebuglog("%s logic device %s test config list no found" % (slot_name, device))
            return CHECK_OK, device_list

        for one_device_config in tmp_device_list:
            one_device_config["slot"] = slot_index
            device_list.append(one_device_config)

        return CHECK_OK, device_list

    def do_test(self, device, slot):
        try:
            device_list = []
            if slot is None:
                for slot_name, slot_config in self.upgrade_param.items():
                    if not slot_name.startswith("slot"):
                        continue
                    match = re.search(r"slot(\d+)", slot_name)
                    slot_index = int(match.group(1))
                    ret, val = self.generate_test_device_list(slot_config, slot_name, slot_index, device)
                    if ret == CHECK_OK:
                        device_list.extend(val)
                    elif ret == ERR_FW_CARD_ABSENT:
                        continue
                    else:
                        return ERR_FW_CONFIG_FOUND, val
                if len(device_list) == 0:
                    return ERR_FW_CONFIG_FOUND, ("all slot logic device %s test config list not found" % device)
            else:
                # slot present check
                slot = int(slot)
                slot_name = "slot%d" % slot
                slot_config = self.upgrade_param.get(slot_name, {})
                ret , val = self.generate_test_device_list(slot_config, slot_name, slot, device)
                if ret != CHECK_OK:
                    return ERR_FW_CONFIG_FOUND, val
                device_list.extend(val)
                if len(device_list) == 0:
                    return ERR_FW_CONFIG_FOUND, ("logic device %s test config list not found" % device)

            # test_file existence check
            for test_config in device_list:
                test_file = test_config.get("file", None)
                display_name = test_config.get("display_name", None)
                if test_file is None or display_name is None:
                    log = "test_config:%s lack of config" % test_config
                    upgradeerror(log)
                    return ERR_FW_CONFIG_FOUND, log
                if not os.path.isfile(test_file):
                    return ERR_FW_FILE_FOUND, ("%s not found" % test_file)

            # start testing
            RET = 0
            pre_check_failed = 0
            pre_check_failed_summary = ""
            failed_summary = "chain test failed.\ntest fail chain:"
            success_summary = "test success chain:"
            for test_config in device_list:
                success_chain = []
                fail_chain = []
                test_file = test_config.get("file", None)
                display_name = test_config.get("display_name", None)
                chip_select = test_config.get("chip_select", None)
                pre_check_conf = test_config.get("pre_check", None)
                slot = test_config.get("slot")
                if pre_check_conf is not None:
                    status, msg = self.do_pre_check(pre_check_conf)
                    if status is False:
                        pre_check_failed += 1
                        log = "\nname:%s, pre check failed, msg: %s" % (display_name, msg)
                        upgradedebuglog(log)
                        pre_check_failed_summary += log
                        continue
                    upgradedebuglog("name:%s, pre check ok, msg: %s" % (display_name, msg))
                ret, log = self.do_upgrade_test(test_file, slot, TEST_UPGRADE, success_chain, fail_chain, chip_select)
                if ret != FIRMWARE_SUCCESS:
                    RET = -1
                    if len(fail_chain) != 0:
                        for chain in fail_chain:
                            upgradeerror("slot:%d, chain:%d, name:%s test failed" % (slot, chain, display_name))
                            failed_summary += "\n    slot:%d, chain:%d, name:%s;" % (slot, chain, display_name)
                    else:
                        upgradeerror("slot:%d, name:%s test failed" % (slot, display_name))
                        failed_summary += "\n    slot:%d, name:%s;" % (slot, display_name)
                else:
                    for chain in success_chain:
                        upgradedebuglog("slot:%d, chain:%d, name:%s test success" % (slot, chain, display_name))
                        success_summary += "\n    slot:%d, chain:%d, name:%s;" % (slot, chain, display_name)
            if RET != 0:
                return ERR_FW_UPGRADE, failed_summary
            if pre_check_failed == len(device_list):
                return ERR_FW_NO_FILE_SUCCESS, failed_summary + pre_check_failed_summary
            return FIRMWARE_SUCCESS, success_summary
        except Exception as e:
            return ERR_FW_RAISE_EXCEPTION, str(e)

    def do_test_main(self, device, slot):
        print("+================================+")
        print("|Doing upgrade test, please wait.|")
        exec_os_cmd("touch %s" % FW_UPGRADE_STARTED_FLAG)
        exec_os_cmd("sync")
        ret, log = self.do_test(device, slot)
        exec_os_cmd("rm -rf %s" % FW_UPGRADE_STARTED_FLAG)
        exec_os_cmd("sync")
        if ret == FIRMWARE_SUCCESS:
            print("|         test succeeded!        |")
            print("+================================+")
            print(log)
            sys.exit(0)
        else:
            print("|         test failed!           |")
            print("+================================+")
            print("FAILED REASON:")
            print(log)
            sys.exit(1)

    def do_bmc_upgrade_main(self, file, chip_select, chanel, erase_type):
        bmc_upgrade_config = self.upgrade_param.get("BMC", {})
        exec_os_cmd("touch %s" % FW_UPGRADE_STARTED_FLAG)
        exec_os_cmd("sync")
        ret, log = self.upgrading(bmc_upgrade_config, file, self.devtype,
                                  self.subtype, chip_select, BMC_UPGRADE, chanel, erase_type, None, None)
        exec_os_cmd("rm -rf %s" % FW_UPGRADE_STARTED_FLAG)
        exec_os_cmd("sync")
        if ret is True:
            print("===========upgrade succeeded!============")
            sys.exit(0)
        else:
            print("============upgrade failed!==============")
            print("FAILED REASON:")
            print("%s" % log)
            sys.exit(1)

    def do_bios_upgrade_main_by_rfu(self, file):
        bios_upgrade_config = {
            "name": "BIOS",
        }
        ret, log = self.upgrading(bios_upgrade_config, file, self.devtype,
                                  self.subtype, None, BIOS_UPGRADE, None, None, None, None)
        if ret == True:
            print("===========upgrade succeeded!============")
            exit(0)
        else:
            print("============upgrade failed!==============")
            print("FAILED REASON:")
            print(("%s" % log))
            exit(1)

    def do_fw_sync_result_output(self, ret, log):
        if ret == True:
            print("===========upgrade succeeded!============")
            exit(0)
        else:
            print("============upgrade failed!==============")
            print("FAILED REASON:")
            print(("%s" % log))
            exit(1)

    def do_fw_sync(self, type):
        fw_sync_type_list = ['BIOS', ]

        if type not in fw_sync_type_list:
            log = "fw sync type:%s is not support" % (type)
            self.do_fw_sync_result_output(False, log)

        if self.fw_sync_cfg is None:
            self.do_fw_sync_result_output(False, "fw_sync_cfg is none")

        for index in range(len(self.fw_sync_cfg)):
            target_type = self.fw_sync_cfg[index].get('fw_sync_type', None)
            if target_type == type:
                master_slave_status_cfg = self.fw_sync_cfg[index].get('master_slave_status', None)
                if master_slave_status_cfg is None:
                    self.do_fw_sync_result_output(False, "master_slave_status cfg is none")

                ret, master_slave_status = get_value(master_slave_status_cfg)
                if ret is False:
                    self.do_fw_sync_result_output(False, "get master_slave_status value fail")

                master_sync_to_slave_cfg = self.fw_sync_cfg[index].get('master_sync_to_slave', None)
                slave_sync_to_master_cfg = self.fw_sync_cfg[index].get('slave_sync_to_master', None)
                if (master_sync_to_slave_cfg is None) or (slave_sync_to_master_cfg is None):
                    self.do_fw_sync_result_output(False, "master_sync_to_slave cfg is None or slave_sync_to_master cfg is none")

                target_status_val0 = master_sync_to_slave_cfg.get('target_status_val', None)
                target_status_val1 = slave_sync_to_master_cfg.get('target_status_val', None)
                if (target_status_val0 is None) or (target_status_val1 is None):
                    self.do_fw_sync_result_output(False, "target_status_val is none")

                if master_slave_status in target_status_val0:
                    sync_cmd_set = master_sync_to_slave_cfg.get('sync_cmd_set', None)
                elif master_slave_status in target_status_val1:
                    sync_cmd_set = slave_sync_to_master_cfg.get('sync_cmd_set', None)
                else:
                    self.do_fw_sync_result_output(False, "master_slave_status not in target_status_val")

                if sync_cmd_set is None:
                    self.do_fw_sync_result_output(False, "sync_cmd_set is none")

                for cmd in sync_cmd_set:
                    ret, log = set_value(cmd)
                    if ret == False:
                        self.do_fw_sync_result_output(False, log)

                self.do_fw_sync_result_output(True, "do fw sync success")

        log = "no find fw_sync_type == %s cfg" % (type)
        self.do_fw_sync_result_output(False, log)

    def do_onie_upgrade_main(self, file):
        onie_upgrade_config = {
            "name": "ONIE",
        }
        ret, log = self.upgrading(onie_upgrade_config, file, self.devtype,
                                  self.subtype, None, ONIE_UPGRADE, None, None, None, None)
        if ret == True:
            print("===========upgrade succeeded!============")
            exit(0)
        else:
            print("============upgrade failed!==============")
            exit(1)

# single file upgrade operation
class FileUpg(object):
    def __init__(self, config, file, devtype, subtype, slot, filetype, chain, upg_type, chain_index=None, chip_select=None, refresh_config=None):
        self.config = config
        self.file = file
        self.devtype = devtype
        self.subtype = subtype
        self.slot = slot
        self.filetype = filetype # The type of the firmware file (e.g., cpld, fpga, bcm, etc.).
        self.chain = chain
        self.upg_type = upg_type # The type of upgrade (cold, warm, etc.).
        self.chain_index = chain_index
        self.chip_select = chip_select
        self.refresh_config = refresh_config

    def __repr__(self):
        if self.chip_select is not None:
            return "file:%s slot:%d filetype:%s chain:%d chip_select:%s" % (self.file, self.slot, self.filetype, self.chain, self.chip_select)
        return "file:%s slot:%d filetype:%s chain:%d" % (self.file, self.slot, self.filetype, self.chain)

# manage multiple files update
class FwUpg(object):
    def __init__(self):
        self.upg_platform = BasePlatform()
        self.warm_upg_platform = WarmBasePlatform()
        self.max_slot_num = self.upg_platform.max_slot_num
        self.file_list = []
        self.refresh_list = WARM_UPGRADE_PARAM.get("refresh_list", [])

    def do_file_refresh(self, fw_upg_instance):
        fw_upg_config = fw_upg_instance.config
        fw_upg_file = fw_upg_instance.file
        fw_upg_devtype = fw_upg_instance.devtype
        fw_upg_subype = fw_upg_instance.subtype
        fw_upg_slot = fw_upg_instance.slot
        fw_upg_filetype = fw_upg_instance.filetype
        fw_upg_chain = fw_upg_instance.chain
        refresh_config = fw_upg_instance.refresh_config
        dev_name = fw_upg_config.get("name", None)
        upgrade_way = fw_upg_config.get("upgrade_way", UPGRADE_BY_FIRMWARE_UPGRADE_COMMON)
        if upgrade_way == UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER:
            status, new_file_type, new_chain = update_filetype_and_chain(fw_upg_config)
            if status is False:
                msg = ("upgrade way: %s new_file_type or new_chain not define, new_file_type: %s, new_chain: %s" %
                    (upgrade_way, new_file_type, new_chain))
                return False, msg
            upgradedebuglog("new_file_type: %s, new_chain: %s" % (new_file_type, new_chain))
            fw_upg_filetype = new_file_type
            fw_upg_chain = new_chain

        upgradedebuglog("%s start warm upgrade, file: %s, devtype:0x%x, subtype: 0x%x, slot: %d, filetype: %s, chain: %d" %
                        (dev_name, fw_upg_file, fw_upg_devtype, fw_upg_subype, fw_upg_slot, fw_upg_filetype, fw_upg_chain))
        status, output = self.warm_upg_platform.do_warmupgrade(fw_upg_file, fw_upg_devtype, fw_upg_subype, fw_upg_slot,
                                                               fw_upg_filetype, fw_upg_chain, refresh_config)
        if status is False:
            upgradeerror("%s warm upgrade failed, msg: %s" % (dev_name, output))
            return False, output
        upgradedebuglog("%s warm upgrade success" % dev_name)
        return True, "upgrade success"

    def do_refresh(self):
        try:
            exec_os_cmd("touch %s" % WARM_UPGRADE_STARTED_FLAG)
            exec_os_cmd("sync")

            # stop upper layer services access
            ret, log = self.warm_upg_platform.stop_services_access()
            if ret is False:
                upgradeerror("stop upper layer services access failed")
                upgradeerror(log)
                return ERR_FW_UPGRADE, log
            upgradedebuglog("stop upper layer services access success")

            # gloabl refresh init command
            global_init_cmd = self.warm_upg_platform.global_init_cmd
            ret, log = self.upg_platform.do_fw_upg_init_cmd("global_init_cmd", global_init_cmd)
            if ret is False:
                return ERR_FW_UPGRADE, log

            refresh_success_msg = ""
            for file_instance in self.file_list:
                file_info = repr(file_instance)
                ret, log = self.do_file_refresh(file_instance)
                if ret is False:
                    msg = "firmware_name: %s, slot: %d, filetype: %s, chain: %d refresh failed\n" % \
                        (file_instance.config.get("name"), file_instance.slot, file_instance.filetype, file_instance.chain)
                    upgradeerror(msg)
                    if len(refresh_success_msg) > 0:
                        msg += "\nAlready Refresh List: \n" + refresh_success_msg
                    return ERR_FW_UPGRADE, msg
                upgradedebuglog("%s %s refresh success." % (file_instance.config.get("name"), file_info))
                refresh_success_msg += ("firmware_name: %s, slot: %d, filetype: %s, chain: %d\n" %
                    (file_instance.config.get("name"), file_instance.slot, file_instance.filetype, file_instance.chain))
            msg = "all files refresh success."
            upgradedebuglog(msg)
            return FIRMWARE_SUCCESS, refresh_success_msg
        except Exception as e:
            msg = "do warm upg exception happend. log:%s" % str(e)
            upgradeerror(msg)
            return ERR_FW_UPGRADE, msg
        finally:
            # gloabl refresh finish command
            global_finish_cmd = self.warm_upg_platform.global_finish_cmd
            self.upg_platform.do_fw_upg_init_cmd("global_finish_cmd", global_finish_cmd)
            # start upper layer services access
            self.warm_upg_platform.start_services_access()
            if os.path.isfile(WARM_UPGRADE_STARTED_FLAG):
                exec_os_cmd("rm -rf %s" % WARM_UPGRADE_STARTED_FLAG)
                exec_os_cmd("sync")

    def do_file_cold_upg(self, fw_upg_instance):
        try:
            upgradedebuglog("start cold upgrade")
            fw_upg_config = fw_upg_instance.config
            fw_upg_file = fw_upg_instance.file
            fw_upg_devtype = fw_upg_instance.devtype
            fw_upg_subype = fw_upg_instance.subtype
            fw_upg_slot = fw_upg_instance.slot
            fw_upg_filetype = fw_upg_instance.filetype
            fw_upg_chain = fw_upg_instance.chain
            ret, log = self.upg_platform.upgrading(
                fw_upg_config, fw_upg_file, fw_upg_devtype, fw_upg_subype, fw_upg_slot, COLD_UPGRADE,
                None, None, fw_upg_filetype, fw_upg_chain)
            if ret is False:
                upgradeerror("cold upgrade %s slot%d failed, log:%s" % (fw_upg_file, fw_upg_slot, log))
                return ERR_FW_UPGRADE, log
            log = "cold upgrade %s slot%d success" % (fw_upg_file, fw_upg_slot)
            upgradedebuglog(log)
            return FIRMWARE_SUCCESS, log
        except Exception as e:
            msg = "do cold upg exception happend. log:%s" % str(e)
            upgradeerror(msg)
            return ERR_FW_UPGRADE, msg

    def do_refresh_init_check(self, firmware_name, slot):
        firmware_name = firmware_name.upper()
        if firmware_name == FW_REFRESH_ALL:
            if slot is None:  # traverse all slots
                for i in range(0, self.max_slot_num + 1):
                    slot_config = FW_REFRESH_CONFIG.get("slot%d" % i)
                    if slot_config is None:
                        msg = "slot%d do not have refresh_config, skip" % i
                        upgradedebuglog(msg)
                        continue
                    for fw_name in slot_config:
                        ret, log = self.upg_platform.initial_check_refresh(fw_name, i, self.file_list)
                        accept_ret = (CHECK_OK, ERR_FW_CARD_ABSENT)
                        # if check_ok or slot absent
                        if ret in accept_ret:
                            msg = "firmware_name: %s, slot%d refresh initial check ret: %d, acceptable ret." % (fw_name, i, ret)
                            upgradedebuglog(msg)
                            continue
                        return ret, log
            else:
                slot_config = FW_REFRESH_CONFIG.get("slot%d" % slot)
                if slot_config is None:
                    msg = "slot%d refresh config not find" % slot
                    return ERR_FW_CONFIG_FOUND, msg
                else:
                    for fw_name in slot_config:
                        ret, log = self.upg_platform.initial_check_refresh(fw_name, slot, self.file_list)
                        if ret != CHECK_OK:
                            return ret, log
        else:
            fw_name_list = firmware_name.split(':')
            for fw_name in fw_name_list:
                if slot is None:  # traverse all slots
                    fw_refresh_config_find = False
                    for i in range(0, self.max_slot_num + 1):
                        ret, log = self.upg_platform.initial_check_refresh(fw_name, i, self.file_list)
                        if ret == CHECK_OK:
                            fw_refresh_config_find = True
                    if fw_refresh_config_find == False:
                        return ERR_FW_CONFIG_FOUND, "Can't find %s refresh config, please execute upgrade.py refresh -h to get help" % fw_name
                else:
                    ret, log = self.upg_platform.initial_check_refresh(fw_name, slot, self.file_list)
                    if ret != CHECK_OK:
                        msg = "firmware_name: %s, slot%d refresh initial check not ok, ret: %d, msg: %s" % (fw_name, slot, ret, log)
                        return ret, msg

        msg = "firmware_name: %s all slots init check ok" % firmware_name
        upgradedebuglog(msg)
        return CHECK_OK, msg


    def do_file_init_check(self, file_path, slot, upg_type, chip_select, firmware_name, specify_chain):
        upgradedebuglog("do_file_init_check, file_path: %s, slot: %s, upg_type: %s, chip_select: %s, firmware_name: %s, specify_chain: %s" %
            (file_path, slot, upg_type, chip_select, firmware_name, specify_chain))

        if slot is None:  # traverse all slots
            for i in range(0, self.max_slot_num + 1):
                ret, log = self.upg_platform.initial_check(file_path, i, upg_type, chip_select, self.file_list, firmware_name, specify_chain)
                if ret != CHECK_OK:
                    upgradedebuglog(
                        "file: %s, slot%d initial check not ok, ret: %d, msg: %s" %
                        (file_path, i, ret, log))
                    accept_error = (ERR_FW_CARD_ABSENT, ERR_FW_HEAD_CHECK, ERR_FW_FOOL_PROOF, ERR_FW_NO_TYPE_FOUND)
                    if ret in accept_error:
                        msg = "file: %s, slot%d initial check ret: %d, acceptable error." % (file_path, i, ret)
                        upgradedebuglog(msg)
                        continue
                    return ret, log
        else:
            slot = int(slot, 10)
            ret, log = self.upg_platform.initial_check(file_path, slot, upg_type, chip_select, self.file_list, firmware_name, specify_chain)
            if ret != CHECK_OK:
                msg = "file: %s, slot%d initial check not ok, ret: %d,  msg: %s" % (file_path, slot, ret, log)
                return ret, msg
        msg = "file: %s all slots init check ok" % file_path
        return CHECK_OK, msg

    def do_dir_init_check(self, path, slot, upg_type, chip_select, firmware_name, specify_chain):
        for root, dirs, names in os.walk(path):
            # root: directory absolute path
            # dirs: folder path collection under directory
            # names: file path collection under directory
            for filename in names:
                # file_path is file absolute path
                file_path = os.path.join(root, filename)
                ret, log = self.do_file_init_check(file_path, slot, upg_type, chip_select, firmware_name, specify_chain)
                if ret != CHECK_OK:
                    return ret, log
        msg = "all files in dir have been check ok"
        upgradedebuglog(msg)
        return CHECK_OK, msg

    def do_fw_upg(self, path, slot, upg_type, chip_select, firmware_name, specify_chain):
        match_zip_file_flag = False
        try:
            upgradedebuglog("do_fw_upg, path: %s, slot: %s, chip_select: %s upg_type: %s, firmware_name: %s, specify_chain: %s" %
                (path, slot, chip_select, upg_type, firmware_name, specify_chain))

            if slot is not None and not slot.isdigit():
                if chip_select is not None:
                    msg = "Invalid params, slot: %s, chip_select: %s" % (slot, chip_select)
                    upgradeerror(msg)
                    return ERR_FW_INVALID_PARAM, msg
                chip_select = slot
                slot = None
                upgradedebuglog("After adjust params slot %s, chip_select %s" % (slot, chip_select))

            if firmware_name is not None and chip_select is not None:
                msg = ("Invalid params, firmware_name: %s, chip_select must be None, but it is actually %s" %
                    (firmware_name, chip_select))
                upgradeerror(msg)
                return ERR_FW_INVALID_PARAM, msg

            if firmware_name is not None and specify_chain is not None:
                msg = ("Invalid params, firmware_name: %s, specify_chain must be None, but it is actually %s" %
                    (firmware_name, specify_chain))
                upgradeerror(msg)
                return ERR_FW_INVALID_PARAM, msg

            upgradedebuglog("start init check")
            if os.path.isfile(path) and path.endswith(".zip"):
                if firmware_name is not None:
                    msg = "path upgrade is not supported when firmware_name: %s is specified." % firmware_name
                    upgradeerror(msg)
                    return ERR_FW_INVALID_PARAM, msg
                upgradedebuglog("firmware upgrade via compressed package: %s" % path)
                # remove origin firmware upgrade file
                exec_os_cmd("rm -rf %s" % UPGRADE_FILE_DIR)
                cmd = "unzip -o %s -d /tmp/" % path
                if is_upgrade_debug_mode():
                    status, output = exec_os_cmd_log(cmd)
                else:
                    status, output = exec_os_cmd(cmd)
                if status:
                    msg = "unzip %s failed, log: %s" % (path, output)
                    upgradeerror(msg)
                    return ERR_FW_UNZIP_FAILED, msg
                match_zip_file_flag = True
                path = UPGRADE_FILE_DIR

            if os.path.isdir(path):
                if firmware_name is not None:
                    msg = "directory upgrade is not supported when firmware_name: %s is specified." % firmware_name
                    upgradeerror(msg)
                    return ERR_FW_INVALID_PARAM, msg
                ret, msg = self.do_dir_init_check(path, slot, upg_type, chip_select, firmware_name, specify_chain)
            elif os.path.isfile(path):
                ret, msg = self.do_file_init_check(path, slot, upg_type, chip_select, firmware_name, specify_chain)
            else:
                ret = ERR_FW_FILE_FOUND
                msg = "path: %s not found" % path
                upgradeerror(msg)

            if ret != CHECK_OK:
                return ret, msg

            # self.file_list is a collection of all check ok files
            if len(self.file_list) == 0:
                msg = "all file upgrade check not be satisfied."
                upgradeerror(msg)
                return ERR_FW_NO_FILE_SUCCESS, msg

            SUCCUSS_FILE_SUMMARY = "SUCCESS FILE: \n"
            # file cold upgrade
            upgradedebuglog("start all files cold upgrade, file_list len: %d" % len(self.file_list))
            for file_instance in self.file_list:
                file_info = repr(file_instance)
                upgradedebuglog("%s start to cold upgrade" % file_info)
                ret, log = self.do_file_cold_upg(file_instance)
                if ret != FIRMWARE_SUCCESS:
                    msg = "%s cold upgrade failed, ret:%d, \n log:\n%s." % (file_info, ret, log)
                    upgradeerror(msg)
                    return ret, msg
                SUCCUSS_FILE_SUMMARY += "%s \n" % file_info
                upgradedebuglog("%s cold upgrade success." % file_info)

            # file refresh upgrade
            if upg_type == WARM_UPGRADE:
                upgradedebuglog("start all files refresh upgrade")
                ret, log = self.do_refresh()
                if ret != FIRMWARE_SUCCESS:
                    return ret, log

            msg = "all file upgrade success"
            upgradedebuglog(msg)
            return FIRMWARE_SUCCESS, SUCCUSS_FILE_SUMMARY
        except Exception as e:
            msg = "do fw upgrade exception happend. log: %s" % str(e)
            upgradeerror(msg)
            return ERR_FW_UPGRADE, msg
        finally:
            if match_zip_file_flag is True:
                exec_os_cmd("rm -rf %s" % UPGRADE_FILE_DIR)

    # firmware_name: An optional parameter for specifying the firmware type. (FCB_CPLD, etc.).
    def fw_upg(self, path, slot, upg_type, chip_select, firmware_name, specify_chain):
        print("+================================+")
        print("|  Doing upgrade, please wait... |")
        exec_os_cmd("touch %s" % FW_UPGRADE_STARTED_FLAG)
        exec_os_cmd("sync")
        ret, log = self.do_fw_upg(path, slot, upg_type, chip_select, firmware_name, specify_chain)
        exec_os_cmd("rm -rf %s" % FW_UPGRADE_STARTED_FLAG)
        exec_os_cmd("sync")
        if ret == FIRMWARE_SUCCESS:
            print("|       upgrade succeeded!       |")
            print("+================================+")
            print(log)
            sys.exit(0)
        else:
            print("|        upgrade failed!         |")
            print("+================================+")
            print("FAILED REASON:")
            print("%s" % log)
            sys.exit(1)

    def do_refresh_list_sort(self):
        tmp_file_list = []
        tail_file_list = []

        if len(self.refresh_list) > 0:
            upgradedebuglog("do_refresh_list_sort")
            upgradedebuglog("raw file_list: %s" % self.file_list)
            for refresh_name in self.refresh_list:
                for file_instance in self.file_list:
                    if file_instance.refresh_config.get("name") == refresh_name:
                        tmp_file_list.append(file_instance)
            for file_instance in self.file_list:
                if file_instance.refresh_config.get("name") not in self.refresh_list:
                    tail_file_list.append(file_instance)
            tmp_file_list.extend(tail_file_list)
            self.file_list = tmp_file_list
            upgradedebuglog("after file_list: %s" % self.file_list)

    def do_fw_refresh(self, firmware_name, slot):
        try:
            upgradedebuglog("do_fw_refresh, firmware_name: %s, slot: %s" % (firmware_name, slot))
            if len(FW_REFRESH_CONFIG_ERRMSG) != 0:
                upgradeerror(FW_REFRESH_CONFIG_ERRMSG)
                return ERR_FW_MULTI_CHAIN_CHECK, FW_REFRESH_CONFIG_ERRMSG

            if len(FW_REFRESH_CONFIG) == 0:
                msg = "firmware refresh config is empty"
                upgradeerror(msg)
                return ERR_FW_CONFIG_FOUND, msg

            # just for debug
            devtype = self.upg_platform.devtype
            for slot_name, slot_config in FW_REFRESH_CONFIG.items():
                for fw_name, fw_config in slot_config.items():
                    subtype = fw_config["subtype"]
                    filetype = fw_config["filetype"]
                    chain = fw_config["chain"]
                    upgradedebuglog("%s: support firmware_name: %s, devtype: 0x%x, subtype: 0x%x, filetype: %s, chain: %d" %
                        (slot_name, fw_name, devtype, subtype, filetype, chain))

            ret, msg = self.do_refresh_init_check(firmware_name, slot)
            if ret != CHECK_OK:
                return ret, msg

            # self.file_list is a collection of all check ok files
            if len(self.file_list) == 0:
                msg = "Can't find %s refresh config, please execute upgrade.py refresh -h to get help" % firmware_name
                upgradeerror(msg)
                return ERR_FW_NO_FILE_SUCCESS, msg
            self.do_refresh_list_sort()

            SUCCUSS_FILE_SUMMARY = "SUCCESS FILE: \n"
            # file refresh upgrade
            upgradedebuglog("start all files refresh upgrade")
            ret, log = self.do_refresh()
            if ret != FIRMWARE_SUCCESS:
                return ret, log

            msg = "all file refresh success"
            upgradedebuglog(msg)
            SUCCUSS_FILE_SUMMARY += log
            return FIRMWARE_SUCCESS, SUCCUSS_FILE_SUMMARY
        except Exception as e:
            msg = "do fw upgrade exception happend. log: %s" % str(e)
            upgradeerror(msg)
            return ERR_FW_UPGRADE, msg


    def fw_refresh(self, fw_name, slot):
        print("+================================+")
        print("|  Doing refresh, please wait... |")
        exec_os_cmd("touch %s" % FW_UPGRADE_STARTED_FLAG)
        exec_os_cmd("sync")
        ret, log = self.do_fw_refresh(fw_name, slot)
        exec_os_cmd("rm -rf %s" % FW_UPGRADE_STARTED_FLAG)
        exec_os_cmd("sync")
        log = log.rstrip("\n")
        if ret == FIRMWARE_SUCCESS:
            print("|       refresh succeeded!       |")
            print("+================================+")
            print(log)
            sys.exit(0)
        else:
            print("|        refresh failed!         |")
            print("+================================+")
            print("FAILED REASON:")
            print("%s" % log)
            sys.exit(1)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''upgrade script'''


def do_cold_upgrade(file_name, slot_num=None, chip_select=None, name=None, chain=None):
    fwupg = FwUpg()
    fwupg.fw_upg(file_name, slot_num, COLD_UPGRADE, chip_select, name, chain)


# cold upgrade
@main.command()
@click.argument('file_name', required=True)
@click.argument('slot_num', required=False, default=None)
@click.argument('chip_select', required=False, default=None)
@click.option('-n', '--name', default=None, help=FW_NAME_CLICK_HELP)
@click.option('-c', '--chain', default=None, type=int, help='Specify the chain of firmware')
@click.option('-u', '--uconf_skip', is_flag=True, help='Skip user configuration partition during upgrade, currently only used for BIOS upgrade')

def cold(file_name, slot_num, chip_select, name, chain, uconf_skip):
    '''cold upgrade'''
    #when the -u option is passed, the global variable COLD_UPGRADE_KEEP_USER_CONFIG will be set to True,
    #in the BIOS upgrade process, this variable will determine whether to skip the user configuration partition upgrade. Other upgrades that need to preserve a certain partition can reuse this parameter.
    global COLD_UPGRADE_KEEP_USER_CONFIG
    if uconf_skip:
        COLD_UPGRADE_KEEP_USER_CONFIG = True
    do_cold_upgrade(file_name, slot_num, chip_select, name, chain)


# erase by cold upgrade
# Example:
#   upgrade.py erase CPU_CPLD
#   upgrade.py erase CPU_CPLD -s 16m
@main.command()
@click.argument('name', required=True)
@click.option('-s', '--size', default='1m', show_default=True,
              help='Erase file size. Supports B/K/M/G suffix, such as 1048576, 1m, 16m.')
def erase(name, size):
    '''erase firmware by generating 0xFF image and reusing cold upgrade flow'''
    temp_file = None
    try:
        size_bytes = parse_erase_size(size)
    except Exception as e:
        print("invalid erase size: %s" % str(e))
        sys.exit(1)

    try:
        fd, temp_file = tempfile.mkstemp(prefix='erase_', suffix='.bin', dir='/tmp')
        os.close(fd)
        generate_ff_file(temp_file, size_bytes)
        upgradedebuglog("erase temp file generated: %s, size: %d bytes" % (temp_file, size_bytes))
        # Reuse the same code path as `cold <file> -n <name>` with default params.
        do_cold_upgrade(temp_file, name=name)
    except SystemExit:
        raise
    except Exception as e:
        print("erase upgrade failed, reason: %s" % str(e))
        sys.exit(1)
    finally:
        if temp_file and os.path.exists(temp_file):
            os.remove(temp_file)


# warm upgrade
@main.command()
@click.argument('file_name', required=True)
@click.argument('slot_num', required=False, default=None)
@click.argument('chip_select', required=False, default=None)
@click.option('-n', '--name', default=None, help=FW_NAME_CLICK_HELP)
@click.option('-c', '--chain', default=None, type=int, help='Specify the chain of firmware')
def warm(file_name, slot_num, chip_select, name, chain):
    '''warm upgrade'''
    fwupg = FwUpg()
    fwupg.fw_upg(file_name, slot_num, WARM_UPGRADE, chip_select, name, chain)


# test upgrade
@main.command()
@click.argument('device', required=True)
@click.argument('slot_num', required=False, default=None)
def test(device, slot_num):
    '''upgrade test'''
    platform = BasePlatform()
    platform.do_test_main(device, slot_num)


# BMC upgrade
@main.command()
@click.argument('file_name', required=True)
@click.argument('chip_select', required=False, default="2")
@click.argument('chanel', required=False, default=None)
@click.argument('erase_type', required=False, default="full")
def bmc(file_name, chip_select, chanel, erase_type):
    '''BMC upgrade'''
    platform = BasePlatform()
    platform.do_bmc_upgrade_main(file_name, chip_select, chanel, erase_type)


# BIOS upgrade
@main.command()
@click.argument('file_name', required=True)
def bios(file_name):
    '''BIOS upgrade'''
    platform = BasePlatform()
    platform.do_bios_upgrade_main_by_rfu(file_name)


# current BIOS/BMC/... sync to another
@main.command()
@click.argument('type', required=True)
def sync_fw(type):
    '''fw sync upgrade'''
    platform = BasePlatform()
    platform.do_fw_sync(type)


# ONIE upgrade
@main.command()
@click.argument('file_name', required=True)
def onie(file_name):
    '''ONIE upgrade'''
    platform = BasePlatform()
    platform.do_onie_upgrade_main(file_name)


# remove_header
@main.command()
@click.argument('file_name', required=True)
@click.argument('out_file_name', required=False, default=None)
def remove_header(file_name, out_file_name):
    '''remove_header'''
    platform = BasePlatform()
    ret, msg = do_fw_upg_raw_file_generate(file_name, platform.head_info_config, out_file_name)
    if ret is False:
        print("remove file header fail, reason: %s" % msg)
    else:
        print("remove file header success, output file: %s" % msg)


# refresh
@main.command()
@click.option('-n', '--name', required=True, help=FW_REFRESH_NAME_CLICK_HELP)
@click.option('-s', '--slot', default=None, type=int, help='Specify the slot of firmware, if not specified, traverse all slots')
def refresh(name, slot):
    '''refresh firmware'''
    fwupg = FwUpg()
    fwupg.fw_refresh(name, slot)

# PSU upgrade
@main.command()
@click.argument('file_name', required=True)
@click.argument('slot', required=False, type=int, default=None)
@click.argument('psu_model', required=False, default=None)
@click.argument('force', required=False, type=int, default=None)
def psu(file_name, slot, psu_model, force):
    '''PSU upgrade'''
    psu_upg = PsuUpgrade()
    psu_upg.do_psu_upgrade_main(file_name, slot, psu_model, force)

if __name__ == '__main__':
    signal_init()
    debug_init()
    main()
