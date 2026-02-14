#############################################################################
# PDDF
#
# PDDF psu base class inherited from the base class
#
# All the supported PSU SysFS aattributes are
# - psu_present
# - psu_model_name
# - psu_power_good
# - psu_mfr_id
# - psu_serial_num
# - psu_fan_dir
# - psu_v_out
# - psu_v_out_min
# - psu_v_out_max
# - psu_i_out
# - psu_p_out
# - psu_p_out_max
# - psu_fan1_speed_rpm
# - psu_temp1_input
# - psu_temp1_high_threshold
# - psu_v_in
# - psu_i_in
# - psu_p_in
#############################################################################


try:
    from sonic_platform_base.psu_base import PsuBase
    from sonic_platform.fan import Fan
    from sonic_platform.thermal import Thermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PddfPsu(PsuBase):
    """PDDF generic PSU class"""

    pddf_obj = {}
    plugin_data = {}

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PsuBase.__init__(self)
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data
        self.platform = self.pddf_obj.get_platform()
        self.psu_index = index + 1

        self.num_psu_fans = int(self.pddf_obj.get_num_psu_fans('PSU{}'.format(index+1)))
        for psu_fan_idx in range(self.num_psu_fans):
            psu_fan = Fan(0, psu_fan_idx, pddf_data, pddf_plugin_data, True, self.psu_index)
            self._fan_list.append(psu_fan)

        self.num_psu_thermals = int(self.pddf_obj.get_num_psu_thermals('PSU{}'.format(index+1)))
        for psu_thermal_idx in range(self.num_psu_thermals):
            psu_thermal = Thermal(psu_thermal_idx, pddf_data, pddf_plugin_data, True, self.psu_index)
            self._thermal_list.append(psu_thermal)

    def get_num_fans(self):
        """
        Retrieves the number of fan modules available on this PSU

        Returns:
            An integer, the number of fan modules available on this PSU
        """
        return len(self._fan_list)

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        if 'name' in self.plugin_data['PSU']:
            return self.plugin_data['PSU']['name'][str(self.psu_index)]
        else:
            return "PSU{}".format(self.psu_index)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        status = 0
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_present")
        if not output:
            return False

        mode = output['mode']
        status = output['status']

        vmap = self.plugin_data['PSU']['psu_present'][mode]['valmap']

        if status.rstrip('\n') in vmap:
            return vmap[status.rstrip('\n')]
        else:
            return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_model_name")
        if not output:
            return None

        model = output['status']

        # strip_non_ascii
        stripped = (c for c in model if 0 < ord(c) < 127)
        model = ''.join(stripped)

        return model.rstrip('\n')

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_serial_num")
        if not output:
            return None

        serial = output['status']

        # strip_non_ascii
        stripped = (c for c in serial if 0 < ord(c) < 127)
        serial = ''.join(stripped)

        return serial.rstrip('\n')

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        device = "PSU{}".format(self.psu_index)

        output = self.pddf_obj.get_attr_name_output(device, "psu_power_good")
        if not output:
            return False

        mode = output['mode']
        status = output['status']

        vmap = self.plugin_data['PSU']['psu_power_good'][mode]['valmap']

        if status.rstrip('\n') in vmap:
            return vmap[status.rstrip('\n')]
        else:
            return False

    def get_mfr_id(self):
        """
        Retrieves the manufacturer id of the device

        Returns:
            string: Manufacturer Id of device
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_mfr_id")
        if not output:
            return None

        mfr = output['status']

        # strip_non_ascii
        stripped = (c for c in mfr if 0 < ord(c) < 127)
        mfr = ''.join(stripped)

        return mfr.rstrip('\n')

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_v_out")
        if not output:
            return 0.0

        v_out = output['status']

        return float(v_out)/1000

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_i_out")
        if not output:
            return 0.0

        i_out = output['status']

        # current in mA
        return float(i_out)/1000

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_p_out")
        if not output:
            return 0.0

        p_out = output['status']

        # power is returned in micro watts
        return float(p_out)/1000000

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        return self.get_status()

    def set_status_led(self, color):
        if 'psu_led_color' in self.plugin_data['PSU']:
            led_color_map = self.plugin_data['PSU']['psu_led_color']['colmap']
            if color in led_color_map:
                # change the color properly
                new_color = led_color_map[color]
                color = new_color
        led_device_name = "PSU{}".format(self.psu_index) + "_LED"
        result, msg = self.pddf_obj.set_system_led_color(led_device_name, color)
        return (result)

    def get_status_led(self):
        psu_led_device = "PSU{}_LED".format(self.psu_index)
        if psu_led_device not in self.pddf_obj.data.keys():
            # Implement a generic status_led color scheme
            if self.get_powergood_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_OFF

        result, color = self.pddf_obj.get_system_led_color(psu_led_device)
        return (color)

    def get_temperature(self, thermal_index=1):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_input".format(thermal_index))
        if not output:
            return 0.0

        temp1 = output['status']

        # temperature returned is in milli celcius
        return float(temp1)/1000

    def get_input_voltage(self):
        """
        Retrieves current PSU input voltage

        Returns:
            A float number, the input voltage in volts,
            e.g. 12.1
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_v_in")
        if not output:
            return 0.0

        v_in = output['status']

        return float(v_in)/1000

    def get_input_current(self):
        """
        Retrieves present electric current supplied to the PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_i_in")
        if not output:
            return 0.0

        i_in = output['status']

        # current in mA
        return float(i_in)/1000

    def get_input_power(self):
        """
        Retrieves current energy supplied to the PSU
        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_p_in")
        if not output:
            return 0.0

        p_in = output['status']

        # power is returned in micro watts
        return float(p_in)/1000000

    def get_temperature_high_threshold(self, thermal_index=1):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_high_threshold".format(thermal_index))
        if not output:
            return 0.0

        temp_high_thresh = output['status']
        return float(temp_high_thresh)/1000

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output
        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_v_out_max")
        if not output:
            return 0.0

        v_out_max = output['status']
        return float(v_out_max)/1000

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output
        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_v_out_min")
        if not output:
            return 0.0

        v_out_min = output['status']
        return float(v_out_min)/1000

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU
        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_p_out_max")
        if not output:
            return 0.0

        p_out_max = output['status']
        # max power is in milliwatts
        return float(p_out_max)/1000

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.psu_index

    def is_replaceable(self):
        """
        Indicate whether PSU is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def dump_sysfs(self):
        return self.pddf_obj.cli_dump_dsysfs('psu')
