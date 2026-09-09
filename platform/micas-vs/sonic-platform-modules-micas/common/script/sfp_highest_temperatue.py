#!/usr/bin/python3
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

import os
import importlib.machinery
import time
import syslog
import subprocess
import fcntl

sfp_temperature_file = "/tmp/highest_sff_temp"

SFP_TEMP_DEBUG_FILE = "/etc/.sfp_temp_debug_flag"
SFP_TEMP_RECORD_DEBUG = 1
SFP_TEMP_RECORD_ERROR = 2
debuglevel = 0

# For TH5 CPO only 
cpo_temperature_oe_file = "/etc/sonic/highest_oe_temp"
cpo_temperature_rlm_file = "/etc/sonic/highest_rlm_temp"
cpo_onie_platform = "x86_64-micas_m2-w6940-128x1-fr4-r0"

def sfp_temp_debug(s):
    if SFP_TEMP_RECORD_DEBUG & debuglevel:
        syslog.openlog("SFP_TEMP_DEBUG", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def sfp_temp_error(s):
    if SFP_TEMP_RECORD_ERROR & debuglevel:
        syslog.openlog("SFP_TEMP_ERROR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)

def file_rw_lock(file_path):
    pidfile = open(file_path, "r")
    try:
        fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
        sfp_temp_debug("file lock success")
        return True, pidfile
    except Exception:
        if pidfile is not None:
            pidfile.close()
            pidfile = None
        return False, pidfile


def file_rw_unlock(pidfile):
    try:
        if pidfile is not None:
            fcntl.flock(pidfile, fcntl.LOCK_UN)
            pidfile.close()
            pidfile = None
            sfp_temp_debug("file unlock success")
        else:
            sfp_temp_debug("pidfile is invalid, do nothing")
        return True
    except Exception as e:
        sfp_temp_error("file unlock err, msg:%s" % (str(e)))
        return False


def get_sfp_highest_temperature():
    highest_temperature = 0
    platform_sfputil = None

    sfputil_dir = "/usr/share/sonic/device/"
    try:
        if not os.path.exists(sfputil_dir):
            sfputil_dir = "/usr/share/sonic/platform/"
            sfputil_path = sfputil_dir + "/plugins/sfputil.py"
        else:
            cmd = "cat /host/machine.conf | grep onie_platform"
            ret, output = subprocess.getstatusoutput(cmd)
            if ret != 0:
                sfp_temp_error("cmd: %s execution fail, output: %s" % (cmd, output))

            onie_platform = output.split("=")[1]
            sfputil_path = sfputil_dir + onie_platform + "/plugins/sfputil.py"

        module = importlib.machinery.SourceFileLoader("sfputil", sfputil_path).load_module()
        platform_sfputil_class = getattr(module, "SfpUtil")
        platform_sfputil = platform_sfputil_class()

        temperature = platform_sfputil.get_highest_temperature()
        highest_temperature = int(temperature) * 1000
    except Exception as e:
        sfp_temp_error("get sfp temperature error, msg:%s" % str(e))
        highest_temperature = -9999000      # fix me in future, should be -99999000

    return highest_temperature


def write_sfp_highest_temperature(temperature, path):

    loop = 1000
    ret = False
    pidfile = None
    try:
        if os.path.exists(path) is False:
            with open(path, 'w') as sfp_f:
                pass
        for i in range(0, loop):
            ret, pidfile = file_rw_lock(path)
            if ret is True:
                break
            time.sleep(0.001)

        if ret is False:
            sfp_temp_error("take file lock timeout")
            return

        with open(path, 'w') as sfp_f:
            sfp_f.write("%s\n" % str(temperature))

        file_rw_unlock(pidfile)
        return
    except Exception as e:
        sfp_temp_error("write sfp temperature error, msg:%s" % str(e))
        file_rw_unlock(pidfile)
        return

def get_cpo_highest_temperature():

    oe_temp = get_cpo_highest_temperature_oe()
    rlm_temp = get_cpo_highest_temperature_rlm()

    return oe_temp, rlm_temp

def get_cpo_highest_temperature_oe():
    highest_temperature = 0
    platform_sfputil = None
    sfputil_dir = "/usr/share/sonic/device/"
    try:
        if not os.path.exists(sfputil_dir):
            sfputil_dir = "/usr/share/sonic/platform/"
            sfputil_path = sfputil_dir + "/plugins/sfputil.py"
        else:
            cmd = "cat /host/machine.conf | grep onie_platform"
            ret, output = subprocess.getstatusoutput(cmd)
            if ret != 0:
                sfp_temp_error("cmd: %s execution fail, output: %s" % (cmd, output))

            onie_platform = output.split("=")[1]
            sfputil_path = sfputil_dir + onie_platform + "/plugins/sfputil.py"
        module = importlib.machinery.SourceFileLoader("sfputil", sfputil_path).load_module()
        platform_sfputil_class = getattr(module, "SfpUtil")
        platform_sfputil = platform_sfputil_class()

        temperature = platform_sfputil.get_highest_temperature_cpo_oe()
        highest_temperature = int(temperature) * 1000
    except Exception as e:
        sfp_temp_error("get sfp temperature error, msg:%s" % str(e))
        highest_temperature = -99999000

    return highest_temperature

def get_cpo_highest_temperature_rlm():
    highest_temperature = 0
    platform_sfputil = None
    sfputil_dir = "/usr/share/sonic/device/"
    try:
        if not os.path.exists(sfputil_dir):
            sfputil_dir = "/usr/share/sonic/platform/"
            sfputil_path = sfputil_dir + "/plugins/sfputil.py"
        else:
            cmd = "cat /host/machine.conf | grep onie_platform"
            ret, output = subprocess.getstatusoutput(cmd)
            if ret != 0:
                sfp_temp_error("cmd: %s execution fail, output: %s" % (cmd, output))

            onie_platform = output.split("=")[1]
            sfputil_path = sfputil_dir + onie_platform + "/plugins/sfputil.py"

        module = importlib.machinery.SourceFileLoader("sfputil", sfputil_path).load_module()
        platform_sfputil_class = getattr(module, "SfpUtil")
        platform_sfputil = platform_sfputil_class()

        temperature = platform_sfputil.get_highest_temperature_cpo_rlm()
        highest_temperature = int(temperature) * 1000
    except Exception as e:
        sfp_temp_error("get sfp temperature error, msg:%s" % str(e))
        highest_temperature = -99999000

    return highest_temperature

def is_th5_cpo():
    cmd = "cat /host/machine.conf | grep onie_platform"
    ret, output = subprocess.getstatusoutput(cmd)
    if ret != 0:
        sfp_temp_error("cmd: %s execution fail, output: %s" % (cmd, output))
    if output.split("=")[1] == cpo_onie_platform:
        return True

    return False

def debug_init():
    global debuglevel

    try:
        with open(SFP_TEMP_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0


def main():
    if is_th5_cpo():
        while True:
            debug_init()
            temperature_oe = 0
            try:
                temperature_oe = get_cpo_highest_temperature_oe()
                write_sfp_highest_temperature(temperature_oe, cpo_temperature_oe_file)
            except Exception as e:
                sfp_temp_error("get/write sfp temperature error, msg:%s" % str(e))
                write_sfp_highest_temperature(-99999000, cpo_temperature_oe_file)
            temperature_rlm = 0
            try:
                temperature_rlm = get_cpo_highest_temperature_rlm()
                write_sfp_highest_temperature(temperature_rlm, cpo_temperature_rlm_file)
            except Exception as e:
                sfp_temp_error("get/write sfp temperature error, msg:%s" % str(e))
                write_sfp_highest_temperature(-99999000, cpo_temperature_rlm_file)
            time.sleep(5)
    else:
        while True:
            debug_init()
            temperature = 0
            try:
                temperature = get_sfp_highest_temperature()
                write_sfp_highest_temperature(temperature, sfp_temperature_file)
            except Exception as e:
                sfp_temp_error("get/write sfp temperature error, msg:%s" % str(e))
                write_sfp_highest_temperature(-9999000, sfp_temperature_file)
            time.sleep(5)


if __name__ == '__main__':
    main()
