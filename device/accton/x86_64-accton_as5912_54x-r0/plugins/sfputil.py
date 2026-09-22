# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import string
    import pprint
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
    PORT_END = 27
    PORTS_IN_BLOCK = 27
    QSFP_PORT_START = 25
    QSFP_PORT_END = 27

    BASE_VAL_PATH = "/sys/class/i2c-adapter/i2c-{0}/{1}-0050/"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _cpld_mapping = ["8-0063", "7-0064"]

    _port_to_i2c_mapping = {
        1:  26,
        2:  27,
        3:  28,
        4:  29,
        5:  30,
        6:  31,
        7:  32,
        8:  33,
        9:  34,
        10: 35,
        11: 36,
        12: 37,
        13: 38,
        14: 39,
        15: 40,
        16: 41,
        17: 42,
        18: 43,
        19: 44,
        20: 45,
        21: 46,
        22: 47,
        23: 48,
        24: 49,
        25: 21,  # QSFP
        26: 22,
        27: 23,
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
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/eeprom'
        for x in range(self.port_start, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self._port_to_i2c_mapping[x])

        self.get_transceiver_change_event()
        SfpUtilBase.__init__(self)

    def get_cpld_num(self, port_num):
        cpld_i = 0
        if (port_num >= self.qsfp_port_start):
            cpld_i = 1

        return cpld_i

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        cpld_i = self.get_cpld_num(port_num)
        cpld_ps = self._cpld_mapping[cpld_i]
        path = "/sys/bus/i2c/devices/{0}/present_{1}"
        index = ((port_num-1) % 24) + 1
        port_ps = path.format(cpld_ps, index)

        content = "0"
        try:
            val_file = open(port_ps)
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
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
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
        raise NotImplementedError

    @property
    def _get_present_bitmap(self):
        nodes = []
        port_num = [24, 3]

        path = "/sys/bus/i2c/devices/{0}/"
        cpld_path = path.format(self._cpld_mapping[0])
        nodes.append((cpld_path + "module_present_all", port_num[0]))
        cpld_path = path.format(self._cpld_mapping[1])
        nodes.append((cpld_path + "module_present_all", port_num[1]))

        bitmap = []
        for node in nodes:
            try:
                reg_file = open(node[0])

            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False
            cpld_bm = reg_file.readline().rstrip().zfill(node[1]/4)
            bitmap.append(cpld_bm)
            reg_file.close()

        rev = "".join(bitmap[::-1])
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
            pprint.pprint(port_dict)
            return True, port_dict
        else:
            return True, {}
        return False, {}
