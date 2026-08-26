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

    FANTRAY_DIR_STATUS_REG_MAP = 0xA33
    FANTRAY_PWM_CTRL_REG_MAP = 0xA44
    FANTRAY_PRESENT_REG_MAP = 0xA47

    FANTRAY_LED_CTRL_REG_MAP = {
        1: 0xA45,
        2: 0xA45,
        3: 0xA45,
        4: 0xA45,
        5: 0xA46,
        6: 0xA46,
        7: 0xA46,
    }

    FAN_RPM_STATUS_REG_MAP = {
		1 :	0xA34,
		2 :	0xA3B,
		3 :	0xA35,
		4 :	0xA3C,
		5 :	0xA36,
		6 :	0xA3D,
		7 :	0xA37,
		8 :	0xA3E,
		9 :	0xA38,
		10:	0xA3F,
		11:	0xA39,
		12:	0xA40,
		13:	0xA3A,
		14:	0xA41
    }

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self._api_helper = APIHelper()
        self.target_speed = 0

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed which is
                 considered tolerable
        """
        return 15 if "PSU" in self.get_name() else 10
    
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
            reg = self.FANTRAY_PRESENT_REG_MAP
            status, result = self._api_helper.cpld_lpc_read(reg)
            mask = 1 << (self.fantray_index -1)
            if (int(result, 16) & mask) == mask  and status == True:
                return True
            else:
                return False
    

    def get_direction(self):
        if self.is_psu_fan or not self._api_helper.is_bmc_present():
            return super().get_direction()
        
        status, result = self._api_helper.cpld_lpc_read(self.FANTRAY_DIR_STATUS_REG_MAP)    
        if status:
            mask = 1 << (self.fantray_index -1)
            return "EXHAUST" if (int(result, 16) & mask) == mask else "INTAKE"

    def get_speed(self):
        """
        Obtain the fan speed ratio (rpm/max rpm) according to the fan maximum rpm in the pd-plugin.json file
        returns: if the value > 100, return 100. else return Speed/percentage of maximum speed.
        """
        fan_name = self.get_name()
        speed_rpm = self.get_speed_rpm()
        if "PSU" in fan_name:
            max_psu_fan_rpm = int(self.plugin_data['PSU']['PSU_FAN_MAX_SPEED'])
            psu_speed_percentage = int(round(speed_rpm / max_psu_fan_rpm * 100))
            return 100 if psu_speed_percentage > 100 else psu_speed_percentage

        direction = self.get_direction()
        f_r_fan = "Front" if "Front" in fan_name else "Rear"
        max_fan_rpm = int(self.plugin_data['FAN']['FAN_MAX_RPM_SPEED'][direction][f_r_fan])
        speed_percentage = int(round(speed_rpm / max_fan_rpm * 100))
        return 100 if speed_percentage > 100 else speed_percentage
            
    def get_speed_rpm(self):
        """
        Retrieves the speed of fan in RPM

        Returns:
            An integer, Speed of fan in RPM
        """
        if self.is_psu_fan or not self._api_helper.is_bmc_present():
            return super().get_speed_rpm()
        
        if not self.get_presence():
            return 0
                
        idx = (self.fantray_index-1)*self.platform['num_fans_pertray'] + self.fan_index
        reg = self.FAN_RPM_STATUS_REG_MAP.get(idx)
        status, result = self._api_helper.cpld_lpc_read(reg)
        if self.fan_index == 1:
            rpm_speed = 156 * int(result, 16)
        else:
            rpm_speed = 142 * int(result, 16)
        return rpm_speed

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        if self.is_psu_fan or not self._api_helper.is_bmc_present():
            return super().get_target_speed()
        else:
            if self.get_presence():
                if self.target_speed == 0:
                    reg = self.FANTRAY_PWM_CTRL_REG_MAP
                    status, fpwm = self._api_helper.cpld_lpc_read(reg)
                    pwm_to_dc = eval(self.plugin_data['FAN']['pwm_to_duty_cycle'])
                    speed_percentage = int(round(pwm_to_dc(int(fpwm, 16))))
                    return speed_percentage
                else:
                    return self.target_speed
            else:
                return 100

    def get_status_led(self):
        """
        Gets the state of the fan status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above

        Note:
            STATUS_LED_COLOR_GREEN = "green"
            STATUS_LED_COLOR_AMBER = "amber"
            STATUS_LED_COLOR_OFF = "off"
        """
        if self.is_psu_fan:
            return "N/A"
        if not self._api_helper.is_bmc_present():
            return super().get_status_led()
        
        reg = self.FANTRAY_LED_CTRL_REG_MAP.get(self.fantray_index)
        status, result = self._api_helper.cpld_lpc_read(reg)
        offset = (self.fantray_index - 1) % 4
        mask = 3 << offset
        if status == True:
            result = (int(result, 16) & mask) >> offset
        else:
            result = 0

        status_led = {
            0: self.STATUS_LED_COLOR_OFF,
            1: self.STATUS_LED_COLOR_GREEN,
            2: self.STATUS_LED_COLOR_AMBER,
        }.get(result, self.STATUS_LED_COLOR_OFF)

        return status_led
