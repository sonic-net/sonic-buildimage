#!/usr/bin/env python3
import sys
import os
import copy
import syslog
import time
import click
import traceback
import logging
from platform_config import get_config_param
from platform_util import CHECK_VALUE_OK, check_value, set_value, get_value, setup_logger, BSP_COMMON_LOG_DIR

CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}
LOG_FILE = BSP_COMMON_LOG_DIR + "dfx_clock_monitor_debug.log"
logger = setup_logger(LOG_FILE)

class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        if len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
        return None


SYSLOG_IDENTIFIER = "CLOCK_MONITOR"

CLOCK_ALARM = 1
CLOCK_NORMAL = 0

DEBUG_FILE = "/etc/.clock_monitor_debug_flag"

# use to change clock status debug
CLOCK_MONITOR_STATUS_OK_DEBUG_FILE = "/etc/.clock_monitor_status_ok_debug_flag"
CLOCK_MONITOR_STATUS_NOTOK_DEBUG_FILE = "/etc/.clock_monitor_status_notok_debug_flag"

RC32312_SYSFS_PATH = "/sys/bus/i2c/devices/%s/%s"

RC32312_CLOCK_REG_INIT = [
    {"name": "xtal_los_evt", "gettype": "sysfs", "loc": "xtal_los_evt", "value": 0x3},
    {"name": "xtal_los_cnt", "gettype": "sysfs", "loc": "xtal_los_cnt", "value": 0},
    {"name": "apll_event", "gettype": "sysfs", "loc": "apll_event", "value": 0x7f},
    {"name": "apll_log_event", "gettype": "sysfs", "loc": "apll_log_event", "value": 0},
]

RC32312_LOCK_STATUS_CHECK = [
    {"name": "apll_event", "gettype": "sysfs", "loc": "apll_event", "okval": 0},
]

RC32312_CLOCK_STATUS_SAVE = [
    {"name": "apll_event", "gettype": "sysfs", "loc": "apll_event"},
    {"name": "apll_log_event", "gettype": "sysfs", "loc": "apll_log_event"},
    {"name": "apll_sts", "gettype": "sysfs", "loc": "apll_sts"},
    {"name": "xtal_los_evt", "gettype": "sysfs", "loc": "xtal_los_evt"},
    {"name": "xtal_los_cnt", "gettype": "sysfs", "loc": "xtal_los_cnt"},
    {"name": "xtal_los_sts", "gettype": "sysfs", "loc": "xtal_los_sts"},
]


