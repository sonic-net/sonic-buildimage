#!/usr/bin/env python_nos
import os
import json
import subprocess
import time
import sys
import syslog
import traceback
import logging
import click
from plat_hal.interface import interface
from plat_hal.baseutil import baseutil
from algorithm.pid import pid
from algorithm.openloop import openloop
from algorithm.hysteresis import hysteresis
from time import monotonic as _time
from public.platform_common_config import DEFAULT_TEMP_CRIT_RECOVER_CMD, PRODUCT_STRATEGY, PRODUCT_STRATEGY_DEFAULT, PRODUCT_STRATEGY_1, S3IP_SYSFS_NAME, ALL_FAN_ABSENT_LOG, SFF_TEMP_STORE_FILE
from platform_util import *

SWITCH_TEMP = "SWITCH_TEMP"
INLET_TEMP = "INLET_TEMP"
BOARD_TEMP = "BOARD_TEMP"
OUTLET_TEMP = "OUTLET_TEMP"
CPU_TEMP = "CPU_TEMP"
SFF_TEMP = "SFF_TEMP"

TEMP_LEVEL_WARNING = "warning"
TEMP_LEVEL_CRITICAL = "critical"
TEMP_LEVEL_EMERGENCY = "emergency"
TEMP_LEVEL_ALERT = "alert"

EVT_FAN_ABSENT = 'FAN_ABSENT'
EVT_FAN_ROTOR_ERR = 'FAN_ROTOR_ERR'
EVT_ALL_FAN_ERROR = 'ALL_FAN_ERROR'
EVT_PSU_ABSENT = 'PSU_ABSENT'
EVT_PSU_STATUS_ERROR = 'PSU_STATUS_ERROR'
EVT_TEMP_DIFF = 'TEMP_DIFF'
EVT_TEMP_SENSOR_FAIL = 'TEMP_SENSOR_FAIL'
EVT_TEMP_WARNING = 'TEMP_WARNING'
EVT_TEMP_CRITICAL = 'TEMP_CRITICAL'
EVT_TEMP_EMERGENCY = 'TEMP_EMERGENCY'

FAN_CONTRL_FILE = "/tmp/.fancontrol_factest_mode_en"
DEBUG_FILE = "/etc/.fancontrol_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "hal_fanctrl_debug.log"
logger = setup_logger(LOG_FILE)
# coordination with REBOOT_CAUSE_PARA
OTP_SWITCH_REBOOT_JUDGE_FILE = "/etc/.otp_reboot_flag"
OTP_OTHER_REBOOT_JUDGE_FILE = OTP_SWITCH_REBOOT_JUDGE_FILE
SYSLOG_TITLE = "FANCONTROL"
DYNAMIC_OVERTEMP_REBOOT_SYSFS_FILE = f"/sys/{S3IP_SYSFS_NAME}/system/overtemp_reboot"
# used for manually open reboot_flag
FAN_CONTROL_REBOOT_FLAG_FILE = BSP_COMMON_DEBUG_FILE_DIR + ".fancontrol_reboot_flag"

OVERTEMP_KEEP_DEFAULT = 0
OVERTEMP_REBOOT = 1
OVERTEMP_NOT_REBOOT = 2

OVERTEMP_REBOOT_INIT = 1
OVERTEMP_NOT_REBOOT_INIT = 0

F2B_AIR_FLOW = "intake"
B2F_AIR_FLOW = "exhaust"
E2_F2B_AIR_FLOW = "F2B"
E2_B2F_AIR_FLOW = "B2F"
ONIE_E2_NAME = "ONIE_E2"

TEMP_REBOOT_CRIT_SWITCH_FLAG = 1
TEMP_REBOOT_CRIT_OTHER_FLAG = 2
LOG_LAST_TIME = {}
def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def fancontrol_debug(s):
    logger.debug(s)

def fancontrol_error(s):
    logger.error(s)

def fanairflow_debug(s):
    logger.debug(s)

def fanairflow_info(s):
    logger.info(s)

