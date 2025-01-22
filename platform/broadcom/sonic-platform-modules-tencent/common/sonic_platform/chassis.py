#!/usr/bin/env python3

#############################################################################
#
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import time
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.psu import Psu
    from sonic_platform.fan import Fan
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.thermal import Thermal
    from sonic_platform.component import Component
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.logger import logger
    from sonic_platform.dcdc import Dcdc

    from plat_hal.interface import interface
    from plat_hal.baseutil import baseutil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Chassis(ChassisBase):
    """
    Platform-specific Chassis class
    """
    # List of Dcdc objects representing all dcdc
    # available on the chassis
    _dcdc_list = None

    STATUS_INSERTED = "1"
    STATUS_REMOVED = "0"
    STATUS_NORMAL = "0"
    STATUS_ABNORMAL = "1"
    port_dict = {}
    fan_present_dict = {}
    voltage_status_dict = {}


    def __init__(self):
        ChassisBase.__init__(self)
        self._dcdc_list = []
        self.int_case = interface()
        conf = None
        conf = baseutil.get_config()
        port_cfg = conf.get("sfps", None)
        self.port_start = port_cfg.get("port_start", 0)
        self.port_end = port_cfg.get("port_end", 0)

        sfp_node = Sfp(self.port_start)
        if sfp_node._get_config("port_index_start") == 1:
            self._sfp_list.append(sfp_node)
        # Initialize SFP list

        # sfp.py will read eeprom contents and retrive the eeprom data.
        # It will also provide support sfp controls like reset and setting
        # low power mode.
        # We pass the eeprom path and sfp control path from chassis.py
        # So that sfp.py implementation can be generic to all platforms
        for index in range(self.port_start, self.port_end + 1):
            sfp_node = Sfp(index)
            self._sfp_list.append(sfp_node)
            if sfp_node.get_presence():
                self.port_dict[index] = self.STATUS_INSERTED
                # sfp_node.check_sfp_optoe_type()
            else:
                self.port_dict[index] = self.STATUS_REMOVED

        self._eeprom = Eeprom(self.int_case)

        fan_num = self.int_case.get_fan_total_number()
        drawer_fan_list = []
        for index in range(fan_num):
            fanobj = Fan(self.int_case, index + 1)
            self._fan_list.append(fanobj)
            drawer_fan_list.append(fanobj)
        if drawer_fan_list:
            fan_drawer = FanDrawer(0, fan_list=drawer_fan_list)
            self._fan_drawer_list.append(fan_drawer)

        psu_num = self.int_case.get_psu_total_number()
        for index in range(psu_num):
            psuobj = Psu(self.int_case, index + 1)
            self._psu_list.append(psuobj)

        thermal_num = self.int_case.get_temp_id_number()
        for index in range(thermal_num):
            thermalobj = Thermal(self.int_case, index + 1)
            self._thermal_list.append(thermalobj)

        component_num = self.int_case.get_cpld_total_number()
        for index in range(component_num):
            componentobj = Component(self.int_case, index + 1)
            self._component_list.append(componentobj)

        dcdc_num = self.int_case.get_dcdc_total_number()
        for index in range(dcdc_num):
            dcdcobj = Dcdc(self.int_case, index + 1)
            self._dcdc_list.append(dcdcobj)
        '''
        # init fan present status
        for index in range(0, len(self._fan_list)):
            if self._fan_list[index].get_presence() is True:
                self.fan_present_dict[index] = self.STATUS_INSERTED
            else:
                self.fan_present_dict[index] = self.STATUS_REMOVED

        # init voltage status
        for index in range(0, len(self._dcdc_list)):
            name = self._dcdc_list[index].get_name()
            value = self._dcdc_list[index].get_value()
            high = self._dcdc_list[index].get_high_threshold()
            low = self._dcdc_list[index].get_low_threshold()
            if (value is None) or (value > high) or (value < low):
                self.voltage_status_dict[name] = self.STATUS_ABNORMAL
            else:
                self.voltage_status_dict[name] = self.STATUS_NORMAL
        '''

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        name = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            self.log_error('syseeprom is not inited.')
            return ''

        e = sys_eeprom.read_eeprom()
        name = sys_eeprom.modelstr(e)
        if name is None:
            self.log_error('syseeprom name is error.')
            return ''
        return name

    def get_presence(self):
        """
        Retrieves the presence of the chassis
        Returns:
            bool: True if chassis is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        model = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            self.log_error('syseeprom is not inited.')
            return ''

        e = sys_eeprom.read_eeprom()
        model = sys_eeprom.modelnumber(e)
        if model is None:
            self.log_error('syseeprom model number is error.')
            return ''
        return model

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        serial_number = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            self.log_error('syseeprom is not inited.')
            return ''

        e = sys_eeprom.read_eeprom()
        serial_number = sys_eeprom.serial_number_str(e)
        if serial_number is None:
            self.log_error('syseeprom serial number is error.')
            return ''

        return serial_number

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        device_version = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            self.log_error('syseeprom is not inited.')
            return ''

        e = sys_eeprom.read_eeprom()
        device_version = sys_eeprom.deviceversion(e)
        if device_version is None:
            self.log_error('syseeprom serial number is error.')
            return ''

        return device_version

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self.get_serial_number()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

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

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        ret, color = self.int_case.get_led_color_by_type('SYS_LED')
        if ret is True:
            return color
        else:
            return 'N/A'

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        base_mac = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            self.log_error('syseeprom is not inited.')
            return ''

        e = sys_eeprom.read_eeprom()
        base_mac = sys_eeprom.base_mac_addr(e)
        if base_mac is None:
            self.log_error('syseeprom base mac is error.')
            return ''

        return base_mac.upper()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
            Ex. { '0x21':'AG9064', '0x22':'V1.0', '0x23':'AG9064-0109867821',
                  '0x24':'001c0f000fcd0a', '0x25':'02/03/2018 16:22:00',
                  '0x26':'01', '0x27':'REV01', '0x28':'AG9064-C2358-16G'}
        """
        sys_eeprom_dict = dict()
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            self.log_error('syseeprom is not inited.')
            return {}

        e = sys_eeprom.read_eeprom()
        if sys_eeprom._TLV_HDR_ENABLED:
            if not sys_eeprom.is_valid_tlvinfo_header(e):
                self.log_error('syseeprom tlv header error.')
                return {}
            total_len = (e[9] << 8) | e[10]
            tlv_index = sys_eeprom._TLV_INFO_HDR_LEN
            tlv_end = sys_eeprom._TLV_INFO_HDR_LEN + total_len
        else:
            tlv_index = sys_eeprom.eeprom_start
            tlv_end = sys_eeprom._TLV_INFO_MAX_LEN

        while (tlv_index + 2) < len(e) and tlv_index < tlv_end:
            if not sys_eeprom.is_valid_tlv(e[tlv_index:]):
                self.log_error("Invalid TLV field starting at EEPROM offset %d" % tlv_index)
                break

            tlv = e[tlv_index:tlv_index + 2 + e[tlv_index + 1]]
            name, value = sys_eeprom.decoder(None, tlv)
            sys_eeprom_dict[name] = value

            if e[tlv_index] == sys_eeprom._TLV_CODE_QUANTA_CRC or \
                    e[tlv_index] == sys_eeprom._TLV_CODE_CRC_32:
                break
            tlv_index += e[tlv_index + 1] + 2

        return sys_eeprom_dict

    def get_thermal_manager(self):
        """
        Retrieves thermal manager class on this chassis
        :return: A class derived from ThermalManagerBase representing the
        specified thermal manager. ThermalManagerBase is returned as default
        """
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
        reset_num = self.int_case.get_cpu_reset_num()
        # cold reboot
        if reset_num == 0:
            return(self.REBOOT_CAUSE_POWER_LOSS, None)

        return (self.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_module(self, index):
        """
        Retrieves module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the module to
            retrieve

        Returns:
            An object dervied from ModuleBase representing the specified
            module
        """
        module = None

        try:
            if self.get_num_modules():
                module = self._module_list[index]
        except IndexError:
            sys.stderr.write("Module index {} out of range (0-{})\n".format(
                             index, len(self._module_list)-1))

        return module

    def get_fan_drawer(self, index):
        """
        Retrieves fan drawers represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan drawer to
            retrieve

        Returns:
            An object dervied from FanDrawerBase representing the specified fan
            drawer
        """
        fan_drawer = None

        try:
            if self.get_num_fan_drawers():
                fan_drawer = self._fan_drawer_list[index]
        except IndexError:
            sys.stderr.write("Fan drawer index {} out of range (0-{})\n".format(
                             index, len(self._fan_drawer_list)-1))

        return fan_drawer

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
                # check for fan
                fan_change_dict = self.get_fan_change_event()
                # check for voltage
                voltage_change_dict = self.get_voltage_change_event()

                if sfp_change_dict or fan_change_dict or voltage_change_dict:
                    change_event_dict["sfp"] = sfp_change_dict
                    change_event_dict["fan"] = fan_change_dict
                    change_event_dict["voltage"] = voltage_change_dict
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
            logger.error(str(e))
            print(e)
        print("get_change_event: Should not reach here.")
        return False, change_event_dict

    def get_transceiver_change_event(self):
        current_port_dict = {}
        ret_dict = {}

        # Check for OIR events and return ret_dict
        for index in range(self.port_start, self.port_end + 1):
            if self._sfp_list[index].get_presence():
                current_port_dict[index] = self.STATUS_INSERTED
            else:
                current_port_dict[index] = self.STATUS_REMOVED

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

    def get_fan_change_event(self):
        currernt_fan_present_dict = {}
        ret_dict = {}

        # Check for OIR events and return ret_dict
        for index in range(0, len(self._fan_list)):
            if self._fan_list[index].get_presence() is True:
                currernt_fan_present_dict[index] = self.STATUS_INSERTED
            else:
                currernt_fan_present_dict[index] = self.STATUS_REMOVED

        if len(self.fan_present_dict) == 0:       # first time
            self.fan_present_dict = currernt_fan_present_dict
            return {}

        if currernt_fan_present_dict == self.fan_present_dict:
            return {}

        # updated fan_present_dict
        for index, status in currernt_fan_present_dict.items():
            if self.fan_present_dict[index] != status:
                ret_dict[str(index)] = status
        self.fan_present_dict = currernt_fan_present_dict
        return ret_dict

    def get_voltage_change_event(self):
        currernt_voltage_status_dict = {}
        ret_dict = {}

        # Check for OIR events and return ret_dict
        for index in range(0, len(self._dcdc_list)):
            name = self._dcdc_list[index].get_name()
            value = self._dcdc_list[index].get_value()
            high = self._dcdc_list[index].get_high_threshold()
            low = self._dcdc_list[index].get_low_threshold()
            if (value is None) or (value > high) or (value < low):
                currernt_voltage_status_dict[name] = self.STATUS_ABNORMAL
            else:
                currernt_voltage_status_dict[name] = self.STATUS_NORMAL

        if len(self.voltage_status_dict) == 0:   # first time
            self.voltage_status_dict = currernt_voltage_status_dict
            return {}

        if currernt_voltage_status_dict == self.voltage_status_dict:
            return {}

        # updated voltage_status_dict
        for name, status in currernt_voltage_status_dict.items():
            if self.voltage_status_dict[name] != status:
                ret_dict[name] = status
        self.voltage_status_dict = currernt_voltage_status_dict
        return ret_dict
