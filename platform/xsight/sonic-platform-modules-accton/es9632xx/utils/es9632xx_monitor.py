#!/usr/bin/env python
#
# Copyright (C) 2017 Accton Technology Corporation
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
#    1/10/2018: Jostar modify for as7716_32
#    2/27/2018: Roy Lee modify for as7312_54x
# ------------------------------------------------------------------

try:
    import getopt
    import os
    import os.path
    import sys
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import signal
    from systemd.journal import JournalHandler
    from es9632xx.fanutil import FanUtil
    from es9632xx.thermalutil import ThermalUtil
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'es9632xx_monitor'
DUTY_MAX = 100
SHUTDOWN_FILE = '/usr/share/sonic/firmware/pmon_system_shutdown'
SYSTEM_HALT_ON_OVERHEAT = 1
SYSTEM_WARN_ON_OVERHEAT = 2

global log_file
global log_level

#   (LM75_1+ LM75_2+ LM75_3) is LM75 at i2c addresses 0x48, 0x49, and 0x4A.
#   TMP = (LM75_1+ LM75_2+ LM75_3)/3
#1. If TMP < 35, All fans run with duty 31.25%.
#2. If TMP>=35 or the temperature of any one of fan is higher than 40,
#   All fans run with duty 50%
#3. If TMP >= 40 or the temperature of any one of fan is higher than 45,
#   All fans run with duty 62.5%.
#4. If TMP >= 45 or the temperature of any one of fan is higher than 50,
#   All fans run with duty 100%.
#5. Any one of 6 fans is fault, set duty = 100%.
#6. Direction factor. If it is B2F direction, duty + 12%.

 # MISC:
 # 1.Check single LM75 before applied average.
 # 2.If no matched fan speed is found from the policy,
 #     use FAN_DUTY_CYCLE_MIN as default speed
 # Get current temperature
 # 4.Decision 3: Decide new fan speed depend on fan direction/current fan speed/temperature




# Make a class we can use to capture stdout and sterr in the log
def handler(signum, frame):
    fan = FanUtil()
    logging.debug('INFO:Cause signal %d, set fan speed max.', signum)
    fan.set_fan_duty_cycle(DUTY_MAX)
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
    #monitor = es9632xx_monitor(log_file, log_level)

    # Loop forever, doing something useful hopefully:
    while True:
        #monitor.manage_fans()
        if os.path.exists(SHUTDOWN_FILE) and os.path.isfile(SHUTDOWN_FILE):
            try:
                file = open(SHUTDOWN_FILE)
                file_content = file.readline().split('-')
                ext_string = ""
                if 1 < len(file_content):
                    ext_string = file_content[1]
                if SYSTEM_HALT_ON_OVERHEAT == int(file_content[0]):
                    os.remove(SHUTDOWN_FILE)
                    logging.warning("System HALT due to high temperature of: {} !".format(ext_string))
                    os.system("/opt/xplt/utils/es9632x_reset_x1.sh")
                    os.system("/usr/sbin/shutdown -H now")
                if SYSTEM_WARN_ON_OVERHEAT == int(file_content[0]):
                    logging.warning("System warning due to overheat of: {} !".format(ext_string))
                    os.remove(SHUTDOWN_FILE)
            except:
                time.sleep(1)
        else:
            time.sleep(2)


if __name__ == '__main__':
    main(sys.argv[1:])
