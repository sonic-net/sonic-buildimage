#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import sys
import os
import time
import traceback
import syslog
import logging
from platform_config import *
from platform_util import *
from time import monotonic as _time

DEBUG_FILE = "/etc/.hw_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "hw_monitor_debug.log"
logger = setup_logger(LOG_FILE)

STATUS_OK = 1
STATUS_NOT_OK = 0
STATUS_UNDEFINED = -1000

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def hw_error(s):
    logger.error(s)

def hw_info(s):
    logger.info(s)

def hw_debug(s):
    logger.debug(s)

class Monitoring(object):
    def __init__(self, config, log_file_path):
        self.each_dev_config = config
        self.log_file_path = log_file_path

    def monitoring(self):
        self.id = self.each_dev_config.get('id', None)
        self.stop_monitor_condition = self.each_dev_config.get('stop_monitor_condition', [])
        self.disposable_action = self.each_dev_config.get('disposable_action', [])
        self.action_list = self.each_dev_config.get('action', [])
        self.func_call = self.each_dev_config.get('func_call', [])

        try:
            # judging whether this device needs to be monitored, set the tmp flag to 0
            if len(self.stop_monitor_condition) == 0:
                # The device without judgment conditions must be monitored
                hw_debug("%s doesn't have stop_monitor_condition" % (self.id))
            else:
                tmp = 0
                for stop_config in self.stop_monitor_condition:
                    name = stop_config.get("name", None)
                    ret, log, value = check_value_and_get_value(stop_config)
                    if ret is not True:
                        hw_error("stop %s monitor point is not ok, doing monitor." % (name))
                        tmp = -1
                        break
                    if log == CHECK_VALUE_OK:
                        hw_debug("stop %s monitor point check ok" % (name))
                    else:
                        hw_debug("stop %s monitor point check not ok" % (name))
                        tmp = -1
                        break
                # devices that all judgment conditions ok, mean that no need to monitor
                if tmp == 0:
                    hw_debug("stop %s all monitor condition is ok, no monitoring" % self.id)
                    return True

            # before mounting the device, set the tmp flag to 0
            tmp = 0
            # disposable_action is mean only execute one time action
            for disposable_action in self.disposable_action:
                name = disposable_action.get("name", None)
                if disposable_action.get("action_flag", 0) == 0:
                    hw_debug("disposable %s will do disposable_action" % (name))
                    ret, log = set_value(disposable_action)
                    if ret is False:
                        hw_error("disposable %s excute failed. ret:%s, log:%s. \n disposable_action:%s \n"
                                 % (name, ret, log, disposable_action))
                        tmp = -1
                    else:
                        # action finish flag set 1
                        hw_info("disposable %s doing success" % (name))
                        disposable_action["action_flag"] = 1

            # execute non one-time action items
            for action_config in self.action_list:
                name = action_config.get("name", None)
                if action_config.get("log_file_path") is None:
                    action_config["log_file_path"] = self.log_file_path
                ret, log = set_value(action_config)
                if ret is False:
                    hw_error("action %s excute failed. ret:%s, log:%s \n action_config:%s"
                             % (name, ret, log, action_config))
                    tmp = -1
                else:
                    hw_info("action %s doing success" % (name))

            for func_call in self.func_call:
                name = func_call.get("name", None)
                func = func_call.get("func")
                param = func_call.get("param")
                ret = eval(func)(eval(param))
                if ret is False:
                    hw_error("func_call %s excute failed. func:%s param:%s" % (name, func, param))
                    tmp = -1
                else:
                    hw_info("func_call %s doing %s success" % (name, func_call))

            # after all action items are executed, verify whether the stop conditions are met
            for stop_config in self.stop_monitor_condition:
                name = stop_config.get("name", None)
                ret, log, value = check_value_and_get_value(stop_config)
                if ret is not True:
                    hw_error("stop %s result check failed. ret:%s, log:%s \n stop_config:%s"
                             % (name, ret, log, stop_config))
                    tmp = -1
                else:
                    if log == CHECK_VALUE_OK:
                        hw_debug("stop %s check ok " % (name))
                    else:
                        hw_debug("stop %s check not ok " % (name))
                        tmp = -1

            if tmp == 0:
                hw_info("monitoring all device mount success")
                return True
            return False
        except Exception as e:
            hw_error("monitoring exception happening, log:%s" % str(e))
        return False


