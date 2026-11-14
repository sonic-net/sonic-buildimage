#!/usr/bin/env python
##################################################################
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
##################################################################

try:
    import os
    import sys
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common import logger
    from sonic_platform.component import Component
    from sonic_platform.fan import Fan
    from sonic_platform.thermal import Thermal
    from sonic_platform.led import *
 #   from sonic_platform.sensor import VoltageSensor

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_SELECT_DELAY = 3600
COPPER_PORT_START = 1
COPPER_PORT_END = 24
SFP_PORT_START = 25
SFP_PORT_END = 28
PORT_END = 28
MAX_COMPONENT=2

NUM_FAN_TRAY = 1
PSU_NUMBER = 1

NUM_OF_BOARD_THERMALS = 3

CONFIG_DB_INDEX = 4

SYSLOG_IDENTIFIER = "chassis"
sonic_logger=logger.Logger(SYSLOG_IDENTIFIER)

SYSLED_I2CBUS = 0
SYSLED_I2CADDR = 0x33
SYSLED_REG = 0x38

SYSLED_STATES = {
    "STATUS_LED_OFF": 0b0100_0111,
    "STATUS_LED_COLOR_GREEN": 0b0000_0000,
    "STATUS_LED_COLOR_GREEN_BLINK": 0b0000_0101,
    "STATUS_LED_COLOR_AMBER": 0b0000_0010,
    "STATUS_LED_COLOR_AMBER_BLINK": 0b0000_0110,
    "STATUS_LED_COLOR_OFF": 0b0000_0111
}

class Chassis(ChassisBase):
    """
    Platform-specific Chassis class
    customized for the platform.
    """
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"
    HOST_CHK_CMD = "docker > /dev/null 2>&1"
    PLATFORM = "arm64-larch_canisminor"
    HWSKU = "canisminor"

    def __init__(self):
        ChassisBase.__init__(self)
        # Port numbers for Initialize SFP list
        self.COPPER_PORT_START = COPPER_PORT_START
        self.COPPER_PORT_END = COPPER_PORT_END
        self.SFP_PORT_START = SFP_PORT_START
        self.SFP_PORT_END = SFP_PORT_END
        self.PORT_END = PORT_END

        self._sysled_color = self.STATUS_LED_COLOR_OFF

        self.eeprom_path = '/sys/bus/i2c/devices/{}-0050/eeprom'

        #self.__init_eeprom()
        self.__init_sfp()
        self.__init_fans()
        #self.__init_component()
        #self.__init_psu()
        self.__init_thermals()
        self.__init_leds()


        # Instantiate ONIE system eeprom object
        self._eeprom = Eeprom()

        for i in range(MAX_COMPONENT):
             component = Component(i)
             self._component_list.append(component)

    def __init_thermals(self):
        for i in range(NUM_OF_BOARD_THERMALS):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

        # Fetching ASIC temperature is not supported yet for Aldrin3M
        #import redis
        #redis_client = redis.Redis(host='127.0.0.1', db=CONFIG_DB_INDEX)
        #redis_client.hset('ASIC_SENSORS|ASIC_SENSORS_POLLER_STATUS','admin_status','enable')
        #redis_client.hset('ASIC_SENSORS|ASIC_SENSORS_POLLER_INTERVAL','interval','10')
        #
        # self._thermal_list.append(Thermal(NUM_OF_BOARD_THERMALS))

    def __init_fans(self):
       from sonic_platform.fan_drawer import FanDrawer
       self._fan_drawer_list.append(FanDrawer(0, 2,1))
       self._fan_list.extend(self._fan_drawer_list[-1]._fan_list)

    def __init_leds(self):
        sysled_reg_bitfield = I2CRegisterBitField(SYSLED_I2CBUS, SYSLED_I2CADDR, SYSLED_REG, 0b0000_0111, SYSLED_STATES)
        self.sysled = I2CLed(sysled_reg_bitfield)

    def __init_sfp(self):
        # for non-sfp ports create dummy objects for copper / non-sfp ports
        for index in range(self.COPPER_PORT_START, self.COPPER_PORT_END+1):
            sfp_node = Sfp(index, 'COPPER', '', 0)
            self._sfp_list.append(sfp_node)

        for index in range(self.SFP_PORT_START, self.SFP_PORT_END+1):
            port=index-1
            i2cdev = index - self.SFP_PORT_START + 1
            port_eeprom_path = self.eeprom_path.format(i2cdev)
            if not os.path.exists(port_eeprom_path):
                sonic_logger.log_info(" DEBUG - path %s -- did not exist " % port_eeprom_path )
            sfp_node = Sfp(index, 'SFP', port_eeprom_path, i2cdev )
            self._sfp_list.append(sfp_node)

        self.sfp_event_initialized = False

    def __is_host(self):
        return os.system(self.HOST_CHK_CMD) == 0

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
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

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

    def get_all_thermals(self):
        """
        Retrieves all thermals available on this chassis
        Returns:
            A list of objects representing all thermals available on this chassis
        """
        return self._thermal_list

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
            self.MAX_SELECT_EVENT_RETURNED = self.PORT_END
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

    def set_status_led(self, color):
        return self.sysled.set_state(color)

    def initizalize_system_led(self):
        """
        This function is not defined in chassis base class,
        system-health command would invoke chassis.initizalize_system_led(),
        add this stub function just to let the command sucessfully execute
        """
        pass


    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return self._sysled_color
