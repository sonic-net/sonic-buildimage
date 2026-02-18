#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import sys
import os
import time
import syslog
import signal
import click
import traceback
import logging
from platform_util import get_value, set_value, exec_os_cmd, exec_os_cmd_log, write_sysfs, check_value, setup_logger, BSP_COMMON_LOG_DIR
from platform_config import UPGRADE_SUMMARY, WARM_UPGRADE_STARTED_FLAG, FW_UPGRADE_STARTED_FLAG
from warm_upgrade import WarmBasePlatform
from wbutil.baseutil import get_board_id
from time import monotonic as _time
import shutil
from public.platform_common_config import UPGRADE_BY_FIRMWARE_UPGRADE_COMMON, UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER, UPGRADE_BY_AFU, UPGRADE_BY_CUSTOM, SUPPORT_UPGRADE_LIST


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
MAX_HEADER_SIZE = 1000

FIlE_WITH_HEADER = 1
FIlE_WITHOUT_HEADER = 2

UPGRADE_FILE_DIR = "/tmp/firmware/"
DEBUG_FILE = "/etc/.upgrade_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "upgrade_debug.log"
logger = setup_logger(LOG_FILE)

COLD_UPGRADE = 1
WARM_UPGRADE = 2
TEST_UPGRADE = 3
BMC_UPGRADE = 4

CHANEL_LPC = "lpc"
CHANEL_USB = "usb"
BMC_UPGRADE_SUPPORT_CHANEL_LIST = [CHANEL_LPC, CHANEL_USB]

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

SUPPORT_FILE_TYPE = ("VME", "SYSFS", "SPI-LOGIC-DEV", "MTD", "ISC", "JBI", "VME-I2C")

FW_NAME_CLICK_HELP = "Specify the name of firmware.\n"
AFU_UPGRADE_CMD = "/usr/local/bin/afulnx_64 %s /P /B /N /L /K /RLC:E"

def is_upgrade_debug_mode():
    return os.path.exists(DEBUG_FILE)

def help_get_firmware_name():
    fw_name = ""
    try:
        max_slot_num = UPGRADE_SUMMARY.get("max_slot_num", 0)
        for slot in range(0, max_slot_num + 1):
            slot_fw_name = ""
            tmp_name = ""
            slot_config =  UPGRADE_SUMMARY.get("slot%d" % slot, {})
            for filetype in SUPPORT_FILE_TYPE:
                file_type_conf = slot_config.get(filetype, {})
                for chain, chain_conf in file_type_conf.items():
                    if isinstance(chain_conf, dict):
                        name = chain_conf.get('name')
                        if isinstance(name, str):
                            tmp_name += name + ", "
                    elif isinstance(chain_conf, list):
                        for chain_conf_item in chain_conf:
                            name = chain_conf_item.get('name')
                            if isinstance(name, str):
                                tmp_name += name + ", "
            if len(tmp_name) != 0:
                slot_fw_name = ("slot%d: %s" % (slot, tmp_name)).rstrip(", ") + "\n"
                fw_name += slot_fw_name

    except Exception as e:
        fw_name = ""
    return fw_name


FW_NAME_CLICK_HELP += help_get_firmware_name()


CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}


class AliasedGroup(click.Group):

    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        if len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
        return None


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


