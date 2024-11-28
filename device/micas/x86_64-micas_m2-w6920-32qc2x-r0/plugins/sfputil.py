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
    import traceback
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 33
    PORTS_IN_BLOCK = 34

    EEPROM_OFFSET = 63
    SFP_DEVICE_TYPE = "optoe2"
    QSFP_DEVICE_TYPE = "optoe1"
    QSFPDD_DEVICE_TYPE = "optoe3"
    I2C_MAX_ATTEMPT = 3

    _port_to_eeprom_mapping = {}
    port_to_i2cbus_mapping ={}

    sfp_ports_list = []
    qsfp_ports_list = []
    osfp_ports_list = []

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return self.qsfp_ports_list

    @property
    def osfp_ports(self):
        return self.osfp_ports_list

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        self.sfp_ports_list = []
        self.qsfp_ports_list = []
        self.osfp_ports_list = []
        for x in range(self.PORT_START, self.PORTS_IN_BLOCK):
            self.port_to_i2cbus_mapping[x] = (x + self.EEPROM_OFFSET)

        self.port_to_i2cbus_mapping[0] = 65
        self.port_to_i2cbus_mapping[1] = 66
        self.port_to_i2cbus_mapping[2] = 63
        self.port_to_i2cbus_mapping[3] = 64
        self.port_to_i2cbus_mapping[28] = 93
        self.port_to_i2cbus_mapping[29] = 94
        self.port_to_i2cbus_mapping[30] = 91
        self.port_to_i2cbus_mapping[31] = 92
        self.port_to_i2cbus_mapping[32] = 96
        self.port_to_i2cbus_mapping[33] = 95
        
        for x in range(self.PORT_START, self.PORTS_IN_BLOCK):
            ret, sfp_type = self._get_sfp_type(x)
            if (ret == True):
                if sfp_type == "qsfp_dd":
                    self.osfp_ports_list.append(x)
                elif sfp_type == "qsfp":
                    self.qsfp_ports_list.append(x)
                elif sfp_type == "sfp":
                    self.sfp_ports_list.append(x)
        SfpUtilBase.__init__(self)

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

        if port_num in self.port_to_eeprom_mapping.keys():
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
                print("Could not find i2c bus %s. Driver not loaded?" % sysfs_sfp_i2c_adapter_path)
                return None

            sysfs_sfp_i2c_client_path = "%s/%s-00%s" % (sysfs_sfp_i2c_adapter_path,
                                                        str(i2c_adapter_id),
                                                        hex(devid)[-2:])

            # If sfp device is not present on bus, Add it
            if not os.path.exists(sysfs_sfp_i2c_client_path):
                if port_num in self.qsfp_ports:
                    ret = self._add_new_sfp_device(
                            sysfs_sfp_i2c_adapter_path, devid, self.QSFP_DEVICE_TYPE)
                elif port_num in self.osfp_ports:
                    ret = self._add_new_sfp_device(
                            sysfs_sfp_i2c_adapter_path, devid, self.QSFPDD_DEVICE_TYPE)
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
                if isinstance(raw[n], str):
                    eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
                elif isinstance(raw[n], int):
                    eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except Exception as e:
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
        cmd = "cat /sys/wb_plat/sff/sff{}/present".format(str(port_num+1))
        ret, output = subprocess.getstatusoutput(cmd)
        if ret != 0:
            return False
        if output == "1":
            return True
        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num

        return True

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        return True

    def get_transceiver_change_event(self):
        return False, {}

    def _get_sfp_type(self, port_num):
        try:
            if self.get_presence(port_num) == False:
                return False, None

            eeprom_path = self._get_port_eeprom_path(port_num, 0x50)
            with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                eeprom_raw = self._read_eeprom_specific_bytes(eeprom, 0, 1)
                if (eeprom_raw[0] == '1e' or eeprom_raw[0] == '18'):
                    return True, "qsfp_dd"
                elif (eeprom_raw[0] == '11' or eeprom_raw[0] == '0D'):
                    return True, "qsfp"
                elif (eeprom_raw[0] == '03'):
                    return True, "sfp"
        except Exception as e:
            print(traceback.format_exc())

        return False, None

    def twos_comp(self, num, bits):
        try:
            if ((num & (1 << (bits - 1))) != 0):
                num = num - (1 << bits)
            return num
        except Exception:
            return 0

    def get_highest_temperature(self):
        offset = 0
        hightest_temperature = -9999

        presence_flag = False
        read_eeprom_flag = False
        temperature_valid_flag = False

        for port in range(self.PORT_START, self.PORTS_IN_BLOCK):
            if self.get_presence(port) == False:
                continue

            presence_flag = True

            if port in self.osfp_ports:
                offset = 14
            elif port in self.qsfp_ports:
                offset = 22
            else:
                offset = 96

            eeprom_path = self._get_port_eeprom_path(port, 0x50)
            try:
                with open(eeprom_path, mode="rb", buffering=0) as eeprom:
                    read_eeprom_flag = True
                    eeprom_raw = self._read_eeprom_specific_bytes(eeprom, offset, 2)
                    msb = int(eeprom_raw[0], 16)
                    lsb = int(eeprom_raw[1], 16)

                    result = (msb << 8) | (lsb & 0xff)
                    result = self.twos_comp(result, 16)
                    result = float(result / 256.0)
                    if -50 <= result <= 200:
                        temperature_valid_flag = True
                        if hightest_temperature < result:
                            hightest_temperature = result
            except Exception as e:
                ##print(traceback.format_exc())
                pass

        # all port not presence
        if presence_flag == False:
            hightest_temperature = -10000

        # all port read eeprom fail
        elif read_eeprom_flag == False:
            hightest_temperature = -9999

        # all port temperature invalid
        elif read_eeprom_flag == True and temperature_valid_flag == False:
            hightest_temperature = -10000

        hightest_temperature = round(hightest_temperature, 2)

        return hightest_temperature