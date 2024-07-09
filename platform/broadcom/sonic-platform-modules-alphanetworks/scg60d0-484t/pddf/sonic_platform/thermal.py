try:
    import os
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")



class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_cpu_temperature(self):
        dev_path = "/sys/devices/platform/coretemp.0/hwmon/"
        hwmon_node = os.listdir(dev_path)[0]
        node = dev_path + hwmon_node + '/temp1_input'
        try:
            with open(node, 'r') as fp:
                temp = float(fp.read()) / 1000
        except IOError:
            return None
        return temp
