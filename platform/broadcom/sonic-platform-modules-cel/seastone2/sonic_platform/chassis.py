#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

try:
    import sys
    import time
    import os
    import re
    import shutil
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform_base.sfp_base import SfpBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_DRAWER = 4
NUM_FAN_PER_DRAWER = 2
NUM_PSU = 2
NUM_THERMAL = 13
NUM_SFP = 33
NUM_COMPONENT = 9

REBOOT_CAUSE_REG = "0xA106"
BASE_CPLD_PLATFORM = "baseboard"
BASE_GETREG_PATH = "/sys/devices/platform/{}/getreg".format(BASE_CPLD_PLATFORM)
IPMI_GET_SYS_STATUS_LED="ipmitool raw 0x3A 0x0C 0x00 0x02 0x62"
IPMI_SET_SYS_STATUS_LED="ipmitool raw 0x3A 0x0C 0x00 0x03 0x62 {}"

ORG_HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
TMP_HW_REBOOT_CAUSE_FILE="/tmp/hw-reboot-cause.txt"

class Chassis(ChassisBase):
    """Platform-specific Chassis class"""
    sfp_status_dict={}

    #led color status
    SYSLED_COLOR_VAL_MAP = {\
        'off': '0x33',\
        'green': '0x10',\
        'green_blink_1hz': '0x11',\
        'amber': '0x20'\
    }

    SYSLED_VAL_COLOR_DESC_MAP = {
        0x33: 'off',\
        0x10: 'green',\
        0x11: 'green_blink_1hz',\
        0x20: 'amber'\
    }

    def __init__(self):
        ChassisBase.__init__(self)
        self._api_helper = APIHelper()
        self.sfp_module_initialized = False
        self.fan_module_initialized = False
        self._watchdog = None
        self._airflow_direction = None
        self.__initialize_eeprom()
        
        self.__initialize_thermals()
        self.__initialize_fan()
        self.__initialize_psu()
        self.__initialize_components()

    def __initialize_sfp(self):
        if not self.sfp_module_initialized:
            from sonic_platform.sfp import Sfp
            for index in range(0, NUM_SFP):
                sfp = Sfp(index)
                self._sfp_list.append(sfp)
                present = sfp.get_presence()
                self.sfp_status_dict[sfp.index] = '1' if present else '0'
            self.sfp_module_initialized = True

    def __initialize_psu(self):
        from sonic_platform.psu import Psu
        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)

    def __initialize_fan(self):
        from sonic_platform.fan import Fan
        from sonic_platform.fan_drawer import FanDrawer
        for fand_index in range(0, NUM_FAN_DRAWER):
            drawer_fan_list=[]
            for fan_index in range(0, NUM_FAN_PER_DRAWER):
                fan = Fan(fand_index, fan_index)
                self._fan_list.append(fan)
                drawer_fan_list.append(fan)
            fan_drawer = FanDrawer(fand_index, drawer_fan_list)
            self._fan_drawer_list.append(fan_drawer)
        self.fan_module_initialized = True
        

    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal
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
        return True

    def get_status_led(self):
        color_str = 'N/A'
        status, output = self._api_helper.run_command(IPMI_GET_SYS_STATUS_LED)
        if status:
            color_val = int(output, 16) & 0x33
            color_str = self.SYSLED_VAL_COLOR_DESC_MAP.get(color_val, color_str)

        return color_str

    def set_status_led(self, color):
        status = False

        color_val = self.SYSLED_COLOR_VAL_MAP.get(color, None)
        if color_val != None:
            cmd = IPMI_SET_SYS_STATUS_LED.format(color_val)
            status, res = self._api_helper.run_command(cmd)

        return status

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
        hw_reboot_cause = self._api_helper.get_register_value(
            BASE_GETREG_PATH, REBOOT_CAUSE_REG)


        # This tmp copy is to retain the reboot-cause only for the current boot
        if os.path.isfile(ORG_HW_REBOOT_CAUSE_FILE):
            shutil.move(ORG_HW_REBOOT_CAUSE_FILE, TMP_HW_REBOOT_CAUSE_FILE)

        if hw_reboot_cause == "0x33" and os.path.isfile(TMP_HW_REBOOT_CAUSE_FILE):
            with open(TMP_HW_REBOOT_CAUSE_FILE) as hw_cause_file:
                reboot_info = hw_cause_file.readline().rstrip('\n')
                match = re.search(r'Reason:(.*),Time:(.*)', reboot_info)
                if match is not None:
                    if match.group(1) == 'system':
                        return (self.REBOOT_CAUSE_NON_HARDWARE, 'System cold reboot')


        if hw_reboot_cause == "0x99":
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC
            description = 'ASIC Overload Reboot'
        elif hw_reboot_cause == "0x88":
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU
            description = 'CPU Overload Reboot'
        elif hw_reboot_cause == "0x77":
            reboot_cause = self.REBOOT_CAUSE_WATCHDOG
            description = 'Hardware Watchdog Reset'
        elif hw_reboot_cause == "0x55":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'CPU Cold Reset'
        elif hw_reboot_cause == "0x44":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'CPU Warm Reset'
        elif hw_reboot_cause == "0x33":
            # When NON_HARDWARE is used and device is powercycled via IPMI
            # reboot cause computed as Unknown, Hence using HARDWARE_OTHER
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Soft-Set Cold Reset'
        elif hw_reboot_cause == "0x22":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'Soft-Set Warm Reset'
        elif hw_reboot_cause == "0x11":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'Power On Reset'
        elif hw_reboot_cause == "0x00":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'Power Cycle Reset'
        else:
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Hardware reason'

        return (reboot_cause, description)

    ##############################################################
    ######################## SFP methods #########################
    ##############################################################

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis
        Returns:
            An integer, the number of sfps available on this chassis
        """
        self.__initialize_sfp()

        return len(self._sfp_list)

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis
        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        self.__initialize_sfp()

        return self._sfp_list

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (0-based) index <index>
        Args:
            index: An integer, the index (0-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 0.
            For example, 0 for Ethernet1, 1 for Ethernet2 and so on.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None
        self.__initialize_sfp()

        try:
            sfp = self._sfp_list[index - 1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (0-{})\n".format(
                             index, len(self._sfp_list) - 1))
        return sfp

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
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
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

    def get_revision(self):
        """
        Retrieves the hardware revision for the chassis
        Returns:
            A string containing the hardware revision for this chassis.
        """
        return self._eeprom.get_revision()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    ##############################################################
    ###################### Event methods #########################
    ##############################################################

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
            return SfpBase.SFP_PORT_TYPE_BIT_QSFP28
        elif index == 33:
            return SfpBase.SFP_PORT_TYPE_BIT_SFP_PLUS
        else:
            raise NotImplementedError


