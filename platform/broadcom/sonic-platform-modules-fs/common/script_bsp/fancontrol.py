#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import sys
import click
import os
import subprocess
import time
from platform_util import *
import syslog
import traceback
import glob
import copy
import fcntl
import math
from wbutil.smbus import SMBus
from public.platform_common_config import SDKCHECK_PARAMS

CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])
DEBUG_FILE = "/etc/.fancontrol_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "fancontrol_debug.log"
logger = setup_logger(LOG_FILE)
OTP_REBOOT_JUDGE_FILE = "/etc/.otp_reboot_flag"  # coordination with REBOOT_CAUSE_PARA


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


def fanwarninglog(s):
    syslog.openlog("FANCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, s)
    logger.warning(s)

def fancriticallog(s):
    syslog.openlog("FANCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_CRIT, s)
    logger.critical(s)

def fanerror(s):
    syslog.openlog("FANCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)
    logger.error(s)

def faninfo(s):
    logger.info(s)

def fanwarningdebuglog(debug, s):
    logger.debug(s)

def exec_os_cmd(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        print(output)
    return status, output


pidfile = -1


def file_rw_lock(file_path):
    global pidfile
    pidfile = open(file_path, "r")
    try:
        fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)  # Create an exclusive lock that does not block other processes that are locked
        fanwarningdebuglog(DEBUG_FANCONTROL, "file lock success")
        return True
    except Exception as e:
        pidfile.close()
        pidfile = -1
        return False


def file_rw_unlock():
    try:
        global pidfile

        if pidfile != -1:
            fcntl.flock(pidfile, fcntl.LOCK_UN)  # Release lock
            pidfile.close()
            pidfile = -1
            fanwarningdebuglog(DEBUG_FANCONTROL, "file unlock success")
        else:
            fanwarningdebuglog(DEBUG_FANCONTROL, "pidfile is invalid, do nothing")
        return True
    except Exception as e:
        fanwarningdebuglog(DEBUG_FANCONTROL, "file unlock err, msg:%s" % (str(e)))
        return False


def readsysfs(location):
    try:
        locations = glob.glob(location)
        with open(locations[0], 'rb') as fd1:
            retval = fd1.read()
        retval = retval.rstrip('\r\n')
        retval = retval.lstrip(" ")
    except Exception as e:
        return False, (str(e) + " location[%s]" % location)
    return True, retval


class pid(object):
    __pid_config = None

    def __init__(self):
        self.__pid_config = copy.deepcopy(MONITOR_CONST.MONITOR_PID_MODULE)

    def get_para(self, name):
        para = self.__pid_config.get(name)
        return para

    def get_temp_update(self, pid_para, current_temp):
        temp = pid_para["value"]
        if temp is None:
            return None
        temp.append(current_temp)
        del temp[0]
        return temp

    def cacl(self, last_pwm, name, current_temp):
        delta_pwm = 0
        fanwarningdebuglog(DEBUG_FANCONTROL, "pid last_pwm = 0x%x" % last_pwm)

        pid_para = self.get_para(name)
        if pid_para is None:
            fanwarningdebuglog(DEBUG_FANCONTROL, "get %s pid para failed" % name)
            return None

        temp = self.get_temp_update(pid_para, current_temp)
        if temp is None:
            fanwarningdebuglog(DEBUG_FANCONTROL, "get %s update failed" % name)
            return None

        type = pid_para["type"]
        Kp = pid_para["Kp"]
        Ki = pid_para["Ki"]
        Kd = pid_para["Kd"]
        target = pid_para["target"]
        pwm_min = pid_para["pwm_min"]
        pwm_max = pid_para["pwm_max"]
        flag = pid_para["flag"]

        if flag != 1:
            fanwarningdebuglog(DEBUG_FANCONTROL, "%s pid flag == 0" % name)
            return None

        if type == "duty":
            current_pwm = round(last_pwm * 100 / 255)
        else:
            current_pwm = last_pwm

        if (temp[2] is None):
            tmp_pwm = current_pwm
        elif ((temp[0] is None) or (temp[1] is None)):
            delta_pwm = Ki * (temp[2] - target)
            tmp_pwm = current_pwm + delta_pwm
        else:
            delta_pwm = Kp * (temp[2] - temp[1]) + Ki * (temp[2] - target) + Kd * (temp[2] - 2 * temp[1] + temp[0])
            tmp_pwm = current_pwm + delta_pwm

        fanwarningdebuglog(DEBUG_FANCONTROL, "pid delta_pwm = %d" % delta_pwm)
        if type == "duty":
            pwm = round(tmp_pwm * 255 / 100)
        else:
            pwm = tmp_pwm

        pwm = min(pwm, pwm_max)
        pwm = max(pwm, pwm_min)
        fanwarningdebuglog(DEBUG_FANCONTROL, "pid last_pwm = 0x%x, pwm = 0x%x" % (last_pwm, pwm))
        return pwm


class FanControl():
    critnum = 0
    __pwm = MONITOR_CONST.MIN_SPEED  # Finally set the fan speed

    error_temp = -9999  # Temperature acquisition failure
    invalid_temp = -10000  # Invalid temperature acquisition

    def __init__(self):
        self._fanOKNum = 0
        self._psuOKNum = 0
        self._intemp = -100.0
        self._mac_aver = -100.0
        self._mac_max = -100.0
        self._preIntemp = -1000  # Last time temperature
        self._outtemp = -100
        self._boardtemp = -100
        self._cputemp = -1000
        self._sfftemp = self.invalid_temp
        self._pre_openloop_pwm = MONITOR_CONST.MIN_SPEED
        self.pid = pid()
        pass

    @property
    def fanOKNum(self):
        return self._fanOKNum

    @property
    def psuOKNum(self):
        return self._psuOKNum

    @property
    def cputemp(self):
        return self._cputemp

    @property
    def sfftemp(self):
        return self._sfftemp

    @property
    def intemp(self):
        return self._intemp

    @property
    def outtemp(self):
        return self._outtemp

    @property
    def boardtemp(self):
        return self._boardtemp

    @property
    def mac_aver(self):
        return self._mac_aver

    @property
    def preIntemp(self):
        return self._preIntemp

    @preIntemp.setter
    def preIntemp(self, val):
        self._preIntemp = val

    @property
    def pre_openloop_pwm(self):
        return self._pre_openloop_pwm

    @pre_openloop_pwm.setter
    def pre_openloop_pwm(self, val):
        self._pre_openloop_pwm = val

    @property
    def mac_max(self):
        return self._mac_max

    def sortCallback(self, element):
        return element['id']

    def get_value_one_time(self, config):
        try:
            way = config.get("gettype")
            if way == 'sysfs':
                loc = config.get("loc")
                ret, val = readsysfs(loc)
                if ret == True:
                    return True, int(val, 16)
                else:
                    return False, ("sysfs read %s failed. log:%s" % (loc, val))
            elif way == "i2c":
                bus = config.get("bus")
                addr = config.get("loc")
                offset = config.get("offset")
                ret, val = rji2cget(bus, addr, offset)
                if ret == True:
                    return True, int(val, 16)
                else:
                    return False, ("i2c read failed. bus:%d , addr:0x%x, offset:0x%x" % (bus, addr, offset))
            elif way == "io":
                io_addr = config.get('io_addr')
                val = io_rd(io_addr)
                if len(val) != 0:
                    return True, int(val, 16)
                else:
                    return False, ("io_addr read 0x%x failed" % io_addr)
            elif way == "i2cword":
                bus = config.get("bus")
                addr = config.get("loc")
                offset = config.get("offset")
                ret, val = rji2cgetWord(bus, addr, offset)
                if ret == True:
                    return True, int(val, 16)
                else:
                    return False, ("i2cword read failed. bus:%d, addr:0x%x, offset:0x%x" % (bus, addr, offset))
            elif way == "devfile":
                path = config.get("path")
                offset = config.get("offset")
                read_len = config.get("read_len")
                ret, val_list = dev_file_read(path, offset, read_len)
                if ret == True:
                    return True, val_list
                else:
                    return False, ("devfile read failed. path:%s, offset:0x%x, read_len:%d" % (path, offset, read_len))
            elif way == 'cmd':
                cmd = config.get("cmd")
                ret, val = exec_os_cmd(cmd)
                if ret:
                    return False, ("cmd read exec %s failed, log: %s" % (cmd, val))
                else:
                    return True, int(val, 16)
            else:
                return False, "not support read type"
        except Exception as e:
            fanwarningdebuglog(DEBUG_COMMON, "get_value_one_time happen exception, log:%s" % str(e))
            return False, str(e)

    def get_value(self, config):
        retrytime = 3
        for i in range(retrytime):
            ret, val = self.get_value_one_time(config)
            if ret is True:
                return True, val
            if (i + 1) < retrytime:
                time.sleep(0.1)
        return False, val

    def get_sff_temp(self, path):
        loop = 1000
        ret = False
        try:
            for i in range(0, loop):
                ret = file_rw_lock(path)
                if ret is True:
                    break
                time.sleep(0.001)

            if ret is False:
                fanwarningdebuglog(DEBUG_FANCONTROL, "take sff temp file lock timeout")
                rval = self.invalid_temp
                return rval

            with open(path, 'r') as fd1:
                retval = fd1.read().strip()
            file_rw_unlock()
            rval = float(retval) / 1000
            return rval
        except Exception as e:
            fanwarningdebuglog(DEBUG_FANCONTROL, "get_sff_temp error, msg:%s" % str(e))
            file_rw_unlock()
            rval = self.invalid_temp
            return rval

    def gettemp(self, ret):
        u'''Obtain the air inlet, air outlet, hot spot, and CPU temperature'''
        temp_conf = MONITOR_DEV_STATUS.get('temperature', None)

        if temp_conf is None:
            fanerror("gettemp: config error")
            return False
        for item_temp in temp_conf:
            try:
                retval = ""
                rval = None
                name = item_temp.get('name')
                location = item_temp.get('location')
                if name == "cpu":
                    L = []
                    for dirpath, dirnames, filenames in os.walk(location):
                        for file in filenames:
                            if file.endswith("input"):
                                L.append(os.path.join(dirpath, file))
                        L = sorted(L, reverse=False)
                    for i in range(len(L)):
                        nameloc = "%s/temp%d_label" % (location, i + 1)
                        valloc = "%s/temp%d_input" % (location, i + 1)
                        with open(nameloc, 'r') as fd1:
                            retval2 = fd1.read()
                        with open(valloc, 'r') as fd2:
                            retval3 = fd2.read()
                        ret_t = {}
                        ret_t["name"] = retval2.strip()
                        ret_t["value"] = float(retval3) / 1000
                        ret.append(ret_t)
                        fanwarningdebuglog(DEBUG_COMMON, "gettemp %s : %f" % (ret_t["name"], ret_t["value"]))
                else:
                    locations = glob.glob(location)
                    ret_t = {}
                    if len(locations) != 0:
                        if name == "sff":
                            rval = self.get_sff_temp(locations[0])
                            ret_t["value"] = rval
                        else:
                            with open(locations[0], 'r') as fd1:
                                retval = fd1.read().strip()
                            rval = float(retval) / 1000
                            ret_t["value"] = rval
                    else:
                        fanwarningdebuglog(DEBUG_COMMON, "file %s not exist" % (location))
                        ret_t["value"] = self.invalid_temp
                    ret_t["name"] = name
                    ret.append(ret_t)
                    fanwarningdebuglog(DEBUG_COMMON, "gettemp %s : %f" % (ret_t["name"], ret_t["value"]))
            except Exception as e:
                fanerror("gettemp error:name:%s" % name)
                fanerror(str(e))
        return True

    def checkslot(self, ret):
        u'''Gets the subcard status'''
        slots_conf = MONITOR_DEV_STATUS.get('slots', None)
        slotpresent = MONITOR_DEV_STATUS_DECODE.get('slotpresent', None)

        if slots_conf is None or slotpresent is None:
            return False
        for item_slot in slots_conf:
            totalerr = 0
            try:
                ret_t = {}
                ret_t["id"] = item_slot.get('name')
                ret_t["status"] = ""
                gettype = item_slot.get('gettype')
                presentbit = item_slot.get('presentbit')
                if gettype == "io":
                    io_addr = item_slot.get('io_addr')
                    val = io_rd(io_addr)
                    if val is not None:
                        retval = val
                    else:
                        totalerr -= 1
                        fanerror(" %s  %s" % (item_slot.get('name'), "lpc read failure"))
                else:
                    bus = item_slot.get('bus')
                    loc = item_slot.get('loc')
                    offset = item_slot.get('offset')
                    ind, val = rji2cget(bus, loc, offset)
                    if ind == True:
                        retval = val
                    else:
                        totalerr -= 1
                        fanerror(" %s  %s" % (item_slot.get('name'), "i2c read failure"))
                if totalerr < 0:
                    ret_t["status"] = "ERR"
                    ret.append(ret_t)
                    continue
                val_t = (int(retval, 16) & (1 << presentbit)) >> presentbit
                fanwarningdebuglog(DEBUG_COMMON, "%s present:%s" % (item_slot.get('name'), slotpresent.get(val_t)))
                if val_t != slotpresent.get('okval'):
                    ret_t["status"] = "ERR"
                    ret.append(ret_t)
                    fanwarningdebuglog(DEBUG_COMMON, "%s absent" % (ret_t["id"]))
                    continue
                else:
                    file_cfg_enable = item_slot.get('file_cfg_enable', 0)
                    if file_cfg_enable == 1:
                        docker_command = ""
                        docker = item_slot.get('docker', None)
                        if docker is not None:
                            docker_command = "docker exec %s" % docker
                            fanwarningdebuglog(
                                DEBUG_COMMON, "%s status file in docker:%s." %
                                (item_slot.get('name'), docker))
                        file_path = item_slot.get('file_path', "")
                        rd_command = "%s cat %s" % (docker_command, file_path)
                        fanwarningdebuglog(DEBUG_COMMON, "get slot status,exec command:%s." % (rd_command))
                        rd_status, rd_value = rj_os_system(rd_command)
                        rd_value = rd_value.strip().replace("\r", "").replace("\n", "")
                        fanwarningdebuglog(DEBUG_COMMON, "%s get value:%s." % (item_slot.get('name'), rd_value))
                        if rd_status:
                            ret_t["status"] = "NONE"
                            fanwarningdebuglog(
                                DEBUG_COMMON, "%s %s led_status_file:%s not exit" %
                                (item_slot.get('name'), docker, file_path))
                        else:
                            slots_led_status = MONITOR_DEV_STATUS.get('slots_led_status', None)
                            led_status = slots_led_status.get(rd_value, "ERR")
                            fanwarningdebuglog(DEBUG_COMMON, "%s led_status:%s" % (ret_t["id"], led_status))
                            ret_t["status"] = led_status
                    else:
                        ret_t["status"] = "PRESENT"
            except Exception as e:
                ret_t["status"] = "ERR"
                totalerr -= 1
                fanerror("checkslot error")
                fanerror(str(e))
            ret.append(ret_t)
        fanwarningdebuglog(DEBUG_COMMON, "finally slot status %s" % (ret))
        return True

    def checkpsu(self, ret):
        u'''Obtain power supply status, output, and alarms'''
        psus_conf = MONITOR_DEV_STATUS.get('psus', None)
        psupresent = MONITOR_DEV_STATUS_DECODE.get('psupresent', None)
        psuoutput = MONITOR_DEV_STATUS_DECODE.get('psuoutput', None)
        psualert = MONITOR_DEV_STATUS_DECODE.get('psualert', None)

        if psus_conf is None or psupresent is None or psuoutput is None:
            fanerror("checkpsu: config error")
            return False
        for item_psu in psus_conf:
            try:
                ret_t = {}
                ret_t["id"] = item_psu.get('name')
                ret_t["status"] = ""
                status, val = self.get_value(item_psu)
                if status is True:
                    retval = val
                else:
                    fanerror(" %s  %s" % (item_psu.get('name'), val))
                    ret_t["status"] = "UNKNOWN"
                    ret.append(ret_t)
                    continue

                presentbit = item_psu.get('presentbit')
                statusbit = item_psu.get('statusbit')
                alertbit = item_psu.get('alertbit')
                val_present = (retval & (1 << presentbit)) >> presentbit
                val_status = (retval & (1 << statusbit)) >> statusbit
                val_alert = (retval & (1 << alertbit)) >> alertbit
                fanwarningdebuglog(DEBUG_COMMON, "%s present:%s output:%s alert:%s" %
                                   (item_psu.get('name'), psupresent.get(val_present), psuoutput.get(val_status), psualert.get(val_alert)))

                if val_present != psupresent.get('okval'):
                    ret_t["status"] = "ABSENT"
                    ret.append(ret_t)
                    continue

                clear_fault_conf = item_psu.get('clear_fault', None)
                if (val_status != psuoutput.get('okval') or val_alert !=
                        psualert.get('okval')) and clear_fault_conf is not None:
                    fanwarningdebuglog(DEBUG_COMMON, "%s not OK, try to clear faults" % (item_psu.get('name')))
                    self.get_value(clear_fault_conf)
                    fanwarningdebuglog(
                        DEBUG_COMMON,
                        "after clear faults, read %s status again" %
                        (item_psu.get('name')))
                    status, retval = self.get_value(item_psu)
                    if status is True:
                        val_present = (retval & (1 << presentbit)) >> presentbit
                        val_status = (retval & (1 << statusbit)) >> statusbit
                        val_alert = (retval & (1 << alertbit)) >> alertbit
                        fanwarningdebuglog(DEBUG_COMMON, "after clear faults, %s present:%s output:%s alert:%s" %
                                           (item_psu.get('name'), psupresent.get(val_present), psuoutput.get(val_status), psualert.get(val_alert)))
                    else:
                        fanerror(" %s  %s" % (item_psu.get('name'), retval))
                        ret_t["status"] = "NOT OK"
                        ret.append(ret_t)
                        continue

                if val_status != psuoutput.get('okval') or val_alert != psualert.get('okval'):
                    ret_t["status"] = "NOT OK"
                else:
                    ret_t["status"] = "OK"
            except Exception as e:
                fanerror("checkpsu error")
                fanerror(str(e))
                ret_t["status"] = "UNKNOWN"
            ret.append(ret_t)
        return True

    def checkfan(self, ret):
        u'''Obtain the fan status. Whether the fan is on or running'''
        fans_conf = MONITOR_DEV_STATUS.get('fans', None)
        fanpresent = MONITOR_DEV_STATUS_DECODE.get('fanpresent', None)
        fanroll = MONITOR_DEV_STATUS_DECODE.get('fanroll', None)

        if fans_conf is None or fanpresent is None or fanroll is None:
            fanerror("checkfan: config error")
            return False
        for item_fan in fans_conf:
            totalerr = 0
            try:
                ret_t = {}
                ret_t["id"] = item_fan.get('name')
                ret_t["status"] = ""
                presentstatus = item_fan.get('presentstatus')
                presentbus = presentstatus.get('bus')
                presentloc = presentstatus.get('loc')
                presentaddr = presentstatus.get('offset')
                presentbit = presentstatus.get('bit')
                ind, val = rji2cget(presentbus, presentloc, presentaddr)
                if ind == True:
                    val_t = (int(val, 16) & (1 << presentbit)) >> presentbit
                    fanwarningdebuglog(
                        DEBUG_COMMON, "checkfan:%s present status:%s" %
                        (item_fan.get('name'), fanpresent.get(val_t)))
                    if val_t != fanpresent.get('okval'):
                        ret_t["status"] = "ABSENT"
                        ret.append(ret_t)
                        continue
                else:
                    fanerror("checkfan: %s get present status error." % item_fan.get('name'))
                motors = item_fan.get("rollstatus")
                for motor in motors:
                    statusbus = motor.get('bus', None)
                    statusloc = motor.get('loc', None)
                    statusaddr = motor.get('offset', None)
                    statusbit = motor.get('bit', None)
                    ind, val = rji2cget(statusbus, statusloc, statusaddr)
                    if ind == True:
                        val_t = (int(val, 16) & (1 << statusbit)) >> statusbit
                        fanwarningdebuglog(
                            DEBUG_COMMON, "checkfan:%s roll status:%s" %
                            (motor.get('name'), fanroll.get(val_t)))
                        if val_t != fanroll.get('okval'):
                            totalerr -= 1
                    else:
                        totalerr -= 1
                        fanerror("checkfan: %s get %s status error." % item_fan.get('name'), motor["name"])
            except Exception as e:
                totalerr -= 1
                fanerror("checkfan error")
                fanerror(str(e))
            if totalerr < 0:
                ret_t["status"] = "NOT OK"
            else:
                ret_t["status"] = "OK"
            ret.append(ret_t)
        return True

    def getCurrentSpeed(self):
        try:
            loc = fanloc[0].get("location", "")
            sped = get_sysfs_value(loc)
            value = strtoint(sped)
            return value
        except Exception as e:
            fanerror("%%policy: get current speedlevel error")
            fanerror(str(e))
            return None

    def present_psu_fan_speed_set(self, psu_status, duty):
        for status_item in psu_status:
            psu_id = status_item['id']
            if (status_item['status'] == "OK"):
                item_psu = PSU_FAN_FOLLOW.get(psu_id, None)
                if item_psu is not None:
                    try:
                        ret, val = rji2cset_pec(item_psu['bus'], item_psu['loc'],
                                                item_psu['offset1'], item_psu['value1'])
                        if ret == False:
                            fanwarningdebuglog(DEBUG_COMMON, "%s fan speed i2c_pec set failed" % psu_id)
                            continue
                        rji2cset_wordpec(item_psu['bus'], item_psu['loc'], item_psu['offset2'], duty)
                    except Exception as e:
                        fanwarningdebuglog(DEBUG_COMMON, "present_psu_fan_speed_set error")
                else:
                    fanwarningdebuglog(DEBUG_COMMON, "config file get %s cfg failed" % psu_id)
            else:
                fanwarningdebuglog(DEBUG_COMMON, "%s is ABSENT" % psu_id)
        return

    def psu_fan_speed_set(self, duty):
        psu_status = []
        ret = self.checkpsu(psu_status)
        if ret == True:
            self.present_psu_fan_speed_set(psu_status, duty)
        else:
            fanerror("psu_fan_speed_set: get psu config error.")
        return

    def fanSpeedSet(self, level):
        if level >= MONITOR_CONST.MAX_SPEED:
            level = MONITOR_CONST.MAX_SPEED
        if level <= MONITOR_CONST.MIN_SPEED:
            level = MONITOR_CONST.MIN_SPEED
        for item in fanloc:
            try:
                loc = item.get("location", "")
                write_sysfs_value(loc, "0x%02x" % level)
            except Exception as e:
                fanerror(str(e))
                fanerror("%%policy: config fan runlevel error")
        if PSU_FAN_FOLLOW is not None:  # psu_fan speed follow fan
            try:
                duty = round(level * 100 / 255)
                self.psu_fan_speed_set(duty)
            except Exception as e:
                fanerror(str(e))
                fanerror("%%policy: config psu_fan runlevel error")

    def fanSpeedSetMax(self):
        try:
            self.fanSpeedSet(MONITOR_CONST.MAX_SPEED)
        except Exception as e:
            fanerror("%%policy:fanSpeedSetMax failed")
            fanerror(str(e))

    def fanStatusCheck(self):  # The fan status is detected. If a fan is abnormal, the fan spins at full speed
        if self.fanOKNum < MONITOR_CONST.FAN_TOTAL_NUM:
            fanwarninglog("%%DEV_MONITOR-FAN: Normal fan number: %d" % (self.fanOKNum))
            return False
        return True

    def setFanAttr(self, val):
        u'''Set the status of each fan'''
        for item in val:
            fanid = item.get("id")
            fanattr = fanid + "status"
            fanstatus = item.get("status")
            setattr(FanControl, fanattr, fanstatus)
            fanwarningdebuglog(DEBUG_COMMON, "fanattr:%s,fanstatus:%s" % (fanattr, fanstatus))

    def getFanPresentNum(self, curFanStatus):
        fanoknum = 0
        for item in curFanStatus:
            if item["status"] == "OK":
                fanoknum += 1
        self._fanOKNum = fanoknum
        fanwarningdebuglog(DEBUG_COMMON, "fanOKNum = %d" % self._fanOKNum)

    def getFanStatus(self):
        try:
            curFanStatus = []
            ret = self.checkfan(curFanStatus)
            if ret == True:
                self.setFanAttr(curFanStatus)
                self.getFanPresentNum(curFanStatus)
                fanwarningdebuglog(DEBUG_COMMON, "%%policy:getFanStatus success")
                return 0
        except AttributeError as e:
            fanerror(str(e))
        except Exception as e:
            fanerror(str(e))
        return -1

    def getPsuOkNum(self, curPsuStatus):
        psuoknum = 0
        for item in curPsuStatus:
            if item.get("status") == "OK":
                psuoknum += 1
        self._psuOKNum = psuoknum
        fanwarningdebuglog(DEBUG_COMMON, "psuOKNum = %d" % self._psuOKNum)

    def getPsuStatus(self):
        try:
            curPsuStatus = []
            ret = self.checkpsu(curPsuStatus)
            if ret == True:
                self.getPsuOkNum(curPsuStatus)
                fanwarningdebuglog(DEBUG_COMMON, "%%policy:getPsuStatus success")
                return 0
        except AttributeError as e:
            fanerror(str(e))
        except Exception as e:
            fanerror(str(e))
        return -1

    def getMonitorTemp(self, temp):
        for item in temp:
            if item.get('name') == "lm75in":
                self._intemp = item.get('value', self._intemp)
            if item.get('name') == "lm75out":
                self._outtemp = item.get('value', self._outtemp)
            if item.get('name') == "lm75hot":
                self._boardtemp = item.get('value', self._boardtemp)
            if item.get('name') == "Physical id 0" or item.get('name') == "Package id 0":
                self._cputemp = item.get('value', self._cputemp)
            if item.get('name') == "sff":
                self._sfftemp = item.get('value', self._sfftemp)
        fanwarningdebuglog(DEBUG_COMMON, "intemp:%f, outtemp:%f, boadrtemp:%f, cputemp:%f, sfftemp:%f" %
                           (self._intemp, self._outtemp, self._boardtemp, self._cputemp, self._sfftemp))

    def getTempStatus(self):
        try:
            monitortemp = []
            ret = self.gettemp(monitortemp)
            if ret == True:
                self.getMonitorTemp(monitortemp)
                fanwarningdebuglog(DEBUG_COMMON, "%%policy:getTempStatus success")
                return 0
        except AttributeError as e:
            fanerror(str(e))
        except Exception as e:
            fanerror(str(e))
        return -1

    def getMacStatus_bcmcmd(self):
        try:
            if waitForDocker(SDKCHECK_PARAMS, timeout=0) == True:
                sta, ret = getMacTemp()
                if sta == True:
                    self._mac_aver = float(ret.get("average", self._mac_aver))
                    self._mac_max = float(ret.get("maximum", self._mac_max))
                    fanwarningdebuglog(DEBUG_COMMON, "mac_aver:%f, mac_max:%f" % (self.mac_aver, self._mac_max))
                else:
                    fanwarningdebuglog(DEBUG_COMMON, "%%policy:getMacStatus_bcmcmd failed")
            else:
                fanwarningdebuglog(DEBUG_COMMON, "%%policy:getMacStatus_bcmcmd SDK not OK")
            return 0
        except AttributeError as e:
            fanerror(str(e))
        return -1

    def getMacStatus_sysfs(self, conf):
        try:
            sta, ret = getMacTemp_sysfs(conf)
            if sta == True:
                self._mac_aver = float(ret) / 1000
                self._mac_max = float(ret) / 1000
                fanwarningdebuglog(DEBUG_COMMON, "mac_aver:%f, mac_max:%f" % (self.mac_aver, self._mac_max))
            elif conf.get("try_bcmcmd", 0) == 1:
                fanwarningdebuglog(DEBUG_COMMON, "get sysfs mac temp failed.try to use bcmcmd")
                self.getMacStatus_bcmcmd()
            else:
                fanwarningdebuglog(DEBUG_COMMON, "%%policy:getMacStatus_sysfs failed")
            return 0
        except AttributeError as e:
            fanerror(str(e))
        return -1

    def getMacStatus(self):
        try:
            mactempconf = MONITOR_DEV_STATUS.get('mac_temp', None)
            if mactempconf is not None:
                self.getMacStatus_sysfs(mactempconf)
            else:
                self.getMacStatus_bcmcmd()
            return 0
        except AttributeError as e:
            fanerror(str(e))
        return -1

    def settSlotAttr(self, val):
        u'''Set the presence status properties for each child card'''
        for item in val:
            slotid = item.get("id")
            slotattr = slotid + "status"
            slotstatus = item.get("status")
            setattr(FanControl, slotattr, slotstatus)
            fanwarningdebuglog(DEBUG_COMMON, "slotattr:%s,slotstatus:%s" % (slotattr, slotstatus))

    def getSlotStatus(self):
        try:
            curSlotStatus = []
            ret = self.checkslot(curSlotStatus)
            if ret == True:
                self.settSlotAttr(curSlotStatus)
                fanwarningdebuglog(DEBUG_COMMON, "%%policy:getSlotStatus success")
        except AttributeError as e:
            fanerror(str(e))
        return 0

    def fanctrol(self):  # Fan speed regulation
        openloop_pwm = MONITOR_CONST.MIN_SPEED
        try:
            if self.preIntemp <= -1000:  # First open loop speed regulation
                openloop_pwm = self.policySpeed(self.intemp)
                self.pre_openloop_pwm = openloop_pwm
                fanwarningdebuglog(DEBUG_FANCONTROL, "openloop_pwm = 0x%x" % openloop_pwm)
                return openloop_pwm

            fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:Previous temperature[%.2f] , Current temperature[%.2f]" % (self.preIntemp, self.intemp))
            if self.intemp < MONITOR_CONST.TEMP_MIN:
                fanwarningdebuglog(
                    DEBUG_FANCONTROL, "%%policy:Air inlet  %.2f  Minimum temperature: %.2f" %
                    (self.intemp, MONITOR_CONST.TEMP_MIN))
                openloop_pwm = MONITOR_CONST.MIN_SPEED  # Default level
            elif self.intemp >= MONITOR_CONST.TEMP_MIN and self.intemp > self.preIntemp:
                fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:Temperature rise")
                openloop_pwm = self.policySpeed(self.intemp)
            elif self.intemp >= MONITOR_CONST.TEMP_MIN and (self.preIntemp - self.intemp) > MONITOR_CONST.MONITOR_FALL_TEMP:
                fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:Temperature drop %d Degree or above" % MONITOR_CONST.MONITOR_FALL_TEMP)
                openloop_pwm = self.policySpeed(self.intemp)
            else:
                openloop_pwm = self.pre_openloop_pwm
                fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:Don't make any changes")
        except Exception as e:
            fanerror("%%policy: fancontrol error")
            fanerror(str(e))
        self.pre_openloop_pwm = openloop_pwm
        fanwarningdebuglog(DEBUG_FANCONTROL, "openloop_pwm = 0x%x" % openloop_pwm)
        return openloop_pwm

    def pidmethod(self):  # pid algorithm
        # sff
        pid_value = MONITOR_CONST.MIN_SPEED
        self._sfftemp = int(self._sfftemp)  # make sure temp is int
        if self._sfftemp == self.invalid_temp:  # temp is invalid
            temp = None
            self.pid.cacl(self.__pwm, "SFF_TEMP", temp)  # temp is invalid. Only the PID algorithm records the invalid temperature
            pid_value = MONITOR_CONST.MIN_SPEED
            fanwarningdebuglog(DEBUG_FANCONTROL, "sff_temp is invalid, pid_value = 0x%x" % pid_value)
            fanwarningdebuglog(DEBUG_FANCONTROL, "sfftemp = %d, invalid_temp = %d" % (self._sfftemp, self.invalid_temp))
        elif self._sfftemp == self.error_temp:  # temp is error
            temp = None
            self.pid.cacl(self.__pwm, "SFF_TEMP", temp)  # The temp error only causes the PID algorithm to record invalid temperature
            pid_value = MONITOR_CONST.MIN_SPEED
            fanwarningdebuglog(DEBUG_FANCONTROL, "sff_temp is error, pid_value = 0x%x" % pid_value)
            fanwarningdebuglog(DEBUG_FANCONTROL, "sfftemp = %d, error_temp = %d" % (self._sfftemp, self.error_temp))
        else:
            pid_value = self.pid.cacl(self.__pwm, "SFF_TEMP", self._sfftemp)  # self.__pwm is result of the last speed adjustment
        if pid_value is None:
            pid_value = self.__min_pwm
        return pid_value

    # Start speed control
    def startFanCtrol(self):
        pwm_list = []
        pwm_min = MONITOR_CONST.MIN_SPEED
        pwm_list.append(pwm_min)
        pwm_max = MONITOR_CONST.MAX_SPEED

        if MONITOR_CONST.MONITOR_PID_FLAG == 1:
            pid_value = self.pidmethod()
            if pid_value is None:
                pid_value = pwm_min
            pwm_list.append(pid_value)

        openloop_value = self.fanctrol()
        if openloop_value is None:
            openloop_value = pwm_min
        pwm_list.append(openloop_value)

        over_crit_pwm = self.checkCrit()
        pwm_list.append(over_crit_pwm)

        if self.checkWarning() == True:
            over_warning_pwm = pwm_max
            pwm_list.append(over_warning_pwm)

        if self.fanStatusCheck() == False:
            fan_error_pwm = pwm_max
            pwm_list.append(fan_error_pwm)

        if self.checkDevError() == True:
            check_dev_error_pwm = MONITOR_CONST.MAC_ERROR_SPEED
            pwm_list.append(check_dev_error_pwm)

        self.__pwm = max(pwm_list)
        fanwarningdebuglog(DEBUG_FANCONTROL, "__pwm = 0x%x\n" % self.__pwm)
        self.fanSpeedSet(self.__pwm)

        fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:Complete speed control:0x%x" % self.__pwm)

    def policySpeed(self, temp):  # Fan speed control strategy
        fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:Speed regulation formula speed regulation")
        speed_level = MONITOR_CONST.MIN_SPEED + MONITOR_CONST.K * (temp - MONITOR_CONST.TEMP_MIN)
        self.preIntemp = self.intemp
        return math.ceil(speed_level)

    def getBoardMonitorMsg(self, ledcontrol=False):
        ret_t = 0
        try:
            ret_t += self.getFanStatus()  # Get the status of the fans and get the number of fans OK
            ret_t += self.getTempStatus()  # Obtain the inlet, outlet, hot spot temperature,CPU temperature
            ret_t += self.getMacStatus()  # Get MAC maximum mild average temperature
            if ledcontrol == True:
                ret_t += self.getSlotStatus()  # Gets the subcard status
                ret_t += self.getPsuStatus()  # Obtain OK number of power supplies
            if ret_t == 0:
                return True
        except Exception as e:
            fanerror(str(e))
        return False

    # Device failure strategy    Tmac-Tin >= 50 or Tmac-Tin<=-50 
    def checkDevError(self):
        try:
            if (self.mac_aver - self.intemp) >= MONITOR_CONST.MAC_UP_TEMP or (self.mac_aver -
                                                                              self.intemp) <= MONITOR_CONST.MAC_LOWER_TEMP:
                fanwarningdebuglog(DEBUG_FANCONTROL, "%%DEV_MONITOR-TEMP: MAC temp get failed.")
                return True
        except Exception as e:
            fanerror("%%policy:checkDevError failed")
            fanerror(str(e))
        return False

    def checkTempWarning(self):
        u'''Check whether a general alarm is generated'''
        try:
            if self._mac_aver >= MONITOR_CONST.MAC_WARNING_THRESHOLD \
                    or self._outtemp >= MONITOR_CONST.OUTTEMP_WARNING_THRESHOLD \
                    or self._boardtemp >= MONITOR_CONST.BOARDTEMP_WARNING_THRESHOLD \
                    or self._cputemp >= MONITOR_CONST.CPUTEMP_WARNING_THRESHOLD \
                    or self._intemp >= MONITOR_CONST.INTEMP_WARNING_THRESHOLD:
                fanwarningdebuglog(DEBUG_COMMON, "The temperature exceeds the minor alarm threshold")
                return True
        except Exception as e:
            fanerror("%%policy: checkTempWarning failed")
            fanerror(str(e))
        return False

    def checkTempCrit(self):
        u'''Check whether a major alarm is generated'''
        try:
            if self._mac_aver >= MONITOR_CONST.MAC_CRITICAL_THRESHOLD \
                or (self._outtemp >= MONITOR_CONST.OUTTEMP_CRITICAL_THRESHOLD
                    and self._boardtemp >= MONITOR_CONST.BOARDTEMP_CRITICAL_THRESHOLD
                    and self._cputemp >= MONITOR_CONST.CPUTEMP_CRITICAL_THRESHOLD
                    and self._intemp >= MONITOR_CONST.INTEMP_CRITICAL_THRESHOLD):
                fanwarningdebuglog(DEBUG_COMMON, "The temperature exceeds the major alarm threshold")
                return True
        except Exception as e:
            fanerror("%%policy: checkTempCrit failed")
            fanerror(str(e))
        return False

    def checkFanStatus(self):
        u'''Checking fan status'''
        for item in MONITOR_FAN_STATUS:
            maxoknum = item.get('maxOkNum')
            minoknum = item.get('minOkNum')
            status = item.get('status')
            if self.fanOKNum >= minoknum and self.fanOKNum <= maxoknum:
                fanwarningdebuglog(DEBUG_COMMON, "checkFanStatus:fanOKNum:%d,status:%s" % (self.fanOKNum, status))
                return status
        fanwarningdebuglog(DEBUG_COMMON, "checkFanStatus Error:fanOKNum:%d" % (self.fanOKNum))
        return None

    def checkPsuStatus(self):
        u'''Check power supply status'''
        for item in MONITOR_PSU_STATUS:
            maxoknum = item.get('maxOkNum')
            minoknum = item.get('minOkNum')
            status = item.get('status')
            if self.psuOKNum >= minoknum and self.psuOKNum <= maxoknum:
                fanwarningdebuglog(DEBUG_COMMON, "checkPsuStatus:psuOKNum:%d,status:%s" % (self.psuOKNum, status))
                return status
        fanwarningdebuglog(DEBUG_COMMON, "checkPsuStatus Error:psuOKNum:%d" % (self.psuOKNum))
        return None

    def dealSysLedStatus(self):
        u'''Set SYSLED based on temperature, fan and power status'''
        try:
            fanstatus = self.checkFanStatus()
            psustatus = self.checkPsuStatus()
            if self.checkTempCrit() == True or fanstatus == "red" or psustatus == "red":
                status = "red"
            elif self.checkTempWarning() == True or fanstatus == "yellow" or psustatus == "yellow":
                status = "yellow"
            else:
                status = "green"
            self.setSysLed(status)
            fanwarningdebuglog(DEBUG_LEDCONTROL, "%%ledcontrol:dealSysLedStatus success, status:%s," % status)
        except Exception as e:
            fanerror(str(e))

    def dealSysFanLedStatus(self):
        u'''According to the status point panel fan light'''
        try:
            status = self.checkFanStatus()
            if status is not None:
                self.setSysFanLed(status)
                fanwarningdebuglog(DEBUG_LEDCONTROL, "%%ledcontrol:dealSysFanLedStatus success, status:%s," % status)
        except Exception as e:
            fanerror("%%ledcontrol:dealSysLedStatus error")
            fanerror(str(e))

    def dealPsuLedStatus(self):
        u'''Set the PSU-LED based on the power supply status'''
        try:
            status = self.checkPsuStatus()
            if status is not None:
                self.setSysPsuLed(status)
            fanwarningdebuglog(DEBUG_LEDCONTROL, "%%ledcontrol:dealPsuLedStatus success, status:%s," % status)
        except Exception as e:
            fanerror("%%ledcontrol:dealPsuLedStatus error")
            fanerror(str(e))

    def dealLocFanLedStatus(self):
        u'''Click the fan indicator based on the fan status'''
        for item in MONITOR_FANS_LED:
            try:
                index = MONITOR_FANS_LED.index(item) + 1
                fanattr = "fan%dstatus" % index
                val_t = getattr(FanControl, fanattr, None)
                if val_t == "NOT OK":
                    rji2cset(item["bus"], item["devno"], item["addr"], item["red"])
                elif val_t == "OK":
                    rji2cset(item["bus"], item["devno"], item["addr"], item["green"])
                else:
                    pass
                fanwarningdebuglog(
                    DEBUG_LEDCONTROL, "%%ledcontrol:dealLocFanLed success.fanattr:%s, status:%s" %
                    (fanattr, val_t))
            except Exception as e:
                fanerror("%%ledcontrol:dealLocFanLedStatus error")
                fanerror(str(e))

    def dealSlotLedStatus(self):
        u'''The sub-card status indicator is based on the sub-card status'''
        slotLedList = DEV_LEDS.get("SLOTLED", [])
        slots_led_cfg = DEV_LEDS.get("slots_led_cfg", [])
        for item in slotLedList:
            try:
                index = slotLedList.index(item) + 1
                slotattr = "slot%dstatus" % index
                val_t = getattr(FanControl, slotattr, None)
                color = slots_led_cfg.get(val_t, "red")
                self.setled(item, color)
                fanwarningdebuglog(
                    DEBUG_LEDCONTROL, "%%ledcontrol:dealSlotLedStatus success.slotattr:%s, status:%s" %
                    (slotattr, val_t))
            except Exception as e:
                fanerror("%%ledcontrol:dealSlotLedStatus error")
                fanerror(str(e))

    def dealBmcLedstatus(self, val):
        pass

    def dealLctLedstatus(self, val):
        pass

    def setled(self, item, color):
        if item.get('type', 'i2c') == 'sysfs':
            rjsysset(item["cmdstr"], item.get(color))
        else:
            mask = item.get('mask', 0xff)
            ind, val = rji2cget(item["bus"], item["devno"], item["addr"])
            if ind == True:
                setval = (int(val, 16) & ~mask) | item.get(color)
                rji2cset(item["bus"], item["devno"], item["addr"], setval)
            else:
                fanwarningdebuglog(DEBUG_LEDCONTROL, "led %s" % "i2c read failure")  # The line card is absent, the light register will fail to read

    def setSysLed(self, color):
        for item in MONITOR_SYS_LED:
            self.setled(item, color)

    def setSysFanLed(self, color):
        for item in MONITOR_SYS_FAN_LED:
            self.setled(item, color)

    def setSysPsuLed(self, color):
        for item in MONITOR_SYS_PSU_LED:
            self.setled(item, color)

    def checkWarning(self):
        try:
            if self.checkTempWarning() == True:
                fanwarningdebuglog(DEBUG_FANCONTROL, "Anti-shake start")
                time.sleep(MONITOR_CONST.SHAKE_TIME)
                fanwarningdebuglog(DEBUG_FANCONTROL, "Anti-shake stop")
                self.getBoardMonitorMsg()  # Read it again
                if self.checkTempWarning() == True:
                    fanwarninglog("%%DEV_MONITOR-TEMP:The temperature of device is over warning value.")
                    return True
        except Exception as e:
            fanerror("%%policy: checkWarning failed")
            fanerror(str(e))
        return False

    def checkCrit(self):
        over_crit_pwm = MONITOR_CONST.MIN_SPEED
        reboot_flag = False
        try:
            if self.checkTempCrit() == True:
                over_crit_pwm = MONITOR_CONST.MAX_SPEED
                self.fanSpeedSet(over_crit_pwm)
                for i in range(MONITOR_CONST.CRITICAL_NUM):
                    time.sleep(MONITOR_CONST.SHAKE_TIME)
                    self.getBoardMonitorMsg()  # Read it again
                    if self.checkTempCrit() == True:
                        fancriticallog("%%DEV_MONITOR-TEMP:The temperature of device is over reboot critical value.")
                        reboot_flag = True
                        continue
                    else:
                        fancriticallog("%%DEV_MONITOR-TEMP:The temperature of device is not over reboot critical value.")
                        reboot_flag = False
                        break
                if reboot_flag is True:
                    reboot_log = "The temperature of device is over critical value."
                    reboot_log_cmd = "echo '%s' > /dev/ttyS0" % reboot_log
                    fancriticallog(reboot_log)
                    exec_os_cmd(reboot_log_cmd)
                    reboot_log = "The system is going to reboot now."
                    reboot_log_cmd = "echo '%s' > /dev/ttyS0" % reboot_log
                    fancriticallog(reboot_log)
                    exec_os_cmd(reboot_log_cmd)
                    fancriticallog("self._intemp = %d" % self._intemp)
                    fancriticallog("self._mac_aver = %d" % self._mac_aver)
                    fancriticallog("self._mac_max = %d" % self._mac_max)
                    fancriticallog("self._outtemp = %d" % self._outtemp)
                    fancriticallog("self._boardtemp = %d" % self._boardtemp)
                    fancriticallog("self._cputemp = %d" % self._cputemp)
                    create_judge_file = "touch %s" % OTP_REBOOT_JUDGE_FILE
                    exec_os_cmd(create_judge_file)
                    exec_os_cmd("sync")
                    time.sleep(3)
                    os.system("/sbin/reboot")
        except Exception as e:
            fanerror("%%policy: checkCrit failed")
            fanerror(str(e))
        return over_crit_pwm


def callback():
    pass


def doFanCtrol(fanCtrol):
    ret = fanCtrol.getBoardMonitorMsg()
    if ret == True:
        fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:startFanCtrol")
        fanCtrol.startFanCtrol()
    else:
        fanCtrol.fanSpeedSetMax()
        fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:getBoardMonitorMsg error")


def doLedCtrol(fanCtrol):
    fanCtrol.getBoardMonitorMsg(ledcontrol=True)  # Acquisition state
    fanCtrol.dealSysLedStatus()        # Dot system light
    fanCtrol.dealSysFanLedStatus()     # Click panel fan light
    fanCtrol.dealLocFanLedStatus()     # Turn on the fan light
    fanCtrol.dealPsuLedStatus()        # Point panel PSU light
    fanCtrol.dealSlotLedStatus()       # Idea card status indicator
    fanwarningdebuglog(DEBUG_LEDCONTROL, "%%ledcontrol:doLedCtrol success")


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def run(interval, fanCtrol):
    loop = MONITOR_CONST.MONITOR_INTERVAL
    # waitForDocker()
    while True:
        try:
            debug_init()
            if loop >= MONITOR_CONST.MONITOR_INTERVAL:  # Fan speed regulation
                loop = 0
                fanwarningdebuglog(DEBUG_FANCONTROL, "%%policy:fanCtrol")
                doFanCtrol(fanCtrol)
            else:
                fanwarningdebuglog(DEBUG_LEDCONTROL, "%%ledcontrol:start ledctrol")  # LED control
                doLedCtrol(fanCtrol)
            time.sleep(interval)
            loop += interval
        except Exception as e:
            traceback.print_exc()
            fanerror(str(e))


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass


@main.command()
def start():
    '''start fan control'''
    faninfo("FANCTROL start")
    fanCtrol = FanControl()
    interval = MONITOR_CONST.MONITOR_LED_INTERVAL
    run(interval, fanCtrol)


@main.command()
def stop():
    '''stop fan control '''
    faninfo("stop")


# device_i2c operation
if __name__ == '__main__':
    debug_init()
    main()
