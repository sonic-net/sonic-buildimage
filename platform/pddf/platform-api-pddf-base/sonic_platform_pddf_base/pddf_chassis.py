#############################################################################
# PDDF
# Module contains an implementation of SONiC PDDF Chassis Base API and
# provides the chassis information
#
#############################################################################

try:
    import sys
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.psu import Psu
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.thermal import Thermal
    from sonic_platform.eeprom import Eeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

voltage_sensor_present = True
try:
    from sonic_platform.voltage_sensor import VoltageSensor
except ImportError as e:
    voltage_sensor_present = False

current_sensor_present = True
try:
    from sonic_platform.current_sensor import CurrentSensor
except ImportError as e:
    current_sensor_present = False

asicthermal_present = True
try:
    from sonic_platform.asic_thermal import AsicThermal
except ImportError as e:
    asicthermal_present = False

component_present = True
try:
    from sonic_platform.component import Component
except ImportError as e:
    component_present = False

dcdc_present = True
try:
    from sonic_platform.dcdc import Dcdc
except ImportError as e:
    dcdc_present = False

class PddfChassis(ChassisBase):
    """
    PDDF Generic Chassis class
    """
    pddf_obj = {}
    plugin_data = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):

        ChassisBase.__init__(self)
        self._dcdc_list = []

        self.pddf_obj = pddf_data if pddf_data else None
        self.plugin_data = pddf_plugin_data if pddf_plugin_data else None
        if not self.pddf_obj or not self.plugin_data:
            try:
                from . import pddfapi
                import json
                self.pddf_obj = pddfapi.PddfApi()
                with open('/usr/share/sonic/platform/pddf/pd-plugin.json') as pd:
                    self.plugin_data = json.load(pd)
            except Exception as e:
                raise Exception("Error: Unable to load PDDF JSON data - %s" % str(e))

        self.platform_inventory = self.pddf_obj.get_platform()

        # Initialize EEPROM
        try:
            self._eeprom = Eeprom(self.pddf_obj, self.plugin_data)
        except Exception as err:
            sys.stderr.write("Unable to initialize syseeprom - {}".format(repr(err)))
            # Dont exit as we dont want failure in loading other components


        # FANs
        for i in range(self.platform_inventory.get('num_fantrays', 0)):
            fandrawer = FanDrawer(i, self.pddf_obj, self.plugin_data)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        # PSUs
        for i in range(self.platform_inventory.get('num_psus', 0)):
            psu = Psu(i, self.pddf_obj, self.plugin_data)
            self._psu_list.append(psu)

        # OPTICs
        for index in range(self.platform_inventory.get('num_ports', 0)):
            sfp = Sfp(index, self.pddf_obj, self.plugin_data)
            self._sfp_list.append(sfp)

        # THERMALs
        for i in range(self.platform_inventory.get('num_temps', 0)):
            thermal = Thermal(i, self.pddf_obj, self.plugin_data)
            self._thermal_list.append(thermal)

        if voltage_sensor_present:
            # VOLTAGE SENSORs
            for i in range(self.platform_inventory.get('num_voltage_sensors', 0)):
                voltage = VoltageSensor(i, self.pddf_obj, self.plugin_data)
                self._voltage_sensor_list.append(voltage)

        if current_sensor_present:
            # CURRENT SENSORs
            for i in range(self.platform_inventory.get('num_current_sensors', 0)):
                current = CurrentSensor(i, self.pddf_obj, self.plugin_data)
                self._current_sensor_list.append(current)

        if asicthermal_present:
            # ASIC Thermal
            position_offset = len(self._thermal_list)
            for i in range(self.platform_inventory.get('num_asic_temps', 0)):
                asicthermal = AsicThermal(i, position_offset, self.pddf_obj)
                self._thermal_list.append(asicthermal)

        if component_present:
            # Components (Programmables)
            for i in range(self.platform_inventory.get('num_components', 0)):
                component = Component(i, self.pddf_obj, self.plugin_data)
                self._component_list.append(component)

        if dcdc_present:
            # DC/DC
            for i in range(self.platform_inventory.get('num_dcdcs', 0)):
                dcdc = Dcdc(i, self.pddf_obj, self.plugin_data)
                self._dcdc_list.append(dcdc)

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

    def get_service_tag(self):
        """
        Retrieves the service tag of the chassis
        Returns:
            string: Sevice tag of chassis
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

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr()

    def get_revision(self):
        """
        Retrieves the hardware revision for the chassis

        Returns:
            A string containing the hardware revision for this
            chassis.
        """
        return self._eeprom.revision_str()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this
            chassis.
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
        raise NotImplementedError

    ##############################################
    # Component methods
    ##############################################

    ##############################################
    # Module methods
    ##############################################
    # All module methods are part of chassis_base.py
    # if they need to be overwritten, define them here

    ##############################################
    # Fan methods
    ##############################################
    # All fan methods are part of chassis_base.py
    # if they need to be overwritten, define them here

    ##############################################
    # PSU methods
    ##############################################
    # All psu methods are part of chassis_base.py
    # if they need to be overwritten, define them here

    ##############################################
    # THERMAL methods
    ##############################################
    # All thermal methods are part of chassis_base.py
    # if they need to be overwritten, define them here

    ##############################################
    # SFP methods
    ##############################################
    # All sfp methods are part of chassis_base.py
    # if they need to be overwritten, define them here

    ##############################################
    # System LED  methods
    ##############################################
    # APIs used by PDDF. Use them for debugging front panel
    # system LED and fantray LED issues
    def set_system_led(self, led_device_name, color):
        """
        Sets the color of an LED device in PDDF
        Args:
           led_device_name: a pre-defined LED device name list used in pddf-device.json.
           color: A string representing the color with which to set a LED
        Returns:
           bool: True if the LED state is set successfully, False if not
        """
        result, msg = self.pddf_obj.set_system_led_color(led_device_name, color)
        if not result and msg:
            print(msg)
        return (result)

    def get_system_led(self, led_device_name):
        """
        Gets the color of an LED device in PDDF
        Returns:
            string: color of LED or message if failed.
        """
        result, output = self.pddf_obj.get_system_led_color(led_device_name)
        return (output)

    ##############################################
    # Other methods
    ##############################################
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
            syslog.syslog(syslog.LOG_WARNING, "{}".format(e))
        return self._watchdog

    ##############################################
    # DCDC methods
    ##############################################

    def get_num_dcdcs(self):
        """
        Retrieves the number of DC/DC converters available on this chassis

        Returns:
            An integer, the number of DC/DC converters available on this
            chassis
        """
        return len(self._dcdc_list)

    def get_all_dcdcs(self):
        """
        Retrieves all DC/DC converters available on this chassis

        Returns:
            A list of objects derived from DcdcBase representing all DC/DC
            converters available on this chassis
        """
        return self._dcdc_list

    def get_dcdc(self, index):
        """
        Retrieves DC/DC converter represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the DC/DC converter to
            retrieve

        Returns:
            An object derived from DcdcBase representing the specified DC/DC
            converter
        """
        dcdc = None

        try:
            dcdc = self._dcdc_list[index]
        except IndexError:
            sys.stderr.write("DC/DC index {} out of range (0-{})\n".format(
                index, len(self._dcdc_list) - 1))

        return dcdc

    ##############################################
    # Device lookup
    ##############################################

    def get_all_devices(self, device_type):
        """
        Gets the list of devices of a specified type.

        Args:
            device_type: String corresponding to the DEVICE_TYPE member defined in
                         the platform base classes.

        Returns:
            List of devices with DEVICE_TYPE equal to device_type.
        """
        device_type = device_type.lower()
        if device_type == Sfp.DEVICE_TYPE.lower():
            return self.get_all_sfps()
        elif device_type == Psu.DEVICE_TYPE.lower():
            return self.get_all_psus()
        elif device_type == FanDrawer.DEVICE_TYPE.lower():
            return self.get_all_fan_drawers()
        elif device_type == Thermal.DEVICE_TYPE.lower():
            return self.get_all_thermals()
        elif dcdc_present and device_type == Dcdc.DEVICE_TYPE.lower():
            return self.get_all_dcdcs()
        return []

    def get_device(self, device_type, index):
        """
        Gets a device of a specifc type and index.

        Args:
            device_type: String corresponding to the DEVICE_TYPE member defined in
                         the platform base classes.
            index: An integer, the index of the device to retrieve. The index format
                   corresponds to that of the device type specified.

        Returns:
            Device object if one exists at the type and index, None otherwise.
        """
        if device_type == Sfp.DEVICE_TYPE.lower():
            return self.get_sfp(index)
        elif device_type == Psu.DEVICE_TYPE.lower():
            return self.get_psu(index)
        elif device_type == FanDrawer.DEVICE_TYPE.lower():
            return self.get_fan_drawer(index)
        elif device_type == Thermal.DEVICE_TYPE.lower():
            return self.get_thermal(index)
        elif dcdc_present and device_type == Dcdc.DEVICE_TYPE.lower():
            return self.get_dcdc(index)
        return None
