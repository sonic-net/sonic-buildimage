#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import os
import time
import syslog
from plat_hal.interface import interface
from plat_hal.baseutil import baseutil
from platform_util import io_rd, wbi2cget

INTELLIGENT_MONITOR_DEBUG_FILE = "/etc/.intelligent_monitor_debug"

debuglevel = 0


def monitor_syslog_debug(s):
    if debuglevel:
        syslog.openlog("INTELLIGENT_MONITOR_DEBUG", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def monitor_syslog(s):
    syslog.openlog("INTELLIGENT_MONITOR", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_WARNING, s)


def pmon_syslog_notice(s):
    syslog.openlog("PMON_SYSLOG", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_NOTICE, s)


class IntelligentMonitor():
    def __init__(self):
        self.dcdc_dict = {}
        self.int_case = interface()
        self.__config = baseutil.get_monitor_config()
        self.__intelligent_monitor_para = self.__config.get('intelligent_monitor_para', {})
        self.__interval = self.__intelligent_monitor_para.get('interval', 60)
        self.__dcdc_whitelist = self.__config.get('dcdc_monitor_whitelist', {})
        self.__error_ret = self.int_case.error_ret

    @property
    def error_ret(self):
        return self.__error_ret

    @property
    def interval(self):
        return self.__interval

    def debug_init(self):
        global debuglevel
        if os.path.exists(INTELLIGENT_MONITOR_DEBUG_FILE):
            debuglevel = 1
        else:
            debuglevel = 0

    def dcdc_whitelist_check(self, dcdc_name):
        try:
            check_item = self.__dcdc_whitelist.get(dcdc_name, {})
            if len(check_item) == 0:
                return False
            gettype = check_item.get("gettype", None)
            checkbit = check_item.get("checkbit", None)
            okval = check_item.get("okval", None)
            if gettype is None or checkbit is None or okval is None:
                monitor_syslog('%%INTELLIGENT_MONITOR-3-DCDC_WHITELIST_FAILED: %s config error. gettype:%s, checkbit:%s, okval:%s' %
                               (dcdc_name, gettype, checkbit, okval))
                return False
            if gettype == "io":
                io_addr = check_item.get('io_addr', None)
                val = io_rd(io_addr)
                if val is not None:
                    retval = val
                else:
                    monitor_syslog(
                        '%%INTELLIGENT_MONITOR-3-DCDC_WHITELIST_FAILED: %s io_rd error. io_addr:%s' %
                        (dcdc_name, io_addr))
                    return False
            elif gettype == "i2c":
                bus = check_item.get('bus', None)
                addr = check_item.get('addr', None)
                offset = check_item.get('offset', None)
                ind, val = wbi2cget(bus, addr, offset)
                if ind is True:
                    retval = val
                else:
                    monitor_syslog('%%INTELLIGENT_MONITOR-3-DCDC_WHITELIST_FAILED: %s i2cget error. bus:%s, addr:%s, offset:%s' %
                                   (dcdc_name, bus, addr, offset))
                    return False
            else:
                monitor_syslog('%%INTELLIGENT_MONITOR-3-DCDC_WHITELIST_FAILED: %s gettype not support' % dcdc_name)
                return False

            val_t = (int(retval, 16) & (1 << checkbit)) >> checkbit
            if val_t != okval:
                return False
            return True
        except Exception as e:
            monitor_syslog('%%WHITELIST_CHECK: %s check error, msg: %s.' % (dcdc_name, str(e)))
            return False

    def update_dcdc_status(self):
        try:
            self.dcdc_dict = self.int_case.get_dcdc_all_info()
            for dcdc_name, item in self.dcdc_dict.items():
                ret = self.dcdc_whitelist_check(dcdc_name)
                if ret is False:
                    if item['Value'] == self.error_ret:
                        monitor_syslog(
                            '%%INTELLIGENT_MONITOR-3-DCDC_SENSOR_FAILED: The value of %s read failed.' %
                            (dcdc_name))
                    elif float(item['Value']) > float(item['Max']):
                        pmon_syslog_notice('%%PMON-5-VOLTAGE_HIGH: %s voltage %.3f%s is larger than max threshold %.3f%s.' %
                                           (dcdc_name, float(item['Value']), item['Unit'], float(item['Max']), item['Unit']))
                    elif float(item['Value']) < float(item['Min']):
                        pmon_syslog_notice('%%PMON-5-VOLTAGE_LOW: %s voltage %.3f%s is lower than min threshold %.3f%s.' %
                                           (dcdc_name, float(item['Value']), item['Unit'], float(item['Min']), item['Unit']))
                    else:
                        monitor_syslog_debug('%%INTELLIGENT_MONITOR-6-DCDC_SENSOR_OK: %s normal, value is %.3f%s.' %
                                             (dcdc_name, item['Value'], item['Unit']))
                else:
                    monitor_syslog_debug(
                        '%%INTELLIGENT_MONITOR-6-DCDC_WHITELIST_CHECK: %s is in dcdc whitelist, not monitor voltage' %
                        dcdc_name)
                    continue
        except Exception as e:
            monitor_syslog('%%INTELLIGENT_MONITOR-3-EXCEPTION: update dcdc sensors status error, msg: %s.' % (str(e)))

    def doWork(self):
        self.update_dcdc_status()

    def run(self):
        while True:
            try:
                self.debug_init()
                self.doWork()
                time.sleep(self.interval)
            except Exception as e:
                monitor_syslog('%%INTELLIGENT_MONITOR-3-EXCEPTION: %s.' % (str(e)))


if __name__ == '__main__':
    intelligent_monitor = IntelligentMonitor()
    intelligent_monitor.run()
