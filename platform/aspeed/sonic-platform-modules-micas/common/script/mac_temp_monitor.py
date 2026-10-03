#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import logging
import os
import sys
import syslog
import time
from datetime import datetime
import click


from platform_util import (
    read_s3ip_sysfs,
    setup_logger,
    BSP_COMMON_LOG_DIR,
    getPid,
    unload_process_byPid,
    AliasedGroup,
    CONTEXT_SETTINGS,
    exec_os_cmd,
    record_reboot_cause,
    get_reboot_history,
    platform_reboot,
    allow_syslog,
    common_syslog_error,
    common_syslog_warn,
    common_syslog_crit,
    common_syslog_alert,
    common_syslog_emerg,
)


from public.platform_common_config import S3IP_SYSFS_NAME


#log file
LOG_FILE = BSP_COMMON_LOG_DIR + "mac_temp_monitor_debug.log"
logger = setup_logger(LOG_FILE)

DEBUG_FILE  = "/etc/.mac_temp_monitor_debug_flag"
OVERHEAT_REASON = "MAC OVERHEAT"
OVERHEAT_REBOOT_INFO = "MAC OVERHEAT CRITICAL REBOOT"
OVERHEAT_POWERDOWN_INFO = "MAC OVERHEAT CRITICAL POWERDOWN"

CRITICAL_NUM = 3
MONITOR_INTERVAL = 5
CRITICAL_WINDOW = 3600
CHECK_CRITICAL_NUM = 3
CHECK_WARNING_NUM = 3
CHECK_CRITICAL_AGAIN_NUM = 3
CHECK_CRITICAL_AGAIN_TIME = 20


POWER_DOWN_CMD = "curl -k -u root:root -H \"Content-Type: application/json\" -X POST 'https://240.1.1.2/redfish/v1/Systems/1/Actions/ComputerSystem.Reset' -H 'Content-Type: application/json' -d '{\"ResetType\": \"ForceOff\"}'"

MAC_TEMP_SYSFS = f"/sys/{S3IP_SYSFS_NAME}/temp_sensor"
MAC_TEMP_ALIAS_KEY = "MAC_TEMP_MAX"


LEVEL_NORMAL = "normal"
LEVEL_WARNING = "warning"
LEVEL_CRITICAL = "critical"
LEVEL_UNKNOWN = "unknown"

SYSLOG_TITLE = "MACTEMP_MONITOR"
LOG_LAST_TIME = {}

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def log_debug(msg):
    logger.debug(msg)


def log_info(msg):
    logger.info(msg)


def log_warning(msg):
    logger.warning(msg)


def log_error(msg):
    logger.error(msg)


