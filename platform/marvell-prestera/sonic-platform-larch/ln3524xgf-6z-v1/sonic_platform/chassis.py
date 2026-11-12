#!/usr/bin/env python
##################################################################
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
##################################################################

import time

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import SfpBoardConfig, Sfp
    from sonic_platform.psu import Psu
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
    from sonic_platform.thermal_manager import ThermalManager
    from sonic_platform.thermal import Thermal
    from sonic_platform.led import *
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


MAX_COMPONENT = 2

NUM_FAN_TRAY = 5
NUM_OF_BOARD_THERMALS = 2
NUM_PSU = 2

SYSLOG_IDENTIFIER = "chassis"

sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)


class Chassis(ChassisBase):
    def __init__(self):
        ChassisBase.__init__(self)

        self.__init_eeprom()
        self.__init_components()
        self.__init_thermals()
        self.__init_leds()
        self.__init_fans()
        self.__init_sfp()
        self.__init_psu()
   
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
        return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

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
            sfp = self._sfp_list[index - 1]
        except IndexError:
            sonic_logger.log_error("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp
    
    def set_status_led(self, color):
        try:
            return self._status_led.set_state(color)
        except (ValueError, OSError):
            return False

    def get_status_led(self):
        return self._status_led.get_state()

    def set_uid_led(self, color):
        try:
            return self._id_led.set_state(color)
        except (ValueError, OSError):
            return False

    def get_uid_led(self):
        return self._id_led.get_state()

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
        start_time = time.time() * 1000
        sfp_dict = {}
        events_dict = { 'sfp': sfp_dict }

        while True:
            for x in range(1, SfpBoardConfig.QSFP_PORT_END + 1):
                if True == self.get_sfp(x).get_presence():
                    presence = '1'
                else:
                    presence = '0'
                if presence != self.presence_list[x]:
                    self.presence_list[x] = presence
                    sfp_dict[int(x)] = presence

            if len(sfp_dict) > 0:
                return True, events_dict

            if timeout:
                time.sleep(1)
                now_time = time.time() * 1000
                if (now_time - start_time >= timeout):
                    return True, events_dict

    def __init_eeprom(self):
        # Instantiate ONIE system eeprom object (TLV based)
        self._eeprom = Eeprom()

    def __init_components(self):
        for i in range(MAX_COMPONENT):
            component = Component(i)
            self._component_list.append(component)

    def __init_thermals(self):
        for i in range(NUM_OF_BOARD_THERMALS):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

    def __init_leds(self):
        sysled_reg_bitfield = I2CRegisterBitField(LED_I2CBUS, LED_I2CADDR, None, SYSLED_MASK, SYSLED_STATES)
        fan_reg_bitfield = I2CRegisterBitField(LED_I2CBUS, LED_I2CADDR, None, FANLED_MASK, FANLED_STATES)
        id_reg_bitfield = I2CRegisterBitField(LED_I2CBUS, LED_I2CADDR, None, IDLED_MASK, IDLED_STATES)
       
        self._status_led = I2CLed(sysled_reg_bitfield)
        self._fan_led = I2CLed(fan_reg_bitfield)
        self._id_led = I2CLed(id_reg_bitfield)

        # The SYS LED is owned by system-health, which drives it from the
        # normal/fault/booting colours in system_health_monitoring_config.json.
        # Forcing it green here would override that from every process that
        # builds a Chassis (each pmon daemon, every platform-API CLI), painting
        # over a real fault within seconds - see ln3548xgf-6z-v1 (#36083).
        self._fan_led.set_state("STATUS_LED_COLOR_OFF")
        self._id_led.set_state("STATUS_LED_COLOR_OFF")

    def __init_fans(self):
        from sonic_platform.fan_drawer import FanDrawer

        for i in range(NUM_FAN_TRAY):
           fan_drawer = FanDrawer(i)
           self._fan_drawer_list.append(fan_drawer)
           self._fan_list.extend(fan_drawer._fan_list)

    def __init_sfp(self):
        # The list order must follow the physical "index" column of
        # port_config.ini, which xcvrd/sfputil pass to get_sfp():
        #   index 1..6  -> QSFP ports 25..30 (Ethernet24,28,...,44)
        #   index 7..30 -> SFP  ports 1..24  (Ethernet0..23)
        for i in range(SfpBoardConfig.QSFP_PORT_START, SfpBoardConfig.QSFP_PORT_END + 1):
            try:
                self._sfp_list.append(Sfp(i, SfpBoardConfig.QSFP_TYPE))
            except Exception as e:
                sonic_logger.log_warning("Port {} QSFP init failed: {}".format(i, e))
                self._sfp_list.append(None)  # keep index aligned with port number

        for i in range(SfpBoardConfig.SFP_PORT_START, SfpBoardConfig.QSFP_PORT_START):
            try:
                self._sfp_list.append(Sfp(i, SfpBoardConfig.SFP_TYPE))
            except Exception as e:
                sonic_logger.log_warning("Port {} SFP init failed: {}".format(i, e))
                self._sfp_list.append(None)  # keep index aligned with port number

        self.presence_list = ['0'] * (SfpBoardConfig.QSFP_PORT_END + 1)

    def __init_psu(self):
        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)


    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this
            chassis.
        """
        return self._eeprom.serial_number_str()

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

    def get_thermal_manager(self):
        """
        Retrieves thermal manager class on this chassis
        :return: A class derived from ThermalManagerBase representing the
        specified thermal manager.
        """
        return ThermalManager

    def initizalize_system_led(self):
        """
        This function is not defined in chassis base class,
        system-health command would invoke chassis.initizalize_system_led(),
        add this stub function just to let the command sucessfully execute
        """
        pass