#!/usr/bin/env python
#
# Name: chassis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import os
    import re
    import time
    import collections
    from sonic_py_common import device_info
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.eeprom import Eeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_TRAY = 2
NUM_THERMAL = 1
NUM_PSU = 2

USR_SHARE_SONIC_PATH = "/usr/share/sonic"
HOST_DEVICE_PATH = USR_SHARE_SONIC_PATH + "/device"
CONTAINER_PLATFORM_PATH = USR_SHARE_SONIC_PATH + "/platform"

REBOOT_CAUSE_DIR = "/host/reboot-cause/"
REBOOT_CAUSE_FILE = os.path.join(REBOOT_CAUSE_DIR, "reboot-cause.txt")

class Chassis(ChassisBase):
    # System status LED
    _led = None

    def __init__(self):
        ChassisBase.__init__(self)

        self.data = {'valid':0, 'last':0}
        self.sfp_presence = {}

        if os.path.isdir(CONTAINER_PLATFORM_PATH):
            platform_path = CONTAINER_PLATFORM_PATH
        else:
            platform = device_info.get_platform()
            if platform is None:
                raise
            platform_path = os.path.join(HOST_DEVICE_PATH, platform)

        port_config_file = "/".join([platform_path, "V682-48x8c", "port_config.ini"])
        try:
            f = open(port_config_file)
        except:
            raise
        for line in f:
            line.strip()
            if re.search('^#', line) is not None:
                Port_cfg = collections.namedtuple('Port_cfg', line.split()[1:])
                break
        f.close()
        f = open(port_config_file)
        _port_cfgs = [Port_cfg(*tuple((line.strip().split())))
                           for line in f if re.search('^#', line) is None]
        f.close()

        # Initialize EEPROM
        self._eeprom = Eeprom()

        # Initialize FAN
        for i in range(NUM_FAN_TRAY):
            fandrawer = FanDrawer(i + 1)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        # Initialize PSU
        for index in range(0, NUM_PSU):
            psu = Psu(index + 1)
            self._psu_list.append(psu)

        # Initialize THERMAL
        for index in range(0, NUM_THERMAL):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

        # Initialize SFP
        for port_cfg in _port_cfgs:
            sfp = Sfp(int(port_cfg.index), 'SFP' if int(port_cfg.index) < 48 else 'QSFP')
            self._sfp_list.append(sfp)
            self.sfp_presence[int(port_cfg.index)] = False

##############################################
# Device methods
##############################################

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
        Retrieves the serial number of the chassis
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_number_str()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

##############################################
# Chassis methods
##############################################

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
            Ex. { '0x21':'AG9064', '0x22':'V1.0', '0x23':'AG9064-0109867821',
                  '0x24':'001c0f000fcd0a', '0x25':'02/03/2018 16:22:00',
                  '0x26':'01', '0x27':'REV01', '0x28':'AG9064-C2358-16G'}
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
        if os.path.exists(REBOOT_CAUSE_FILE):
            with open(REBOOT_CAUSE_FILE) as reboot_cause_file:
                reboot_cause = reboot_cause_file.readline()
            if re.search(r'User issued', reboot_cause) is None:
                return (self.REBOOT_CAUSE_POWER_LOSS, None)
            else:
                return (self.REBOOT_CAUSE_NON_HARDWARE, None)
        else:
            return (self.REBOOT_CAUSE_POWER_LOSS, None)

    def get_change_event(self, timeout=2000):
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
                  value is a dictionary with key:value pairs in the
                  format of {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
        """
        SFP_STATUS_INSERTED = '1'
        SFP_STATUS_REMOVED = '0'

        now = time.time()
        port_dict = {}

        if timeout < 1000:
            timeout = 1000
        timeout = (timeout) / float(1000) # Convert to secs

        if now < (self.data['last'] + timeout) and self.data['valid']:
            return False, {'sfp': {}}

        for sfp in self._sfp_list:
            sfp_presence = sfp.get_presence()
            if sfp_presence != self.sfp_presence[sfp.index]:
                self.sfp_presence[sfp.index] = sfp_presence
                if sfp_presence:
                    port_dict[sfp.index] = SFP_STATUS_INSERTED
                else:
                    port_dict[sfp.index] = SFP_STATUS_REMOVED

        if bool(port_dict):
            self.data['last'] = now
            self.data['valid'] = 1
            ret = True
        else:
            time.sleep(0.5)
            ret = False

        ret_dict = {'sfp': port_dict}
        return ret, ret_dict

    def get_num_psus(self):
        return len(self._psu_list)

    def get_psu(self, psu_index):
        return self._psu_list[psu_index]

    def initizalize_system_led(self):
        from .led import SystemLed
        Chassis._led = SystemLed()

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        return False if not Chassis._led else Chassis._led.set_status(color)

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return None if not Chassis._led else Chassis._led.get_status()
