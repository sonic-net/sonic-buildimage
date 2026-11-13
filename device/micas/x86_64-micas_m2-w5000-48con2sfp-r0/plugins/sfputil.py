# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import subprocess
    import re
    import os
    import threading
    if "sonic_sfp_ali" in os.environ and os.environ["sonic_sfp_ali"]:
        from sonic_sfp_ali.sfputilbase import SfpUtilBase
    else:
        from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 49
    PORT_END = 50
    PORTS_IN_BLOCK = 51
    SFP_DEVICE_TYPE = "optoe2"
    QSFP_DEVICE_TYPE = "optoe1"
    I2C_MAX_ATTEMPT = 3

    _port_to_eeprom_mapping = {}
    port_to_i2cbus_mapping ={}
    port_dict = {}
    port_presence_info = {}
    port_txdis_info = {}
    port_protect_info = {}

    hightest_temperature = 0

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def sfp_ports(self):
        return list(range(self.PORT_START, self.PORTS_IN_BLOCK))
    
    @property
    def qsfp_ports(self):
        return []

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        self.port_to_i2cbus_mapping = {49: 7, 50: 6}
        
        self.port_presence_info["/sys/bus/i2c/devices/i2c-5/5-0030/sfp_presence0"] = [49, 50]
        self.port_txdis_info["/sys/bus/i2c/devices/i2c-5/5-0030/sfp_txdis0"] = [49, 50]
        self.port_protect_info["/sys/bus/i2c/devices/i2c-5/5-0030/sfp_protect0"] = [49, 50]

        SfpUtilBase.__init__(self)

    def _write_eeprom_specific_bytes(self, sysfsfile_eeprom, offset, num_bytes, write_buffer):
        try:
            sysfsfile_eeprom.seek(offset)
            sysfsfile_eeprom.write(write_buffer)
        except IOError:
            print("Error: writing EEPROM sysfs file")
            return False

        return True

    def _sfp_read_file_path(self, file_path, offset, num_bytes):
        attempts = 0
        while attempts < self.I2C_MAX_ATTEMPT:
            try:
                file_path.seek(offset)
                read_buf = file_path.read(num_bytes)
            except:
                attempts += 1
                time.sleep(0.05)
            else:
                return True, read_buf
        return False, None

    def _sfp_eeprom_present(self, sysfs_sfp_i2c_client_eeprompath, offset):
        """Tries to read the eeprom file to determine if the
        device/sfp is present or not. If sfp present, the read returns
        valid bytes. If not, read returns error 'Connection timed out"""

        if not os.path.exists(sysfs_sfp_i2c_client_eeprompath):
            return False
        else:
            with open(sysfs_sfp_i2c_client_eeprompath, "rb", buffering=0) as sysfsfile:
                rv, buf = self._sfp_read_file_path(sysfsfile, offset, 1)
                return rv

    def _add_new_sfp_device(self, sysfs_sfp_i2c_adapter_path, devaddr, devtype):
        try:
            sysfs_nd_path = "%s/new_device" % sysfs_sfp_i2c_adapter_path

            # Write device address to new_device file
            nd_file = open(sysfs_nd_path, "w")
            nd_str = "%s %s" % (devtype, hex(devaddr))
            nd_file.write(nd_str)
            nd_file.close()

        except Exception as err:
            print("Error writing to new device file: %s" % str(err))
            return 1
        else:
            return 0

    def _get_port_eeprom_path(self, port_num, devid):
        sysfs_i2c_adapter_base_path = "/sys/class/i2c-adapter"

        if port_num in list(self.port_to_eeprom_mapping.keys()):
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom_mapping[port_num]
        else:
            sysfs_i2c_adapter_base_path = "/sys/class/i2c-adapter"

            i2c_adapter_id = self._get_port_i2c_adapter_id(port_num)
            if i2c_adapter_id is None:
                print("Error getting i2c bus num")
                return None

            # Get i2c virtual bus path for the sfp
            sysfs_sfp_i2c_adapter_path = "%s/i2c-%s" % (sysfs_i2c_adapter_base_path,
                                                        str(i2c_adapter_id))

            # If i2c bus for port does not exist
            if not os.path.exists(sysfs_sfp_i2c_adapter_path):
                print(("Could not find i2c bus %s. Driver not loaded?" % sysfs_sfp_i2c_adapter_path))
                return None

            sysfs_sfp_i2c_client_path = "%s/%s-00%s" % (sysfs_sfp_i2c_adapter_path,
                                                        str(i2c_adapter_id),
                                                        hex(devid)[-2:])

            # If sfp device is not present on bus, Add it
            if not os.path.exists(sysfs_sfp_i2c_client_path):
                if port_num in self.qsfp_ports:
                    ret = self._add_new_sfp_device(
                            sysfs_sfp_i2c_adapter_path, devid, self.QSFP_DEVICE_TYPE)
                else:
                    ret = self._add_new_sfp_device(
                            sysfs_sfp_i2c_adapter_path, devid, self.SFP_DEVICE_TYPE)
                if ret != 0:
                    print("Error adding sfp device")
                    return None

            sysfs_sfp_i2c_client_eeprom_path = "%s/eeprom" % sysfs_sfp_i2c_client_path

        return sysfs_sfp_i2c_client_eeprom_path

    def _read_eeprom_specific_bytes(self, sysfsfile_eeprom, offset, num_bytes):
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        rv, raw = self._sfp_read_file_path(sysfsfile_eeprom, offset, num_bytes)
        if rv == False:
            return None

        try:
            for n in range(0, num_bytes):
                eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
        except:
            return None

        return eeprom_raw

    def get_eeprom_dom_raw(self, port_num):
        if port_num in self.qsfp_ports:
            # QSFP DOM EEPROM is also at addr 0x50 and thus also stored in eeprom_ifraw
            return None
        else:
            # Read dom eeprom at addr 0x51
            return self._read_eeprom_devid(port_num, self.IDENTITY_EEPROM_ADDR, 256)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        presence_path = None
        for presence_key in self.port_presence_info:
            if port_num in self.port_presence_info[presence_key]:
                presence_path = presence_key
                presence_offset = self.port_presence_info[presence_key].index(port_num)
                break
        if presence_path == None:
            return False

        try:
            data = open(presence_path, "rb")
        except IOError:
            return False

        presence_data = data.read(2)
        if presence_data == "":
            return False
        result = int(presence_data, 16)
        data.close()

        # ModPrsL is active low
        if result & (1 << presence_offset) == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num

        return True

    def reset(self, port_num):
        return True

    def _do_write_file(self, file_handle, offset, value):
        file_handle.seek(offset)
        file_handle.write(hex(value))

    def reset_all(self):
        return True

    def get_transceiver_change_event(self):
        return False, {}

    def tx_disable(self, port_num, disable):
        if port_num not in self.sfp_ports :
            return False
        if not self.get_presence(port_num) :
            return False
        
        path = None
        for key in self.port_protect_info:
            if port_num in self.port_protect_info[key]:
                path = key
                break
        if path == None:
            return False
        with open(path, 'rb+') as file:
            data = 0x59
            file.write(hex(data))
        
        path = None
        offset = 0
        data = 0
        for key in self.port_txdis_info:
            if port_num in self.port_txdis_info[key]:
                path = key
                offset = self.port_txdis_info[key].index(port_num)
                break
        if path == None:
            return False
        with open(path, 'rb+') as file:
            data_raw = file.read(2)
            if data_raw == '':
                return False
            
            data_ori = int(data_raw, 16)
            if disable:
                data = data_ori | (1 << offset)
            else:
                data = data_ori & (~(1 << offset))
            
            file.write(hex(data))
        
        path = None
        for key in self.port_protect_info:
            if port_num in self.port_protect_info[key]:
                path = key
                break
        if path == None:
            return False
        with open(path, 'rb+') as file:
            data = 0x4e
            file.write(hex(data))
        
        return True
