#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the led status which are available in the platform
#
#############################################################################

class SystemLed(object):
    STATUS_LED_COLOR_GREEN = 'green'
    STATUS_LED_COLOR_ORANGE = 'orange'
    STATUS_LED_COLOR_OFF = 'off'

    SYSTEM_LED_PATH = '/sys/class/leds/system/brightness'

    def set_status(self, color):
        status = False

        if color == SystemLed.STATUS_LED_COLOR_GREEN:
            with open(SystemLed.SYSTEM_LED_PATH, 'w') as led:
                led.write('1')
                status = True
        elif color == SystemLed.STATUS_LED_COLOR_OFF:
            with open(SystemLed.SYSTEM_LED_PATH, 'w') as led:
                led.write('3')
                status = True
        elif color == SystemLed.STATUS_LED_COLOR_ORANGE:
            with open(SystemLed.SYSTEM_LED_PATH, 'w') as led:
                led.write('2')
                status = True

        return status

    def get_status(self):
        with open(SystemLed.SYSTEM_LED_PATH, 'r') as led:
            if led.read().rstrip('\n') == '1':
                return SystemLed.STATUS_LED_COLOR_GREEN
            elif led.read().rstrip('\n') == '3':
                return SystemLed.STATUS_LED_COLOR_OFF
            else:
                return SystemLed.STATUS_LED_COLOR_ORANGE
