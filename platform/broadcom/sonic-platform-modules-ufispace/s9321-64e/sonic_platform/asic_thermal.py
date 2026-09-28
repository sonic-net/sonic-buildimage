#!/usr/bin/env python
try:
    from sonic_platform_pddf_base.pddf_asic_thermal import PddfAsicThermal
    from swsscommon.swsscommon import SonicV2Connector
    import subprocess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class AsicThermal(PddfAsicThermal):
    """PDDF Platform-Specific ASIC Thermal class"""

    def __init__(self, index, position_offset, pddf_data=None):
        PddfAsicThermal.__init__(self, index, position_offset, pddf_data)
        thermal_obj = pddf_data.data[self.get_thermal_obj_name()]
        self.table_index = thermal_obj['dev_attr']['table_index']

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_temperature(self):
        try:
            db = SonicV2Connector()
            db.connect(db.STATE_DB)
            data_dict = db.get_all(db.STATE_DB, self.ASIC_TEMP_INFO)
            return float(data_dict[self.table_index])
        except:
            return 0.0