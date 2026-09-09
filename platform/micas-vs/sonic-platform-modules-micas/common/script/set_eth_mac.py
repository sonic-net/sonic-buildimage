#!/usr/bin/env python
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import syslog
import os
import re
import eepromutil.onietlv as ot
from eepromutil.fru import ipmifru
from platform_config import STARTMODULE, SET_MAC_CONF
from platform_util import byteTostr, dev_file_read, exec_os_cmd


STANDARD_MAC_LEN = 12
SETMAC_DEBUG_FILE = "/etc/.setmac_debug_flag"

SETMACERROR = 1
SETMACDEBUG = 2
debuglevel = 0

cfg_prefix = "iface"
mac_prefix = "hwaddress ether"

def setmac_debug(s):
    if SETMACDEBUG & debuglevel:
        syslog.openlog("SETMAC", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_INFO, s)


def setmac_error(s):
    if SETMACERROR & debuglevel:
        syslog.openlog("SETMAC", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)

def setmac_info(s):
    syslog.openlog("SETMAC", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)


def debug_init():
    global debuglevel
    try:
        with open(SETMAC_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0


def decode_mac(encodedata):
    if encodedata == None:
        return None
    ret = ":".join("%02x" % ord(data) for data in encodedata)
    return ret.upper()


def validate_mac(value):
    if value is None:
        setmac_error("mac is none")
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
        setmac_error("mac format error")
        return False
    if len(temp_value) != STANDARD_MAC_LEN:
        setmac_error("mac len error len:%d" % len(temp_value))
        return False
    if temp_value == "000000000000":
        setmac_error("illegal zero mac")
        return False
    if int(temp_value, 16) >> 40 & 1 == 1:
        setmac_error("illegal mac")
        return False
    setmac_debug("mac validate success")
    return True


def get_onie_eeprom(eeprom):
    try:
        onietlv = ot.onie_tlv()
        rets = onietlv.decode(eeprom)
        setmac_debug("%-20s %-5s %-5s  %-20s" % ("TLV name", "Code", "lens", "Value"))
        for item in rets:
            if item["code"] == 0xfd:
                setmac_debug("%-20s 0x%-02X   %-5s" % (item["name"], item["code"], item["lens"]))
            else:
                setmac_debug("%-20s 0x%-02X   %-5s %-20s" % (item["name"], item["code"], item["lens"], item["value"]))
    except Exception as e:
        setmac_error(str(e))
        return False, str(e)
    return True, rets


def get_fru_eeprom_info(eeprom):
    try:
        fru = ipmifru()
        fru.decodeBin(eeprom)
    except Exception as e:
        setmac_error(str(e))
        return False, str(e)
    return True, fru


def get_mac_from_eeprom(eeprom_conf):
    name = eeprom_conf.get("name")
    e2_type = eeprom_conf.get("e2_type")
    e2_path = eeprom_conf.get("e2_path")
    e2_size = eeprom_conf.get("e2_size", 256)
    mac_location = eeprom_conf.get("mac_location", {})
    e2_mac = None

    support_e2_type = ("fru", "onie_tlv")
    if e2_type not in support_e2_type:
        msg = "Unsupport e2 type: %s" % e2_type
        return False, msg

    setmac_debug("===================%s===================" % name)
    ret, binval_bytes = dev_file_read(e2_path, 0, e2_size)
    if ret is False:
        msg = "eeprom read error, eeprom path: %s, msg: %s" % (e2_path, binval_bytes)
        return False, msg
    binval = byteTostr(binval_bytes)

    # onie_tlv
    if e2_type == "onie_tlv":
        status, eeprom_info = get_onie_eeprom(binval)
        if status is False:
            msg = "get_onie_eeprom failed, msg: %s" % (eeprom_info)
            return False, msg

        field = mac_location.get("field", "")
        for eeprom_info_item in eeprom_info:
            if eeprom_info_item.get("name") == field:
                e2_mac = eeprom_info_item.get("value")
                break
        if e2_mac is None:
            msg = "get_onie_eeprom mac address failed, e2_mac is None"
            return False, msg
        return True, e2_mac

    # fru
    status, eeprom_info = get_fru_eeprom_info(binval)
    if status is False:
        msg = "get_fru_eeprom_info failed, msg: %s" % (eeprom_info)
        return False, msg

    area = mac_location.get("area", "")
    field = mac_location.get("field", "")
    fru_area = getattr(eeprom_info, area, None)
    fru_field = getattr(fru_area, field, None)
    e2_mac = decode_mac(fru_field)
    if e2_mac is None:
        msg = "decode_mac failed, area: %s, field: %s, value: %s" % (area, field, fru_field)
        return False, msg
    return True, e2_mac


def read_mac_from_config_file(ifcfg):
    ifcfg_file_path = ifcfg.get("ifcfg_file_path")
    if not os.path.exists(ifcfg_file_path):
        msg = "%s not exist" % ifcfg_file_path
        return False, msg
    try:
        fd = open(ifcfg_file_path, 'r')
        for line in reversed(fd.readlines()):
            if line.strip().startswith(mac_prefix):
                mac = line.strip().replace(mac_prefix, "").strip()
                return True, mac
    except Exception as e:
        setmac_error(str(e))
        return False, str(e)
    return False, "mac address not found in %s" % ifcfg_file_path


def set_e2_mac_to_config_file(eth_name, mac, ifcfg):
    try:
        ifcfg_file_path = ifcfg.get("ifcfg_file_path")
        cfg_file_dir = os.path.dirname(ifcfg_file_path)
        if not os.path.exists(cfg_file_dir):
            cmd = "mkdir -p %s" % cfg_file_dir
            setmac_info("Create interfaces config directory: %s" % cfg_file_dir)
            exec_os_cmd(cmd)
            exec_os_cmd("sync")
        wr_val = cfg_prefix + " %s\n" % eth_name
        wr_val += "    %s %s\n" % (mac_prefix, mac)
        with open(ifcfg_file_path, "w") as fd:
            fd.write(wr_val)
        exec_os_cmd("sync")
        setmac_info("Create interfaces config: %s with mac address: %s" % (ifcfg_file_path, mac))
        return True
    except Exception as e:
        setmac_error(str(e))
        return False

def get_eth_current_mac(eth_name):
    get_mac_cmd = "ifconfig %s |grep ether |awk '{print $2}'" % eth_name
    status, output = exec_os_cmd(get_mac_cmd)
    if status or len(output) == 0:
        msg = "get mac exec cmd : %s fail, msg: %s" % (get_mac_cmd, output)
        setmac_error(msg)
        return False, msg
    mac = output.replace("\n", "").upper()
    return True, mac

def set_eth_mac(eth_name, mac):
    set_mac_cmd = "ifconfig %s hw ether %s" % (eth_name, mac)
    status, output = exec_os_cmd(set_mac_cmd)
    if status:
        setmac_error("run cmd: %s fail, msg: %s" % (set_mac_cmd, output))
        return False
    setmac_info("ifconfig %s with mac address: %s success" % (eth_name, mac))
    return True


def doSetmac():
    if STARTMODULE.get('set_eth_mac', 0) == 0:
        setmac_debug("set_eth_mac config not set")
        return

    try:
        if SET_MAC_CONF is None:
            setmac_debug("set_mac_conf in none")
            return

        if len(SET_MAC_CONF) == 0:
            setmac_debug("set_mac_conf list is none")
            return

        for setmac_item in SET_MAC_CONF:
            eth_name = setmac_item.get("eth_name")
            e2_name = setmac_item.get("e2_name", "")
            ifcfg = setmac_item.get("ifcfg")
            if eth_name is None or ifcfg is None:
                setmac_error("set_mac_conf error, eth_name or ifcfg is None")
                continue

            # decode mac by eeprom
            status, e2_mac = get_mac_from_eeprom(setmac_item)
            if status is False:
                setmac_error("get mac from %s eeprom fail, msg: %s" % (e2_name, e2_mac))
                continue
            status = validate_mac(e2_mac)
            if status is False:
                setmac_error("e2_mac: %s invalid" % e2_mac)
                continue
            setmac_debug("get mac from %s eeprom info success, mac: %s" % (e2_name, e2_mac))

            # read config file mac address
            status, cfg_mac = read_mac_from_config_file(ifcfg)
            setmac_debug("read_mac_from_config_file, status: %s, cfg_mac: %s" % (status, cfg_mac))
            if status is False or cfg_mac != e2_mac:
                set_e2_mac_to_config_file(eth_name, e2_mac, ifcfg)
                # check current eth mac
                status, current_mac = get_eth_current_mac(eth_name)
                if status is False:
                    setmac_error("get %s current mac fail" % eth_name)
                    continue
                setmac_debug("current_mac:%s len:%d" % (current_mac, len(current_mac)))
                if current_mac != e2_mac:
                    set_eth_mac(eth_name, e2_mac)
    except Exception as e:
        setmac_error(str(e))
    return


if __name__ == '__main__':
    debug_init()
    doSetmac()
