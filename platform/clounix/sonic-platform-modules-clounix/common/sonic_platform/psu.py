#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_psu import PddfPsu
    from sonic_platform.thermal import Thermal
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Psu(PddfPsu):
    """PDDF Platform-Specific PSU class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfPsu.__init__(self, index, pddf_data, pddf_plugin_data)

        self._thermal_list.clear()
        self.num_psu_thermals = 3 # Fixing it 1 for now
        for psu_thermal_idx in range(self.num_psu_thermals):
            psu_thermal = Thermal(psu_thermal_idx, pddf_data, pddf_plugin_data, True, self.psu_index)
            self._thermal_list.append(psu_thermal)  
            
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        if not self.get_presence():
            return None
        
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
        if not self.get_presence():
            return None
    
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
        if not self.get_presence():
            return False
        
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
        if not self.get_presence():
            return False
        
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
        if not self.get_presence():
            return 0.0
        
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
        if not self.get_presence():
            return 0.0
        
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
        if not self.get_presence():
            return 0.0
        
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_p_out")
        if not output:
            return 0.0

        p_out = output['status']

        # power is returned in micro watts
        return float(p_out)/1000000

    def set_status_led(self, color):
        if not self.get_presence():
            return False
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
        if not self.get_presence():
            return self.STATUS_LED_COLOR_OFF
        
        psu_led_device = "PSU{}_LED".format(self.psu_index)
        if psu_led_device not in self.pddf_obj.data.keys():
            # Implement a generic status_led color scheme
            if self.get_powergood_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_OFF

        result, color = self.pddf_obj.get_system_led_color(psu_led_device)
        return (color)

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if not self.get_presence():
            return 0.0

        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_input")
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
        if not self.get_presence():
            return 0.0
    
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
        if not self.get_presence():
            return 0.0
    
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
        if not self.get_presence():
            return 0.0
    
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_p_in")
        if not output:
            return 0.0

        p_in = output['status']

        # power is returned in micro watts
        return float(p_in)/1000000

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU
        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if not self.get_presence():
            return 0.0
    
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_temp1_high_threshold")
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
        if not self.get_presence():
            return 0.0
    
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
        if not self.get_presence():
            return 0.0
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
        if not self.get_presence():
            return 0.0
    
        device = "PSU{}".format(self.psu_index)
        output = self.pddf_obj.get_attr_name_output(device, "psu_p_out_max")
        if not output:
            return 0.0

        p_out_max = output['status']
        # max power is in milliwatts
        return float(p_out_max)/1000