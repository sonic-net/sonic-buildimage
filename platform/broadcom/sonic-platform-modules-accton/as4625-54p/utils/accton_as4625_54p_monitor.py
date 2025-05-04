#!/usr/bin/env python
# -*- coding: utf-8 -*
# Copyright (c) 2019 Edgecore Networks Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
# ------------------------------------------------------------------

try:
    import sys
    import logging
    import logging.config
    import logging.handlers
    import time
    import sonic_platform.platform
    from sonic_platform.helper import APIHelper
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as4625_54p_monitor'
I2C_PATH = "/sys/bus/i2c/devices/{}-00{}/"

FAN_SPEED_DEFAULT = 38
FAN_SPEED_MAX = 100

ERROR_CONFIG_LOGGING = 1
ERROR_CHASSIS_LOAD = 2
ERROR_DEVICE_MONITOR_LOAD = 3

global log_file
global log_level

platform_chassis = None

def configure_logging(log_file, log_level):
    """Needs a logger and a logger level."""
    # set up logging to file
    logging.basicConfig(
        filename=log_file,
        filemode='w',
        level=log_level,
        format='[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
        datefmt='%H:%M:%S')
    # set up logging to console
    if log_level == logging.DEBUG:
        console = logging.StreamHandler()
        console.setLevel(log_level)
        formatter = logging.Formatter(
            '%(name)-12s: %(levelname)-8s %(message)s')
        console.setFormatter(formatter)
        logging.getLogger('').addHandler(console)

    sys_handler = logging.handlers.SysLogHandler(
        address='/dev/log')
    sys_handler.setLevel(logging.WARNING)
    logging.getLogger('').addHandler(sys_handler)

# Instantiate platform-specific Chassis class
def load_platform_chassis():
    global platform_chassis

    # Load new platform api class
    if platform_chassis:
        return True

    try:
        platform_chassis = sonic_platform.platform.Platform().get_chassis()
    except Exception as e:
        logging.error("Failed to instantiate Chassis: %s", repr(e))

    if not platform_chassis:
        return False

    return True

THERMAL_1_ON_MAIN_BROAD = '0x4a'
THERMAL_2_ON_MAIN_BROAD = '0x4b'
THERMAL_3_ON_MAIN_BROAD = '0x4d'
THERMAL_5_ON_MAIN_BROAD = '0x4f'

