
try:
    from sonic_platform_base.sensor_base import VoltageSensorBase, SensorBase
    from subprocess import getstatusoutput
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class PddfVoltageSensor(VoltageSensorBase):
    """PDDF generic Voltage Sensor class"""
    pddf_obj = {}
    plugin_data = {}

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data
        self.sensor_index = index + 1
        
        self.sensor_obj_name = f"VOLTAGE{self.sensor_index}"
        if self.sensor_obj_name not in self.pddf_obj.data:
            raise ValueError(f"Voltage sensor {self.sensor_obj_name} not found in PDDF data")

        self.sensor_obj = self.pddf_obj.data[self.sensor_obj_name]
        self.display_name = self.sensor_obj.get('dev_attr', {}).get('display_name', self.sensor_obj_name)
        

    def get_name(self):
        if 'dev_attr' in self.sensor_obj.keys():
            if 'display_name' in self.sensor_obj['dev_attr']:
                return str(self.sensor_obj['dev_attr']['display_name'])
        # In case of errors
        return (self.sensor_obj_name)
    
    def get_value_all(self, attr_name):
        output = self.pddf_obj.get_attr_name_output(self.sensor_obj_name, attr_name)
        if not output:
            return None

        if output['status'].isalpha():
            return None
        else:
            attr_value = float(output['status']) * self.sensor_obj.get('dev_attr', {}).get('scaling_factor', 1.0)
            return float(round(attr_value))

    def get_value(self):
        return self.get_value_all("volt1_input")

    def get_low_threshold(self):
        return self.get_value_all("volt1_low_threshold")

    def get_high_threshold(self):
        return self.get_value_all("volt1_high_threshold")
    
    def get_high_critical_threshold(self):
        return self.get_value_all("volt1_crit_high_threshold")
    
    def get_low_critical_threshold(self):
        return self.get_value_all("volt1_crit_low_threshold")