class ClockMonitor():
    status_ok_debug_flag = 0
    status_notok_debug_flag = 0

    def __init__(self):
        self.clock_list_conf = get_config_param("CLOCK_MONITOR_PARAM", []).get("clock_list", [])

    def log_error(self, msg):
        # msg = msg.decode('utf-8').encode('gb2312')
        logger.error(msg)

    def log_info(self, msg):
        # msg = msg.decode('utf-8').encode('gb2312')
        logger.info(msg)

    def log_debug(self, msg):
        # msg = msg.decode('utf-8').encode('gb2312')
        logger.debug(msg)

    def debug_init(self):
        if os.path.exists(DEBUG_FILE):
            logger.setLevel(logging.DEBUG)
        else:
            logger.setLevel(logging.INFO)

        if os.path.exists(CLOCK_MONITOR_STATUS_OK_DEBUG_FILE):
            self.status_ok_debug_flag = 1
        else:
            self.status_ok_debug_flag = 0

        if os.path.exists(CLOCK_MONITOR_STATUS_NOTOK_DEBUG_FILE):
            self.status_notok_debug_flag = 1
            self.status_ok_debug_flag = 0
        else:
            self.status_notok_debug_flag = 0

    def init_clock(self, clock_item_conf):
        # Clear event
        bus_addr = clock_item_conf.get("bus_addr", None)
        tmp_clock_init_conf_list = clock_item_conf.get("clock_init_conf_list", [])
        for init_conf in tmp_clock_init_conf_list:
            init_conf_name = init_conf.get("name", "")
            ret, log = set_value(init_conf)
            if ret is False:
                self.log_error("%s: action %s init failed. log: %s" % (bus_addr, init_conf_name, log))
            else:
                self.log_debug("%s: action %s init success" % (bus_addr, init_conf_name))

    def check_lock_status(self, clock_item_conf):
        tmp_lock_status = CLOCK_NORMAL
        bus_addr = clock_item_conf.get("bus_addr", None)
        name = clock_item_conf.get("name", "")
        tmp_clock_check_conf_list = clock_item_conf.get("clock_check_conf_list", [])

        for check_conf in tmp_clock_check_conf_list:
            check_reg_name = check_conf.get("name", "")
            tmp_last_check_status = check_conf.get("last_status", CHECK_VALUE_OK)
            ret, val = check_value(check_conf)
            if ret is not True:
                self.log_error("%s %s check_value failed. ret:%s, log:%s" % (bus_addr, check_reg_name, ret, val))
                continue
            else:
                if val != tmp_last_check_status:
                    check_conf["last_status"] = val
                    if val == CHECK_VALUE_OK:
                        self.log_info(
                            "DEASSERT: [%s %s] %s check status change to ok" %
                            (bus_addr, name, check_reg_name))
                    else:
                        self.log_info(
                            "ASSERT: [%s %s] %s check status change to not ok" %
                            (bus_addr, name, check_reg_name))
                if val == CHECK_VALUE_OK:
                    self.log_debug("%s %s check status ok" % (bus_addr, check_reg_name))
                else:
                    tmp_lock_status = CLOCK_ALARM
                    self.log_debug("%s %s check status not ok" % (bus_addr, check_reg_name))
        return tmp_lock_status

    def save_clock_status(self, clock_item_conf):
        bus_addr = clock_item_conf.get("bus_addr", None)
        name = clock_item_conf.get("name", "")
        tmp_clock_save_conf_list = clock_item_conf.get("clock_save_conf_list", [])
        for save_conf in tmp_clock_save_conf_list:
            save_reg_name = save_conf.get("name", "")
            ret, value = get_value(save_conf)
            if ret is False:
                self.log_info("%s %s: get failed, log: %s." % (bus_addr, save_reg_name, value))
            else:
                self.log_info("%s %s: 0x%x." % (bus_addr, save_reg_name, value))

    def do_monitor(self):
        for clock_item_conf in self.clock_list_conf:
            name = clock_item_conf.get("name", "")
            bus_addr = clock_item_conf.get("bus_addr", None)
            if bus_addr is None:
                self.log_error("%s bus_addr is None, cannot monitor." % name)
                continue

            last_lock_status = clock_item_conf.get("last_status", CLOCK_NORMAL)
            tmp_lock_status = self.check_lock_status(clock_item_conf)

            # use to debug, change lock_status
            if self.status_ok_debug_flag:
                tmp_lock_status = CLOCK_NORMAL
                self.log_info("DEBUG MODE: [%s %s] lock_status set to CLOCK_NORMAL." % (bus_addr, name))

            # use to debug, change lock_status
            if self.status_notok_debug_flag:
                tmp_lock_status = CLOCK_ALARM
                self.log_info("DEBUG MODE: [%s %s] lock_status set to CLOCK_ALARM." % (bus_addr, name))

            # only status change, record rc32312 reg info
            if tmp_lock_status != last_lock_status:
                clock_item_conf["last_status"] = tmp_lock_status
                # not ok -> ok
                if tmp_lock_status == CLOCK_NORMAL:
                    self.log_info("DEASSERT: [%s %s] RC32312 clock recovery." % (bus_addr, name))
                else:
                    self.log_info("ASSERT: [%s %s] RC32312 clock error ." % (bus_addr, name))

                # record rc32312 reg info
                self.save_clock_status(clock_item_conf)

                # not ok -> ok
                if tmp_lock_status == CLOCK_NORMAL:
                    # Clear event
                    self.init_clock(clock_item_conf)
            else:
                self.log_debug("[%s %s] RC32312 clock status [%s], not change." % (bus_addr, name, tmp_lock_status))

    def rc32312_config_list_init(self, bus_addr, rc32312_reg_config_list, clock_reg_config_list):
        for init_conf in rc32312_reg_config_list:
            init_conf_name = init_conf.get("name", "")
            init_conf["loc"] = RC32312_SYSFS_PATH % (bus_addr, init_conf["loc"])
            self.log_debug(init_conf_name + ": " + init_conf["loc"])
        tmp_conf_list = rc32312_reg_config_list + clock_reg_config_list
        return tmp_conf_list

    def run(self, interval):
        self.debug_init()
        self.log_debug("init_clock start")
        for clock_item_conf in self.clock_list_conf:
            name = clock_item_conf.get("name", "")
            bus_addr = clock_item_conf.get("bus_addr", None)
            if bus_addr is None:
                self.log_error("%s bus_addr is None, cannot monitor." % name)
                exit(-1)

            # init clock reg list
            clock_item_conf["clock_init_conf_list"] = []
            clock_item_conf["clock_check_conf_list"] = []
            clock_item_conf["clock_save_conf_list"] = []

            tmp_rc32312_clock_init_reg_conf = []
            tmp_rc32312_clock_check_reg_conf = []
            tmp_rc32312_clock_save_reg_conf = []

            tmp_rc32312_clock_init_reg_conf = copy.deepcopy(RC32312_CLOCK_REG_INIT)
            clock_reg_init_conf = clock_item_conf.get("clock_reg_init", [])
            clock_item_conf["clock_init_conf_list"] = self.rc32312_config_list_init(
                bus_addr, tmp_rc32312_clock_init_reg_conf, clock_reg_init_conf)

            tmp_rc32312_clock_check_reg_conf = copy.deepcopy(RC32312_LOCK_STATUS_CHECK)
            lock_status_check_conf = clock_item_conf.get("lock_status_check", [])
            clock_item_conf["clock_check_conf_list"] = self.rc32312_config_list_init(
                bus_addr, tmp_rc32312_clock_check_reg_conf, lock_status_check_conf)

            tmp_rc32312_clock_save_reg_conf = copy.deepcopy(RC32312_CLOCK_STATUS_SAVE)
            clock_status_save_conf = clock_item_conf.get("clock_status_save", [])
            clock_item_conf["clock_save_conf_list"] = self.rc32312_config_list_init(
                bus_addr, tmp_rc32312_clock_save_reg_conf, clock_status_save_conf)

            self.init_clock(clock_item_conf)

        while True:
            try:
                self.debug_init()
                ret = self.do_monitor()
            except Exception as e:
                traceback.print_exc()
                self.log_error(str(e))
            time.sleep(interval)


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''


@main.command()
def start():
    '''start clock monitor'''
    clock_monitor = ClockMonitor()
    interval = get_config_param("CLOCK_MONITOR_PARAM", []).get("interval", 3)
    clock_monitor.run(interval)


# clock_monitor operation
if __name__ == '__main__':
    main()
