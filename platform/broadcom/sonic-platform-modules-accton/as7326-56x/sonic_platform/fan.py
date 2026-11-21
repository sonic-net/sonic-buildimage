#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten
    # Since AS4630 psu_fan airflow direction cant be read from sysfs, it is fixed as 'F2B' or 'intake'


    def _get_speed_from_pwm(self, pwm):
        """Preserve the AS7326 profile's original integer-division result.

        In particular, PWM 15 is 100%, not round(100.75) == 101%.
        The same conversion must be used for measured and target speed.
        """
        return int(self._pwm_to_duty_cycle.convert(pwm))
