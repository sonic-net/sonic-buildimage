#############################################################################
# PDDF
#
# PDDF thermal base class inherited from the base class
#
# All the supported Temperature Sensor SysFS aattributes are
# - temp1_high_crit_threshold
# - temp1_high_threshold
# - temp1_input
# - temp_low_threshold
# - temp1_low_crit_threshold
#############################################################################

try:
    from sonic_platform_base.thermal_base import ThermalBase
    from subprocess import getstatusoutput
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class PddfThermal(ThermalBase):
    """PDDF generic Thermal class"""
    pddf_obj = {}
    plugin_data = {}

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data

        self.platform = self.pddf_obj.get_platform()

        self.thermal_index = index + 1
        self.thermal_obj_name = "TEMP{}".format(self.thermal_index)
        self.thermal_obj = self.pddf_obj.data[self.thermal_obj_name]

        self.is_psu_thermal = is_psu_thermal
        if self.is_psu_thermal:
            self.thermals_psu_index = psu_index

    def get_name(self):
        if self.is_psu_thermal:
            return "PSU{}_TEMP{}".format(self.thermals_psu_index, self.thermal_index)
        else:
            if 'dev_attr' in self.thermal_obj.keys():
                if 'display_name' in self.thermal_obj['dev_attr']:
                    return str(self.thermal_obj['dev_attr']['display_name'])
            # In case of errors
            return (self.thermal_obj_name)

    def get_presence(self):
        if self.is_psu_thermal:
            # Temp sensor on the PSU
            device = "PSU{}".format(self.thermals_psu_index)
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
        else:
            # Temp sensor on the board
            return True

    def get_temperature(self):
        if self.is_psu_thermal:
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_input".format(self.thermal_index))
            if not output:
                return None

            temp1 = output['status']
            # temperature returned is in milli celcius
            return float(temp1)/1000
        else:
            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_input")
            if not output:
                return None

            if output['status'].isalpha():
                attr_value = None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))

    def get_high_threshold(self):
        if not self.is_psu_thermal:
            # Hardcoded values in JSON take precedence over hwmon sysfs values
            if json_value := self.thermal_obj['dev_info'].get('high_threshold'):
                return float(json_value)

            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_high_threshold")
            if not output:
                return None

            if output['status'].isalpha():
                attr_value = None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))
        else:
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_high_threshold".format(self.thermal_index))
            if not output:
                return None

            temp1 = output['status']
            # temperature returned is in milli celcius
            return float(temp1)/1000


    def get_low_threshold(self):
        if not self.is_psu_thermal:
            # Hardcoded values in JSON take precedence over hwmon sysfs values
            if json_value := self.thermal_obj['dev_info'].get('low_threshold'):
                return float(json_value)

            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_low_threshold")
            if not output:
                return None

            if output['status'].isalpha():
                attr_value = None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))
        else:
            raise NotImplementedError

    def set_high_threshold(self, temperature):
        if not self.is_psu_thermal:
            node = self.pddf_obj.get_path(self.thermal_obj_name, "temp1_high_threshold")
            if node is None:
                print("ERROR %s does not exist" % node)
                return None

            cmd = "echo '%d' > %s" % (temperature * 1000, node)
            ret, _ = getstatusoutput(cmd)
            if ret == 0:
                return (True)
            else:
                return (False)
        else:
            raise NotImplementedError

    def set_low_threshold(self, temperature):
        if not self.is_psu_thermal:
            node = self.pddf_obj.get_path(self.thermal_obj_name, "temp1_low_threshold")
            if node is None:
                print("ERROR %s does not exist" % node)
                return None
            cmd = "echo '%d' > %s" % (temperature * 1000, node)
            ret, _ = getstatusoutput(cmd)
            if ret == 0:
                return (True)
            else:
                return (False)
        else:
            raise NotImplementedError

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu_thermal:
            # Hardcoded values in JSON take precedence over hwmon sysfs values
            if json_value := self.thermal_obj['dev_info'].get('high_crit_threshold'):
                return float(json_value)

            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_high_crit_threshold")
            if not output:
                return None

            if output['status'].isalpha():
                attr_value = None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))
        else:
            raise NotImplementedError

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu_thermal:
            # Hardcoded values in JSON take precedence over hwmon sysfs values
            if json_value := self.thermal_obj['dev_info'].get('low_crit_threshold'):
                return float(json_value)

            output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_low_crit_threshold")
            if not output:
                return None

            if output['status'].isalpha():
                attr_value = None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))
        else:
            raise NotImplementedError

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.thermal_index

    def is_replaceable(self):
        """
        Indicate whether Thermal is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        # Usually thermal sensor is not replaceable
        return False

    # Helper Functions
    def get_temp_label(self):
        label = None
        if 'bmc' in self.pddf_obj.data[self.thermal_obj_name].keys():
            return label
        else:
            if self.thermal_obj_name in self.pddf_obj.data.keys():
                dev = self.pddf_obj.data[self.thermal_obj_name]
                if 'i2c' not in dev:
                    label = self.get_name()
                elif 'topo_info' in dev['i2c']:
                    topo_info = dev['i2c']['topo_info']
                    label = "%s-i2c-%d-%x" % (topo_info['dev_type'], int(topo_info['parent_bus'], 0),
                                          int(topo_info['dev_addr'], 0))
                elif 'path_info' in dev['i2c']:
                    label = self.get_name()

            return (label)

    def dump_sysfs(self):
        return self.pddf_obj.cli_dump_dsysfs('temp-sensors')
