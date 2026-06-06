#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import click
from platform_util import dev_file_read, byteTostr, get_value, set_value, setup_logger, get_monotonic_time, exec_os_cmd, BSP_COMMON_LOG_DIR, common_syslog_warn, common_syslog_notice
from platform_config import SET_FW_MAC_CONF, SET_MAC_NEED_REBOOT
from eepromutil.fru import *
from eepromutil.fantlv import *
import eepromutil.onietlv as ot
import syslog
import time
import os
import re
import logging
import subprocess

WAIT_TIME = 10 * 60

TLV_CODE_MAC_BASE  = 0x24
SYSE2_PATH = "/sys/bus/i2c/devices/1-0056/eeprom"

STANDARD_MAC_LEN = 12

DEBUG_FILE = "/etc/.setmac_eth_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "setmac_eth_debug.log"
logger = setup_logger(LOG_FILE)


SETMAC_ONE_BY_ONE = 1
SETMAC_ONCE = 2 

SETMAC_TITLE = "SETMAC"

def setmac_warn(s):
    logger.info(s)
    common_syslog_warn(SETMAC_TITLE, s)

def setmac_notice(s):
    logger.info(s)
    common_syslog_notice(SETMAC_TITLE, s)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])


class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        elif len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))


def decode_mac(encodedata):
    if encodedata == None:
        return None
    ret = ":".join("%02x" % ord(data) for data in encodedata)
    return ret.upper()

def validate_mac(value):
    if value is None:
        logger.error("mac is none")
        return False
    if value.find('-') != -1:
        pattern = re.compile(r"^\s*([0-9a-fA-F]{2,2}-){5,5}[0-9a-fA-F]{2,2}\s*$")
        temp_value = value.replace("-", "")
    elif value.find(':') != -1:
        pattern = re.compile(r"^\s*([0-9a-fA-F]{2,2}:){5,5}[0-9a-fA-F]{2,2}\s*$")
        temp_value = value.replace(":", "")
    else:
        pattern = re.compile(r"^\s*([0-9a-fA-F]{2,2}){5,5}[0-9a-fA-F]{2,2}\s*$")
        temp_value = value
    if not pattern.match(value):
        logger.error("mac format error")
        return False
    if len(temp_value) != STANDARD_MAC_LEN:
        logger.error("mac len error len:%d" % len(temp_value))
        return False
    if temp_value == "000000000000":
        logger.error("illegal zero mac")
        return False
    if int(temp_value, 16) >> 40 & 1 == 1:
        logger.error("illegal mac")
        return False
    logger.debug("mac validate success")
    return True


def get_onie_eeprom(eeprom):
    # Read ONIE format E2 information
    try:
        onietlv = ot.onie_tlv()
        rets = onietlv.decode(eeprom)
        logger.debug("%-20s %-5s %-5s  %-20s" % ("TLV name", "Code", "lens", "Value"))
        for item in rets:
            if item["code"] == 0xfd:
                logger.debug("%-20s 0x%-02X   %-5s" % (item["name"], item["code"], item["lens"]))
            else:
                logger.debug("%-20s 0x%-02X   %-5s %-20s" % (item["name"], item["code"], item["lens"], item["value"]))
    except Exception as e:
        logger.error(str(e))
        return False, None
    return True, rets

def get_fru_eeprom_info(eeprom):
    # "Read FRU format E2 information.
    try:
        fru = ipmifru()
        fru.decodeBin(eeprom)
    except Exception as e:
        logger.error(str(e))
        return False, None
    return True, fru


