#!/usr/bin/python3
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


def sfp_temp_debug(s):
    if SFP_TEMP_RECORD_DEBUG & debuglevel:
        syslog.openlog("SFP_TEMP_DEBUG", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def sfp_temp_error(s):
    if SFP_TEMP_RECORD_ERROR & debuglevel:
        syslog.openlog("SFP_TEMP_ERROR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


pidfile = None


def file_rw_lock():
    global pidfile
    pidfile = open(sfp_temperature_file, "r")
    try:
        fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
        sfp_temp_debug("file lock success")
        return True
    except Exception:
        if pidfile is not None:
            pidfile.close()
            pidfile = None
        return False


def file_rw_unlock():
    try:
        global pidfile

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
            cmd = "cat /host/machine.conf | grep onie_build_platform"
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
        highest_temperature = -9999000

    return highest_temperature


def write_sfp_highest_temperature(temperature):

    loop = 1000
    ret = False
    try:
        if os.path.exists(sfp_temperature_file) is False:
            with open(sfp_temperature_file, 'w') as sfp_f:
                pass
        for i in range(0, loop):
            ret = file_rw_lock()
            if ret is True:
                break
            time.sleep(0.001)

        if ret is False:
            sfp_temp_error("take file lock timeout")
            return

        with open(sfp_temperature_file, 'w') as sfp_f:
            sfp_f.write("%s\n" % str(temperature))

        file_rw_unlock()
        return
    except Exception as e:
        sfp_temp_error("write sfp temperature error, msg:%s" % str(e))
        file_rw_unlock()
        return


def debug_init():
    global debuglevel

    try:
        with open(SFP_TEMP_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0


def main():
    while True:
        debug_init()
        temperature = 0
        try:
            temperature = get_sfp_highest_temperature()
            write_sfp_highest_temperature(temperature)
        except Exception as e:
            sfp_temp_error("get/write sfp temperature error, msg:%s" % str(e))
            write_sfp_highest_temperature(-9999000)
        time.sleep(5)


if __name__ == '__main__':
    main()
