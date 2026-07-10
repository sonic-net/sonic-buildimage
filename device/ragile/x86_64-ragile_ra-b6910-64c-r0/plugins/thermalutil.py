import os.path
import sys
import json
sys.path.append('/usr/share/sonic/platform/plugins')
import pddfparse


class ThermalUtil:
    def __init__(self):
        global pddf_obj
        global plugin_data
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)) + '/../pddf/pd-plugin.json')) as pd:
            plugin_data = json.load(pd)

        pddf_obj = pddfparse.PddfParse()
        self.platform = pddf_obj.get_platform()
        self.num_thermals = self.platform['num_temps']
        self.info = []

    def get_num_thermals(self):
        return (self.num_thermals)

    def get_thermal_info(self):
        list = []
        pddf_obj.get_device_list(list, "TEMP_SENSOR")
        list.sort()
        for dev in list:
            data = {}
            device_name = dev['dev_info']['device_name']
            topo_info = dev['i2c']['topo_info']
            label = "%s-i2c-%d-%x" % (topo_info['dev_type'],
                                      int(topo_info['parent_bus'], 0), int(topo_info['dev_addr'], 0))
            attr_list = dev['i2c']['attr_list']
            data['device_name'] = device_name
            data['label'] = label
            for attr in attr_list:
                attr_name = attr['attr_name']
                node = pddf_obj.get_path(device_name, attr_name)
                if node is None:
                    return False
                try:
                    with open(node, 'r') as f:
                        attr_value = int(f.read())
                except IOError:
                    return False
                data[attr_name] = attr_value/float(1000)
            self.info.append(data)

    def show_thermal_temp_values(self, idx):
        if idx < 1 or idx > self.num_thermals:
            print("Invalid temperature sensor idx %d" % idx)
            return None
        self.get_thermal_info()
        thermal_name = "TEMP"+"%d" % idx
        label = ""
        value = ""
        for temp in self.info:
            if thermal_name == temp['device_name']:
                label = temp['label']
                value = "temp1\t %+.1f C (high = %+.1f C, hyst = %+.1f C)" % (
                    temp['temp1_input'], temp['temp1_max'], temp['temp1_max_hyst'])
            else:
                continue

        return (label, value)

    def show_temp_values(self):
        self.get_thermal_info()
        for temp in self.info:
            print(temp['label'])
            print("temp1\t %+.1f C (high = %+.1f C, hyst = %+.1f C)" %
                  (temp['temp1_input'], temp['temp1_max'], temp['temp1_max_hyst']))

    def dump_sysfs(self):
        return pddf_obj.cli_dump_dsysfs('temp-sensors')
