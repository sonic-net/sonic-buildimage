#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import time
    import sys
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_COMPONENT = 2

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    port_dict = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten
    def initizalize_system_led(self):
        return True

    def get_status_led(self):
        return self.get_system_led("SYS_LED")

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device

        Note:
            We overload this method to ensure that watchdog is only initialized
            when it is referenced. Currently, only one daemon can open the
            watchdog. To initialize watchdog in the constructor causes multiple
            daemon try opening watchdog when loading and constructing a chassis
            object and fail. By doing so we can eliminate that risk.
        """
        if self._watchdog is None:
            from sonic_platform.watchdog import Watchdog
            self._watchdog = Watchdog()

        return self._watchdog
        try:
            if self._watchdog is None:
                from sonic_platform.watchdog import Watchdog
                self._watchdog = Watchdog()
        except Exception as e:
            sonic_logger.log_warning(" Fail to load watchdog {}".format(repr(e)))

        return self._watchdog

    def get_system_airflow(self):
        """
        Retrieve system airflow
        Returns:
            string: INTAKE or EXHAUST
        """
        status = self._fan_drawer_list[0]._fan_list[0].get_direction()
        return status

    def get_thermal_manager(self):
        """
        Retrieves thermal manager class on this chassis

        Returns:
            A class derived from ThermalManagerBase representing the
            specified thermal manager
        """
        from .thermal_manager import ThermalManager
        return ThermalManager

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level
        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.
        Returns:
            (bool, dict):
                - bool: True if call successful, False if not;
                - dict: A nested dictionary where key is a device type,
                        value is a dictionary with key:value pairs in the format of
                        {'device_id':'device_event'}, where device_id is the device ID
                        for this device and device_event.
                        The known devices's device_id and device_event was defined as table below.
                         -----------------------------------------------------------------
                         device   |     device_id       |  device_event  |  annotate
                         -----------------------------------------------------------------
                         'fan'          '<fan number>'     '0'              Fan removed
                                                           '1'              Fan inserted
                         'sfp'          '<sfp number>'     '0'              Sfp removed
                                                           '1'              Sfp inserted
                                                           '2'              I2C bus stuck
                                                           '3'              Bad eeprom
                                                           '4'              Unsupported cable
                                                           '5'              High Temperature
                                                           '6'              Bad cable
                         'voltage'      '<monitor point>'  '0'              Vout normal
                                                           '1'              Vout abnormal
                         --------------------------------------------------------------------
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0', '12':'1'},
                       'voltage':{'U20':'0', 'U21':'1'}}
                  Indicates that:
                     fan 0 has been removed, fan 2 has been inserted.
                     sfp 11 has been removed, sfp 12 has been inserted.
                     monitored voltage U20 became normal, voltage U21 became abnormal.
                  Note: For sfp, when event 3-6 happened, the module will not be avalaible,
                        XCVRD shall stop to read eeprom before SFP recovered from error status.
        """

        change_event_dict = {"fan": {}, "sfp": {}, "voltage": {}}

        start_time = int(time.time())
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = int(timeout / 1000)  # Convert to secs
        else:
            print("get_change_event:Invalid timeout value", timeout)
            return False, change_event_dict

        end_time = start_time + timeout
        if start_time > end_time:
            print(
                "get_change_event:" "time wrap / invalid timeout value",
                timeout,
            )
            return False, change_event_dict  # Time wrap or possibly incorrect timeout
        try:
            while timeout >= 0:
                # check for sfp
                sfp_change_dict = self.get_transceiver_change_event()
                # check for fan
                # fan_change_dict = self.get_fan_change_event()
                # check for voltage
                # voltage_change_dict = self.get_voltage_change_event()

                if sfp_change_dict:
                    change_event_dict["sfp"] = sfp_change_dict
                    # change_event_dict["fan"] = fan_change_dict
                    # change_event_dict["voltage"] = voltage_change_dict
                    return True, change_event_dict
                if forever:
                    time.sleep(1)
                else:
                    timeout = end_time - int(time.time())
                    if timeout >= 1:
                        time.sleep(1)  # We poll at 1 second granularity
                    else:
                        if timeout > 0:
                            time.sleep(timeout)
                        return True, change_event_dict
        except Exception as e:
            print(e)
        print("get_change_event: Should not reach here.")
        return False, change_event_dict

    def get_transceiver_change_event(self):
        current_port_dict = {}
        ret_dict = {}

        # Check for OIR events and return ret_dict
        for index in range(1, self.platform_inventory['num_ports'] + 1):
            if self.get_sfp(index).get_presence():
                current_port_dict[index] = self.plugin_data["XCVR"]["plug_status"]["inserted"]
            else:
                current_port_dict[index] = self.plugin_data["XCVR"]["plug_status"]["removed"]

        if len(self.port_dict) == 0:       # first time
            self.port_dict = current_port_dict
            return {}

        if current_port_dict == self.port_dict:
            return {}

        # Update reg value
        for index, status in current_port_dict.items():
            if self.port_dict[index] != status:
                ret_dict[index] = status
                #ret_dict[str(index)] = status
        self.port_dict = current_port_dict
        for index, status in ret_dict.items():
            if int(status) == 1:
                pass
                #self._sfp_list[int(index)].check_sfp_optoe_type()
        return ret_dict

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Returns: An object dervied from SfpBase representing the specified sfp

        """
        sfp = None
        try:
            # The index will start from 1
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("override: SFP index {} out of range (1-{})\n".format(index, len(self._sfp_list)))
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

        reboot_cause_path = self.plugin_data['REBOOT_CAUSE']['reboot_cause_file']

        try:
            with open(reboot_cause_path, 'r', errors='replace') as fd:
                data = fd.read()
                sw_reboot_cause = data.strip()
        except IOError:
            sw_reboot_cause = "Unknown"

        return ('REBOOT_CAUSE_NON_HARDWARE', sw_reboot_cause)

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.modelstr()

    def get_revision(self):
        """
        Retrieves the revision of the chassis
        Returns:
            string: Revisionr of chassis
        """
        return self._eeprom.revision_str()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this
            chassis.
        """

        return self.get_serial()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