def fancontrol_warn(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_warn(SYSLOG_TITLE, s)
    logger.warning(s)

def fancontrol_crit(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_crit(SYSLOG_TITLE, s)
    logger.critical(s)

def fancontrol_alert(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_alert(SYSLOG_TITLE, s)
    logger.error(s)

def fancontrol_emerg(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_emerg(SYSLOG_TITLE, s)
    logger.error(s)

def hal_fanctrl_syslog_err(title, info):
    if ALL_FAN_ABSENT_LOG != "":
        syslog.openlog(f"{title}", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_ERR, info)

def exec_os_cmd(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        print(output)
    return status, output

error_temp = -9999  # get temp error
invalid_temp = -10000  # get temp invalid
PRE_FAN_NOK_UNKNOWN = "UNKNOWN"


class DevFan(object):

    def __init__(self, name, hal_interface):
        self.__name = name
        self.origin_name = None
        self.display_name = None
        self.air_flow = None
        self.air_flow_inconsistent = False
        self.int_case = hal_interface

    @property
    def name(self):
        return self.__name

    def get_fan_rotor_number(self):
        return self.int_case.get_fan_rotor_number(self.name)

    def get_fan_presence(self):
        return self.int_case.get_fan_presence(self.name)

    def get_fan_rotor_status(self, rotor_name):
        return self.int_case.get_fan_rotor_status(self.name, rotor_name)

    def get_fan_fru_info(self):
        return self.int_case.get_fan_fru_info(self.name)

    @property
    def na_ret(self):
        return self.int_case.na_ret

    def update_fru_info(self):
        try:
            dic = self.get_fan_fru_info()
            self.origin_name = dic["PN"]
            self.air_flow = dic["AirFlow"]
            self.display_name = dic["DisplayName"]
        except Exception as e:
            fanairflow_debug("update %s fru info error, msg: %s" % (self.name, str(e)))
            self.origin_name = self.na_ret
            self.air_flow = self.na_ret
            self.display_name = self.na_ret


class DevPsu(object):

    def __init__(self, name, hal_interface):
        self.__name = name
        self.origin_name = None
        self.display_name = None
        self.air_flow = None
        self.air_flow_inconsistent = False
        self.int_case = hal_interface

    @property
    def name(self):
        return self.__name

    def get_psu_fru_info(self):
        return self.int_case.get_psu_fru_info(self.name)

    @property
    def na_ret(self):
        return self.int_case.na_ret

    def update_fru_info(self):
        try:
            dic = self.get_psu_fru_info()
            self.origin_name = dic["PN"]
            self.air_flow = dic["AirFlow"]
            self.display_name = dic["DisplayName"]
        except Exception as e:
            fanairflow_debug("update %s fru info error, msg: %s" % (self.name, str(e)))
            self.origin_name = self.na_ret
            self.air_flow = self.na_ret
            self.display_name = self.na_ret

class AbnormalEvent:
    def __init__(self, err_type):
        self.err_type = err_type
        self.event_status = False
        self.event_details = ""

        # save last reported event
        self.last_event_status = False
        self.last_event_details = ""

    def event_assert(self, details):
        self.event_status = True
        self.event_details = details

    def event_deassert(self):
        self.event_status = False
        self.event_details = ""

    def need_record(self):
        if self.event_status != self.last_event_status:
            return True
        if self.event_status and self.event_details != self.last_event_details:
            return True
        return False

    def update_last_event(self):
        self.last_event_status = self.event_status
        self.last_event_details = self.event_details

    def __str__(self):
        return f'[{self.err_type}] {self.event_details if self.event_details else "No error"}'

class fancontrol(object):
    __int_case = None

    __pwm = 0x80

    def __init__(self):
        self.int_case = interface()
        self.__config = baseutil.get_monitor_config()
        self.__openloop_config = self.__config["openloop"]
        self.__pid_config = self.__config["pid"]
        self.__hyst_config = self.__config.get("hyst", {})
        self.__temps_threshold_config = self.__config["temps_threshold"]
        self.abnormal_events = {
            EVT_FAN_ABSENT: AbnormalEvent(EVT_FAN_ABSENT),
            EVT_FAN_ROTOR_ERR: AbnormalEvent(EVT_FAN_ROTOR_ERR),
            EVT_ALL_FAN_ERROR: AbnormalEvent(EVT_ALL_FAN_ERROR),
            EVT_PSU_ABSENT: AbnormalEvent(EVT_PSU_ABSENT),
            EVT_PSU_STATUS_ERROR: AbnormalEvent(EVT_PSU_STATUS_ERROR),
            EVT_TEMP_DIFF: AbnormalEvent(EVT_TEMP_DIFF),
            EVT_TEMP_SENSOR_FAIL: AbnormalEvent(EVT_TEMP_SENSOR_FAIL),
            EVT_TEMP_WARNING: AbnormalEvent(EVT_TEMP_WARNING),
            EVT_TEMP_CRITICAL: AbnormalEvent(EVT_TEMP_CRITICAL),
            EVT_TEMP_EMERGENCY: AbnormalEvent(EVT_TEMP_EMERGENCY),
        }
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['temp'] = 0
            temp_threshold['fail_num'] = 0
            temp_threshold['warning_num'] = 0  # temp warning times
            temp_threshold['critical_num'] = 0  # temp critical times
            temp_threshold['emergency_num'] = 0  # temp emergency times
            temp_threshold['alert_num'] = 0  # temp alert times
            temp_threshold.setdefault('ignore_threshold', 0)  # default temp threshold on
            temp_threshold.setdefault('invalid', invalid_temp)
            temp_threshold.setdefault('error', error_temp)
            temp_threshold.setdefault('ignore_threshold_pwm', 0)

        self.__otp_reboot_judge_file_config = self.__config.get("otp_reboot_judge_file", None)
        if self.__otp_reboot_judge_file_config is None:
            self.__otp_switch_reboot_judge_file = OTP_SWITCH_REBOOT_JUDGE_FILE
            self.__otp_other_reboot_judge_file = OTP_OTHER_REBOOT_JUDGE_FILE
        else:
            self.__otp_switch_reboot_judge_file = self.__otp_reboot_judge_file_config.get(
                "otp_switch_reboot_judge_file", OTP_SWITCH_REBOOT_JUDGE_FILE)
            self.__otp_other_reboot_judge_file = self.__otp_reboot_judge_file_config.get(
                "otp_other_reboot_judge_file", OTP_OTHER_REBOOT_JUDGE_FILE)

        self.__fan_rotor_error_num = {}
        self.__fan_present_status = {}  # {"FAN1":0, "FAN2":1...} 1:present, 0:absent
        self.__fan_rotate_status = {}  # {"FAN1":0, "FAN2":1...} 1:OK, 0:NOT OK
        self.__fan_repair_flag = {}    # {"FAN1":0, "FAN2":1...} 1:repair, 0:give up
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            self.__fan_present_status[fan_name] = 1  # present
            self.__fan_rotate_status[fan_name] = 1  # OK
            self.__fan_repair_flag[fan_name] = 1  # repair
            rotor_num = self.get_rotor_number(fan_name)
            tmp_fan = {}
            for j in range(rotor_num):
                rotor_name = "Rotor" + str(j + 1)
                tmp_fan[rotor_name] = 0  # not error
            self.__fan_rotor_error_num[fan_name] = tmp_fan

        self.__fancontrol_para = self.__config["fancontrol_para"]
        self.__interval = self.__fancontrol_para.get("interval", 5)
        self.__fan_status_interval = self.__fancontrol_para.get("fan_status_interval", 0)
        self.__max_pwm = self.__fancontrol_para.get("max_pwm", 0xff)
        self.__min_pwm = self.__fancontrol_para.get("min_pwm", 0x80)
        self.__abnormal_pwm = self.__fancontrol_para.get("abnormal_pwm", 0xbb)
        self.__warning_pwm = self.__fancontrol_para.get("warning_pwm", 0xff)
        self.__critical_pwm = self.__fancontrol_para.get("critical_pwm", self.__max_pwm)
        self.__emerg_pwm = self.__fancontrol_para.get("emerg_pwm", self.__max_pwm)
        self.__temp_invalid_pid_pwm = self.__fancontrol_para.get("temp_invalid_pid_pwm", 0x80)
        self.__temp_error_pid_pwm = self.__fancontrol_para.get("temp_error_pid_pwm", 0x80)
        self.__temp_fail_num = self.__fancontrol_para.get("temp_fail_num", 3)
        self.__check_temp_fail = self.__fancontrol_para.get("check_temp_fail", [])
        self.__temp_warning_num = self.__fancontrol_para.get("temp_warning_num", 3)
        self.__temp_critical_num = self.__fancontrol_para.get("temp_critical_num", 3)
        self.__temp_emergency_num = self.__fancontrol_para.get("temp_emergency_num", 3)
        self.__temp_alert_num = self.__fancontrol_para.get("temp_alert_num", 1)
        self.__temp_warning_countdown = self.__fancontrol_para.get("temp_warning_countdown", 60)
        self.__temp_critical_countdown = self.__fancontrol_para.get("temp_critical_countdown", 60)
        self.__temp_emergency_countdown = self.__fancontrol_para.get("temp_emergency_countdown", 60)
        self.__rotor_error_count = self.__fancontrol_para.get("rotor_error_count", 6)
        self.__inlet_mac_diff = self.__fancontrol_para.get("inlet_mac_diff", 50)
        self.__inlet_mac_diff_flag = self.__fancontrol_para.get("check_inlet_mac_diff", 1)
        self.__check_crit_reboot_flag = self.__fancontrol_para.get("check_crit_reboot_flag", OVERTEMP_REBOOT_INIT)
        self.__dynamic_check_crit_reboot_flag = self.__fancontrol_para.get("dynamic_check_crit_reboot_flag", None)
        self.__check_emerg_reboot_flag = self.__fancontrol_para.get("check_emerg_reboot_flag", OVERTEMP_REBOOT_INIT)
        self.__check_alert_reboot_flag = self.__fancontrol_para.get("check_alert_reboot_flag", OVERTEMP_NOT_REBOOT_INIT)
        self.__check_crit_reboot_num = self.__fancontrol_para.get("check_crit_reboot_num", 3)
        self.__check_crit_sleep_time = self.__fancontrol_para.get("check_crit_sleep_time", 20)
        self.__check_emerg_reboot_num = self.__fancontrol_para.get("check_emerg_reboot_num", 3)
        self.__check_emerg_sleep_time = self.__fancontrol_para.get("check_emerg_sleep_time", 20)
        self.__check_alert_reboot_num = self.__fancontrol_para.get("check_alert_reboot_num", 3)
        self.__check_alert_sleep_time = self.__fancontrol_para.get("check_alert_sleep_time", 0.05)
        self.__check_temp_emergency = self.__fancontrol_para.get("check_temp_emergency", 0)
        self.__check_temp_critical = self.__fancontrol_para.get("check_temp_critical", 1)
        self.__check_temp_warning = self.__fancontrol_para.get("check_temp_warning", 1)
        self.__check_temp_emergency_reboot = self.__fancontrol_para.get("check_temp_emergency_reboot", [])
        self.__check_temp_critical_reboot = self.__fancontrol_para.get("check_temp_critical_reboot", [])
        self.__check_temp_alert_reboot = self.__fancontrol_para.get("check_temp_alert_reboot", [])
        self.__psu_absent_fullspeed_num = self.__fancontrol_para.get("psu_absent_fullspeed_num", 1)
        self.__fan_absent_fullspeed_num = self.__fancontrol_para.get("fan_absent_fullspeed_num", 1)
        self.__rotor_error_fullspeed_num = self.__fancontrol_para.get("rotor_error_fullspeed_num", 1)
        self.__psu_fan_control = self.__fancontrol_para.get("psu_fan_control", 1)  # default control psu fan
        self.__fan_plug_in_pwm = self.__fancontrol_para.get("fan_plug_in_pwm", 0x80)
        self.__sff_temp_s3ip_scheme = self.__fancontrol_para.get("sff_temp_s3ip_scheme", 0)
        self.__sff_temp_retry_times = self.__fancontrol_para.get("sff_temp_retry_times", 3)
        self.__sff_temp_retry_interval = self.__fancontrol_para.get("sff_temp_retry_interval", 0.02)
        self.__fan_plug_in_default_countdown = self.__fancontrol_para.get("fan_plug_in_default_countdown", 0)
        self.__deal_fan_error_policy = self.__fancontrol_para.get("deal_fan_error", 0)
        self.__deal_fan_error_conf = self.__fancontrol_para.get("deal_fan_error_conf", {})
        self.__deal_fan_error_default_countdown = self.__deal_fan_error_conf.get("countdown", 0)
        self.__check_crit_recover_cmd = self.__fancontrol_para.get(
            "check_crit_recover_cmd", DEFAULT_TEMP_CRIT_RECOVER_CMD)
        self.__deal_over_temp_reboot_cmd = self.__fancontrol_para.get("deal_over_temp_reboot_cmd", [])
        self.__deal_over_temp_reboot_pwm = self.__fancontrol_para.get("deal_over_temp_reboot_pwm")
        self.__fanctrl_mode = self.__fancontrol_para.get("fanctrl_mode", {"loc": f"/sys/{S3IP_SYSFS_NAME}/fan/fanctrl_mode", "gettype": "sysfs"})
        self.__fanctrl_fixed_ratio = self.__fancontrol_para.get("fanctrl_fixed_ratio", {"loc": f"/sys/{S3IP_SYSFS_NAME}/fan/fanctrl_fixed_ratio", "gettype": "sysfs", "int_decode": 10})
        self.__fanctrl_mode_pwm_def = self.__fancontrol_para.get("__fanctrl_mode_pwm_def", 0x80)
        self.__fanctrl_duration = self.__fancontrol_para.get("fanctrl_duration", {"loc": f"/sys/{S3IP_SYSFS_NAME}/fan/fanctrl_duration", "gettype": "sysfs", "int_decode": 10})
        self.__fanctrl_duration_def = self.__fancontrol_para.get("__fanctrl_duration_def", 0)
        self.__fanctrl_duration_max = self.__fancontrol_para.get("fanctrl_duration_max", {"loc": f"/sys/{S3IP_SYSFS_NAME}/fan/fanctrl_duration_max", "gettype": "sysfs", "int_decode": 10})
        self.__fanctrl_duration_update_time = self.__fancontrol_para.get("fanctrl_duration_update_time", {"loc": f"/sys/{S3IP_SYSFS_NAME}/fan/fanctrl_duration_update_time", "gettype": "sysfs", "int_decode": 10})
        self.__fanctrl_start_time = None
        self.__fanctrl_last_duration_update_time = None
        self.__deal_all_fan_absent_method_flag = self.__fancontrol_para.get("deal_all_fan_absent_method_flag", 0)
        if self.__deal_all_fan_absent_method_flag:
            self.__all_fan_absent_power_off_cmd = self.__fancontrol_para.get("all_fan_absent_power_off_cmd", "/sbin/reboot")
        self.__deal_all_fan_error_method_flag = self.__fancontrol_para.get("deal_all_fan_error_method_flag", 0)
        if self.__deal_all_fan_error_method_flag:
            self.__all_fan_error_switch_temp_critical_temp = self.__fancontrol_para.get(
                "all_fan_error_switch_temp_critical_temp", 100)
            self.__all_fan_error_recover_log = self.__fancontrol_para.get(
                "all_fan_error_recover_log", "Reboot the system.")
            self.__all_fan_error_recover_cmd = self.__fancontrol_para.get("all_fan_error_recover_cmd", "/sbin/reboot")
            self.__all_fan_error_check_crit_reboot_num = self.__fancontrol_para.get(
                "all_fan_error_check_crit_reboot_num", 3)
            self.__all_fan_error_check_crit_sleep_time = self.__fancontrol_para.get(
                "all_fan_error_check_crit_sleep_time", 20)
        self.__fan_rotor_threshold_config = self.__config.get("fan_rotor_threshold", {})
        if len(self.__fan_rotor_threshold_config) != 0:
            self.lowest_rpm = self.__fancontrol_para.get("lowest_rpm", 1000)
            self.fan_rpm_check_warning = self.__fancontrol_para.get("fan_rpm_check_warning", 85)

        self.__warning_countdown = 0  # temp warning flag for normal fancontrol
        self.__critical_countdown = 0  # temp critical flag for normal fancontrol
        self.__emergency_countdown = 0  # temp emergency flag for normal fancontrol
        self.__fan_plug_in_countdown = 0  # fan plug in flag for normal fancontrol
        self.__deal_fan_error_countdown = 0
        self.__fan_absent_num = 0
        self.__fan_nok_num = 0
        self.__pre_fan_nok = PRE_FAN_NOK_UNKNOWN
        self.openloop = openloop()
        self.pid = pid()
        self.hyst = hysteresis()
        self.__pwm = self.__min_pwm
        self.__warning_pwm_flag = False
        self.__critical_pwm_flag = False
        self.__emergency_pwm_flag = False

        self.__board_air_flow = ""
        self.__fan_air_flow_monitor = self.__fancontrol_para.get("fan_air_flow_monitor", 0)
        self.__psu_air_flow_monitor = self.__fancontrol_para.get("psu_air_flow_monitor", 0)
        self.__air_flow_correct_fan_pwm = self.__fancontrol_para.get("air_flow_correct_fan_pwm", 0xff)
        self.__air_flow_correct_psu_pwm = self.__fancontrol_para.get("air_flow_correct_psu_pwm", 0xff)
        self.__air_flow_error_fan_pwm = self.__fancontrol_para.get("air_flow_error_fan_pwm", 0)
        self.__air_flow_error_psu_pwm = self.__fancontrol_para.get("air_flow_error_psu_pwm", 0)
        self.fan_air_flow_inconsistent_flag = False
        self.psu_air_flow_inconsistent_flag = False
        self.air_flow_inconsistent_flag = False
        self.fan_obj_list = []
        self.psu_obj_list = []
        self.__last_sff_over_temp_port = None
        self.__last_sff_over_temp_value = None
        self.__curr_max_sff_temp_port = None
        self.__curr_max_sff_temp_value = None

    @property
    def na_ret(self):
        return self.int_case.na_ret

    def get_onie_e2_obj(self, name):
        return self.int_case.get_onie_e2_obj(name)

    @property
    def board_air_flow(self):
        air_flow_tuple = (F2B_AIR_FLOW, B2F_AIR_FLOW, E2_F2B_AIR_FLOW, E2_B2F_AIR_FLOW)
        if self.__board_air_flow not in air_flow_tuple:
            self.__board_air_flow = self.int_case.get_device_airflow(ONIE_E2_NAME)
            fanairflow_debug("board_air_flow: %s" % self.__board_air_flow)
        return self.__board_air_flow

    @property
    def fan_air_flow_monitor(self):
        return self.__fan_air_flow_monitor

    @property
    def psu_air_flow_monitor(self):
        return self.__psu_air_flow_monitor

    @property
    def air_flow_correct_fan_pwm(self):
        return self.__air_flow_correct_fan_pwm

    @property
    def air_flow_correct_psu_pwm(self):
        return self.__air_flow_correct_psu_pwm

    @property
    def air_flow_error_fan_pwm(self):
        return self.__air_flow_error_fan_pwm

    @property
    def air_flow_error_psu_pwm(self):
        return self.__air_flow_error_psu_pwm

    def get_para(self, t):
        para = self.__pid_config.get(t)
        return para

    def abnormal_event_process(self, event, event_flag, event_list = []):
        if event_flag:
            detail = self.abnormal_events[event].err_type + ": " + ", ".join(event_list)
            self.abnormal_events[event].event_assert(detail)
        else:
            self.abnormal_events[event].event_deassert()

    def update_over_temp_threshold_num(self):
        for temp_threshold in self.__temps_threshold_config.values():
            if temp_threshold['ignore_threshold']:
                continue
            alert_threshold = temp_threshold.get('alert', None)
            emergency_threshold = temp_threshold.get(TEMP_LEVEL_EMERGENCY, None)
            critical_threshold = temp_threshold.get(TEMP_LEVEL_CRITICAL, None)
            warning_threshold = temp_threshold.get(TEMP_LEVEL_WARNING, None)
            fancontrol_debug("%s warning = %s, critical = %s, alert = %s, emergency = %s" %
                             (temp_threshold['name'], warning_threshold, critical_threshold,
                              alert_threshold, emergency_threshold))

            if alert_threshold is not None and temp_threshold['temp'] >= alert_threshold:
                temp_threshold['alert_num'] += 1
            else:
                temp_threshold['alert_num'] = 0

            if emergency_threshold is not None and temp_threshold['temp'] >= emergency_threshold:
                temp_threshold['emergency_num'] += 1
            else:
                temp_threshold['emergency_num'] = 0

            if critical_threshold is not None and temp_threshold['temp'] >= critical_threshold:
                temp_threshold['critical_num'] += 1
            else:
                temp_threshold['critical_num'] = 0

            if warning_threshold is not None and temp_threshold['temp'] >= warning_threshold:
                temp_threshold['warning_num'] += 1
            else:
                temp_threshold['warning_num'] = 0

            fancontrol_debug("%s warning_num = %d, critical_num = %d, alert_num = %d, emergency_num = %d" %
                             (temp_threshold['name'], temp_threshold['warning_num'], temp_threshold['critical_num'],
                              temp_threshold['alert_num'], temp_threshold.get("emergency_num")))

            # Clear last SFF over-temp state when SFF_TEMP is no longer over threshold.
            if temp_threshold['name'] == SFF_TEMP and temp_threshold['warning_num'] == 0 and \
                    temp_threshold['critical_num'] == 0 and temp_threshold['emergency_num'] == 0:
                self.__last_sff_over_temp_port = None
                self.__last_sff_over_temp_value = None

    def get_sff_temp_and_port(self):
        retry_times = max(1, int(self.__sff_temp_retry_times))
        retry_interval = float(self.__sff_temp_retry_interval)
        last_err = "unknown error"

        for _ in range(retry_times):
            try:
                # Temperature read from this file is in milli-degree Celsius.
                with open(SFF_TEMP_STORE_FILE, "r") as sff_fd:
                    data = sff_fd.read()
            except Exception as e:
                last_err = "read sff temp file failed, reason: %s" % str(e)
                if retry_interval > 0:
                    time.sleep(retry_interval)
                continue

            try:
                data_dict = json.loads(data)
                max_info = data_dict.get("max", {})
                # Temperature read from SFF_TEMP_STORE_FILE file is in milli-degree Celsius.
                temp = int(max_info.get("temp", 0)) / 1000
                index = int(max_info.get("index", 0))
                if index <= 0:
                    last_err = "invalid sff max index: %s" % index
                    if retry_interval > 0:
                        time.sleep(retry_interval)
                    continue
                return True, {"temp": temp, "index": index}
            except Exception as e:
                last_err = "parse sff temp file failed, reason: %s" % str(e)
                if retry_interval > 0:
                    time.sleep(retry_interval)

        return False, last_err

    def log_sff_over_temp_if_changed(self, level, temp_threshold):
        if self.__sff_temp_s3ip_scheme == 0:
            return
        if temp_threshold['name'] != SFF_TEMP:
            return

        if self.__curr_max_sff_temp_port is None or self.__curr_max_sff_temp_value is None:
            fancontrol_debug("skip sff port log due to invalid sff snapshot")
            return

        max_temp = self.__curr_max_sff_temp_value
        max_port = self.__curr_max_sff_temp_port
        if max_port == self.__last_sff_over_temp_port and max_temp == self.__last_sff_over_temp_value:
            return

        self.__last_sff_over_temp_port = max_port
        self.__last_sff_over_temp_value = max_temp
        threshold_val = temp_threshold.get(level)
        msg = "%%FANCONTROL-3-SFF_TEMP_HIGH: eth%d transceiver max temperature %sC is larger than %s threshold %sC." % \
              (max_port, max_temp, level, threshold_val)
        additional_key = "%s_eth%d" % (SFF_TEMP, max_port)
        if level == TEMP_LEVEL_WARNING:
            fancontrol_warn(msg, additional_key)
        elif level == TEMP_LEVEL_CRITICAL:
            fancontrol_crit(msg, additional_key)
        elif level == TEMP_LEVEL_EMERGENCY:
            fancontrol_alert(msg, additional_key)
        elif level == TEMP_LEVEL_ALERT:
            fancontrol_alert(msg, additional_key)

    def _get_temp_threshold_value(self, temp_threshold, sensorlist):
        if self.__sff_temp_s3ip_scheme == 1 and temp_threshold['name'] == SFF_TEMP:
            ret, max_info = self.get_sff_temp_and_port()
            if not ret:
                return False, None, max_info

            temp_value = int(max_info["temp"])
            self.__curr_max_sff_temp_value = temp_value
            self.__curr_max_sff_temp_port = int(max_info["index"])
            return True, temp_value, None
        else:
            sensor = sensorlist.get(temp_threshold['name'])
            if sensor is None or sensor["Value"] is None:
                return False, None, None

            temp_value = int(sensor["Value"])
            if temp_value == self.int_case.error_ret:
                return False, None, None

            return True, temp_value, None

    def get_monitor_temp(self, record_flag = True):
        sensorlist = self.int_case.get_temp_info()
        self.__curr_max_sff_temp_port = None
        self.__curr_max_sff_temp_value = None

        for temp_threshold in self.__temps_threshold_config.values():
            ret, temp_value, err_msg = self._get_temp_threshold_value(temp_threshold, sensorlist)
            if not ret:
                temp_threshold['fail_num'] += 1
                if err_msg is None:
                    fancontrol_error("get %s failed, fail_num = %d" %
                                     (temp_threshold['name'], temp_threshold['fail_num']))
                else:
                    fancontrol_error("get %s failed, fail_num = %d, msg:%s" %
                                     (temp_threshold['name'], temp_threshold['fail_num'], err_msg))
            else:
                temp_threshold['fail_num'] = 0
                temp_threshold.setdefault('fix', 0)
                temp_threshold['temp'] = temp_value + temp_threshold['fix']
                if record_flag:
                    fanairflow_info("get %s = %d" % (temp_threshold['name'], temp_threshold['temp']))
            fancontrol_debug("%s = %d" % (temp_threshold['name'], temp_threshold['temp']))
        self.update_over_temp_threshold_num()

    def is_temp_warning(self):
        warning_flag = False
        self.__warning_pwm_flag = False
        event_list = []
        for temp_threshold in self.__temps_threshold_config.values():
            if temp_threshold['ignore_threshold']:
                continue
            if temp_threshold['warning_num'] >= self.__temp_warning_num:
                warning_flag = True
                event_list.append(f"{temp_threshold['name']} temperature {temp_threshold['temp']}C is larger than warning threshold {temp_threshold.get(TEMP_LEVEL_WARNING)}C.")
                fancontrol_warn("%%FANCONTROL-4-TEMP_HIGH: %s temperature %sC is larger than warning threshold %sC." %
                                (temp_threshold['name'], temp_threshold['temp'], temp_threshold.get(TEMP_LEVEL_WARNING)), temp_threshold['name'])
                self.log_sff_over_temp_if_changed(TEMP_LEVEL_WARNING, temp_threshold)
                if not temp_threshold['ignore_threshold_pwm']:
                    self.__warning_pwm_flag = True
        return warning_flag, event_list

    def checkTempWarning(self):
        try:
            warning_flag, event_list = self.is_temp_warning()
            if warning_flag:
                if self.__warning_pwm_flag is True:
                    self.__warning_countdown = self.__temp_warning_countdown
                fancontrol_debug("temp is over warning")
                return True, event_list
            if self.__warning_countdown > 0:
                self.__warning_countdown -= 1
            return False, []
        except Exception as e:
            fancontrol_error("%%policy: checkTempWarning failed")
            fancontrol_error(str(e))
        return False, []

    def checkTempWarningCountdown(self):
        if self.__warning_countdown > 0:
            return True
        return False

    def is_temp_critical(self):
        critical_flag = False
        self.__critical_pwm_flag = False
        event_list = []
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['critical_flag'] = False
            if temp_threshold['ignore_threshold']:
                continue
            if temp_threshold['critical_num'] >= self.__temp_critical_num:
                critical_flag = True
                temp_threshold['critical_flag'] = True
                event_list.append(f"{temp_threshold['name']} temperature {temp_threshold['temp']}C is larger than critical threshold {temp_threshold.get(TEMP_LEVEL_CRITICAL)}C.")
                fancontrol_crit("%%FANCONTROL-2-TEMP_HIGH: %s temperature %sC is larger than critical threshold %sC." %
                                (temp_threshold['name'], temp_threshold['temp'], temp_threshold.get(TEMP_LEVEL_CRITICAL)), temp_threshold['name'])
                self.log_sff_over_temp_if_changed(TEMP_LEVEL_CRITICAL, temp_threshold)
                if not temp_threshold['ignore_threshold_pwm']:
                    self.__critical_pwm_flag = True

        return critical_flag, event_list

    def checkTempCritical(self):
        try:
            critical_flag, event_list = self.is_temp_critical()
            if critical_flag:
                if self.__critical_pwm_flag is True:
                    self.__critical_countdown = self.__temp_critical_countdown
                fancontrol_debug("temp is over critical")
                return True, event_list
            if self.__critical_countdown > 0:
                self.__critical_countdown -= 1
            return False, []
        except Exception as e:
            fancontrol_error("%%policy: checkTempCrit failed")
            fancontrol_error(str(e))
        return False, []

    def is_temp_emergency(self):
        emergency_flag = False
        self.__emergency_pwm_flag = False
        event_list = []
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['emergency_flag'] = False
            if temp_threshold['ignore_threshold']:
                continue
            if temp_threshold['emergency_num'] >= self.__temp_emergency_num:
                emergency_flag = True
                temp_threshold['emergency_flag'] = True
                event_list.append(f"{temp_threshold['name']} temperature {temp_threshold['temp']}C is larger than emergency threshold {temp_threshold.get(TEMP_LEVEL_EMERGENCY)}C.")
                fancontrol_alert("%%FANCONTROL-1-TEMP_HIGH: %s temperature %sC is larger than emergency threshold %sC." %
                                 (temp_threshold['name'], temp_threshold['temp'], temp_threshold.get(TEMP_LEVEL_EMERGENCY)), temp_threshold['name'])
                self.log_sff_over_temp_if_changed(TEMP_LEVEL_EMERGENCY, temp_threshold)
                if not temp_threshold['ignore_threshold_pwm']:
                    self.__emergency_pwm_flag = True
        return emergency_flag, event_list

    def checkTempEmergency(self):
        try:
            temp_emerg_flag , event_list = self.is_temp_emergency()
            if temp_emerg_flag:
                if self.__emergency_pwm_flag is True:
                    self.__emergency_countdown = self.__temp_emergency_countdown
                fancontrol_debug("temp is over emergency")
                return True, event_list
            if self.__emergency_countdown > 0:
                self.__emergency_countdown -= 1
            return False, []
        except Exception as e:
            fancontrol_error("%%policy: checkTempEmergency failed")
            fancontrol_error(str(e))
        return False, []

    def checkTempCriticalCountdown(self):
        if self.__critical_countdown > 0:
            return True
        return False

    def checkTempEmergencyCountdown(self):
        if self.__emergency_countdown > 0:
            return True
        return False

    def checkTempRebootCrit(self):
        try:
            if self.is_temp_critical():
                if self.__check_temp_critical_reboot:
                    temp_crit_reboot_flag = 0
                    for temp_list in self.__check_temp_critical_reboot:
                        for temp in temp_list:
                            tmp = self.__temps_threshold_config.get(temp)
                            if tmp['critical_flag'] is False:
                                fancontrol_debug("temp_list %s, temp: %s not critical" % (temp_list, temp))
                                temp_crit_reboot_flag = 0
                                break
                            if SWITCH_TEMP in temp_list:
                                temp_crit_reboot_flag = TEMP_REBOOT_CRIT_SWITCH_FLAG
                            else:
                                temp_crit_reboot_flag = TEMP_REBOOT_CRIT_OTHER_FLAG
                        if temp_crit_reboot_flag != 0:
                            fancontrol_debug("temp_list %s, all temp is over critical reboot" % temp_list)
                            return temp_crit_reboot_flag
                    return 0
                else:
                    temp_dict = dict(self.__temps_threshold_config)
                    tmp = temp_dict.get(SWITCH_TEMP)
                    if tmp['critical_flag'] is True:
                        fancontrol_debug("switch temp is over reboot critical")
                        return TEMP_REBOOT_CRIT_SWITCH_FLAG

                    if PRODUCT_STRATEGY == PRODUCT_STRATEGY_1:
                        pass
                    else:
                        del temp_dict[SWITCH_TEMP]
                        for temp_items in temp_dict.values():
                            if temp_items['ignore_threshold']:
                                continue
                            if temp_items['critical_flag'] is False:
                                return 0

                        fancontrol_debug("other temp is over reboot critical")
                        return TEMP_REBOOT_CRIT_OTHER_FLAG
        except Exception as e:
            fancontrol_error("%%policy: checkTempRebootCrit failed")
            fancontrol_error(str(e))
        return 0

    def over_temp_reboot_func(self):
        if self.__deal_over_temp_reboot_cmd:
            if self.__deal_over_temp_reboot_pwm:
                fancontrol_debug("set pwm: %s" % self.__deal_over_temp_reboot_pwm)
                self.set_all_fan_speed_pwm(self.__deal_over_temp_reboot_pwm)
            for command in self.__deal_over_temp_reboot_cmd:
                fancontrol_debug("run deal_over_temp_reboot_cmd: %s" % command)
                ret, log = set_value(command)
                if ret is False:
                    fancontrol_error("do deal_over_temp_reboot_cmd: %s failed, msg: %s" % ( command, log))
                    return False, log
                msg = "deal_over_temp_reboot_cmd set success"
                fancontrol_debug(msg)
            return True, msg
        else:
            os.system(self.__check_crit_recover_cmd)

    def checkCritReboot(self):
        try:
            reboot_flag = self.checkTempRebootCrit()
            if reboot_flag > 0:
                self.set_all_fan_speed_pwm(self.__critical_pwm)
                for i in range(self.__check_crit_reboot_num):
                    time.sleep(self.__check_crit_sleep_time)
                    self.get_monitor_temp()
                    if self.__check_alert_reboot_flag == 1:
                        self.checkAlertReboot()
                    reboot_flag = self.checkTempRebootCrit()
                    if reboot_flag > 0:
                        fancontrol_emerg("%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot critical value lasts for %d seconds." %
                                         (self.__check_crit_sleep_time * (i + 1)), self.__check_crit_sleep_time * (i + 1))
                        continue
                    fancontrol_debug("The temperature of device is not over reboot critical value.")
                    break
                if reboot_flag > 0:
                    fancontrol_emerg(
                        "%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot critical value, system is going to reboot now.")
                    for temp_threshold in self.__temps_threshold_config.values():
                        fancontrol_emerg(
                            "%%FANCONTROL-0-TEMP_EMERG: %s temperature: %sC." %
                            (temp_threshold['name'], temp_threshold['temp']), temp_threshold['name'])
                    if reboot_flag == TEMP_REBOOT_CRIT_SWITCH_FLAG:
                        create_judge_file = "touch %s" % self.__otp_switch_reboot_judge_file
                    else:
                        create_judge_file = "touch %s" % self.__otp_other_reboot_judge_file
                    exec_os_cmd(create_judge_file)
                    exec_os_cmd("sync")
                    time.sleep(3)
                    self.over_temp_reboot_func()
        except Exception as e:
            fancontrol_error("%%policy: checkCritReboot failed")
            fancontrol_error(str(e))

    def is_temp_alert(self):
        alert_flag = False
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['alert_flag'] = False
            if temp_threshold['ignore_threshold']:
                continue
            if temp_threshold['alert_num'] >= self.__temp_alert_num:
                alert_flag = True
                temp_threshold['alert_flag'] = True
                fancontrol_alert("%%FANCONTROL-2-TEMP_HIGH: %s temperature %sC is larger than alert threshold %sC." %
                                 (temp_threshold['name'], temp_threshold['temp'], temp_threshold.get('alert')))
                self.log_sff_over_temp_if_changed(TEMP_LEVEL_ALERT, temp_threshold)
        return alert_flag

    def checkTempRebootAlert(self):
        try:
            if self.is_temp_alert():
                if self.__check_temp_alert_reboot:
                    temp_alert_reboot_flag = 0
                    for temp_list in self.__check_temp_alert_reboot:
                        for temp in temp_list:
                            tmp = self.__temps_threshold_config.get(temp)
                            if tmp['alert_flag'] is False:
                                fancontrol_debug("temp_list %s, temp: %s not alert" % (temp_list, temp))
                                temp_alert_reboot_flag = 0
                                break
                            if SWITCH_TEMP in temp_list:
                                temp_alert_reboot_flag = TEMP_REBOOT_CRIT_SWITCH_FLAG
                            else:
                                temp_alert_reboot_flag = TEMP_REBOOT_CRIT_OTHER_FLAG
                        if temp_alert_reboot_flag != 0:
                            fancontrol_debug("temp_list %s, all temp is over alert reboot" % temp_list)
                            return temp_alert_reboot_flag
                    return 0
        except Exception as e:
            fancontrol_error("%%policy: checkTempRebootAlert failed")
            fancontrol_error(str(e))
        return 0

    def checkAlertReboot(self):
        try:
            reboot_flag = self.checkTempRebootAlert()
            if reboot_flag > 0:
                for i in range(self.__check_alert_reboot_num):
                    time.sleep(self.__check_alert_sleep_time)
                    self.get_monitor_temp()
                    reboot_flag = self.checkTempRebootAlert()
                    if reboot_flag > 0:
                        fancontrol_emerg("%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot alert value lasts for %f seconds." %
                                         (self.__check_alert_sleep_time * (i + 1)))
                        continue
                    fancontrol_debug("The temperature of device is not over reboot alert value.")
                    break
                if reboot_flag > 0:
                    fancontrol_emerg(
                        "%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot alert value, system is going to reboot now.")
                    for temp_threshold in self.__temps_threshold_config.values():
                        fancontrol_emerg(
                            "%%FANCONTROL-TEMP_EMERG: %s temperature: %sC." %
                            (temp_threshold['name'], temp_threshold['temp']))
                    if reboot_flag == TEMP_REBOOT_CRIT_SWITCH_FLAG:
                        create_judge_file = "touch %s" % self.__otp_switch_reboot_judge_file
                    else:
                        create_judge_file = "touch %s" % self.__otp_other_reboot_judge_file
                    exec_os_cmd(create_judge_file)
                    exec_os_cmd("sync")
                    self.over_temp_reboot_func()
        except Exception as e:
            fancontrol_error("%%policy: checkAlertReboot failed")
            fancontrol_error(str(e))

    def checkTempRebootEmerg(self):
        try:
            temp_emerg_flag , event_list = self.is_temp_emergency()
            if temp_emerg_flag:
                temp_emerg_reboot_flag = False
                for temp_list in self.__check_temp_emergency_reboot:
                    for temp in temp_list:
                        tmp = self.__temps_threshold_config.get(temp)
                        if tmp['emergency_flag'] is False:
                            fancontrol_debug("temp_list %s, temp: %s not emergency" % (temp_list, temp))
                            temp_emerg_reboot_flag = False
                            break
                        temp_emerg_reboot_flag = True
                    if temp_emerg_reboot_flag is True:
                        fancontrol_debug("temp_list %s, all temp is over emergency reboot" % temp_list)
                        return True, event_list
        except Exception as e:
            fancontrol_error("%%policy: checkTempRebootEmerg failed")
            fancontrol_error(str(e))
        return False, []

    def checkEmergReboot(self):
        try:
            reboot_flag = False
            if self.checkTempRebootEmerg() is True:
                self.set_all_fan_speed_pwm(self.__emerg_pwm)
                for i in range(self.__check_emerg_reboot_num):
                    time.sleep(self.__check_emerg_sleep_time)
                    self.get_monitor_temp()
                    if self.__check_alert_reboot_flag == 1:
                        self.checkAlertReboot()
                    if self.checkTempRebootEmerg() is True:
                        fancontrol_emerg("%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot emergency value lasts for %d seconds." %
                                         (self.__check_emerg_sleep_time * (i + 1)), self.__check_emerg_sleep_time * (i + 1))
                        reboot_flag = True
                        continue
                    fancontrol_debug("The temperature of device is not over reboot emergency value.")
                    reboot_flag = False
                    break
                if reboot_flag is True:
                    fancontrol_emerg(
                        "%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot emergency value, system is going to reboot now.")
                    for temp_threshold in self.__temps_threshold_config.values():
                        fancontrol_emerg(
                            "%%FANCONTROL-0-TEMP_EMERG: %s temperature: %sC." %
                            (temp_threshold['name'], temp_threshold['temp']))
                    create_judge_file = "touch %s" % OTP_SWITCH_REBOOT_JUDGE_FILE
                    exec_os_cmd(create_judge_file)
                    exec_os_cmd("sync")
                    time.sleep(3)
                    self.over_temp_reboot_func()
        except Exception as e:
            fancontrol_error("%%policy: checkEmergReboot failed")
            fancontrol_error(str(e))

    def all_fan_error_checkTempRebootCrit(self):
        try:
            temp_dict = dict(self.__temps_threshold_config)
            tmp = temp_dict.get(SWITCH_TEMP)
            switch_temp_value = tmp['temp']
            if switch_temp_value >= self.__all_fan_error_switch_temp_critical_temp:
                fancontrol_debug("all fan error, switch temp[%d] is over critical[%d]."
                                 % (switch_temp_value, self.__all_fan_error_switch_temp_critical_temp))
                return True
        except Exception as e:
            fancontrol_error("%%policy: all_fan_error_checkTempRebootCrit failed")
            fancontrol_error(str(e))
        return False

    def all_fan_error_checkCritReboot(self):
        try:
            reboot_flag = self.all_fan_error_checkTempRebootCrit()
            if reboot_flag > 0:
                self.set_all_fan_speed_pwm(self.__max_pwm)
                for i in range(self.__all_fan_error_check_crit_reboot_num):
                    time.sleep(self.__all_fan_error_check_crit_sleep_time)
                    self.get_monitor_temp()
                    reboot_flag = self.all_fan_error_checkTempRebootCrit()
                    if reboot_flag > 0:
                        fancontrol_emerg("%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot critical value lasts for %d seconds." %
                                         (self.__all_fan_error_check_crit_sleep_time * (i + 1)), (self.__all_fan_error_check_crit_sleep_time * (i + 1)))
                        continue
                    else:
                        fancontrol_debug("The temperature of device is not over reboot critical value.")
                        break
                if reboot_flag > 0:
                    fancontrol_emerg("%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot critical value.")
                    fancontrol_emerg(self.__all_fan_error_recover_log)
                    exec_os_cmd("sync")
                    time.sleep(3)
                    exec_os_cmd(self.__all_fan_error_recover_cmd)
        except Exception as e:
            fancontrol_error("%%policy: all_fan_error_checkCritReboot failed")
            fancontrol_error(str(e))

    def all_fan_absent_poweroff_system(self):
        try:
            fancontrol_emerg("%%FANCONTROL-0-ABSENT_EMERG:  All fans are absent, system is going to power off now.")
            hal_fanctrl_syslog_err("hal_fanctrl", ALL_FAN_ABSENT_LOG)
            exec_os_cmd("sync")
            time.sleep(3)
            exec_os_cmd(self.__all_fan_absent_power_off_cmd)
        except Exception as e:
            fancontrol_error("%%policy: all_fan_absent_poweroff_system failed")
            fancontrol_error(str(e))

    def get_fan_total_number(self):
        return self.int_case.get_fan_total_number()

    def get_rotor_number(self, fan_name):
        return self.int_case.get_fan_rotor_number(fan_name)

    def get_fan_presence(self, fan_name):
        return self.int_case.get_fan_presence(fan_name)

    def get_fan_rotor_status(self, fan_name, rotor_name):
        return self.int_case.get_fan_rotor_status(fan_name, rotor_name)

    def get_psu_total_number(self):
        return self.int_case.get_psu_total_number()

    def get_psu_presence(self, psu_name):
        return self.int_case.get_psu_presence(psu_name)

    def get_psu_input_output_status(self, psu_name):
        return self.int_case.get_psu_input_output_status(psu_name)

    def checkFanPresence(self):
        absent_num = 0
        event_list = []
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            rotor_num = self.get_rotor_number(fan_name)
            tmp_fan = self.__fan_rotor_error_num.get(fan_name)
            status = self.get_fan_presence(fan_name)
            if status is False:
                absent_num = absent_num + 1
                self.__fan_present_status[fan_name] = 0
                event_list.append(fan_name)
                fancontrol_debug("%s absent" % fan_name)
            else:
                if self.__fan_present_status[fan_name] == 0:    # absent -> present
                    self.__pre_fan_nok = PRE_FAN_NOK_UNKNOWN
                    self.__fan_plug_in_countdown = self.__fan_plug_in_default_countdown
                    self.__fan_repair_flag[fan_name] = 1
                    for j in range(rotor_num):
                        rotor_name = "Rotor" + str(j + 1)
                        tmp_fan[rotor_name] = 0
                self.__fan_present_status[fan_name] = 1
                fancontrol_debug("%s presence" % fan_name)

        return absent_num, event_list

    def get_fan_rated_speed(self, rotor_name, pwm):
        pwm_id = int(pwm / 10)
        rotor_threshold_config = self.__fan_rotor_threshold_config[rotor_name]
        if (pwm_id < 0) or (pwm_id >= len(rotor_threshold_config)):
            rated_speed = 0
            fancontrol_debug("pwm_id = %d, len(rotor_threshold_config) = %d" % (pwm_id, len(rotor_threshold_config)))
        else:
            rated_speed = rotor_threshold_config[pwm_id]['rpm']
        return rated_speed

    def checkFanRotorStatus_general(self, fan_name, rotor_index, event_list):
        tmp_fan = self.__fan_rotor_error_num.get(fan_name)
        rotor_name = "Rotor" + str(rotor_index + 1)
        status = self.get_fan_rotor_status(fan_name, rotor_name)
        if status is True:
            tmp_fan[rotor_name] = 0
            fancontrol_debug("%s %s ok" % (fan_name, rotor_name))
        else:
            tmp_fan[rotor_name] += 1
            if tmp_fan[rotor_name] >= self.__rotor_error_count:
                event_list.append("%s-%s" % (fan_name, rotor_name))
                fancontrol_debug("%s %s error" % (fan_name, rotor_name))
            fancontrol_debug("%s %s error %d times" % (fan_name, rotor_name, tmp_fan[rotor_name]))

        return status

    def checkFanRotorStatus_strategy_default(self, fan_name, rotor_index, event_list):
        return self.checkFanRotorStatus_general(fan_name, rotor_index, event_list)

    def checkFanRotorStatus_strategy_1(self, fan_name, rotor_index, event_list):
        status  = self.checkFanRotorStatus_general(fan_name, rotor_index, event_list)
        if status is False:
            return status

        if len(self.__fan_rotor_threshold_config) != 0:
            tmp_pwm = self.get_speed_pwm(fan_name, rotor_index + 1)
            tmp_speed = self.get_speed(fan_name, rotor_index + 1)
            rotor_name = "Rotor" + str(rotor_index + 1)
            rated_speed = self.get_fan_rated_speed(rotor_name, tmp_pwm)
            if (tmp_speed < self.lowest_rpm) or (tmp_speed < rated_speed * self.fan_rpm_check_warning / 100):
                status = False
                event_list.append("%s-%s" % (fan_name, rotor_name))
                fancontrol_debug("%s %s error" % (fan_name, rotor_name))
                fancontrol_debug("pwm = %d, speed = %d, rated_speed = %d" % (tmp_pwm, tmp_speed, rated_speed))
            else:
                fancontrol_debug("%s %s ok" % (fan_name, rotor_name))
                fancontrol_debug("pwm = %d, speed = %d, rated_speed = %d" % (tmp_pwm, tmp_speed, rated_speed))

        return status

    def checkFanRotorStatus(self):
        err_rotor_num = 0
        event_list = []
        self.__fan_nok_num = 0
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            rotor_num = self.get_rotor_number(fan_name)
            fan_rotor_err_cnt = 0
            for j in range(rotor_num):
                if PRODUCT_STRATEGY == PRODUCT_STRATEGY_1:
                    rotor_status  = self.checkFanRotorStatus_strategy_1(fan_name, j, event_list)
                else:
                    rotor_status = self.checkFanRotorStatus_strategy_default(fan_name, j, event_list)

                if rotor_status is False:
                    err_rotor_num += 1
                    fan_rotor_err_cnt += 1

            if fan_rotor_err_cnt == 0:
                self.__fan_rotate_status[fan_name] = 1  # FAN is ok
            else:
                self.__fan_rotate_status[fan_name] = 0  # FAN is not ok
                self.__fan_nok_num += 1
        fancontrol_debug("fan not ok number:%d." % self.__fan_nok_num)

        return err_rotor_num, event_list

    def checkPsuPresence(self):
        absent_num = 0
        event_list = []
        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            status = self.get_psu_presence(psu_name)
            if status is False:
                absent_num = absent_num + 1
                event_list.append(psu_name)
                fancontrol_debug("%s absent" % psu_name)
            else:
                fancontrol_debug("%s presence" % psu_name)
        return absent_num, event_list

    '''
    def checkPsuStatus(self):
        err_num = 0
        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            status = self.get_psu_input_output_status(psu_name)
            if status is False:
                err_num = err_num + 1
                fancontrol_debug("%s error" % psu_name)
            else:
                fancontrol_debug("%s ok" % psu_name)
        return err_num
    '''
    def checkDevError(self):
        dev_err_flag = False
        event_list = []
        switchtemp = self.__temps_threshold_config.get(SWITCH_TEMP)['temp']
        inlettemp = self.__temps_threshold_config.get(INLET_TEMP)['temp']
        temp_diff = abs(switchtemp - inlettemp)
        fancontrol_debug("|switchtemp - inlettemp| = %d" % temp_diff)
        if temp_diff >= self.__inlet_mac_diff:
            dev_err_flag = True
            fancontrol_debug("temp_diff is over than inlet_mac_diff(%d)" % self.__inlet_mac_diff)
            detail = f"The temperature difference is greater than {self.__inlet_mac_diff} degrees, SWITCH:{switchtemp}, INLET:{inlettemp}"
            event_list.append(detail)
        return dev_err_flag, event_list

    def checktempfail(self):
        event_list = []
        temp_fail_flag = False
        for temp in self.__check_temp_fail:
            temp_name = temp.get("temp_name")
            temp_fail_num = self.__temps_threshold_config.get(temp_name)['fail_num']
            if temp_fail_num >= self.__temp_fail_num:
                temp_fail_flag = True
                event_list.append(f"{temp_name} fail time is greater than {self.__temp_fail_num}")
                fancontrol_debug("%s temp_fail_num = %d" % (temp_name, temp_fail_num))
                fancontrol_debug("self.__temp_fail_num = %d" % self.__temp_fail_num)
        return temp_fail_flag, event_list

    def abnormal_check(self):
        pwm_list = []
        pwm_min = self.__min_pwm
        pwm_list.append(pwm_min)

        self.__fan_absent_num, event_list = self.checkFanPresence()
        if self.__fan_absent_num >= self.__fan_absent_fullspeed_num:
            fan_absent_pwm = self.__max_pwm
            pwm_list.append(fan_absent_pwm)
            fancontrol_debug("fan_absent_pwm = 0x%x" % fan_absent_pwm)
            self.abnormal_event_process(EVT_FAN_ABSENT, True, event_list)
        else:
            self.abnormal_event_process(EVT_FAN_ABSENT, False, [])

        rotor_err_num, event_list = self.checkFanRotorStatus()
        if rotor_err_num >= self.__rotor_error_fullspeed_num:
            rotor_err_pwm = self.__max_pwm
            pwm_list.append(rotor_err_pwm)
            fancontrol_debug("rotor_err_pwm = 0x%x" % rotor_err_pwm)
            self.abnormal_event_process(EVT_FAN_ROTOR_ERR, True, event_list)
        else:
            self.abnormal_event_process(EVT_FAN_ROTOR_ERR, False, [])

        if self.__deal_all_fan_error_method_flag:
            fan_num = self.get_fan_total_number()
            # all fan absent or fail
            if (self.__fan_absent_num == fan_num) or (self.__fan_nok_num == fan_num):
                event_list = ["All fan error or absent."]
                fancontrol_debug("All fan error or absent.")
                self.abnormal_event_process(EVT_ALL_FAN_ERROR, True, event_list)
                if self.__deal_all_fan_absent_method_flag:
                    self.all_fan_absent_poweroff_system()
                self.all_fan_error_checkCritReboot()
            else :
                self.abnormal_event_process(EVT_ALL_FAN_ERROR, False, [])

        if self.__check_alert_reboot_flag == 1:
            self.checkAlertReboot()

        if self.__check_temp_emergency == 1:
            status, event_list = self.checkTempEmergency()
            if status is True:
                # do reset check
                if self.__check_emerg_reboot_flag == 1:
                    self.checkEmergReboot()
            if self.__emergency_pwm_flag is True:
                self.abnormal_event_process(EVT_TEMP_EMERGENCY, True, event_list)
                over_emerg_pwm = self.__emerg_pwm
                pwm_list.append(over_emerg_pwm)
                fancontrol_debug("over_emerg_pwm = 0x%x" % over_emerg_pwm)
            else:
                if self.checkTempEmergencyCountdown() is True:  # temp lower than emergency in 5 min
                    over_emerg_countdown_pwm = self.__emerg_pwm
                    pwm_list.append(over_emerg_countdown_pwm)
                    fancontrol_debug("TempEmergencyCountdown: %d, over_emerg_countdown_pwm = 0x%x" %
                                     (self.__emergency_countdown, over_emerg_countdown_pwm))
                else:
                    self.abnormal_event_process(EVT_TEMP_EMERGENCY, False, [])

        if self.__check_temp_critical == 1:

            status, event_list = self.checkTempCritical()
            if status is True:
                # do reset check
                if self.__check_crit_reboot_flag == 1:
                    self.checkCritReboot()

            if self.__critical_pwm_flag is True:
                self.abnormal_event_process(EVT_TEMP_CRITICAL, True, event_list)
                over_crit_pwm = self.__critical_pwm
                pwm_list.append(over_crit_pwm)
                fancontrol_debug("over_crit_pwm = 0x%x" % over_crit_pwm)
            else:
                if self.checkTempCriticalCountdown() is True:  # temp lower than critical in 5 min
                    over_crit_countdown_pwm = self.__critical_pwm
                    pwm_list.append(over_crit_countdown_pwm)
                    fancontrol_debug("TempCriticalCountdown: %d, over_crit_countdown_pwm = 0x%x" %
                                     (self.__critical_countdown, over_crit_countdown_pwm))
                else:
                    self.abnormal_event_process(EVT_TEMP_CRITICAL, False, [])

        if self.__check_temp_warning == 1:
            status, event_list = self.checkTempWarning()
            if self.__warning_pwm_flag is True:
                self.abnormal_event_process(EVT_TEMP_WARNING, True, event_list)
                over_warn_pwm = self.__warning_pwm
                pwm_list.append(over_warn_pwm)
                fancontrol_debug("over_warn_pwm = 0x%x" % over_warn_pwm)
            else:
                if self.checkTempWarningCountdown() is True:  # temp lower than warning in 5 min
                    over_warn_countdown_pwm = self.__warning_pwm
                    pwm_list.append(over_warn_countdown_pwm)
                    fancontrol_debug("TempWarningCountdown: %d, over_warn_countdown_pwm = 0x%x" %
                                     (self.__warning_countdown, over_warn_countdown_pwm))
                else:
                    self.abnormal_event_process(EVT_TEMP_WARNING, False, [])

        psu_absent_num, event_list = self.checkPsuPresence()
        if psu_absent_num >= self.__psu_absent_fullspeed_num:
            psu_absent_pwm = self.__max_pwm
            pwm_list.append(psu_absent_pwm)
            fancontrol_debug("psu_absent_pwm = 0x%x" % psu_absent_pwm)
            self.abnormal_event_process(EVT_PSU_ABSENT, True, event_list)
        else:
            self.abnormal_event_process(EVT_PSU_ABSENT, False, [])

        if self.__inlet_mac_diff_flag == 1:
            dev_err_flag, event_list = self.checkDevError()
            self.abnormal_event_process(EVT_TEMP_DIFF, dev_err_flag, event_list)
            if dev_err_flag:
                dev_err_pwm = self.__abnormal_pwm
            else:
                dev_err_pwm = self.__min_pwm
            pwm_list.append(dev_err_pwm)
            fancontrol_debug("dev_err_pwm = 0x%x" % dev_err_pwm)

        temp_fail_flag, event_list = self.checktempfail()
        self.abnormal_event_process(EVT_TEMP_SENSOR_FAIL, temp_fail_flag, event_list)
        if temp_fail_flag:
            temp_fail_pwm = self.__abnormal_pwm
        else:
            temp_fail_pwm = self.__min_pwm
        pwm_list.append(temp_fail_pwm)
        fancontrol_debug("temp_fail_pwm = 0x%x" % temp_fail_pwm)

        pwm = max(pwm_list)
        return pwm

    def get_error_fan(self):
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            if self.__fan_rotate_status[fan_name] == 0:
                return fan_name
        return None

    def fan_error_update_pwm(self, fan_pwm_dict):
        try:
            fancontrol_debug("enter deal fan error policy")
            ori_fan_pwm_dict = fan_pwm_dict.copy()

            err_fan_name = self.get_error_fan()
            if err_fan_name is None:
                fancontrol_debug("fan name is None, do nothing.")
                return ori_fan_pwm_dict

            if self.__fan_repair_flag[err_fan_name] == 0:
                fancontrol_debug("%s already repaired, do nothing." % err_fan_name)
                return ori_fan_pwm_dict

            if self.__pre_fan_nok != err_fan_name:
                fancontrol_debug(
                    "not ok fan change from %s to %s, update countdown." %
                    (self.__pre_fan_nok, err_fan_name))
                self.__deal_fan_error_countdown = self.__deal_fan_error_default_countdown
                if self.__pre_fan_nok != PRE_FAN_NOK_UNKNOWN:
                    fancontrol_debug(
                        "%s repaire success, %s NOT OK, try to repaire." %
                        (self.__pre_fan_nok, err_fan_name))
                    self.__fan_repair_flag[self.__pre_fan_nok] = 0
                self.__pre_fan_nok = err_fan_name

            if self.__deal_fan_error_countdown > 0:
                self.__deal_fan_error_countdown -= 1
            fancontrol_debug("%s repaire, countdown %d." % (err_fan_name, self.__deal_fan_error_countdown))

            if self.__deal_fan_error_countdown == 0:
                self.__fan_repair_flag[err_fan_name] = 0
                fancontrol_debug("%s set repaire fail flag, use origin pwm." % err_fan_name)
                return ori_fan_pwm_dict

            fan_err_pwm_conf_list = self.__deal_fan_error_conf[err_fan_name]
            for item in fan_err_pwm_conf_list:
                fan_pwm_dict[item["name"]] = item["pwm"]
            fancontrol_debug("fan pwm update, fan pwm dict:%s" % fan_pwm_dict)

            return fan_pwm_dict
        except Exception as e:
            fancontrol_error("%%policy: deal_fan_error raise Exception:%s" % str(e))
            self.__pre_fan_nok = PRE_FAN_NOK_UNKNOWN
        return ori_fan_pwm_dict

    def get_fan_pwm_dict(self, default_pwm):
        fan_pwm_dict = {}
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            fan_pwm_dict[fan_name] = default_pwm
        if self.__deal_fan_error_policy:
            if self.__fan_absent_num == 0 and self.__fan_nok_num == 1:
                fan_pwm_dict = self.fan_error_update_pwm(fan_pwm_dict)
            else:
                if self.__pre_fan_nok != PRE_FAN_NOK_UNKNOWN and self.__fan_rotate_status[self.__pre_fan_nok] == 1:
                    fancontrol_debug("%s repaire success." % (self.__pre_fan_nok))
                    self.__fan_repair_flag[self.__pre_fan_nok] = 0
                self.__pre_fan_nok = PRE_FAN_NOK_UNKNOWN
        return fan_pwm_dict

    def get_psu_pwm_dict(self, default_pwm):
        psu_pwm_dict = {}
        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            psu_pwm_dict[psu_name] = default_pwm
        return psu_pwm_dict

    def check_board_air_flow(self):
        board_air_flow = self.board_air_flow
        air_flow_tuple = (F2B_AIR_FLOW, B2F_AIR_FLOW, E2_F2B_AIR_FLOW, E2_B2F_AIR_FLOW)
        if board_air_flow not in air_flow_tuple:
            fanairflow_debug("get board air flow error, value [%s]" % board_air_flow)
            return False
        fanairflow_debug("board air flow check ok: %s" % board_air_flow)
        return True

    def check_fan_air_flow(self):
        if self.fan_air_flow_monitor:
            fanairflow_debug("open air flow monitor, check fan air flow")
            ret = self.check_board_air_flow()
            if ret is False:
                fanairflow_debug("get board air flow error, set fan_air_flow_inconsistent_flag False")
                self.fan_air_flow_inconsistent_flag = False
                return
            air_flow_inconsistent_flag_tmp = False
            for fan_obj in self.fan_obj_list:
                fan_obj.update_fru_info()
                fanairflow_debug("%s origin name: [%s], display name: [%s] air flow [%s]" %
                                 (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow))
                if fan_obj.air_flow == self.na_ret:
                    fanairflow_debug("%s get air flow failed, set air_flow_inconsistent flag False" % fan_obj.name)
                    fan_obj.air_flow_inconsistent = False
                    continue
                if fan_obj.air_flow != self.board_air_flow:
                    fanairflow_debug("%s air flow error, origin name: [%s], display name: [%s], fan air flow [%s], board air flow [%s]" %
                                     (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow, self.board_air_flow))
                    air_flow_inconsistent_flag_tmp = True
                    fan_obj.air_flow_inconsistent = True
                else:
                    fanairflow_debug("%s air flow check ok, origin name: [%s], display name: [%s], fan air flow: [%s], board air flow: [%s]" %
                                     (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow, self.board_air_flow))
                    fan_obj.air_flow_inconsistent = False
            self.fan_air_flow_inconsistent_flag = air_flow_inconsistent_flag_tmp
        else:
            fanairflow_debug("air flow monitor not open, set fan_air_flow_inconsistent_flag False")
            self.fan_air_flow_inconsistent_flag = False
        return

    def check_psu_air_flow(self):
        if self.psu_air_flow_monitor:
            fanairflow_debug("open air flow monitor, check psu air flow")
            ret = self.check_board_air_flow()
            if ret is False:
                fanairflow_debug("get board air flow error, set psu_air_flow_inconsistent_flag False")
                self.psu_air_flow_inconsistent_flag = False
                return
            air_flow_inconsistent_flag_tmp = False
            for psu_obj in self.psu_obj_list:
                psu_obj.update_fru_info()
                fanairflow_debug("%s origin name: [%s], display name: [%s] air flow [%s]" %
                                 (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow))
                if psu_obj.air_flow == self.na_ret:
                    fanairflow_debug("%s get air flow failed, set air_flow_inconsistent flag False" % psu_obj.name)
                    psu_obj.air_flow_inconsistent = False
                    continue
                if psu_obj.air_flow != self.board_air_flow:
                    fanairflow_debug("%s air flow error, origin name: [%s], display name: [%s], psu air flow [%s], board air flow [%s]" %
                                     (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow, self.board_air_flow))
                    air_flow_inconsistent_flag_tmp = True
                    psu_obj.air_flow_inconsistent = True
                else:
                    fanairflow_debug("%s air flow check ok, origin name: [%s], display name: [%s], psu air flow: [%s], board air flow: [%s]" %
                                     (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow, self.board_air_flow))
                    psu_obj.air_flow_inconsistent = False
            self.psu_air_flow_inconsistent_flag = air_flow_inconsistent_flag_tmp
        else:
            fanairflow_debug("air flow monitor not open, set psu_air_flow_inconsistent_flag False")
            self.psu_air_flow_inconsistent_flag = False
        return

    def manual_mode_check(self):
        if self.__fanctrl_mode is None:
            return False

        ret, manual_mode = get_value(self.__fanctrl_mode)
        if not ret:
            fancontrol_debug("manual_mode get fail: %s" % ret)
            return False

        if manual_mode != 0:
            self.refresh_manual_mode_start_time()

            duration = self.manual_mode_get_duration()
            if duration == 0:
                fancontrol_debug("Manual mode duration is forever.")
                return True

            elapsed_time = (_time() - self.__fanctrl_start_time)
            if elapsed_time >= duration:
                fancontrol_debug("Manual mode duration expired. Switching back to auto mode.")
                temp_config = {
                    "gettype" : "sysfs",
                    "loc" : "/sys/s3ip/fan/fanctrl_mode",
                    "value": 0,
                }
                ret, log = set_value(temp_config)
                if not ret:
                    fancontrol_error("fanctrl_mode set fail: %s" % log)
                    return False
                self.__fanctrl_start_time = None
                return False
            else:
                fancontrol_debug("Manual mode duration: %d seconds remaining." % (duration - elapsed_time))
                return True

        self.__fanctrl_start_time = None
        self.__fanctrl_last_duration_update_time = None
        return False

    def manual_mode_get_pwm(self):
        manual_pwm = self.__fanctrl_mode_pwm_def
        ret, manual_ratio = get_value(self.__fanctrl_fixed_ratio)
        if not ret:
            fancontrol_error("fanctrl_fixed_ratio get fail: %s" % ret)
            return self.__fanctrl_mode_pwm_def

        if 0 <= manual_ratio <= 100:
            manual_pwm = int(self.__max_pwm * manual_ratio / 100)

        fancontrol_debug("manual adjustment pwm get success: (ratio %d, pwm 0x%x)" % (manual_ratio, manual_pwm))
        return manual_pwm

    def dynamic_check_overtemp_reboot_flag(self):
        reboot_flag_config = {
            "loc": DYNAMIC_OVERTEMP_REBOOT_SYSFS_FILE,
            "gettype": "sysfs",
            "int_decode": 10,
        }
        ret, reboot_flag = get_value(reboot_flag_config)

        if (not ret) or (reboot_flag ==  OVERTEMP_KEEP_DEFAULT):
            self.__check_crit_reboot_flag = self.__fancontrol_para.get("check_crit_reboot_flag", OVERTEMP_REBOOT_INIT)
            self.__check_alert_reboot_flag = self.__fancontrol_para.get("check_alert_reboot_flag", OVERTEMP_NOT_REBOOT_INIT)
            self.__check_emerg_reboot_flag = self.__fancontrol_para.get("check_emerg_reboot_flag", OVERTEMP_REBOOT_INIT)
            fancontrol_debug("overtemp_reboot get fail, keep default value" )
            return

        if reboot_flag in [OVERTEMP_REBOOT, OVERTEMP_NOT_REBOOT]:
            self.__check_crit_reboot_flag = reboot_flag
            self.__check_alert_reboot_flag = reboot_flag
            self.__check_emerg_reboot_flag = reboot_flag
            fancontrol_debug("refresh reboot_flag=%d" % reboot_flag)
            return


    def manual_mode_get_duration(self):
        ret, manual_duration_max = get_value(self.__fanctrl_duration_max)
        if not ret:
            fancontrol_error("fanctrl_duration_max get fail: %s" % ret)
            return self.__fanctrl_duration_def

        ret, manual_duration = get_value(self.__fanctrl_duration)
        if not ret:
            fancontrol_error("fanctrl_duration get fail: %s" % ret)
            return self.__fanctrl_duration_def

        if 0 <= manual_duration <= manual_duration_max:
            fancontrol_debug("fanctrl_duration get success: %d" % manual_duration)
            return manual_duration
        else:
            fancontrol_error("fanctrl_duration out of range! use default value 0")
            return self.__fanctrl_duration_def

    def refresh_manual_mode_start_time(self):
        if self.__fanctrl_start_time is None:
            self.__fanctrl_start_time = _time()

        if self.__fanctrl_duration_update_time is None:
            return

        ret, duration_update_time = get_value(self.__fanctrl_duration_update_time)
        if not ret:
            fancontrol_debug("fanctrl_duration_update_time get fail: %s" % ret)
            return

        if self.__fanctrl_last_duration_update_time != duration_update_time:
            self.__fanctrl_last_duration_update_time = duration_update_time
            self.__fanctrl_start_time = _time()
            fancontrol_debug("fanctrl_duration rewritten, restart manual mode timer: %d" % duration_update_time)

    def record_abnormal_evt(self):
        abnormal_logs = []
        for evt in self.abnormal_events.values():
            if evt.event_status and evt.need_record():
                abnormal_logs.append(str(evt))
                evt.update_last_event()

            # error recover
            elif not evt.event_status and evt.need_record():
                recover_log = f"[{evt.err_type}] recover to normal"
                abnormal_logs.append(recover_log)
                evt.update_last_event()

        if abnormal_logs:
            all_log = "\n".join(abnormal_logs)
            fanairflow_info(all_log)

    def do_fancontrol(self):
        pwm_list = []
        pwm_min = self.__min_pwm
        pwm_list.append(pwm_min)

        # first check air flow
        self.check_fan_air_flow()
        self.check_psu_air_flow()
        if self.fan_air_flow_inconsistent_flag is True or self.psu_air_flow_inconsistent_flag is True:
            self.air_flow_inconsistent_flag = True
        else:
            self.air_flow_inconsistent_flag = False
        fanairflow_debug("check_air_flow, air_flow_inconsistent_flag: %s" % self.air_flow_inconsistent_flag)
        if self.__dynamic_check_crit_reboot_flag == 1:
            self.dynamic_check_overtemp_reboot_flag()
        # get_monitor_temp
        self.get_monitor_temp(False)
        fancontrol_debug("last_pwm = 0x%x" % self.__pwm)
        # openloop linear
        linear_flag = self.__openloop_config.get("linear", {}).get("flag", 0)
        if linear_flag == 0:
            fancontrol_debug("openloop linear flag is 0, do nothing")
        else:
            fancontrol_debug("do openloop linear policy")
            inlettemp = self.__temps_threshold_config.get(INLET_TEMP)['temp']
            linear_value = self.openloop.linear_cacl(inlettemp)
            if linear_value is None:
                linear_value = self.__min_pwm
            pwm_list.append(linear_value)
            fancontrol_debug("linear_value = 0x%x" % linear_value)

        # openloop curve
        curve_flag = self.__openloop_config.get("curve", {}).get("flag", 0)
        if curve_flag == 0:
            fancontrol_debug("openloop curve flag is 0, do nothing")
        else:
            fancontrol_debug("do openloop curve policy")
            inlettemp = self.__temps_threshold_config.get(INLET_TEMP)['temp']
            curve_value = self.openloop.curve_cacl(inlettemp)
            if curve_value is None:
                curve_value = self.__min_pwm
            pwm_list.append(curve_value)
            fancontrol_debug("curve_value = 0x%x" % curve_value)

        # hyst
        for hyst_index in self.__hyst_config.values():
            temp_name = hyst_index.get("name")
            hyst_flag = hyst_index.get("flag", 0)
            if hyst_flag == 0:
                fancontrol_debug("%s hyst flag is 0, do nothing" % temp_name)
                continue
            tmp_temp = int(self.__temps_threshold_config.get(temp_name)['temp'])  # make sure temp is int
            hyst_value = self.hyst.cacl(temp_name, tmp_temp)
            if hyst_value is None:
                hyst_value = self.__min_pwm
            pwm_list.append(hyst_value)
            fancontrol_debug("%s hyst_value = 0x%x" % (temp_name, hyst_value))

        # pid
        for pid_index in self.__pid_config.values():
            temp_name = pid_index.get("name")
            pid_flag = pid_index.get("flag", 0)
            if pid_flag == 0:
                fancontrol_debug("%s pid flag is 0, do nothing" % temp_name)
                continue
            tmp_temp = self.__temps_threshold_config.get(temp_name)['temp']
            if tmp_temp is not None:
                tmp_temp = int(tmp_temp)  # make sure temp is int
                invalid_temp_val = self.__temps_threshold_config.get(temp_name)['invalid']
                error_temp_val = self.__temps_threshold_config.get(temp_name)['error']
                if tmp_temp == invalid_temp_val:  # temp is invalid
                    temp = None
                    self.pid.cacl(self.__pwm, temp_name, temp)  # temp invalid, PID need to record None
                    pid_value = self.__temp_invalid_pid_pwm
                    fancontrol_debug("%s is invalid, pid_value = 0x%x" % (temp_name, pid_value))
                    fancontrol_debug("temp = %d, invalid_temp = %d" % (tmp_temp, invalid_temp_val))
                elif tmp_temp == error_temp_val:  # temp is error
                    temp = None
                    self.pid.cacl(self.__pwm, temp_name, temp)  # temp error, PID need to record None
                    pid_value = self.__temp_error_pid_pwm
                    fancontrol_debug("%s is error, pid_value = 0x%x" % (temp_name, pid_value))
                    fancontrol_debug("temp = %d, error_temp = %d" % (tmp_temp, error_temp_val))
                else:
                    pid_value = self.pid.cacl(self.__pwm, temp_name, tmp_temp)
            else:  # temp get failed
                pid_value = self.pid.cacl(self.__pwm, temp_name, tmp_temp)
            if pid_value is None:
                pid_value = self.__min_pwm
            pwm_list.append(pid_value)
            fancontrol_debug("%s pid_value = 0x%x" % (temp_name, pid_value))

        # abnormal
        abnormal_value = self.abnormal_check()
        pwm_list.append(abnormal_value)
        fancontrol_debug("abnormal_value = 0x%x" % abnormal_value)

        if self.__fan_plug_in_countdown > 0 and self.__fan_absent_num == 0:
            fancontrol_debug("fan plug in countdown %d, set plug in pwm: 0x%x" %
                             (self.__fan_plug_in_countdown, self.__fan_plug_in_pwm))
            self.__pwm = self.__fan_plug_in_pwm
            self.__fan_plug_in_countdown -= 1
        else:
            self.__pwm = max(pwm_list)
        fancontrol_debug("__pwm = 0x%x\n" % self.__pwm)

        ret = self.manual_mode_check()
        if ret == True:
            # into manual mode
            fancontrol_debug("into manual mode\n")
            self.__pwm = self.manual_mode_get_pwm()
            fancontrol_debug("manual mode __pwm = 0x%x\n" % self.__pwm)
            self.set_fan_speed_direct_dict(self.__pwm)
            return

        if self.air_flow_inconsistent_flag is True:
            fanairflow_debug("air flow inconsistent, set all fan speed pwm")
            self.set_all_fan_speed_pwm(self.__pwm)
        else:
            fanairflow_debug("air flow consistent, deal fan error policy")
            fan_pwm_dict = self.get_fan_pwm_dict(self.__pwm)
            psu_pwm_dict = self.get_psu_pwm_dict(self.__pwm)
            self.set_fan_pwm_independent(fan_pwm_dict, psu_pwm_dict)
        self.record_abnormal_evt()

    def run(self):
        start_time = _time()
        while True:
            try:
                debug_init()
                if os.path.exists(FAN_CONTRL_FILE) is True:
                    fancontrol_debug("file exists, do nothing!")
                    time.sleep(5)
                    continue
                if os.path.exists(FAN_CONTROL_REBOOT_FLAG_FILE) is True:
                    if self.__check_crit_reboot_flag == 0:
                        fancontrol_warn("file:%s exists, enable overtemp reboot flag!" % FAN_CONTROL_REBOOT_FLAG_FILE)
                        self.__check_crit_reboot_flag = 1
                        self.__check_emerg_reboot_flag = 1
                        self.__check_alert_reboot_flag = 1
                if self.__fan_status_interval > 0 and self.__fan_status_interval < self.__interval:
                    delta_time = _time() - start_time
                    if delta_time >= self.__interval or delta_time < 0:
                        self.do_fancontrol()
                        start_time = _time()
                    else:
                        self.checkFanPresence()
                    time.sleep(self.__fan_status_interval)
                else:
                    self.do_fancontrol()
                    time.sleep(self.__interval)
            except Exception as e:
                traceback.print_exc()
                fancontrol_error(str(e))
                time.sleep(self.__interval)

    def set_all_fan_speed_pwm(self, pwm):
        fan_pwm_dict = {}
        psu_pwm_dict = {}
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            fan_pwm_dict[fan_name] = pwm

        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            psu_pwm_dict[psu_name] = pwm
        self.set_fan_pwm_independent(fan_pwm_dict, psu_pwm_dict)

    def set_fan_pwm_independent(self, fan_pwm_dict, psu_pwm_dict):
        if self.air_flow_inconsistent_flag is True:
            for psu_obj in self.psu_obj_list:
                if psu_obj.air_flow_inconsistent is True:
                    psu_pwm_dict[psu_obj.name] = self.air_flow_error_psu_pwm
                    fanairflow_debug("%s air flow error, origin name: [%s], display name: [%s], psu air flow: [%s], board air flow: [%s], set psu pwm: 0x%x" %
                                     (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow, self.board_air_flow, self.air_flow_error_psu_pwm))
                else:
                    psu_pwm_dict[psu_obj.name] = self.air_flow_correct_psu_pwm
                    fanairflow_debug("%s air flow correct, origin name: [%s], display name: [%s], psu air flow: [%s], board air flow: [%s], set psu pwm: 0x%x" %
                                     (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow, self.board_air_flow, self.air_flow_correct_psu_pwm))

            for fan_obj in self.fan_obj_list:
                if fan_obj.air_flow_inconsistent is True:
                    fan_pwm_dict[fan_obj.name] = self.air_flow_error_fan_pwm
                    fanairflow_debug("%s air flow error, origin name: [%s], display name: [%s], fan air flow: [%s], board air flow: [%s], set fan pwm: 0x%x" %
                                     (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow, self.board_air_flow, self.air_flow_error_fan_pwm))
                else:
                    fan_pwm_dict[fan_obj.name] = self.air_flow_correct_fan_pwm
                    fanairflow_debug("%s air flow correct, origin name: [%s], display name: [%s], fan air flow: [%s], board air flow: [%s], set fan pwm: 0x%x" %
                                     (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow, self.board_air_flow, self.air_flow_correct_fan_pwm))
        self.set_fan_speed_pwm_func(fan_pwm_dict, psu_pwm_dict)


    def set_fan_speed_pwm_func(self, fan_pwm_dict, psu_pwm_dict):
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            self.fan_set_speed_pwm_by_name(fan_name, fan_pwm_dict[fan_name])
        if self.__psu_fan_control == 1:
            psu_num = self.get_psu_total_number()
            for i in range(psu_num):
                psu_name = "PSU" + str(i + 1)
                self.psu_set_speed_pwm_by_name(psu_name, psu_pwm_dict[psu_name])

    def set_fan_speed_direct_dict(self, pwm):
        fan_pwm_dict = {}
        psu_pwm_dict = {}
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            fan_pwm_dict[fan_name] = pwm

        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            psu_pwm_dict[psu_name] = pwm

        self.set_fan_speed_pwm_func(fan_pwm_dict, psu_pwm_dict)

    def get_speed(self, fan_name, rotor_index):
        return self.int_case.get_fan_speed(fan_name, rotor_index)

    def get_speed_pwm(self, fan_name, rotor_index):
        return self.int_case.get_fan_speed_pwm(fan_name, rotor_index)

    def fan_set_speed_pwm_by_name(self, fan_name, pwm):
        duty = round(pwm * 100 / 255)
        rotor_len = self.get_rotor_number(fan_name)
        for i in range(rotor_len):
            val = self.int_case.set_fan_speed_pwm(fan_name, i + 1, duty)
            if val != 0:
                fancontrol_error("%s rotor%d: %d" % (fan_name, i + 1, val))

    def psu_set_speed_pwm_by_name(self, psu_name, pwm):
        duty = round(pwm * 100 / 255)
        status = self.int_case.set_psu_fan_speed_pwm(psu_name, int(duty))
        if status is not True:
            fancontrol_error("set %s speed fail" % psu_name)

    def fan_obj_init(self):
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            fan_obj = DevFan(fan_name, self.int_case)
            self.fan_obj_list.append(fan_obj)
        fanairflow_debug("fan object initialize success")

    def psu_obj_init(self):
        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            psu_obj = DevPsu(psu_name, self.int_case)
            self.psu_obj_list.append(psu_obj)
        fanairflow_debug("psu object initialize success")

@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass

@main.command()
def start():
    '''start process '''
    debug_init()
    fanairflow_info("enter main")
    fan_control = fancontrol()
    fan_control.fan_obj_init()
    fan_control.psu_obj_init()
    fan_control.run()

@main.command()
def stop():
    '''stop process '''
    script_name = os.path.basename(sys.argv[0])
    unload_process_byPid(script_name)

@main.command()
def restart():
    stop()
    start()

if __name__ == '__main__':
    main()
