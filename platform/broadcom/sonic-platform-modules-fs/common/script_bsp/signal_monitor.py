#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import time
from datetime import datetime
import syslog
import math
import click
import fcntl
import logging
from platform_config import *
from platform_util import setup_logger, BSP_COMMON_LOG_DIR

CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

DEBUG_FILE = "/etc/signal_record/.debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "signal_monitor_debug.log"
logger = setup_logger(LOG_FILE)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

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


def signal_debug(s):
    logger.debug(s)

def signal_error(s):
    logger.error(s)

def signal_syslog_warning(s):
    syslog.openlog("SIGNAL_MONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, s)
    logger.warning(s)

def signal_syslog_info(s):
    syslog.openlog("SIGNAL_MONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_INFO, s)
    logger.info(s)

def signal_syslog_error(s):
    syslog.openlog("SIGNAL_MONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)
    logger.error(s)

def debug_init():
    global debuglevel

    try:
        with open(SIGNAL_RECORD_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception as e:
        debuglevel = 0
    return


def dev_file_write(path, offset, buf):
    msg = ""
    value = ""
    fd = -1

    if not os.path.exists(path):
        msg = path + " not found !"
        return False, msg

    signal_debug("dev_file_write path:%s, offset:0x%x, value: %s" % (path, offset, buf))

    try:
        fd = os.open(path, os.O_WRONLY)
        os.lseek(fd, offset, os.SEEK_SET)
        ret = os.write(fd, bytes(buf))
    except Exception as e:
        msg = str(e)
        return False, msg
    finally:
        if fd > 0:
            os.close(fd)

    return True, ret


def dev_file_read(path, offset, len):
    value = []
    msg = ""
    ret = ""
    fd = -1

    if not os.path.exists(path):
        msg = path + " not found !"
        return False, msg

    try:
        fd = os.open(path, os.O_RDONLY)
        os.lseek(fd, offset, os.SEEK_SET)
        ret = os.read(fd, len)
        for item in ret:
            value.append(item)
    except Exception as e:
        msg = str(e)
        return False, msg
    finally:
        if fd > 0:
            os.close(fd)

    signal_debug("dev_file_read path:%s, offset:0x%x, len:%d, value: %s" % (path, offset, len, value))
    return True, value


def timestr_convert_to_unix_ts(timestr):
    try:
        timestr_obj = datetime.strptime(timestr, "%Y-%m-%d %H:%M:%S.%f")
        unix_ts = int(time.mktime(timestr_obj.timetuple()) * 1000.0 + timestr_obj.microsecond / 1000.0)
        signal_debug("timestr:%s convert to unix timestamp %d success." % (timestr, unix_ts))
    except Exception as e:
        msg = str(e)
        return False, msg
    return True, unix_ts


pidfile = 0


def ApplicationInstance():
    global pidfile
    pidfile = open(os.path.realpath(__file__), "r")
    try:
        fcntl.flock(pidfile, fcntl.LOCK_EX | fcntl.LOCK_NB)  # Create an exclusive lock that does not block other processes that are locked
    except BaseException:
        signal_syslog_warning("%SIGNAL_MONITOR-4-CONFLICT: signal monitor already running.")
        exit(1)


class SignalRecord(object):

    def __init__(self, config):
        self.__dev_name = config["name"]                          # Signal recording device name
        self.__config_dev = config["config_dev"]                  # Configuration logic device
        self.__record_dev = config["record_dev"]                  # Recording logic device
        self.__record_file_path = config["record_file"]           # Number of software records File path
        self.__signal_conf = config["signal_conf"]                # Signal configuration list
        self.__per_record_len = config.get("per_record_len", 16)  # The length of each record
        self.__max_record_num = config.get("max_record_num", 512)  # Maximum number of records supported
        # Configure register parameter
        self.__unix_ts_set_reg = config.get("unix_ts_set_reg", 0xf1)  # Unix timestamp setting register
        self.__unix_ts_len = config.get("unix_ts_len", 5)            # unix timestamp Length 5 bytes (40bit)
        self.__time_set_reg = config.get("time_set_reg", 0xf6)       # Unix timestamp configuration effective register
        self.__ufm_op_reg = config.get("ufm_op_reg", 0xf7)           # UFM operation register
        self.__record_cnt_reg = config.get("record_cnt_reg", 0xf8)   # Count register
        # Record register parameter
        self.__record_mask_reg = config.get("record_mask_reg", 0x00)   # Flag bit register
        self.__record_flag = config.get("record_flag", 0xa5)           # The exception record flag bit is written as 0XA5, not 0
        self.__record_ts_reg = config.get("record_ts_reg", 0x01)       # Record timestamp
        self.__time_base = config.get("time_base", "2020-01-01 00:00:00.000")
        self.__earse_threshold = config.get("earse_threshold", 500)
        self.__update_unix_ts_cnt = config.get("update_unix_ts_cnt", 60)  # The default poll is 60 updates to the UNIX timestamp
        self.__unix_ts_base = 0    # base time Indicates the unix time stamp

    @property
    def dev_name(self):
        return self.__dev_name

    @property
    def config_dev(self):
        return self.__config_dev

    @property
    def record_dev(self):
        return self.__record_dev

    @property
    def record_file_path(self):
        return self.__record_file_path

    @property
    def signal_conf(self):
        return self.__signal_conf

    @property
    def per_record_len(self):
        return self.__per_record_len

    @property
    def max_record_num(self):
        return self.__max_record_num

    @property
    def unix_ts_set_reg(self):
        return self.__unix_ts_set_reg

    @property
    def time_set_reg(self):
        return self.__time_set_reg

    @property
    def ufm_op_reg(self):
        return self.__ufm_op_reg

    @property
    def record_cnt_reg(self):
        return self.__record_cnt_reg

    @property
    def record_mask_reg(self):
        return self.__record_mask_reg

    @property
    def record_flag(self):
        return self.__record_flag

    @property
    def record_ts_reg(self):
        return self.__record_ts_reg

    @property
    def unix_ts_len(self):
        return self.__unix_ts_len

    @property
    def earse_threshold(self):
        if self.__earse_threshold > self.__max_record_num:
            return self.__max_record_num
        return self.__earse_threshold

    @property
    def update_unix_ts_cnt(self):
        return self.__update_unix_ts_cnt

    @property
    def time_base(self):
        return self.__time_base

    @property
    def unix_ts_base(self):
        if self.__unix_ts_base == 0:
            status, value = timestr_convert_to_unix_ts(self.time_base)
            if status is True:
                self.__unix_ts_base = value
            else:
                signal_error("%s get unix_ts_base failed, msg:%s" % (self.dev_name, value))
        return self.__unix_ts_base

    @property
    def dev_record_cnt(self):
        # Read the register to get the number of records
        status, value = dev_file_read(self.config_dev, self.record_cnt_reg, 2)
        if status is True:
            record_value = ((value[0] << 8) | value[1]) & 0xffff
            if record_value > self.max_record_num:
                signal_error("%s invalid device record number %d" % (self.dev_name, record_value))
                return -1
            return record_value
        signal_error("%s get dev_record_cnt failed, msg:%s" % (self.dev_name, value))
        return -1

    @property
    def file_record_cnt(self):
        # Number of records obtained from reading a file
        if not os.path.isfile(self.record_file_path):
            signal_error("%s not found" % self.record_file_path)
            return -1
        try:
            with open(self.record_file_path, "r") as fd:
                value = fd.read()
            file_record_value = int(value)
            return file_record_value
        except Exception as e:
            signal_error("get %s value error: %s" % (self.record_file_path, str(e)))
        return -1

    @file_record_cnt.setter
    def file_record_cnt(self, value):
        if not isinstance(value, int):
            signal_error("%s set file_record_cnt error, value type is %s" % (self.dev_name, type(value)))
            raise ValueError("set file_record_cnt error, value must be an integer!")

        if value < 0 or value > self.max_record_num:
            signal_error("%s set file_record_cnt error, invalid value: %d" % (self.dev_name, value))
            raise ValueError("set file_record_cnt error, invalid value: %d" % value)

        dir_path = os.path.dirname(self.record_file_path)
        if not os.path.exists(dir_path):
            os.makedirs(dir_path)
        with open(self.record_file_path, "w") as fd:
            fd.write(str(value))
        signal_debug("set %s file_record_cnt %d success" % (self.record_file_path, value))

    @property
    def time_set(self):
        return 0

    @time_set.setter
    def time_set(self, value):
        buf = []

        if not isinstance(value, int):
            signal_error("%s time_set error, value type is %s" % (self.dev_name, type(value)))
            raise ValueError("time_set error, value must be an integer!")

        if value != 1:
            signal_error("%s time_set error, invalid value %s" % (self.dev_name, value))
            raise ValueError("time_set error, invalid value %s" % value)

        buf.append(value)
        status, msg = dev_file_write(self.config_dev, self.time_set_reg, buf)
        if status is False:
            signal_error("%s time_set error: %s" % (self.dev_name, msg))
            raise Exception("time_set error: %s" % msg)
        signal_debug("%s time_set success" % self.dev_name)

    @property
    def ufm_op_status(self):
        # Read UFM status 0: idle 1:loading/earsing, failure-1
        status, value = dev_file_read(self.config_dev, self.ufm_op_reg, 1)
        if status is True:
            return (value[0] & 0x01)
        signal_error("%s get ufm_op_status failed, msg:%s" % (self.dev_name, value))
        return -1

    @property
    def record_start_flag(self):
        # Check whether the UFM key signal recording function is enabled. 1: The UFM key signal recording function is enabled.
        # 0: The UFM key signal recording function is disabled
        status, value = dev_file_read(self.config_dev, self.ufm_op_reg, 1)
        if status is True:
            return ((value[0] >> 3) & 0x01)
        signal_error("%s get ufm_op_status failed, msg:%s" % (self.dev_name, value))
        return -1

    @property
    def ufm_load(self):
        return 0

    @ufm_load.setter
    def ufm_load(self, value):
        buf = []

        if not isinstance(value, int):
            signal_error("%s ufm_load error, value type is %s" % (self.dev_name, type(value)))
            raise ValueError("ufm_load error, value must be an integer!")

        if value != 1:
            signal_error("%s ufm_load error, invalid value %s" % (self.dev_name, value))
            raise ValueError("ufm_load error, invalid value %s" % value)

        value = value << 1
        buf.append(value)
        status, msg = dev_file_write(self.config_dev, self.ufm_op_reg, buf)
        if status is False:
            signal_error("%s ufm_load error: %s" % (self.dev_name, msg))
            raise Exception("ufm_load error: %s" % msg)
        signal_debug("%s ufm_load success" % self.dev_name)

    @property
    def ufm_earse(self):
        return 0

    @ufm_earse.setter
    def ufm_earse(self, value):
        buf = []

        if not isinstance(value, int):
            signal_error("%s ufm_earse error, value type is %s" % (self.dev_name, type(value)))
            raise ValueError("ufm_earse error, value must be an integer!")

        if value != 1:
            signal_error("%s ufm_earse error, invalid value %s" % (self.dev_name, value))
            raise ValueError("ufm_earse error, invalid value %s" % value)

        value = value << 2
        buf.append(value)
        status, msg = dev_file_write(self.config_dev, self.ufm_op_reg, buf)
        if status is False:
            signal_error("%s ufm_earse error: %s" % (self.dev_name, msg))
            raise Exception("ufm_earse error: %s" % msg)
        signal_debug("%s ufm_earse success" % self.dev_name)

    @property
    def config_unix_ts(self):
        # Read Unix timestamp configuration values
        # Return Unix timestamp
        val_tmp = 0
        status, value = dev_file_read(self.config_dev, self.unix_ts_set_reg, self.unix_ts_len)
        if status is True:
            for i in range(self.unix_ts_len):
                val_tmp |= value[i] << ((self.unix_ts_len - i - 1) * 8)
            return val_tmp
        signal_error("%s get config_unix_ts failed, msg:%s" % (self.dev_name, value))
        return -1

    @config_unix_ts.setter
    def config_unix_ts(self, value):
        # Write the Unix timestamp to the register
        # @value: indicates the timestamp corresponding to the ms time
        buf = []

        if not isinstance(value, int):
            signal_error("%s set config_unix_ts error, value type is %s" % (self.dev_name, type(value)))
            raise ValueError("set config_unix_ts error, value must be an integer!")

        if (value < 0) or (value >= int(math.pow(256, self.unix_ts_len))):
            signal_error("%s set config_unix_ts error, invalid value: %d" % (self.dev_name, value))
            raise ValueError("set config_unix_ts error, invalid value: %d" % value)

        for i in range(self.unix_ts_len):
            val_tmp = (value >> ((self.unix_ts_len - i - 1) * 8)) & 0xff
            buf.append(val_tmp)
        status, msg = dev_file_write(self.config_dev, self.unix_ts_set_reg, buf)
        if status is False:
            signal_error("%s config_unix_ts error: %s" % (self.dev_name, msg))
            raise Exception("config_unix_ts error: %s" % msg)
        signal_debug("%s config_unix_ts success" % self.dev_name)

    def __get_record_mask(self, record_num):
        # Read record bit 1: there is a read record. 0: there is no read record. -1: The read failed
        # @record_num starting from 1, maximum size 512
        status, value = dev_file_read(self.record_dev, self.record_mask_reg +
                                      ((record_num - 1) * self.per_record_len), 1)
        if status is True:
            if value[0] == self.record_flag:
                return 1
            return 0
        signal_error("%s get record_mask failed, msg:%s" % (self.dev_name, value))
        return -1

    def __get_record_unix_ts(self, record_num):
        # Read CPLD exception record timestamp
        # @record_num starting from 1, maximum size 512
        val_tmp = 0
        status, value = dev_file_read(self.record_dev, self.record_ts_reg +
                                      ((record_num - 1) * self.per_record_len), self.unix_ts_len)
        if status is True:
            for i in range(self.unix_ts_len):
                val_tmp |= value[i] << ((self.unix_ts_len - i - 1) * 8)
            return val_tmp
        signal_error("%s get record_unix_ts failed, msg:%s" % (self.dev_name, value))
        return -1

    def set_device_timestamp(self):
        #  Set the Unix timestamp
        msg = ""

        try:
            unix_ts_now = int(round(time.time() * 1000))
            unix_ts_set = unix_ts_now - self.unix_ts_base
            self.config_unix_ts = unix_ts_set
            self.time_set = 1  # Setting takes effect
            signal_debug("%s set_device_timestamp %d success" % (self.dev_name, unix_ts_set))
            return True, msg
        except Exception as e:
            msg = str(e)
            signal_error("%s set_device_timestamp failed, msg:%s" % (self.dev_name, msg))
        return False, msg

    def decode_device_timestamp(self, dev_unix_ts):
        # Parse the timestamp
        # unix_ts Read unix timestamp
        # Returns the "%Y-%m-%d %H:% m :%S.%f" time string
        msg = ""

        try:
            real_unix_ts = dev_unix_ts + self.unix_ts_base
            timestamp_s = real_unix_ts / 1000
            timestamp_ms = real_unix_ts % 1000
            time_array = time.localtime(timestamp_s)
            other_style_time = time.strftime("%Y-%m-%d %H:%M:%S", time_array)
            timestr = "%s.%03d" % (other_style_time, timestamp_ms)
            signal_debug("%s dev_unix_ts:%d, read_unix_ts:%d, decode_device_timestamp: %s" %
                         (self.dev_name, dev_unix_ts, real_unix_ts, timestr))
            return True, timestr
        except Exception as e:
            msg = str(e)
            signal_error("%s decode_device_timestamp failed, msg:%s" % (self.dev_name, msg))
        return False, msg

    def __key_signal_record(self, record_num):
        # Record abnormal signals and write to syslog
        # @record_num starting from 1, maximum size 512
        # 0 is returned for no records, 1 is returned for normal records, and -1 is returned for abnormal records
        errcnt = 0

        record_flag = self.__get_record_mask(record_num)
        if record_flag != 1:
            signal_debug("%s record_num:%d, nothing to record." % (self.dev_name, record_num))
            return record_flag

        dev_unix_ts = self.__get_record_unix_ts(record_num)
        if dev_unix_ts < 0:
            signal_error("%s record_num:%d, get device record unix timestamp failed." % (self.dev_name, record_num))
            return -1

        status, timestr = self.decode_device_timestamp(dev_unix_ts)
        if status is False:
            signal_error("%s record_num:%d, decode device timestamp failed: %s" % (self.dev_name, record_num, timestr))
            return -1

        for item in self.signal_conf:
            loc = item["loc"]
            offset = item["offset"]
            status, value = dev_file_read(loc, offset + ((record_num - 1) * self.per_record_len), 1)
            if status is False:
                signal_error(
                    "%s get record value failed, loc: %s, offset:0x%x, record_num:%d." %
                    (self.dev_name, loc, offset, record_num))
                errcnt -= 1
                continue
            record_value = value[0]
            signal_list = item["signal_list"]
            for signal in signal_list:
                bit = signal["bit"]
                okval = signal["okval"]
                if ((record_value >> bit) & 0x01) != okval:  # An abnormal signal was recorded
                    signal_syslog_info(
                        "%%SIGNAL_MONITOR-4-ABNORMAL: %s %s signal [%s] abnormal" %
                        (timestr, self.dev_name, signal["name"]))
        if errcnt < 0:
            return -1
        return 1

    def key_signal_check(self):
        dev_record_cnt = self.dev_record_cnt
        file_record_cnt = self.file_record_cnt
        signal_debug("%s key signal check, file_record_cnt:%d, dev_record_cnt:%d." %
                     (self.dev_name, file_record_cnt, dev_record_cnt))

        # Failed to read the file. Traversal starts from 0
        if file_record_cnt < 0:
            self.file_record_cnt = 0
            file_record_cnt = 0

         # There are new records that are not preserved
        if file_record_cnt < dev_record_cnt:
            signal_debug("%s new record occur, file_record_cnt:%d, dev_record_cnt:%d." %
                         (self.dev_name, file_record_cnt, dev_record_cnt))
            self.ufm_load = 1
            time.sleep(0.01)
            for record_num in range(file_record_cnt + 1, dev_record_cnt + 1):
                self.__key_signal_record(record_num)
            self.file_record_cnt = dev_record_cnt  # Update record value

        # Determine whether to delete the UFM
        if dev_record_cnt >= self.earse_threshold:
            if self.dev_record_cnt == dev_record_cnt:  # If no new record is generated, delete the UFM
                signal_debug("%s record number %d reach earse threshold %d." %
                             (self.dev_name, dev_record_cnt, self.earse_threshold))
                self.file_record_cnt = 0
                self.ufm_earse = 1
                time.sleep(0.1)
            else:  # Set a new record
                signal_debug("%s try to earse ufm, but new record occur, check again." % self.dev_name)
                self.key_signal_check()
        signal_debug("%s key signal check finished." % self.dev_name)
        return

    def cold_reboot_dev_init(self):
        # Cold start initialization
        # ufm_load
        # Read the file to determine how many files the software has recorded
        # If the file does not exist: the system may be upgraded, cold restart
        # Traverse records from 1 to 512 -> Write all records to syslog
        #If the file exists (according to the file value N), record_num traverses from N+1 -> max
        # Set UNIX timestamp, effective
        # Write file value is 0
        # Erase UFM and start CPLD to start recording
        # Initialize OK, return 0, initialize exception returns -1
        signal_syslog_info("%%SIGNAL_MONITOR-6-INFO: %s cold reboot init start." % self.dev_name)
        try:
            self.ufm_load = 1  # load
            time.sleep(0.01)
            file_record_cnt = self.file_record_cnt
            if file_record_cnt < 0:  # File reading failure
                file_record_cnt = 0  # Traverse the record from the beginning
            for record_num in range(file_record_cnt + 1, self.max_record_num + 1):
                ret = self.__key_signal_record(record_num)
                if ret == 0:
                    break
            status, msg = self.set_device_timestamp()
            # Failed to set the timestamp
            if status is False:
                signal_syslog_error(
                    "%%SIGNAL_MONITOR-3-TIME_ERROR: %s set device unix timestamp failed." %
                    self.dev_name)
                return -1
            self.file_record_cnt = 0
            self.ufm_earse = 1
            time.sleep(0.1)
            signal_syslog_info("%%SIGNAL_MONITOR-6-INFO: %s cold reboot init success." % self.dev_name)
            return 0
        except Exception as e:
            signal_syslog_error(
                "%%SIGNAL_MONITOR-3-EXCEPTION: %s cold reboot init error: %s." %
                (self.dev_name, str(e)))
        return -1

    def warm_reboot_dev_init(self):
        # Hot start initialization
        # Read the CPLD status and determine whether the CPLD has enabled key signal recording
        # Not enabled >>> Enable this feature for the first time
        # 1. Set the UNIX timestamp to take effect; Write file value is 0; Erase the UFM and start the CPLD to start recording
        # Opened
        # 1. File does not exist (upgraded sonic) >>> Set the file count value to 0
        # Initialize OK, return 0, initialize exception returns -1
        signal_syslog_info("%%SIGNAL_MONITOR-6-INFO: %s warm reboot init start." % self.dev_name)
        try:
            record_start_flag = self.record_start_flag

            if record_start_flag == -1:  # If the read fails and the status is not certain, handle the reading according to cold start
                signal_syslog_warning(
                    "%%SIGNAL_MONITOR-4-FAILED: %s get record start flag failed, try to cold reboot init." %
                    self.dev_name)
                return self.cold_reboot_dev_init()

            if record_start_flag == 0:  # The key signal recording function of the CPLD is disabled
                status, msg = self.set_device_timestamp()
                # Failed to set the timestamp
                if status is False:
                    signal_syslog_error(
                        "%%SIGNAL_MONITOR-3-TIME_ERROR: %s set device unix timestamp failed." %
                        self.dev_name)
                    return -1
                self.file_record_cnt = 0
                self.ufm_earse = 1
                time.sleep(0.1)
                signal_syslog_info(
                    "%%SIGNAL_MONITOR-6-INFO: %s warm reboot start signal monitor success." %
                    self.dev_name)
                return 0
            # The key signal recording function of the CPLD has been enabled
            if self.file_record_cnt < 0:  # maybe upgrade os first time
                self.file_record_cnt = 0
            signal_syslog_info("%%SIGNAL_MONITOR-6-INFO: %s warm reboot init success." % self.dev_name)
            return 0
        except Exception as e:
            signal_syslog_error(
                "%%SIGNAL_MONITOR-3-EXCEPTION: %s warm reboot init error: %s." %
                (self.dev_name, str(e)))
        return -1


def get_reboot_type(conf):
    # Obtain the restart device Restart type 0: cold start 1: hot start -1: failed
    loc = conf["loc"]
    offset = conf["offset"]

    status, value = dev_file_read(loc, offset, 1)
    if status is False:
        signal_syslog_warning("%%SIGNAL_MONITOR-4-FAILED: get reboot type failed, loc: %s, offset:0x%x" % (loc, offset))
        return -1

    if value[0] == conf["cold"]:
        return 0
    return 1


def cold_reboot_init(obj_list):
    errcnt = 0

    for obj in obj_list:
        ret = obj.cold_reboot_dev_init()
        errcnt += ret
    return errcnt


def warm_reboot_init(obj_list):
    errcnt = 0

    for obj in obj_list:
        ret = obj.warm_reboot_dev_init()
        errcnt += ret
    return errcnt


def run(interval, obj_list):
    loop = 0
    while True:
        loop += 1
        for obj in obj_list:
            try:
                debug_init()
                obj.key_signal_check()
                if (loop % obj.update_unix_ts_cnt) == 0:
                    obj.set_device_timestamp()
            except Exception as e:
                signal_syslog_error(
                    "%%SIGNAL_MONITOR-3-EXCEPTION: %s signal monitor error, msg: %s." %
                    (obj.dev_name, str(e)))
        time.sleep(interval)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    pass


@main.command()
def start():
    '''start signal record'''
    # Check whether the signal_monitor process exists. Only one process can be run
    ApplicationInstance()

    signal_syslog_info("%SIGNAL_MONITOR-6-INFO: signal monitor start")

    obj_list = []
    for item in KEY_SIGNAL_CONF["device"]:  # instantiation
        obj = SignalRecord(item)
        obj_list.append(obj)

    reboot_type_conf = KEY_SIGNAL_CONF["reboot_type"]
    reboot_type = get_reboot_type(reboot_type_conf)
    if reboot_type != 1:  # Treat as cold start
        ret = cold_reboot_init(obj_list)
    else:
        ret = warm_reboot_init(obj_list)
    if ret < 0:
        exit(-1)

    interval = KEY_SIGNAL_CONF["polling"]
    run(interval, obj_list)


if __name__ == '__main__':
    debug_init()
    main()
