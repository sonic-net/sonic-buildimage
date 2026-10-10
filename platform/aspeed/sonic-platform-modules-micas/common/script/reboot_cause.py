#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import sys
import os
import time
import click
import syslog
import logging
import datetime
from platform_util import AliasedGroup, CONTEXT_SETTINGS, get_value, set_value, exec_os_cmd, setup_logger, BSP_COMMON_LOG_DIR
from platform_config import REBOOT_CAUSE_PARA
from public.platform_common_config import (
    REBOOT_CAUSE_NON_HARDWARE,
    REBOOT_CAUSE_POWER_LOSS,
    REBOOT_CAUSE_THERMAL_OVERLOAD_CPU,
    REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,
    REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER,
    REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED,
    REBOOT_CAUSE_WATCHDOG,
    REBOOT_CAUSE_HARDWARE_OTHER,
    REBOOT_CAUSE_CPU_COLD_RESET,
    REBOOT_CAUSE_CPU_WARM_RESET,
    REBOOT_CAUSE_BIOS_RESET,
    REBOOT_CAUSE_PSU_SHUTDOWN,
    REBOOT_CAUSE_BMC_SHUTDOWN,
    REBOOT_CAUSE_BMC_SHUTDOWN_ON_OFF,
    REBOOT_CAUSE_RESET_BUTTON_SHUTDOWN,
    REBOOT_CAUSE_RESET_BUTTON_COLD_SHUTDOWN,
    REBOOT_CAUSE_PATH,
    REBOOT_CAUSE_PATH_DIR,
    SONIC_REBOOT_CAUSE_MATCH_LIST,
    REBOOT_CAUSE_STR2INT,
    REBOOT_TYPE_PATH,
    PRODUCT_STRATEGY,
    PRODUCT_STRATEGY_2,
    SONIC_REBOOT_CAUSE_PATH,
    REBOOT_TYPE_PATH_BASE
)
DEBUG_FILE = "/etc/.reboot_cause_debug"
REBOOT_CAUSE_STARTED_FLAG = "/tmp/.reboot_cause_started_flag"

LOG_FILE = BSP_COMMON_LOG_DIR + "reboot_cause_debug.log"
FIRST_BOOT_PLATFORM_FILE = "/tmp/notify_firstboot_to_platform"

logger = setup_logger(LOG_FILE)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def record_syslog_debug(s):
    logger.debug(s)

def record_syslog(s):
    logger.warning(s)

