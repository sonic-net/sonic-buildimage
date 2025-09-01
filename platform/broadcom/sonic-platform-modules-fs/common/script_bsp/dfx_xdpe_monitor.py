#!/usr/bin/python3
# -*- coding: UTF-8 -*-
import sys
import os
import time
import logging
import traceback
import syslog
from logging.handlers import RotatingFileHandler
from platform_util import read_sysfs
from platform_config import DFX_XDPE_MONITOR_INFO

LOG_DIR = "/var/log/bsp_tech"
LOG_FILE = LOG_DIR + "/dfx_xdpe_monitor.log"
DFX_XDPE_MONITOR_DEBUG_FILE = "/etc/.dfx_xdpe_monitor_debug"

debuglevel = 0
STATUS_OK = 0
STATUS_BYTE = "/sys/bus/i2c/devices/%s/status%d_byte"
STATUS_WORD = "/sys/bus/i2c/devices/%s/status%d_word"
DFX_INFO = "/sys/bus/i2c/devices/%s/dfx_info%d"

def monitor_syslog_init():
    global dev_logger
    if not os.path.exists(LOG_DIR):
        os.system("mkdir -p %s" % LOG_DIR)
        os.system("sync")
    dev_logger = logging.getLogger(__name__)
    dev_logger.setLevel(logging.DEBUG)
    dev_handler = RotatingFileHandler(filename = LOG_FILE, maxBytes = 1 * 1024 * 1024, backupCount = 1)
    dev_handler.setFormatter(logging.Formatter('%(asctime)s, %(filename)s, %(levelname)s, %(message)s'))
    dev_handler.setLevel(logging.DEBUG)
    dev_logger.addHandler(dev_handler)

def monitor_syslog(s):
    dev_logger.info("LINE:%s, %s" % (sys._getframe(1).f_lineno, s))

def monitor_syslog_debug(s):
    if debuglevel == 1:
        syslog.openlog("DFX_XDPE_MONITOR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)

def monitor_syslog_error(s):
    syslog.openlog("DFX_XDPE_MONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_ERR, s)

class Device():
    def __init__(self, dev_conf):
        self.dfx_info_conf = dev_conf
        self.dev_name = dev_conf.get('name', '')
        self.bus_addr = self.dfx_info_conf.get("loc", None)
        self.page_range = self.dfx_info_conf.get("page", [])
        self.last_status = {}  # {0:[byte_status, word_status}, 1: [byte_status, word_status].....}
        self.last_dfx_info = {}  # {0:dfx_info, 1: dfx_info.....}

    def dfx_info_check(self):
        monitor_syslog_debug("dev: %s check start." % self.dev_name)
        try:
            if self.bus_addr is None or len(self.page_range) == 0:
                monitor_syslog_error("dev: %s conf is None." % self.dev_name)
                return False
            
            # initialize the last_status
            if len(self.last_status) == 0:
                for i in self.page_range:
                    self.last_status[i] = [STATUS_OK, STATUS_OK]
                    self.last_dfx_info[i] = ""

            # get current status
            for i in self.page_range:
                loc = STATUS_BYTE % (self.bus_addr, i)
                ret, status_byte_i = read_sysfs(loc)
                if ret is False:
                    monitor_syslog_error("dev: %s, loc:%s, status%d get status failed." % (self.dev_name, loc, i))
                    continue

                loc = STATUS_WORD % (self.bus_addr, i)
                ret, status_word_i = read_sysfs(loc)
                if ret is False:
                    monitor_syslog_error("dev: %s, loc:%s, status_word%d get status failed." % (self.dev_name, loc, i))
                    continue

                page_i_status = [int(status_byte_i, 16), int(status_word_i, 16)]

                # status changed
                if self.last_status.get(i, []) != page_i_status:
                    loc = DFX_INFO % (self.bus_addr, i)
                    ret, dfx_info = read_sysfs(loc)
                    if ret is False:
                        s = "dev: %s, page%d, loc:%s, dfx_info%d get dfx info failed." % (self.dev_name, i, loc, i)
                        monitor_syslog_error(s)
                        dfx_info = s

                    # ok->fail.
                    if self.last_status.get(i, []) == [STATUS_OK, STATUS_OK]:
                        monitor_syslog('dev: %s, page%d, status error detected. \n (byte status, word_status) are (%s).\nDFX INFO:\n%s' % \
                                       (self.dev_name, i, ", ".join(str(hex(x)) for x in page_i_status), dfx_info))
                    # fail->ok.
                    elif page_i_status  == [STATUS_OK, STATUS_OK]:
                        monitor_syslog('dev: %s, page%d, status error recovered. \n (byte status, word_status) are (%s).\n%s' % \
                                       (self.dev_name, i, ", ".join(str(hex(x)) for x in page_i_status), dfx_info))
                    # fail->fail. value changed.
                    else:
                        s = 'dev: %s, page%d, status error changed and value changed. \n (byte status, word_status) are (%s) changed to (%s). \n'
                        s += '\nDFX INFO Before:\n%s'
                        s += '\nDFX INFO After:\n%s'
                        
                        monitor_syslog(s % (self.dev_name, i, ", ".join(str(hex(x)) for x in self.last_status.get(i, [])), ", ".join(str(hex(x)) for x in page_i_status), self.last_dfx_info[i], dfx_info))

                    self.last_status[i] = page_i_status
                    self.last_dfx_info[i] = dfx_info
                else:
                    monitor_syslog_debug('dev: %s, page%d, status Not changed. \n (byte status, word_status) are (%s).' % \
                                   (self.dev_name, i, ", ".join(str(hex(x)) for x in page_i_status)))

            monitor_syslog_debug("dev: %s check fininshed." % self.dev_name)
        except Exception:
            monitor_syslog_error('dev: %s check error. msg: %s\n' % (self.dev_name, str(traceback.format_exc())))

class Monitor():
    def __init__(self):
        self.interval = DFX_XDPE_MONITOR_INFO.get('interval', 5)
        devices_conf = DFX_XDPE_MONITOR_INFO.get('device_list', [])
        
        self.dev_list_obj = []
        for dev_conf in devices_conf:
            tmp_dev_obj = Device(dev_conf)
            self.dev_list_obj.append(tmp_dev_obj)

    def debug_init(self):
        global debuglevel
        if os.path.exists(DFX_XDPE_MONITOR_DEBUG_FILE):
            debuglevel = 1
        else:
            debuglevel = 0

    def doWork(self):
        for dev in self.dev_list_obj:
            dev.dfx_info_check()

    def run(self):
        while True:
            try:
                self.debug_init()
                self.doWork()
            except Exception as e:
                monitor_syslog_error('%%DFX_XDPE_MONITOR-3-EXCEPTION: %s.' % (str(e)))
            time.sleep(self.interval)

if __name__ == '__main__':
    monitor_syslog_init()
    monitor = Monitor()
    monitor.run()