class Intelligent_Monitor(Monitoring):
    def __init__(self):
        self.dev_config_list = []
        self.monitor_config = HW_MONITOR_PARAM.copy()
        self.init_delay = self.monitor_config.get('init_delay', 0)
        self.ready_timeout = self.monitor_config.get('ready_timeout', 0)
        self.timeout_action_list = self.monitor_config.get('timeout_action', [])
        self.dev_ready_check_list = self.monitor_config.get('dev_ready_check', [])
        self.status_monitor_interval = self.monitor_config.get("status_monitor_interval", 0)
        self.dev_monitor_interval = self.monitor_config.get("dev_monitor_interval", 60)
        self.device_list = self.monitor_config.get("device", [])

    def do_monitor(self):
        for config in self.device_list:
            try:
                status_change_flag = config.get('status_change_flag', 0)
                name = config.get('name', None)
                subdevice_list = config.get('subdevice', [])
                log_file_path = config.get('log_file_path', "/var/log/bsp_tech/hw_common_record.log")

                # dev status detected, but there is no status change flag, don't monitor
                if status_change_flag == 0:
                    hw_debug("do_monitor %s status not from not ok to ok, do nothing" % name)
                    continue

                self.dev_config_list = []
                # the config of each device as a Class
                for sub_config in subdevice_list:
                    config_instance = Monitoring(sub_config, log_file_path)
                    self.dev_config_list.append(config_instance)

                # monitor each device in the list, monitor tmp flag set 0
                tmp = 0
                for dev in self.dev_config_list:
                    # monitor each device in the list
                    ret = dev.monitoring()
                    if ret is False:
                        hw_error("do_monitor %s monitor falied" % dev)
                        tmp = -1
                if tmp == 0:
                    # dev status changed, clean status_change_flag
                    config['status_change_flag'] = 0
                    hw_debug("do_monitor %s all success" % name)

            except Exception as e:
                hw_error("do_monitor exception happening, log:%s" % str(e))
        return

    def leave_option(self, config):
        leave_option_list = config.get("leave_option", [])

        if len(leave_option_list) == 0:
            hw_debug("leave_option is not configured, do nothing")
            return True, "not configured"

        try:
            tmp = 0
            for leave_option_config in leave_option_list:
                name = leave_option_config.get("name", None)
                if leave_option_config.get("log_file_path") is None:
                    leave_option_config["log_file_path"] = config.get('log_file_path', "/var/log/bsp_tech/hw_common_record.log")
                ret, log = set_value(leave_option_config)
                if ret is False:
                    hw_error("leave_option %s set failed. log:%s \n leave_option_config:%s"
                                % (name, log, leave_option_config))
                    tmp = -1
                else:
                    hw_debug("leave_option %s doing success." % (name))

            if tmp == 0:
                # all doing success, clean leave_option_flag
                config['leave_option_flag'] = 0
                hw_debug("leave_option_config %s all success" % leave_option_config)
                return True, "leave_option all success"
            return False, "leave_option has mistakes"
        except Exception as e:
            log = "leave_option exception happening, log:%s" % str(e)
            hw_error(log)
            return False, log


    def update_dev_status(self):
        for each_dev_config in self.device_list:
            monitor_list = each_dev_config.get("monitor_point", [])
            each_dev_ready_check_list = each_dev_config.get("each_dev_ready_check", [])
            each_dev_monitor_list_last_status = each_dev_config.get("last_status", STATUS_UNDEFINED)
            # not monitor point, keeping monitoring
            if len(monitor_list) == 0:
                hw_debug("has no monitor_list")
                each_dev_config['status_change_flag'] = 1
                continue

            # check device ready
            ret, log = self.dev_ready_check(each_dev_ready_check_list)
            if ret is False:
                hw_debug('dev_ready_check not ok, log:%s' % log)
                continue

            # check the monitor point, each_dev_tmp_status use to get each_monitor_list status 
            each_dev_tmp_status = []
            for each_monitor_list in monitor_list:
                recovery_flag = 0
                leave_flag = 0
                #first set each_monitor_list status not ok
                each_monitor_list_status = STATUS_NOT_OK
                for each_monitor_point_config in each_monitor_list:
                    name = each_monitor_point_config.get("name", None)
                    # get current status
                    ret, log, value = check_value_and_get_value(each_monitor_point_config)
                    if ret is True:
                        if log == CHECK_VALUE_OK:
                            #if one of monitor_point item is ok, set each_monitor_list status ok
                            each_monitor_list_status = STATUS_OK
                            #break and set this monitor_list status ok
                            break
                    else:
                        #if all item check fail set each_monitor_list undefine
                        hw_error("update_dev_status %s status check failed. \n config:%s" % (name, each_monitor_point_config))
                        hw_error("error reason: %s" % log)
                        each_monitor_list_status = STATUS_UNDEFINED
                each_dev_tmp_status.append(each_monitor_list_status)

            hw_debug("get each_dev_tmp_status: %s" % each_dev_tmp_status)
            # if one of item is not ok
            if STATUS_NOT_OK in each_dev_tmp_status:
                each_dev_config["last_status"] = STATUS_NOT_OK
                if each_dev_monitor_list_last_status == STATUS_OK:
                    recovery_flag = -1
                    hw_debug("update_dev_status %s status from ok change to not ok." % (name))
                elif each_dev_monitor_list_last_status == STATUS_UNDEFINED:
                    recovery_flag = -1
                    hw_debug("first get %s status. result is not ok" % (name))
                else:
                    recovery_flag = -1
                    leave_flag = -1
            # if status is undef, do nothing
            elif STATUS_UNDEFINED in each_dev_tmp_status:
                hw_debug("get %s status fail, do nothing" % (name))
                recovery_flag = -1
                leave_flag = -1
            else:
                each_dev_config["last_status"] = STATUS_OK
                if each_dev_monitor_list_last_status == STATUS_NOT_OK:
                    # last status is not ok, now is ok, identify status change
                    leave_flag = -1
                    hw_debug("update_dev_status %s status from not ok change to ok." % (name))
                elif each_dev_monitor_list_last_status == STATUS_UNDEFINED:
                    leave_flag = -1
                    hw_debug("first get %s status. result is ok" % (name))
                else:
                    recovery_flag = -1
                    leave_flag = -1

            # the status of all monitor points in the list changes from OK to no ok, flagging dev status change flag
            if recovery_flag == 0:
                hw_debug("update_dev_status %s all config status from not ok change to ok." % (name))
                each_dev_config['status_change_flag'] = 1
            # the status of all monitor points in the list changes from not ok to ok, doing leave option
            if leave_flag == 0:
                hw_debug("update_dev_status %s all config status from ok change to not ok." % (name))
                each_dev_config['leave_flag'] = 1
        if each_dev_config.get('leave_flag', 0) == 1:
            ret, log = self.leave_option(each_dev_config)
            if ret is False:
                hw_debug("update_dev_status %s leave_option failed." % (name))
            else:
                each_dev_config['leave_flag'] = 0
                hw_debug("update_dev_status %s leave_option success." % (name))

    def dev_ready_check(self, check_list):
        try:
            if len(check_list) == 0:
                log = 'dev_ready_check list is None, not checking'
                hw_debug(log)
                return True, log

            for each_check_list in check_list:
                if len(each_check_list) == 0:
                    log = 'dev_ready_check each_check_list is None, config error'
                    hw_error(log)
                    return False, log

                tmp = 0
                for each_check_config in each_check_list:
                    name = each_check_config.get('name', None)
                    ret, val, value = check_value_and_get_value(each_check_config)
                    if ret is False:
                        hw_error("dev_ready_check %s check failed. \n config %s" % (name, each_check_config))
                        hw_error("error log: %s" % val)
                        tmp = -1
                        break
                    if val != CHECK_VALUE_OK:
                        log = 'dev_ready_check %s check not ok' % name
                        hw_debug(log)
                        tmp = -1
                        break
                    else:
                        hw_debug('dev_ready_check %s check ok' % name)
                if tmp == 0:
                    log = 'dev_ready_check all check ok. \n list:%s' % each_check_list
                    hw_debug(log)
                    return True, log
            log = 'dev_ready_check not all check ok. \n list:%s' % check_list
            hw_debug(log)
            return False, log
        except Exception as e:
            log = "dev_ready_check exception happening, log:%s" % str(e)
            hw_error(log)
            return False, log

    def wait_dev_ready(self):
        start_time = _time()
        while True:
            try:
                debug_init()
                if self.ready_timeout > 0 and _time() - start_time >= self.ready_timeout:
                    hw_error("wait_dev_ready check is timeout.")
                    for timeout_action_config in self.timeout_action_list:
                        name = timeout_action_config.get('name', None)
                        ret, log = set_value(timeout_action_config)
                        if ret is False:
                            hw_error("wait_dev_ready %s timeout_action set failed. log:%s \n config:%s" %(name, log, timeout_action_config))
                        else:
                            hw_info("wait_dev_ready %s timeout_action set success." % name)
                    return

                ret, log = self.dev_ready_check(self.dev_ready_check_list)
                if ret is False:
                    hw_debug('wait_dev_ready dev_ready_check not ok, log:%s' % log)
                    time.sleep(self.dev_monitor_interval)
                    continue

                hw_debug('wait_dev_ready dev_ready_check all check ok')
                return
            except Exception as e:
                traceback.print_exc()
                hw_error("wait_dev_ready exception happening, log:%s" % str(e))
                exit(-1)

    def run(self):
        debug_init()
        if self.init_delay > 0:
            hw_debug("run init_delay is %d, doing delay" % self.init_delay)
            time.sleep(self.init_delay)

        self.wait_dev_ready()

        # set statrt time
        start_time = _time()
        while True:
            try:
                debug_init()
                if self.status_monitor_interval > 0 and self.status_monitor_interval < self.dev_monitor_interval:
                    if _time() - start_time >= self.dev_monitor_interval:
                        # time exceeds the device monitoring polling cycle, doing device monitor
                        self.do_monitor()
                        start_time = _time()
                    else:
                        self.update_dev_status()
                        time.sleep(self.status_monitor_interval)
                else:
                    self.update_dev_status()
                    self.do_monitor()
                    time.sleep(self.dev_monitor_interval)
            except Exception as e:
                traceback.print_exc()
                hw_error("run exception happening, log:%s" % str(e))
                exit(-1)

if __name__ == '__main__':
    hw_monitor = Intelligent_Monitor()
    hw_monitor.run()
