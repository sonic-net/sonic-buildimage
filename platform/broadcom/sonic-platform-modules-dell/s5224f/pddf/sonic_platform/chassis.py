#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import sys
    import time
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from sonic_platform.component import Component
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"
MAX_S5224F_COMPONENT = 5

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    _global_port_pres_dict = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        
        self.PORT_START = 1
        self.PORT_END = (self.platform_inventory['num_ports'] + 1)

        for index in range(self.PORT_START, self.PORT_END):
            present = self.get_sfp(index).get_presence()
            self._global_port_pres_dict[index] = '1' if present else '0'

        for i in range(MAX_S5224F_COMPONENT):
            component = Component(i)
            self._component_list.append(component)
    
    # Provide the functions/variables below for which implementation is to be overwritten
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

    # check for this event change for sfp / do we need to handle timeout/sleep
    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level
        """
        port_dict = {}

        SFP_REMOVED = '0'
        SFP_INSERTED = '1'

        start_time = time.time()
        time_period = timeout/float(1000)

        while time.time() < (start_time + time_period) or timeout == 0:
            for port_num in range(self.PORT_START, self.PORT_END):
                presence = self.get_sfp(port_num).get_presence()
                if presence and self._global_port_pres_dict[port_num] == '0':
                    self._global_port_pres_dict[port_num] = SFP_INSERTED
                    port_dict[port_num] = SFP_INSERTED
                elif not presence and self._global_port_pres_dict[port_num] == '1':
                    self._global_port_pres_dict[port_num] = SFP_REMOVED
                    port_dict[port_num] = SFP_REMOVED

            if port_dict:
                return True, {'sfp':port_dict}

            time.sleep(0.5)

        return True, {'sfp':{}} # Timeout

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
        try:
            with open(REBOOT_CAUSE_PATH) as fd:
                reboot_cause = int(fd.read(), 16)
        except EnvironmentError:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

        if reboot_cause & 0x1:
            return (self.REBOOT_CAUSE_POWER_LOSS, "Power on reset")
        elif reboot_cause & 0x2:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
        elif reboot_cause & 0x4:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "PSU Shutdown")
        elif reboot_cause & 0x8:
            return (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, "Thermal overload")
        elif reboot_cause & 0x10:
            return (self.REBOOT_CAUSE_WATCHDOG, "Watchdog reset")
        elif reboot_cause & 0x20:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "BMC Shutdown")
        elif reboot_cause & 0x40:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Hot-Swap Shutdown")
        elif reboot_cause & 0x80:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Reset Button Shutdown")
        elif reboot_cause & 0x100:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Reset Button Cold Reboot")
        else:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_revision(self):
        """
        Retrieves the hardware revision for the chassis
        Returns:
            A string containing the hardware revision for this chassis.
        """
        return self._eeprom.revision_str().encode('utf-8').hex()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether Chassis is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_eeprom(self):
        """
        Retrieves the Sys Eeprom instance for the chassis.
        Returns :
            The instance of the Sys Eeprom
        """
        return self._eeprom

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            from sonic_platform.watchdog import Watchdog
            self._watchdog = Watchdog()

        return self._watchdog
