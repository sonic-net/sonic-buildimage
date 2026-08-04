try:
    import glob
    import sonic_platform.platform
    import sonic_platform.chassis
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class SfpUtil(SfpUtilBase):
    """Platform specific sfputil class"""

    _port_start = 49
    _port_end = 52
    ports_in_block = 4

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        49: 2,
        50: 3,
        51: 4,
        52: 5
    }
    _changed_ports = [0, 0, 0, 0]

    @property
    def port_start(self):
        return self._port_start

    @property
    def port_end(self):
        return self._port_end

    @property
    def qsfp_ports(self):
        return list(range(0, 0))

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        # Mux Ordering
        mux_dev = sorted(glob.glob("/sys/class/i2c-adapter/i2c-0/i2c-[0-9]"))
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        y = 0
        for x in range(self.port_start, self.port_end + 1):
            mux_dev_num = mux_dev[y]
            self.port_to_i2c_mapping[x] = mux_dev_num[-1]
            y = y + 1
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path
            self._port_to_eeprom_mapping[x] = port_eeprom_path
        SfpUtilBase.__init__(self)

        self.chassis = sonic_platform.platform.Platform().get_chassis()
        
    def reset(self, port_num):
        # print " SfpUtil(SfpUtilBase) re-directed to chassis PMON 2.0 "
        if self.chassis is not None:
            return self.chassis.get_sfp(port_num).reset()
        else:
            return False

    def set_low_power_mode(self, port_nuM, lpmode):
        # print " SfpUtil(SfpUtilBase) targeted for deprecation "
        return False

    def get_low_power_mode(self, port_num):
        # print " SfpUtil(SfpUtilBase) targeted for deprecation "
        return False

    def get_presence(self, port_num):
        # print " SfpUtil(SfpUtilBase) re-directed to chassis PMON 2.0 "
        if self.chassis is not None:
            return self.chassis.get_sfp(port_num).get_presence()
        else:
            return False

    def get_transceiver_change_event(self, timeout):
        # print " SfpUtil(SfpUtilBase) targeted for deprecation "

        raise NotImplementedError
