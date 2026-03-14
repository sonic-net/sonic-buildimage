try:
    import time
    import os
    import select
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    PORT_START = 1
    PORT_END = 55
    SFP_PORT_START = 49
    SFP_PORT_END = 52
    port_to_i2c_mapping = {
        49: 15,
        50: 14,
        51: 17,
        52: 16
    }
    _port_to_eeprom_mapping = {}
    _sfp_port = list(range(SFP_PORT_START, SFP_PORT_END + 1))

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return []

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    def __init__(self):
        # Override port_to_eeprom_mapping for class initialization
        eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'
        for x in range(self.PORT_START, self.PORT_END + 1):
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x]) if x in self._sfp_port else None
            self.port_to_eeprom_mapping[x] = port_eeprom_path
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        sfp_modabs_path = '/sys/devices/platform/e1031.smc/SFP/sfp_modabs'

        if port_num not in self._sfp_port:
            return False

        status = 1
        try:
            with open(sfp_modabs_path, 'r') as port_status:
                status = int(port_status.read(), 16)
                status = (status >> (port_num - self.SFP_PORT_START)) & 1
        except IOError:
            return False

        return status == 0

    def get_low_power_mode(self, port_num):
        raise NotImplementedError

    def set_low_power_mode(self, port_num, lpmode):
        raise NotImplementedError

    def reset(self, port_num):
        raise NotImplementedError

    def get_transceiver_change_event(self, timeout=0):
        epoll = select.epoll()
        port_dict = {}
        timeout_sec = timeout/1000
        modabs_interrupt_path = '/sys/devices/platform/e1031.smc/SFP/modabs_int'
        try:
            # We get notified when there is an SCI interrupt from GPIO SUS7
            fd = open("/sys/devices/platform/hlx-ich.0/sci_int_gpio_sus7", "r")
            fd.read()

            epoll.register(fd.fileno(), select.EPOLLIN & select.EPOLLET)
            events = epoll.poll(timeout=timeout_sec if timeout != 0 else -1)
            if events:
                found_flag = 0
                # Read the QSFP ABS interrupt & status registers
                with open(modabs_interrupt_path, 'r') as port_changes:
                    changes = int(port_changes.read(), 16)
                    for port_num in self._sfp_port:
                        change = (changes >> (port_num - 49)) & 1
                        if change == 1:
                            port_dict[str(port_num)] = str(
                                int(self.get_presence(port_num)))
                            found_flag = 1

                    if not found_flag:
                        return False, {}

                return True, port_dict

        finally:
            fd.close()
            epoll.close()

        return False, {}
