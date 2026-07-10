# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#
try:
    import time
    import os
    from ctypes import create_string_buffer
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

# from xcvrd
SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 49
    PORT_END = 54
    PORTS_IN_BLOCK = 54
    QSFP_PORT_START = 49
    QSFP_PORT_END = 54

    BASE_VAL_PATH = "/sys/class/i2c-adapter/i2c-{0}/{1}-0050/"
    BASE_OOM_PATH = "/sys/bus/i2c/devices/{0}-0050/"
    BASE_CPLD_PATH = "/sys/bus/i2c/devices/{0}-0060/"
    I2C_BUS_ORDER = -1

    # The sidebands of QSFP is different.
    # present is in-order.
    # But lp_mode and reset are not.
    qsfp_sb_map = [0, 2, 4, 1, 3, 5]

    _port_to_is_present = {}
    _port_to_lp_mode = {}

    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
        49:  [1, 4],  # QSFP_start
        50:  [2, 6],
        51:  [3, 3],
        52:  [4, 5],
        53:  [5, 7],
        54:  [6, 2],
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

    # Two i2c buses might get flipped order, check them both.
    def update_i2c_order(self):
        if self.I2C_BUS_ORDER < 0:
            eeprom_path = "/sys/bus/i2c/devices/1-0057/eeprom"
            if os.path.exists(eeprom_path):
                self.I2C_BUS_ORDER = 0
            eeprom_path = "/sys/bus/i2c/devices/0-0057/eeprom"
            if os.path.exists(eeprom_path):
                self.I2C_BUS_ORDER = 1
        return self.I2C_BUS_ORDER

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        order = self.update_i2c_order()
        present_path = self.BASE_CPLD_PATH.format(order)
        present_path = present_path + "module_present_" + str(port_num)
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

        order = self.update_i2c_order()
        lp_mode_path = self.BASE_CPLD_PATH.format(order)
        lp_mode_path = lp_mode_path + "module_lp_mode_"
        lp_mode_path = lp_mode_path + str(port_num)

        content = "0"
        try:
            val_file = open(lp_mode_path)
            content = val_file.readline().rstrip()
            val_file.close()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        if content == "1":
            return True

        return False

    def get_low_power_mode(self, port_num):
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        if not self.get_presence(port_num):
            return self.get_low_power_mode_cpld(port_num)

        try:
            eeprom = None
            eeprom = open(self.port_to_eeprom_mapping[port_num], "rb")
            eeprom.seek(93)
            lpmode = ord(eeprom.read(1))

            if not (lpmode & 0x1):  # 'Power override' bit is 0
                return self.get_low_power_mode_cpld(port_num)
            else:
                if ((lpmode & 0x2) == 0x2):
                    return True  # Low Power Mode if "Power set" bit is 1
                else:
                    return False  # High Power Mode if "Power set" bit is 0
        except IOError as err:
            print("Error: unable to open file: %s" % str(err))
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
            eeprom = open(self.port_to_eeprom_mapping[port_num], "r+b")
            eeprom.seek(93)
            eeprom.write(buffer[0])
            return True
        except IOError as err:
            print("Error: unable to open file: %s" % str(err))
            return False
        finally:
            if eeprom is not None:
                eeprom.close()
                time.sleep(0.01)

    def reset(self, port_num):
        if port_num < self.qsfp_port_start or port_num > self.qsfp_port_end:
            return False

        order = self.update_i2c_order()
        lp_mode_path = self.BASE_CPLD_PATH.format(order)
        mod_rst_path = lp_mode_path + "module_reset_"
        mod_rst_path = mod_rst_path + str(port_num)
        print(mod_rst_path)

        try:
            reg_file = open(mod_rst_path, 'r+', buffering=0)
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
    def _get_presence_bitmap(self):
        nodes = []
        order = self.update_i2c_order()

        present_path = self.BASE_CPLD_PATH.format(order)
        nodes.append(present_path + "module_present_all")

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

    data = {'present': 0}

    def get_transceiver_change_event(self, timeout=2000):
        port_dict = {}
        port = 0

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

            return True, port_dict
        else:
            return True, {}
        return False, {}
