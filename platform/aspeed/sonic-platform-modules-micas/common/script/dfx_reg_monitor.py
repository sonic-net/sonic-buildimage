#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-
import sys
import os
import time
import threading
import traceback
from platform_config import *
from platform_util import CHECK_VALUE_OK, read_sysfs, check_value_and_get_value, get_value, set_value
from dfx_reg_monitor_h import *
from public.platform_common_config import S3IP_SYSFS_NAME

STATUS_OK = "OK"
STATUS_NOT_OK = "NOT OK"
STATUS_PRESENT = "PRESENT"
STATUS_ABSENT = "ABSENT"

'''
DFX_REG_MONITOR_PARAM = ["dfx_clk", "dfx_vr"]
DFX_CLK_MONITOR_PARAM = {
    "init_delay": 60,
    "interval":60,
    "device": [
        {
            "name": "CDCV304_CLK_TEST_RESULT",
            "monitor_point": {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x53, "mask": 0x01, "okval": 0x1},
        },
        {
            "name": "CDCV304_CLK_TEST_ERR_CNT",
            "monitor_point": {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x54, "okval": 0x0},
        },
        {
            "name": "FPGA1_CPLD_CLK_1_STATUS0",
            "monitor_point": {"gettype": "devfile", "path": "/dev/cpld3", "offset": 0x30, "mask": 0x1, "okval": 0x1},
            "present": {"gettype": "sysfs", "loc": "/sys/s3ip/system/cpu_board_status", "okval": 1, "presentbit": 1},
        },
    ],
}
DFX_VR_MONITOR_PARAM = {
    "init_delay": 60,
    "interval":60,
    "device": [
        {
            "name": "MAC_CORE_VR_HOT",
            "monitor_point": {"gettype": "devfile", "path": "/dev/cpld1", "offset": 0x60, "mask": 0x04, "okval": 0x04},
        },
    ],
}
'''

