#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import time
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    #from rgutil.logutil import Logger
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PORT_START = 0
PORTS_IN_BLOCK = 56
FAN_NUM_PER_DRAWER = 1

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    SFP_STATUS_INSERTED = "1"
    SFP_STATUS_REMOVED = "0"
    port_dict = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)

    def get_revision(self):
        val  = ord(self._eeprom.revision_str())
        test = "{}".format(val)
        return test

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

        return (self.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_change_event(self, timeout=0):
        change_event_dict = {"fan": {}, "sfp": {}}
        sfp_status, sfp_change_dict = self.get_transceiver_change_event(timeout)
        change_event_dict["sfp"] = sfp_change_dict
        if sfp_status is True:
            return True, change_event_dict

        return False, {}

    def get_transceiver_change_event(self, timeout=0):
        start_time = time.time()
        currernt_port_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
        else:
            print("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print(
                "get_transceiver_change_event:" "time wrap / invalid timeout value",
                timeout,
            )
            return False, {}  # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            # Check for OIR events and return updated port_dict
            for index in range(PORT_START, PORTS_IN_BLOCK):
                if self._sfp_list[index].get_presence():
                    currernt_port_dict[index] = self.SFP_STATUS_INSERTED
                else:
                    currernt_port_dict[index] = self.SFP_STATUS_REMOVED
            if currernt_port_dict == self.port_dict:
                if forever:
                    time.sleep(1)
                else:
                    timeout = end_time - time.time()
                    if timeout >= 1:
                        time.sleep(1)  # We poll at 1 second granularity
                    else:
                        if timeout > 0:
                            time.sleep(timeout)
                        return True, {}
            else:
                # Update reg value
                self.port_dict = currernt_port_dict
                print(self.port_dict)
                return True, self.port_dict
        print("get_transceiver_change_event: Should not reach here.")
        return False, {}
