# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import sys
    import time
    from ctypes import create_string_buffer
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 55
    QSFP_PORT_START = 1
    QSFP_PORT_END = 53
    CPLD_FMT = '/sys/bus/i2c/devices/{bus}-00{addr}/'

    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
           1: 33,
           2: 34,
           3: 37,
           4: 38,
           5: 41,
           6: 42,
           7: 45,
           8: 46,
           9: 49,
           10: 50,
           11: 53,
           12: 54,
           13: 57,
           14: 58,
           15: 61,
           16: 62,
           17: 65,
           18: 66,
           19: 69,
           20: 70,
           21: 35,
           22: 36,
           23: 39,
           24: 40,
           25: 43,
           26: 44,
           27: 47,
           28: 48,
           29: 51,
           30: 52,
           31: 55,
           32: 56,
           33: 59,
           34: 60,
           35: 63,
           36: 64,
           37: 67,
           38: 68,
           39: 71,
           40: 72,
           41: 93,
           42: 84,
           43: 83,
           44: 82,
           45: 81,
           46: 86,
           47: 85,
           48: 88,
           49: 87,
           50: 90,
           51: 89,
           52: 92,
           53: 91,
           54: 30,
           55: 31
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
        return range(self.QSFP_PORT_START, self.QSFP_PORT_END + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        eeprom_path = '/sys/bus/i2c/devices/{0}-0050/eeprom'
        for x in range(self.port_start, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self._port_to_i2c_mapping[x])

        SfpUtilBase.__init__(self)

    def get_cpld_path(self, port_num):
        if ((1 <= port_num <= 10) or (21 <= port_num <= 30) or (54 <= port_num <= 55)):
            return self.CPLD_FMT.format(bus=str(12), addr=str(62))
        elif ((11 <= port_num <= 20) or (31 <= port_num <= 40)):
            return self.CPLD_FMT.format(bus=str(13), addr=str(63))
        elif (41 <= port_num <= 53):
            return self.CPLD_FMT.format(bus=str(76), addr=str(64))

        return str()

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        port_ps = self.get_cpld_path(port_num) + 'module_present_{0}'.format(port_num)

        try:
            val_file = open(port_ps)
        except IOError as e:
            print ('Error: unable to open file: ', str(e))
            return False

        content = val_file.readline().rstrip()
        val_file.close()

        # content is a string, either "0" or "1"
        return content == "1"

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

            if not (lpmode & 0x1): # 'Power override' bit is 0
                return False # Default High Power Mode
            else:
                if ((lpmode & 0x2) == 0x2):
                    return True # Low Power Mode if "Power set" bit is 1
                else:
                    return False # High Power Mode if "Power set" bit is 0
        except IOError as e:
            print ('Error: unable to open file: ', str(e))
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
                return False # Port is not present, unable to set the eeprom

            # Fill in write buffer
            regval = 0x3 if lpmode else 0x1 # 0x3:Low Power Mode, 0x1:High Power Mode
            buffer = create_string_buffer(1)
            if sys.version_info[0] >= 3:
                buffer[0] = regval
            else:
                buffer[0] = chr(regval)

            # Write to eeprom
            eeprom = open(self.port_to_eeprom_mapping[port_num], mode="r+b", buffering=0)
            eeprom.seek(93)
            eeprom.write(buffer[0])
            return True
        except IOError as e:
            print ('Error: unable to open file: ', str(e))
            return False
        finally:
            if eeprom is not None:
                eeprom.close()
                time.sleep(0.01)

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        port_ps = self.get_cpld_path(port_num) + 'module_reset_{0}'.format(port_num)

        try:
            reg_file = open(port_ps, mode="w")
        except IOError as e:
            print ('Error: unable to open file: ', str(e))
            return False

        #toggle reset
        reg_file.seek(0)
        reg_file.write('1')
        time.sleep(1)
        reg_file.seek(0)
        reg_file.write('0')
        reg_file.close()

        return True

    @property
    def get_transceiver_status(self):
        bitmap = 0

        for port in range(self.port_start, self.port_end+1):
            if not self.get_presence(port):
                continue
            bitmap |= (1 << (port - self.port_start))

        return bitmap

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
                fp_port = port
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
