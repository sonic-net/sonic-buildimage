### On ln3612y2zt the LEDs are connected to the Virgo MB CPLD (bus 18, addr
### 0x75). The CPLD is managed by the virgo2_cpld kernel driver, which exposes
### each LED as a flat sysfs attribute holding a 3-bit value (0..6).

import os

# Base sysfs directory exported by the virgo2_cpld driver for the MB CPLD.
CPLD_SYSFS_DIR = "/sys/bus/i2c/devices/18-0075"


class CpldLed(object):
    """
    Controls a single CPLD-attached LED via its driver sysfs attribute.

    attr:   the CPLD LED sysfs attribute name (e.g. "led_system").
    states: mapping of a caller-defined state name to the CPLD LED field
            value (an integer in the range 0..6 accepted by the driver).
    """

    def __init__(self, attr, states):
        self.path = os.path.join(CPLD_SYSFS_DIR, attr)
        self.states = states
        self.reversed_states = {v: k for k, v in states.items()}

    def set_state(self, state):
        try:
            value = self.states[state]
        except KeyError:
            raise ValueError("Invalid state {}".format(state))

        with open(self.path, "w") as fd:
            fd.write(str(value))

    def get_state(self):
        try:
            with open(self.path, "r") as fd:
                value = int(fd.read().strip())
        except (OSError, ValueError):
            return "INVALID"

        return self.reversed_states.get(value, "INVALID")
