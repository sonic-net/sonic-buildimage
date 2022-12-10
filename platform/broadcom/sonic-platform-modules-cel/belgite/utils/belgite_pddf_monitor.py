#!/usr/bin/env python3
#
# Copyright (C) Celestica Technology Corporation
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# ------------------------------------------------------------------
# HISTORY:
#    9/16/2021 (A.D.)
# ------------------------------------------------------------------

try:
    import sys
    import getopt
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import signal
    import math
    from sonic_platform import platform
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
FUNCTION_NAME = 'cel_belgite_monitor'
DUTY_MAX = 100
FAN_NUMBER = 3
SENSOR_NUMBER = 4
CPU_CORE_TEMP = r"/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp1_input"


class cel_belgite_monitor(object):
    """
    Make a class we can use to capture stdout and sterr in the log
    """
    # static temp var
    _ori_temp = 0
    _new_perc = DUTY_MAX / 2
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")
    init_fan_temperature = [0, 0, 0, 0]

    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""
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
            filemode='w',
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

    def get_all_temperature(self):
        """
        return: all temperature
        """
        all_temperature_list = list()
        for sensor_index in range(SENSOR_NUMBER):
            temp = self.platform_chassis_obj.get_thermal(sensor_index).get_temperature()
            if temp is None or str(temp).strip() == "":
                return False
            temp = temp*1000
            all_temperature_list.append(temp)
        u4_temperature = all_temperature_list[0]
        u7_temperature = all_temperature_list[1]
        # default CPU temperature 70
        cpu_temperature = 70000
        try:
            with open(CPU_CORE_TEMP, "r") as f:
                cpu_temperature = float(f.read().strip())
        except Exception as E:
            logging.debug('Error: %s' % E)
        u60_temperature = all_temperature_list[3]   
        return [u4_temperature, u7_temperature, cpu_temperature, u60_temperature]

    def get_fan_speed_by_temperature(self, temp_list):
        fan1_direction = self.platform_chassis_obj.get_fan(0).get_direction()
        logging.debug('INFO: fan direction: %s' % str(fan1_direction))
        all_temp = self.get_all_temperature()
        logging.debug('INFO: all_temp: %s' % str(all_temp))
        # B2F=intake: U7 temperature， F2B-EXHAUST: U4 temperature
        a = 1 if fan1_direction.lower() == "intake" else 0
        sensor_temp = all_temp[a]
        cup_temp = all_temp[2]
        u60_temp = all_temp[3]
        logging.debug('sensor_temp:%d cup_temp:%d u60_temp:%d' % (sensor_temp, cup_temp, u60_temp))
        update_temp_sensor, update_temp_cpu, update_temp_u60 = True, True, True
        if all_temp[a] - temp_list[a] < 0:
            update_temp_sensor = False
        if cup_temp - temp_list[2] < 0:
            update_temp_cpu = False
        if u60_temp - temp_list[3] < 0:
            update_temp_u60 = False

        # U4 U7
        if not update_temp_sensor:  # temperature down
            b = math.trunc(1400/13)
            if sensor_temp <= 32000:
                sensor_temp_speed = 40
            elif sensor_temp >= 45000:
                sensor_temp_speed = 100
            else:
                sensor_temp_speed = int(math.trunc(60 / 13) * math.trunc(sensor_temp / 1000) - b)
        else:   # temperature up
            b = math.trunc(1580 / 13)
            if sensor_temp <= 35000:
                sensor_temp_speed = 40
            elif sensor_temp >= 48000:
                sensor_temp_speed = 100
            else:
                sensor_temp_speed = int(math.trunc(60/13) * math.trunc(sensor_temp/1000) - b)

        # CPU
        if not update_temp_cpu:  # temperature down
            b = 228
            if cup_temp <= 67000:
                cpu_temp_speed = 40
            elif cup_temp >= 82000:
                cpu_temp_speed = 100
            else:
                cpu_temp_speed = int(4 * (cup_temp / 1000) - b)
        else:   # temperature up
            b = 240
            if cup_temp <= 70000:
                cpu_temp_speed = 40
            elif cup_temp >= 85000:
                cpu_temp_speed = 100
            else:
                cpu_temp_speed = int(4 * (cup_temp / 1000) - b)

        # U60
        if not update_temp_u60:  # temperature down
            b = 168
            if u60_temp <= 52000:
                u60_temp_speed = 40
            elif u60_temp >= 67000:
                u60_temp_speed = 100
            else:
                u60_temp_speed = int(4 * (u60_temp / 1000) - b)
        else:   # temperature up
            b = 180
            if u60_temp <= 55000:
                u60_temp_speed = 40
            elif u60_temp >= 70000:
                u60_temp_speed = 100
            else:
                u60_temp_speed = int(4 * (u60_temp / 1000) - b)
        return max([sensor_temp_speed, cpu_temp_speed, u60_temp_speed])

    def manage_fans(self):
        fan_presence_list = [True, True, True]  # whether fan is absent or not 
        for fan_index in range(FAN_NUMBER):
            if not self.platform_chassis_obj.get_fan(fan_index).get_presence() or not \
                    self.platform_chassis_obj.get_fan(fan_index).get_status():
                fan_presence_list[fan_index] = False
                logging.debug('self.platform_chassis_obj.get_fan(fan_index).get_presence():%s'
                              % str(self.platform_chassis_obj.get_fan(fan_index).get_presence()))
                logging.debug('self.platform_chassis_obj.get_fan(fan_index).get_status():%s'
                              % str(self.platform_chassis_obj.get_fan(fan_index).get_status()))
            else:
                fan_presence_list[fan_index] = True

        fans_inserted_num = FAN_NUMBER - fan_presence_list.count(False)
        if fans_inserted_num == 0:  # all fans broken, power off 
            self.syslog.critical("No fans inserted. Severe overheating hazard. "
                                 "Please insert Fans immediately or power off the device\n")

            # power off 
        elif fans_inserted_num in [1, 2]:   # 1 or 2 present, full speed 
            self._new_perc = DUTY_MAX
        else:   # 3 fans normal, manage the fans follow thermal policy 
            self._new_perc = self.get_fan_speed_by_temperature(self.init_fan_temperature)
            logging.debug('INFO: 3 fans inserted: self._new_perc: %s' % str(self._new_perc))
            self.init_fan_temperature = self.get_all_temperature()

        for i in range(FAN_NUMBER):
            aa = self.platform_chassis_obj.get_fan(i).get_speed()
            logging.debug("INFO: Get before setting fan speed: %s" % aa)
            if self._new_perc < 40:
                self._new_perc = 40
            if self._new_perc > 100:
                self._new_perc = 100
            set_stat = self.platform_chassis_obj.get_fan(i).set_speed(self._new_perc)
            if set_stat is True:
                logging.debug('INFO: PASS. set_fan%d_duty_cycle (%d)' % (i, self._new_perc))
            else:
                logging.debug('INFO: FAIL. set_fan%d_duty_cycle (%d)' % (i, self._new_perc))


