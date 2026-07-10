# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import string
    from ctypes import create_string_buffer
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))
# from xcvrd
SFP_STATUS_REMOVED = '0'
SFP_STATUS_INSERTED = '1'


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 32
    PORTS_IN_BLOCK = 32

    BASE_OOM_PATH = "/sys/bus/i2c/devices/{0}-0050/"
    BASE_CPLD_PATH = "/sys/bus/i2c/devices/11-0060/"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
        1: 29,
        2: 30,
        3: 31,
        4: 32,
        5: 34,
        6: 33,
        7: 36,
        8: 35,
        9: 25,
        10: 26,
        11: 27,
        12: 28,
        13: 37,
        14: 38,
        15: 39,
        16: 40,
        17: 41,
        18: 42,
        19: 43,
        20: 44,
        21: 53,
        22: 54,
        23: 55,
        24: 56,
        25: 45,
        26: 46,
        27: 47,
        28: 48,
        29: 49,
        30: 50,
        31: 51,
        32: 52,
    }

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.PORT_START, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = self.BASE_OOM_PATH + "eeprom"

        for x in range(self.port_start, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self._port_to_i2c_mapping[x]
            )
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        present_path = self.BASE_CPLD_PATH + "module_present_" + str(port_num)
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

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            eeprom = None

            if not self.get_presence(port_num):
                return False

            eeprom = open(self.port_to_eeprom_mapping[port_num], "rb")
            eeprom.seek(93)
            lpmode = ord(eeprom.read(1))

            if ((lpmode & 0x3) == 0x3):
                return True  # Low Power Mode if "Power override" bit is 1 and "Power set" bit is 1
            else:
                # High Power Mode if one of the following conditions is matched:
                # 1. "Power override" bit is 0
                # 2. "Power override" bit is 1 and "Power set" bit is 0
                return False
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        finally:
            if eeprom is not None:
                eeprom.close()
                time.sleep(0.01)

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
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
            eeprom = open(self.port_to_eeprom_mapping[port_num], "r+b")
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
        if port_num < self.port_start or port_num > self.port_end:
            return False

        mod_rst_path = self.BASE_CPLD_PATH + "module_reset_" + str(port_num)
        self.__port_to_mod_rst = mod_rst_path
        try:
            reg_file = open(self.__port_to_mod_rst, 'r+')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = '1'

        reg_file.write(reg_value)
        reg_file.close()

        return True

    @property
    def _get_present_bitmap(self):
        nodes = []

        nodes.append(self.BASE_CPLD_PATH + "module_present_all")
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

        reg_value = self._get_present_bitmap
        changed_ports = self.data['present'] ^ reg_value
        if changed_ports:
            for port in range(self.port_start, self.port_end+1):
                # Mask off the bit corresponding to our port
                mask = (1 << (port - 1))
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
