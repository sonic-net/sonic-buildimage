#!/usr/bin/env python_nos
import os
import time
import logging
import traceback
import syslog
from platform_config import get_config_param
from platform_util import setup_logger, get_value, set_value, common_syslog_crit, BSP_COMMON_LOG_DIR
from platform_util import write_file_with_lock


STATUS_FAIL = 'fail'
STATUS_BREAK = 'break'
STATUS_LEAK = 'leak'
STATUS_NORMAL = 'normal'

LEAK_STATUS_FILE = "/etc/.%s_leak_status"

LEAK_STATUS_FAIL_VALUE = -1
LEAK_STATUS_BREAK_VALUE = 0
LEAK_STATUS_OK_VALUE = 1
LEAK_STATUS_NOK_VALUE = 2

LEAK_FACTEST_FILE = "/tmp/.leak_monitor_factest_mode_en"
DEBUG_FILE = "/etc/.leak_monitor_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "leak_monitor_debug.log"
logger = setup_logger(LOG_FILE)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def leak_error(s):
    logger.error(s)


def leak_info(s):
    logger.info(s)


def leak_debug(s):
    logger.debug(s)


class LeakMonitor:
    def __init__(self):
        self.leak_monitor_cfg = get_config_param("LEAK_MONITOR_PARAM", {})
        self.interval = self.leak_monitor_cfg.get('interval', 5)
        self.break_vol_threshold = self.leak_monitor_cfg.get('break_vol_threshold', 2800)
        self.leak_vol_threshold = self.leak_monitor_cfg.get('leak_vol_threshold', 1800)
        self.fail_num = self.leak_monitor_cfg.get('fail_num', 6)
        self.break_num = self.leak_monitor_cfg.get('break_num', 6)
        self.leak_num = self.leak_monitor_cfg.get('leak_num', 3)
        self.abnormal_filter_num = self.leak_monitor_cfg.get('abnormal_filter_num', 3)
        self.abnormal_filter_time = self.leak_monitor_cfg.get('abnormal_filter_time', 0.1)
        self.break_power_down_flag = self.leak_monitor_cfg.get('break_power_down_flag', False)
        self.leak_power_down_flag = self.leak_monitor_cfg.get('leak_power_down_flag', False)
        self.power_down_cmd = self.leak_monitor_cfg.get('power_down_cmd', [])
        self.monitor_points = self.leak_monitor_cfg.get('monitor_points', [])
        self.log_interval = self.leak_monitor_cfg.get('log_interval', 24 * 60 * 60)
        self.last_log_time = {}
        for point in self.monitor_points:
            point['fail_count'] = 0
            point['break_count'] = 0
            point['leak_count'] = 0

    def handle_power_down_cmd(self):
        for command in self.power_down_cmd:
            leak_debug("run handle_power_down_cmd: %s" % command)
            ret, log = set_value(command)
            if ret is False:
                leak_error("do handle_power_down_cmd: %s failed, msg: %s" % (command, log))
                return False
        return True

    def get_leakage_filter(self, point):
        filter_break_count = 0
        filter_leak_count = 0
        name = point['name']
        config = point['value']
        for i in range(self.abnormal_filter_num):
            ret, vol = get_value(config)
            if ret is False:
                return STATUS_FAIL, vol

            if vol > self.break_vol_threshold:
                filter_break_count += 1
            elif vol < self.leak_vol_threshold:
                filter_leak_count += 1
            else:
                # Once normal, return directly to normal
                leak_debug(f"{name}: voltage normal, vol = {vol}mv")
                return STATUS_NORMAL, vol
            time.sleep(self.abnormal_filter_time)

        if filter_break_count >= self.abnormal_filter_num:
            leak_debug(f"{name}: rope break detected, vol = {vol}mv")
            return STATUS_BREAK, vol
        if filter_leak_count >= self.abnormal_filter_num:
            leak_debug(f"{name}: leak detected, vol = {vol}mv")
            return STATUS_LEAK, vol

        return STATUS_NORMAL, vol

    def get_leakage_status(self):
        try:
            for point in self.monitor_points:
                name = point['name']
                status, value = self.get_leakage_filter(point)
                if status == STATUS_FAIL:
                    point['fail_count'] += 1
                    point['break_count'] = 0
                    point['leak_count'] = 0
                elif status == STATUS_BREAK:
                    point['break_count'] += 1
                    point['fail_count'] = 0
                    point['leak_count'] = 0
                elif status == STATUS_LEAK:
                    point['leak_count'] += 1
                    point['fail_count'] = 0
                    point['break_count'] = 0
                elif status == STATUS_NORMAL:
                    # voltage normal, resetting counters.
                    point['fail_count'] = 0
                    point['break_count'] = 0
                    point['leak_count'] = 0
                else:
                    leak_info(f"{name}: unknown status {status}")
        except Exception as e:
            leak_error(f"Exception in get_leakage_status: {e}\n{traceback.format_exc()}")

    # Use to limit log frequency
    def log_with_interval(self, log_key, msg):
        """
        Print the log message only if the time elapsed since the last log
        with the same key exceeds the specified interval.

        Args:
            log_key (str): Unique identifier for the log message.
            msg (str): The log message to print.
        """
        """
        now = time.monotonic()
        last_time = self.last_log_time.get(log_key, 0)
        if now - last_time >= self.log_interval:
            leak_error(msg)
            common_syslog_crit("LEAK_MONITOR", msg)
            self.last_log_time[log_key] = now
        else:
            leak_debug(msg)
        """
        leak_error(msg)

    def record_leak_status(self, point):
        record_file = (LEAK_STATUS_FILE % point['name']).lower()
        value = str(point['status'])
        ret, log = write_file_with_lock(record_file, value)
        if ret is False:
            leak_error("write file: %s failed, msg: %s" % (record_file, log))

    def leak_detect(self):
        power_down_flag = False
        try:
            self.get_leakage_status()
            for point in self.monitor_points:
                name = point['name']
                point['status'] = LEAK_STATUS_OK_VALUE
                if point['fail_count'] >= self.fail_num:
                    point['status'] = LEAK_STATUS_FAIL_VALUE
                    # Set 'fail_count' to 'fail_num', avoid continuously increasing to overflow
                    point['fail_count'] = self.fail_num
                    msg = f"{name} get_value failed."
                    self.log_with_interval(f"{name}_FAIL", msg)

                if point['break_count'] >= self.break_num:
                    point['status'] = LEAK_STATUS_BREAK_VALUE
                    # Set 'break_count' to 'fail_num', avoid continuously increasing to overflow
                    point['break_count'] = self.break_num
                    if self.break_power_down_flag:
                        power_down_flag = True
                    msg = f"{name} break."
                    self.log_with_interval(f"{name}_BREAK", msg)

                if point['leak_count'] >= self.leak_num:
                    point['status'] = LEAK_STATUS_NOK_VALUE
                    # Set 'leak_count' to 'leak_num', avoid continuously increasing to overflow
                    point['leak_count'] = self.leak_num
                    if self.leak_power_down_flag:
                        power_down_flag = True
                    msg = f"{name} detected leakage."
                    self.log_with_interval(f"{name}_LEAK", msg)

                self.record_leak_status(point)

            if power_down_flag is True:
                msg = "Going to power down now."
                self.log_with_interval(f"LEAK_POWER_DOWN", msg)
                self.handle_power_down_cmd()
        except Exception as e:
            leak_error(f"Exception in leak_detect: {e}")

    def run(self):
        while True:
            try:
                debug_init()
                if os.path.exists(LEAK_FACTEST_FILE) is True:
                    leak_debug("file exists, do nothing!")
                    time.sleep(self.interval)
                    continue
                self.leak_detect()
            except Exception as e:
                leak_error(f"Exception in leak monitor: {e}")
            time.sleep(self.interval)


if __name__ == '__main__':
    leak_monitor = LeakMonitor()
    leak_monitor.run()
