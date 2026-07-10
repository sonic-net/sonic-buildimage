#!/usr/bin/env python

class PsuFru:
    """PSU FRU class"""
    eeprom = ""
    mfr_id = "not available"
    model = "not available"
    serial = "not available"
        
    def __init__(self, psu_index):
        self.psu_index = psu_index
        self.eeprom = "/sys/bus/i2c/devices/2-00{}/eeprom".format(49 + psu_index)
        self._parse_fru_eeprom()

    def _parse_fru_eeprom(self):
        """
        Parsing eeprom fru content of PSU
        """
        try:
            with open(self.eeprom, 'rb') as eeprom:
                data = eeprom.read()
                 
                # check if dummy content
                if data[0] == 0xff:
                    return
                     
                i = 11

                data_len = (data[i]&0x3f)
                i += 1
                self.mfr_id = data[i:i+data_len].decode('utf-8')
                i += data_len

                data_len = (data[i]&0x3f)
                i += 1
                i += data_len

                data_len = (data[i]&0x3f)
                i += 1
                self.model = data[i:i+data_len].decode('utf-8')
                i += data_len

                data_len = (data[i]&0x3f)
                i += 1
                i += data_len

                data_len = (data[i]&0x3f)
                i += 1
                self.serial = data[i:i+data_len].decode('utf-8')
        except Exception as e:
            return

