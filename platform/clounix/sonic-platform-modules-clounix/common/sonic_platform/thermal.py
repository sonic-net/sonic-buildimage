#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
    from subprocess import getstatusoutput
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

core_temp_path = "/sys/devices/platform/*coretemp*/hwmon/hwmon*/"
fpga_pvt_temp_path = "/sys/bus/pci/devices/0000*/"
# (X = PVT RAW DATA OUT)
#step 1 Untrimmed T = -3.5451E-13*(X^4) + 4.0015E-9*(X^3) - 2.0541E-5*(X^2) + 9.5040E-2*(X^1) - 5.5733E+1
#step 2 K(T) = (0.00021375*T^2) + (0.035818*T) + (2.5039)
#step 3 3.Trimmed DATA_OUT=PVT RAW DATA OUT +(TRIMG-15)*K(T) ±TRIMO
#(Note: TRIMO < 128: Trimmed DATA_OUT = PVT RAW DATA OUT + (TRIMG-15) * K(T) -TRIMO; 
#TRIMO >= 128: Trimmed DATA_OUT = PVT RAW DATA OUT + (TRIMG-15) * K(T) +(TRIMO-128))
#Trim temperature = -3.5451E-13*Y^4+4.0015E-99*Y^3-2.0541E-5*Y^2+9.5040E-2*Y-5.5733E+1 (Y = Trimmed DATA_OUT)
#default value :
#
#TRIMG 16
#TRIMO 0
def calculate_fpga_pvt_temperature(value):
    TRIMO = 0
    TRIMG = 15
    untrimmed_t = -3.5451E-13*pow(value,4) + 4.0015E-9*pow(value,3) - 2.0541E-5*pow(value,2) + 9.5040E-2*(value) - 55.733
    k_t =  (0.00021375*pow(untrimmed_t,2)) + (0.035818*untrimmed_t) + (2.5039)  
    if TRIMO >=128:
        trimmed_outdata = value + (TRIMG-15) * k_t +(TRIMO-128)
    else:
        trimmed_out_data = value + (TRIMG-15) * k_t - TRIMO
    trimmed_t = -3.5451E-13*pow(trimmed_out_data,4)+4.0015E-09*pow(trimmed_out_data,3)-2.0541E-05*pow(trimmed_out_data,2)+9.5040E-02*trimmed_out_data-55.733
    return round(trimmed_t,1)

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
                return calculate_fpga_pvt_temperature(int(output['status']))
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
            device = "PSU{}".format(self.thermals_psu_index)
            output = self.pddf_obj.get_attr_name_output(device, "psu_temp{}_high_crit_threshold".format(self.thermal_index))
            if not output:
                return None

            temp1 = output['status']
            # temperature returned is in milli celcius
            return float(temp1)/1000
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
                return "PSU{} Ambient".format(self.thermals_psu_index)
            elif self.thermal_index == 2:
                return "PSU{} SR Hotspot".format(self.thermals_psu_index)
            elif self.thermal_index == 3:
                return "PSU{} PFC Hotspot".format(self.thermals_psu_index)
            else:
                return "PSU{} temp{}".format(self.thermals_psu_index, self.thermal_index)
        elif self.is_vc_sensor_thermal:
            return "{} temp{}".format(self.thermal_obj['dev_attr']['display_name'], self.thermal_index)
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
