#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the sfp management function
#
#############################################################################

try:
    import time
    from multiprocessing import Lock

    from sonic_platform.cls_sfp import ClsPddfSfp
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


class Sfp(ClsPddfSfp):
    """
    PDDF Platform-Specific Sfp class
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        ClsPddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)
        self.eeprom_lock = Lock()

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_device_type(self):
        device = 'PORT{}'.format(self.port_index)
        return self.pddf_obj.get_device_type(device)

    def get_lpmode(self):
        device = 'PORT{}'.format(self.port_index)
        if self.pddf_obj.get_device_type(device).startswith('SFP'):
            return super().get_tx_disable()
        else:
            return super().get_lpmode()

    def set_lpmode(self, lpmode):
        device = 'PORT{}'.format(self.port_index)
        if self.pddf_obj.get_device_type(device).startswith('SFP'):
            if lpmode == False:
                super().tx_disable(True)
                time.sleep(0.1)
            return super().tx_disable(lpmode)
        else:
            if lpmode == False:
                super().set_lpmode(True)
                time.sleep(0.1)
            return super().set_lpmode(lpmode)

    # Provide the functions/variables below for which implementation is to be overwritten
    # Add reties to work around FPGAPCI 0050/eeprom: offset 0x0: sometimes read failed
    def __read_eeprom(self, offset, num_bytes):
        """
        read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Args:
            offset :
                Integer, the offset from which the read transaction will start
            num_bytes:
                Integer, the number of bytes to be read

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        buf = None
        eeprom_raw = []
        sysfs_sfp_i2c_client_eeprom_path = self.eeprom_path

        if not self.get_presence():
            return None

        sysfsfile_eeprom = None
        attempts = 0
        max_retries = 5
        success = False
        while attempts < max_retries and not success:
            try:
                if attempts > 0:
                    time.sleep(0.2)
                sysfsfile_eeprom = open(sysfs_sfp_i2c_client_eeprom_path, "rb", 0)
                sysfsfile_eeprom.seek(offset)
                buf = sysfsfile_eeprom.read(num_bytes)
                success = True
            except Exception as ex:
                attempts += 1
                if attempts == max_retries:
                   return None
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()

        if buf is None:
            return None

        for x in buf:
            eeprom_raw.append(x)

        while len(eeprom_raw) < num_bytes:
            eeprom_raw.append(0)
        return bytes(eeprom_raw)

    # Read out any bytes from any offset
    def read_eeprom(self, offset, num_bytes):
        """
        read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be read

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        self.eeprom_lock.acquire()
        bytes = self.__read_eeprom(offset, num_bytes)
        self.eeprom_lock.release()
        return bytes

    def get_port_or_cage_type(self):
        if self.port_index >= 1 and self.port_index <= 64:
            return self.SFP_CAGE_TYPE_OSFP
        elif self.port_index == 65 or self.port_index == 66:
            return self.SFP_CAGE_TYPE_SFP
        else:
            return "N/A"
