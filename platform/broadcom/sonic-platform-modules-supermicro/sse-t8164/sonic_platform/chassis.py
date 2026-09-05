#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import syslog
    import threading
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
        self.sfp_module_initialized = False
        self.lock = threading.Lock()

    def __initialize_sfp(self):
        with self.lock:
            if self.sfp_module_initialized:
                return

            for port_num in range(1, self.platform_inventory['num_ports']+1):    # start from 1
                # get_sfp() uses front port index start from 1
                sfp = self._sfp_list[port_num-1]
                presence = sfp.get_presence()
                self._global_port_pres_dict[port_num] = '1' if presence else '0'

            self.sfp_module_initialized = True

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis
        Returns:
            An integer, the number of sfps available on this chassis
        """
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return len(self._sfp_list)

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis
        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return self._sfp_list


    # Provide the functions/variables below for which implementation is to be overwritten
    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 0.
            For example, 0 for Ethernet0, 1 for Ethernet4 and so on.
        Returns:
            An object derived from SfpBase representing the specified sfp
        """
        sfp = None

        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        try:
            # The 'index' starts from 1 for this platform
            sfp = self._sfp_list[index-1]
        except IndexError:
            syslog.syslog(syslog.LOG_INFO, "SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

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
        watchdog_dir = '/sys/class/watchdog'
        description = 'Hardware Watchdog Reset'
        hw_reboot_cause = ""
        import os
        if not os.path.exists(watchdog_dir):
            syslog.syslog(syslog.LOG_INFO, "Watchdog is not supported on this platform")
            return (self.REBOOT_CAUSE_NON_HARDWARE, 'Unknown reason')

        for item in os.listdir(watchdog_dir):
            path = os.path.join(watchdog_dir, item)
            if os.path.isdir(path):
                bootstatus_file = os.path.join(path, "bootstatus")
                if os.path.exists(bootstatus_file):
                    try:
                        with open(bootstatus_file, "r") as f:
                            hw_reboot_cause = f.read().strip('\n')
                        if hw_reboot_cause == "32":
                            return (self.REBOOT_CAUSE_WATCHDOG, description)
                    except Exception as e:
                        raise Exception('Error while trying to find the HW reboot cause - {}'.format(str(e)))

                reboot_reason_file = os.path.join(path, "reboot_reason")
                if os.path.exists(reboot_reason_file):
                    try:
                        with open(reboot_reason_file, "r") as f:
                            hw_reboot_cause = f.read().strip('\n')
                        if hw_reboot_cause == "2":
                            return (self.REBOOT_CAUSE_WATCHDOG, description)
                    except Exception as e:
                        raise Exception('Error while trying to find the HW reboot cause - {}'.format(str(e)))

        syslog.syslog(syslog.LOG_INFO, "No watchdog reset detected")
        return (self.REBOOT_CAUSE_NON_HARDWARE, 'Unknown reason')

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level
        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.
        Returns:
            (bool, dict):
                - True if call successful, False if not;
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
        """
        start_ms = time.time() * 1000
        port_dict = {}
        change_dict = {}
        change_dict['sfp'] = port_dict
        while True:
            time.sleep(0.5)
            for port_num in range(1, self.platform_inventory['num_ports']+1):
                # get_presence() no wait for MgmtInit duration
                presence = self.get_sfp(port_num).get_presence()
                if self._global_port_pres_dict[port_num] == '0':
                    if presence:
                        self._global_port_pres_dict[port_num] = '1'
                        port_dict[port_num] = '1'
                else:
                    if not presence:
                        self._global_port_pres_dict[port_num] = '0'
                        port_dict[port_num] = '0'
                        # xcvr_api should be refreshed
                        self.get_sfp(port_num)._xcvr_api = None
                        self.get_sfp(port_num).xcvr_id = 0

            if(len(port_dict) > 0):
                return True, change_dict

            if timeout:
                now_ms = time.time() * 1000
                if (now_ms - start_ms >= timeout):
                    return True, change_dict

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                from sonic_platform.watchdog import Watchdog
                # Create the watchdog Instance
                self._watchdog = Watchdog()

        except Exception as e:
            syslog.syslog(syslog.LOG_INFO, "Fail to load watchdog due to {}".format(e))
        return self._watchdog

    def initizalize_system_led(self):
        return True

    def set_status_led(self, *args):
        if len(args) == 1:
            color = args[0]
        else:
            return False

        return super().set_system_led("SYS_LED", color)

    def get_status_led(self, *args):
        return super().get_system_led("SYS_LED")
