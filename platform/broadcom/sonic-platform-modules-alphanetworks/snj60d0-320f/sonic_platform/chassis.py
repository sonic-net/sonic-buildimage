#############################################################################
# Alphanetworks
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

import time

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.fan import Fan
    from sonic_platform.psu import Psu
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.thermal import Thermal
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.led import FanLed
    from sonic_platform.led import PsuLed
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Chassis(ChassisBase):
    NUM_THERMAL = 3
    NUM_FANDRAWER = 6
    NUM_FANSPERDRAWER = 2
    NUM_FAN = NUM_FANDRAWER * NUM_FANSPERDRAWER
    NUM_PSU = 2
    NUM_SFP = 34
    HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/"
    PMON_REBOOT_CAUSE_PATH = "/usr/share/sonic/platform/api_files/reboot-cause/"
    REBOOT_CAUSE_FILE = "reboot-cause.txt"
    PREV_REBOOT_CAUSE_FILE = "previous-reboot-cause.txt"
    HOST_CHK_CMD = "docker > /dev/null 2>&1"

    def __init__(self):
        ChassisBase.__init__(self)
        # initialize thermals
        for index in range(0, Chassis.NUM_THERMAL):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

        # initialize fans
        for index in range(0, Chassis.NUM_FANDRAWER):
            fan_drawer = FanDrawer(index)
            for i in range(0, Chassis.NUM_FANSPERDRAWER):
                fan_index = Chassis.NUM_FANSPERDRAWER * index + i
                fan = Fan(fan_index, False)
                fan_drawer._fan_list.append(fan)
                self._fan_list.append(fan)
            self._fan_drawer_list.append(fan_drawer)

        # initialize fan led
        self.fan_led = FanLed.get_fanLed()
        self.fan_led.set_fans(self._fan_list)

        # initialize psus
        for index in range(0, Chassis.NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)

        # initialize psu led
        self.psu_led = PsuLed.get_psuLed()
        self.psu_led.set_psus(self._psu_list)

        # initialize sfps
        self.sfp_state = []
        for index in range(0, Chassis.NUM_SFP):
            if (index < Chassis.NUM_SFP-2):
                sfp = Sfp(index, 'QSFP')
            else:
                sfp = Sfp(index, 'SFP')
            self._sfp_list.append(sfp)

        # initialize eeprom
        self._eeprom = Eeprom()

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
        change_event_dict = {"sfp": {}}
        sfp_status, sfp_change_dict = self.get_transceiver_change_event(timeout)
        change_event_dict["sfp"] = sfp_change_dict
        if sfp_status is True:
            return True, change_event_dict

        return False, {}

    def get_transceiver_change_event(self, timeout=0):
        start_time = time.time()       
        # SFP status definition from xcvrd
        SFP_STATUS_INSERTED = '1'
        SFP_STATUS_REMOVED = '0'

        timeout = (timeout/1000)
        end_time = start_time + timeout
        while (timeout >= 0):
            new_sfp_state = []
            change_dict = {}
            for index in range(self.get_num_sfps()):
                # get current status
                state = self._sfp_list[index].get_presence() 
                new_sfp_state.append(state)

                if self.sfp_state == []:
                    change_dict[index] = SFP_STATUS_INSERTED if state == True else SFP_STATUS_REMOVED
                elif state != self.sfp_state[index]:
                    change_dict[index] = SFP_STATUS_INSERTED if state == True else SFP_STATUS_REMOVED

            self.sfp_state = new_sfp_state
            current_time = time.time()

            if bool(change_dict):
                return True, change_dict
            elif timeout == 0 or current_time < end_time:
                time.sleep(1)
                continue
            else:
                return True, {}

        return False, {}

    def get_thermal_manager(self):
        from sonic_platform.thermal_manager import ThermalManager
        return ThermalManager

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            string: The name of the device
        """
        return self._eeprom.modelstr()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.part_number_str()

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
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
        return self._eeprom.get_system_eeprom_info()

    def initizalize_system_led(self):
        from .led import SystemLed
        self._status_led = SystemLed.get_systemLed()

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        if self._status_led is None:
            self.initizalize_system_led()

        return self._status_led.set_status(color)

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        if self._status_led is None:
            self.initizalize_system_led()

        return self._status_led.get_status()

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                from sonic_platform.watchdog import Watchdog
                # Create the watchdog Instance
                self._watchdog = Watchdog()

        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, "Fail to load watchdog due to {}".format(e))
        return self._watchdog

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
        
        reboot_cause_path = (Chassis.HOST_REBOOT_CAUSE_PATH + Chassis.REBOOT_CAUSE_FILE)
        sw_reboot_cause = "Unknown"
        try:
            with open(reboot_cause_path, 'r') as fd:
                sw_reboot_cause = fd.read().strip()
        except IOError:
            pass
         
        return ('REBOOT_CAUSE_NON_HARDWARE', sw_reboot_cause)