# Temperature Policy
class device_monitor(object):
    PWM_STATE_NORMAL = 0
    PWM_STATE_CRITICAL = 1

    def __init__(self, chassis):
        self._api_helper = APIHelper()
        self.chassis = chassis
        self.warning = False
        self.shutdown = False
        self.fan_failed = False
        self.pwm_state = self.PWM_STATE_NORMAL

        self._set_all_fan_speed(self._get_default_speed())

    def _set_all_fan_speed(self, speed):
        ret = True

        for drawer_index, drawer in enumerate(self.chassis.get_all_fan_drawers()):
            for fan_index, fan in enumerate(drawer.get_all_fans()):
                if fan.set_speed(speed) is not True:
                    logging.error("fan.set_speed error, drawer(%d)-fan(%d)", drawer_index, fan_index)
                    ret = False

        return ret

    def _get_default_speed(self):
        return FAN_SPEED_DEFAULT

    def _get_fan_speed(self):
        return {
            self.PWM_STATE_NORMAL: FAN_SPEED_DEFAULT,
            self.PWM_STATE_CRITICAL: FAN_SPEED_MAX
        }.get(self.pwm_state, self._get_default_speed())

    def _get_system_temperature(self):
        sys_thermals = [
            THERMAL_1_ON_MAIN_BROAD,
            THERMAL_2_ON_MAIN_BROAD,
            THERMAL_3_ON_MAIN_BROAD,
            THERMAL_5_ON_MAIN_BROAD
        ]

        sys_temp = 0
        for thermal in self.chassis.get_all_thermals():
            for item in sys_thermals:
                if item in thermal.get_name():
                    sys_temp += thermal.get_temperature()
                    break

        return sys_temp

    def _get_system_low_threshold(self):
        return 135

    def _get_system_high_threshold(self):
        return 202

    def _refresh_thermal_status(self):
        # check if current state is valid
        if (self.pwm_state < self.PWM_STATE_NORMAL or
            self.pwm_state > self.PWM_STATE_CRITICAL):
            self.pwm_state = self.PWM_STATE_NORMAL

        # check system temperature
        temperature = self._get_system_temperature()
        if temperature > self._get_system_high_threshold():
            self.warning = True
        else:
            self.warning = False

        if self.pwm_state == self.PWM_STATE_CRITICAL:
            if temperature < self._get_system_low_threshold():
                self.pwm_state = self.PWM_STATE_NORMAL
        else:
            if temperature > self._get_system_high_threshold():
                self.pwm_state = self.PWM_STATE_CRITICAL
                logging.warning("system temperature(%d) reach high threshold(%d)",
                                 temperature, self._get_system_high_threshold())

        # check temperature of each thermal
        for thermal in self.chassis.get_all_thermals():
            temperature = thermal.get_temperature()

            if temperature >= thermal.get_high_critical_threshold():
                self.warning = True
                self.shutdown = True
                logging.critical("thermal(%s) temperature(%d) reach shutdown threshold(%d)",
                                  thermal.get_name(), temperature,
                                  thermal.get_high_critical_threshold())
            elif temperature >= thermal.get_high_threshold():
                self.warning = True
                logging.warning("thermal(%s) temperature(%d) reach high threshold(%d)",
                                thermal.get_name(), temperature,
                                thermal.get_high_threshold())

        if self.warning is True or self.shutdown is True:
            self.pwm_state = self.PWM_STATE_CRITICAL

        return True

    def _refresh_fan_status(self):
        self.fan_failed = False
        for drawer_index, drawer in enumerate(self.chassis.get_all_fan_drawers()):
            for fan_index, fan in enumerate(drawer.get_all_fans()):
                if fan.get_presence() is not True:
                    self.fan_failed = True
                    logging.error("drawer(%d)-fan(%d) is not present",
                                   drawer_index, fan_index)
                    break

                if fan.get_status() is not True:
                    self.fan_failed = True
                    logging.error("drawer(%d)-fan(%d) is not operational ",
                                   drawer_index, fan_index)
                    break

        return True

    def manage_fans(self):
        try:
            prev_warning = self.warning
            self._refresh_fan_status()
            self._refresh_thermal_status()
        except:
            self._set_all_fan_speed(FAN_SPEED_MAX)
            logging.error("Error occurred while updating fan and thermal status")
            return

        if self.fan_failed is True:
            self._set_all_fan_speed(FAN_SPEED_MAX)
        else:
            self._set_all_fan_speed(self._get_fan_speed())

        if prev_warning != self.warning:
            if self.warning is True:
                logging.warning('Alarm for temperature high is detected')
            else:
                logging.info('Alarm for temperature high is cleared')

        if self.shutdown is True:
            # critical case*/
            logging.critical(
                'Alarm-Critical for temperature critical is detected, disable PoE')
            path = I2C_PATH.format('0', '64') + 'pwr_enable_poe'
            self._api_helper.write_txt_file(path, 0)

            logging.critical(
                'Alarm-Critical for temperature critical is detected, trigger thermal shutdown')
            path = I2C_PATH.format('0', '64') + 'thermal_shutdown'
            time.sleep(2)
            self._api_helper.write_txt_file(path, 1)

            logging.critical(
                'Alarm-Critical for temperature critical is detected, shutdown DUT')
            path = I2C_PATH.format('0', '64') + 'pwr_enable_mb'
            time.sleep(2)
            self._api_helper.write_txt_file(path, 0)

        return

def main(argv):
    global platform_chassis

    # configure logging
    try:
        log_file = '%s.log' % FUNCTION_NAME
        log_level = logging.INFO
        configure_logging(log_file, log_level)
    except:
        print("Failed to configure logging")
        sys.exit(ERROR_CONFIG_LOGGING)

    # Load platform-specific Chassis class
    if not load_platform_chassis():
        sys.exit(ERROR_CHASSIS_LOAD)

    try:
        monitor = device_monitor(platform_chassis)
    except:
        logging.error("Failed to instantiate device_monitor")
        sys.exit(ERROR_DEVICE_MONITOR_LOAD)

    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)  # 10sec

if __name__ == '__main__':
    main(sys.argv[1:])
