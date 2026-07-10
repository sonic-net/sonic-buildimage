# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import datetime
    import fcntl
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 0
    PORT_END = 31
    PORTS_IN_BLOCK = 32

    EEPROM_OFFSET = 20
    SFP_LOCK_FILE="/etc/sonic/sfp_lock"

    _port_to_eeprom_mapping = {}
    port_dict = {}

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
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def get_transceiver_status(self):

        try:
            reg_file = open("/sys/devices/platform/dell-s6000-cpld.0/qsfp_modprs")

        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        reg_file.close()

        return int(content, 16)

    def __init__(self):

        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"

        for x in range(0, self.port_end + 1):
            self._port_to_eeprom_mapping[x] = eeprom_path.format(x + self.EEPROM_OFFSET)

        # Get Transceiver status
        self.modprs_register = self.get_transceiver_status

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            fd = open(self.SFP_LOCK_FILE, "r")
        except IOError as e:
            print("Error: unable to open file: "+ str(e))
            return False
        fcntl.flock(fd, fcntl.LOCK_EX)
        self.set_modsel(port_num)

        try:
            reg_file = open("/sys/devices/platform/dell-s6000-cpld.0/qsfp_modprs")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            fcntl.flock(fd, fcntl.LOCK_UN)
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)
        fcntl.flock(fd, fcntl.LOCK_UN)
        # ModPrsL is active low
        if reg_value & mask == 0:
            return True

        return False

    def get_modsel(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/devices/platform/dell-s6000-cpld.0/qsfp_modsel")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)

        if reg_value & mask == 1:
            return False

        return True

    def set_modsel(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/devices/platform/dell-s6000-cpld.0/qsfp_modsel", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << port_num)
        reg_value = reg_value | int("0xffffffff", 16)
        reg_value = reg_value & ~mask

        # Convert our register value back to a hex string and write back
        content = hex(reg_value)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def get_eeprom_raw(self, port_num, num_bytes=256):
        # Read interface id EEPROM at addr 0x50
        try:
            fd = open(self.SFP_LOCK_FILE, "r")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return None
        fcntl.flock(fd, fcntl.LOCK_EX)
        self.set_modsel(port_num)
        eeprom_bytes = self._read_eeprom_devid(port_num, self.IDENTITY_EEPROM_ADDR, 0, num_bytes)
        fcntl.flock(fd, fcntl.LOCK_UN)
        return eeprom_bytes

    def get_eeprom_dom_raw(self, port_num):
        if port_num in self.osfp_ports:
            return None
        if port_num in self.qsfp_ports:
            # QSFP DOM EEPROM is also at addr 0x50 and thus also stored in eeprom_ifraw
            return None
        else:
            # Read dom eeprom at addr 0x51
            if not self.get_modsel(port_num):
                try:
                    fd = open(self.SFP_LOCK_FILE, "r")
                except IOError as e:
                    print("Error: unable to open file: %s" % str(e))
                    return None
                fcntl.flock(fd, fcntl.LOCK_EX)
                self.set_modsel(port_num)
                eeprom_bytes = self._read_eeprom_devid(port_num, self.DOM_EEPROM_ADDR, 0)
                fcntl.flock(fd, fcntl.LOCK_UN)
                return eeprom_bytes
            else:
                return self._read_eeprom_devid(port_num, self.DOM_EEPROM_ADDR, 0)

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open("/sys/devices/platform/dell-s6000-cpld.0/qsfp_lpmode")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))

        content = reg_file.readline().rstrip()

        # content is a string containing the hex representation of the register
        reg_value = int(content, 16)

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

        try:
            reg_file = open("/sys/devices/platform/dell-s6000-cpld.0/qsfp_lpmode", "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

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
        QSFP_RESET_REGISTER_DEVICE_FILE = "/sys/devices/platform/dell-s6000-cpld.0/qsfp_reset"

        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = reg_file.readline().rstrip()

        # File content is a string containing the hex representation of the register
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

        # Flip the bit back high and write back to the register to take port out of reset
        try:
            reg_file = open(QSFP_RESET_REGISTER_DEVICE_FILE, "w")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_value | mask
        reg_file.seek(0)
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def get_transceiver_change_event(self, timeout=0):

        start_time = time.time()
        port_dict = {}
        port = self.port_start
        forever = False

        if timeout == 0:
            forever = True
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
            reg_value = self.get_transceiver_status
            if reg_value != self.modprs_register:
                changed_ports = self.modprs_register ^ reg_value
                while port >= self.port_start and port <= self.port_end:

                    # Mask off the bit corresponding to our port
                    mask = (1 << port)

                    if changed_ports & mask:
                        # ModPrsL is active low
                        if reg_value & mask == 0:
                            port_dict[port] = '1'
                        else:
                            port_dict[port] = '0'

                    port += 1

                # Update reg value
                self.modprs_register = reg_value
                return True, port_dict

            if forever:
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
