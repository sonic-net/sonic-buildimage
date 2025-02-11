#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import os
import time
import logging
from logging.handlers import RotatingFileHandler

from plat_hal.interface import interface
from plat_hal.baseutil import baseutil


DEBUG_FILE = "/etc/.monitor_fan_debug_flag"

LOG_FILE = "/var/log/intelligent_monitor/monitor_fan_log"

E2_NAME = "ONIE_E2"


def _init_logger():
    if not os.path.exists(LOG_FILE):
        os.system("mkdir -p %s" % os.path.dirname(LOG_FILE))
        os.system("sync")
    handler = RotatingFileHandler(filename=LOG_FILE, maxBytes=5 * 1024 * 1024, backupCount=1)
    formatter = logging.Formatter("%(asctime)s %(levelname)s %(filename)s[%(funcName)s][%(lineno)s]: %(message)s")
    handler.setFormatter(formatter)
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)
    logger.addHandler(handler)
    return logger


class Fan(object):

    def __init__(self, name, hal_interface):
        self.name = name
        self.fan_dict = {}
        self.int_case = hal_interface
        self.update_time = 0
        self.pre_present = False
        self.pre_status = True
        self.plugin_cnt = 0
        self.plugout_cnt = 0
        self.status_normal_cnt = 0
        self.status_error_cnt = 0

    def fan_dict_update(self):
        local_time = time.time()
        if not self.fan_dict or (local_time - self.update_time) >= 1:  # update data every 1 seconds
            self.update_time = local_time
            self.fan_dict = self.int_case.get_fan_info(self.name)

    def get_model(self):
        self.fan_dict_update()
        return self.fan_dict["NAME"]

    def get_serial(self):
        self.fan_dict_update()
        return self.fan_dict["SN"]

    def get_presence(self):
        return self.int_case.get_fan_presence(self.name)

    def get_rotor_speed(self, rotor_name):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        fan_dir = {}
        fan_dir = self.int_case.get_fan_info_rotor(self.name)
        # get fan rotor pwm
        value = fan_dir[rotor_name]["Speed"]
        max_speed = fan_dir[rotor_name]["SpeedMax"]

        if isinstance(value, str) or value is None:
            return 0
        pwm = value * 100 / max_speed
        if pwm > 100:
            pwm = 100
        elif pwm < 0:
            pwm = 0
        return int(pwm)

    def get_rotor_speed_tolerance(self, rotor_name):
        """
        Retrieves the speed tolerance of the fan
        Returns:
            An integer, the percentage of variance from target speed which is
        considered tolerable
        """
        # The default tolerance value is fixed as 30%
        fan_dir = {}
        fan_dir = self.int_case.get_fan_info_rotor(self.name)
        # get fan rotor tolerance
        tolerance = fan_dir[rotor_name]["Tolerance"]

        if isinstance(tolerance, str) or tolerance is None:
            return 30
        return tolerance

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        pwm = self.int_case.get_fan_speed_pwm(self.name, 0)
        return int(pwm)

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        if not self.get_presence():
            return False

        rotor_num = self.int_case.get_fan_rotor_number(self.name)
        for i in range(rotor_num):
            rotor_name = "Rotor" + str(i + 1)
            speed = self.get_rotor_speed(rotor_name)
            tolerance = self.get_rotor_speed_tolerance(rotor_name)
            target = self.get_target_speed()
            if (speed - target) > target * tolerance / 100:
                return False
            if (target - speed) > target * tolerance / 100:
                return False

        return True

    def get_direction(self):
        """
        Retrieves the fan airflow direction
        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction

        Notes:
            - Forward/Exhaust : Air flows from Port side to Fan side.
            - Reverse/Intake  : Air flows from Fan side to Port side.
        """
        self.fan_dict_update()
        return self.fan_dict["AirFlow"]


