#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import os
    import re
    import shutil
    import subprocess
    from . import component
    from .event import XcvrEvent
    from .helper import APIHelper
    from .thermal import ThermalMon, THERMAL_MONITOR_SENSORS
    from sonic_py_common import logger
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

RESET_SOURCE_OS_REG = '0xa106'
LPC_SYSLED_REG = '0xa162'
LPC_GETREG_PATH = "/sys/bus/platform/devices/baseboard/getreg"
LPC_SETREG_PATH = "/sys/bus/platform/devices/baseboard/setreg"
LED_CTRL_MODE_GET_CMD = "ipmitool raw 0x3a 0x42 0x01"

ORG_HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
TMP_HW_REBOOT_CAUSE_FILE="/tmp/hw-reboot-cause.txt"

SYSLOG_IDENTIFIER = "Chassis"
helper_logger = logger.Logger(SYSLOG_IDENTIFIER)

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """
    SYSLED_COLOR_VAL_MAP = {
        'off': '0xff',
        'green': '0xdc',
        'amber': '0xec',
        'amber_blink': '0xee',
        'amber_blink_4hz': '0xee',
        'amber_blink_1hz': '0xed',
        'green_blink': '0xde',
        'green_blink_4hz': '0xde',
        'green_blink_1hz': '0xdd'
    }

    SYSLED_VAL_COLOR_MAP = {
        '0xff': 'off',
        '0xdc': 'green',
        '0xec': 'amber',
        '0xee': 'amber_blink_4hz',
        '0xed': 'amber_blink_1hz',
        '0xde': 'green_blink_4hz',
        '0xdd': 'green_blink_1hz'
    }

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        self._api_helper = APIHelper()

        for index in range(self.platform_inventory['num_components']):
            component_obj = component.Component(index)
            self._component_list.append(component_obj)

        if not self._api_helper.is_bmc_present():
            thermal_count = len(self._thermal_list)
            for idx, name in enumerate(THERMAL_MONITOR_SENSORS):
                thermal = ThermalMon(thermal_count + idx, name)
                self._thermal_list.append(thermal)

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
        Args:
            None
        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        led_status = self._api_helper.lpc_getreg(LPC_GETREG_PATH, LPC_SYSLED_REG)
        color = self.SYSLED_VAL_COLOR_MAP.get(led_status, 'unknown')
        return color

    def set_status_led(self, color):
        """
        Sets the state of the system LED
        Args:
            color: A string representing the color with which to set the
                   system LED
        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        if self._api_helper.is_bmc_present():
            led_mode_cmd = LED_CTRL_MODE_GET_CMD
            if os.getuid() != 0:
                cmd = "sudo " + cmd
                led_mode_cmd = "sudo " + led_mode_cmd
            status, mode = self._api_helper.get_cmd_output(led_mode_cmd)
            # led take automatic control mode, led not settable
            if status != 0 or mode.strip() == "01":
                helper_logger.log_info("SYS LED takes automatic ctrl mode!")
                return False

        # Set SYS_LED through baseboard cpld
        color_val = self.SYSLED_COLOR_VAL_MAP.get(color, None)
        if color_val == None:
            helper_logger.log_error("SYS LED color {} not support!".format(color))
            return False

        status = self._api_helper.lpc_setreg(LPC_SETREG_PATH, LPC_SYSLED_REG, color_val)

        return status

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
        """
        hw_reboot_cause = self._api_helper.lpc_getreg(LPC_GETREG_PATH, RESET_SOURCE_OS_REG)
        
        if hw_reboot_cause == "0x33" and os.path.isfile(TMP_HW_REBOOT_CAUSE_FILE):
            with open(TMP_HW_REBOOT_CAUSE_FILE) as hw_cause_file:
               reboot_info = hw_cause_file.readline().rstrip('\n')
               match = re.search(r'Reason:(.*),Time:(.*)', reboot_info)
               description = 'CPU cold reset'
               if match is not None:
                  if match.group(1) == 'system':
                     return (self.REBOOT_CAUSE_NON_HARDWARE, 'System cold reboot')
        elif hw_reboot_cause == "0x99":
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC
            description = 'ASIC Overload Reboot'
        elif hw_reboot_cause == "0x88":
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU
            description = 'CPU Overload Reboot'
        elif hw_reboot_cause == "0x66":
            reboot_cause = self.REBOOT_CAUSE_WATCHDOG
            description = 'Hardware Watchdog Reset'
        elif hw_reboot_cause == "0x55":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'CPU Cold Reset'
        elif hw_reboot_cause == "0x44":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'CPU Warm Reset'
        elif hw_reboot_cause == "0x33":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'Soft-Set Cold Reset'
        elif hw_reboot_cause == "0x22":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'Soft-Set Warm Reset'
        elif hw_reboot_cause == "0x11":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'Power Off Reset'
        elif hw_reboot_cause == "0x00":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'Power Cycle Reset'
        else:
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Hardware reason'

        return (reboot_cause, description)

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                from sonic_platform.cpld_watchdog import Watchdog
                # Create the watchdog Instance from cpld watchdog
                self._watchdog = Watchdog()

        except Exception as e:
            helper_logger.log_error("Fail to load watchdog due to {}".format(e))
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
        if self.get_num_sfps() == 0:
            for index in range(self.platform_inventory['num_ports']):
                sfp = Sfp(index, self.pddf_obj, self.plugin_data)
                self._sfp_list.append(sfp)

        succeed, sfp_event = XcvrEvent(self._sfp_list).get_xcvr_event(timeout)
        if succeed:
            return True, {'sfp': sfp_event}

        return False, {'sfp': {}}

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
        return self._eeprom.revision_str()

    def get_system_airflow(self):
        """
        Retrieve system airflow
        Returns:
            string: INTAKE or EXHAUST
        """
        airflow = self.get_serial()[5:8]
        if airflow == "B2F":
            return "INTAKE"
        elif airflow == "F2B":
            return "EXHAUST"
        return "Unknown"

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