def handler(signum, frame):
    platform_chassis = platform.Platform().get_chassis()
    for _ in range(FAN_NUMBER):
        set_stat = platform_chassis.get_fan(_).set_speed(DUTY_MAX)
        if set_stat is True:
            logging.debug('INFO:Cause signal %d, set fan speed max.' % signum)
        else:
            logging.debug('INFO: FAIL. set_fan_duty_cycle (%d)' % DUTY_MAX)
        # Enable the CPLD Heartbeat back
        status, output = getstatusoutput_noshell(["i2cset", "-f", "-y", "75", "0x40", "0x22", "0x00"])
        if status == 0:
            logging.debug('INFO: CPLD Heartbeat check is enabled back')
    sys.exit(0)


def main(argv):
    global test_temp

    log_file = '/home/admin/%s.log' % FUNCTION_NAME
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

        if sys.argv[1] == '-t':
            if len(sys.argv) != 6:
                print("temp test, need input 4 temp")
                return 0

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
    # Disaable the CPLD Heartbeat check to control Fan speed from CPU via ADT7470
    getstatusoutput_noshell(['i2cset', '-f', '-y', '2', '0x32', '0x30', '0x01'])

    monitor = cel_belgite_monitor(log_file, log_level)

    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)


if __name__ == '__main__':
    main(sys.argv[1:])
