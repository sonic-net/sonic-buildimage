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
    _port_end = 54
    ports_in_block = 4

    _port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {
        49: 2,
        50: 3,
        51: 4,
        52: 5,
        53: 6,
        54: 7
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
        mux_dev = sorted(glob.glob("/sys/class/i2c-adapter/i2c-1/i2c-[2-7]"))
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        y = 0
        self.presence = {}
        for x in range(self.port_start, self.port_end + 1):
            mux_dev_num = mux_dev[y]
            self.port_to_i2c_mapping[x] = mux_dev_num[-1]
            y = y + 1
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path
            self._port_to_eeprom_mapping[x] = port_eeprom_path
            self.presence[x] = False;

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
#
#        raise NotImplementedError
#       eyen remove above line and add below. Refer to centec/arm64-centec_e530_48t4x_p-r0/plugins/sfputil.py

        now = time.time()
        port_dict = {}

        if timeout < 1000:
            timeout = 1000
        timeout = (timeout) / float(1000) # Convert to secs

        if now < (self.data['last'] + timeout) and self.data['valid']:
            return True, {}

        for x in range(self.port_start, self.port_end + 1):
            presence = self.get_presence(x)
            if presence != self.presence[x]:
                self.presence[x] = presence
                # index in port_config.ini
                if presence:
                    port_dict[x] = SFP_STATUS_INSERTED
                else:
                    port_dict[x] = SFP_STATUS_REMOVED

        if bool(port_dict):
            self.data['last'] = now
            self.data['valid'] = 1
            return True, port_dict
        else:
            time.sleep(0.5)
            return True, {}

