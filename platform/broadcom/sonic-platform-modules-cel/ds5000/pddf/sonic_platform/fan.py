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


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    STATUS_LED_COLOR_AUTO = "auto"

    FANTRAY_PWM_CTRL_REG_MAP = {
        1: 0xA140,
        2: 0xA144,
        3: 0xA148
    }

    FANTRAY_LED_CTRL_REG_MAP = {
        1: 0xA137,
        2: 0xA138,
        3: 0xA139
    }

    FANTRAY_STATUS_REG_MAP = {
        1: 0xA141,
        2: 0xA145,
        3: 0xA149
    }

    FAN_RPM_STATUS_REG_MAP = {
        1: 0xA142,
        2: 0xA143,
        3: 0xA146,
        4: 0xA147,
        5: 0xA14A,
        6: 0xA14B
    }

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self._api_helper = APIHelper()
        self.target_speed = 0

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_name(self):
        """
        Retrieves the fan name
        Returns: String containing fan-name
        """
        if self.is_psu_fan:
            if 'fan_name' in self.plugin_data['PSU']:
                return self.plugin_data['PSU']['fan_name'][str(self.fans_psu_index)][str(self.fan_index)]

            return "PSU{}_FAN{}".format(self.fans_psu_index, self.fan_index)
        else:
            if 'name' in self.plugin_data['FAN']:
                return self.plugin_data['FAN']['name'][str(self.fantray_index)][str(self.fan_index)]

            return "Fantray{}_{}".format(self.fantray_index, self.fan_index)

    def get_psu_presence(self):
        from sonic_platform.psu import Psu
        psu = Psu(self.fans_psu_index - 1, self.pddf_obj, self.plugin_data)
        return psu.get_presence()

    def get_psu_powergood_status(self):
        from sonic_platform.psu import Psu
        psu = Psu(self.fans_psu_index - 1, self.pddf_obj, self.plugin_data)
        return psu.get_powergood_status()

    def get_presence(self):
        if self.is_psu_fan:
            return self.get_psu_presence()
        else:
            reg = self.FANTRAY_STATUS_REG_MAP.get(self.fantray_index)
            status, result = self._api_helper.cpld_lpc_read(reg)
            if (int(result, 16) & 0x4 == 0x0) and status == True:
                return True
            else:
                return False

    def get_status(self):

        if self.is_psu_fan:
            return self.get_psu_powergood_status()
        else:
            reg = self.FANTRAY_STATUS_REG_MAP.get(self.fantray_index)
            status, result = self._api_helper.cpld_lpc_read(reg)
            if (int(result, 16) & 0x8 == 0x0) and status == True:
                return True
            else:
                return False

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        return self.FAN_DIRECTION_EXHAUST

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if self.is_psu_fan:
            return super().get_speed()
        else:
            if self.get_presence() is False:
                return 0

            reg = self.FANTRAY_PWM_CTRL_REG_MAP.get(self.fantray_index)
            status, fpwm = self._api_helper.cpld_lpc_read(reg)
            pwm_to_dc = eval(self.plugin_data['FAN']['pwm_to_duty_cycle'])
            speed_percentage = int(round(pwm_to_dc(int(fpwm, 16))))

            return speed_percentage
            
    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in RPM

        Returns:
            An integer, Speed of fan in RPM
        """
        if self.is_psu_fan:
            return super().get_speed_rpm()
        else:
            if self.get_presence() is False:
                return 0
                
            idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
            reg = self.FAN_RPM_STATUS_REG_MAP.get(idx)
            status, result = self._api_helper.cpld_lpc_read(reg)
            if self.fan_index == 1:
                rpm_speed = 78 * int(result, 16)
            else:
                rpm_speed = 69 * int(result, 16)
            return rpm_speed

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if self.is_psu_fan:
            return self.get_speed()
        else:
            if self.get_presence() and self.get_status():
                if self.target_speed == 0:
                    reg = self.FANTRAY_PWM_CTRL_REG_MAP.get(self.fantray_index)
                    status, fpwm = self._api_helper.cpld_lpc_read(reg)
                    pwm_to_dc = eval(self.plugin_data['FAN']['pwm_to_duty_cycle'])
                    speed_percentage = int(round(pwm_to_dc(int(fpwm, 16))))
                    return speed_percentage
                else:
                    return self.target_speed
            else:
                return 100

    def set_hw_speed(self, speed):
        """
        Sets the fan speed
        
        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)

        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        if self.is_psu_fan:
            return False

        if speed <= 0 or speed > 100:
            return False

        if 'duty_cycle_to_pwm' not in self.plugin_data['FAN']:
            return False

        duty_cycle_to_pwm = eval(self.plugin_data['FAN']['duty_cycle_to_pwm'])
        pwm = int(round(duty_cycle_to_pwm(speed)))

        # FAN 1 & 2 in same fantray share the same register, skip Fan2 setting
        if self.fan_index == 2:
            return True
        # Set FAN PWM through baseboard CPLD
        reg = self.FANTRAY_PWM_CTRL_REG_MAP.get(self.fantray_index)
        status = self._api_helper.cpld_lpc_write(reg, pwm)
        if status == True:
            self.target_speed = pwm

        return status

    def set_speed(self, speed):
        if self.is_psu_fan:
            return False
    
        if speed > 0:
            if self._api_helper.fsc_enabled():
                self._api_helper.fsc_enable(False)
                if self._api_helper.fsc_enabled():
                    return False
            return self.set_hw_speed(speed)
        elif speed == 0:
            if self._api_helper.fsc_enabled() == False:
                self._api_helper.fsc_enable(True)
                if self._api_helper.fsc_enabled() == False:
                    return False
            self.target_speed = 0
            return True
        else:
            return False

    def set_status_led(self, color):
        color_dict = {
            self.STATUS_LED_COLOR_GREEN: 0x5,
            self.STATUS_LED_COLOR_AMBER: 0x6,
            self.STATUS_LED_COLOR_OFF: 0x4,
            self.STATUS_LED_COLOR_AUTO: 0x0
        }

        if color_dict.get(color, 0xff) == 0xff or self.is_psu_fan:
            return False

        reg = self.FANTRAY_LED_CTRL_REG_MAP.get(self.fantray_index)
        return self._api_helper.cpld_lpc_write(reg, color_dict.get(color))

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above

        Note:
            STATUS_LED_COLOR_GREEN = "green"
            STATUS_LED_COLOR_AMBER = "amber"
            STATUS_LED_COLOR_RED = "red"
            STATUS_LED_COLOR_OFF = "off"
        """
        if self.is_psu_fan:
            return "N/A"

        reg = self.FANTRAY_STATUS_REG_MAP.get(self.fantray_index)
        status, result = self._api_helper.cpld_lpc_read(reg)
        if status == True:
            result = int(result, 16) & 0x3
        else:
            result = 0

        status_led = {
            0: self.STATUS_LED_COLOR_OFF,
            1: self.STATUS_LED_COLOR_GREEN,
            2: self.STATUS_LED_COLOR_AMBER,
        }.get(result, self.STATUS_LED_COLOR_OFF)

        return status_led