class Reg_Monitor:
    def __init__(self, t, config):
        # Initialize debug log and record log
        self.t = t
        logger_init(self.t)
        debug_level_init(self.t)

        self.config = config

        # Wait for a period of time after booting before starting monitoring
        self.init_delay = self.config.get('init_delay', 0)
        # Monitoring polling period
        self.interval = self.config.get("interval", 60)
        # List of devices to monitor
        self.device_list = self.config.get("device", [])


    def getpresentstatus(self, param):
        try:
            ret = {}
            ret["status"] = ''
    
            ret_t, val = get_value(param)
            if ret_t is False:
                ret["status"] = STATUS_NOT_OK
                error_log(self.t, "get present status failed, param: %s, log: %s" % (param, val))
                return ret
    
            presentbit = param.get('presentbit', 0)
            okval = param.get('okval')
            val_t = (val & (1 << presentbit)) >> presentbit
            if val_t != okval:
                ret["status"] = STATUS_ABSENT
            else:
                ret["status"] = STATUS_PRESENT
        except Exception as e:
            ret["status"] = STATUS_NOT_OK
            error_log(self.t, "getpresentstatus error, msg: %s" % (traceback.format_exc()))
        return ret

    def clear_status(self, config):
        try:
            ret = {}
            param = config.get("clear_status", None)
            if param is None:
                return True

            ret_t, val = set_value(param)
            if ret_t is False:
                error_log(self.t, "clear status failed, param: %s, log: %s" % (param, val))
                return False
            
            debug_log(self.t, "clear status success, param: %s, log: %s" % (param, val))
            return True
        except Exception as e:
            ret["status"] = "NOT OK"
            error_log(self.t, "clear_status error, msg: %s" % (traceback.format_exc()))
            return False


    def change_to_not_ok_action(self, config):
        try:
            action_change_to_fail_list = config.get("action_change_to_not_ready", [])
            for action_change_to_fail in action_change_to_fail_list:
                if action_change_to_fail:
                    ret, log = get_value(action_change_to_fail.get)
                    if ret is True:
                        record_log(self.t, "%s\n" % log)
                    else:
                        info_log(self.t, f"{action_change_to_fail} action fail. ({log})")
            return
        except Exception as e:
            error_log(self.t, "change_to_not_ok_action error, msg: %s" % (traceback.format_exc()))
            return

    def pre_check(self, config):
        slotid = config.get("slotid", 0)
        slot_sup_list = config.get("slot_sup_list", [])
        is_only_master_cm_monitor = config.get("is_only_master_cm_monitor", 0)
        try:
            # Validate the status of the sub-card, hardcoded to get from the s3ip path
            if slotid != 0:
                slot_status_path = f"/sys/{S3IP_SYSFS_NAME}/slot/slot{slotid}/status"
                ret, val = read_sysfs(slot_status_path)
                if ret == False:
                    error_log(self.t, "slotid status path:%s read error. log:%s" % (slotid, val))
                    return ret
                val = int(val, 10)
                if val != 1:
                    debug_log(self.t, "slotid status path:%s not ready. val:%d" % (slotid, val))
                    return False

                # Check if the sub-card board id is in the support list, hardcoded to get the sub-card id from the s3ip path
                if len(slot_sup_list) != 0:
                    slot_card_type_path = f"/sys/{S3IP_SYSFS_NAME}/slot/slot{slotid}/slot_card_type"
                    ret, val = read_sysfs(slot_card_type_path)
                    if ret == False:
                        error_log(self.t, "slotid card type path:%s read error. log:%s" % (slotid, val))
                        return ret

                    slot_card_type = int(val, 16)

                    if slot_card_type not in slot_sup_list:
                        debug_log(self.t, "slotid:%d card_type:0x%x not in support list:%s" % (slotid, slot_card_type, slot_sup_list))
                        return False

            # Monitor only on the master board
            if is_only_master_cm_monitor == 1:
                cm_flag_path = f"/sys/{S3IP_SYSFS_NAME}/system/is_main_mgmt_board"
                ret, val = read_sysfs(cm_flag_path)
                if ret == False:
                    error_log(self.t, "path:%s read error. log:%s" % (cm_flag_path, val))
                    return ret
                status = int(val, 10)
                if status != 1:
                    debug_log(self.t, "this card is slave cm board, do nothing")
                    return False
            return True
        except Exception as e:
            error_log(self.t, 'pre_check exception: %s.' % (str(e)))
            return False

    def monitor(self):
        try:
            for each_dev_config in self.device_list:
                name = each_dev_config.get("name", None)
                config = each_dev_config.get("monitor_point", {})
                action_change_to_fail_list = each_dev_config.get("action_change_to_not_ready", [])
                # Form the print string based on slotid, print SLOT content if sub-card or line card exists
                slotid = each_dev_config.get("slotid", 0)
                slot_str = "SLOT:%d " % slotid
                if slotid == 0:
                    slot_str = ""

                ret = self.pre_check(each_dev_config)
                if ret is False:
                    debug_log(self.t, "%s pre_check not ready, do nothing." % (name))
                    continue

                present = each_dev_config.get("present", None)
                if present is not None:
                    presentstatus = self.getpresentstatus(present)
                    debug_log(self.t, "%s present status:%s" % (name, presentstatus.get('status')))
                    if presentstatus.get('status') != STATUS_PRESENT:
                        continue
                # if begin not present, need init all
                if config['init_flag'] == STATUS_NOT_OK:
                    self.init_status_record()

                # Poll all conditions, perform corresponding log actions when there is a status change
                last_status = config['last_status']
                last_value = config['last_value']
                ret, result, val = check_value_and_get_value(config)
                if ret is True:
                    config['last_value'] = val
                    # ret is True indicates that the hardware can be read normally
                    if result == CHECK_VALUE_OK: 
                        # This time the status is ok
                        config['last_status'] = STATUS_OK
                        if last_status == STATUS_NOT_OK:
                            # not ok -> ok
                            info_log(self.t, "%s %s status from not ok change to ok.(val 0x%x)" % (slot_str, name, val))
                            record_log(self.t, "%sstate of %s signal is changed to NORMAL." % (slot_str, name))
                            record_log(self.t, "config:%s." % config)
                            record_log(self.t, "config reg read val:0x%x\n" % val)
                    else:
                        # This time the status is not ok
                        config['last_status'] = STATUS_NOT_OK
                        if last_status == STATUS_OK:
                            # ok -> not ok
                            info_log(self.t, "%s %s status from ok change to not ok.(val 0x%x)" % (slot_str, name, val))
                            record_log(self.t, "%sstate of %s signal is changed to ABNORMAL." % (slot_str, name))
                            record_log(self.t, "config:%s." % config)
                            record_log(self.t, "config reg read val:0x%x\n" % val)
                            self.change_to_not_ok_action(each_dev_config)
                        else:
                            # last_status is not ok and last_value != val
                            if last_value != val:
                                info_log(self.t, f"{slot_str} {name} status val from {last_value} change to {val}.")
                                record_log(self.t, "%sstate of %s signal is ABNORMAL." % (slot_str, name))
                                record_log(self.t, "config:%s." % config)
                                record_log(self.t, "config reg read val:0x%x\n" % val)
                                self.change_to_not_ok_action(each_dev_config)
                            else:
                                debug_log(self.t, "%s %s status fail." % (slot_str, name))

                        # record last to clear status
                        self.clear_status(each_dev_config)
                else:
                    # Hardware exception
                    config['last_status'] = STATUS_NOT_OK
                    if last_status == STATUS_OK:
                        # Only log the first time a hardware exception occurs to avoid continuous exceptions from flooding the logs
                        error_log(self.t, "%s status get failed." % (name))
                        record_log(self.t, "%sstate of %s signal is changed to EXCEPTION." % (slot_str, name))
                        record_log(self.t, "config:%s." % config)
                        record_log(self.t, "exception reason:%s\n" % result)
                debug_log(self.t, "%sstate of %s signal." % (slot_str, name))
                debug_log(self.t, "config:%s." % config)
                debug_log(self.t, "config reg read val:0x%x\n" % val)
            return True
        except Exception as e:
            error_log(self.t, 'Reg_Monitor.monitor exception: %s.' % (str(e)))
            error_log(self.t, str(traceback.format_exc()))
            return False

    def init_flag_init(self):
        try:
            # Assign and print the initial state for each device in the device_list that needs monitoring
            for each_dev_config in self.device_list:
                name = each_dev_config.get("name", None)
                config = each_dev_config.get("monitor_point", {})
                if len(config) == 0:
                    error_log(self.t, 'name:%s monitor_point config undefined.' % name)
                    continue
                config['init_flag'] = STATUS_NOT_OK
        except Exception as e:
            error_log(self.t, 'Reg_Monitor.init_flag_init exception: %s.' % (str(e)))

    def init_status_record(self):
        try:
            # Assign and print the initial state for each device in the device_list that needs monitoring
            for each_dev_config in self.device_list:
                name = each_dev_config.get("name", None)
                config = each_dev_config.get("monitor_point", {})
                if len(config) == 0:
                    error_log(self.t, 'name:%s monitor_point config undefined.' % name)
                    continue
                if config['init_flag'] == STATUS_OK:
                    continue
                present = each_dev_config.get("present", None)
                if present is not None:
                    presentstatus = self.getpresentstatus(present)
                    debug_log(self.t, "%s present status:%s" % (name, presentstatus.get('status')))
                    if presentstatus.get('status') != STATUS_PRESENT:
                        continue

                # Form the print string based on slotid, print SLOT content if sub-card or line card exists
                slotid = each_dev_config.get("slotid", 0)
                slot_str = "SLOT:%d " % slotid
                if slotid == 0:
                    slot_str = ""

                ret = self.pre_check(each_dev_config)
                if ret is False:
                    info_log(self.t, "%sinitial state of %s is NOT READY." % (slot_str, name))
                    config['last_status'] = STATUS_NOT_OK
                    continue

                ret, result, val = check_value_and_get_value(config)
                if ret is True:
                    # ret is True indicates that the hardware can be read normally
                    if result == CHECK_VALUE_OK:
                        # The state is ready
                        config['last_status'] = STATUS_OK
                        config['last_value'] = val
                        info_log(self.t, "%s %s initial status is ok.(val 0x%x)" % (slot_str, name, val))
                        record_log(self.t, "%sinitial state of %s is READY." % (slot_str, name))
                        record_log(self.t, "config:%s." % config)
                        record_log(self.t, "config reg read val:0x%x\n" % val)
                    else:
                        # The state is not ready
                        config['last_status'] = STATUS_NOT_OK
                        config['last_value'] = val
                        info_log(self.t, "%s %s initial status is not ok.(val 0x%x)" % (slot_str, name, val))
                        record_log(self.t, "%sinitial state of %s is NOT READY." % (slot_str, name))
                        record_log(self.t, "config:%s." % config)
                        record_log(self.t, "config reg read val:0x%x\n" % val)
                else:
                    # Hardware exception
                    config['last_status'] = STATUS_NOT_OK
                    config['last_value'] = -1
                    record_log(self.t, "%sinitial state of %s is EXCEPTION." % (slot_str, name))
                    record_log(self.t, "config:%s." % config)
                    record_log(self.t, "exception reason:%s\n" % result)
                
                config['init_flag'] = STATUS_OK
            return True
        except Exception as e:
            error_log(self.t, 'Reg_Monitor.init_status_record exception: %s.' % (str(e)))

    def run(self):
        # Some monitoring devices need to wait for a period of time after booting before monitoring
        if self.init_delay > 0:
            debug_log(self.t, "run init_delay is %d, doing delay" % self.init_delay)
            time.sleep(self.init_delay)

        # Record the initial state of the device
        self.init_flag_init()
        self.init_status_record()

        while True:
            try:
                debug_level_init(self.t)
                self.monitor()
            except Exception as e:
                error_log(self.t, 'Reg_Monitor.run exception: %s.' % (str(e)))
                return False
            time.sleep(self.interval)