def get_mac_from_eeprom(eeprom_conf):
    name = eeprom_conf.get("name")
    e2_type = eeprom_conf.get("e2_type", "onie_tlv")
    e2_path = eeprom_conf.get("e2_path", SYSE2_PATH)
    e2_size = eeprom_conf.get("e2_size", 256)
    e2_mac = ""
    logger.debug("===================%s===================" % name)
    ret, binval = dev_file_read(e2_path, 0, e2_size)
    if not ret:
        logger.error("get mac from %s fail" % e2_path)
        return ret, binval
    binval = byteTostr(binval)
    if binval.startswith("ERR"):
        logger.debug("eeprom read error, eeprom path: %s, msg: %s" % (e2_path, binval))
        return False, None
    if e2_type == "onie_tlv":
        status, eeprom_info = get_onie_eeprom(binval)
        if status:
            for eeprom_info_item in eeprom_info:
                if eeprom_info_item.get("code") == TLV_CODE_MAC_BASE:
                    e2_mac = eeprom_info_item.get("value")
                    return True, e2_mac
            logger.error("not find mac tlvcode in eeprom!")
    else:
        logger.error("UnSupport eeprom type: %s" % e2_type)
    return False, None

def set_dev_mac_func(mac_addr, set_mac_cfg):
    cmd = set_mac_cfg.get("cmd")
    type = set_mac_cfg.get("type")
    pre_cmd = set_mac_cfg.get("pre_cmd", [])
    if cmd is None:
        log = "set_dev_mac cmd is none"
        logger.error(log)
        return False, log
    for item in pre_cmd:
        ret, log = set_value(item)
        if ret is False:
            log = ("set_dev_mac_func exec pre_cmd fail, cmd: %s" % item)
            logger.error(log)
            return False, log
    logger.debug("set mac addr by type %d" % type)
    # One command to set a byte MAC address
    if type == SETMAC_ONE_BY_ONE:
        mac_list = mac_addr.split(":")
        for i in range(int(STANDARD_MAC_LEN/2)):
            tmp_dict = {}
            tmp_dict['cmd'] = cmd.get('cmd') % (i, mac_list[i])
            tmp_dict['gettype'] = cmd.get('gettype', 'cmd')
            tmp_dict['delay'] = cmd.get('delay', 0)
            logger.debug("set mac cmd%d: %s" % (i, tmp_dict))
            ret, log = set_value(tmp_dict)
            if ret is False:
                log = ("set_dev_mac_func fail, cmd: %s" % tmp_dict)
                logger.error(log)
                return False, log
    # One command to set the complete MAC address
    elif type == SETMAC_ONCE:
        tmp_dict = {}
        tmp_dict['cmd'] = cmd.get('cmd') % mac_addr
        tmp_dict['gettype'] = cmd.get('gettype', 'cmd')
        tmp_dict['delay'] = cmd.get('delay', 0)
        logger.debug("set mac cmd: %s" % tmp_dict)
        ret, log = set_value(tmp_dict)
        if ret is False:
            log = ("set_dev_mac_func fail, cmd: %s" % tmp_dict)
            logger.error(log)
            return False, log
    else:
        log = ("unsupport set_dev_mac type, type: %d" % type)
        logger.error(log)
        return False, log
    log = ("set_dev_mac_func success")
    logger.debug(log)
    return True, log

def get_and_validate_act_mac(get_act_mac, eth_name, e2_mac, wait_time):
    start_time = get_monotonic_time()
    get_mac_log_flag = True
    while True:
        if get_monotonic_time() - start_time > wait_time and get_mac_log_flag:
            get_mac_log_flag = False
            start_time = get_monotonic_time()
            setmac_warn("Failed to set the system mac(%s) for %s within %s seconds." % (e2_mac, eth_name, wait_time))
        ret, act_mac = get_value(get_act_mac)
        if not ret:
            logger.debug("get %s act mac fail, reason %s" % (eth_name, act_mac))
            time.sleep(5)
            continue
        act_mac = act_mac.lower().strip()
        status = validate_mac(act_mac)
        if not status:
            logger.debug("validate %s act mac fail. (act_mac %s, e2_mac %s)" % (eth_name, act_mac, e2_mac))
            time.sleep(5)
        else:
            logger.debug("get act mac success, mac: %s " % (act_mac))
            return True, act_mac

