# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import os
    import string
    from ctypes import create_string_buffer
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

# from xcvrd
SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 32
    PORTS_IN_BLOCK = 32
    QSFP_PORT_START = 1
    QSFP_PORT_END = 32

    I2C_DEV_PATH = "/sys/bus/i2c/devices/"
    BASE_VAL_PATH = "/sys/class/i2c-adapter/i2c-{0}/{1}-0050/"
    BASE_OOM_PATH = "/sys/bus/i2c/devices/{0}-0050/"
    CPLD_ADDRESS = ['-0062', '-0064']

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
        1: [1, 2],
        2: [2, 3],
        3: [3, 4],
        4: [4, 5],
        5: [5, 6],
        6: [6, 7],
        7: [7, 8],
        8: [8, 9],
        9: [9, 10],
        10: [10, 11],
        11: [11, 12],
        12: [12, 13],
        13: [13, 14],
        14: [14, 15],
        15: [15, 16],
        16: [16, 17],
        17: [17, 18],
        18: [18, 19],
        19: [19, 20],
        20: [20, 21],
        21: [21, 22],
        22: [22, 23],
        23: [23, 24],
        24: [24, 25],
        25: [25, 26],
        26: [26, 27],
        27: [27, 28],
        28: [28, 29],
        29: [29, 30],
        30: [30, 31],
        31: [31, 32],
        32: [32, 33],
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_port_start(self):
        return self.QSFP_PORT_START

    @property
    def qsfp_port_end(self):
        return self.QSFP_PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.QSFP_PORT_START, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = self.BASE_OOM_PATH + "eeprom"

        for x in range(self.port_start, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self._port_to_i2c_mapping[x][1]
            )
        SfpUtilBase.__init__(self)

    def get_cpld_dev_path(self, port_num):
        if port_num <= 16:
            cpld_num = 0
        else:
            cpld_num = 1

        # cpld can be at either bus 0 or bus 1.
        cpld_path = self.I2C_DEV_PATH + str(0) + self.CPLD_ADDRESS[cpld_num]
        if not os.path.exists(cpld_path):
            cpld_path = self.I2C_DEV_PATH + str(1) + self.CPLD_ADDRESS[cpld_num]
        return cpld_path

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        cpld_path = self.get_cpld_dev_path(port_num)
        present_path = cpld_path + "/module_present_"
        present_path += str(self._port_to_i2c_mapping[port_num][0])

        self.__port_to_is_present = present_path

        content = "0"
        try:
            val_file = open(self.__port_to_is_present)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

        if content == "1":
            return True

        return False

    def get_low_power_mode_cpld(self, port_num):
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        cpld_path = self.get_cpld_dev_path(port_num)
        _path = cpld_path + "/module_lp_mode_"
        _path += str(self._port_to_i2c_mapping[port_num][0])

        content = "0"
        try:
            reg_file = open(_path)
            content = reg_file.readline().rstrip()
            reg_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        if not self.get_presence(port_num):
            return False

        try:
            eeprom = None

            eeprom = open(self.port_to_eeprom_mapping[port_num], mode="rb", buffering=0)
            eeprom.seek(93)
            lpmode = ord(eeprom.read(1))

            if not (lpmode & 0x1):  # 'Power override' bit is 0
                return self.get_low_power_mode_cpld(port_num)
            else:
                if ((lpmode & 0x2) == 0x2):
                    return True  # Low Power Mode if "Power set" bit is 1
                else:
                    return False  # High Power Mode if "Power set" bit is 0
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        finally:
            if eeprom is not None:
                eeprom.close()
                time.sleep(0.01)

    def set_low_power_mode(self, port_num, lpmode):
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        try:
            eeprom = None

            if not self.get_presence(port_num):
                return False  # Port is not present, unable to set the eeprom

            # Fill in write buffer
            regval = 0x3 if lpmode else 0x1  # 0x3:Low Power Mode, 0x1:High Power Mode
            buffer = create_string_buffer(1)
            buffer[0] = chr(regval)

            # Write to eeprom
            eeprom = open(self.port_to_eeprom_mapping[port_num], mode="r+b", buffering=0)
            eeprom.seek(93)
            eeprom.write(buffer[0])
            return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        finally:
            if eeprom is not None:
                eeprom.close()
                time.sleep(0.01)

    def reset(self, port_num):
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        cpld_path = self.get_cpld_dev_path(port_num)
        _path = cpld_path + "/module_reset_"
        _path += str(self._port_to_i2c_mapping[port_num][0])

        try:
            reg_file = open(_path, 'w')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_file.seek(0)
        reg_file.write('1')
        time.sleep(1)
        reg_file.seek(0)
        reg_file.write('0')
        reg_file.close()

        return True

    @property
    def get_transceiver_status(self):
        nodes = []

        cpld_path = self.get_cpld_dev_path(self.port_start)
        nodes.append(cpld_path + "/module_present_all")
        cpld_path = self.get_cpld_dev_path(self.port_end)
        nodes.append(cpld_path + "/module_present_all")

        bitmap = ""
        for node in nodes:
            try:
                reg_file = open(node)

            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False
            bitmap += reg_file.readline().rstrip() + " "
            reg_file.close()

        rev = bitmap.split(" ")
        rev = "".join(rev[::-1])
        return int(rev, 16)

    data = {'valid': 0, 'last': 0, 'present': 0}

    def get_transceiver_change_event(self, timeout=2000):
        now = time.time()
        port_dict = {}
        port = 0

        if timeout < 1000:
            timeout = 1000
        timeout = (timeout) / float(1000)  # Convert to secs

        if now < (self.data['last'] + timeout) and self.data['valid']:
            return True, {}

        reg_value = self.get_transceiver_status
        changed_ports = self.data['present'] ^ reg_value
        if changed_ports:
            for port in range(self.port_start, self.port_end+1):
                # Mask off the bit corresponding to our port
                fp_port = self._port_to_i2c_mapping[port][0]
                mask = (1 << (fp_port - 1))
                if changed_ports & mask:

                    if (reg_value & mask) == 0:
                        port_dict[port] = SFP_STATUS_REMOVED
                    else:
                        port_dict[port] = SFP_STATUS_INSERTED

            # Update cache
            self.data['present'] = reg_value
            self.data['last'] = now
            self.data['valid'] = 1

            return True, port_dict
        else:
            return True, {}
        return False, {}