class Main_Monitor:
    def __init__(self):
        # Initialize debug log and record log
        self.t = "dfx_reg_monitor"
        logger_init(self.t)
        debug_level_init(self.t)

        self.monitor_list = getattr(module_product, "DFX_REG_MONITOR_PARAM", [])
        if len(self.monitor_list) == 0:
            error_log(self.t, "lack of REG_MONITOR_PARAM config")
            exit(-1)

    def run(self):
        # Traverse all monitoring items in the REG_MONITOR_PARAM configuration file, and start a thread for each to perform monitoring
        for type in self.monitor_list:
            # The configuration file should be named "%s_MONITOR_PARAM", retrieve the configuration file here
            type_upper = type.upper()
            config_param = "%s_MONITOR_PARAM" % type_upper
            config = getattr(module_product, config_param, {})
            if len(config) == 0:
                error_log(self.t, "lack of %s config" % config_param)
                exit(-1)

            # Instantiate the Reg_Monitor class with the monitoring type and corresponding type config file, and call the run function for monitoring
            class_instance = Reg_Monitor(type, config)
            th = threading.Thread(target=class_instance.run, args=())
            th.start()
            info_log(self.t, "thread:%s start." % type)

if __name__ == '__main__':
    main_monitor = Main_Monitor()
    main_monitor.run()