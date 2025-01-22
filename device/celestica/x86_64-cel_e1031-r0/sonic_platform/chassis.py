#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################


try:
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_py_common import device_info
    from .common import Common
    from .event import SfpEvent
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_TRAY = 3
NUM_PSU = 2
NUM_THERMAL = 7
NUM_SFP = 55
NUM_COMPONENT = 3
RESET_REGISTER = "0x112"
HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/previous-reboot-cause.txt"
PMON_REBOOT_CAUSE_PATH = "/usr/share/sonic/platform/api_files/reboot-cause/previous-reboot-cause.txt"
STATUS_LED_PATH = "/sys/devices/platform/e1031.smc/master_led"


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        ChassisBase.__init__(self)

        self._api_common = Common()
        self._is_host = self._api_common.is_host()

        self.__initialize_eeprom()
        self.__initialize_fan()
        self.__initialize_psu()
        self.__initialize_thermals()
        self.__initialize_components()

        self.sfp_module_initialized = False
        self._reboot_cause_path = HOST_REBOOT_CAUSE_PATH if self._is_host else PMON_REBOOT_CAUSE_PATH

    def __initialize_sfp(self):
        sfputil_helper = SfpUtilHelper()
        port_config_file_path = device_info.get_path_to_port_config_file()
        sfputil_helper.read_porttab_mappings(port_config_file_path, 0)

        from .sfp import Sfp
        for index in range(0, NUM_SFP):
            sfp = Sfp(index, sfputil_helper.logical[index])
            self._sfp_list.append(sfp)
        self.sfp_module_initialized = True

    def __initialize_psu(self):
        from .psu import Psu
        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)

    def __initialize_fan(self):
        from .fan_drawer import FanDrawer
        for i in range(NUM_FAN_TRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list += fandrawer.get_all_fans()

    def __initialize_thermals(self):
        from .thermal import Thermal
        airflow = self.__get_air_flow()
        for index in range(0, NUM_THERMAL):
            thermal = Thermal(index, airflow)
            self._thermal_list.append(thermal)

    def __initialize_eeprom(self):
        from .eeprom import Tlv
        self._eeprom = Tlv()

    def __initialize_components(self):
        from .component import Component
        for index in range(0, NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

    def __get_air_flow(self):
        air_flow_path = '/usr/share/sonic/device/{}/fan_airflow'.format(
            self._api_common.get_platform()) if self._is_host else '/usr/share/sonic/platform/fan_airflow'
        air_flow = self._api_common.read_txt_file(air_flow_path)
        return air_flow or 'B2F'

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_mac()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis
        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.get_eeprom()

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
        description = 'None'
        reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
        hw_reboot_cause = self._component_list[0].get_register_value(
            RESET_REGISTER)
        sw_reboot_cause = self._api_common.read_txt_file(
            self._reboot_cause_path) or "Unknown"

        if hw_reboot_cause == "0x55":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = sw_reboot_cause
        elif hw_reboot_cause == "0x11":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
        elif hw_reboot_cause == "0x33":
            reboot_cause = self.REBOOT_CAUSE_WATCHDOG
        elif hw_reboot_cause == "0x88":
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU
        elif hw_reboot_cause == "0x99":
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC
        else:
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'Unknown reason'
        return (reboot_cause, description)

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            from sonic_platform.watchdog import Watchdog
            self._watchdog = Watchdog()

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
        # SFP event
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        sfp_event = SfpEvent(self._sfp_list).get_sfp_event(timeout)
        if sfp_event:
            return True, {'sfp': sfp_event}

        return False, {'sfp': {}}

    ##############################################################
    ######################## SFP methods #########################
    ##############################################################

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

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 1.
            For example, 1 for Ethernet0, 2 for Ethernet4 and so on.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return super(Chassis, self).get_sfp(index - 1)

    ##############################################################
    ################## ThermalManager methods ####################
    ##############################################################

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._api_common.get_hwsku()

    def get_presence(self):
        """
        Retrieves the presence of the Chassis
        Returns:
            bool: True if Chassis is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._eeprom.get_pn()

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self.get_serial_number()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
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

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the PSU status LED
                   Note: Only support green and off
        Returns:
            bool: True if status LED state is set successfully, False if not
        """

        status_str = {
            self.STATUS_LED_COLOR_GREEN: 'green',
            self.STATUS_LED_COLOR_AMBER: 'amber',
            self.STATUS_LED_COLOR_OFF: 'off'
        }.get(color, 'off')

        return self._api_common.write_txt_file(STATUS_LED_PATH, status_str)

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        status = self._api_common.read_txt_file(STATUS_LED_PATH)
        status_str = {
            'on': self.STATUS_LED_COLOR_GREEN,
            'amber': self.STATUS_LED_COLOR_AMBER,
            'off': self.STATUS_LED_COLOR_OFF
        }.get(status, None)

        return status_str