def temp_monitor_warn(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_warn(SYSLOG_TITLE, s)
    log_warning(s)


def temp_monitor_crit(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_crit(SYSLOG_TITLE, s)
    log_info(s)


def temp_monitor_alert(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_alert(SYSLOG_TITLE, s)
    log_info(s)


def temp_monitor_error(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_error(SYSLOG_TITLE, s)
    log_error(s)


def temp_monitor_emerg(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_emerg(SYSLOG_TITLE, s)
    log_info(s)


def check_single_instance():
    script_name = os.path.basename(__file__)
    current_pid = str(os.getpid())

    running_pids = [pid for pid in getPid(script_name) if pid.isdigit() and pid != current_pid]
    if running_pids:
        return False
    return True


class MacTempMonitor(object):
    def __init__(self, polling_time=MONITOR_INTERVAL, critical_limit=CRITICAL_NUM, critical_window=CRITICAL_WINDOW):
        self.polling_time = polling_time
        self.critical_limit = critical_limit
        self.critical_window = critical_window
        self.__critical_checktimes = 0
        self.__warning_checktimes = 0
        self.__check_critical_again_num = CHECK_CRITICAL_AGAIN_NUM
        self.__check_critical_again_time = CHECK_CRITICAL_AGAIN_TIME


    """
    description: get the reboot times caused by MAC overheat in the critical window,
             information is obtained from reboot history file.
    return: True: success
        False: failed
        count: reboot times in the critical window.
    """
    def get_reboot_times(self):
        count = 0
        now_ts = time.time()
        try:
            ret, history = get_reboot_history()
            if ret is False:
                return False, history

            for cause, time_str in history:
                if cause != OVERHEAT_REASON:
                    continue

                try:
                    event_ts = datetime.strptime(time_str.strip(), "%Y-%m-%d %H:%M:%S,%f").timestamp()
                except Exception:
                    continue

                if 0 <= now_ts - event_ts <= self.critical_window:
                    count += 1

        except Exception:
            return False, 0

        return True, count


    def handle_critical_event(self, temp, max_temp):

        temp_monitor_crit("%%PMON-5-TEMP_HIGH: MAC temperature %sC is larger than max threshold %sC." % (temp, max_temp))

        ret = self.check_critical_again()
        if ret is False:
            log_info("MAC temperature check critical again, current temp: %s, critical threshold: %s, the temperature is not critical, skip this round." % (temp, max_temp))
            return

        ret, reboot_times = self.get_reboot_times()
        if ret is False:
            log_error("get reboot times failed, reboot_times: %s" % reboot_times)
            reboot_times = 0

        log_debug("This is the %s time that MAC temperature exceeds the threshold in the last %s seconds." % (reboot_times, self.critical_window))

        #cpu power down
        if reboot_times >= self.critical_limit:
            temp_monitor_crit("%%PMON-5-TEMP_HIGH: MAC temperature overheat, cpu power down.")

            record_reboot_cause(OVERHEAT_REASON, OVERHEAT_POWERDOWN_INFO) #write reboot cause to history file
            exec_os_cmd("sync")
            time.sleep(3)

            ret, output = exec_os_cmd(POWER_DOWN_CMD)
            if ret is False:
                log_error("Temperature overheat, power down failed: %s, current temp: %s, critical threshold: %s" % (output, temp, max_temp))
        #cpu reboot
        else:
            temp_monitor_crit("%%PMON-5-TEMP_HIGH: MAC temperature overheat, cpu reboot.")

            ret, msg = platform_reboot(OVERHEAT_REASON, OVERHEAT_REBOOT_INFO)
            if ret is False:
                log_error("Temperature overheat, reboot failed: %s, current temp: %s, critical threshold: %s" % (msg, temp, max_temp))
        return


    def handle_warning_event(self, temp, high_temp):
        temp_monitor_warn("%%PMON-5-TEMP_WARN: MAC temperature %sC is larger than warning threshold %sC." % (temp, high_temp))
        return


    def get_mac_temp_info(self):
        matched_paths = []

        try:
            for entry in os.listdir(MAC_TEMP_SYSFS):
                if not entry.startswith("temp"):
                    continue

                index = entry[4:]
                if not index.isdigit():
                    continue

                temp_dir = os.path.join(MAC_TEMP_SYSFS, entry)
                alias_path = os.path.join(temp_dir, "alias")
                ret, alias = read_s3ip_sysfs(alias_path)
                if ret is False:
                    continue

                if MAC_TEMP_ALIAS_KEY in alias:
                    matched_paths.append(temp_dir)
        except Exception as e:
            return False, "scan temp sensor path failed, msg: %s" % str(e)

        if len(matched_paths) == 0:
            return False, "cannot find temp node with alias containing %s" % MAC_TEMP_ALIAS_KEY

        if len(matched_paths) > 1:
            return False, "found multiple temp nodes with alias containing %s: %s" % (
                MAC_TEMP_ALIAS_KEY,
                matched_paths,
            )

        temp_dir = matched_paths[0]
        temp_info = {"path": temp_dir}

        for field in ("value", "max", "high"):
            field_path = os.path.join(temp_dir, field)
            ret, field_value = read_s3ip_sysfs(field_path)
            if ret is False:
                return False, "get %s failed, sysfs: %s, msg: %s" % (field, field_path, field_value)
            
            temp_info[field] = int(field_value) / 1000

        return True, temp_info


    """
    description: check multiple times with interval to avoid false alarm again.
    return: True: still critical, 
        False: not critical anymore
    """
    def check_critical_again(self):
        for i in range(self.__check_critical_again_num - 1):
            time.sleep(self.__check_critical_again_time)

            ret, temp_info = self.get_mac_temp_info()
            if ret is False:
                log_error("Failed to get MAC temperature when check critical again, msg: %s" % temp_info)
                return True
        
            temp = temp_info["value"]
            log_debug("Check MAC temperature again, current check times: %s, current temp: %s, max temp: %s, high temp: %s" %\
                (i + 1, temp, temp_info["max"], temp_info["high"]))

            if temp < temp_info["max"]:
                return False

        return True


    """
    description: check multiple times with interval to avoid false alarm.
    return: level 
    """
    def level_judge(self, temp_info):
        temp = temp_info["value"]

        if temp >= temp_info["max"]:
            self.__critical_checktimes += 1
            self.__warning_checktimes += 1
        elif temp >= temp_info["high"]:
            self.__warning_checktimes += 1
        else:
            self.__critical_checktimes = 0
            self.__warning_checktimes = 0


        if self.__critical_checktimes >= CHECK_CRITICAL_NUM:
            self.__critical_checktimes = 0
            self.__warning_checktimes = 0
            level = LEVEL_CRITICAL
        elif self.__warning_checktimes >= CHECK_WARNING_NUM:
            self.__warning_checktimes = 0
            level = LEVEL_WARNING
        else:
            level = LEVEL_NORMAL

        log_debug(
            "current mac temp path: %s, value: %s, max: %s, high: %s , level: %s, critical_checktimes: %s, warning_checktimes: %s" % (
                temp_info["path"],
                temp_info["value"],
                temp_info["max"],
                temp_info["high"],
                level,
                self.__critical_checktimes,
                self.__warning_checktimes
            )
        )

        return level


    def run(self):
        while True:
            debug_init()

            ret, temp_info = self.get_mac_temp_info()
            if ret is False:
                log_error("Failed to get MAC temperature, msg: %s" % temp_info)
                time.sleep(self.polling_time)
                continue

            level = self.level_judge(temp_info)

            if level == LEVEL_CRITICAL:
                self.handle_critical_event(temp_info["value"], temp_info["max"])
            elif level == LEVEL_WARNING:
                self.handle_warning_event(temp_info["value"], temp_info["high"])
            elif level == LEVEL_UNKNOWN:
                log_error("Failed to get MAC temperature, skip this round.")
            else:
                log_debug("LEVEL_NORMAL. current temp: %s, max temp: %s, high temp: %s, critical_checktimes: %s, warning_checktimes: %s" % \
                    (temp_info["value"], temp_info["max"], temp_info["high"], self.__critical_checktimes, self.__warning_checktimes))
    
            time.sleep(self.polling_time)

        return


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    debug_init()


@main.command()
def start():
    '''start mac temp monitor'''
    if check_single_instance() is False:
        log_info("mac temp monitor already running, exit")
        return
    log_info("start mac temp monitor, polling_time: %s, critical_limit: %s, critical_window: %s" % \
        (MONITOR_INTERVAL, CRITICAL_NUM, CRITICAL_WINDOW))
    monitor = MacTempMonitor()
    monitor.run()


@main.command()
def stop():
    '''stop mac temp monitor'''
    log_info("Stop MAC temperature monitor")
    script_name = os.path.basename(sys.argv[0])
    unload_process_byPid(script_name)


@main.command()
def restart():
    '''restart mac temp monitor'''
    stop()
    start()


if __name__ == '__main__':
    main()