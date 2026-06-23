# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
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
    QSFP_PORT_END = 64
    PORT_END = 66
    PORTS_IN_BLOCK = 66

    FPGA_PCIE_PATH = "/sys/devices/platform/as9736_64d_fpga/"
    PCIE_UDB_EEPROM_PATH = '/sys/devices/platform/pcie_udb_fpga_device.{0}/eeprom'
    PCIE_LDB_EEPROM_PATH = '/sys/devices/platform/pcie_ldb_fpga_device.{0}/eeprom'

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(self.PORT_START, self.PORTS_IN_BLOCK - 1))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):

        for x in range(self.port_start, self.port_end+1):
            if x <= 32:
                self.port_to_eeprom_mapping[x] = self.PCIE_UDB_EEPROM_PATH.format(x - 1)
            else:
                self.port_to_eeprom_mapping[x] = self.PCIE_LDB_EEPROM_PATH.format(x - 33)

        SfpUtilBase.__init__(self)

    def __write_txt_file(self, file_path, value):
        try:
            reg_file = open(file_path, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_file.write(str(value))
        reg_file.close()

        return True

    def get_presence(self, port_num):
        present_path = self.FPGA_PCIE_PATH + "module_present_" + str(port_num)
        self.__port_to_is_present = present_path
        try:
            val_file = open(present_path)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to access file: %s" % str(e))
            return False

        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        if port_num > self.QSFP_PORT_END: #sfp not support lpmode
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
        if port_num > self.QSFP_PORT_END: #sfp not support lpmode:
            return False

        try:
            eeprom = None
            if not self.get_presence(port_num):
                return False  # Port is not present, unable to set the eeprom

            # Fill in write buffer
            # 0x3:Low Power Mode. "Power override" bit is 1 and "Power set" bit is 1
            # 0x9:High Power Mode. "Power override" bit is 1 ,"Power set" bit is 0 and "High Power Class Enable" bit is 1
            regval = 0x3 if lpmode else 0x9

            buffer = create_string_buffer(1)
            buffer[0] = regval

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
        if port_num > self.QSFP_PORT_END: #sfp not support lpmode:
            return False
        if not self.get_presence(port_num):
            return False  # Port is not present, unable to set reset

        mod_rst_path = self.FPGA_PCIE_PATH + "module_reset_" + str(port_num)

        self.__port_to_mod_rst = mod_rst_path

        ret = self.__write_txt_file(self.__port_to_mod_rst, 1)
        if ret is not True:
            return ret
        
        time.sleep(0.2)
        ret = self.__write_txt_file(self.__port_to_mod_rst, 0)
        time.sleep(0.2)
        
        return ret

    @property
    def _get_presence_bitmap(self):

        bits = []
        for x in range(self.port_start, self.port_end + 1):
            bits.append(str(int(self.get_presence(x))))

        rev = "".join(bits[::-1])
        return int(rev, 2)

    data = {'present': 0}

    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}

        if timeout == 0:
            cd_ms = sys.maxsize
        else:
            cd_ms = timeout

        # poll per second
        while cd_ms > 0:
            reg_value = self._get_presence_bitmap
            changed_ports = self.data['present'] ^ reg_value
            if changed_ports != 0:
                break
            time.sleep(1)
            cd_ms = cd_ms - 1000

        if changed_ports != 0:
            for port in range(self.port_start, self.port_end + 1):
                # Mask off the bit corresponding to our port
                mask = (1 << (port - self.port_start))
                if changed_ports & mask:
                    if (reg_value & mask) == 0:
                        port_dict[port] = SFP_STATUS_REMOVED
                    else:
                        port_dict[port] = SFP_STATUS_INSERTED

            # Update cache
            self.data['present'] = reg_value
            return True, port_dict
        else:
            return True, {}

