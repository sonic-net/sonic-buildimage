#!/usr/bin/env python3
import os
import subprocess
import time
import syslog
import traceback
from plat_hal.interface import interface
from plat_hal.baseutil import baseutil
from algorithm.pid import pid
from algorithm.openloop import openloop
from algorithm.hysteresis import hysteresis


SWITCH_TEMP = "SWITCH_TEMP"
INLET_TEMP = "INLET_TEMP"
BOARD_TEMP = "BOARD_TEMP"
OUTLET_TEMP = "OUTLET_TEMP"
CPU_TEMP = "CPU_TEMP"

FANCTROL_DEBUG_FILE = "/etc/.fancontrol_debug_flag"
# coordination with REBOOT_CAUSE_PARA
OTP_SWITCH_REBOOT_JUDGE_FILE = "/etc/.otp_reboot_flag"
OTP_OTHER_REBOOT_JUDGE_FILE = OTP_SWITCH_REBOOT_JUDGE_FILE

FANCTROLERROR = 1
FANCTROLDEBUG = 2
FANAIRFLOWDEBUG = 4

debuglevel = 0

F2B_AIR_FLOW = "intake"
B2F_AIR_FLOW = "exhaust"
ONIE_E2_NAME = "ONIE_E2"

TEMP_REBOOT_CRIT_SWITCH_FLAG = 1
TEMP_REBOOT_CRIT_OTHER_FLAG = 2


