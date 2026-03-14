#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the led status which are available in the platform
#
#############################################################################

from subprocess import Popen, PIPE, STDOUT

class SystemLed(object):
    STATUS_LED_COLOR_GREEN = 'green'
    STATUS_LED_COLOR_ORANGE = 'orange'
    STATUS_LED_COLOR_OFF = 'off'

    SYSTEM_LED_PATH = '/sys/class/leds/system/brightness'

    def set_status(self, color):
        status = False

        if color == SystemLed.STATUS_LED_COLOR_ORANGE:
            cmd = 'i2cset -y 0 0x36 0x2 0xb'
            Popen(cmd, shell=True)
            status = True
        elif color == SystemLed.STATUS_LED_COLOR_OFF:
            cmd = 'i2cset -y 0 0x36 0x2 0x0'
            Popen(cmd, shell=True)
            status = True
        elif color == SystemLed.STATUS_LED_COLOR_GREEN:
            cmd = 'i2cset -y 0 0x36 0x2 0x5'
            Popen(cmd, shell=True)
            status = True

        return status

    def get_status(self):
        cmd = 'i2cget -y 0 0x36 0x2'
        status = int(Popen(cmd, stdout=PIPE, stderr=STDOUT, shell=True).stdout.readline(), 16)
        if status == 11:
            return SystemLed.STATUS_LED_COLOR_ORANGE
        elif status == 0:
            return SystemLed.STATUS_LED_COLOR_OFF
        else:
            return SystemLed.STATUS_LED_COLOR_GREEN
