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

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based 
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self._api_helper = APIHelper()
        self.target_speed = 0

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_psu(self):
        from sonic_platform.psu import Psu
        psu = Psu(self.fans_psu_index - 1, self.pddf_obj, self.plugin_data)
        return psu

    def get_presence(self):
        if self.is_psu_fan:
            return self.get_psu().get_presence()
        else:
            return super().get_presence()

    def get_status(self):
        if self.is_psu_fan:
            return self.get_psu().get_powergood_status()
        else:
            if self.get_status_led() == self.STATUS_LED_COLOR_GREEN:
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
        dir = super().get_direction()
        if "Front" in self.get_name() and dir == "F2B":
            return self.FAN_DIRECTION_INTAKE
        if "Rear" in self.get_name() and dir == "B2F":
            return self.FAN_DIRECTION_INTAKE
        return self.FAN_DIRECTION_EXHAUST

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
            return super().get_target_speed()

    def set_speed(self, speed):
        if self.is_psu_fan:
            return False
    
        if speed > 0:
            if self._api_helper.fsc_enabled():
                self._api_helper.fsc_enable(False)
                if self._api_helper.fsc_enabled():
                    return False
            #return self.set_hw_speed(speed)
            return super().set_speed(speed)
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
        # led status is controlled by hw, indicates the status,we don't allow to set
        return False

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
        
        if self.get_presence():
            result = super().get_status_led()
        else:
            result = self.STATUS_LED_COLOR_OFF

        return result

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        if self.is_psu_fan:
            return self.get_psu().get_model()
        else:
            idx = self.fantray_index
            dev_name = "FANTRAY-DEVICE" + str(idx)
            output = self.pddf_obj.get_attr_name_output(dev_name, "fantray_model")

            try:
                model = output['status']
            except Exception:
                return 'N/A'

            # strip_non_ascii
            stripped = (c for c in model if 0 < ord(c) < 127)
            model = ''.join(stripped)

            return model.rstrip('\n')

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        if self.is_psu_fan:
            return self.get_psu().get_serial()
        else:
            idx = self.fantray_index
            dev_name = "FANTRAY-DEVICE" + str(idx)
            output = self.pddf_obj.get_attr_name_output(dev_name, "fantray_serial")

            try:
                model = output['status']
            except Exception:
                return 'N/A'

            # strip_non_ascii
            stripped = (c for c in model if 0 < ord(c) < 127)
            model = ''.join(stripped)

            return model.rstrip('\n')
    
    def get_mfr_id(self):
        """
        Retrieves the manufacturer ID of the device

        Returns:
            string: Manufacturer ID of device
        """
        if self.is_psu_fan:
            return self.get_psu().get_mfr_id()
        else:
            idx = self.fantray_index
            dev_name = "FANTRAY-DEVICE" + str(idx)
            output = self.pddf_obj.get_attr_name_output(dev_name, "fantray_mfr_id")

            try:
                model = output['status']
            except Exception:
                return 'N/A'

            # strip_non_ascii
            stripped = (c for c in model if 0 < ord(c) < 127)
            model = ''.join(stripped)

            return model.rstrip('\n')