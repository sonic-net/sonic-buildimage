try:
    from sonic_platform_pddf_base.pddf_thermal import PddfThermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")



class Thermal(PddfThermal):
    """PDDF Platform-Specific Thermal class"""

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None, is_psu_thermal=False, psu_index=0):
        PddfThermal.__init__(self, index, pddf_data, pddf_plugin_data, is_psu_thermal, psu_index) 
    # Provide the functions/variables below for which implementation is to be overwritten

    def get_name(self):
        thermal_name = None
        if self.is_psu_thermal and "thermal_name" in self.plugin_data['PSU']:
            thermal_name = self.plugin_data['PSU']['thermal_name'][str(self.thermals_psu_index)]
        return super().get_name() if thermal_name is None else thermal_name
