# -*- coding: utf-8 -*-

#############################################################################
# Ruijie B6510-48VS8CQ
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import time
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.common import Common
    from sonic_platform.sfp import Sfp
    from sonic_platform.sfp import PORT_START
    from sonic_platform.sfp import PORTS_IN_BLOCK
    from sonic_platform.logger import logger
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Chassis(ChassisBase):
    """
    Ruijie B6510-48VS8CQ Platform-specific Chassis class
    """
    def __init__(self):
        ChassisBase.__init__(self)
        self.CHASSIS_CONFIG = 'chassis.json'
        self.THERMAL_CONFIG = 'thermal.json'
        self.SFP_CONFIG = 'sfp.json'
        self.PSU_CONFIG = 'psu.json'
        self.FAN_CONFIG = 'fan.json'
        self.COMPONENT_CONFIG = 'component.json'

        self.SFP_STATUS_INSERTED = "1"
        self.SFP_STATUS_REMOVED = "0"
        self.port_dict = {}
        self.enable_read= ["i2cset", "-f", "-y", "2", "0x35", "0x2a", "0x01"]
        self.disable_read = ["i2cset", "-f", "-y", "2", "0x35", "0x2a", "0x00"]
        self.enable_write = ["i2cset", "-f", "-y", "2", "0x35", "0x2b", "0x00"]
        self.disable_write = ["i2cset", "-f", "-y", "2", "0x35", "0x2b", "0x01"]
        self.enable_erase = ["i2cset", "-f", "-y", "2", "0x35", "0x2c", "0x01"]
        self.disable_erase = ["i2cset", "-f", "-y", "2", "0x35", "0x2c", "0x00"]
        self.read_value = ["i2cget", "-f", "-y", "2", "0x35", "0x25"]
        self.write_value = ["i2cset", "-f", "-y", "2", "0x35", "0x21", "0x0a"]
        self.set_sys_led_cmd = ["i2cset", "-f", "-y", "2", "0x33", "0xb2"]
        self.get_sys_led_cmd = ["i2cget", "-f", "-y", "2", "0x33", "0xb2"]
        self.led_status = "red" 
        # Initialize SFP list
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # It will also provide support sfp controls like reset and setting
        # low power mode.
        # We pass the eeprom path and sfp control path from chassis.py
        # So that sfp.py implementation can be generic to all platforms
        for index in range(PORT_START, PORTS_IN_BLOCK):
            sfp_node = Sfp(index)
            self._sfp_list.append(sfp_node)
            if sfp_node.get_presence():
                self.port_dict[index] = self.SFP_STATUS_INSERTED
            else:
                self.port_dict[index] = self.SFP_STATUS_REMOVED

        self._api_common = Common()
        config_path = self._api_common.get_config_path(self.CHASSIS_CONFIG)
        self._config = self._api_common.load_json_file(config_path)
        self.__initialize_eeprom()

        if self._api_common.is_host():
            self.__initialize_fan()
            self.__initialize_psu()
            self.__initialize_thermals()
        else:
            self.__initialize_components()

    def __initialize_fan(self):
        from sonic_platform.fan import Fan
        from sonic_platform.fan_drawer import FanDrawer

        fan_config_path = self._api_common.get_config_path(self.FAN_CONFIG)
        self.fan_config = self._api_common.load_json_file(fan_config_path)["fans"]

        if self.fan_config:
            drawer_fan_list = []
            for index in range(0, len(self.fan_config)):
                fan = Fan(index, config=self.fan_config[index])
                self._fan_list.append(fan)
                drawer_fan_list.append(fan)
            fan_drawer = FanDrawer(0, fan_list=drawer_fan_list)
            self._fan_drawer_list.append(fan_drawer)

    def __initialize_psu(self):
        from sonic_platform.psu import Psu

        psu_config_path = self._api_common.get_config_path(self.PSU_CONFIG)
        self.psu_config = self._api_common.load_json_file(psu_config_path)["psus"]

        if self.psu_config:
            for index in range(0, len(self.psu_config)):
                psu = Psu(index, config=self.psu_config[index])
                self._psu_list.append(psu)

    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal

        thermal_config_path = self._api_common.get_config_path(self.THERMAL_CONFIG)
        self.thermal_config = self._api_common.load_json_file(thermal_config_path)['thermals']

        if self.thermal_config:
            for index in range(0, len(self.thermal_config)):
                thermal = Thermal(index, config=self.thermal_config[index])
                self._thermal_list.append(thermal)

    def __initialize_eeprom(self):
        from sonic_platform.eeprom import Eeprom
        self._eeprom = Eeprom(config=self._config["eeprom"])


    def __initialize_components(self):
        from sonic_platform.component import Component

        component_config_path = self._api_common.get_config_path(self.COMPONENT_CONFIG)
        self.component_config = self._api_common.load_json_file(component_config_path)['components']

        if self.component_config:
            for index in range(0, len(self.component_config)):
                component = Component(index, config=self.component_config[index])
                self._component_list.append(component)

    def _init_standard_config(self, conflist, class_name, objlist):
        for conf in conflist:
            obj = globals()[class_name](conf.get("name"), config=conf)
            objlist.append(obj)

    def _init_by_hal(self, hal_interface):
        self.hal_interface = hal_interface
        self.hal_interface.get_fans()

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        return self._eeprom.modelstr()

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
        return self._eeprom.part_number_str()

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_str()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.system_eeprom_info()

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
            is_power_loss = False
            # enable read
            getstatusoutput_noshell(self.disable_write)
            getstatusoutput_noshell(self.enable_read)
            ret , log = getstatusoutput_noshell(self.read_value)
            if ret == 0 and "0x0a" in log:
                is_power_loss = True

            # erase i2c and e2
            getstatusoutput_noshell(self.enable_erase)
            time.sleep(1)
            getstatusoutput_noshell(self.disable_erase)
            # clear data
            getstatusoutput_noshell(self.enable_write)
            getstatusoutput_noshell(self.disable_read)
            getstatusoutput_noshell(self.disable_write)
            getstatusoutput_noshell(self.enable_read)
            # enable write and set data
            getstatusoutput_noshell(self.enable_write)
            getstatusoutput_noshell(self.disable_read)
            getstatusoutput_noshell(self.write_value)
            if is_power_loss:
                return(self.REBOOT_CAUSE_POWER_LOSS, None)
        except Exception as e:
            logger.error(str(e))

        return (self.REBOOT_CAUSE_NON_HARDWARE, None)

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
                  Specifically for SFP event, besides SFP plug in and plug out,
                  there are some other error event could be raised from SFP, when
                  these error happened, SFP eeprom will not be avalaible, XCVRD shall
                  stop to read eeprom before SFP recovered from error status.
                      status='2' I2C bus stuck,
                      status='3' Bad eeprom,
                      status='4' Unsupported cable,
                      status='5' High Temperature,
                      status='6' Bad cable.
        """
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

    def get_all_components(self):
        return self._component_list

    def get_all_fans(self):
        return self._fan_list

    def get_all_psus(self):
        return self._psu_list

    def get_all_thermals(self):
        return self._thermal_list

    def get_supervisor_slot(self):
        """
        Retrieves the physical-slot of the supervisor-module in the modular
        chassis. On the supervisor or line-card modules, it will return the
        physical-slot of the supervisor-module.
        On the fixed-platforms, the API can be ignored.
        Users of the API can catch the exception and return a default
        ModuleBase.MODULE_INVALID_SLOT and bypass code for fixed-platforms.
        Returns:
            An integer, the vendor specific physical slot identifier of the
            supervisor module in the modular-chassis.
        """
        return 0

    def get_my_slot(self):
        """
        Retrieves the physical-slot of this module in the modular chassis.
        On the supervisor, it will return the physical-slot of the supervisor
        module. On the linecard, it will return the physical-slot of the
        linecard module where this instance of SONiC is running.
        On the fixed-platforms, the API can be ignored.
        Users of the API can catch the exception and return a default
        ModuleBase.MODULE_INVALID_SLOT and bypass code for fixed-platforms.
        Returns:
            An integer, the vendor specific physical slot identifier of this
            module in the modular-chassis.
        """
        return 0

    def is_modular_chassis(self):
        """
        Retrieves whether the sonic instance is part of modular chassis
        Returns:
            A bool value, should return False by default or for fixed-platforms.
            Should return True for supervisor-cards, line-cards etc running as part
            of modular-chassis.
        """
        return True

    def init_midplane_switch(self):
        """
        Initializes the midplane functionality of the modular chassis. For
        example, any validation of midplane, populating any lookup tables etc
        can be done here. The expectation is that the required kernel modules,
        ip-address assignment etc are done before the pmon, database dockers
        are up.
        Returns:
            A bool value, should return True if the midplane initialized
            successfully.
        """
        return True

    def get_module_index(self, module_name):
        """
        Retrieves module index from the module name
        Args:
            module_name: A string, prefixed by SUPERVISOR, LINE-CARD or FABRIC-CARD
            Ex. SUPERVISOR0, LINE-CARD1, FABRIC-CARD5
        Returns:
            An integer, the index of the ModuleBase object in the module_list
        """
        return 0

    def set_status_led(self, color):
        """
        Sets the state of the system LED
        Args:
            color: A string representing the color with which to set the
                   system LED
        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        colors = {
            "amber" : "0x00",
            "red" : "0x02",
            "green" : "0x04"
        }
        regval = colors.get(color, None)
        if regval is None:
            print("Invaild color input.")
            return False
        cmd = self.set_sys_led_cmd + [regval]
        ret, log = getstatusoutput_noshell(cmd)
        if ret != 0:
            print("Cannot execute %s" % self.set_sys_led_cmd + regval)
            return False
        self.led_status = color
        return True

    def get_status_led(self):
        """
        Gets the state of the system LED
        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        ret , log = getstatusoutput_noshell(self.get_sys_led_cmd)
        if ret != 0:
            print("Cannot execute %s" % self.get_sys_led_cmd)
            return False
        colors = {
            "0x00" : "amber",
            "0x02" : "red",
            "0x04" : "green"
        }
        color = colors.get(log, None)
        if color is None:
            return "Unknown color status"
        self.led_status = color
        return self.led_status