def doSetmac():
    try:
        if SET_FW_MAC_CONF is None:
            logger.error("set_mac_conf in none")
            return
        if len(SET_FW_MAC_CONF) == 0:
            logger.error("set_mac_conf list is none")
            return
        all_success_flag = True
        any_mac_needs_set = False
        for setmac_item in SET_FW_MAC_CONF:
            eth_name = setmac_item.get("name", "")
            e2_name = setmac_item.get("e2_name", "")
            get_act_mac = setmac_item.get("get_act_mac", None)
            set_mac_cfg = setmac_item.get("set_mac_cfg", None)
            check_mac_cfg = setmac_item.get("check_mac_cfg", None)
            set_mac_end_cmd =  setmac_item.get("set_mac_end_cmd", [])
            enable_mac_cfg = setmac_item.get("enable_mac_cfg", None)

            if get_act_mac is None or set_mac_cfg is None:
                all_success_flag = False
                logger.error("%s setmac config err, please check" % eth_name)
                continue

            # Parse the MAC from E2
            status, e2_mac = get_mac_from_eeprom(setmac_item)
            if not status:
                all_success_flag = False
                logger.error("get mac from %s eeprom fail" % e2_name)
                continue
            e2_mac = e2_mac.lower()
            status = validate_mac(e2_mac)
            if not status:
                all_success_flag = False
                logger.error("validate %s eeprom mac fail" % eth_name)
                continue
            logger.debug("get mac from %s eeprom info success, mac: %s " % (e2_name, e2_mac))

            status, act_mac = get_and_validate_act_mac(get_act_mac, eth_name, e2_mac, WAIT_TIME)
            if not status:
                all_success_flag = False
                logger.error("get act_mac fail: %s " % (act_mac))
                continue

            if act_mac != e2_mac:
                any_mac_needs_set = True
                logger.info("%s act_mac not equl e2_mac, do set_dev_mac_func" % eth_name)
                ret, log = set_dev_mac_func(e2_mac, set_mac_cfg)
                if not ret:
                    all_success_flag = False
                    logger.debug("set e2_mac to %s fail" % eth_name)
                    continue
            else:
                logger.debug("%s act_mac(%s) equl e2_mac(%s), do nothing" % (eth_name, act_mac, e2_mac))
                continue

            if check_mac_cfg is not None:
                ret, check_mac = get_value(check_mac_cfg)
                if not ret:
                    all_success_flag = False
                    logger.error("get %s check mac fail" % eth_name)
                    continue
                check_mac = check_mac.lower().strip()
                logger.debug("get check_mac success, mac: %s " % (check_mac))
                if check_mac != e2_mac:
                    all_success_flag = False
                    logger.error("check %s mac fail" % eth_name)
                    continue
                logger.info("check_mac success")
            
            if enable_mac_cfg is not None:
                ret, log = set_dev_mac_func(e2_mac, enable_mac_cfg)
                if not ret:
                    all_success_flag = False
                    logger.debug("enable %s e2_mac fail, reason: %s" % (eth_name, log))
                    continue
                logger.info("enable %s mac(%s) success" % (eth_name, e2_mac))

            set_mac_cmd_flag = True
            for set_mac_end_cmd_item in set_mac_end_cmd:
                ret, log = set_value(set_mac_end_cmd_item)
                if not ret:
                    all_success_flag = False
                    logger.error("%s exec end cmd fail, cmd: %s, reason: %s" % (eth_name, set_mac_end_cmd_item, log))
                    set_mac_cmd_flag = False
                    break
                logger.debug("exec set_mac_end_cmd:%s success" % set_mac_end_cmd_item)
            if set_mac_cmd_flag is True:
                setmac_notice("Success to set the system mac(%s) for %s" % (e2_mac, eth_name))

        if any_mac_needs_set and all_success_flag:
            logger.info("All eth mac set success, do reboot!")
            if SET_MAC_NEED_REBOOT:
                exec_os_cmd("sync")
                time.sleep(3)
                os.system("/sbin/reboot")
        elif any_mac_needs_set and not all_success_flag:
            logger.error("Some ETH MAC addresses failed to set. No reboot.")
        else:
            logger.info("No ETH MAC addresses needed to be set. No reboot.")

    except Exception as e:
        logger.error(str(e))
    return


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass


@main.command()
def start():
    '''start setmac'''
    logger.debug("start setmac")
    doSetmac()
    time.sleep(5) #delay to satisfy supervisor
    exit(0)


if __name__ == '__main__':
    debug_init()
    main()
