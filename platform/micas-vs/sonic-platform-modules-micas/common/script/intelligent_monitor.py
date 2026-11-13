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
import time
import syslog
from plat_hal.interface import interface
from plat_hal.baseutil import baseutil
from platform_util import get_value

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

            checkbit = check_item.get("checkbit", None)
            okval = check_item.get("okval", None)
            if checkbit is None or okval is None:
                monitor_syslog('%%INTELLIGENT_MONITOR-3-DCDC_WHITELIST_FAILED: %s config error. checkbit:%s, okval:%s' %
                               (dcdc_name, checkbit, okval))
                return False
            ret, retval = get_value(check_item)
            if ret is False:
                monitor_syslog(
                    '%%INTELLIGENT_MONITOR-3-DCDC_WHITELIST_FAILED: %s get value failed, msg:%s' % (dcdc_name, retval))
                return False

            val_t = retval & (1 << checkbit) >> checkbit
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
