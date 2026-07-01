#!/usr/bin/env python_nos
import os
import time
import sys
import syslog
import traceback
from fcntl import ioctl
import logging
import click
from plat_hal.interface import interface
from plat_hal.baseutil import baseutil
from platform_util import *
from public.platform_common_config import PRODUCT_STRATEGY, PRODUCT_STRATEGY_1, S3IP_SYSFS_NAME
try:
    import abc
except ImportError as error:
    raise ImportError(str(error) + " - required module not found") from error

SWITCH_TEMP = "SWITCH_TEMP"
F2B_AIR_FLOW = "intake"
B2F_AIR_FLOW = "exhaust"
E2_F2B_AIR_FLOW = "F2B"
E2_B2F_AIR_FLOW = "B2F"
ONIE_E2_NAME = "ONIE_E2"

# status
STATUS_PRESENT = "PRESENT"
STATUS_ABSENT = "ABSENT"
STATUS_OK = "OK"
STATUS_NOT_OK = "NOT OK"
STATUS_FAILED = "FAILED"
STATUS_UNKNOWN = "UNKNOWN"

LED_CONTRL_FILE = "/tmp/.ledcontrol_factest_mode_en"
DEBUG_FILE = "/etc/.ledcontrol_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "hal_ledctrl_debug.log"
SYSTEM_STATUS_FILE = f"/sys/{S3IP_SYSFS_NAME}/system/system_status"
SYSTEM_STATUS_OK = 0
SYSTEM_STATUS_NOTICE = 1
SYSTEM_STATUS_WARNING = 2
SYSTEM_STATUS_CRITICAL = 3

logger = setup_logger(LOG_FILE)
# led status defined
COLOR_GREEN = 1
COLOR_AMBER = 2
COLOR_RED = 3
COLOR_FLASH = 4
LED_STATUS_DICT = {COLOR_GREEN: "green", COLOR_AMBER: "amber", COLOR_RED: "red", COLOR_FLASH: "flash"}
SYSLOG_TITLE_AIR_FLOW = "FLOW_MONITOR"

LOG_LAST_TIME = {}

MASTER_FLASH = 0
SLAVE_FLASH = 1

def ledcontrol_debug(s):
    logger.debug(s)

def ledcontrol_error(s):
    logger.error(s)


