#!/usr/bin/python3
# -*- coding: UTF-8 -*-
import sys
import os
import time
import syslog
import logging
from platform_util import get_value, set_value, exec_os_cmd, setup_logger, BSP_COMMON_LOG_DIR
from platform_config import REBOOT_CAUSE_PARA

DEBUG_FILE = "/etc/.reboot_cause_debug"
REBOOT_CAUSE_STARTED_FLAG = "/tmp/.reboot_cause_started_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "reboot_cause_debug.log"
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
        self.reboot_cause_para = REBOOT_CAUSE_PARA.copy()
        self.reboot_cause_list = self.reboot_cause_para.get('reboot_cause_list', None)
        self.other_reboot_cause_record = self.reboot_cause_para.get('other_reboot_cause_record', None)

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

    def reboot_cause_record(self, item_list):
        RET = {"RETURN_KEY1": 0}
        try:
            for item in item_list:
                record_type = item.get('record_type', None)
                if record_type == 'file':
                    file_mode = item.get('mode', None)
                    file_log = item.get('log', None)
                    file_path = item.get('path', None)
                    file_max_size = item.get('file_max_size', 0)

                    if file_path is None:
                        record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: record type is file, but path is none.')
                        continue

                    if file_max_size > 0:
                        file_size = 0
                        if os.path.exists(file_path):
                            file_size = os.path.getsize(file_path) // file_max_size
                        if file_size >= 1:
                            reocrd_cmd = "mv %s %s_bak" % (file_path, file_path)
                            status, output = exec_os_cmd(reocrd_cmd)
                            if status:
                                record_syslog(
                                    '%%REBOOT_CAUSE-3-EXCEPTION: exec cmd %s failed, %s' %
                                    (reocrd_cmd, output))

                    if file_mode == 'cover':
                        operate_cmd = ">"
                    elif file_mode == 'add':
                        operate_cmd = ">>"
                    else:
                        RET["RETURN_KEY1"] = -1
                        record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: file record mode:%s not support.' % file_mode)
                        continue

                    create_dir = "mkdir -p %s" % os.path.dirname(file_path)
                    status, ret_t = exec_os_cmd(create_dir)
                    if status != 0:
                        RET["RETURN_KEY1"] = -1
                        record_syslog(
                            '%%REBOOT_CAUSE-3-EXCEPTION: create %s failed, msg: %s' %
                            (os.path.dirname(file_path), ret_t))
                        continue

                    status, date = exec_os_cmd("date")
                    if status != 0 or len(date) == 0:
                        RET["RETURN_KEY1"] = -1
                        record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: get date failed.')
                        continue

                    if  isinstance(file_log, dict):
                        ret, file_log = get_value(file_log)
                        if ret is False:
                            RET["RETURN_KEY1"] = -1
                            record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: get reboot reason fail. reason: %s' % file_log)
                            continue

                    reocrd_cmd = "echo %s %s %s %s" % (file_log, date, operate_cmd, file_path)
                    status, ret_t = exec_os_cmd(reocrd_cmd)
                    if status != 0:
                        RET["RETURN_KEY1"] = -1
                        record_syslog('%%REBOOT_CAUSE-3-EXCEPTION: get date failed, msg: %s' % ret_t)
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


if __name__ == '__main__':
    debug_init()
    reboot_cause = RebootCause()
    reboot_cause.run()