class RebootCause():

    def __init__(self):
        self.previous_cause_file_path = REBOOT_CAUSE_PATH
        self.reboot_cause_para = REBOOT_CAUSE_PARA.copy()
        self.reboot_cause_list = self.reboot_cause_para.get('reboot_cause_list', None)
        self.other_reboot_cause_record = self.reboot_cause_para.get('other_reboot_cause_record', None)
        self.reboot_cause_str2int = REBOOT_CAUSE_STR2INT

    def monitor_point_check(self, item):
        try:
            gettype = item.get('gettype', None)
            mask = item.get('mask', None)
            okval = item.get('okval', None)
            compare_mode = item.get('compare_mode', "equal")
            ret, value = get_value(item)
            if ret is True:
                record_syslog('%%REBOOT_CAUSE-1-INFO: get reboot cause, item: %s value: %s.' % (item, value))
                if mask:
                    value &= mask
                if compare_mode == "equal":
                    if isinstance(okval, list):
                        if value in okval:
                            return True
                    else:
                        if value == okval:
                            return True
                elif compare_mode == "great":
                    if value > okval:
                        return True
                elif compare_mode == "ignore":
                    return True
                else:
                    record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: compare_mode %s not match error.' % (compare_mode))
            else:
                record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: base point check type:%s not support.' % gettype)
        except Exception as e:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: base point check error. msg: %s.' % (str(e)))
        return False

    def get_latest_pstore_file_utc_time(self):
        """Get UTC time of the latest modified file in /var/log/pstore/"""
        latest_mtime = 0.0

        PSTORE_FILE_PATH = "/sys/fs/pstore"

        if not os.path.exists(PSTORE_FILE_PATH):
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: %s does not exist' % PSTORE_FILE_PATH)
            return None
        try:
            for filename in os.listdir(PSTORE_FILE_PATH):
                file_path = os.path.join(PSTORE_FILE_PATH, filename)
                if os.path.isfile(file_path):
                    mtime = os.path.getmtime(file_path)
                    if mtime > latest_mtime:
                        latest_mtime = mtime
        except PermissionError:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: Permission denied to access %s' % PSTORE_FILE_PATH)
            return None

        if latest_mtime == 0.0:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: No valid files found in %s' % PSTORE_FILE_PATH)
            return None

        # Convert to date -u format string (e.g.: Mon Mar  9 03:06:08 PM UTC 2026)
        # Using time module instead of datetime for better compatibility
        return time.strftime("%a %b %e %I:%M:%S %p UTC %Y", time.gmtime(latest_mtime))

    def process_watchdog_reboot_time(self):
        """Process time logic for watchdog reboot scenario"""
        # 1. Get latest pstore file time
        pstore_time_str = self.get_latest_pstore_file_utc_time()
        if not pstore_time_str:
            # Fallback to system time
            pstore_time_str = time.strftime("%a %b %e %I:%M:%S %p UTC %Y", time.gmtime())
            record_syslog('%%REBOOT_CAUSE: Failed to get pstore time, using current system time.' )
        
        # 2. Calculate time difference using timestamps
        # Get current UTC time in seconds since epoch
        current_time = time.time()
        
        # Get pstore time in seconds since epoch using time.strptime
        try:
            pstore_time_struct = time.strptime(pstore_time_str, "%a %b %e %I:%M:%S %p UTC %Y")
            pstore_utc = time.mktime(pstore_time_struct)
            # Adjust for timezone difference (since mktime assumes local time)
            # Convert back to UTC by removing timezone offset
            local_time = time.localtime(pstore_utc)
            utc_offset = time.mktime(local_time) - time.mktime(time.gmtime(pstore_utc))
            time_diff = int(abs(current_time - pstore_utc - utc_offset))
            
            # 3. Log the time difference (in seconds)
            record_syslog('%%REBOOT_CAUSE-1-INFO: Time difference between system and pstore: %d seconds' % time_diff)
        except Exception as e:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: Failed to calculate time difference: %s' % str(e))

        # 4. Use pstore_time as timestamp for subsequent msg_cmd
        return 0, pstore_time_str

    def record_reboot_type_and_msg_strategy_2(self, type_val, file_msg):
        success = True
        if type_val == self.reboot_cause_str2int[REBOOT_CAUSE_WATCHDOG]:
            status, date = self.process_watchdog_reboot_time()
        else:
            status, date = exec_os_cmd("date")
        if status != 0 or len(date) == 0:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: get date failed.')
            success = False

        reason = "N/A"
        if type_val == self.reboot_cause_str2int[REBOOT_CAUSE_POWER_LOSS]:
            reason = "DeviceColdReboot: device boot with power cycle, reboot_cause node [%d]" % type_val
        msg_cmd = 'echo "%s [Time: %s, Reason: %s]" > %s' % (file_msg, date, reason, self.previous_cause_file_path)
        status, output = exec_os_cmd(msg_cmd)
        if status:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: record msg fail, exec cmd %s failed, %s' % (msg_cmd, output))
            success = False

        if type_val != self.reboot_cause_str2int[REBOOT_CAUSE_NON_HARDWARE]:
            type_val = self.reboot_cause_str2int[REBOOT_CAUSE_HARDWARE_OTHER]
        record_cmd = "echo %s > %s" % (type_val, REBOOT_TYPE_PATH)
        status, output = exec_os_cmd(record_cmd)
        if status:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: record type fail, exec cmd %s failed, %s' % (record_cmd, output))
            success = False

        return success

    def record_reboot_type_and_msg_default(self, type_val, file_msg):
        success = True
        status, date = exec_os_cmd("date")
        if status != 0 or len(date) == 0:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: get date failed.')
            success = False

        record_cmd = "echo %s > %s" % (type_val, REBOOT_TYPE_PATH)
        status, output = exec_os_cmd(record_cmd)
        if status:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: record type fail, exec cmd %s failed, %s' % (record_cmd, output))
            success = False

        msg_cmd = 'echo "User issued \'%s\' command [User: admin, Time: %s]" > %s' % (file_msg, date, self.previous_cause_file_path)
        status, output = exec_os_cmd(msg_cmd)
        if status:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: record msg fail, exec cmd %s failed, %s' % (msg_cmd, output))
            success = False
        return success

    def record_reboot_type_and_msg(self, type_val, file_msg):
        if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2:
            return self.record_reboot_type_and_msg_strategy_2(type_val, file_msg)
        else:
            return self.record_reboot_type_and_msg_default(type_val, file_msg)

    def reboot_cause_record(self, item_list):
        RET = {"RETURN_KEY1": 0}
        try:
            create_dir = "mkdir -p %s %s" % (os.path.dirname(REBOOT_TYPE_PATH_BASE), os.path.dirname(REBOOT_CAUSE_PATH_DIR))
            status, ret_t = exec_os_cmd(create_dir)
            if status != 0:
                RET["RETURN_KEY1"] = -1
                record_syslog(
                    '%%REBOOT_CAUSE-3-EXCEPTION: create %s failed, msg: %s' %
                    (os.path.dirname(self.previous_cause_file_path), ret_t))

            #determine-reboot-cause will record first boot.
            if os.path.exists(FIRST_BOOT_PLATFORM_FILE):
                return self.record_reboot_type_and_msg(self.reboot_cause_str2int[REBOOT_CAUSE_NON_HARDWARE], 'First boot of SONiC')

            #reboot cmd
            if os.path.exists(SONIC_REBOOT_CAUSE_PATH):
                with open(SONIC_REBOOT_CAUSE_PATH, "r") as f:
                    file_msg = f.read()
                    if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2:
                        if "Unknown" not in file_msg:
                            return self.record_reboot_type_and_msg(self.reboot_cause_str2int[REBOOT_CAUSE_NON_HARDWARE], 'Software reboot')
                    else:
                        for sonic_reboot_cause_item in SONIC_REBOOT_CAUSE_MATCH_LIST:
                            match_str = sonic_reboot_cause_item.get("match_str", None)
                            record_msg = sonic_reboot_cause_item.get("record_msg", None)
                            reboot_cause = sonic_reboot_cause_item.get("reboot_cause", REBOOT_CAUSE_NON_HARDWARE)
                            if match_str is None or record_msg is None:
                                record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: invalid SONIC_REBOOT_CAUSE_MATCH_LIST item: %s' % sonic_reboot_cause_item)
                                continue
                            if match_str in file_msg:
                                return self.record_reboot_type_and_msg(self.reboot_cause_str2int.get(reboot_cause, self.reboot_cause_str2int[REBOOT_CAUSE_NON_HARDWARE]), record_msg)
            for item in item_list:
                record_type = item.get('record_type', None)
                if record_type == 'file':
                    file_log = item.get('log', None)
                    file_max_size = item.get('file_max_size', 0)
                    if file_max_size > 0:
                        file_size = 0
                        if os.path.exists(self.previous_cause_file_path):
                            file_size = os.path.getsize(self.previous_cause_file_path) // file_max_size
                        if file_size >= 1:
                            record_cmd = "mv %s %s_bak" % (self.previous_cause_file_path, self.previous_cause_file_path)
                            status, output = exec_os_cmd(record_cmd)
                            if status:
                                record_syslog(
                                    '%%REBOOT_CAUSE-3-EXCEPTION: exec cmd %s failed, %s' %
                                    (record_cmd, output))

                    if  isinstance(file_log, dict):
                        ret, file_log = get_value(file_log)
                        if ret is False:
                            RET["RETURN_KEY1"] = -1
                            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: get reboot reason fail. reason: %s' % file_log)
                            continue

                    type_match_val = next((v for k, v in self.reboot_cause_str2int.items() if k in file_log),
                                         self.reboot_cause_str2int[REBOOT_CAUSE_HARDWARE_OTHER])
                    status = self.record_reboot_type_and_msg(type_match_val, file_log)
                    if status != True:
                        record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: record hw cause fail')
                        continue
                    exec_os_cmd('sync')
                else:
                    RET["RETURN_KEY1"] = -1
                    record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: record_type:%s not support.' % record_type)
                    continue
        except Exception as e:
            RET["RETURN_KEY1"] = -1
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: reboot cause record error. msg: %s.' % (str(e)))
        if RET["RETURN_KEY1"] == 0:
            return True
        return False

    def reboot_cause_check(self):
        try:
            reboot_cause_flag = False
            if self.reboot_cause_list is None:
                record_syslog_debug('%%REBOOT_CAUSE-6-DEBUG: reboot cause check config not found')
                return
            for item in self.reboot_cause_list:
                name = item.get('name', None)
                monitor_point = item.get('monitor_point', None)
                record = item.get('record', None)
                finish_operation_list = item.get('finish_operation', [])
                if name is None or monitor_point is None or record is None:
                    record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: reboot cause check get config failed.name:%s, monitor_point:%s, record:%s' %
                                  (name, monitor_point, record))
                    return
                ret = self.monitor_point_check(monitor_point)
                if ret is True:
                    record_syslog_debug('%%REBOOT_CAUSE-6-DEBUG: %s reboot cause is happen' % name)
                    self.reboot_cause_record(record)
                    reboot_cause_flag = True
                    for finish_operation_item in finish_operation_list:
                        ret, log = set_value(finish_operation_item)
                        if ret is False:
                            log = "%%REBOOT_CAUSE-3-EXCEPTION: " + log
                            record_syslog(log)
                    break

            if reboot_cause_flag is False and self.other_reboot_cause_record is not None:
                record_syslog_debug('%%REBOOT_CAUSE-6-DEBUG: other reboot cause is happen')
                self.reboot_cause_record(self.other_reboot_cause_record)
        except Exception as e:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: reboot cause check error. msg: %s.' % (str(e)))
        return

    def run(self):
        try:
            debug_init()
            if os.path.exists(REBOOT_CAUSE_STARTED_FLAG):
                record_syslog_debug(
                    '%%REBOOT_CAUSE-6-DEBUG: Reboot cause has been started and will not be started again')
                time.sleep(5)
                sys.exit(0)
            self.reboot_cause_check()
            exec_os_cmd("touch %s" % REBOOT_CAUSE_STARTED_FLAG)
            exec_os_cmd("sync")
            time.sleep(5)
            sys.exit(0)
        except Exception as e:
            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: %s.' % (str(e)))

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass

@main.command()
def start():
    '''start process '''
    debug_init()
    reboot_cause = RebootCause()
    reboot_cause.run()

@main.command()
def stop():
    '''stop process '''
    pass

@main.command()
def restart():
    stop()
    start()

if __name__ == '__main__':
    main()
