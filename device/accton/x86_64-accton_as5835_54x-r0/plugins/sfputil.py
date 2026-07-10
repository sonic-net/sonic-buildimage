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

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 72
    PORTS_IN_BLOCK = 72
    QSFP_PORT_START = 49
    QSFP_PORT_END = 72

    BASE_VAL_PATH = "/sys/class/i2c-adapter/i2c-{0}/{1}-0050/"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _cpld_mapping = {
        0:  "3-0060",
        1:  "3-0061",
        2:  "3-0062",
    }
    _port_to_i2c_mapping = {
        1:  42,
        2:  43,
        3:  44,
        4:  45,
        5:  46,
        6:  47,
        7:  48,
        8:  49,
        9:  50,
        10: 51,
        11: 52,
        12: 53,
        13: 54,
        14: 55,
        15: 56,
        16: 57,
        17: 58,
        18: 59,
        19: 60,
        20: 61,
        21: 62,
        22: 63,
        23: 64,
        24: 65,
        25: 66,
        26: 67,
        27: 68,
        28: 69,
        29: 70,
        30: 71,
        31: 72,
        32: 73,
        33: 74,
        34: 75,
        35: 76,
        36: 77,
        37: 78,
        38: 79,
        39: 80,
        40: 81,
        41: 82,
        42: 83,
        43: 84,
        44: 85,
        45: 86,
        46: 87,
        47: 88,
        48: 89,
        49: 28,  # QSFP49
        50: 28,
        51: 28,
        52: 28,
        53: 29,  # QSFP50
        54: 29,
        55: 29,
        56: 29,
        57: 26,  # QSFP51
        58: 26,
        59: 26,
        60: 26,
        61: 30,  # QSFP52
        62: 30,
        63: 30,
        64: 30,
        65: 31,  # QSFP53
        66: 31,
        67: 31,
        68: 31,
        69: 27,  # QSFP54
        70: 27,
        71: 27,
        72: 27,
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

        SfpUtilBase.__init__(self)

    # For port 49~54 are QSFP, here presumed they're all split to 4 lanes.

    def get_cage_num(self, port_num):
        cage_num = port_num
        if (port_num >= self.QSFP_PORT_START):
            cage_num = (port_num - self.QSFP_PORT_START)/4
            cage_num = int(cage_num + self.QSFP_PORT_START)

        return cage_num

    # For cage 1~38 are at cpld2, others are at cpld3.
    def get_cpld_num(self, port_num):
        return 1 if (port_num < 39) else 2

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        cage_num = self.get_cage_num(port_num)
        cpld_i = self.get_cpld_num(port_num)

        cpld_ps = self._cpld_mapping[cpld_i]
        path = "/sys/bus/i2c/devices/{0}/module_present_{1}"
        port_ps = path.format(cpld_ps, cage_num)

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

    def get_low_power_mode_cpld(self, port_num):
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        cage_num = self.get_cage_num(port_num)
        cpld_i = self.get_cpld_num(port_num)

        cpld_ps = self._cpld_mapping[cpld_i]
        path = "/sys/bus/i2c/devices/{0}/module_lpmode_{1}"
        lp_mode_path = path.format(cpld_ps, cage_num)

        try:
            val_file = open(lp_mode_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = val_file.readline().rstrip()
        val_file.close()

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

        cage_num = self.get_cage_num(port_num)
        cpld_i = self.get_cpld_num(port_num)
        cpld_ps = self._cpld_mapping[cpld_i]
        path = "/sys/bus/i2c/devices/{0}/module_reset_{1}"
        port_ps = path.format(cpld_ps, cage_num)
        try:
            reg_file = open(port_ps, mode='w', buffering=0)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # toggle reset
        reg_file.seek(0)
        reg_file.write('0')
        time.sleep(1)
        reg_file.seek(0)
        reg_file.write('1')
        reg_file.close()

        return True

    @property
    def _get_presence_bitmap(self):
        nodes = []
        nodes.append("/sys/bus/i2c/devices/3-0061/module_present_all")
        nodes.append("/sys/bus/i2c/devices/3-0062/module_present_all")

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
        rev.pop()  # Remove the last useless character

        # Convert bitmap into continuously port order
        rev[4] = hex((int(rev[4], 16) | ((int(rev[5], 16) & 0x3) << 6)))[2:]      # Port 33-40
        rev[5] = hex((int(rev[5], 16) >> 2) | ((int(rev[6], 16) & 0x3) << 6))[2:]  # Port 41-48

        # Expand port 49-54
        tmp = rev.pop()
        for i in range(2, 8):
            val = (int(tmp, 16) >> i) & 0x1
            rev.append(hex(val)[2:])

        for i in range(0, 6):
            rev[i] = rev[i].zfill(2)

        rev = "".join(rev[::-1])
        return int(rev, 16)

    data = {'valid': 0, 'present': 0}

    def get_transceiver_change_event(self, timeout=0):

        start_time = time.time()
        port_dict = {}
        port = 0
        blocking = False

        if timeout == 0:
            blocking = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
        else:
            print("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print('get_transceiver_change_event:'
                  'time wrap / invalid timeout value', timeout)

            return False, {}  # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            # Check for OIR events and return updated port_dict

            reg_value = self._get_presence_bitmap
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
                self.data['valid'] = 1
                return True, port_dict

            if blocking:
                time.sleep(1)
            else:
                timeout = end_time - time.time()
                if timeout >= 1:
                    time.sleep(1)  # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, {}
        print("get_transceiver_change_event: Should not reach here.")
        return False, {}