class MonitorFan(object):

    def __init__(self):
        self.int_case = interface()
        self.logger = _init_logger()
        self.fan_obj_list = []
        self.__config = baseutil.get_monitor_config()
        self.__monitor_fan_config = self.__config.get("monitor_fan_para", {})
        self.__present_interval = self.__monitor_fan_config.get("present_interval", 0.5)
        self.__status_interval = self.__monitor_fan_config.get("status_interval", 5)
        self.__present_check_cnt = self.__monitor_fan_config.get("present_check_cnt", 3)
        self.__status_check_cnt = self.__monitor_fan_config.get("status_check_cnt", 3)

    def debug_init(self):
        if os.path.exists(DEBUG_FILE):
            self.logger.setLevel(logging.DEBUG)
        else:
            self.logger.setLevel(logging.INFO)

    def get_fan_total_number(self):
        return self.int_case.get_fan_total_number()

    def get_device_airflow(self):
        return self.int_case.get_device_airflow(E2_NAME)

    def fan_obj_init(self):
        fan_num = self.get_fan_total_number()
        for i in range(fan_num):
            fan_name = "FAN" + str(i + 1)
            fan_obj = Fan(fan_name, self.int_case)
            self.fan_obj_list.append(fan_obj)
        self.logger.info("fan object initialize success")

    def fan_airflow_check(self, fan_obj):
        fan_airflow = fan_obj.get_direction()
        device_airflow = self.get_device_airflow()
        if fan_airflow != device_airflow:
            self.logger.error("%s airflow[%s] not match device airflow[%s]", fan_obj.name, fan_airflow, device_airflow)
        else:
            self.logger.debug("%s airflow[%s] match device airflow[%s]", fan_obj.name, fan_airflow, device_airflow)

    def fan_plug_in_out_check(self, fan_obj):
        present = fan_obj.get_presence()
        if present is True:
            self.logger.debug("%s is present", fan_obj.name)
        else:
            self.logger.debug("%s is absent", fan_obj.name)

        if present != fan_obj.pre_present:
            if present is True:
                fan_obj.plugin_cnt += 1
                fan_obj.plugout_cnt = 0
                if fan_obj.plugin_cnt >= self.__present_check_cnt:
                    fan_obj.pre_present = True
                    self.logger.info("%s [serial:%s] is plugin", fan_obj.name, fan_obj.get_serial())
                    self.fan_airflow_check(fan_obj)
            else:
                fan_obj.plugin_cnt = 0
                fan_obj.plugout_cnt += 1
                if fan_obj.plugout_cnt >= self.__present_check_cnt:
                    fan_obj.pre_present = False
                    self.logger.info("%s is plugout", fan_obj.name)
        else:
            fan_obj.plugin_cnt = 0
            fan_obj.plugout_cnt = 0
            self.logger.debug("%s present status is not change", fan_obj.name)

    def fan_status_check(self, fan_obj):
        status = fan_obj.get_status()
        if status is True:
            self.logger.debug("%s is normal", fan_obj.name)
        else:
            self.logger.debug("%s is error", fan_obj.name)

        if status != fan_obj.pre_status:
            if status is True:
                fan_obj.status_normal_cnt += 1
                fan_obj.status_error_cnt = 0
                if fan_obj.status_normal_cnt >= self.__status_check_cnt:
                    fan_obj.pre_status = True
                    self.logger.info(
                        "%s [serial:%s] is form error change to normal",
                        fan_obj.name,
                        fan_obj.get_serial())
            else:
                fan_obj.status_normal_cnt = 0
                fan_obj.status_error_cnt += 1
                if fan_obj.status_error_cnt >= self.__status_check_cnt:
                    fan_obj.pre_status = False
                    self.logger.info(
                        "%s [serial:%s] is form normal change to error",
                        fan_obj.name,
                        fan_obj.get_serial())
        else:
            fan_obj.status_normal_cnt = 0
            fan_obj.status_error_cnt = 0
            self.logger.debug("%s status is not change", fan_obj.name)

    def checkFanPresence(self):
        for fan_obj in self.fan_obj_list:
            self.fan_plug_in_out_check(fan_obj)

    def checkFanStatus(self):
        for fan_obj in self.fan_obj_list:
            self.fan_status_check(fan_obj)

    def run(self):
        start_time = time.time()
        while True:
            try:
                self.debug_init()
                delta_time = time.time() - start_time
                if self.__present_interval <= self.__status_interval:
                    if delta_time >= self.__status_interval or delta_time < 0:
                        self.checkFanStatus()
                        start_time = time.time()
                    else:
                        self.checkFanPresence()
                    time.sleep(self.__present_interval)
                else:
                    if delta_time >= self.__present_interval or delta_time < 0:
                        self.checkFanPresence()
                        start_time = time.time()
                    else:
                        self.checkFanStatus()
                    time.sleep(self.__status_interval)
            except Exception as e:
                self.logger.error('EXCEPTION: %s.', str(e))


if __name__ == '__main__':
    monitor_fan = MonitorFan()
    monitor_fan.fan_obj_init()
    monitor_fan.run()
