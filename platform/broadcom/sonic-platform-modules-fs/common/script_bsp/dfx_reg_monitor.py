#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import sys
import os
import time
import threading
import traceback
from platform_config import *
from platform_util import CHECK_VALUE_OK, read_sysfs, check_value_and_get_value
from dfx_reg_monitor_h import *

STATUS_OK = "OK"
STATUS_NOT_OK = "NOT OK"

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

    def pre_check(self, config):
        slotid = config.get("slotid", 0)
        slot_sup_list = config.get("slot_sup_list", [])
        is_only_master_cm_monitor = config.get("is_only_master_cm_monitor", 0)
        try:
            # Validate the status of the sub-card, hardcoded to get from the s3ip path
            if slotid != 0:
                slot_status_path = "/sys/s3ip/slot/slot%d/status" % slotid
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
                    slot_card_type_path = "/sys/s3ip/slot/slot%d/slot_card_type" % slotid
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
                cm_flag_path = "/sys/s3ip/system/is_main_mgmt_board"
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

                # Form the print string based on slotid, print SLOT content if sub-card or line card exists
                slotid = each_dev_config.get("slotid", 0)
                slot_str = "SLOT:%d " % slotid
                if slotid == 0:
                    slot_str = ""

                ret = self.pre_check(each_dev_config)
                if ret is False:
                    debug_log(self.t, "%s pre_check not ready, do nothing." % (name))
                    continue

                # Poll all conditions, perform corresponding log actions when there is a status change
                last_status = config['last_status']
                ret, result, val = check_value_and_get_value(config)
                if ret is True:
                    # ret is True indicates that the hardware can be read normally
                    if result == CHECK_VALUE_OK: 
                        # This time the status is ok
                        config['last_status'] = STATUS_OK
                        if last_status == STATUS_NOT_OK:
                            # not ok -> ok
                            info_log(self.t, "%s %s status from not ok change to ok." % (slot_str, name))
                            record_log(self.t, "%sstate of %s signal is changed to NORMAL." % (slot_str, name))
                            record_log(self.t, "config:%s." % config)
                            record_log(self.t, "config reg read val:0x%x\n" % val)
                    else:
                        # This time the status is not ok
                        config['last_status'] = STATUS_NOT_OK
                        if last_status == STATUS_OK:
                            # ok -> not ok
                            info_log(self.t, "%s %s status from ok change to not ok." % (slot_str, name))
                            record_log(self.t, "%sstate of %s signal is changed to ABNORMAL." % (slot_str, name))
                            record_log(self.t, "config:%s." % config)
                            record_log(self.t, "config reg read val:0x%x\n" % val)
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
            error_log(str(traceback.format_exc()))
            return False

    def init_status_record(self):
        try:
            # Assign and print the initial state for each device in the device_list that needs monitoring
            for each_dev_config in self.device_list:
                name = each_dev_config.get("name", None)
                config = each_dev_config.get("monitor_point", {})
                if len(config) == 0:
                    error_log(self.t, 'name:%s monitor_point config undefined.' % name)
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
                        record_log(self.t, "%sinitial state of %s is READY." % (slot_str, name))
                        record_log(self.t, "config:%s." % config)
                        record_log(self.t, "config reg read val:0x%x\n" % val)
                    else:
                        # The state is not ready
                        config['last_status'] = STATUS_NOT_OK
                        record_log(self.t, "%sinitial state of %s is NOT READY." % (slot_str, name))
                        record_log(self.t, "config:%s." % config)
                        record_log(self.t, "config reg read val:0x%x\n" % val)
                else:
                    # Hardware exception
                    config['last_status'] = STATUS_NOT_OK
                    record_log(self.t, "%sinitial state of %s is EXCEPTION." % (slot_str, name))
                    record_log(self.t, "config:%s." % config)
                    record_log(self.t, "exception reason:%s\n" % result)
            return True
        except Exception as e:
            error_log(self.t, 'Reg_Monitor.init_status_record exception: %s.' % (str(e)))

    def run(self):
        # Some monitoring devices need to wait for a period of time after booting before monitoring
        if self.init_delay > 0:
            debug_log(self.t, "run init_delay is %d, doing delay" % self.init_delay)
            time.sleep(self.init_delay)

        # Record the initial state of the device
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