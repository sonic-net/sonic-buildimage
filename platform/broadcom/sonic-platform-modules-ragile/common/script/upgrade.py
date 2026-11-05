#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import sys
import os
import time
import syslog
import signal
import click
from platform_util import get_value, set_value, exec_os_cmd, exec_os_cmd_log
from platform_config import UPGRADE_SUMMARY, WARM_UPGRADE_STARTED_FLAG
from warm_upgrade import WarmBasePlatform


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

FIRMWARE_SUCCESS = 0
CHECK_OK = 0


UPGRADE_DEBUG_FILE = "/etc/.upgrade_debug_flag"
UPGRADE_FILE_DIR = "/tmp/firmware/"

UPGRADEDEBUG = 1

debuglevel = 0

COLD_UPGRADE = 1
WARM_UPGRADE = 2
TEST_UPGRADE = 3
BMC_UPGRADE = 4

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
    global debuglevel
    if os.path.exists(UPGRADE_DEBUG_FILE):
        debuglevel = debuglevel | UPGRADEDEBUG
    else:
        debuglevel = debuglevel & ~(UPGRADEDEBUG)


def upgradewarninglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("UPGRADE", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, s)


def upgradecriticallog(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("UPGRADE", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT, s)


def upgradeerror(s):
    # s = s.decode('utf-8').encode('gb2312')
    syslog.openlog("UPGRADE", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)


def upgradedebuglog(s):
    # s = s.decode('utf-8').encode('gb2312')
    if UPGRADEDEBUG & debuglevel:
        syslog.openlog("UPGRADE", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


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
        self.cold_chain_config = {}
        self.subtype = None
        self.chain = None
        self.filetype = None
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

    def subprocess_warm_upgrade(self, config, file, main_type, sub_type, slot):
        dev_name = config.get("name", None)
        status, output = self.subprocess_firmware_upgrade(config, file, main_type, sub_type, slot)
        if status is False:
            upgradeerror("%s warm upgrade failed" % dev_name)
            return False, output
        command = "warm_upgrade.py %s 0x%x 0x%x %s %s %s" % (file, main_type, sub_type, slot, self.filetype, self.chain)
        upgradedebuglog("warm upgrade cmd: %s" % command)
        if os.path.exists(UPGRADE_DEBUG_FILE):
            status, output = exec_os_cmd_log(command)
        else:
            status, output = exec_os_cmd(command)
        if status:
            upgradeerror("%s warm upgrade failed" % dev_name)
            return False, output
        upgradedebuglog("%s warm upgrade success" % dev_name)
        return True, "upgrade success"

    def do_fw_upg_init_cmd(self, dev_name, init_cmd_list):
        # pre operation
        try:
            for init_cmd_config in init_cmd_list:
                ret, log = set_value(init_cmd_config)
                if ret is False:
                    upgradeerror("%s do init cmd: %s failed, msg: %s" % (dev_name, init_cmd_config, log))
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
            ret_t, log = set_value(finish_cmd_config)
            if ret_t is False:
                upgradeerror("%s do finish cmd: %s failed, msg: %s" % (dev_name, finish_cmd_config, log))
                ret = -1
        if ret != 0:
            msg = "%s firmware finish cmd exec failed" % dev_name
            upgradeerror(msg)
            return False, msg
        msg = "%s firmware finish cmd all set success" % dev_name
        upgradedebuglog(msg)
        return True, msg

    def subprocess_firmware_upgrade(self, config, file, main_type, sub_type, slot):
        dev_name = config.get("name", None)
        init_cmd_list = config.get("init_cmd", [])
        finish_cmd_list = config.get("finish_cmd", [])
        try:
            ret, log = self.do_fw_upg_init_cmd(dev_name, init_cmd_list)
            if ret is False:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, log
            time.sleep(0.5)  # delay 0.5s after execute init_cmd
            command = "firmware_upgrade %s 0x%x 0x%x %s" % (file, main_type, sub_type, slot)
            upgradedebuglog("firmware upgrade cmd: %s" % command)
            if os.path.exists(UPGRADE_DEBUG_FILE):
                status, output = exec_os_cmd_log(command)
            else:
                status, output = exec_os_cmd(command)
            if status:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                upgradeerror("%s firmware upgrade failed, msg: %s" % (dev_name, output))
                return False, output
            upgradedebuglog("%s firmware upgrade success" % dev_name)
            ret, log = self.do_fw_upg_finish_cmd(dev_name, init_cmd_list)
            if ret is False:
                return False, log
            return True, "upgrade success"
        except Exception as e:
            self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            return False, str(e)

    def subprocess_test_upgrade(self, config, file, main_type, sub_type, slot):
        dev_name = config.get("name", None)
        init_cmd_list = config.get("init_cmd", [])
        finish_cmd_list = config.get("finish_cmd", [])
        try:
            ret, log = self.do_fw_upg_init_cmd(dev_name, init_cmd_list)
            if ret is False:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, log
            time.sleep(0.5)  # delay 0.5s after execute init_cmd
            command = "firmware_upgrade test %s 0x%x 0x%x %s" % (file, main_type, sub_type, slot)
            upgradedebuglog("firmware upgrade cmd: %s" % command)
            if os.path.exists(UPGRADE_DEBUG_FILE):
                status, output = exec_os_cmd_log(command)
            else:
                status, output = exec_os_cmd(command)
            if status:
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                upgradeerror("%s test upgrade failed, msg: %s" % (dev_name, output))
                return False, output
            upgradedebuglog("%s test upgrade success" % dev_name)
            ret, log = self.do_fw_upg_finish_cmd(dev_name, init_cmd_list)
            if ret is False:
                return False, log
            return True, "upgrade success"
        except Exception as e:
            self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            return False, str(e)

    def subprocess_bmc_upgrade(self, config, file, chip_select, erase_type):
        dev_name = config.get("name", None)
        init_cmd_list = config.get("init_cmd", [])
        finish_cmd_list = config.get("finish_cmd", [])
        save_set_reg_list = config.get("save_set_reg", [])
        try:
            # save and set reg
            ret, log = self.save_and_set_value(save_set_reg_list)
            if ret is False:
                upgradeerror(log)
                self.recover_save_value(save_set_reg_list)
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, log
            upgradedebuglog("%s save and set cmd all set success" % dev_name)
            time.sleep(0.5)  # delay 0.5s after execute save and set reg

            # pre operation
            ret, log = self.do_fw_upg_init_cmd(dev_name, init_cmd_list)
            if ret is False:
                self.recover_save_value(save_set_reg_list)
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, log

            upgradedebuglog("%s bmc init cmd all set success" % dev_name)
            time.sleep(0.5)  # delay 0.5s after execute init_cmd

            command = "fw_upgrade upgrade %s %s %s" % (file, chip_select, erase_type)
            upgradedebuglog("fw_upgrade upgrade cmd: %s" % command)
            status, output = exec_os_cmd_log(command)
            if status:
                upgradeerror("%s bmc upgrade failed" % dev_name)
                self.recover_save_value(save_set_reg_list)
                self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
                return False, output
            upgradedebuglog("%s bmc upgrade success" % dev_name)

            ret1, log1 = self.recover_save_value(save_set_reg_list)
            if ret1 is False:
                upgradeerror("bmc upgrade recover save value failed, msg: %s" % log1)
            ret2, log2 = self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            if ret2 is False:
                upgradeerror("bmc upgrade do finish command failed, msg: %s" % log2)
            if ret1 is False or ret2 is False:
                return False, "bmc upgrade do recover save value or finish command failed"
            return True, "upgrade success"

        except Exception as e:
            self.recover_save_value(save_set_reg_list)
            self.do_fw_upg_finish_cmd(dev_name, finish_cmd_list)
            return False, str(e)

    def file_head_param_check(self, head_info_config):
        try:
            self.DEVTYPE = head_info_config.get('DEVTYPE', None)
            self.SUBTYPE = head_info_config.get('SUBTYPE', '0')
            self.TYPE = head_info_config.get('TYPE', None)
            self.CHAIN = head_info_config.get('CHAIN', None)
            self.CHIPNAME = head_info_config.get('CHIPNAME', None)
            self.VERSION = head_info_config.get('VERSION', None)
            self.FILETYPE = head_info_config.get('FILETYPE', None)
            self.CRC = head_info_config.get('CRC', None)

            if self.devtype != int(self.DEVTYPE, 16):
                return ERR_FW_HEAD_CHECK, ("no support %s devtype" % self.DEVTYPE)

            if self.SUBTYPE is not None:
                self.SUBTYPE_LIST = self.SUBTYPE.split(',')
                self.SUBTYPE_LIST = [int(tmp_subtype, base=16) for tmp_subtype in self.SUBTYPE_LIST]
                if len(self.SUBTYPE) != 0 and self.subtype not in self.SUBTYPE_LIST:
                    return ERR_FW_HEAD_CHECK, ("no support %s SUBTYPE" % self.SUBTYPE)

            if len(self.CHAIN) == 0 or len(self.FILETYPE) == 0:
                return ERR_FW_HEAD_CHECK, ("CHAIN:%s, FILETYPE:%s get failed" % (self.CHAIN, self.FILETYPE))
            self.chain = int(self.CHAIN)
            self.filetype = self.FILETYPE
            upgradedebuglog("file head param: devtype:0x%x, subtype:0x%x, chain:%s, filetype:%s"
                            % (self.devtype, self.subtype, self.chain, self.filetype))
            return CHECK_OK, "SUCCESS"
        except Exception as e:
            return ERR_FW_RAISE_EXCEPTION, str(e)

    def parse_file_head(self, file):
        try:
            self.head_info_config = {}
            with open(file, 'r', errors='ignore') as fd:
                rdbuf = fd.read()
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

    def upgrading(self, config, file, devtype, subtype, slot, option_flag, erase_type=None):
        dev_name = config.get("name", None)
        if option_flag == COLD_UPGRADE:
            status, output = self.subprocess_firmware_upgrade(config, file, devtype, subtype, slot)
        elif option_flag == WARM_UPGRADE:
            status, output = self.subprocess_warm_upgrade(config, file, devtype, subtype, slot)
        elif option_flag == TEST_UPGRADE:
            status, output = self.subprocess_test_upgrade(config, file, devtype, subtype, slot)
        elif option_flag == BMC_UPGRADE:
            status, output = self.subprocess_bmc_upgrade(config, file, slot, erase_type)
        else:
            log = "%s set error option flag" % dev_name
            upgradeerror(log)
            return False, log

        if status is False:
            upgradeerror("%s upgrade failed" % dev_name)
            return False, output
        upgradedebuglog("%s upgrade success" % dev_name)
        return True, "upgrade success"

    def initial_check(self, file, slot, upg_type):
        try:
            upgradedebuglog("BasePlatform initial_check, file: %s, slot: %s, upg_type: %s" %
                            (file, slot, upg_type))

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

            upgradedebuglog("do file head parse...")
            self.subtype = slot_config.get("subtype", 0)
            ret, log = self.parse_file_head(file)
            if ret != CHECK_OK:
                return ret, log
            upgradedebuglog("file head parse success")

            upgradedebuglog("do file head check...")
            ret, log = self.file_head_param_check(self.head_info_config)
            if ret != CHECK_OK:
                msg = "file: %s, head check failed, msg: %s" % (file, log)
                upgradedebuglog(msg)
                return ret, msg
            upgradedebuglog("file head check ok")

            upgradedebuglog("get upgrade chain config...")
            filetype_config = slot_config.get(self.filetype, {})
            if len(filetype_config) == 0:
                msg = "file: %s filetype: %s no support" % (file, self.filetype)
                upgradedebuglog(msg)
                return ERR_FW_CONFIG_FOUND, msg
            chain_num = "chain%s" % self.chain
            chain_config = filetype_config.get(chain_num, {})
            if len(chain_config) == 0:
                msg = "file: %s get %s config failed" % (file, chain_num)
                upgradedebuglog(msg)
                return ERR_FW_CONFIG_FOUND, msg
            self.cold_chain_config = chain_config
            upgradedebuglog("get %s filetype: %s %s config success" % (slot_name, self.filetype, chain_num))

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
                    msg = "file: %s %s chain config not support warm upgrade" % (file, slot_name)
                    upgradedebuglog(msg)
                    return ERR_FW_NOSUPPORT_HOT, msg
                upgradedebuglog("file: %s %s chain config support warm upgrade" % (file, slot_name))

            filesizecheck = chain_config.get("filesizecheck", 0)
            if filesizecheck != 0:
                upgradedebuglog("do file size check...")
                file_size = self.get_file_size_k(file)
                if file_size > filesizecheck:
                    msg = "file: %s size: %s exceed %s" % (file, file_size, filesizecheck)
                    upgradedebuglog(msg)
                    return ERR_FW_CHECK_SIZE, msg
                msg = "file: %s size: %s check ok" % (file, file_size)
                upgradedebuglog(msg)

            msg = "file: %s slot: %s upgrade type: %s check ok" % (file, slot, upg_type)
            upgradedebuglog(msg)
            return CHECK_OK, msg
        except Exception as e:
            return ERR_FW_RAISE_EXCEPTION, str(e)

    def do_upgrade(self, file, slot, upg_type):
        try:
            ret, log = self.initial_check(file, slot, upg_type)
            if ret != CHECK_OK:
                return ret, log

            # start upgrading
            upgradedebuglog("start upgrading")
            ret, log = self.upgrading(self.cold_chain_config, file, self.devtype, self.subtype, slot, upg_type)
            if ret is False:
                upgradeerror("upgrade failed")
                return ERR_FW_UPGRADE, log
            upgradedebuglog("upgrade success")
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
                chain_num = test_config.get("chain", None)
                test_file = test_config.get("file", None)
                display_name = test_config.get("display_name", None)
                if chain_num is None or test_file is None or display_name is None:
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
                chain_num = test_config.get("chain", None)
                test_file = test_config.get("file", None)
                display_name = test_config.get("display_name", None)
                pre_check_conf = test_config.get("pre_check", None)
                if pre_check_conf is not None:
                    status, msg = self.do_pre_check(pre_check_conf)
                    if status is False:
                        pre_check_failed += 1
                        log = "\nchain:%d, name:%s, pre check failed, msg: %s" % (chain_num, display_name, msg)
                        upgradedebuglog(log)
                        pre_check_failed_summary += log
                        continue
                    upgradedebuglog("chain:%d, name:%s, pre check ok, msg: %s" % (chain_num, display_name, msg))
                ret, log = self.do_upgrade(test_file, slot, TEST_UPGRADE)
                if ret != FIRMWARE_SUCCESS:
                    RET = -1
                    upgradeerror("chain:%d, name:%s test failed" % (chain_num, display_name))
                    failed_summary += "\n    chain:%d, name:%s;" % (chain_num, display_name)
                else:
                    upgradedebuglog("chain:%d, name:%s test success" % (chain_num, display_name))
                    success_summary += "\n    chain:%d, name:%s;" % (chain_num, display_name)
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
        ret, log = self.do_test(device, slot)
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

    def do_bmc_upgrade_main(self, file, chip_select, erase_type):
        bmc_upgrade_config = self.upgrade_param.get("BMC", {})
        ret, log = self.upgrading(bmc_upgrade_config, file, self.devtype,
                                  self.subtype, chip_select, BMC_UPGRADE, erase_type)
        if ret is True:
            print("===========upgrade succeeded!============")
            sys.exit(0)
        else:
            print("============upgrade failed!==============")
            print("FAILED REASON:")
            print("%s" % log)
            sys.exit(1)


class FileUpg(object):
    def __init__(self, config, file, devtype, subtype, slot, filetype, chain, upg_type):
        self.config = config
        self.file = file
        self.devtype = devtype
        self.subtype = subtype
        self.slot = slot
        self.filetype = filetype
        self.chain = chain
        self.upg_type = upg_type

    def __repr__(self):
        return "file:%s slot:%d" % (self.file, self.slot)


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
            ret, log = self.upg_platform.upgrading(
                fw_upg_config, fw_upg_file, fw_upg_devtype, fw_upg_subype, fw_upg_slot, COLD_UPGRADE)
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

    def do_file_init_check(self, file_path, slot, upg_type):
        upgradedebuglog("do_file_init_check, file_path: %s, slot: %s, upg_type: %s" % (file_path, slot, upg_type))

        if slot is None:  # traverse all slots
            for i in range(0, self.max_slot_num + 1):
                ret, log = self.upg_platform.initial_check(file_path, i, upg_type)
                if ret != CHECK_OK:
                    upgradedebuglog(
                        "file: %s, slot%d initial check not ok, ret: %d, msg: %s" %
                        (file_path, i, ret, log))
                    accept_error = (ERR_FW_CARD_ABSENT, ERR_FW_HEAD_CHECK, ERR_FW_FOOL_PROOF)
                    if ret in accept_error:
                        msg = "file: %s, slot%d initial check ret: %d, acceptable error." % (file_path, i, ret)
                        upgradedebuglog(msg)
                        continue
                    return ret, log
                file_instance = FileUpg(self.upg_platform.cold_chain_config, file_path, self.upg_platform.devtype,
                                        self.upg_platform.subtype, i, self.upg_platform.filetype, self.upg_platform.chain, upg_type)
                self.file_list.append(file_instance)
        else:
            slot = int(slot, 10)
            ret, log = self.upg_platform.initial_check(file_path, slot, upg_type)
            if ret != CHECK_OK:
                msg = "file: %s, slot%d initial check not ok, ret: %d,  msg: %s" % (file_path, slot, ret, log)
                return ret, msg
            file_instance = FileUpg(self.upg_platform.cold_chain_config, file_path, self.upg_platform.devtype,
                                    self.upg_platform.subtype, slot, self.upg_platform.filetype, self.upg_platform.chain, upg_type)
            self.file_list.append(file_instance)
        msg = "file: %s all slots init check ok" % file_path
        return CHECK_OK, msg

    def do_dir_init_check(self, path, slot, upg_type):
        for root, dirs, names in os.walk(path):
            # root: directory absolute path
            # dirs: folder path collection under directory
            # names: file path collection under directory
            for filename in names:
                # file_path is file absolute path
                file_path = os.path.join(root, filename)
                ret, log = self.do_file_init_check(file_path, slot, upg_type)
                if ret != CHECK_OK:
                    return ret, log
        msg = "all files in dir have been check ok"
        upgradedebuglog(msg)
        return CHECK_OK, msg

    def do_fw_upg(self, path, slot, upg_type):
        match_zip_file_flag = False
        try:
            upgradedebuglog("do_fw_upg, path: %s, slot: %s, upg_type: %s" % (path, slot, upg_type))
            if slot is not None and not slot.isdigit():
                msg = "invalid slot param: %s" % slot
                upgradeerror(msg)
                return ERR_FW_INVALID_PARAM, msg

            upgradedebuglog("start init check")
            if os.path.isfile(path) and path.endswith(".zip"):
                upgradedebuglog("firmware upgrade via compressed package: %s" % path)
                # remove origin firmware upgrade file
                exec_os_cmd("rm -rf %s" % UPGRADE_FILE_DIR)
                cmd = "unzip -o %s -d /tmp/" % path
                if os.path.exists(UPGRADE_DEBUG_FILE):
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
                ret, msg = self.do_dir_init_check(path, slot, upg_type)
            elif os.path.isfile(path):
                ret, msg = self.do_file_init_check(path, slot, upg_type)
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
            upgradedebuglog("start all files cold upgrade")
            for file_instance in self.file_list:
                file_info = repr(file_instance)
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
            msg = "do dir upgrade exception happend. log: %s" % str(e)
            upgradeerror(msg)
            return ERR_FW_UPGRADE, msg
        finally:
            if match_zip_file_flag is True:
                exec_os_cmd("rm -rf %s" % UPGRADE_FILE_DIR)

    def fw_upg(self, path, slot, upg_type):
        print("+================================+")
        print("|  Doing upgrade, please wait... |")
        ret, log = self.do_fw_upg(path, slot, upg_type)
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
def cold(file_name, slot_num):
    '''cold upgrade'''
    fwupg = FwUpg()
    fwupg.fw_upg(file_name, slot_num, COLD_UPGRADE)


# warm upgrade
@main.command()
@click.argument('file_name', required=True)
@click.argument('slot_num', required=False, default=None)
def warm(file_name, slot_num):
    '''warm upgrade'''
    fwupg = FwUpg()
    fwupg.fw_upg(file_name, slot_num, WARM_UPGRADE)


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
@click.argument('erase_type', required=False, default="full")
def bmc(file_name, chip_select, erase_type):
    '''BMC upgrade'''
    platform = BasePlatform()
    platform.do_bmc_upgrade_main(file_name, chip_select, erase_type)


if __name__ == '__main__':
    signal_init()
    debug_init()
    main()
