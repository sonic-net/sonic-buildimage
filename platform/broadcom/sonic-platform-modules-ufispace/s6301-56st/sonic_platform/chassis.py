#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import time
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from sonic_py_common import device_info
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
        self._initialize_components()

    def _initialize_components(self):
        from sonic_platform.component import Component
        for index in range(NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)
            
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        return device_info.get_platform()

    def initizalize_system_led(self):
        return True

    def get_status_led(self):
        return self.get_system_led("SYS_LED")

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
                         'sfp'          '<sfp number>'     '0'              Sfp removed
                                                           '1'              Sfp inserted
                                                           '2'              I2C bus stuck
                                                           '3'              Bad eeprom
                                                           '4'              Unsupported cable
                                                           '5'              High Temperature
                                                           '6'              Bad cable
                         --------------------------------------------------------------------
                  Ex. 'sfp':{'11':'0', '12':'1'},
                  Indicates that:
                     sfp 11 has been removed, sfp 12 has been inserted.
                  Note: For sfp, when event 3-6 happened, the module will not be avalaible,
                        XCVRD shall stop to read eeprom before SFP recovered from error status.
        """

        change_event_dict = {"sfp": {}}

        start_time = time.time()
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
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

                if sfp_change_dict:
                    change_event_dict["sfp"] = sfp_change_dict
                    return True, change_event_dict
                if forever:
                    time.sleep(1)
                else:
                    timeout = end_time - time.time()
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

    def get_transceiver_change_event(self, timeout=0):
        current_port_dict = {}
        ret_dict = {}

        # Check for OIR events and return ret_dict
        for index in range(self.platform_inventory['num_ports']):
            if self._sfp_list[index].get_presence():
                current_port_dict[index] = self.plugin_data['XCVR']['plug_status']['inserted']
            else:
                current_port_dict[index] = self.plugin_data['XCVR']['plug_status']['removed']

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
            # sfputil already convert to physical port index according to config
            sfp = self._sfp_list[index]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def set_system_led(self, led_device_name, color):
        """
        Sets the color of an System LED device
        Args:
           led_device_name: a pre-defined LED device name list used in pddf-device.json.
           color: A string representing the color with which to set a LED
        Returns:
           bool: True if the LED state is set successfully, False if not
        """

        if led_device_name in self.plugin_data['LED']['capability']['rw']:
            result, msg  = self.pddf_obj.set_system_led_color(led_device_name, color)
            if not result and msg:
                print(msg)
            return (result)
        else:
            print("Not Support")
            return False            

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