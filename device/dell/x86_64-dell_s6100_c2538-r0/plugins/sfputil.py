# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#
#    +-i2c----+--------+
#    |    6  |   7     |
#    | +-------------+ |
#    | |IOM1  | IOM3 | |
#    | |IOM2  | IOM4 | |
#    | +-------------+ |
#    |   8    |   9    |
#    +--------+--------+


try:
    import time
    import os
    import logging
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 63
    PORTS_IN_BLOCK = 64
    POLL_INTERVAL = 1
    _port_to_eeprom_mapping = {}
    _port_to_i2c_mapping = {
        0: [6, 18, 34, 50, 66],
        1: [6, 19, 35, 51, 67],
        2: [6, 20, 36, 52, 68],
        3: [6, 21, 37, 53, 69],
        4: [6, 22, 38, 54, 70],
        5: [6, 23, 39, 55, 71],
        6: [6, 24, 40, 56, 72],
        7: [6, 25, 41, 57, 73],
        8: [6, 26, 42, 58, 74],
        9: [6, 27, 43, 59, 75],
        10: [6, 28, 44, 60, 76],
        11: [6, 29, 45, 61, 77],
        12: [6, 30, 46, 62, 78],
        13: [6, 31, 47, 63, 79],
        14: [6, 32, 48, 64, 80],
        15: [6, 33, 49, 65, 81],
        16: [8, 18, 34, 50, 66],
        17: [8, 19, 35, 51, 67],
        18: [8, 20, 36, 52, 68],
        19: [8, 21, 37, 53, 69],
        20: [8, 22, 38, 54, 70],
        21: [8, 23, 39, 55, 71],
        22: [8, 24, 40, 56, 72],
        23: [8, 25, 41, 57, 73],
        24: [8, 26, 42, 58, 74],
        25: [8, 27, 43, 59, 75],
        26: [8, 28, 44, 60, 76],
        27: [8, 29, 45, 61, 77],
        28: [8, 30, 46, 62, 78],
        29: [8, 31, 47, 63, 79],
        30: [8, 32, 48, 64, 80],
        31: [8, 33, 49, 65, 81],
        32: [7, 18, 34, 50, 66],
        33: [7, 19, 35, 51, 67],
        34: [7, 20, 36, 52, 68],
        35: [7, 21, 37, 53, 69],
        36: [7, 22, 38, 54, 70],
        37: [7, 23, 39, 55, 71],
        38: [7, 24, 40, 56, 72],
        39: [7, 25, 41, 57, 73],
        40: [7, 26, 42, 58, 74],
        41: [7, 27, 43, 59, 75],
        42: [7, 28, 44, 60, 76],
        43: [7, 29, 45, 61, 77],
        44: [7, 30, 46, 62, 78],
        45: [7, 31, 47, 63, 79],
        46: [7, 32, 48, 64, 80],
        47: [7, 33, 49, 65, 81],
        48: [9, 18, 34, 50, 66],
        49: [9, 19, 35, 51, 67],
        50: [9, 20, 36, 52, 68],
        51: [9, 21, 37, 53, 69],
        52: [9, 22, 38, 54, 70],
        53: [9, 23, 39, 55, 71],
        54: [9, 24, 40, 56, 72],
        55: [9, 25, 41, 57, 73],
        56: [9, 26, 42, 58, 74],
        57: [9, 27, 43, 59, 75],
        58: [9, 28, 44, 60, 76],
        59: [9, 29, 45, 61, 77],
        60: [9, 30, 46, 62, 78],
        61: [9, 31, 47, 63, 79],
        62: [9, 32, 48, 64, 80],
        63: [9, 33, 49, 65, 81]
    }

    IOM_1_PORT_START = 0
    IOM_1_PORT_END = 15

    IOM_2_PORT_START = 16
    IOM_2_PORT_END = 31

    IOM_3_PORT_START = 32
    IOM_3_PORT_END = 47

    IOM_4_PORT_START = 48
    IOM_4_PORT_END = 63

    BASE_VAL_PATH = "/sys/class/i2c-adapter/i2c-{0}/{0}-003e/"

    oir_fd = -1
    epoll = -1

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(0, self.PORTS_IN_BLOCK + 1))

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
    def iom4_port_start(self):
        return self.IOM_4_PORT_START

    @property
    def iom4_port_end(self):
        return self.IOM_4_PORT_END

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def port_to_i2c_mapping(self):
        return self._port_to_i2c_mapping

    def __init__(self):
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/i2c-{1}/{1}-0050/eeprom"
        global port_to_eeprom_path

        for port_num in range(0, self.port_end + 1):
            if port_num >= self.iom1_port_start and port_num <=\
                    self.iom1_port_end:
                assigned = 0
                # i2c-6
                for x in range(1, 5):
                    port_to_eeprom_path = eeprom_path.format(
                        self.port_to_i2c_mapping[port_num][0],
                        self.port_to_i2c_mapping[port_num][x])
                    if (os.path.isfile(port_to_eeprom_path)):
                        self.port_to_eeprom_mapping[port_num] =\
                            port_to_eeprom_path
                        assigned = 1
                    elif (not assigned):
                        self.port_to_eeprom_mapping[port_num] =\
                            "No IOM"

            elif(port_num >= self.iom2_port_start and
                    port_num <= self.iom2_port_end):
                assigned = 0
                # i2c-8
                for x in range(1, 5):
                    port_to_eeprom_path = eeprom_path.format(
                        self.port_to_i2c_mapping[port_num][0],
                        self.port_to_i2c_mapping[port_num][x])
                    if (os.path.isfile(port_to_eeprom_path)):
                        self.port_to_eeprom_mapping[port_num] =\
                            port_to_eeprom_path
                        assigned = 1
                    elif (not assigned):
                        self.port_to_eeprom_mapping[port_num] =\
                            "No IOM"

            elif(port_num >= self.iom3_port_start and port_num <=
                    self.iom3_port_end):
                assigned = 0
                # i2c-7
                for x in range(1, 5):
                    port_to_eeprom_path = eeprom_path.format(
                        self.port_to_i2c_mapping[port_num][0],
                        self.port_to_i2c_mapping[port_num][x])
                    if (os.path.isfile(port_to_eeprom_path)):
                        self.port_to_eeprom_mapping[port_num] =\
                            port_to_eeprom_path
                        assigned = 1
                    elif (not assigned):
                        self.port_to_eeprom_mapping[port_num] =\
                            "No IOM"

            elif(port_num >= self.iom4_port_start and port_num <=
                    self.iom4_port_end):
                assigned = 0
                # i2c-9
                for x in range(1, 5):
                    port_to_eeprom_path = eeprom_path.format(
                        self.port_to_i2c_mapping[port_num][0],
                        self.port_to_i2c_mapping[port_num][x])
                    if (os.path.isfile(port_to_eeprom_path)):
                        self.port_to_eeprom_mapping[port_num] =\
                            port_to_eeprom_path
                        assigned = 1
                    elif (not assigned):
                        self.port_to_eeprom_mapping[port_num] =\
                            "No IOM"
        SfpUtilBase.__init__(self)
        self._transceiver_presence = self._get_transceiver_presence()

    def __del__(self):
        if self.oir_fd != -1:
            self.epoll.unregister(self.oir_fd.fileno())
            self.epoll.close()
            self.oir_fd.close()

    def get_presence(self, port_num):

        global i2c_line

        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # port_num and i2c match
        if(port_num >= self.iom1_port_start and port_num <=
                self.iom1_port_end):
            i2c_line = 14
        elif(port_num >= self.iom2_port_start and port_num <=
                self.iom2_port_end):
            i2c_line = 16
        elif(port_num >= self.iom3_port_start and port_num <=
                self.iom3_port_end):
            i2c_line = 15
        elif(port_num >= self.iom4_port_start and port_num <=
                self.iom4_port_end):
            i2c_line = 17

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

        # Rationalize port settings
        if port_num > 15:
            port_num = port_num % 16

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # ModPrsL is active low
        if reg_value & mask == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # port_num and i2c match
        if(port_num >= self.iom1_port_start and port_num <=
                self.iom1_port_end):
            i2c_line = 14
        elif(port_num >= self.iom2_port_start and port_num <=
                self.iom2_port_end):
            i2c_line = 16
        elif(port_num >= self.iom3_port_start and port_num <=
                self.iom3_port_end):
            i2c_line = 15
        elif(port_num >= self.iom4_port_start and port_num <=
                self.iom4_port_end):
            i2c_line = 17

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

        # Rationalize port settings
        if port_num > 15:
            port_num = port_num % 16

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        # LPMode is active high
        if reg_value & mask == 0:
            return False

        return True

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # port_num and i2c match
        if(port_num >= self.iom1_port_start and port_num <=
                self.iom1_port_end):
            i2c_line = 14
        elif(port_num >= self.iom2_port_start and port_num <=
                self.iom2_port_end):
            i2c_line = 16
        elif(port_num >= self.iom3_port_start and port_num <=
                self.iom3_port_end):
            i2c_line = 15
        elif(port_num >= self.iom4_port_start and port_num <=
                self.iom4_port_end):
            i2c_line = 17

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

        # Rationalize port settings
        if port_num > 15:
            port_num = port_num % 16

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

        global i2c_line

        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        # port_num and i2c match
        if(port_num >= self.iom1_port_start and port_num <=
                self.iom1_port_end):
            i2c_line = 14
        elif(port_num >= self.iom2_port_start and port_num <=
                self.iom2_port_end):
            i2c_line = 16
        elif(port_num >= self.iom3_port_start and port_num <=
                self.iom3_port_end):
            i2c_line = 15
        elif(port_num >= self.iom4_port_start and port_num <=
                self.iom4_port_end):
            i2c_line = 17

        try:
            qsfp_path = self.BASE_VAL_PATH.format(i2c_line)+"qsfp_lpmode"
            reg_file = open(qsfp_path, "r+")

        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # Absence of IOM throws read error
        if (content == 'read error'):
            print("it's empty")
            return False

        # File content is a string containing the hex representation
        # of the register
        reg_value = int(content, 16)

        # Rationalize port settings
        if port_num > 15:
            port_num = port_num % 16

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

        # Flip the bit back high and write back to the register to take port
        # out of reset
        try:
            qsfp_path = self.BASE_VAL_PATH.format(i2c_line)+"qsfp_lpmode"
            reg_file = open(qsfp_path, "w")

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

        if not os.path.isfile(reg_file):
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

    def _get_transceiver_presence(self):

        cpld2_modprs = self.get_register(
                    "/sys/class/i2c-adapter/i2c-14/14-003e/qsfp_modprs")
        cpld3_modprs = self.get_register(
                    "/sys/class/i2c-adapter/i2c-15/15-003e/qsfp_modprs")
        cpld4_modprs = self.get_register(
                    "/sys/class/i2c-adapter/i2c-16/16-003e/qsfp_modprs")
        cpld5_modprs = self.get_register(
                    "/sys/class/i2c-adapter/i2c-17/17-003e/qsfp_modprs")

        # If IOM is not present, register read will fail.
        # Handle the scenario gracefully
        if cpld2_modprs == 'read error' or cpld2_modprs == 'ERR':
            cpld2_modprs = '0x0'
        if cpld3_modprs == 'read error' or cpld3_modprs == 'ERR':
            cpld3_modprs = '0x0'
        if cpld4_modprs == 'read error' or cpld4_modprs == 'ERR':
            cpld4_modprs = '0x0'
        if cpld5_modprs == 'read error' or cpld5_modprs == 'ERR':
            cpld5_modprs = '0x0'

        # Make it contiguous
        transceiver_presence = (int(cpld2_modprs, 16) & 0xffff) |\
                               ((int(cpld4_modprs, 16) & 0xffff) << 16) |\
                               ((int(cpld3_modprs, 16) & 0xffff) << 32) |\
                               ((int(cpld5_modprs, 16) & 0xffff) << 48)

        return transceiver_presence

    def get_transceiver_change_event(self, timeout=0):
        """
        Returns a dictionary containing optics insertion/removal status.
        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.
        Returns:
            (bool, dict):
                - True if call successful, False if not;
        """
        port_dict = {}
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            return False, port_dict # Incorrect timeout

        while True:
            if forever:
                timer = self.POLL_INTERVAL
            else:
                timer = min(timeout, self.POLL_INTERVAL)
                start_time = time.time()

            time.sleep(timer)
            cur_presence = self._get_transceiver_presence()

            # Update dict only if a change has been detected
            if cur_presence != self._transceiver_presence:
                changed_ports = self._transceiver_presence ^ cur_presence
                for port in range(self.port_end):
                    # Mask off the bit corresponding to particular port
                    mask = 1 << port
                    if changed_ports & mask:
                        # qsfp_modprs 1 => optics is removed
                        if cur_presence & mask:
                            port_dict[port] = '0'
                        # qsfp_modprs 0 => optics is inserted
                        else:
                            port_dict[port] = '1'

                # Update current presence
                self._transceiver_presence = cur_presence
                break

            if not forever:
                elapsed_time = time.time() - start_time
                timeout = round(timeout - elapsed_time, 3)
                if timeout <= 0:
                    break

        return True, port_dict
