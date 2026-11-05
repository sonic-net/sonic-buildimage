#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from swsscommon.swsscommon import SonicV2Connector
    from sonic_py_common.general import getstatusoutput_noshell_pipe
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Fan(PddfFan):
    """PDDF Platform-Specific Fan class"""

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        # idx is 0-based
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)

    # Provide the functions/variables below for which implementation is to be overwritten e.g.
    #def get_name(self):
        ## Since AS7712 has two fans in a tray, modifying this function to return proper name
        #if self.is_psu_fan:
            #return "PSU_FAN{}".format(self.fan_index)
        #else:
            #return "Fantray{}_{}".format(self.fantray_index, {1:'Front', 2:'Rear'}.get(self.fan_index,'none'))

    def get_direction(self):
        """
        Retrieves the direction of fan

        Returns:
            A string, either FAN_DIRECTION_INTAKE or FAN_DIRECTION_EXHAUST
            depending on fan direction
        """
        # read part number from eeprom
        # psu fan follows the same rule
        db = SonicV2Connector()
        db.connect(db.STATE_DB)
        eeprom_table = db.get_all(db.STATE_DB, 'EEPROM_INFO|0x22')
        if "Name" in eeprom_table and eeprom_table["Name"] == "Part Number" and "Value" in eeprom_table:
            part_number = eeprom_table["Value"]
        else:
            _, output = getstatusoutput_noshell_pipe(["sudo", "decode-syseeprom"], ["grep", "'Part Number'"], ["grep", "-oE", "'[^ ]+$'"])
            part_number = output.strip()

        if "T8196SR" in part_number:
            return self.FAN_DIRECTION_INTAKE
        else:
            return self.FAN_DIRECTION_EXHAUST
