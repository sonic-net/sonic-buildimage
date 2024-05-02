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
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

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
