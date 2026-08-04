#!/usr/bin/env python
#
# Copyright (C) 2019 Accton Technology Corporation
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
#    mm/dd/yyyy (A.D.)
#    11/13/2017: Polly Hsu, Create
#    05/08/2019: Roy Lee, changed for as5712-54x.
# ------------------------------------------------------------------

try:
    import getopt
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import signal
    from as5712_54x.fanutil import FanUtil
    from as5712_54x.thermalutil import ThermalUtil
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'accton_as5712_monitor'
DUTY_MAX = 100

global log_file
global log_level

# Make a class we can use to capture stdout and sterr in the log
class accton_as5712_monitor(object):
    # static temp var
    _ori_temp = 0
    _new_perc = 0

    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format= '[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )

        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

    def manage_fans(self):
        FAN_LEV1_UP_TEMP = 57700  # temperature
        FAN_LEV1_DOWN_TEMP = 0    # unused
        FAN_LEV1_SPEED_PERC = DUTY_MAX # percentage*/

        FAN_LEV2_UP_TEMP = 53000
        FAN_LEV2_DOWN_TEMP = 52700
        FAN_LEV2_SPEED_PERC = 80

        FAN_LEV3_UP_TEMP = 49500
        FAN_LEV3_DOWN_TEMP = 47700
        FAN_LEV3_SPEED_PERC = 65

        FAN_LEV4_UP_TEMP = 0     # unused
        FAN_LEV4_DOWN_TEMP = 42700
        FAN_LEV4_SPEED_PERC = 40


        thermal = ThermalUtil()
        fan = FanUtil()

        temp1 = thermal.get_thermal_1_val()
        if temp1 is None:
            return False

        temp2 = thermal.get_thermal_2_val()
        if temp2 is None:
            return False

        new_temp = (temp1 + temp2) / 2

        for x in range(fan.get_idx_fan_start(), fan.get_num_fans()+1):
            fan_stat = fan.get_fan_status(x)
            if fan_stat is None:
                return False
            if fan_stat is False:
                self._new_perc = FAN_LEV1_SPEED_PERC
                logging.debug('INFO. SET new_perc to %d (FAN fault. fan_num:%d)', self._new_perc, x)
                break
            logging.debug('INFO. fan_stat is True (fan_num:%d)', x)

        if fan_stat is not None and fan_stat is not False:
            diff = new_temp - self._ori_temp
            if diff  == 0:
                logging.debug('INFO. RETURN. THERMAL temp not changed. %d / %d (new_temp / ori_temp)', new_temp, self._ori_temp)
                return True
            else:
                if diff >= 0:
                    is_up = True
                    logging.debug('INFO. THERMAL temp UP %d / %d (new_temp / ori_temp)', new_temp, self._ori_temp)
                else:
                    is_up = False
                    logging.debug('INFO. THERMAL temp DOWN %d / %d (new_temp / ori_temp)', new_temp, self._ori_temp)

            if is_up is True:
                if new_temp  >= FAN_LEV1_UP_TEMP:
                    self._new_perc = FAN_LEV1_SPEED_PERC
                elif new_temp  >= FAN_LEV2_UP_TEMP:
                    self._new_perc = FAN_LEV2_SPEED_PERC
                elif new_temp  >= FAN_LEV3_UP_TEMP:
                    self._new_perc = FAN_LEV3_SPEED_PERC
                else:
                    self._new_perc = FAN_LEV4_SPEED_PERC
                logging.debug('INFO. SET. FAN_SPEED as %d (new THERMAL temp:%d)', self._new_perc, new_temp)
            else:
                if new_temp <= FAN_LEV4_DOWN_TEMP:
                    self._new_perc = FAN_LEV4_SPEED_PERC
                elif new_temp  <= FAN_LEV3_DOWN_TEMP:
                    self._new_perc = FAN_LEV3_SPEED_PERC
                elif new_temp  <= FAN_LEV2_DOWN_TEMP:
                    self._new_perc = FAN_LEV2_SPEED_PERC
                else:
                    self._new_perc = FAN_LEV1_SPEED_PERC
                logging.debug('INFO. SET. FAN_SPEED as %d (new THERMAL temp:%d)', self._new_perc, new_temp)

        cur_perc = fan.get_fan_duty_cycle(fan.get_idx_fan_start())
        if cur_perc == self._new_perc:
            logging.debug('INFO. RETURN. FAN speed not changed. %d / %d (new_perc / ori_perc)', self._new_perc, cur_perc)
            return True

        set_stat = fan.set_fan_duty_cycle(fan.get_idx_fan_start(), self._new_perc)
        if set_stat is True:
            logging.debug('INFO: PASS. set_fan_duty_cycle (%d)', self._new_perc)
        else:
            logging.debug('INFO: FAIL. set_fan_duty_cycle (%d)', self._new_perc)

        logging.debug('INFO: GET. ori_perc is %d. ori_temp is %d', cur_perc, self._ori_temp)
        self._ori_temp = new_temp
        logging.debug('INFO: UPDATE. ori_perc to %d. ori_temp to %d', cur_perc, self._ori_temp)

        return True

def handler(signum, frame):
        fan = FanUtil()
        logging.debug('INFO:Cause signal %d, set fan speed max.', signum)
        fan.set_fan_duty_cycle(fan.get_idx_fan_start(), DUTY_MAX)
        sys.exit(0)

def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv,'hdl:',['lfile='])
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
    monitor = accton_as5712_monitor(log_file, log_level)

    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)

if __name__ == '__main__':
    main(sys.argv[1:])