class BasePlatform():

    def __init__(self):
        self.upgrade_param = UPGRADE_SUMMARY.copy()
        self.devtype = self.upgrade_param.get('devtype', None)
        self.max_slot_num = self.upgrade_param.get("max_slot_num", 0)
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
        command = "warm_upgrade.py %s 0x%x 0x%x %s %s %s" % (file, main_type, sub_type, slot, filetype, chain)
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

    def do_fw_upg_cmd_once(self, dev_name, cmd_config):
        support_operation_list = ["set_value", "check_value", "backup_value"]
        operation = cmd_config.get("operation", "set_value")
        if operation not in support_operation_list:
            log = "%s do cmd: %s failed, invalid operation type: %s" % (dev_name, cmd_config, operation)
            upgradeerror(log)
            return False, log

        # set value
        if operation == "set_value":
            ret, log = set_value(cmd_config)
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

    def do_fw_upg_raw_file_generate(self, head_file, raw_file = None):
        try:
            if raw_file is None:
                dir_name = os.path.dirname(head_file)
                file_name = os.path.basename(head_file)
                raw_file = os.path.join(dir_name, f"raw_{file_name}")

            with open(head_file, 'rb') as fd:
                rdbuf = fd.read()

            # Locate header position (same logic as original code; note newline in binary data is b'\n')
            # Note: If "FILEHEADER(\n" in original file is a text string, need to convert to byte string for matching
            file_head_end = rdbuf.index(b')\n') + 2

            # Strip header and retain remaining content (binary data)
            remaining_content = rdbuf[file_head_end:].lstrip()

            with open(raw_file, 'wb') as fd:
                fd.write(remaining_content)

            upgradedebuglog(f"Successfully stripped head from head file: {head_file}")
            return True, raw_file

        except Exception as e:
            msg = f"Failed to backup and strip head: {str(e)}"
            upgradeerror(msg)
            return False, msg

    def upgrade_cmd_generate(self, config, file, main_type, sub_type, slot, filetype, chain):
        upgrade_way = config.get("upgrade_way", UPGRADE_BY_FIRMWARE_UPGRADE_COMMON)
        raw_file = None

        if upgrade_way not in SUPPORT_UPGRADE_LIST:
            return False, "unsupport upgrade way: %s" % upgrade_way, None

        if upgrade_way == UPGRADE_BY_FIRMWARE_UPGRADE_COMMON:
            command = "firmware_upgrade %s 0x%x 0x%x %s %s 0x%x" % (file, main_type, sub_type, slot, filetype, chain)
            return True, command, None
        elif upgrade_way == UPGRADE_BY_FIRMWARE_UPGRADE_UPDATE_HEADER:
            new_file_type = config.get("new_file_type")
            new_chain = config.get("new_chain")
            if new_file_type is None or new_chain is None:
                return False, "upgrade way: %s new_file_type or new_chain not define, new_file_type: %s, new_chain: %s" % (upgrade_way, new_file_type, new_chain), None
            ret, raw_file = self.do_fw_upg_raw_file_generate(file)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = "firmware_upgrade %s 0x%x 0x%x %s %s 0x%x" % (raw_file, main_type, sub_type, slot, new_file_type, new_chain)
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_AFU:
            ret, raw_file = self.do_fw_upg_raw_file_generate(file)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = AFU_UPGRADE_CMD % raw_file
            return True, command, raw_file
        elif upgrade_way == UPGRADE_BY_CUSTOM:
            upgrade_cmd = config.get("upgrade_cmd")
            if upgrade_cmd is None:
                return False, "upgrade way: %s upgrade_cmd not define, upgrade_cmd: %s" % (upgrade_way, upgrade_cmd), None
            ret, raw_file = self.do_fw_upg_raw_file_generate(file)
            if ret is False:
                return False, "upgrade way: %s generate raw file failed, reason: %s" % (upgrade_way, raw_file), None
            command = upgrade_cmd % raw_file
            return True, command, raw_file
        else:
            return False, "unsupport upgrade way: %s" % upgrade_way, None

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
            if raw_file:
                if os.path.exists(raw_file):
                    os.remove(raw_file)
            ret, log = self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            if ret is False:
                return False, log
            return True, "upgrade success"
        except Exception as e:
            self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            return False, str(e)

    def subprocess_test_upgrade(self, config, file, main_type, sub_type, slot, filetype, chain):
        dev_name = config.get("name", None)
        init_cmd_list = config.get("init_cmd", [])
        finish_cmd_list = config.get("finish_cmd", [])
        try:
            ret, log = self.do_fw_upg_init_cmd(dev_name, init_cmd_list)
            self.do_fw_upg_finish_cmd_update(dev_name, init_cmd_list, finish_cmd_list)
            if ret is False:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, log
            time.sleep(0.5)  # delay 0.5s after execute init_cmd
            command = "firmware_upgrade test %s 0x%x 0x%x %s %s 0x%x" % (file, main_type, sub_type, slot, filetype, chain)
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

    def parse_file_head(self, file):
        try:
            self.head_info_config = {}
            with open(file, 'r', errors='ignore') as fd:
                rdbuf = fd.read(MAX_HEADER_SIZE)
            upgradedebuglog("start parse upgrade file head")
            file_head_start = rdbuf.index('FILEHEADER(\n')  # ponit to F
            file_head_start += rdbuf[file_head_start:].index('\n')  # ponit to \n
            file_head_end = rdbuf.index(')\n')
            header_buf = rdbuf[file_head_start + 1: file_head_end - 1]
            upgradedebuglog("upgrade file head find FILEHEADER")
            for line in header_buf.split('\n'):
                head_list = line.split('=', 1)
                head_key = head_list[0]
                head_val = head_list[1]
                self.head_info_config[head_key] = head_val
            upgradedebuglog("file: %s head_info_config: %s" % (file, self.head_info_config))
            return CHECK_OK, "SUCCESS"
        except Exception as e:
            msg = "parse %s head failed, msg: %s" % (file, str(e))
            upgradeerror(msg)
            return ERR_FW_RAISE_EXCEPTION, msg

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
                msg = "upgrade fool proofing failed, device model: %s, upgrade file version: %s" % (
                    dev_model, self.VERSION)
                upgradedebuglog(msg)
                return False, msg
            msg = "upgrade fool proofing pass, device model: %s, upgrade file version: %s" % (dev_model, self.VERSION)
            upgradedebuglog(msg)
            return True, msg
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
        else:
            log = "%s set error option flag" % dev_name
            upgradeerror(log)
            return False, log

        if status is False:
            upgradeerror("%s upgrade failed" % dev_name)
            return False, output
        upgradedebuglog("%s upgrade success" % dev_name)
        return True, "upgrade success"

    def initial_check_chain(self, file, upg_type, chain_config):
        fool_proofing = chain_config.get("fool_proofing")
        if fool_proofing is not None:
            upgradedebuglog("do fool proofing check...")
            status, log = self.upgrade_fool_proofing(fool_proofing)
            if status is False:
                msg = "upgrade fool proofing check failed, msg: %s" % log
                upgradedebuglog(msg)
                return ERR_FW_FOOL_PROOF, msg
            upgradedebuglog("do fool proofing check ok")

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

    def initial_check(self, file, slot, upg_type, chip_select, file_list, firmware_name, specify_chain):
        try:
            upgradedebuglog("BasePlatform initial_check, file: %s, slot: %s, upg_type: %s, chip_select: %s, firmware_name: %s, specify_chain: %s" %
                            (file, slot, upg_type, chip_select, firmware_name, specify_chain))

            upgradedebuglog("do file exist check...")
            if not os.path.isfile(file):
                msg = "%s not found" % file
                upgradedebuglog(msg)
                return ERR_FW_FILE_FOUND, msg
            upgradedebuglog("file exist check ok")

            slot_name = "slot%d" % slot
            slot_config = self.upgrade_param.get(slot_name, {})
            slot_present_config = slot_config.get("present", {})
            if len(slot_present_config) != 0:
                upgradedebuglog("do %s present check..." % slot_name)
                ret, log = self.linecard_present_check(slot_present_config)
                if ret != CHECK_OK:
                    msg = "check %s present error, msg: %s" % (slot_name, log)
                    upgradedebuglog(msg)
                    return ret, msg
                upgradedebuglog("%s present check ok" % slot_name)

            if firmware_name is None:
                upgradedebuglog("do file head parse...")
                self.subtype = slot_config.get("subtype", 0)
                ret, log = self.parse_file_head(file)
                if ret != CHECK_OK:
                    return ret, log
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

    def do_test(self, device, slot):
        try:
            # slot present check
            slot_name = "slot%d" % slot
            slot_config = self.upgrade_param.get(slot_name, {})
            slot_present_config = slot_config.get("present", {})
            if len(slot_present_config) != 0:
                ret, log = self.linecard_present_check(slot_present_config)
                if ret != CHECK_OK:
                    msg = "check %s present error, msg: %s" % (slot_name, log)
                    upgradedebuglog(msg)
                    return ret, msg
                upgradedebuglog("%s present" % slot_name)

            # get list of devices to be tested
            test_config = slot_config.get("TEST", {})
            if len(test_config) == 0:
                return ERR_FW_CONFIG_FOUND, "test config no found"
            device_list = test_config.get(device, [])
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
                            upgradeerror("chain:%d, name:%s test failed" % (chain, display_name))
                            failed_summary += "\n    chain:%d, name:%s;" % (chain, display_name)
                    else:
                        upgradeerror("name:%s test failed" % (display_name))
                        failed_summary += "\n    name:%s;" % (display_name)
                else:
                    for chain in success_chain:
                        upgradedebuglog("chain:%d, name:%s test success" % (chain, display_name))
                        success_summary += "\n    chain:%d, name:%s;" % (chain, display_name)
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

