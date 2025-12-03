#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import time
    import sys
    import glob
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan import Fan
    from .fan_drawer import RealDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.component import Component
    from sonic_py_common import logger
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_SELECT_DELAY = 3600
COPPER_PORT_START = 1
COPPER_PORT_END = 48
SFP_PORT_START = 49
SFP_PORT_END = 52
PORT_END = 52
MAX_7215_COMPONENT=2

# Device counts
MAX_7215_FAN_DRAWERS = 2
MAX_7215_FANS_PER_DRAWER = 1
MAX_7215_PSU = 2
MAX_7215_THERMAL = 5
CPLD_DIR = "/sys/bus/i2c/devices/0-0041/"

SYSLOG_IDENTIFIER = "chassis"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)


class Chassis(ChassisBase):
    """
    Nokia platform-specific Chassis class
        Derived from Dell S6000 platform.
        customized for the 7215 platform.
    """

    def __init__(self):
        ChassisBase.__init__(self)
        self.system_led_supported_color = ['off', 'amber', 'green', 'amber_blink', 'green_blink']
        # Port numbers for SFP List Initialization
        self.COPPER_PORT_START = COPPER_PORT_START
        self.COPPER_PORT_END = COPPER_PORT_END
        self.SFP_PORT_START = SFP_PORT_START
        self.SFP_PORT_END = SFP_PORT_END
        self.PORT_END = PORT_END

        # for non-sfp ports create dummy objects for copper / non-sfp ports
        for index in range(self.COPPER_PORT_START, self.COPPER_PORT_END+1):
            sfp_node = Sfp(index, 'COPPER', 'N/A', 'N/A')
            self._sfp_list.append(sfp_node)

        # Verify optoe2 driver SFP eeprom devices were enumerated and exist
        # then create the sfp nodes
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        mux_dev = sorted(glob.glob("/sys/class/i2c-adapter/i2c-1/i2c-[0-9]"))
        y = 0
        for index in range(self.SFP_PORT_START, self.SFP_PORT_END+1):
            mux_dev_num = mux_dev[y]
            port_i2c_map = mux_dev_num[-1]
            y = y + 1
            port_eeprom_path = eeprom_path.format(port_i2c_map)
            if not os.path.exists(port_eeprom_path):
                sonic_logger.log_info("path %s didnt exist" % port_eeprom_path)
            sfp_node = Sfp(index, 'SFP', port_eeprom_path, port_i2c_map)
            self._sfp_list.append(sfp_node)
        self.sfp_event_initialized = False

        # Instantiate system eeprom object
        self._eeprom = Eeprom()

        # Construct lists fans, power supplies, thermals & components
        drawer_num = MAX_7215_FAN_DRAWERS
        fan_num_per_drawer = MAX_7215_FANS_PER_DRAWER
        drawer_ctor = RealDrawer
        fan_index = 0
        for drawer_index in range(drawer_num):
            drawer = drawer_ctor(drawer_index)
            self._fan_drawer_list.append(drawer)
            for index in range(fan_num_per_drawer):
                fan = Fan(fan_index, drawer, self.get_model())
                fan_index += 1
                drawer._fan_list.append(fan)
                self._fan_list.append(fan)

        for i in range(MAX_7215_PSU):
            psu = Psu(i, self.get_model())
            self._psu_list.append(psu)

        for i in range(MAX_7215_THERMAL):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

        for i in range(MAX_7215_COMPONENT):
            component = Component(self.get_model(), i)
            self._component_list.append(component)
  
    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _write_sysfs_file(self, sysfs_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'

        # Ensure that the write operation has succeeded
        if ((self._read_sysfs_file(sysfs_file)) != value ):
            time.sleep(3)
            if ((self._read_sysfs_file(sysfs_file)) != value ):
                rv = 'ERR'

        return rv
  
    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of physical SFP ports in a
            chassis starting from 1.

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

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr()

    def get_service_tag(self):
        """
        Retrieves the Service Tag of the chassis
        Returns:
            string: Service Tag of chassis
        """
        return self._eeprom.service_tag_str()

    def get_revision(self):
        """
        Retrieves the hardware revision of the chassis

        Returns:
            string: Revision value of chassis
        """
        #Revision is always 0 for 7215-IXS-A1
        return str(0)

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
        
        REBOOT_CAUSE_FILE = "/host/reboot-cause/reboot-cause.txt"
        if os.path.isfile(REBOOT_CAUSE_FILE):
            with open(REBOOT_CAUSE_FILE) as cause_file:
                software_reboot_cause = cause_file.readline().rstrip('\n')
        sonic_logger.log_notice("PAVAN Reboot-cause reported by software  - {}".format(software_reboot_cause))        
        value = self._read_sysfs_file(CPLD_DIR+"last_reset_cause")
        thermal = self._read_sysfs_file(CPLD_DIR+"temp_event_status")
        if (value == 'cold_reset'):
            reboot_cause=(ChassisBase.REBOOT_CAUSE_POWER_LOSS, "Cold Reset")
        elif (value == 'warm_reset'):
            reboot_cause=(ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Warm Reset")
        elif (value == 'wdog_reset' and software_reboot_cause == 'Unknown'):
            reboot_cause=(ChassisBase.REBOOT_CAUSE_WATCHDOG, None)
        elif (value == 'thermal_reset'):
            reboot_cause=(ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, thermal)
        else:
            reboot_cause=(ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

        #Log reboot cause and unmask temperature event    
        sonic_logger.log_notice("Reboot-cause reported by platform - {}".format(reboot_cause))
        self._write_sysfs_file(CPLD_DIR+"temp_event_mask", 0)

        return reboot_cause

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device

        Note:
            We overload this method to ensure that watchdog is only initialized
            when it is referenced. Currently, only one daemon can open the
            watchdog. To initialize watchdog in the constructor causes multiple
            daemon try opening watchdog when loading and constructing a chassis
            object and fail. By doing so we can eliminate that risk.
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
                status = self.sfp_event.check_sfp_status(port_dict, timeout)
                if not port_dict == {}:
                    break
        else:
            # At boot up and in "INIT" state call from xrcvd will have timeout
            # value return true without change after timeout and will
            # transition to "SYSTEM_READY"
            status = self.sfp_event.check_sfp_status(port_dict, timeout)

        if status:
            return True, {'sfp': port_dict}
        else:
            return True, {'sfp': {}}

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

    def initizalize_system_led(self):
        return True

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        if color not in self.system_led_supported_color:
            return False

        if (color == 'off'):
            value = 'off'
        elif (color == 'amber'):
            value ='amber'
        elif (color == 'green'):
            value ='green'
        elif (color == 'amber_blink'):
            value = 'blink4-amber'
        elif (color == 'green_blink'):
            value = 'blink4-green'
        else:
            return False
        # Write sys led
        status = self._write_sysfs_file(CPLD_DIR+"system_led", value)
        
        if status == "ERR":
            return False

        return True

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        # Read sys led
        value = self._read_sysfs_file(CPLD_DIR+"system_led")

        if value == 'off':
            color = 'off'
        elif value == 'amber':
            color = 'amber'
        elif value == 'green':
            color = 'green'
        elif value == 'blink4-amber':
            color = 'amber_blink'
        elif value == 'blink4-green':
            color = 'green_blink'
        else:
            return None

        return color

    def get_position_in_parent(self):
        """
		Retrieves 1-based relative physical position in parent device. If the agent 
        cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', 
        then the value '-1' is returned
		Returns:
		    integer: The 1-based relative physical position in parent device or -1 if 
            cannot determine the position
		"""
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
