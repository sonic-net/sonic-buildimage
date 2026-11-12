#!/usr/bin/env python
##################################################################
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
##################################################################

try:
    import os
    import sys
    import redis
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common import logger
    from sonic_platform.component import Component
    from sonic_platform.fan import Fan
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.thermal_manager import ThermalManager

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_SELECT_DELAY = 3600
COPPER_PORT_START = 1
COPPER_PORT_END = 24
SFP_PORT_START = 25
SFP_PORT_END = 30
PORT_END = 30
MAX_COMPONENT=2

NUM_FAN_TRAY = 1
PSU_NUMBER = 1

NUM_OF_THERMALS = 2

CONFIG_DB_INDEX = 4

SYSLOG_IDENTIFIER = "chassis"
sonic_logger=logger.Logger(SYSLOG_IDENTIFIER)
class Chassis(ChassisBase):
    """
    Platform-specific Chassis class
    derived from Dell S6000 platform.
    customized for the platform.
    """
    reset_reason_dict = {}
    reset_reason_dict[0x02] = ChassisBase.REBOOT_CAUSE_POWER_LOSS
    reset_reason_dict[0x20] = ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC

    reset_reason_dict[0x08] = ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU
    reset_reason_dict[0x10] = ChassisBase.REBOOT_CAUSE_WATCHDOG

    HOST_CHK_CMD = "docker > /dev/null 2>&1"

    def __init__(self):
        ChassisBase.__init__(self)
        self._sysled_color = self.STATUS_LED_COLOR_OFF
        self.__init_eeprom()
        self.__init_sfp()
        self.__init_fan()
        self.__init_component()
        self.__init_psu()
        self.__init_thermals()

    def __init_thermals(self):

        redis_client = redis.Redis(host='127.0.0.1', db=CONFIG_DB_INDEX)

        redis_client.hset('ASIC_SENSORS|ASIC_SENSORS_POLLER_STATUS','admin_status','enable')

        redis_client.hset('ASIC_SENSORS|ASIC_SENSORS_POLLER_INTERVAL','interval','10')

        for i in range(0, NUM_OF_THERMALS):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

    def __init_sfp(self):
        # for non-sfp ports create dummy objects for copper / non-sfp ports
        for index in range(COPPER_PORT_START, COPPER_PORT_END+1):
            sfp_node = Sfp(index, 'COPPER')
            self._sfp_list.append(sfp_node)

        for index in range(SFP_PORT_START, SFP_PORT_END+1):
            sfp_node = Sfp(index, 'SFP')
            self._sfp_list.append(sfp_node)

        self.sfp_event_initialized = False

    def __init_fan(self):
       from sonic_platform.fan_drawer import FanDrawer
       for fant_index in range(NUM_FAN_TRAY):
           fandrawer = FanDrawer(fant_index)
           self._fan_drawer_list.append(fandrawer)
           self._fan_list.extend(fandrawer._fan_list)

    def __init_component(self):
        for i in range(MAX_COMPONENT):
            component = Component(i)
            self._component_list.append(component)

    def __init_eeprom(self):
    # Instantiate ONIE system eeprom object
        self._eeprom = Eeprom()

    def __init_psu(self):
        for i in range(PSU_NUMBER):
            psu = Psu(i)
            self._psu_list.append(psu)

    def __is_host(self):
        return os.system(self.HOST_CHK_CMD) == 0

    def is_modular_chassis(self):
        """
        Retrieves whether the sonic instance is part of modular chassis

        Returns:
            A bool value, should return False by default or for fixed-platforms.
            Should return True for supervisor-cards, line-cards etc running as part
            of modular-chassis.
        """
        return False

    def get_thermal_manager(self):
        """
        Retrieves thermal manager class on this chassis
        :return: A class derived from ThermalManagerBase representing the
        specified thermal manager.
        """
        return ThermalManager

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

    def get_all_thermals(self):
        """
        Retrieves all thermals available on this chassis
        Returns:
            A list of objects representing all thermals available on this chassis
        """
        return self._thermal_list

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of physical SFP ports in a chassis,
            starting from 1.
            For example, 1 for first SFP port in the chassis and so on.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None
        try:
            # The index will start from 1
            sfp = self._sfp_list[index-1]
        except IndexError:
            sonic_logger.log_error("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def get_fan(self, index):
        """
        Retrieves fan module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan module to
            retrieve

        Returns:
            An object dervied from FanBase representing the specified fan
            module
        """
        fan = None
        try:
            # The index will start from 1
            fan = self._fan_list[index-1]
        except IndexError:
            sonic_logger.log_error("Fan index {} out of range (1-{})\n".format(
                             index, len(self._fan_list)))
        return fan

    def get_all_fans(self):
        """
        Retrieves all fan modules available on this chassis

        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this chassis
        """
        return self._fan_list

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

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this
            chassis.
        """
        return self._eeprom.serial_number_str()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the
        chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their
            corresponding values.
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
        #lrr = self._get_cpld_register('mb_reboot_cause')
        #if (lrr != 'ERR'):
        #    reset_reason = lrr
        #    if (reset_reason in self.reset_reason_dict):
        #        return (self.reset_reason_dict[reset_reason], None)
        #
        return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                from sonic_platform.watchdog import WatchdogImplBase
                watchdog_device_path = "/dev/watchdog0"
                self._watchdog = WatchdogImplBase(watchdog_device_path)
        except Exception as e:
            sonic_logger.log_warning(" Fail to load watchdog {}".format(repr(e)))

        return self._watchdog

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
        # Initialize SFP event first
        if not self.sfp_event_initialized:
            from sonic_platform.sfp_event import sfp_event
            self.sfp_event = sfp_event()
            self.sfp_event.initialize()
            self.MAX_SELECT_EVENT_RETURNED = PORT_END
            self.sfp_event_initialized = True

        wait_for_ever = (timeout == 0)
        port_dict = {}
        if wait_for_ever:
            # xrcvd will call this monitor loop in the "SYSTEM_READY" state
            # logger.log_info(" wait_for_ever get_change_event %d" % timeout)
            timeout = MAX_SELECT_DELAY
            while True:
                status = self.sfp_event.check_sfp_status( port_dict, timeout)
                if not port_dict == {}:
                    break
        else:
            # At boot up and in "INIT" state call from xrcvd will have a timeout value
            # return true without change after timeout and will transition to "SYSTEM_READY"
            # logger.log_info(" initial get_change_event %d" % timeout )
            status = self.sfp_event.check_sfp_status( port_dict, timeout)

        if status:
            return True, {'sfp':port_dict}
        else:
            return True, {'sfp':{}}

    def initizalize_system_led(self):
        """
        This function is not defined in chassis base class,
        system-health command would invoke chassis.initizalize_system_led(),
        add this stub function just to let the command sucessfully execute
        """
        pass

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED.

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        """
        The device supports the next states of the SYS led:
        • Green - Blinking: During Start up
        • Green - On: System up.
        All states must be described in system_health_monitoring_config.json

        TODO: Need to implement the LED's color switching
        """
        self._sysled_color = color
        return False # Not implemented yet

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return self._sysled_color