def air_flow_warn(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_warn(SYSLOG_TITLE_AIR_FLOW, s)
    logger.warning(s)

def air_flow_error(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_error(SYSLOG_TITLE_AIR_FLOW, s)
    logger.error(s)

def air_flow_emerg(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, additional_key)
    if print_flag:
        common_syslog_emerg(SYSLOG_TITLE_AIR_FLOW, s)
    logger.error(s)

def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


class DevBase(object):
    __metaclass__ = abc.ABCMeta

    def __init__(self, name, air_flow_monitor):
        self.__name = name
        self.__air_flow_monitor = air_flow_monitor
        self.present = STATUS_UNKNOWN
        self.status = STATUS_UNKNOWN
        self.status_summary = STATUS_UNKNOWN
        self.origin_name = STATUS_UNKNOWN
        self.display_name = STATUS_UNKNOWN
        self.air_flow = STATUS_UNKNOWN
        self.led_status = COLOR_GREEN

    @property
    def name(self):
        return self.__name

    @property
    def air_flow_monitor(self):
        return self.__air_flow_monitor

    @abc.abstractmethod
    def get_present(self):
        """
        Gets the present status of PSU/FAN

        Returns:
            A string, e.g. 'PRESENT, ABSENT, FAILED'
        """
        raise NotImplementedError

    @abc.abstractmethod
    def get_status(self):
        """
        Gets the status of PSU/FAN

        Returns:
            A string, e.g. 'OK, NOT OK, FAILED'
        """
        raise NotImplementedError

    @abc.abstractmethod
    def update_dev_info(self):
        """
        update status and fru info of PSU/FAN

        include present, status, status_summary, part_model_name, product_name, air_flow
        """
        raise NotImplementedError

    @abc.abstractmethod
    def set_module_led(self, color):
        """
        set PSU/FAN module LED status

        Args:
            color: A string representing the color with which to set the
                   PSU/FAN module LED status

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        raise NotImplementedError


class DevPsu(DevBase):

    def __init__(self, name, air_flow_monitor, hal_interface):
        super(DevPsu, self).__init__(name, air_flow_monitor)
        self.int_case = hal_interface

    def get_psu_presence(self):
        return self.int_case.get_psu_presence(self.name)

    def get_psu_input_output_status(self):
        return self.int_case.get_psu_input_output_status(self.name)

    def get_psu_fru_info(self):
        return self.int_case.get_psu_fru_info(self.name)

    @property
    def na_ret(self):
        return self.int_case.na_ret

    def get_present(self):
        try:
            status = self.get_psu_presence()
            if status is True:
                return STATUS_PRESENT
            if status is False:
                return STATUS_ABSENT
        except Exception as e:
            ledcontrol_error("get %s present status error, msg: %s" % (self.name, str(e)))
        return STATUS_FAILED

    def get_status(self):
        try:
            status = self.get_psu_input_output_status()
            if status is True:
                return STATUS_OK
            if status is False:
                return STATUS_NOT_OK
        except Exception as e:
            ledcontrol_error("get %s status error, msg: %s" % (self.name, str(e)))
        return STATUS_FAILED

    def update_dev_info(self):
        try:
            # update status
            self.present = self.get_present()
            if self.present != STATUS_PRESENT:
                self.status = STATUS_UNKNOWN
                self.status_summary = self.present
            else:
                self.status = self.get_status()
                self.status_summary = self.status
            # update fru info if need air flow monitor
            if self.air_flow_monitor:
                dic = self.get_psu_fru_info()
                self.origin_name = dic["PN"]
                self.air_flow = dic["AirFlow"]
                self.display_name = dic["DisplayName"]
        except Exception as e:
            ledcontrol_error("update %s info error, msg: %s" % (self.name, str(e)))
            self.present = STATUS_FAILED
            self.status = STATUS_FAILED
            self.status_summary = STATUS_FAILED
            self.origin_name = self.na_ret
            self.air_flow = self.na_ret
            self.display_name = self.na_ret

    def set_module_led(self, color):
        """
        set PSU module LED is not support, always return True
        """
        return True


class DevFan(DevBase):

    def __init__(self, name, air_flow_monitor, hal_interface, 
                    fan_rotor_threshold_config, lowest_rpm, 
                    fan_rpm_check_warning):
        super(DevFan, self).__init__(name, air_flow_monitor)
        self.int_case = hal_interface
        self.fan_rotor_threshold_config = fan_rotor_threshold_config
        self.lowest_rpm = lowest_rpm
        self.fan_rpm_check_warning = fan_rpm_check_warning

    def get_fan_rotor_number(self):
        return self.int_case.get_fan_rotor_number(self.name)

    def get_fan_presence(self):
        return self.int_case.get_fan_presence(self.name)

    def get_fan_rotor_status(self, rotor_name):
        return self.int_case.get_fan_rotor_status(self.name, rotor_name)

    def get_fan_fru_info(self):
        return self.int_case.get_fan_fru_info(self.name)

    def get_rotor_speed(self, rotor_index):
        return self.int_case.get_fan_speed(self.name, rotor_index)

    def get_rotor_speed_pwm(self, rotor_index):
        return self.int_case.get_fan_speed_pwm(self.name, rotor_index)

    @property
    def na_ret(self):
        return self.int_case.na_ret

    def get_present(self):
        try:
            status = self.get_fan_presence()
            if status is True:
                return STATUS_PRESENT
            if status is False:
                return STATUS_ABSENT
        except Exception as e:
            ledcontrol_error("get %s present status error, msg: %s" % (self.name, str(e)))
        return STATUS_FAILED

    def get_rotor_rated_speed(self, rotor_name, pwm):
        pwm_id = int(pwm / 10)
        rotor_threshold_config = self.fan_rotor_threshold_config[rotor_name]
        if (pwm_id < 0) or (pwm_id >= len(rotor_threshold_config)):
            rated_speed = 0
            ledcontrol_debug("pwm_id = %d, len(rotor_threshold_config) = %d" % (pwm_id, len(rotor_threshold_config)))
        else:
            rated_speed = rotor_threshold_config[pwm_id]['rpm']
        return rated_speed

    def get_rotor_status_strategy_default(self, rotor_index):
        ret = True
        rotor_name = "Rotor" + str(rotor_index + 1)
        roll_status = self.get_fan_rotor_status(rotor_name)
        if roll_status is not True:
            ret = False
            ledcontrol_debug("%s %s error, status %s" % (self.name, rotor_name, roll_status))
        else:
            ledcontrol_debug("%s %s ok" % (self.name, rotor_name))

        return ret

    def get_rotor_status_strategy_1(self, rotor_index, rotor_num):
        ret = True
        rotor_name = "Rotor" + str(rotor_index + 1)
        roll_status = self.get_fan_rotor_status(rotor_name)
        if roll_status is not True:
            ret = False
            ledcontrol_debug("%s %s error, status %s" % (self.name, rotor_name, roll_status))
            return ret
        else:
            ledcontrol_debug("%s %s ok" % (self.name, rotor_name))

        if len(self.fan_rotor_threshold_config) != 0:
            tmp_pwm = self.get_rotor_speed_pwm(rotor_index + 1)
            tmp_speed = self.get_rotor_speed(rotor_index + 1)
            rated_speed = self.get_rotor_rated_speed(rotor_name, tmp_pwm)
            if (tmp_speed < self.lowest_rpm) or (tmp_speed < rated_speed * self.fan_rpm_check_warning / 100):
                ret = False
                ledcontrol_debug("%s %s error" % (rotor_num, rotor_name))
                ledcontrol_debug("pwm = %d, speed = %d, rated_speed = %d" % (tmp_pwm, tmp_speed, rated_speed))
            else:
                ledcontrol_debug("%s %s ok" % (rotor_num, rotor_name))
                ledcontrol_debug("pwm = %d, speed = %d, rated_speed = %d" % (tmp_pwm, tmp_speed, rated_speed))

        return ret

    def get_status(self):
        try:
            rotor_num = self.get_fan_rotor_number()
            err_rotor_num = 0
            for j in range(rotor_num):
                if PRODUCT_STRATEGY == PRODUCT_STRATEGY_1:
                    ret = self.get_rotor_status_strategy_1(j, rotor_num)
                else:
                    ret = self.get_rotor_status_strategy_default(j)

                if ret == False:
                    err_rotor_num += 1

            if err_rotor_num > 0:
                return STATUS_NOT_OK
            return STATUS_OK
        except Exception as e:
            ledcontrol_error("get %s status error, msg: %s" % (self.name, str(e)))
        return STATUS_FAILED

    def update_dev_info(self):
        try:
            # update status
            self.present = self.get_present()
            if self.present != STATUS_PRESENT:
                self.status = STATUS_UNKNOWN
                self.status_summary = self.present
            else:
                self.status = self.get_status()
                self.status_summary = self.status
            # update fru info if need air flow monitor
            if self.air_flow_monitor:
                dic = self.get_fan_fru_info()
                self.origin_name = dic["PN"]
                self.air_flow = dic["AirFlow"]
                self.display_name = dic["DisplayName"]
        except Exception as e:
            ledcontrol_error("update %s fru info error, msg: %s" % (self.name, str(e)))
            self.present = STATUS_FAILED
            self.status = STATUS_FAILED
            self.status_summary = STATUS_FAILED
            self.origin_name = self.na_ret
            self.air_flow = self.na_ret
            self.display_name = self.na_ret

    def set_module_led(self, color):
        ret = self.int_case.set_fan_led(self.name, color)
        if ret == 0:
            return True
        return False


class ledcontrol(object):

    def __init__(self):
        self.fan_obj_list = []
        self.psu_obj_list = []
        self.board_psu_led_status = COLOR_GREEN
        self.board_fan_led_status = COLOR_GREEN
        self.__board_air_flow = ""
        self.int_case = interface()
        self.fd = None

        self.__config = baseutil.get_monitor_config()
        self.__dcdc_whitelist = self.__config.get('dcdc_monitor_whitelist', {})
        self.__fw_upgrade_check = self.__config.get('fw_upgrade_check', [])
        self.__temps_threshold_config = self.__config["temps_threshold"]
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['temp'] = 0
            temp_threshold['fail_num'] = 0
        self.__ledcontrol_para = self.__config["ledcontrol_para"]
        self.__interval = self.__ledcontrol_para.get("interval", 5)
        self.__checkpsu = self.__ledcontrol_para.get("checkpsu", 0)
        self.__checkfan = self.__ledcontrol_para.get("checkfan", 0)
        self.__psu_amber_num = self.__ledcontrol_para.get("psu_amber_num")
        self.__fan_amber_num = self.__ledcontrol_para.get("fan_amber_num")
        self.__psu_air_flow_amber_num = self.__ledcontrol_para.get("psu_air_flow_amber_num", 0)
        self.__fan_air_flow_amber_num = self.__ledcontrol_para.get("fan_air_flow_amber_num", 0)
        self.__board_sys_led = self.__ledcontrol_para.get("board_sys_led", [])
        self.__board_psu_led = self.__ledcontrol_para.get("board_psu_led", [])
        self.__board_fan_led = self.__ledcontrol_para.get("board_fan_led", [])
        self.__board_smb_led = self.__ledcontrol_para.get("board_smb_led", [])
        self.__board_bmc_led = self.__ledcontrol_para.get("board_bmc_led", [])
        self.__psu_air_flow_monitor = self.__ledcontrol_para.get("psu_air_flow_monitor", 0)
        self.__fan_air_flow_monitor = self.__ledcontrol_para.get("fan_air_flow_monitor", 0)
        self.__fan_mix_list = self.__ledcontrol_para.get("fan_mix_list", [])
        self.__sysled_check_temp = self.__ledcontrol_para.get("sysled_check_temp", 1)
        self.__sysled_check_fw_up = self.__ledcontrol_para.get("sysled_check_fw_up", 0)
        self.__smbled_ctrl = self.__ledcontrol_para.get("smbled_ctrl", 0)
        self.__bmcled_ctrl = self.__ledcontrol_para.get("bmcled_ctrl", 0)
        self.__set_fan_module_led = self.__ledcontrol_para.get("set_fan_module_led", 1)
        self.__psu_redundancy_path = self.__ledcontrol_para.get("psu_redundancy_path", f"/sys/{S3IP_SYSFS_NAME}/psu/redundancy_num")
        self.__fan_redundancy_path = self.__ledcontrol_para.get("fan_redundancy_path", f"/sys/{S3IP_SYSFS_NAME}/fan/redundancy_num")
        self.__bios_boot_source_check = self.__ledcontrol_para.get("bios_boot_source_check", 0)
        self.__sysled_pi_ctrl = self.__ledcontrol_para.get("sysled_pi_ctrl", 0)
        self.__fan_rotor_threshold_config = self.__config.get("fan_rotor_threshold", {})
        self.__lowest_rpm = self.__ledcontrol_para.get("lowest_rpm", 1000)
        self.__fan_rpm_check_warning = self.__ledcontrol_para.get("fan_rpm_check_warning", 85)
        self.__sys_led_ioctl = self.__ledcontrol_para.get("sys_led_ioctl", 0)
        if self.__sys_led_ioctl == 1:
            self.__sys_led_ioctl_red = self.__ledcontrol_para.get("sys_led_ioctl_red", 0x5300)
            self.__sys_led_ioctl_green = self.__ledcontrol_para.get("sys_led_ioctl_green", 0x5301)
            self.__sys_led_ioctl_amber = self.__ledcontrol_para.get("sys_led_ioctl_amber", 0x5302)

    @property
    def na_ret(self):
        return self.int_case.na_ret

    @property
    def checkpsu(self):
        return self.__checkpsu

    @property
    def checkfan(self):
        return self.__checkfan

    @property
    def psu_amber_num(self):
        return self.__psu_amber_num

    @property
    def fan_amber_num(self):
        return self.__fan_amber_num

    @property
    def psu_air_flow_amber_num(self):
        return self.__psu_air_flow_amber_num

    @property
    def fan_air_flow_amber_num(self):
        return self.__fan_air_flow_amber_num

    @property
    def psu_air_flow_monitor(self):
        return self.__psu_air_flow_monitor

    @property
    def fan_air_flow_monitor(self):
        return self.__fan_air_flow_monitor

    @property
    def board_sys_led(self):
        return self.__board_sys_led

    @property
    def board_psu_led(self):
        return self.__board_psu_led

    @property
    def board_fan_led(self):
        return self.__board_fan_led

    @property
    def board_smb_led(self):
        return self.__board_smb_led

    @property
    def board_bmc_led(self):
        return self.__board_bmc_led

    @property
    def fan_mix_list(self):
        return self.__fan_mix_list

    @property
    def sysled_check_temp(self):
        return self.__sysled_check_temp

    @property
    def sysled_check_fw_up(self):
        return self.__sysled_check_fw_up

    @property
    def smbled_ctrl(self):
        return self.__smbled_ctrl

    @property
    def bmcled_ctrl(self):
        return self.__bmcled_ctrl

    @property
    def bios_boot_source_check(self):
        return self.__bios_boot_source_check

    @property
    def interval(self):
        return self.__interval

    @property
    def sysled_pi_ctrl(self):
        return self.__sysled_pi_ctrl

    @property
    def is_set_fan_module_led(self):
        return self.__set_fan_module_led

    def get_fan_total_number(self):
        return self.int_case.get_fan_total_number()

    def get_psu_total_number(self):
        return self.int_case.get_psu_total_number()

    def get_onie_e2_obj(self, name):
        return self.int_case.get_onie_e2_obj(name)

    def set_led_color(self, led_name, color):
        try:
            ret = self.int_case.set_led_color(led_name, color)
        except Exception as e:
            ledcontrol_error("set %s led %s error, msg: %s" % (led_name, color, str(e)))
            ret = False
        return ret

    def set_smb_led(self, color):
        for led in self.board_smb_led:
            led_name = led.get("led_name")
            ret = self.set_led_color(led_name, color)
            if ret is True:
                ledcontrol_debug("set %s success, color:%s," % (led_name, color))
            else:
                ledcontrol_debug("set %s failed, color:%s," % (led_name, color))

    def set_sys_led(self, color):
        for led in self.board_sys_led:
            led_name = led.get("led_name")
            ret = self.set_led_color(led_name, color)
            if ret is True:
                ledcontrol_debug("set %s success, color:%s," % (led_name, color))
            else:
                ledcontrol_debug("set %s failed, color:%s," % (led_name, color))

    def set_psu_led(self, color):
        for led in self.board_psu_led:
            led_name = led.get("led_name")
            ret = self.set_led_color(led_name, color)
            if ret is True:
                ledcontrol_debug("set %s success, color:%s," % (led_name, color))
            else:
                ledcontrol_debug("set %s failed, color:%s," % (led_name, color))

    def set_fan_led(self, color):
        for led in self.board_fan_led:
            led_name = led.get("led_name")
            ret = self.set_led_color(led_name, color)
            if ret is True:
                ledcontrol_debug("set %s success, color:%s," % (led_name, color))
            else:
                ledcontrol_debug("set %s failed, color:%s," % (led_name, color))

    def set_fan_module_led(self):
        for fan_obj in self.fan_obj_list:
            color = LED_STATUS_DICT.get(fan_obj.led_status)
            ret = fan_obj.set_module_led(color)
            if ret is True:
                ledcontrol_debug("set %s module led success, color: %s," % (fan_obj.name, color))
            else:
                ledcontrol_debug("set %s module led failed, color: %s," % (fan_obj.name, color))

    def set_bmc_led(self, color):
        for led in self.board_bmc_led:
            led_name = led.get("led_name")
            ret = self.set_led_color(led_name, color)
            if ret is True:
                ledcontrol_debug("set %s success, color:%s," % (led_name, color))
            else:
                ledcontrol_debug("set %s failed, color:%s," % (led_name, color))

    @property
    def board_air_flow(self):
        air_flow_tuple = (F2B_AIR_FLOW, B2F_AIR_FLOW, E2_F2B_AIR_FLOW, E2_B2F_AIR_FLOW)
        if self.__board_air_flow not in air_flow_tuple:
            self.__board_air_flow = self.int_case.get_device_airflow(ONIE_E2_NAME)
            ledcontrol_debug("board_air_flow: %s" % self.__board_air_flow)
        return self.__board_air_flow

    def update_psu_info(self):
        for psu_obj in self.psu_obj_list:
            psu_obj.update_dev_info()
            ledcontrol_debug("%s present: [%s], status: [%s] status_summary [%s]" %
                             (psu_obj.name, psu_obj.present, psu_obj.status, psu_obj.status_summary))
            if psu_obj.air_flow_monitor:
                ledcontrol_debug("%s origin name: [%s], display name: [%s] air flow [%s]" %
                                 (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow))

    def update_fan_info(self):
        for fan_obj in self.fan_obj_list:
            fan_obj.update_dev_info()
            ledcontrol_debug("%s present: [%s], status: [%s] status_summary [%s]" %
                             (fan_obj.name, fan_obj.present, fan_obj.status, fan_obj.status_summary))
            if fan_obj.air_flow_monitor:
                ledcontrol_debug("%s origin name: [%s], display name: [%s] air flow [%s]" %
                                 (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow))

    def get_monitor_temp(self):
        sensorlist = self.int_case.get_temp_info()

        for temp_threshold in self.__temps_threshold_config.values():
            sensor = sensorlist.get(temp_threshold['name'])
            if sensor is None or sensor["Value"] is None:
                temp_threshold['fail_num'] += 1
                ledcontrol_error("get %s failed, fail_num = %d" % (temp_threshold['name'], temp_threshold['fail_num']))
            else:
                temp_threshold['fail_num'] = 0
                temp_threshold.setdefault('fix', 0)
                temp_threshold['temp'] = sensor["Value"] + temp_threshold['fix']
            ledcontrol_debug("%s = %d" % (temp_threshold['name'], temp_threshold['temp']))
            ledcontrol_debug("warning = %d, critical = %d" % (temp_threshold['warning'], temp_threshold['critical']))

    def is_temp_warning(self):
        warning_flag = False
        for temp_threshold in self.__temps_threshold_config.values():
            if temp_threshold['temp'] >= temp_threshold['warning']:
                warning_flag = True
                ledcontrol_debug("%s is over warning" % temp_threshold['name'])
                ledcontrol_debug(
                    "%s = %d, warning = %d" %
                    (temp_threshold['name'],
                     temp_threshold['temp'],
                        temp_threshold['warning']))
        return warning_flag

    def checkTempWarning(self):
        try:
            if self.is_temp_warning():
                ledcontrol_debug("temp is over warning")
                return True
        except Exception as e:
            ledcontrol_error("%%policy: checkTempWarning failed")
            ledcontrol_error(str(e))
        return False

    def is_temp_critical(self):
        critical_flag = False
        for temp_threshold in self.__temps_threshold_config.values():
            temp_threshold['critical_flag'] = False
            if temp_threshold['temp'] >= temp_threshold['critical']:
                critical_flag = True
                temp_threshold['critical_flag'] = True
                ledcontrol_debug("%s is over critical" % temp_threshold['name'])
                ledcontrol_debug(
                    "%s = %d, critical = %d" %
                    (temp_threshold['name'],
                     temp_threshold['temp'],
                        temp_threshold['critical']))
        return critical_flag

    def checkTempCrit_strategy_default(self):
        temp_dict = dict(self.__temps_threshold_config)
        tmp = temp_dict.get(SWITCH_TEMP)
        if tmp['critical_flag'] is True:
            ledcontrol_debug("temp is over critical")
            return True

        del temp_dict[SWITCH_TEMP]
        for temp_items in temp_dict.values():
            if temp_items['critical_flag'] is False:
                return False

        ledcontrol_debug("temp is over critical")
        return True

    def checkTempCrit_strategy_1(self):
        for temp_items in self.__temps_threshold_config.values():
            if temp_items['critical_flag'] is True:
                ledcontrol_debug("temp is over critical")
                return True


    def checkTempCrit(self):
        try:
            if self.is_temp_critical():
                if PRODUCT_STRATEGY == PRODUCT_STRATEGY_1:
                    ret = self.checkTempCrit_strategy_1()
                else:
                    ret = self.checkTempCrit_strategy_default()
                return ret

        except Exception as e:
            ledcontrol_error("%%policy: checkTempCrit failed")
            ledcontrol_error(str(e))
        return False

    def dcdc_whitelist_check(self, dcdc_name):
        try:
            check_item = self.__dcdc_whitelist.get(dcdc_name, {})
            if len(check_item) == 0:
                ledcontrol_debug("%s whitelist config is None" % dcdc_name)
                return False

            checkbit = check_item.get("checkbit", None)
            okval = check_item.get("okval", None)
            if checkbit is None or okval is None:
                ledcontrol_error('%s config error, checkbit:%s, okval:%s' % (dcdc_name, checkbit, okval))
                return False

            ret, retval = get_value(check_item)
            if ret is False:
                ledcontrol_error("get %s whitelist value error, config: %s, msg: %s" % (dcdc_name, check_item, retval))
                return False

            val_t = retval & (1 << checkbit) >> checkbit
            if val_t != okval:
                return False
            return True
        except Exception as e:
            ledcontrol_error('%%WHITELIST_CHECK: %s check error, msg: %s.' % (dcdc_name, str(e)))
            return False

    def get_voltage_led_status(self):
        try:
            led_status = COLOR_GREEN
            dcdc_dict = self.int_case.get_dcdc_all_info()
            for dcdc_name, item in dcdc_dict.items():
                ret = self.dcdc_whitelist_check(dcdc_name)
                if ret is False:
                    if item['Value'] is None or int(item['Value']) == self.int_case.error_ret:
                        ledcontrol_error('The value of %s read failed.' % (dcdc_name))
                    elif float(item['Value']) > float(item['Max']):
                        led_status = COLOR_AMBER
                        ledcontrol_debug('%s voltage %.3f%s is larger than max threshold %.3f%s.' %
                                           (dcdc_name, float(item['Value']), item['Unit'], float(item['Max']), item['Unit']))
                    elif float(item['Value']) < float(item['Min']):
                        led_status = COLOR_AMBER
                        ledcontrol_debug('%s voltage %.3f%s is lower than min threshold %.3f%s.' %
                                           (dcdc_name, float(item['Value']), item['Unit'], float(item['Min']), item['Unit']))
                    else:
                        ledcontrol_debug('%s value %s is in range [%s, %s].' % (dcdc_name, item['Value'], item['Min'], item['Max']))
                else:
                    ledcontrol_debug('%s is in dcdc whitelist, not monitor voltage' % dcdc_name)
        except Exception as e:
            ledcontrol_error('update dcdc sensors status error, msg: %s.' % (str(e)))
        ledcontrol_debug("monitor voltage, set led: %s" % LED_STATUS_DICT.get(led_status))
        return led_status

    def monitor_point_check(self, item):
        try:
            gettype = item.get('gettype', None)
            okval = item.get('okval', None)
            compare_mode = item.get('compare_mode', "equal")
            ret, value = get_value(item)
            if ret is True:
                if compare_mode == "equal":
                    if value == okval:
                        return True
                elif compare_mode == "great":
                    if value > okval:
                        return True
                elif compare_mode == "ignore":
                    return True
                else:
                    ledcontrol_debug('compare_mode %s not match error.' % (compare_mode))
            else:
                ledcontrol_debug('point check failed, gettype: %s, msg: %s' % (gettype, value))
        except Exception as e:
            ledcontrol_error('point check error. msg: %s.' % (str(e)))
        return False

    def get_fw_up_led_status(self):
        fw_upgrade_flag = False
        for item in self.__fw_upgrade_check:
            for monitor_point in item:
                status = self.monitor_point_check(monitor_point)
                if status is False:
                    fw_upgrade_flag = False
                    break
                fw_upgrade_flag = True

            if fw_upgrade_flag is True:
                ledcontrol_debug("Firmware upgrade check: firmware upgrade in progress")
                return COLOR_FLASH
        ledcontrol_debug("Firmware upgrade check: firmware upgrade not in progress")
        return COLOR_GREEN

    def check_board_air_flow(self):
        board_air_flow = self.board_air_flow
        air_flow_tuple = (F2B_AIR_FLOW, B2F_AIR_FLOW, E2_F2B_AIR_FLOW, E2_B2F_AIR_FLOW)
        if board_air_flow not in air_flow_tuple:
            air_flow_error("%%AIR_FLOW_MONITOR-3-BOARD: Get board air flow failed, value: %s." % board_air_flow)
            return False
        ledcontrol_debug("board air flow check ok: %s" % board_air_flow)
        return True

    def get_monitor_fan_status(self):
        fanerrnum = 0
        for fan_obj in self.fan_obj_list:
            status = fan_obj.status_summary
            ledcontrol_debug("%s status: %s" % (fan_obj.name, status))
            if status != STATUS_OK:
                fan_obj.led_status = COLOR_RED
                fanerrnum += 1
            else:
                fan_obj.led_status = COLOR_GREEN
        ledcontrol_debug("fan error number: %d" % fanerrnum)

        if fanerrnum == 0:
            fan_led_status = COLOR_GREEN
        elif fanerrnum <= self.fan_amber_num:
            fan_led_status = COLOR_AMBER
        else:
            fan_led_status = COLOR_RED
        ledcontrol_debug("monitor fan status, set fan led: %s" % LED_STATUS_DICT.get(fan_led_status))
        return fan_led_status

    def get_monitor_psu_status(self):
        psuerrnum = 0
        for psu_obj in self.psu_obj_list:
            status = psu_obj.status_summary
            ledcontrol_debug("%s status: %s" % (psu_obj.name, status))
            if status != STATUS_OK:
                psu_obj.led_status = COLOR_RED
                psuerrnum += 1
            else:
                psu_obj.led_status = COLOR_GREEN
        ledcontrol_debug("psu error number: %d" % psuerrnum)

        if psuerrnum == 0:
            psu_led_status = COLOR_GREEN
        elif psuerrnum <= self.psu_amber_num:
            psu_led_status = COLOR_AMBER
        else:
            psu_led_status = COLOR_RED
        ledcontrol_debug("monitor psu status, set psu led: %s" % LED_STATUS_DICT.get(psu_led_status))
        return psu_led_status

    def get_monitor_fan_air_flow(self):
        if self.fan_air_flow_monitor == 0:
            ledcontrol_debug("fan air flow monitor not open, default green")
            return COLOR_GREEN

        ret = self.check_board_air_flow()
        if ret is False:
            ledcontrol_debug("check board air flow error, skip fan air flow monitor.")
            return COLOR_GREEN

        fan_led_status_list = []
        fan_air_flow_ok_obj_list = []
        fan_air_flow_ok_set = set()
        fan_module_led_list = []
        fan_air_flow_err_num = 0
        for fan_obj in self.fan_obj_list:
            if fan_obj.present != STATUS_PRESENT:
                fan_module_led_list.append(COLOR_GREEN)
                continue
            if fan_obj.air_flow == self.na_ret:
                air_flow_warn("%%AIR_FLOW_MONITOR-4-FAN: %s get air flow failed, fan model: %s, air flow: %s." %
                              (fan_obj.name, fan_obj.display_name, fan_obj.air_flow), fan_obj.name)
                led_status = COLOR_AMBER
                fan_module_led_list.append(led_status)
            elif fan_obj.air_flow != self.board_air_flow:
                air_flow_emerg("%%AIR_FLOW_MONITOR-0-FAN: %s air flow error, fan model: %s, fan air flow: %s, board air flow: %s." %
                            (fan_obj.name, fan_obj.display_name, fan_obj.air_flow, self.board_air_flow), fan_obj.name)
                led_status = COLOR_RED
                fan_air_flow_err_num += 1
            else:
                fan_air_flow_ok_obj_list.append(fan_obj)
                fan_air_flow_ok_set.add(fan_obj.origin_name)
                ledcontrol_debug("%s air flow check ok, origin name: [%s], display name: [%s], fan air flow: [%s], board air flow: [%s]" %
                                 (fan_obj.name, fan_obj.origin_name, fan_obj.display_name, fan_obj.air_flow, self.board_air_flow))
                led_status = COLOR_GREEN
                fan_module_led_list.append(led_status)
            if led_status > fan_obj.led_status:
                fan_obj.led_status = led_status
        if len(fan_module_led_list) != 0:
            fan_led_status = max(fan_module_led_list)
            fan_led_status_list.append(fan_led_status)
        # check fan mixing
        if len(fan_air_flow_ok_set) > 1 and fan_air_flow_ok_set not in self.fan_mix_list:
            for fan_obj in fan_air_flow_ok_obj_list:
                air_flow_warn("%%AIR_FLOW_MONITOR-4-FAN: %s mixing, fan model: %s, air flow: %s." %
                              (fan_obj.name, fan_obj.origin_name, fan_obj.air_flow), fan_obj.name)
            fan_led_status = COLOR_AMBER
            fan_led_status_list.append(fan_led_status)
        # check fan air flow error number
        if fan_air_flow_err_num == 0:
            fan_led_status = COLOR_GREEN
        elif fan_air_flow_err_num <= self.fan_air_flow_amber_num:
            fan_led_status = COLOR_AMBER
        else:
            fan_led_status = COLOR_RED
        fan_led_status_list.append(fan_led_status)

        fan_led_status = max(fan_led_status_list)
        ledcontrol_debug("monitor fan air flow, set fan led: %s" % LED_STATUS_DICT.get(fan_led_status))
        return fan_led_status

    def get_monitor_psu_air_flow(self):
        if self.psu_air_flow_monitor == 0:
            ledcontrol_debug("psu air flow monitor not open, default green")
            return COLOR_GREEN

        ret = self.check_board_air_flow()
        if ret is False:
            ledcontrol_debug("check board air flow error, skip psu air flow monitor.")
            return COLOR_GREEN

        psu_led_status_list = []
        psu_module_led_list = []
        psu_air_flow_err_num = 0
        for psu_obj in self.psu_obj_list:
            if psu_obj.present != STATUS_PRESENT:
                psu_module_led_list.append(COLOR_GREEN)
                continue
            if psu_obj.air_flow == self.na_ret:
                air_flow_warn("%%AIR_FLOW_MONITOR-4-PSU: %s get air flow failed, psu model: %s, air flow: %s." %
                              (psu_obj.name, psu_obj.display_name, psu_obj.air_flow), psu_obj.name)
                led_status = COLOR_AMBER
                psu_module_led_list.append(led_status)
            elif psu_obj.air_flow != self.board_air_flow:
                air_flow_emerg("%%AIR_FLOW_MONITOR-0-PSU: %s air flow error, psu model: %s, psu air flow: %s, board air flow: %s." %
                               (psu_obj.name, psu_obj.display_name, psu_obj.air_flow, self.board_air_flow), psu_obj.name)
                led_status = COLOR_RED
                psu_air_flow_err_num += 1
            else:
                ledcontrol_debug("%s psu air flow check ok, origin name: [%s], display name: [%s], psu air flow: [%s], board air flow: [%s]" %
                                 (psu_obj.name, psu_obj.origin_name, psu_obj.display_name, psu_obj.air_flow, self.board_air_flow))
                led_status = COLOR_GREEN
                psu_module_led_list.append(led_status)
            if led_status > psu_obj.led_status:
                psu_obj.led_status = led_status

        if len(psu_module_led_list) != 0:
            psu_led_status = max(psu_module_led_list)
            psu_led_status_list.append(psu_led_status)

        # check fan air flow error number
        if psu_air_flow_err_num == 0:
            psu_led_status = COLOR_GREEN
        elif psu_air_flow_err_num <= self.psu_air_flow_amber_num:
            psu_led_status = COLOR_AMBER
        else:
            psu_led_status = COLOR_RED
        psu_led_status_list.append(psu_led_status)

        psu_led_status = max(psu_led_status_list)
        ledcontrol_debug("monitor psu air flow, set psu led: %s" % LED_STATUS_DICT.get(psu_led_status))
        return psu_led_status

    def get_temp_led_status(self):
        if self.checkTempCrit() is True:
            led_status = COLOR_RED
        elif self.checkTempWarning() is True:
            led_status = COLOR_AMBER
        else:
            led_status = COLOR_GREEN
        ledcontrol_debug("monitor temperature, set led: %s" % LED_STATUS_DICT.get(led_status))
        return led_status

    def get_sys_led_follow_fan_status(self):

        if self.checkfan:
            sys_led_status = self.board_fan_led_status
            ledcontrol_debug("sys led follow fan led, set sys led: %s" % LED_STATUS_DICT.get(sys_led_status))
        else:
            sys_led_status = COLOR_GREEN
            ledcontrol_debug("sys led don't follow fan led, set default green")
        return sys_led_status

    def get_sys_led_follow_psu_status(self):
        if self.checkpsu:
            sys_led_status = self.board_psu_led_status
            ledcontrol_debug("sys led follow psu led, set sys led: %s" % LED_STATUS_DICT.get(sys_led_status))
        else:
            sys_led_status = COLOR_GREEN
            ledcontrol_debug("sys led don't follow psu led, set default green")
        return sys_led_status

    def get_cpu_boot_src_status(self):
        ret, boot_source = self.int_case.get_cpu_boot_source()
        if ret == True:
            if boot_source == MASTER_FLASH:
                sys_led_status = COLOR_GREEN
            elif boot_source == SLAVE_FLASH:
                sys_led_status = COLOR_AMBER
            else:
                sys_led_status = COLOR_GREEN
                ledcontrol_debug("get cpu boot source invalid, set default green")
        else:
            sys_led_status = COLOR_GREEN
            ledcontrol_debug("get cpu boot source failed, set default green")
        ledcontrol_debug("get cpu boot source, set sys led: %s" % LED_STATUS_DICT.get(sys_led_status))
        return sys_led_status

    def get_bmc_boot_src_status(self):
        ret, boot_source = self.int_case.get_bmc_boot_source()
        if ret == True:
            if boot_source == MASTER_FLASH:
                bmc_led_status = COLOR_GREEN
            elif boot_source == SLAVE_FLASH:
                bmc_led_status = COLOR_AMBER
            else:
                bmc_led_status = COLOR_GREEN
                ledcontrol_debug("get bmc boot source invalid, set default green")
        else:
            bmc_led_status = COLOR_GREEN
            ledcontrol_debug("get bmc boot source failed, set default green")
        ledcontrol_debug("get bmc boot source, set bmc led: %s" % LED_STATUS_DICT.get(bmc_led_status))
        return bmc_led_status

    def set_redundancy_num(self, path, value, component):
        if not os.path.exists(path):
            msg = f"{component} redundancy file not found: {path}"
            ledcontrol_error(msg)
            return False

        try:
            if value is None:
                ledcontrol_error(f"Failed to set {component} redundancy number: {component.lower()}_amber_num is None")
                return False
            exec_cmd = f"echo {value} > {path}"
            status, output = exec_os_cmd(exec_cmd)
            
            if status != 0:
                ledcontrol_error(
                    f"Failed to set {component} redundancy number. "
                    f"Command: {exec_cmd}, "
                    f"Status: {status}, "
                    f"Error: {output.strip()}"
                )
                return False

            ledcontrol_debug(
                f"Successfully set {component} redundancy number. "
                f"Value: {value}, "
                f"Path: {path}"
            )
            return True

        except Exception as e:
            ledcontrol_error(
                f"Unexpected error when setting {component} redundancy number. "
                f"Error: {str(e)}, "
                f"Path: {path}, "
                f"Redundancy_num: {value}"
            )
            return False

    def set_psu_redundancy_num(self):
        return self.set_redundancy_num(self.__psu_redundancy_path, self.psu_amber_num, "PSU")

    def set_fan_redundancy_num(self):
        return self.set_redundancy_num(self.__fan_redundancy_path, self.fan_amber_num, "FAN")

    def deal_Sysled_pi_ctrl_func(self):
        status_color_map = {
            SYSTEM_STATUS_OK: COLOR_GREEN,
            SYSTEM_STATUS_NOTICE: COLOR_GREEN,
            SYSTEM_STATUS_WARNING: COLOR_AMBER,
            SYSTEM_STATUS_CRITICAL: COLOR_RED
        }

        try:
            with open(SYSTEM_STATUS_FILE, 'r') as f:
                status = int(f.read().strip())
        except Exception as e:
            self.set_sys_led("green")
            ledcontrol_error("sysled get system_status fail: %s" % str(e))
            return
        if status not in status_color_map:
            ledcontrol_error("sysled: Invalid system status %d, using default color GREEN" % status)
            sys_led_status = COLOR_GREEN
        else:
            sys_led_status = status_color_map.get(status, COLOR_GREEN)

        sys_led_color = LED_STATUS_DICT.get(sys_led_status)
        self.set_sys_led(sys_led_color)

    def dealSysLedStatus(self):
        if self.sysled_pi_ctrl == 1:
            self.deal_Sysled_pi_ctrl_func()
            return

        sys_led_status_list = []
        if self.sysled_check_temp == 1:
            ledcontrol_debug("sys led check temperature status")
            # get_monitor_temp
            self.get_monitor_temp()
            # monitor temp get sys led status
            sys_led_status = self.get_temp_led_status()
            ledcontrol_debug("monitor temperature to get sys led status: %s" %
                LED_STATUS_DICT.get(sys_led_status))
            sys_led_status_list.append(sys_led_status)
        else:
            ledcontrol_debug("sys led don't need to check temperature status")

        # check sys led follow fan led status
        sys_led_status = self.get_sys_led_follow_fan_status()
        sys_led_status_list.append(sys_led_status)

        # check sys led follow psu led status
        sys_led_status = self.get_sys_led_follow_psu_status()
        sys_led_status_list.append(sys_led_status)

        if self.sysled_check_fw_up == 1:
            ledcontrol_debug("sys led check firmware upgrade")
            # monitor firmware get sys led status
            sys_led_status = self.get_fw_up_led_status()
            ledcontrol_debug("monitor firmware upgrade to get sys led status: %s" %
                LED_STATUS_DICT.get(sys_led_status))
            sys_led_status_list.append(sys_led_status)
        else:
            ledcontrol_debug("sys led don't need to check firmware upgrade")

        # check cpu boot source
        if self.bios_boot_source_check == 1:
            sys_led_status = self.get_cpu_boot_src_status()
            sys_led_status_list.append(sys_led_status)
            ledcontrol_debug("check cpu boot source to get sys led status: %s" % LED_STATUS_DICT.get(sys_led_status))

        sys_led_status = max(sys_led_status_list)
        sys_led_color = LED_STATUS_DICT.get(sys_led_status)

        # set sys led
        self.set_sys_led(sys_led_color)

    def ioctl_dealSysLedStatus(self):
        self.get_monitor_temp()
        self.fd = os.open("/dev/sys_led", os.O_RDWR)
        try:
            if self.checkTempCrit() is True:
                color = "red"
                cmd = self.__sys_led_ioctl_red
            elif self.checkTempWarning() is True:
                color = "amber"
                cmd = self.__sys_led_ioctl_amber
            else:
                color = "green"
                cmd = self.__sys_led_ioctl_green

            ioctl(self.fd, cmd, 0)
            ledcontrol_debug("ioctl_dealSysLedStatus success, color:%s," % color)
        except Exception as e:
            ledcontrol_error(str(e))
        os.close(self.fd)
        self.fd = None

    def dealFanLedStatus(self):
        fan_led_status_list = []
        # update fan info
        self.update_fan_info()

        # monitor fan status first
        fan_led_status = self.get_monitor_fan_status()
        fan_led_status_list.append(fan_led_status)

        # monitor fan air flow
        fan_led_status = self.get_monitor_fan_air_flow()
        fan_led_status_list.append(fan_led_status)

        self.board_fan_led_status = max(fan_led_status_list)
        fan_led_color = LED_STATUS_DICT.get(self.board_fan_led_status)

        # set fan led
        self.set_fan_led(fan_led_color)
        # set fan module led
        if self.is_set_fan_module_led:
            self.set_fan_module_led()

    def dealPsuLedStatus(self):
        psu_led_status_list = []
        # update psu info
        self.update_psu_info()

        # monitor psu status first
        psu_led_status = self.get_monitor_psu_status()
        psu_led_status_list.append(psu_led_status)

        # monitor psu air flow
        psu_led_status = self.get_monitor_psu_air_flow()
        psu_led_status_list.append(psu_led_status)

        self.board_psu_led_status = max(psu_led_status_list)
        psu_led_color = LED_STATUS_DICT.get(self.board_psu_led_status)

        # set psu led
        self.set_psu_led(psu_led_color)

    def dealSmbLedStatus(self):
        if self.smbled_ctrl == 0:
            ledcontrol_debug("Don't need to control SMB led")
            return

        ledcontrol_debug("Start to control SMB led")
        smb_led_status_list = []

        # get_monitor_temp
        self.get_monitor_temp()
        # monitor temp get smb led status
        smb_led_status = self.get_temp_led_status()
        smb_led_status_list.append(smb_led_status)
        ledcontrol_debug("monitor temperature to get smb led status: %s" % LED_STATUS_DICT.get(smb_led_status))

        # monitor volgate get smb led status
        smb_led_status = self.get_voltage_led_status()
        smb_led_status_list.append(smb_led_status)
        ledcontrol_debug("monitor voltage to get smb led status: %s" % LED_STATUS_DICT.get(smb_led_status))

        smb_led_status = max(smb_led_status_list)
        smb_led_color = LED_STATUS_DICT.get(smb_led_status)

        # set smb led
        self.set_smb_led(smb_led_color)

    def dealBmcLedStatus(self):
        if self.bmcled_ctrl == 0:
            ledcontrol_debug("Don't need to control BMC led")
            return

        ledcontrol_debug("Start to control BMC led")
        bmc_led_status_list = []

        # check bmc boot source
        bmc_led_status = self.get_bmc_boot_src_status()
        bmc_led_status_list.append(bmc_led_status)
        ledcontrol_debug("check bmc boot source to get bmc led status: %s" % LED_STATUS_DICT.get(bmc_led_status))

        bmc_led_status = max(bmc_led_status_list)
        bmc_led_color = LED_STATUS_DICT.get(bmc_led_status)

        # set bmc led
        self.set_bmc_led(bmc_led_color)

    def do_ledcontrol(self):
        self.dealPsuLedStatus()
        self.dealFanLedStatus()
        if self.__sys_led_ioctl == 1:
            self.ioctl_dealSysLedStatus()
        else:
            self.dealSysLedStatus()
        self.dealSmbLedStatus()
        self.dealBmcLedStatus()

    def fan_obj_init(self):
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            fan_obj = DevFan(fan_name, self.fan_air_flow_monitor, self.int_case, 
                            self.__fan_rotor_threshold_config, self.__lowest_rpm, self.__fan_rpm_check_warning)
            self.fan_obj_list.append(fan_obj)
        ledcontrol_debug("fan object initialize success")

    def psu_obj_init(self):
        psu_num = self.get_psu_total_number()
        for i in range(psu_num):
            psu_name = "PSU" + str(i + 1)
            psu_obj = DevPsu(psu_name, self.psu_air_flow_monitor, self.int_case)
            self.psu_obj_list.append(psu_obj)
        ledcontrol_debug("psu object initialize success")

    def run(self):
        while True:
            try:
                debug_init()
                if os.path.exists(LED_CONTRL_FILE) is True:
                    ledcontrol_debug("file exists, do nothing!")
                    time.sleep(5)
                    continue
                self.do_ledcontrol()
            except Exception as e:
                traceback.print_exc()
                ledcontrol_error(str(e))
            finally:
                time.sleep(self.interval)
@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    pass

@main.command()
def start():
    '''start process '''
    debug_init()
    ledcontrol_debug("enter main")
    led_control = ledcontrol()
    led_control.set_fan_redundancy_num()
    led_control.set_psu_redundancy_num()
    led_control.fan_obj_init()
    led_control.psu_obj_init()
    led_control.run()

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