# single file upgrade operation
class FileUpg(object):
    def __init__(self, config, file, devtype, subtype, slot, filetype, chain, upg_type, chain_index=None, chip_select=None):
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

    def do_file_refresh(self, fw_upg_instance):
        fw_upg_config = fw_upg_instance.config
        fw_upg_file = fw_upg_instance.file
        fw_upg_devtype = fw_upg_instance.devtype
        fw_upg_subype = fw_upg_instance.subtype
        fw_upg_slot = fw_upg_instance.slot
        fw_upg_filetype = fw_upg_instance.filetype
        fw_upg_chain = fw_upg_instance.chain
        dev_name = fw_upg_config.get("name", None)
        upgradedebuglog("%s start warm upgrade, file: %s, devtype:0x%x, subype: 0x%x, slot: %d, filetype: %s, chain: %d" %
                        (dev_name, fw_upg_file, fw_upg_devtype, fw_upg_subype, fw_upg_slot, fw_upg_filetype, fw_upg_chain))
        status, output = self.warm_upg_platform.do_warmupgrade(fw_upg_file, fw_upg_devtype, fw_upg_subype, fw_upg_slot,
                                                               fw_upg_filetype, fw_upg_chain)
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

            for file_instance in self.file_list:
                file_info = repr(file_instance)
                ret, log = self.do_file_refresh(file_instance)
                if ret is False:
                    msg = "%s refresh failed, ret:%s, \n log:%s." % (file_info, ret, log)
                    upgradeerror(msg)
                    return ERR_FW_UPGRADE, msg
                upgradedebuglog("%s refresh success." % file_info)
            msg = "all files refresh success."
            return FIRMWARE_SUCCESS, msg
        except Exception as e:
            msg = "do warm upg exception happend. log:%s" % str(e)
            upgradeerror(msg)
            return ERR_FW_UPGRADE, msg
        finally:
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


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''upgrade script'''


# cold upgrade
@main.command()
@click.argument('file_name', required=True)
@click.argument('slot_num', required=False, default=None)
@click.argument('chip_select', required=False, default=None)
@click.option('-n', '--name', default=None, help=FW_NAME_CLICK_HELP)
@click.option('-c', '--chain', default=None, type=int, help='Specify the chain of firmware')
def cold(file_name, slot_num, chip_select, name, chain):
    '''cold upgrade'''
    fwupg = FwUpg()
    fwupg.fw_upg(file_name, slot_num, COLD_UPGRADE, chip_select, name, chain)


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
@click.argument('slot_num', required=True)
def test(device, slot_num):
    '''upgrade test'''
    platform = BasePlatform()
    platform.do_test_main(device, int(slot_num))


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

# remove_header
@main.command()
@click.argument('file_name', required=True)
@click.argument('out_file_name', required=False, default=None)
def remove_header(file_name, out_file_name):
    '''remove_header'''
    platform = BasePlatform()
    ret, msg = platform.do_fw_upg_raw_file_generate(file_name, out_file_name)
    if ret is False:
        print("remove file header fail, reason: %s" % msg)
    else:
        print("remove file header success, output file: %s" % msg)


if __name__ == '__main__':
    signal_init()
    debug_init()
    main()
