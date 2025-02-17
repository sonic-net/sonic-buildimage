# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 56
    SFP_PORT_START = 49
    SFP_PORT_END = 56
    SFP_I2C_BUS_START = 0xa
    PORTS_IN_BLOCK = 56

    _port_to_eeprom_mapping = {}
    sfp_status_dict = {}

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
        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'

        for port_idx in range(self.SFP_PORT_START, self.SFP_PORT_END + 1):
            self._port_to_eeprom_mapping[port_idx] = eeprom_path.format(self.SFP_I2C_BUS_START + (port_idx - self.SFP_PORT_START))
            self.sfp_status_dict[port_idx] = 0 #Initialize all modules as absent/removed
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.SFP_PORT_START or port_num > self.SFP_PORT_END:
            return False

        sfp_status_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/i2c-9/i2c-{0}/{0}-0066/xcvr_present"\
                           .format(self.SFP_I2C_BUS_START + (port_num - self.SFP_PORT_START))
        try:
            with open(sfp_status_path, 'r') as fd:
                status = fd.read().rstrip('\r\n')
                if status == '0':
                    return True
                else:
                    return False
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.SFP_PORT_START or port_num > self.SFP_PORT_END:
            return False

        sfp_lpmode_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/i2c-9/i2c-{0}/{0}-0066/xcvr_lpmode"\
                           .format(self.SFP_I2C_BUS_START + (port_num - self.SFP_PORT_START))
        try:
            with open(sfp_lpmode_path, 'r') as fd:
                status = fd.read().rstrip('\r\n')
                if status == '1':
                    return True
                else:
                    return False
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return False

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.SFP_PORT_START or port_num > self.SFP_PORT_END:
            return False

        sfp_lpmode_path = "/sys/devices/pci0000:00/0000:00:12.0/i2c-0/i2c-9/i2c-{0}/{0}-0066/xcvr_lpmode"\
                           .format(self.SFP_I2C_BUS_START + (port_num - self.SFP_PORT_START))
        try:
            with open(sfp_lpmode_path, 'w') as fd:
                if lpmode:
                    fd.write('1')
                else:
                    fd.write('0')

                return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return False

    def reset(self, port_num):
        # SFP reset is not supported
        return False

    def get_transceiver_change_event(self, timeout=0):
        sfp_dict = {}

        SFP_REMOVED = '0'
        SFP_INSERTED = '1'

        SFP_PRESENT = True
        SFP_ABSENT = False

        start_time = time.time()
        time_period = timeout/float(1000) #Convert msecs to secs

        while time.time() < (start_time + time_period) or timeout == 0:
            for port_idx in range(self.SFP_PORT_START, self.SFP_PORT_END + 1):
                if self.sfp_status_dict[port_idx] == SFP_REMOVED and \
                    self.get_presence(port_idx) == SFP_PRESENT:
                    sfp_dict[str(port_idx)] = SFP_INSERTED
                    self.sfp_status_dict[port_idx] = SFP_INSERTED
                elif self.sfp_status_dict[port_idx] == SFP_INSERTED and \
                    self.get_presence(port_idx) == SFP_ABSENT:
                    sfp_dict[str(port_idx)] = SFP_REMOVED
                    self.sfp_status_dict[port_idx] = SFP_REMOVED

            if sfp_dict != {}:
                return (True, {'sfp':sfp_dict})

            time.sleep(0.5)

        return (True, {}) # Timeout
