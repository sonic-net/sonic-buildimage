#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the fan management function
#
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from .helper import APIHelper       
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FAN_PRESENCE_SYSFS_PATH = "/sys/devices/platform/sys_cpld/fantray{}_presence"
FAN_SPEED_SYSFS_PATH = "/sys/devices/platform/sys_cpld/fantray{}_{}_speed"
FAN_PWM_SYSFS_PATH = "/sys/devices/platform/sys_cpld/fantray{}_pwm"
FAN_LED_SYSFS_PATH = "/sys/devices/platform/sys_cpld/fantray{}_led"

class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self._api_helper = APIHelper()

        if not is_psu_fan:
           self._max_speed = { "front": 18000, \
                               "rear": 16000 }[self.get_name()[6:].lower()]

    # Provide the functions/variables below for which implementation is to be overwritten

    # DS5000 BMC does not support OEM command and also impitool
    # sometimes returns junk values or fails to fetch. Hence the below
    # API overrides accesses from CPLD via LPC.

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        # DS5000 has only one FAN type
        return self.FAN_DIRECTION_EXHAUST

    def get_presence(self):
        presence = False

        if self.is_psu_fan:
            return super().get_presence()
        else:
            try:
                with open(FAN_PRESENCE_SYSFS_PATH.format(self.fantray_index)) as fd:
                    data = fd.read().strip()

                if data == "1":
                    presence = True
            except (FileNotFoundError, IOError):
                pass

        return presence

    def get_target_speed(self):
        target_speed = 0

        if self.is_psu_fan:
            # Target speed not usually supported for PSU fans
            raise NotImplementedError
        else:
            try:
                with open(FAN_PWM_SYSFS_PATH.format(self.fantray_index)) as fd:
                    data = fd.read().strip()

                data = int(data, 16)
                if data >= 0 and data <= 255:
                    # pwm to percentage conversion
                    target_speed = (data * 100) / 255
            except (FileNotFoundError, IOError):
                pass

        return round(target_speed)

    def get_speed(self):
        speed = 0

        if self.is_psu_fan:
            return super().get_speed()
        else:
            try:
                with open(FAN_SPEED_SYSFS_PATH.format(self.fantray_index, self.get_name()[6:].lower())) as fd:
                    data = fd.read().strip()

                data = int(data)
                speed = (data * 100) / self._max_speed
            except (FileNotFoundError, IOError):
                pass

        return round(speed)

    def get_status_led(self):
        led_color = "unknown"
        try:
            with open(FAN_LED_SYSFS_PATH.format(self.fantray_index), "r") as fd:
                led_color = fd.read().strip()
        except (FileNotFoundError, IOError):
            pass

        return led_color

    def set_status_led(self, color):
        # BMC controls the FAN LEDs
        if self._api_helper.with_bmc():
            raise NotImplementedError

        try:
            with open(FAN_LED_SYSFS_PATH.format(self.fantray_index), "w") as fd:
                fd.write(color)
        except (FileNotFoundError, IOError):
            return False

        return True
