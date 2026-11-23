#!/usr/bin/env python


try:
    from sonic_platform_pddf_base.pddf_fan_drawer import PddfFanDrawer
    from .fru import ipmifru
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(PddfFanDrawer):
    """PDDF Platform-Specific Fan-Drawer class"""

    def __init__(self, tray_idx, pddf_data=None, pddf_plugin_data=None):
        # idx is 0-based 
        PddfFanDrawer.__init__(self, tray_idx, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten
    @staticmethod
    def byteTostr(val):
        strtmp = ''
        for value in val:
            strtmp += chr(value)
        return strtmp

    def read_fan_eeprom(self):
        eeprom_data = []
        try:
            index = self.fantray_index - 1 + 70
            eeprom_path = "/sys/bus/i2c/devices/{}-0057/eeprom".format(index)
            with open(eeprom_path, "rb") as f:
                eeprom_data = f.read()
        except FileNotFoundError:
            print(f"Error: EEPROM file not found at {eeprom_path}")
        except Exception as e:
            print(f"An error occurred: {e}")
        return eeprom_data

    def get_fru_info(self):
        try:
            if self.get_presence() is False:
                raise Exception("%s: not present" % self.name)
            eeprom = self.read_fan_eeprom()
            if eeprom is None:
                raise Exception("%s: value is none" % self.name)
            fru = ipmifru()
            if isinstance(eeprom, bytes):
                eeprom = self.byteTostr(eeprom)
            fru.decodeBin(eeprom)
            self.boardProductName = fru.boardInfoArea.boardProductName.strip()  # PN
            self.boardSerialNumber = fru.boardInfoArea.boardSerialNumber.strip()  # SN
            self.boardPartNumber = fru.boardInfoArea.boardPartNumber.strip()  # HW
        except Exception:
            self.boardProductName = None
            self.boardSerialNumber = None
            self.boardPartNumber = None
            return False
        return True

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            string: Part number of FAN
        """
        if self.get_fru_info():
            return self.boardPartNumber
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            string: Serial number of FAN
        """
        if self.get_fru_info():
            return self.boardSerialNumber
        return 'N/A'
