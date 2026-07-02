#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the sfp management function
#
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    from sonic_platform.helper import APIHelper
    import time
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

RJ45_PORT_INDEX_END = 24
PRS_PATH = '/sys/bus/platform/devices/sys_cpld1/qsfp{}_present'
RESET_PATH = '/sys/bus/platform/devices/sys_cpld1/qsfp{}_reset'
LP_PATH = '/sys/bus/platform/devices/sys_cpld1/qsfp{}_lpmode'
INT_STATUS_PATH = '/sys/bus/platform/devices/sys_cpld1/qsfp{}_intr_status'

class Sfp(PddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self._api_helper = APIHelper()

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_presence(self):
        """
        Retrieves the presence of the SFP
        Returns:
            bool: True if SFP is present, False if not
        """
        output = self._api_helper.read_one_line_file(PRS_PATH.format((self.port_index - RJ45_PORT_INDEX_END)))
        if not output:
            return False

        modpres = output.rstrip()

        if modpres == '0':
            return True
        else:
            return False

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_status = None
        output = self._api_helper.read_one_line_file(RESET_PATH.format((self.port_index - RJ45_PORT_INDEX_END)))

        if output:
            status = int(output.rstrip())

            if status == 0:
                reset_status = True
            else:
                reset_status = False

        return reset_status

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        lpmode = False
        output = self._api_helper.read_one_line_file(LP_PATH.format((self.port_index - RJ45_PORT_INDEX_END)))

        if output:
            status = int(output.rstrip())

            if status == 1:
                lpmode = True
            else:
                lpmode = False

        return lpmode

    def get_intr_status(self):
        """
        Retrieves the interrupt status for this transceiver
        Returns:
            A Boolean, True if there is interrupt, False if not
        """
        intr_status = False

        # Interrupt status can be checked for absent ports too
        output = self._api_helper.read_one_line_file(INT_STATUS_PATH.format((self.port_index - RJ45_PORT_INDEX_END)))

        if output:
            status = int(output.rstrip())

            if status == 0:
                intr_status = True
            else:
                intr_status = False

        return intr_status

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        status = False
        path = RESET_PATH.format((self.port_index - RJ45_PORT_INDEX_END))

        if path:
            try:
                f = open(path, 'r+')
            except IOError as e:
                return False

            try:
                f.seek(0)
                f.write('0')
                time.sleep(1)
                f.seek(0)
                f.write('1')

                f.close()
                status = True
            except IOError as e:
                status = False
        else:
            # Use common SfpOptoeBase implementation for reset
            status = super().reset()

        return status

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        status = False
        path = LP_PATH.format((self.port_index - RJ45_PORT_INDEX_END))

        if path:
            try:
                f = open(path, 'r+')
            except IOError as e:
                return False

            try:
                if lpmode:
                    f.write('1')
                else:
                    f.write('0')

                f.close()
                status = True
            except IOError as e:
                status = False

        return status

    def get_port_or_cage_type(self):
        if self.port_index >= 1 and self.port_index <= 24:
            return self.SFP_CAGE_TYPE_RJ45
        elif self.port_index >= 25 and self.port_index <= 26:
            return self.SFP_CAGE_TYPE_QSFP
        else:
            return "N/A"
