#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# @Time    : 2023/6/19 17:44
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
# @Function: Fan control strategy main program

try:
    import re
    import os
    import sys
    import getopt
    import subprocess
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import signal
    from sonic_platform import platform
    from . import CPUPIDRegulation
    from . import FanLinearAdjustment
    from . import SwitchInternalPIDRegulation
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

FUNCTION_NAME = "FanControl"
ERROR_COLOR = "amber"
NORMAL_COLOR = "green"
DUTY_MAX = 100
DUTY_MIN  = 50
FAN_NUMBER = 7
PSU_NUMBER = 2
SENSOR_NUMBER = 12
Fan_MIN = 1000

CHECK_PSU_STATUS = "i2cget -y -f 6 0x0d 0x60"

class FanControl(object):
    """
    Make a class we can use to capture stdout in the log
    """
    # static temp var
    _ori_temp = 0
    _new_perc = DUTY_MAX / 2
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")
    init_fan_temperature = [0]

    def __init__(self):
        self.FanLinearAdjustment = FanLinearAdjustment.FanLinearAdjustment(DUTY_MAX, DUTY_MIN, FAN_NUMBER, PSU_NUMBER, SENSOR_NUMBER)
        self.SwitchInternalPIDRegulation = SwitchInternalPIDRegulation.SwitchInternalPIDRegulation()
        self.CPUPIDRegulation = CPUPIDRegulation.CPUPIDRegulation()
        # Needs a logger and a logger level
        formatter = logging.Formatter('%(name)s %(message)s')
        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setFormatter(formatter)
        sys_handler.ident = 'common'
        self.syslog.setLevel(logging.WARNING)
        self.syslog.addHandler(sys_handler)
        self.platform_chassis_obj = platform.Platform().get_chassis()
        self.pwr_led_status = None
        self.fan_led_status = None
        self.alarm_led_status = None
        self.sys_led_status = None
        self.warned_fans = set()

    def get_psu_status(self, fan_duty_list):
        """
        Get PSU Status.If one PSU not OK, all of the fans pwm will increase to 100
        :param fan_duty_list: A list.TO app the fans target pwm
        """
        psu_presence_list = [True, True]
        try:
            psu_info_bytes = subprocess.check_output(CHECK_PSU_STATUS, shell=True)
            psu_info = psu_info_bytes.decode('utf-8').strip()
            psus_present = bin(int(psu_info, 16))[6:8]
            for psu_index in range(PSU_NUMBER):
                psu_presence = True if psus_present[psu_index] == "0" else False
                if not psu_presence:
                    psu_presence_list[psu_index] = False
                else:
                    psu_presence_list[psu_index] = True
            if False in psu_presence_list:
                fan_duty_list.append(DUTY_MAX)
        except Exception:
            pass

    def get_fan_status(self):
        """
        Get all of fans status(fan drawer)
        :return: A list indicating the status of all groups fans
        """
        fan_presence_list = [True, True, True, True, True, True, True]  # Default state: fans are OK
        for fan_drawer_index in range(FAN_NUMBER):
            try:
                fan_presence = self.platform_chassis_obj.get_fan_drawer(fan_drawer_index).get_presence()
                fan_status = self.platform_chassis_obj.get_fan_drawer(fan_drawer_index).get_status()
                if not all([fan_presence, fan_status]):
                    fan_presence_list[fan_drawer_index] = False
            except Exception:
                pass
        return fan_presence_list

    def check_fans_presence(self):
        """
        check all fans presence or not
        """
        fans_inserted_list = self.get_fan_status()
        fans_inserted_num = fans_inserted_list.count(True)
        if fans_inserted_num == 0:  # all fans broken, cpld will power off
            self.syslog.critical("No fans inserted!!! Severe overheating hazard. "
                                 "Please insert Fans immediately or power off the device")

    def set_fans_pwm_by_rpm(self, fan_duty_list):
        """
        Set fans pwm by fans rpm. If all fans normal or 1 fan broken,
        manage the fans follow thermal policy.
        More than 1 fans broken, Will increase the fan speed to 100%
        :param fan_duty_list: A list.TO app the fans target pwm
        """
        fan_rpm_error_list = list()
        for fan in self.platform_chassis_obj.get_all_fans():
            fan_name = fan.get_name()
            fan_speed_rpm = fan.get_speed_rpm()
            if fan_name.endswith("1") and fan_speed_rpm < Fan_MIN :
                fan_rpm_error_list.append(fan_name)
            if fan_name.endswith("2") and fan_speed_rpm < Fan_MIN:
                fan_rpm_error_list.append(fan_name)
        if not fan_rpm_error_list:
            for fan_drawer_index in range(FAN_NUMBER):
                check_led_status = self.platform_chassis_obj.get_fan_drawer(fan_drawer_index).get_status_led()
                if check_led_status != NORMAL_COLOR:
                    self.platform_chassis_obj.get_fan_drawer(fan_drawer_index).set_status_led(NORMAL_COLOR)
            return None
        if len(fan_rpm_error_list) >= 2:
            self.syslog.warning("%s rpm less than the set minimum speed. "
                                "Will increase the fan speed to 100%%" % fan_rpm_error_list)
            fan_duty_list.append(DUTY_MAX)

        fan_modules_index_list = list(set(int(re.findall(r"Fantray(\d)_\d", x)[0]) for x in fan_rpm_error_list))
        for error_fan_drawer in fan_modules_index_list:
            check_led_status = self.platform_chassis_obj.get_fan_drawer(error_fan_drawer-1).get_status_led()
            if check_led_status != ERROR_COLOR:
                self.platform_chassis_obj.get_fan_drawer(error_fan_drawer-1).set_status_led(ERROR_COLOR)

    def get_linear_pid_pwm(self, fan_duty_list):
        """
        Get the pwm value of liner regulation, cpu pid adjustment, switch internal pid adjustment
        :param fan_duty_list: A list.TO app the fans target pwm
        """
        linear_regulation = self.FanLinearAdjustment.linear_control()
        cpu_pid_adjustment = self.CPUPIDRegulation.pid_control()
        sw_pid_adjustment = self.SwitchInternalPIDRegulation.pid_control()
        fan_duty_list.append(linear_regulation)
        fan_duty_list.append(cpu_pid_adjustment)
        fan_duty_list.append(sw_pid_adjustment)

    def manage_fans(self):
        """
        Set the fan speed according to the Fan Control Strategy
        """
        fan_duty_speed_list = list()
        get_target_speed_list = list()

        # Fan speed setting judgment-PSU
        self.get_psu_status(fan_duty_speed_list)

        # Fan speed setting judgment-FAN presence
        self.check_fans_presence()

        # Fan speed setting judgment-FAN SPEED
        self.set_fans_pwm_by_rpm(fan_duty_speed_list)

        # Fan speed setting judgment-linear and cpu pid and sw pid
        self.get_linear_pid_pwm(fan_duty_speed_list)

        self._new_perc = max(fan_duty_speed_list)
        if self._new_perc < DUTY_MIN:
            self._new_perc = DUTY_MIN
        if self._new_perc > DUTY_MAX:
            self._new_perc = DUTY_MAX
            
        for fan_drawer_index in range(FAN_NUMBER):
            get_target_speed = self.platform_chassis_obj.get_fan(fan_drawer_index).get_target_speed()
            get_target_speed_list.append(get_target_speed)

        if self._new_perc not in get_target_speed_list:
            for fan in self.platform_chassis_obj.get_all_fans():
                fan.set_speed(self._new_perc)

    def check_and_set_pwr_led(self):
        any_psu_not_ok = False
        for index in range(2):
            psu_status = self.platform_chassis_obj.get_psu(index).get_powergood_status()
            if psu_status is None or not psu_status:
                self.syslog.warning(f"Power Supply Unit {index + 1} is not OK or missing.")
                any_psu_not_ok = True
                break

        if any_psu_not_ok:
            if self.pwr_led_status != "amber":
                self.platform_chassis_obj.set_system_led("PWR_LED", "amber")
                self.syslog.warning("Setting PWR_LED to amber.")
                self.pwr_led_status = "amber"
                self.check_and_set_sys_led(True)
        else:
            if self.pwr_led_status != "green":
                self.platform_chassis_obj.set_system_led("PWR_LED", "green")
                self.syslog.info("Setting PWR_LED to green.")
                self.pwr_led_status = "green"
                self.check_and_set_sys_led(False)

    def check_and_set_fan_led(self):
        any_fan_not_ok = False
        for fan in self.platform_chassis_obj.get_all_fans():
            fan_status = fan.get_status()
            fan_name = fan.get_name()
            if fan_status is None or not fan_status:
                self.warned_fans.add(fan_name)
                any_fan_not_ok = True

        if any_fan_not_ok:
            self.syslog.warning(f" {self.warned_fans} is not OK or missing.")
            if self.fan_led_status != "amber":
                self.platform_chassis_obj.set_system_led("FAN_LED", "amber")
                self.syslog.warning("Setting FAN_LED to amber.")
                self.fan_led_status = "amber"
                self.check_and_set_sys_led(True)
        else:
            if self.fan_led_status != "green":
                self.platform_chassis_obj.set_system_led("FAN_LED", "green")
                self.syslog.info("Setting FAN_LED to green.")
                self.fan_led_status = "green"
                self.check_and_set_sys_led(False)

    def check_and_set_sys_led(self, is_alarm):
        if is_alarm and self.sys_led_status != "amber":
            self.platform_chassis_obj.set_system_led("SYS_LED", "amber")
            self.syslog.info("Setting SYS_LED to amber.")
            self.sys_led_status = "amber"
        elif not is_alarm and self.sys_led_status != "green":
            self.platform_chassis_obj.set_system_led("SYS_LED", "green")
            self.syslog.info("Setting SYS_LED to green")
            self.sys_led_status = "green"

    def check_and_set_alarm_led(self):
        if self.sys_led_status == "amber":
            if self.alarm_led_status != "amber_blink_1hz":
                self.platform_chassis_obj.set_system_led("ALARM_LED", "amber_blink_1hz")
                self.syslog.info("Setting ALARM_LED to amber_blink_1hz.")
                self.alarm_led_status = "amber_blink_1hz"
        else:
            if self.alarm_led_status != "green":
                self.platform_chassis_obj.set_system_led("ALARM_LED", "green")
                self.syslog.info("Setting ALARM_LED to green")
                self.alarm_led_status = "green"

    def manage_leds(self):
        self.check_and_set_pwr_led()
        self.check_and_set_fan_led()
        self.check_and_set_alarm_led()

def handler(signum, frame):
    logging.warning('Cause signal %d, will set all fan speed to max.' % signum)
    platform_chassis = platform.Platform().get_chassis()
    set_error = list()
    fan_index = 1
    for fan in platform_chassis.get_all_fans():
        set_stat = fan.set_speed(DUTY_MAX)
        fan_drawer = fan_index//2
        if not set_stat:
            set_error.append(fan_drawer)
        fan_index += 1
    if set_error:
        logging.error('Fail. Set Fantray %s to (%d) failed' % (list(set(set_error)), DUTY_MAX))
    sys.exit(0)


def main():
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
    monitor = FanControl()
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_leds()
        monitor.manage_fans()
        time.sleep(10)
