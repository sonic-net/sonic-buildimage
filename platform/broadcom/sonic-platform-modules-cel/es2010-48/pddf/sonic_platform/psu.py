#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the psu management function
#
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    from sonic_platform.helper import APIHelper
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    PLATFORM_PSU_CAPACITY = "550"
    PLATFORM_POE_PSU_CAPACITY = "1600"

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)
        self._model_name = None
        self._api_helper = APIHelper()

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_capacity(self):
        """
        Gets the capacity (maximum output power) of the PSU in watts

        Returns:
            An integer, the capacity of PSU
        """
        if self._model_name is None:
            self._model_name = super().get_model()
        if self._model_name and self.PLATFORM_POE_PSU_CAPACITY in self._model_name:
            return (self.PLATFORM_POE_PSU_CAPACITY)
        else:
            return (self.PLATFORM_PSU_CAPACITY)

    def get_type(self):
        """
        Gets the type of the PSU

        Returns:
            A string, the type of PSU (AC/DC)
        """
        ptype = "AC"

        # This platform supports AC PSU
        return ptype

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if self.get_presence():
            if self.get_powergood_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_AMBER

        return self.STATUS_LED_COLOR_OFF

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        status, result = self._api_helper.cpld_lpc_read(0xA10D, 2)

        if not status or result is None:
            return False

        try:
            reg_val = int(result, 16)
            bit_mask = 1 << (self.psu_index - 1)
            return (reg_val & bit_mask) == 0

        except (ValueError, TypeError):
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        status, result = self._api_helper.cpld_lpc_read(0xA10D, 2)

        if not status or result is None:
            return False

        try:
            reg_val = int(result, 16)
            shift = 5 + self.psu_index
            bit_mask = 1 << shift
            return (reg_val & bit_mask) != 0

        except (ValueError, TypeError):
            return False
