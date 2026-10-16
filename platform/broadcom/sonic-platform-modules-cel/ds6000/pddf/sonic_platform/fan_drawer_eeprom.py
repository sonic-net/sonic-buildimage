try:
    from .ipmi_fru import IpmiFruParser
    import os
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class FanDrawerEeprom(object):

    def __init__(self, pddf_data=None, pddf_plugin_data=None, tray_idx=0):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data
        self.fantray_index = tray_idx + 1
        device_name = "FANTRAY{}_EEPROM".format(self.fantray_index)
        # system EEPROM always has device name EEPROM1
        self.eeprom_path = self.pddf_obj.get_path(device_name, "eeprom")
        if self.eeprom_path is None:
            return
        self.eeprom_tlv_dict = {}
        try:
            self.parser = IpmiFruParser()
            self.parser.load_from_file(self.eeprom_path)
            self.parser.parse()
            self.eeprom_tlv_dict = self.parser.result_dict
        except:
            return

    # Provide the functions/variables below for which implementation is to be overwritten
    def mfr_str(self):
        try:
            result = self.eeprom_tlv_dict["board_info"]["manufacturer"]
            return result
        except:
            return "N/A"
    
    def serial_str(self):
        try:
            result = self.eeprom_tlv_dict["board_info"]["serial_number"]
            return result
        except:
            return "N/A"
     
    def part_number_str(self):
        try:
            result = self.eeprom_tlv_dict["board_info"]["part_number"]
            return result
        except:
            return "N/A"
    
    def fru_fileid_str(self):
        try:
            result = self.eeprom_tlv_dict["board_info"]["fru_file_id"]
            return result
        except:
            return "N/A"
    
    def pcb_ver_str(self):
        try:
            result = self.eeprom_tlv_dict["board_info"]["custom_fields"]["PCBA Version"]
            return result
        except:
            return "N/A"
    
    def dir_str(self):
        try:
            result = self.eeprom_tlv_dict["board_info"]["custom_fields"]["Airflow"]
            return result
        except:
            return "N/A"

