# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import os
    import logging
    import time
    import select
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 32
    PORTS_IN_BLOCK = 32
    IOM_1_PORT_START = 1
    IOM_1_PORT_END = 12
    IOM_2_PORT_START = 13
    IOM_2_PORT_END = 22
    IOM_3_PORT_START = 23
    IOM_3_PORT_END = 32

    BASE_VAL_PATH = "/sys/bus/i2c/devices/i2c-{0}/{0}-003e/"
    OIR_FD_PATH = "/sys/devices/platform/dell_ich.0/sci_int_gpio_sus6"

    oir_fd = -1
    epoll = -1
    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
        0: [0, 00],  # Dummy Entry
        1: [9, 18],
        2: [9, 19],
        3: [9, 20],
        4: [9, 21],
        5: [9, 22],
        6: [9, 23],
        7: [9, 24],
        8: [9, 25],
        9: [8, 26],
        10: [8, 27],
        11: [8, 28],
        12: [8, 29],
        13: [8, 31],  # reordered
        14: [8, 30],
        15: [8, 33],  # reordered
        16: [8, 32],
        17: [7, 34],
        18: [7, 35],
        19: [7, 36],
        20: [7, 37],
        21: [7, 38],
        22: [7, 39],
        23: [7, 40],
        24: [7, 41],
        25: [6, 42],
        26: [6, 43],
        27: [6, 44],
        28: [6, 45],
        29: [6, 46],
        30: [6, 47],
        31: [6, 48],
        32: [6, 49]
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
    def iom1_port_start(self):
        return self.IOM_1_PORT_START

    @property
    def iom1_port_end(self):
        return self.IOM_1_PORT_END

    @property
    def iom2_port_start(self):
        return self.IOM_2_PORT_START

    @property
    def iom2_port_end(self):
        return self.IOM_2_PORT_END

    @property
    def iom3_port_start(self):
        return self.IOM_3_PORT_START

    @property
    def iom3_port_end(self):
        return self.IOM_3_PORT_END

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def port_to_i2c_mapping(self):
        return self._port_to_i2c_mapping

    def __init__(self):
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/i2c-{1}/{1}-0050/eeprom"

        for x in range(0, self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path.format(
                self.port_to_i2c_mapping[x][0],
                self.port_to_i2c_mapping[x][1])

        SfpUtilBase.__init__(self)

    def __del__(self):
        if self.oir_fd != -1:
            self.epoll.unregister(self.oir_fd.fileno())
            self.epoll.close()
            self.oir_fd.close()

    def normalize_port(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return -1, -1
        # port_num and i2c match
        if port_num >= self.iom1_port_start and port_num <= self.iom1_port_end:
            i2c_line = 14
        elif (port_num >= self.iom2_port_start and
                port_num <= self.iom2_port_end):
            i2c_line = 15
        elif (port_num >= self.iom3_port_start and
                port_num <= self.iom3_port_end):
            i2c_line = 16

        # Rationalize port settings
        if port_num >= self.iom1_port_start and port_num <= self.iom1_port_end:
            port_num = port_num - 1
        elif port_num >= self.iom2_port_start and port_num <= self.iom2_port_end:
            port_num = (port_num - 1) % 12
        elif (port_num >= self.iom3_port_start and
                port_num <= self.iom3_port_end):
            port_num = (port_num - 1) % 22

        return i2c_line, port_num

    def get_presence(self, port_num):

        i2c_line, port_num = self.normalize_port(port_num)
        if port_num == -1:
            return False

        try:
            qsfp_path = self.BASE_VAL_PATH.format(i2c_line)+"qsfp_modprs"
            reg_file = open(qsfp_path, "r")

        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # Absence of IOM throws read error
        if (content == 'read error'):
            return False

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # ModPrsL is active low
        if reg_value & mask == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):

        i2c_line, port_num = self.normalize_port(port_num)
        if port_num == -1:
            return False

        try:
            qsfp_path = self.BASE_VAL_PATH.format(i2c_line)+"qsfp_lpmode"
            reg_file = open(qsfp_path, "r")

        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # Absence of IOM throws read error
        if (content == 'read error'):
            return False

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # LPMode is active high
        if reg_value & mask == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):

        i2c_line, port_num = self.normalize_port(port_num)
        if port_num == -1:
            return False

        try:
            qsfp_path = self.BASE_VAL_PATH.format(i2c_line)+"qsfp_lpmode"
            reg_file = open(qsfp_path, "r+")

        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # Absence of IOM throws read error
        if (content == 'read error'):
            return False

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)
        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            reg_value = reg_value | mask
        else:
            reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        content = hex(reg_value)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def reset(self, port_num):

        i2c_line, port_num = self.normalize_port(port_num)
        if port_num == -1:
            return False

        try:
            qsfp_path = self.BASE_VAL_PATH.format(i2c_line)+"qsfp_lpmode"
            reg_file = open(qsfp_path, "r+")

        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # File content is a string containing the hex representation of th
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # ResetL is active low
        reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        reg_file.seek(0)
        reg_file.write(hex(reg_value))
        reg_file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register to take
        # port out of reset
        try:
            qsfp_path = self.BASE_VAL_PATH.format(i2c_line)+"qsfp_lpmode"
            reg_file = open(qsfp_path, "w+")

        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_value | mask
        reg_file.seek(0)
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def get_register(self, reg_file):
        retval = 'ERR'

        if (not os.path.isfile(reg_file)):
            print(reg_file + ' not found !')
            return retval

        try:
            with open(reg_file, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", reg_file, "file !")

        retval = retval.rstrip('\r\n')
        retval = retval.lstrip(" ")
        return retval

    def check_interrupts(self, port_dict):
        retval = 0
        is_port_dict_updated = False
        # Read the QSFP ABS interrupt & status registers
        cpld2_abs_int = self.get_register(
            "/sys/class/i2c-adapter/i2c-14/14-003e/qsfp_abs_int")
        cpld2_abs_sta = self.get_register(
            "/sys/class/i2c-adapter/i2c-14/14-003e/qsfp_abs_sta")
        cpld3_abs_int = self.get_register(
            "/sys/class/i2c-adapter/i2c-15/15-003e/qsfp_abs_int")
        cpld3_abs_sta = self.get_register(
            "/sys/class/i2c-adapter/i2c-15/15-003e/qsfp_abs_sta")
        cpld4_abs_int = self.get_register(
            "/sys/class/i2c-adapter/i2c-16/16-003e/qsfp_abs_int")
        cpld4_abs_sta = self.get_register(
            "/sys/class/i2c-adapter/i2c-16/16-003e/qsfp_abs_sta")

        if (cpld2_abs_int == 'ERR' or cpld2_abs_sta == 'ERR' or
                cpld3_abs_int == 'ERR' or cpld3_abs_sta == 'ERR' or
                cpld4_abs_int == 'ERR' or cpld4_abs_sta == 'ERR'):
            return -1

        cpld2_abs_int = int(cpld2_abs_int, 16)
        cpld2_abs_sta = int(cpld2_abs_sta, 16)
        cpld3_abs_int = int(cpld3_abs_int, 16)
        cpld3_abs_sta = int(cpld3_abs_sta, 16)
        cpld4_abs_int = int(cpld4_abs_int, 16)
        cpld4_abs_sta = int(cpld4_abs_sta, 16)

        # Make it contiguous (discard reserved bits)
        interrupt_reg = (cpld2_abs_int & 0xfff) |\
                        ((cpld3_abs_int & 0x3ff) << 12) |\
                        ((cpld4_abs_int & 0x3ff) << 22)
        status_reg = (cpld2_abs_sta & 0xfff) |\
                     ((cpld3_abs_sta & 0x3ff) << 12) |\
                     ((cpld4_abs_sta & 0x3ff) << 22)

        port = self.port_start
        while port <= self.port_end:
            if interrupt_reg & (1 << (port-1)):
                # update only if atleast one port has generated
                # interrupt
                is_port_dict_updated = True
                if status_reg & (1 << (port-1)):
                    # status reg 1 => optics is removed
                    port_dict[port] = '0'
                else:
                    # status reg 0 => optics is inserted
                    port_dict[port] = '1'
            port += 1
        return retval, is_port_dict_updated

    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}
        try:
            # We get notified when there is an SCI interrupt from GPIO SUS6
            # Open the sysfs file and register the epoll object
            self.oir_fd = open(self.OIR_FD_PATH, "r")
            if self.oir_fd != -1:
                # Do a dummy read before epoll register
                self.oir_fd.read()
                self.epoll = select.epoll()
                self.epoll.register(self.oir_fd.fileno(),
                                    select.EPOLLIN & select.EPOLLET)
            else:
                print("get_transceiver_change_event : unable to create fd")
                return False, {}

            # Check for missed interrupts by invoking self.check_interrupts
            # which will update the port_dict.
            while True:
                interrupt_count_start = self.get_register(self.OIR_FD_PATH)

                retval, is_port_dict_updated = \
                    self.check_interrupts(port_dict)
                if ((retval == 0) and (is_port_dict_updated is True)):
                    return True, port_dict

                interrupt_count_end = self.get_register(self.OIR_FD_PATH)

                if (interrupt_count_start == 'ERR' or
                        interrupt_count_end == 'ERR'):
                    print("get_transceiver_change_event : \
                            unable to retrive interrupt count")
                    break

                # check_interrupts() itself may take upto 100s of msecs.
                # We detect a missed interrupt based on the count
                if interrupt_count_start == interrupt_count_end:
                    break

            # Block until an xcvr is inserted or removed with timeout = -1
            events = self.epoll.poll(
                timeout=timeout if timeout != 0 else -1)
            if events:
                # check interrupts and return the port_dict
                retval, is_port_dict_updated = \
                    self.check_interrupts(port_dict)
                if (retval != 0):
                    return False, {}

            return True, port_dict
        except:
            return False, {}
        finally:
            if self.oir_fd != -1:
                self.epoll.unregister(self.oir_fd.fileno())
                self.epoll.close()
                self.oir_fd.close()
                self.oir_fd = -1
                self.epoll = -1

        return False, {}
