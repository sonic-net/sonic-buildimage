#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import sys
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from .event import SfpEvent
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_COMPONENT = 6
HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/"
PMON_REBOOT_CAUSE_PATH = "/usr/share/sonic/platform/api_files/reboot-cause/"
REBOOT_CAUSE_FILE = "reboot-cause.txt"

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    SYSLED_DEV_NAME = "DIAG_LED"

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        self.__initialize_components()
        self._api_helper = APIHelper()
        self._sfpevent = SfpEvent(self.get_all_sfps())

    def __initialize_components(self):
        from sonic_platform.component import Component
        for index in range(NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_change_event(self, timeout=0):
        return self._sfpevent.get_sfp_event(timeout)

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot

        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """

        reboot_cause_path = (HOST_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE)
        sw_reboot_cause = self._api_helper.read_txt_file(
            reboot_cause_path) or "Unknown"


        return ('REBOOT_CAUSE_NON_HARDWARE', sw_reboot_cause)


    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>

        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 1.
            For example, 1 for Ethernet0, 2 for Ethernet4 and so on.

        Returns:
            An object derived from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            # The index will start from 1
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def initizalize_system_led(self):
        return

    def get_status_led(self):
        return self.get_system_led(self.SYSLED_DEV_NAME)

    def set_status_led(self, color):
        return self.set_system_led(self.SYSLED_DEV_NAME, color)

    def get_port_or_cage_type(self, port):
        from sonic_platform_base.sfp_base import SfpBase
        if port in range(1, 33):
            return SfpBase.SFP_PORT_TYPE_BIT_QSFP | SfpBase.SFP_PORT_TYPE_BIT_QSFP_PLUS | SfpBase.SFP_PORT_TYPE_BIT_QSFP28 | SfpBase.SFP_PORT_TYPE_BIT_QSFP56 | SfpBase.SFP_PORT_TYPE_BIT_QSFPDD
        else:
            return SfpBase.SFP_PORT_TYPE_BIT_SFP

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self._eeprom.revision_str()