def fancontrol_debug(s):
    if FANCTROLDEBUG & debuglevel:
        syslog.openlog("FANCONTROL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def fancontrol_error(s):
    if FANCTROLERROR & debuglevel:
        syslog.openlog("FANCONTROL", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


def fanairflow_debug(s):
    if FANAIRFLOWDEBUG & debuglevel:
        syslog.openlog("AIR_FLOW_MONITOR", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def fancontrol_warn(s):
    syslog.openlog("FANCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_WARNING, s)


def fancontrol_crit(s):
    syslog.openlog("FANCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_CRIT, s)


def fancontrol_alert(s):
    syslog.openlog("FANCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_ALERT, s)


def fancontrol_emerg(s):
    syslog.openlog("FANCONTROL", syslog.LOG_PID)
    syslog.syslog(syslog.LOG_LOCAL1 | syslog.LOG_EMERG, s)


def exec_os_cmd(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        print(output)
    return status, output


def debug_init():
    global debuglevel
    try:
        with open(FANCTROL_DEBUG_FILE, "r") as fd:
            value = fd.read()
        debuglevel = int(value)
    except Exception:
        debuglevel = 0


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


class fancontrol(object):
    __int_case = None

    __pwm = 0x80

    def __init__(self):
        self.int_case = interface()
        self.__config = baseutil.get_monitor_config()
        self.__pid_config = self.__config["pid"]
        self.__hyst_config = self.__config.get("hyst", {})
        self.__temps_threshold_config = self.__config["temps_threshold"]
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['temp'] = 0
            temp_threshold['fail_num'] = 0
            temp_threshold['warning_num'] = 0  # temp warning times
            temp_threshold['critical_num'] = 0  # temp critical times
            temp_threshold['emergency_num'] = 0  # temp emergency times
            temp_threshold.setdefault('ignore_threshold', 0)  # default temp threshold on
            temp_threshold.setdefault('invalid', invalid_temp)
            temp_threshold.setdefault('error', error_temp)

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
        self.__temp_invalid_pid_pwm = self.__fancontrol_para.get("temp_invalid_pid_pwm", 0x80)
        self.__temp_error_pid_pwm = self.__fancontrol_para.get("temp_error_pid_pwm", 0x80)
        self.__temp_fail_num = self.__fancontrol_para.get("temp_fail_num", 3)
        self.__check_temp_fail = self.__fancontrol_para.get("check_temp_fail", [])
        self.__temp_warning_num = self.__fancontrol_para.get("temp_warning_num", 3)
        self.__temp_critical_num = self.__fancontrol_para.get("temp_critical_num", 3)
        self.__temp_emergency_num = self.__fancontrol_para.get("temp_emergency_num", 3)
        self.__temp_warning_countdown = self.__fancontrol_para.get("temp_warning_countdown", 60)
        self.__temp_critical_countdown = self.__fancontrol_para.get("temp_critical_countdown", 60)
        self.__temp_emergency_countdown = self.__fancontrol_para.get("temp_emergency_countdown", 60)
        self.__rotor_error_count = self.__fancontrol_para.get("rotor_error_count", 6)
        self.__inlet_mac_diff = self.__fancontrol_para.get("inlet_mac_diff", 50)
        self.__check_crit_reboot_flag = self.__fancontrol_para.get("check_crit_reboot_flag", 1)
        self.__check_emerg_reboot_flag = self.__fancontrol_para.get("check_emerg_reboot_flag", 1)
        self.__check_crit_reboot_num = self.__fancontrol_para.get("check_crit_reboot_num", 3)
        self.__check_crit_sleep_time = self.__fancontrol_para.get("check_crit_sleep_time", 20)
        self.__check_emerg_reboot_num = self.__fancontrol_para.get("check_emerg_reboot_num", 3)
        self.__check_emerg_sleep_time = self.__fancontrol_para.get("check_emerg_sleep_time", 20)
        self.__check_temp_emergency = self.__fancontrol_para.get("check_temp_emergency", 0)
        self.__check_temp_critical = self.__fancontrol_para.get("check_temp_critical", 1)
        self.__check_temp_warning = self.__fancontrol_para.get("check_temp_warning", 1)
        self.__check_temp_emergency_reboot = self.__fancontrol_para.get("check_temp_emergency_reboot", [])
        self.__psu_absent_fullspeed_num = self.__fancontrol_para.get("psu_absent_fullspeed_num", 1)
        self.__fan_absent_fullspeed_num = self.__fancontrol_para.get("fan_absent_fullspeed_num", 1)
        self.__rotor_error_fullspeed_num = self.__fancontrol_para.get("rotor_error_fullspeed_num", 1)
        self.__psu_fan_control = self.__fancontrol_para.get("psu_fan_control", 1)  # default control psu fan
        self.__fan_plug_in_pwm = self.__fancontrol_para.get("fan_plug_in_pwm", 0x80)
        self.__fan_plug_in_default_countdown = self.__fancontrol_para.get("fan_plug_in_default_countdown", 0)
        self.__deal_fan_error_policy = self.__fancontrol_para.get("deal_fan_error", 0)
        self.__deal_fan_error_conf = self.__fancontrol_para.get("deal_fan_error_conf", {})
        self.__deal_fan_error_default_countdown = self.__deal_fan_error_conf.get("countdown", 0)

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

    @property
    def na_ret(self):
        return self.int_case.na_ret

    def get_onie_e2_obj(self, name):
        return self.int_case.get_onie_e2_obj(name)

    @property
    def board_air_flow(self):
        air_flow_tuple = (F2B_AIR_FLOW, B2F_AIR_FLOW)
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

    def update_over_temp_threshold_num(self):
        for temp_threshold in self.__temps_threshold_config.values():
            if temp_threshold['ignore_threshold']:
                continue
            emergency_threshold = temp_threshold.get('emergency', None)
            critical_threshold = temp_threshold.get('critical', None)
            warning_threshold = temp_threshold.get('warning', None)
            fancontrol_debug("%s warning = %s, critical = %s, emergency = %s" %
                             (temp_threshold['name'], warning_threshold, critical_threshold, emergency_threshold))

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

            fancontrol_debug("%s warning_num = %d, critical_num = %d, emergency_num = %d" %
                             (temp_threshold['name'], temp_threshold['warning_num'], temp_threshold['critical_num'], temp_threshold.get("emergency_num")))

    def get_monitor_temp(self):
        sensorlist = self.int_case.get_temp_info()

        for temp_threshold in self.__temps_threshold_config.values():
            sensor = sensorlist.get(temp_threshold['name'])
            if sensor["Value"] is None or int(sensor["Value"]) == self.int_case.error_ret:
                temp_threshold['fail_num'] += 1
                fancontrol_error("get %s failed, fail_num = %d" % (temp_threshold['name'], temp_threshold['fail_num']))
            else:
                temp_threshold['fail_num'] = 0
                temp_threshold.setdefault('fix', 0)
                temp_threshold['temp'] = sensor["Value"] + temp_threshold['fix']
            fancontrol_debug("%s = %d" % (temp_threshold['name'], temp_threshold['temp']))
        self.update_over_temp_threshold_num()

    def is_temp_warning(self):
        warning_flag = False
        for temp_threshold in self.__temps_threshold_config.values():
            if temp_threshold['ignore_threshold']:
                continue
            if temp_threshold['warning_num'] >= self.__temp_warning_num:
                warning_flag = True
                fancontrol_warn("%%FANCONTROL-4-TEMP_HIGH: %s temperature %sC is larger than warning threshold %sC." %
                                (temp_threshold['name'], temp_threshold['temp'], temp_threshold.get('warning')))
        return warning_flag

    def checkTempWarning(self):
        try:
            if self.is_temp_warning():
                self.__warning_countdown = self.__temp_warning_countdown
                fancontrol_debug("temp is over warning")
                return True
            if self.__warning_countdown > 0:
                self.__warning_countdown -= 1
            return False
        except Exception as e:
            fancontrol_error("%%policy: checkTempWarning failed")
            fancontrol_error(str(e))
        return False

    def checkTempWarningCountdown(self):
        if self.__warning_countdown > 0:
            return True
        return False

    def is_temp_critical(self):
        critical_flag = False
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['critical_flag'] = False
            if temp_threshold['ignore_threshold']:
                continue
            if temp_threshold['critical_num'] >= self.__temp_critical_num:
                critical_flag = True
                temp_threshold['critical_flag'] = True
                fancontrol_crit("%%FANCONTROL-2-TEMP_HIGH: %s temperature %sC is larger than critical threshold %sC." %
                                (temp_threshold['name'], temp_threshold['temp'], temp_threshold.get('critical')))
        return critical_flag

    def checkTempCritical(self):
        try:
            if self.is_temp_critical():
                self.__critical_countdown = self.__temp_critical_countdown
                fancontrol_debug("temp is over critical")
                return True
            if self.__critical_countdown > 0:
                self.__critical_countdown -= 1
            return False
        except Exception as e:
            fancontrol_error("%%policy: checkTempCrit failed")
            fancontrol_error(str(e))
        return False

    def is_temp_emergency(self):
        emergency_flag = False
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['emergency_flag'] = False
            if temp_threshold['ignore_threshold']:
                continue
            if temp_threshold['emergency_num'] >= self.__temp_emergency_num:
                emergency_flag = True
                temp_threshold['emergency_flag'] = True
                fancontrol_alert("%%FANCONTROL-1-TEMP_HIGH: %s temperature %sC is larger than emergency threshold %sC." %
                                 (temp_threshold['name'], temp_threshold['temp'], temp_threshold.get('emergency')))
        return emergency_flag

    def checkTempEmergency(self):
        try:
            if self.is_temp_emergency():
                self.__emergency_countdown = self.__temp_emergency_countdown
                fancontrol_debug("temp is over emergency")
                return True
            if self.__emergency_countdown > 0:
                self.__emergency_countdown -= 1
            return False
        except Exception as e:
            fancontrol_error("%%policy: checkTempEmergency failed")
            fancontrol_error(str(e))
        return False

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
                temp_dict = dict(self.__temps_threshold_config)
                tmp = temp_dict.get(SWITCH_TEMP)
                if tmp['critical_flag'] is True:
                    fancontrol_debug("switch temp is over reboot critical")
                    return TEMP_REBOOT_CRIT_SWITCH_FLAG
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

    def checkCritReboot(self):
        try:
            reboot_flag = self.checkTempRebootCrit()
            if reboot_flag > 0:
                self.set_all_fan_speed_pwm(self.__max_pwm)
                for i in range(self.__check_crit_reboot_num):
                    time.sleep(self.__check_crit_sleep_time)
                    self.get_monitor_temp()
                    reboot_flag = self.checkTempRebootCrit()
                    if reboot_flag > 0:
                        fancontrol_emerg("%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot critical value lasts for %d seconds." %
                                         (self.__check_crit_sleep_time * (i + 1)))
                        continue
                    fancontrol_debug("The temperature of device is not over reboot critical value.")
                    break
                if reboot_flag > 0:
                    fancontrol_emerg(
                        "%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot critical value, system is going to reboot now.")
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
                    time.sleep(3)
                    os.system("/sbin/reboot")
        except Exception as e:
            fancontrol_error("%%policy: checkCritReboot failed")
            fancontrol_error(str(e))

    def checkTempRebootEmerg(self):
        try:
            if self.is_temp_emergency():
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
                        return True
        except Exception as e:
            fancontrol_error("%%policy: checkTempRebootEmerg failed")
            fancontrol_error(str(e))
        return False

    def checkEmergReboot(self):
        try:
            reboot_flag = False
            if self.checkTempRebootEmerg() is True:
                self.set_all_fan_speed_pwm(self.__max_pwm)
                for i in range(self.__check_emerg_reboot_num):
                    time.sleep(self.__check_emerg_sleep_time)
                    self.get_monitor_temp()
                    if self.checkTempRebootEmerg() is True:
                        fancontrol_emerg("%%FANCONTROL-0-TEMP_EMERG: The temperature of device over reboot emergency value lasts for %d seconds." %
                                         (self.__check_emerg_sleep_time * (i + 1)))
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
                    os.system("/sbin/reboot")
        except Exception as e:
            fancontrol_error("%%policy: checkEmergReboot failed")
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

        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            rotor_num = self.get_rotor_number(fan_name)
            tmp_fan = self.__fan_rotor_error_num.get(fan_name)
            status = self.get_fan_presence(fan_name)
            if status is False:
                absent_num = absent_num + 1
                self.__fan_present_status[fan_name] = 0
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
        return absent_num

    def checkFanRotorStatus(self):
        err_num = 0
        self.__fan_nok_num = 0
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            rotor_num = self.get_rotor_number(fan_name)
            tmp_fan = self.__fan_rotor_error_num.get(fan_name)
            fan_rotor_err_cnt = 0
            for j in range(rotor_num):
                rotor_name = "Rotor" + str(j + 1)
                status = self.get_fan_rotor_status(fan_name, rotor_name)
                if status is True:
                    tmp_fan[rotor_name] = 0
                    fancontrol_debug("%s %s ok" % (fan_name, rotor_name))
                else:
                    tmp_fan[rotor_name] += 1
                    if tmp_fan[rotor_name] >= self.__rotor_error_count:
                        err_num = err_num + 1
                        fan_rotor_err_cnt += 1
                        fancontrol_debug("%s %s error" % (fan_name, rotor_name))
                    fancontrol_debug("%s %s error %d times" % (fan_name, rotor_name, tmp_fan[rotor_name]))
            if fan_rotor_err_cnt == 0:
                self.__fan_rotate_status[fan_name] = 1  # FAN is ok
            else:
                self.__fan_rotate_status[fan_name] = 0  # FAN is not ok
                self.__fan_nok_num += 1
        fancontrol_debug("fan not ok number:%d." % self.__fan_nok_num)
        return err_num

    def checkPsuPresence(self):
        absent_num = 0
        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            status = self.get_psu_presence(psu_name)
            if status is False:
                absent_num = absent_num + 1
                fancontrol_debug("%s absent" % psu_name)
            else:
                fancontrol_debug("%s presence" % psu_name)
        return absent_num

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

    def checkDevError(self):
        pwm = self.__min_pwm
        switchtemp = self.__temps_threshold_config.get(SWITCH_TEMP)['temp']
        inlettemp = self.__temps_threshold_config.get(INLET_TEMP)['temp']
        temp_diff = abs(switchtemp - inlettemp)
        fancontrol_debug("|switchtemp - inlettemp| = %d" % temp_diff)
        if temp_diff >= self.__inlet_mac_diff:
            fancontrol_debug("temp_diff is over than inlet_mac_diff(%d)" % self.__inlet_mac_diff)
            if self.__pwm > self.__abnormal_pwm:
                pwm = self.__max_pwm
            else:
                pwm = self.__abnormal_pwm
        return pwm

    def checktempfail(self):
        pwm = self.__min_pwm
        for temp in self.__check_temp_fail:
            temp_name = temp.get("temp_name")
            temp_fail_num = self.__temps_threshold_config.get(temp_name)['fail_num']
            if temp_fail_num >= self.__temp_fail_num:
                pwm = self.__abnormal_pwm
                fancontrol_debug("%s temp_fail_num = %d" % (temp_name, temp_fail_num))
                fancontrol_debug("self.__temp_fail_num = %d" % self.__temp_fail_num)
        return pwm

    def abnormal_check(self):
        pwm_list = []
        pwm_min = self.__min_pwm
        pwm_list.append(pwm_min)

        if self.__check_temp_emergency == 1:
            status = self.checkTempEmergency()
            if status is True:
                over_emerg_pwm = self.__max_pwm
                pwm_list.append(over_emerg_pwm)
                fancontrol_debug("over_emerg_pwm = 0x%x" % over_emerg_pwm)
                # do reset check
                if self.__check_emerg_reboot_flag == 1:
                    self.checkEmergReboot()
            else:
                if self.checkTempEmergencyCountdown() is True:  # temp lower than emergency in 5 min
                    over_emerg_countdown_pwm = self.__max_pwm
                    pwm_list.append(over_emerg_countdown_pwm)
                    fancontrol_debug("TempEmergencyCountdown: %d, over_emerg_countdown_pwm = 0x%x" %
                                     (self.__emergency_countdown, over_emerg_countdown_pwm))

        if self.__check_temp_critical == 1:
            status = self.checkTempCritical()
            if status is True:
                over_crit_pwm = self.__max_pwm
                pwm_list.append(over_crit_pwm)
                fancontrol_debug("over_crit_pwm = 0x%x" % over_crit_pwm)
                # do reset check
                if self.__check_crit_reboot_flag == 1:
                    self.checkCritReboot()
            else:
                if self.checkTempCriticalCountdown() is True:  # temp lower than critical in 5 min
                    over_crit_countdown_pwm = self.__max_pwm
                    pwm_list.append(over_crit_countdown_pwm)
                    fancontrol_debug("TempCriticalCountdown: %d, over_crit_countdown_pwm = 0x%x" %
                                     (self.__critical_countdown, over_crit_countdown_pwm))

        if self.__check_temp_warning == 1:
            status = self.checkTempWarning()
            if status is True:
                over_warn_pwm = self.__warning_pwm
                pwm_list.append(over_warn_pwm)
                fancontrol_debug("over_warn_pwm = 0x%x" % over_warn_pwm)
            else:
                if self.checkTempWarningCountdown() is True:  # temp lower than warning in 5 min
                    over_warn_countdown_pwm = self.__warning_pwm
                    pwm_list.append(over_warn_countdown_pwm)
                    fancontrol_debug("TempWarningCountdown: %d, over_warn_countdown_pwm = 0x%x" %
                                     (self.__warning_countdown, over_warn_countdown_pwm))

        self.__fan_absent_num = self.checkFanPresence()
        if self.__fan_absent_num >= self.__fan_absent_fullspeed_num:
            fan_absent_pwm = self.__max_pwm
            pwm_list.append(fan_absent_pwm)
            fancontrol_debug("fan_absent_pwm = 0x%x" % fan_absent_pwm)

        rotor_err_num = self.checkFanRotorStatus()
        if rotor_err_num >= self.__rotor_error_fullspeed_num:
            rotor_err_pwm = self.__max_pwm
            pwm_list.append(rotor_err_pwm)
            fancontrol_debug("rotor_err_pwm = 0x%x" % rotor_err_pwm)

        psu_absent_num = self.checkPsuPresence()
        if psu_absent_num >= self.__psu_absent_fullspeed_num:
            psu_absent_pwm = self.__max_pwm
            pwm_list.append(psu_absent_pwm)
            fancontrol_debug("psu_absent_pwm = 0x%x" % psu_absent_pwm)

        dev_err_pwm = self.checkDevError()
        pwm_list.append(dev_err_pwm)
        fancontrol_debug("dev_err_pwm = 0x%x" % dev_err_pwm)

        temp_fail_pwm = self.checktempfail()
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
        air_flow_tuple = (F2B_AIR_FLOW, B2F_AIR_FLOW)
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
        # get_monitor_temp
        self.get_monitor_temp()
        fancontrol_debug("last_pwm = 0x%x" % self.__pwm)
        # openloop
        inlettemp = self.__temps_threshold_config.get(INLET_TEMP)['temp']
        linear_value = self.openloop.linear_cacl(inlettemp)
        if linear_value is None:
            linear_value = self.__min_pwm
        pwm_list.append(linear_value)
        fancontrol_debug("linear_value = 0x%x" % linear_value)

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
        if self.air_flow_inconsistent_flag is True:
            fanairflow_debug("air flow inconsistent, set all fan speed pwm")
            self.set_all_fan_speed_pwm(self.__pwm)
        else:
            fanairflow_debug("air flow consistent, deal fan error policy")
            fan_pwm_dict = self.get_fan_pwm_dict(self.__pwm)
            psu_pwm_dict = self.get_psu_pwm_dict(self.__pwm)
            self.set_fan_pwm_independent(fan_pwm_dict, psu_pwm_dict)

    def run(self):
        start_time = time.time()
        while True:
            try:
                debug_init()
                if self.__fan_status_interval > 0 and self.__fan_status_interval < self.__interval:
                    delta_time = time.time() - start_time
                    if delta_time >= self.__interval or delta_time < 0:
                        self.do_fancontrol()
                        start_time = time.time()
                    else:
                        self.checkFanPresence()
                    time.sleep(self.__fan_status_interval)
                else:
                    self.do_fancontrol()
                    time.sleep(self.__interval)
            except Exception as e:
                traceback.print_exc()
                fancontrol_error(str(e))

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
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            self.fan_set_speed_pwm_by_name(fan_name, fan_pwm_dict[fan_name])
        if self.__psu_fan_control == 1:
            psu_num = self.get_psu_total_number()
            for i in range(psu_num):
                psu_name = "PSU" + str(i + 1)
                self.psu_set_speed_pwm_by_name(psu_name, psu_pwm_dict[psu_name])

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


if __name__ == '__main__':
    debug_init()
    fancontrol_debug("enter main")
    fan_control = fancontrol()
    fan_control.fan_obj_init()
    fan_control.psu_obj_init()
    fan_control.run()
