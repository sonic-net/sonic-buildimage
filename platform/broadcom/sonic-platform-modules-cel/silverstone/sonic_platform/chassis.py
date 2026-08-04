#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_py_common import device_info
    from sonic_platform_base.sfp_base import SfpBase
    from .helper import APIHelper
    import time
    import os
    import re
    import shutil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_TRAY = 7
NUM_FAN = 2
NUM_PSU = 2
NUM_THERMAL = 14
NUM_SFP = 32
NUM_COMPONENT = 10

IPMI_OEM_NETFN = "0x3A"
IPMI_GET_REBOOT_CAUSE = "0x03 0x00 0x01 0x06"

IPMI_SET_SYS_LED_CMD = "0x07 0x00 {}"
IPMI_GET_SYS_LED_CMD = "0x08 0x00"

LPC_GETREG_PATH = "/sys/bus/platform/devices/baseboard-lpc/getreg"
LPC_SETREG_PATH = "/sys/bus/platform/devices/baseboard-lpc/setreg"
LPC_STATUS_LED_REG = "0xA162"

ORG_HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
TMP_HW_REBOOT_CAUSE_FILE="/tmp/hw-reboot-cause.txt"

class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    #led color status
    SYSLED_COLOR_VAL_MAP = {
        'off':   '0xff',
        'green': '0xdc',
        'amber': '0xec',
        'green_blink_1hz': '0xdd',
        'green_blink_4hz': '0xde',
        'amber_blink_1hz': '0xed',
        'amber_blink_4hz': '0xee',
        'green_amber_1hz': '0xcd',
        'green_amber_4hz': '0xce'
    }

    SYSLED_VAL_COLOR_DESC_MAP = {
        "0xff": "off",
        "0xdc": "green",
        "0xec": "amber",
        "0xdd": "green blinking 1Hz",
        "0xde": "green blinking 4Hz",
        "0xed": "amber blinking 1Hz",
        "0xee": "amber blinking 4Hz",
        "0xcd": "green & amber alternate 1Hz",
        "0xce": "green & amber alternate 4Hz"
    }

    def __init__(self):
        ChassisBase.__init__(self)
        self._api_helper = APIHelper()
        self.sfp_module_initialized = False
        self._sfp_list = []
        self.sfp_status_dict = {}
        self._watchdog = None
        self._airflow_direction = None

        self.__initialize_eeprom()
        self.is_host = self._api_helper.is_host()

        self.__initialize_fan()
        self.__initialize_psu()
        self.__initialize_thermals()
        self.__initialize_components()

    def __initialize_sfp(self):
        sfputil_helper = SfpUtilHelper()
        port_config_file_path = device_info.get_path_to_port_config_file()
        sfputil_helper.read_porttab_mappings(port_config_file_path, 0)

        #from sonic_platform.sfp import Sfp
        from sonic_platform.sfp import Sfp
        for index in range(0, NUM_SFP):
            #sfp = Sfp(index, sfputil_helper.logical[index])
            sfp = Sfp(index, sfputil_helper.physical_to_logical[index + 1])
            self._sfp_list.append(sfp)
            self.sfp_status_dict[sfp.index] = '1' if sfp.get_presence() else '0'
        self.sfp_module_initialized = True

    def __initialize_psu(self):
        from sonic_platform.psu import Psu
        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)

    def __initialize_fan(self):
        from sonic_platform.fan_drawer import FanDrawer
        for i in range(NUM_FAN_TRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal
        # airflow = self.__get_air_flow()
        for index in range(0, NUM_THERMAL):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

    def __initialize_eeprom(self):
        from sonic_platform.eeprom import Eeprom
        self._eeprom = Eeprom()

    def __initialize_components(self):
        from sonic_platform.component import Component
        for index in range(0, NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

    def initizalize_system_led(self):
        pass

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_mac()

    def get_revision(self):
        """
        Retrieves the hardware revision for the chassis
        Returns:
            A string containing the hardware revision for this chassis.
        """
        return self._eeprom.get_revision()

    def get_serial(self):
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

        status, raw_cause = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_GET_REBOOT_CAUSE)
        hx_cause = raw_cause.split()[0] if status and len(
            raw_cause.split()) > 0 else 00

        # This tmp copy is to retain the reboot-cause only for the current boot
        if os.path.isfile(ORG_HW_REBOOT_CAUSE_FILE):
            shutil.move(ORG_HW_REBOOT_CAUSE_FILE, TMP_HW_REBOOT_CAUSE_FILE)

        if hx_cause == "77" and os.path.isfile(TMP_HW_REBOOT_CAUSE_FILE):
            with open(TMP_HW_REBOOT_CAUSE_FILE) as hw_cause_file:
                reboot_info = hw_cause_file.readline().rstrip('\n')
                match = re.search(r'Reason:(.*),Time:(.*)', reboot_info)
                if match is not None:
                    if match.group(1) == 'system':
                        return (self.REBOOT_CAUSE_NON_HARDWARE, 'System cold reboot')

        reboot_cause = {
            "00": self.REBOOT_CAUSE_HARDWARE_OTHER,
            "11": self.REBOOT_CAUSE_POWER_LOSS,
            "22": self.REBOOT_CAUSE_NON_HARDWARE,
            "33": self.REBOOT_CAUSE_NON_HARDWARE,
            "44": self.REBOOT_CAUSE_NON_HARDWARE,
            "55": self.REBOOT_CAUSE_HARDWARE_OTHER,
            "66": self.REBOOT_CAUSE_WATCHDOG,
            "77": self.REBOOT_CAUSE_POWER_LOSS,
            "88": self.REBOOT_CAUSE_WATCHDOG
        }.get(hx_cause, self.REBOOT_CAUSE_HARDWARE_OTHER)

        description = {
            "00": "Hardware Reason",
            "11": "Power On Reset",
            "22": "Soft-Set CPU Warm Reset",
            "33": "Soft-Set CPU Cold Reset",
            "44": "CPU Warm Reset",
            "55": "CPU Cold Reset",
            "66": "GPIO Watchdog Reset",
            "77": "Power Cycle Reset",
            "88": "Hardware Watchdog Reset"
        }.get(hx_cause, "Unknown reason")

        return (reboot_cause, description)

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

        sfp_dict = {}

        SFP_REMOVED = '0'
        SFP_INSERTED = '1'

        SFP_PRESENT = True
        SFP_ABSENT = False

        start_time = time.time()
        time_period = timeout/float(1000) #Convert msecs to secs

        while time.time() < (start_time + time_period) or timeout == 0:
            for sfp in self._sfp_list:
                port_idx = sfp.index
                if self.sfp_status_dict[port_idx] == SFP_REMOVED and \
                    sfp.get_presence() == SFP_PRESENT:
                    sfp_dict[port_idx] = SFP_INSERTED
                    self.sfp_status_dict[port_idx] = SFP_INSERTED
                elif self.sfp_status_dict[port_idx] == SFP_INSERTED and \
                    sfp.get_presence() == SFP_ABSENT:
                    sfp_dict[port_idx] = SFP_REMOVED
                    self.sfp_status_dict[port_idx] = SFP_REMOVED

            if sfp_dict:
                return True, {'sfp':sfp_dict}

            time.sleep(0.5)

        return True, {'sfp':{}} # Timeout

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
        return super(Chassis, self).get_sfp(index-1)

    ##############################################################
    ####################### Other methods ########################
    ##############################################################

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

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._api_helper.hwsku

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
        color_val = self.SYSLED_COLOR_VAL_MAP.get(color)
        if color_val == None:
            return False

        status = self._api_helper.lpc_setreg(LPC_SETREG_PATH, LPC_STATUS_LED_REG, color_val)
        return status

    def get_status_led(self):
        """
        Gets the state of the PSU status LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        color_val = self._api_helper.lpc_getreg(LPC_GETREG_PATH, LPC_STATUS_LED_REG)
        color = self.SYSLED_VAL_COLOR_DESC_MAP.get(color_val, "N/A")
        return color

    def get_airflow_direction(self):
        if self._airflow_direction == None:
            try:
                vendor_extn = self._eeprom.get_vendor_extn()
                airflow_type = vendor_extn.split()[2][2:4] # Either 0xFB or 0xBF
                if airflow_type == 'FB':
                    direction = 'exhaust'
                elif airflow_type == 'BF':
                    direction = 'intake'
                else:
                    direction = 'N/A'
            except (AttributeError, IndexError):
                direction = 'N/A'

            self._airflow_direction = direction

        return self._airflow_direction

    def get_port_or_cage_type(self, index):
        """
        Retrieves sfp port or cage type corresponding to physical port <index>

        Args:
            index: An integer (>=0), the index of the sfp to retrieve.

        Returns:
            The masks of all types of port or cage that can be supported on the port
            Types are defined in sfp_base.py
        """
        if index in range(1, 32+1):
            return (SfpBase.SFP_PORT_TYPE_BIT_QSFP28 | SfpBase.SFP_PORT_TYPE_BIT_SFP28 | \
                    SfpBase.SFP_PORT_TYPE_BIT_SFP_PLUS | SfpBase.SFP_PORT_TYPE_BIT_QSFPDD | SfpBase.SFP_PORT_TYPE_BIT_QSFP)
        else:
            raise NotImplementedError


