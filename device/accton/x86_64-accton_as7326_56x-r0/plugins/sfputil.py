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
    PORT_END = 56
    PORTS_IN_BLOCK = 56
    QSFP_PORT_START = 49
    QSFP_PORT_END = 56

    BASE_VAL_PATH = "/sys/class/i2c-adapter/i2c-{0}/{1}-0050/"

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _cpld_mapping = {
        1:  "12-0062",
        2:  "18-0060",
        3:  "19-0064",
    }

    _port_to_i2c_mapping = {
        1:  42,
        2:  41,
        3:  44,
        4:  43,
        5:  47,
        6:  45,
        7:  46,
        8:  50,
        9:  48,
        10:  49,
        11:  52,
        12:  51,
        13:  53,
        14:  56,
        15:  55,
        16:  54,
        17:  58,
        18:  57,
        19:  60,
        20:  59,
        21:  61,
        22:  63,
        23:  62,
        24:  64,
        25:  66,
        26:  68,
        27:  65,
        28:  67,
        29:  69,
        30:  71,
        31:  72,
        32:  70,
        33:  74,
        34:  73,
        35:  76,
        36:  75,
        37:  77,
        38:  79,
        39:  78,
        40:  80,
        41:  81,
        42:  82,
        43:  84,
        44:  85,
        45:  83,
        46:  87,
        47:  88,
        48:  86,
        49:  25,  # QSFP49
        50:  26,
        51:  27,
        52:  28,
        53:  29,
        54:  30,
        55:  31,
        56:  32,  # QSFP56
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

    # For port 0~23 and 48~51 are at cpld2, others are at cpld3.
    def get_cpld_num(self, port_num):
        cpld_i = 1
        if (port_num > 30):
            cpld_i = 2
        return cpld_i

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        cpld_i = self.get_cpld_num(port_num)

        cpld_ps = self._cpld_mapping[cpld_i]
        path = "/sys/bus/i2c/devices/{0}/module_present_{1}"
        port_ps = path.format(cpld_ps, port_num)

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
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        cpld_i = self.get_cpld_num(port_num)
        cpld_ps = self._cpld_mapping[cpld_i]
        path = "/sys/bus/i2c/devices/{0}/module_reset_{1}"
        port_ps = path.format(cpld_ps, port_num)

        self.__port_to_mod_rst = port_ps
        try:
            reg_file = open(self.__port_to_mod_rst, 'r+', buffering=0)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # toggle reset
        reg_file.seek(0)
        reg_file.write('1')
        time.sleep(1)
        reg_file.seek(0)
        reg_file.write('0')
        reg_file.close()

        return True

    @property
    def _get_present_bitmap(self):
        nodes = []
        rev = []
        port_num = [30, 26]

        path = "/sys/bus/i2c/devices/{0}/module_present_all"
        cpld_i = self.get_cpld_num(self.port_start)
        cpld_ps = self._cpld_mapping[cpld_i]
        nodes.append((path.format(cpld_ps), port_num[0]))
        cpld_i = self.get_cpld_num(self.port_end)
        cpld_ps = self._cpld_mapping[cpld_i]
        nodes.append((path.format(cpld_ps), port_num[1]))

        bitmaps = ""
        for node in nodes:
            try:
                reg_file = open(node[0])
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False
            bitmap = reg_file.readline().rstrip()
            bitmap = bin(int(bitmap, 16))[2:].zfill(node[1])
            rev.append(bitmap)
            reg_file.close()

        bitmaps = "".join(rev[::-1])
        bitmaps = hex(int(bitmaps, 2))
        return int(bitmaps, 0)

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
