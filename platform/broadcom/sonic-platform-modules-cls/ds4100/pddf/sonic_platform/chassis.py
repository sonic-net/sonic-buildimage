#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import re
    import os
    import subprocess
    import shutil
    import time
    from .helper import APIHelper
    from . import component
    from sonic_py_common import logger
    from .watchdog import Watchdog
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from .thermal import UpdateThermalInfo, NonPddfThermal, NONPDDF_THERMAL_SENSORS
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

LPC_SYSLED_REG = '0xa162'
LPC_GETREG_PATH = "/sys/bus/platform/devices/sys_cpld/getreg"
LPC_SETREG_PATH = "/sys/bus/platform/devices/sys_cpld/setreg"
REBOOT_CAUSE_PATH = "/sys/devices/platform/cpld_wdt/reason"
HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
SYS_LED_SYSFS_PATH = "/sys/bus/platform/devices/sys_cpld/sys_led"
TMP_HW_REBOOT_CAUSE_FILE="/tmp/hw-reboot-cause.txt"

SYSLOG_IDENTIFIER = "Chassis"
helper_logger = logger.Logger(SYSLOG_IDENTIFIER)

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """
    sfp_status_dict = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        self._api_helper = APIHelper()

        for port_idx in range(1, self.platform_inventory['num_ports'] + 1):
            self.sfp_status_dict[port_idx] = self.get_sfp(port_idx).get_presence()

        for index in range(self.platform_inventory['num_component']):
            component_obj = component.Component(index)
            self._component_list.append(component_obj)

        #update thermal threshold based on airflow direction
        UpdateThermalInfo(self.get_system_airflow())

        thermal_count = len(self._thermal_list)
        if not self._api_helper.is_bmc_present():
            for idx, name in enumerate(NONPDDF_THERMAL_SENSORS):
                thermal = NonPddfThermal(thermal_count + idx, name)
                self._thermal_list.append(thermal)


    def initizalize_system_led(self):
        """
        This function is not defined in chassis base class,
        system-health command would invoke chassis.initizalize_system_led(),
        add this stub function just to let the command sucessfully execute
        """
        pass

    def get_status_led(self):
        sys_led_color = "unknown"
        try:
            with open(SYS_LED_SYSFS_PATH, "r") as fd:
                sys_led_color = fd.read().rstrip('\n')
        except Exception as err:
            print(f"Failed to get status LED due to: {err}")
        return sys_led_color

    def set_status_led(self, color):     
        try:
            with open(SYS_LED_SYSFS_PATH, "w") as fd:
                fd.write(color)
        except Exception as err:
            print(f"Failed to set status LED due to: {err}")
            return False
        return True

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        For Quanta the index in sfputil.py starts from 1, so override
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 1.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            if (index == 0):
                raise IndexError
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("override: SFP index {} out of range (1-{})\n".format(
                index, len(self._sfp_list)))

        return sfp
    # Provide the functions/variables below for which implementation is to be overwritten

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot
        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
            REBOOT_CAUSE_POWER_LOSS = "Power Loss"
            REBOOT_CAUSE_THERMAL_OVERLOAD_CPU = "Thermal Overload: CPU"
            REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC = "Thermal Overload: ASIC"
            REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER = "Thermal Overload: Other"
            REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED = "Insufficient Fan Speed"
            REBOOT_CAUSE_WATCHDOG = "Watchdog"
            REBOOT_CAUSE_HARDWARE_OTHER = "Hardware - Other"
            REBOOT_CAUSE_NON_HARDWARE = "Non-Hardware"
        """

        # This tmp copy is to retain the reboot-cause only for the current boot
        if os.path.isfile(HW_REBOOT_CAUSE_FILE):
            shutil.move(HW_REBOOT_CAUSE_FILE, TMP_HW_REBOOT_CAUSE_FILE)

        reboot_cause = self._api_helper.read_txt_file(REBOOT_CAUSE_PATH) or "Unknown"

        reboot_cause_description = {
            '0x11': (self.REBOOT_CAUSE_POWER_LOSS, "Power on reset"),
            '0x22': (self.REBOOT_CAUSE_HARDWARE_CPU, "Soft-set CPU warm reset"),
            '0x33': (self.REBOOT_CAUSE_HARDWARE_CPU, "Soft-set CPU cold reset"),
            '0x44': (self.REBOOT_CAUSE_NON_HARDWARE, "CPU warm reset"),
            '0x66': (self.REBOOT_CAUSE_WATCHDOG, "Hardware Watchdog Reset"),

        }
        if reboot_cause == '0x77':
            if os.path.isfile(TMP_HW_REBOOT_CAUSE_FILE):
                with open(TMP_HW_REBOOT_CAUSE_FILE) as hw_cause_file:
                    reboot_info = hw_cause_file.readline().rstrip('\n')
                    match = re.search(r'Reason:(.*),Time:(.*)', reboot_info)
                    if match is not None and match.group(1) == 'system':
                        return (self.REBOOT_CAUSE_NON_HARDWARE, 'System cold reboot')
            return (self.REBOOT_CAUSE_HARDWARE_CPU, 'Power cycle reset')

        return reboot_cause_description.get(reboot_cause,
                                    (self.REBOOT_CAUSE_NON_HARDWARE, "Unknown reason"))

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                # Create the watchdog Instance
                self._watchdog = Watchdog()
        except Exception as E:
            syslog.syslog(syslog.LOG_ERR, "Fail to load watchdog due to {}".format(E))
        return self._watchdog

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
        sfp_dict = {}

        start_time = time.time()
        time_period = timeout / float(1000)  # Convert msecs to secss

        while time.time() < (start_time + time_period) or timeout == 0:
            for port_idx in range(1, self.platform_inventory['num_ports'] + 1):
                presence = self.get_sfp(port_idx).get_presence()
                if self.sfp_status_dict[port_idx] != presence:
                   self.sfp_status_dict[port_idx] = presence
                   sfp_dict[port_idx] = '1' if presence else '0'

            if sfp_dict:
                return True, {'sfp': sfp_dict}

            time.sleep(0.5)

        return True, {'sfp': {}}  # Timeout

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_number_str()

    def get_revision(self):
        """
        Retrieves the hardware revision for the chassis
        Returns:
            A string containing the hardware revision for this chassis.
        """
        return self._eeprom.get_revision()

    def get_system_airflow(self):
        """
        Retrieve system airflow
        Returns:
            string: INTAKE or EXHAUST
        """
        fans = self.get_all_fans()
        for fan in fans:
            if fan.get_presence():
                dir = fan.get_direction()
                if dir in ['INTAKE', 'EXHAUST']:
                    return dir
        return None

    def get_thermal_manager(self):
        """
        Retrieves thermal manager class on this chasssis

        Returns:
            A class derived from ThermalManagerBase representing the
            specified thermal manager
        """
        if not self._api_helper.is_bmc_present():
            from .thermal_manager import ThermalManager
            return ThermalManager
        return None
