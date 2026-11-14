#!/usr/bin/env python

try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

# ln3848g CPLD (0x33) register 0x01 holds the PWM duty-cycle value in the range
# 0–10 (0 = 0%, 10 = 100%).  There is no hardware tachometer register, so
# fan_input in pddf-device.json also points to reg 0x01 (attr_mult=1).
# The get_speed_rpm() helper therefore returns the raw PWM value (0-10) and
# get_speed() converts it to a percentage.
#
# TODO: Once MAX_RPM is measured on a live board, update this constant and
#       restore the RPM-based get_speed_rpm() implementation below.
MAX_RPM = 23856  # placeholder – update after measuring at PWM=10


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class for LN3848G"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    # -----------------------------------------------------------------
    # Speed
    # fan_input sysfs attribute returns the raw CPLD PWM value (0-10).
    # -----------------------------------------------------------------

    def get_speed(self):
        """
        Returns fan speed as a percentage (0-100).
        Reads the PWM register (0-10) and multiplies by 10.
        """
        pwm_raw = super(Fan, self).get_speed_rpm()  # reads fan_input -> CPLD reg 0x01 -> 0..10
        return min(int(pwm_raw) * 10, 100)

    def get_speed_rpm(self):
        """
        Returns an estimated RPM based on the PWM setting.
        Not a real tachometer reading.
        """
        pwm_raw = super(Fan, self).get_speed_rpm()  # 0..10
        return int(pwm_raw * MAX_RPM / 10)

    def get_target_speed(self):
        return self.get_speed()
