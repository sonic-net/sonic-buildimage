#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from subprocess import getstatusoutput
    from . import pvt_temp
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

core_temp_path = "/sys/devices/platform/*coretemp*/hwmon/hwmon*/"
fpga_pvt_temp_path = "/sys/bus/pci/devices/0000*/"

class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)
        
        self.is_vc_sensor_thermal = False
        self.is_core_thermal = False
        self.is_fpga_pvt_thermal = False
        
    # Provide the functions/variables below for which implementation is to be overwritten
    def get_temperature(self):
        output = {"mode": "", "status": ""}
        if self.is_psu_thermal:
            if not self.get_presence():
                return 0.0
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_input".format(self.thermal_index))
            if not output:
                return None

            temp1 = output['status']
            # temperature returned is in milli celcius
            return float(temp1)/1000
        else:
            if self.is_vc_sensor_thermal:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp{}_input".format(self.thermal_index))
            elif self.is_core_thermal:
                cmd = "cat " + core_temp_path + "temp{}_input".format(self.thermal_index)
                output['mode'], output['status'] = getstatusoutput(cmd)
            elif self.is_fpga_pvt_thermal:
                cmd = "cat " + fpga_pvt_temp_path + "pvt_temp{}_input".format(self.thermal_index)
                output['mode'], output['status'] = getstatusoutput(cmd)
                return pvt_temp.calculate_fpga_pvt_temperature(int(output['status']), self.pvt_chip_type)
            else:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_input")
            if not output:
                return None

            if output['status'].isalpha():
                return None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))
            
    def get_high_threshold(self):
        output = {"mode": "", "status": ""}
        if self.is_psu_thermal:
            if not self.get_presence():
                return 0.0
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_high_threshold".format(self.thermal_index))
            if not output:
                return None

            temp = output['status']
            # temperature returned is in milli celcius
            return float(temp)/1000
        else:
            if self.is_vc_sensor_thermal:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp{}_max".format(self.thermal_index))
            elif self.is_core_thermal:
                cmd = "cat " + core_temp_path + "temp{}_max".format(self.thermal_index)
                output['mode'], output['status'] = getstatusoutput(cmd)
            elif self.is_fpga_pvt_thermal:
                cmd = "cat " + fpga_pvt_temp_path + "pvt_temp{}_max".format(self.thermal_index)
                output['mode'], output['status'] = getstatusoutput(cmd)
            else:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_high_threshold")
            if not output:
                return None

            if output['status'].isalpha():
                return None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))

    def get_low_threshold(self):
        output = {"mode": "", "status": ""}
        
        if not self.is_psu_thermal:
            if self.is_vc_sensor_thermal:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp{}_min".format(self.thermal_index))
            elif self.is_core_thermal or self.is_fpga_pvt_thermal:
                output = None
            else:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_low_threshold")
            if not output:
                # return None
                return 0.001

            if output['status'].isalpha():
                return None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))
        else:
            # raise NotImplementedError
            return 0.001
    
    def set_high_threshold(self, temperature):
        if not self.is_psu_thermal:
            if self.is_vc_sensor_thermal:
                node = self.pddf_obj.get_path(self.thermal_obj_name, "temp{}_max".format(self.thermal_index))
            elif self.is_core_thermal:
                node = core_temp_path + "temp{}_max".format(self.thermal_index) 
            elif self.is_fpga_pvt_thermal:
                node = fpga_pvt_temp_path + "pvt_temp{}_max".format(self.thermal_index)                               
            else:
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
            if self.is_vc_sensor_thermal:
                node = self.pddf_obj.get_path(self.thermal_obj_name, "temp{}_min".format(self.thermal_index))
            elif self.is_core_thermal or self.is_fpga_pvt_thermal:
                node = None
            else:
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
        output = {"mode": "", "status": ""}
        if self.is_psu_thermal:
            if not self.get_presence():
                return 0.0
            gain_factor = 0.0
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_high_crit_threshold".format(self.thermal_index))
            if not output:
                output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_high_threshold".format(self.thermal_index))
                if not output:
                    return None
                gain_factor = 5.0

            temp1 = output['status']
            # temperature returned is in milli celcius
            return float(temp1)/1000 + gain_factor
        else:
            if self.is_vc_sensor_thermal:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp{}_crit".format(self.thermal_index))
            elif self.is_core_thermal:
                cmd = "cat " + core_temp_path + "temp{}_crit".format(self.thermal_index)
                output['mode'], output['status'] = getstatusoutput(cmd)
            elif self.is_fpga_pvt_thermal:
                cmd ="cat " + fpga_pvt_temp_path + "pvt_temp{}_crit".format(self.thermal_index)                               
                output['mode'], output['status'] = getstatusoutput(cmd)
            else:
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

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        output = {"mode": "", "status": ""}
        if not self.is_psu_thermal:
            if self.is_vc_sensor_thermal:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp{}_lcrit".format(self.thermal_index))
            elif self.is_core_thermal or self.is_fpga_pvt_thermal:
                output = None
            else:
                output = self.pddf_obj.get_attr_name_output(self.thermal_obj_name, "temp1_low_crit_threshold")
            
            if not output:
                # return None
                return 0.001

            if output['status'].isalpha():
                return None
            else:
                attr_value = float(output['status'])

            if output['mode'] == 'bmc':
                return attr_value
            else:
                return (attr_value/float(1000))
        else:
            # raise NotImplementedError
            return 0.001
    
    def get_name(self):
        if self.is_psu_thermal:
            if self.thermal_index == 1:
                return "PSU{}_Ambient".format(self.thermals_psu_index)
            elif self.thermal_index == 2:
                return "PSU{}_SR_Hotspot".format(self.thermals_psu_index)
            elif self.thermal_index == 3:
                return "PSU{}_PFC_Hotspot".format(self.thermals_psu_index)
            else:
                return "PSU{}_temp{}".format(self.thermals_psu_index, self.thermal_index)
        elif self.is_vc_sensor_thermal:
            return "{}_temp{}".format(self.thermal_obj['dev_attr']['display_name'], self.thermal_index)
        elif self.is_core_thermal or self.is_fpga_pvt_thermal:
            return self.thermal_obj_name
        else:
            if 'dev_attr' in self.thermal_obj.keys():
                if 'display_name' in self.thermal_obj['dev_attr']:
                    return str(self.thermal_obj['dev_attr']['display_name'])
            # In case of errors
            return (self.thermal_obj_name)
    
    # Helper Functions
    def get_temp_label(self):
        label = None
        if self.thermal_obj and 'bmc' in self.pddf_obj.data[self.thermal_obj_name].keys():
            return label
        else:
            if self.thermal_obj_name in self.pddf_obj.data.keys():
                dev = self.pddf_obj.data[self.thermal_obj_name]
                if 'topo_info' in dev['i2c']:
                    topo_info = dev['i2c']['topo_info']
                    label = "%s-i2c-%d-%x" % (topo_info['dev_type'], int(topo_info['parent_bus'], 0),
                                          int(topo_info['dev_addr'], 0))
                elif 'path_info' in dev['i2c']:
                    label = self.get_name()
            elif self.is_core_thermal:
                cmd = "cat " + core_temp_path + "temp{}_label".format(self.thermal_index)
                _, label = getstatusoutput(cmd)
            elif self.is_fpga_pvt_thermal:
                cmd = "cat " + fpga_pvt_temp_path + "pvt_temp{}_label".format(self.thermal_index)
                _, label = getstatusoutput(cmd)
            return (label)
