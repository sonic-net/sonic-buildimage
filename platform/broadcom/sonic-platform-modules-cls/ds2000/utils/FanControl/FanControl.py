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
FAN_NUMBER = 4
PSU_NUMBER = 2
SENSOR_NUMBER = 6
Fan_Front_MAX = 29125
Fan_Front_MIN = 1370
Fan_Rear_MAX = 25375
Fan_Rear_MIN = 1370
FAN_STATUS_LIST = [True, True, True, True]


class FanControl(object):
    """
        Make a class we can use to capture stdout and sterr in the log
        """
    # static temp var
    _ori_temp = 0
    _new_perc = DUTY_MAX / 2
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")
    init_fan_temperature = [0, 0]

    def __init__(self, log_file, log_level):
        self.FanLinearAdjustment = FanLinearAdjustment.FanLinearAdjustment(log_file, log_level, DUTY_MAX, FAN_NUMBER,
                                                                           PSU_NUMBER, SENSOR_NUMBER)
        self.SwitchInternalPIDRegulation = SwitchInternalPIDRegulation.SwitchInternalPIDRegulation(log_file, log_level)
        self.CPUPIDRegulation = CPUPIDRegulation.CPUPIDRegulation(log_file, log_level)
        # Needs a logger and a logger level
        formatter = logging.Formatter('%(name)s %(message)s')
        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setFormatter(formatter)
        sys_handler.ident = 'common'
        self.syslog.setLevel(logging.WARNING)
        self.syslog.addHandler(sys_handler)
        self.platform_chassis_obj = platform.Platform().get_chassis()
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='a',
            level=log_level,
            format='[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )

        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)
        logging.debug('SET. logfile:%s / loglevel:%d' % (log_file, log_level))

    def get_psu_status(self, fan_duty_list):
        """
        Get PSU Status.If one PSU not OK, all of the fans pwm will increase to 100
        :param fan_duty_list: A list.TO app the fans target pwm
        """
        psu_status_list = [True, True]
        for psu_index in range(PSU_NUMBER):
            psu_presence = self.platform_chassis_obj.get_psu(psu_index).get_presence()
            psu_status = self.platform_chassis_obj.get_psu(psu_index).get_status()
            if not psu_presence:
                psu_status_list[psu_index] = False
                logging.critical(
                    "psu%s was error,presence:%s, status:%s" % (psu_index + 1, str(psu_presence), str(psu_status)))

        if False in psu_status_list:
            fan_duty_list.append(DUTY_MAX)

    def get_fan_presence(self):
        """
        Get all of fans status(fan drawer)
        :return: A list indicating the status of all groups fans
        """
        fan_presence_list = [True, True, True, True]  # Default state: fans are OK
        for fan_drawer_index in range(FAN_NUMBER):
            fan_drawer = self.platform_chassis_obj.get_fan_drawer(fan_drawer_index)
            fan0_presence = fan_drawer.get_fan(0).get_presence()
            if not fan0_presence:
                logging.warning("%s not presence"% (fan_drawer.get_fan(0).get_name()))
            fan1_presence = fan_drawer.get_fan(1).get_presence()
            if not fan1_presence:
                logging.warning("%s not presence"% (fan_drawer.get_fan(1).get_name()))
            if not fan0_presence and not fan1_presence:
                fan_presence_list[fan_drawer_index] = False
                logging.critical("Fan Drawer-%s not presence"% (str(fan_drawer_index + 1)))
        return fan_presence_list

    def check_fans_presence(self, fan_duty_list):
        """
        check all fans presence or not
        """
        fans_presence_list = self.get_fan_presence()
        fans_not_presence_num = fans_presence_list.count(False)
        if fans_not_presence_num != 0:  # all fans broken, power off
            logging.critical("Some fan not presence, change others fan speed to max!")
            fan_duty_list.append(DUTY_MAX)

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
            if fan_name.endswith("1") and (fan_speed_rpm not in range(Fan_Front_MIN, Fan_Front_MAX + 1)):
                fan_rpm_error_list.append(fan_name)
            if fan_name.endswith("2") and (fan_speed_rpm not in range(Fan_Rear_MIN, Fan_Rear_MAX + 1)):
                fan_rpm_error_list.append(fan_name)
        if not fan_rpm_error_list:
            for fan_drawer_index in range(FAN_NUMBER):
                if FAN_STATUS_LIST[fan_drawer_index] == False:
                    FAN_STATUS_LIST[fan_drawer_index] = True
                    self.platform_chassis_obj.get_fan_drawer(fan_drawer_index).set_status_led(NORMAL_COLOR)
            return None
        if len(fan_rpm_error_list) >= 2:
            logging.critical("%s rpm less than the set minimum speed. "
                            "Will increase the fan speed to 100%%" % fan_rpm_error_list)
            fan_duty_list.append(DUTY_MAX)
        else:
            logging.warning("%s rpm less than the set minimum speed. Fans pwm isn't changed" % fan_rpm_error_list)

        fan_modules_index_list = list(set(int(re.findall(r"Fantray(\d)_\d", x)[0]) for x in fan_rpm_error_list))
        for error_fan_drawer in fan_modules_index_list:
            if FAN_STATUS_LIST[error_fan_drawer-1] == True:
                FAN_STATUS_LIST[error_fan_drawer-1] = False
                logging.warning("Fantray%d will be set to %s " % (error_fan_drawer, ERROR_COLOR))
                self.platform_chassis_obj.get_fan_drawer(error_fan_drawer-1).set_status_led(ERROR_COLOR)

        logging.warning("The STA front panel light will be set to %s " % ERROR_COLOR)
        self.platform_chassis_obj.set_status_led(ERROR_COLOR)

    def get_linear_pid_pwm(self, fan_duty_list):
        """
        Get the pwm value of liner regulation, cpu pid adjustment, switch internal pid adjustment
        :param fan_duty_list: A list.TO app the fans target pwm
        """
        linear_regulation = self.FanLinearAdjustment.linear_control()
        cpu_pid_adjustment = self.CPUPIDRegulation.pid_control()
        sw_pid_adjustment = self.SwitchInternalPIDRegulation.pid_control()
        #logging.info("linear regulation PWM:%d, cpu pid PWM:%d, sw pid PWM:%d" % (linear_regulation, cpu_pid_adjustment, sw_pid_adjustment))
        fan_duty_list.append(linear_regulation)
        fan_duty_list.append(cpu_pid_adjustment)
        fan_duty_list.append(sw_pid_adjustment)

    def manage_fans(self):
        """
        Set the fan speed according to the Fan Control Strategy
        """
        fan_duty_speed_list = list()

        # Fan speed setting judgment-PSU
        self.get_psu_status(fan_duty_speed_list)

        # Fan speed setting judgment-FAN presence
        self.check_fans_presence(fan_duty_speed_list)

        # Fan speed setting judgment-FAN SPEED
        self.set_fans_pwm_by_rpm(fan_duty_speed_list)

        # Fan speed setting judgment-linear and cpu pid and sw pid
        self.get_linear_pid_pwm(fan_duty_speed_list)

        self._new_perc = max(fan_duty_speed_list)
        if self._new_perc < 35:
            self._new_perc = 35
        if self._new_perc > 100:
            self._new_perc = 100
        fan_index = 0
        for fan in self.platform_chassis_obj.get_all_fans():
            fan_index += 1
            fan_rpm = fan.get_speed()
            #logging.info("Get before setting fan speed: %s" % fan_rpm)
            set_stat = fan.set_speed(self._new_perc)
            if set_stat is False:
            #logging.info('PASS. Set Fan%d duty_cycle (%d)' % (fan_index, self._new_perc))
            #else:
                logging.error('FAIL. Set Fan%d duty_cycle (%d)' % (fan_index, self._new_perc))


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


def main(argv):
    log_file = '/var/log/%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv, 'hdlt:', ['lfile='])
        except getopt.GetoptError:
            print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
                return 0
            elif opt in ('-d', '--debug'):
                log_level = logging.DEBUG
            elif opt in ('-l', '--lfile'):
                log_file = arg

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
    monitor = FanControl(log_file, log_level)
    # Loop forever, doing something useful hopefully:
    time.sleep(30)
    while True:
        monitor.manage_fans()
        time.sleep(4)
