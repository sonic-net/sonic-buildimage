#############################################################################
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################


try:
    import os
    import time
    import subprocess
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.platDev import PlatDev
    from sonic_platform.fan import Fan
    from sonic_platform.psu import Psu
    from sonic_platform.sfp import Sfp
    from sonic_platform.thermal import Thermal
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

GET_HWSKU_CMD = "sonic-cfggen -d -v DEVICE_METADATA.localhost.hwsku"
GET_PLATFORM_CMD = "sonic-cfggen -d -v DEVICE_METADATA.localhost.platform"


# XCVR type definition
SFP_TYPE = 0
QSFP_TYPE = 1


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    _global_port_pres_dict = {}

    def __init__(self):
        super(Chassis, self).__init__()

        # Initialize SKU name and Platform name
        self.sku_name = self._get_sku_name()
        self.platform_name = self._get_platform_name()
        self.name = self.sku_name

        # get the device infomation of platform
        self.platdev = PlatDev()

        #self._component_list = []
        #self._module_list = []
        #self._fan_drawer_list = []

        self._eeprom = Eeprom()
        # init component
        for i in range(self.platdev.get_component_count()):
            component = Component(i, self.platdev.get_component_name(
                i), self.platdev.get_component_descript(i))
            self._component_list.append(component)
        # init fan list
        if self.platdev.get_fan_support():
            fanlist = self.platdev.get_fan_list()
            for index in range(0, len(fanlist)):
                fan_name = fanlist[index]
                for pos in range(0, self.platdev.get_fan_num_by_name(fan_name)):
                    fan = Fan(
                        index, pos, [self.platdev.get_fan_sysfile_path_by_name(fan_name), ''])
                    self._fan_list.append(fan)

        # init psu list
        psulist = self.platdev.get_psu_list()
        for index in range(0, len(psulist)):
            psu_name = psulist[index]
            psu = Psu(index, [self.platdev.get_psu_attr_path_by_name(psu_name),
                              self.platdev.get_psu_status_path_by_name(psu_name)],
                      self.platdev.bmc_is_exist())
            self._psu_list.append(psu)

        # init thermal list
        thermal_info_list = self.platdev.get_thermal_dev_info_all()
        for index in range(0, len(thermal_info_list)):
            if len(self.platdev.get_thermal_dev_tempidx_by_idx(index)) > 1:
                for idx in self.platdev.get_thermal_dev_tempidx_by_idx(index):
                    thermal = Thermal(idx, self.platdev.get_thermal_dev_name_by_idx(index)+"-{}".format(idx),
                                      self.platdev.get_thermal_dev_sysfile_path_by_idx(
                                          index),
                                      self.platdev.bmc_is_exist(),
                                      self.platdev.get_thermal_dev_support_mask_by_idx(
                                          index),
                                      self.platdev.get_thermal_dev_ext_sysfile_list_by_idx(index))
                    self._thermal_list.append(thermal)
            else:
                thermal = Thermal(1, self.platdev.get_thermal_dev_name_by_idx(index),
                                  self.platdev.get_thermal_dev_sysfile_path_by_idx(
                                      index),
                                  self.platdev.bmc_is_exist(),
                                  self.platdev.get_thermal_dev_support_mask_by_idx(
                                      index),
                                  self.platdev.get_thermal_dev_ext_sysfile_list_by_idx(index))
                self._thermal_list.append(thermal)

        # init sfp list
        port_num = 1
        for sfpg_name in self.platdev.get_sfp_group_list():
            if self.platdev.get_sfp_group_type_by_name(sfpg_name) == 'QSFP28':
                sfp_type = QSFP_TYPE
            else:
                sfp_type = SFP_TYPE

            for x in range(0, self.platdev.get_sfp_group_number_by_name(sfpg_name)):
                eeprom_path_list = ['n/a', 'n/a']
                if self.platdev.get_sfp_group_path_by_name(sfpg_name)[x] != 'n/a':
                    eeprom_path_list[0] = self.platdev.get_sfp_group_path_by_name(sfpg_name)[
                        x] + '/eeprom'
                    if os.path.exists(eeprom_path_list[0].replace("0050", "0051")):
                        eeprom_path_list[1] = eeprom_path_list[0].replace(
                            "0050", "0051")
                # index: port index, start from 0
                # eeprom_path_list : a list of path to eeprom sysfile
                #   [0]: for 0x50
                #   [1]: for 0x51
                # ext_sysfile_list: used to get other function of sfp
                #   [0]: present
                #   [1]: reset
                #   [2]: get lowpower mode
                #   [3]: set lowpower mode
                sfp = Sfp(port_num, eeprom_path_list, sfp_type,
                          self.platdev.get_sfp_ext_sysfile_list())
                port_num += 1
                self._sfp_list.append(sfp)

        self.init_global_port_presence()

    def _get_sku_name(self):
        pipe = subprocess.Popen(
            GET_HWSKU_CMD, shell=True, stdout=subprocess.PIPE)
        out, err = pipe.communicate()
        return out.decode().rstrip('\n')

    def _get_platform_name(self):
        pipe = subprocess.Popen(
            GET_PLATFORM_CMD, shell=True, stdout=subprocess.PIPE)
        out, err = pipe.communicate()
        return out.decode().rstrip('\n')

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            string: The name of the device
        """
        return self.name

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr('')

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.part_number_str()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis
        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.serial_number_str()

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
        # not support any hardware reboot, just return REBOOT_CAUSE_NON_HARDWARE
        # to keep reboot cause as software reboot
        return (self.REBOOT_CAUSE_NON_HARDWARE, None)

    ##############################################
    # System LED methods
    ##############################################

    def set_status_led(self, color):
        """
        Sets the state of the system LED
        Args:
            color: A string representing the color with which to set the
                   system LED
        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        return False

    def get_status_led(self):
        """
        Gets the state of the system LED
        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        raise NotImplementedError

    ##############################################
    # Other methods
    ##############################################

    def init_global_port_presence(self):
        for port_num in range(0, self.platdev.get_sfp_num()):
            presence = self._sfp_list[port_num].get_presence()
            self._global_port_pres_dict[port_num] = '1' if presence else '0'

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
        port_dict = {}
        while True:
            for port_num in range(0, self.platdev.get_sfp_num()):
                presence = self._sfp_list[port_num].get_presence()
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                elif(not presence and
                     self._global_port_pres_dict[port_num] == '1'):
                    self._global_port_pres_dict[port_num] = '0'
                    port_dict[port_num] = '0'

                if len(port_dict) > 0:
                    return True, {'sfp': port_dict}

            time.sleep(1)

    def sfp_debugger(self):
        """
        Try to show all parameters read from eeprom with sfp methods
        """
        print("SFP EEPROM data:")
        for sfp_n in range(0, len(self._sfp_list)):
            print("======SFP{}==TYPE {}====".format(
                sfp_n, self._sfp_list[sfp_n].sfp_type))
            print("get_transceiver_info:")
            print(self._sfp_list[sfp_n].get_transceiver_info())
            print(" ")

            print("get_transceiver_threshold_info:")
            print(self._sfp_list[sfp_n].get_transceiver_threshold_info())
            print(" ")

            print("get_transceiver_bulk_status:")
            print(self._sfp_list[sfp_n].get_transceiver_bulk_status())
            print(" ")

        print("get_lpmode:")
        for sfp_n in range(0, len(self._sfp_list)):
            print("\tsfp{}: {}".format(
                sfp_n, self._sfp_list[sfp_n].get_lpmode()))
            # set_lpmode

        print("get_power_override:")
        for sfp_n in range(0, len(self._sfp_list)):
            print("\tsfp{}: {}".format(
                sfp_n, self._sfp_list[sfp_n].get_power_override()))

        print("get_temperature:")
        for sfp_n in range(0, len(self._sfp_list)):
            print("\tsfp{}: {}".format(
                sfp_n, self._sfp_list[sfp_n].get_temperature()))

        print("get_voltage")
        for sfp_n in range(0, len(self._sfp_list)):
            print("\tsfp{}: {}".format(
                sfp_n, self._sfp_list[sfp_n].get_voltage()))

        print("get_tx_bias")
        for sfp_n in range(0, len(self._sfp_list)):
            print("\tsfp{}: {}".format(
                sfp_n, self._sfp_list[sfp_n].get_tx_bias()))

        print("get_rx_power")
        for sfp_n in range(0, len(self._sfp_list)):
            print("\tsfp{}: {}".format(
                sfp_n, self._sfp_list[sfp_n].get_rx_power()))

        print("get_tx_power")
        for sfp_n in range(0, len(self._sfp_list)):
            print("\tsfp{}: {}".format(
                sfp_n, self._sfp_list[sfp_n].get_tx_power()))
