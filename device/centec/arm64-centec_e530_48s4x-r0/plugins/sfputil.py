#!/usr/bin/env python

# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    from socket import *
    from select import *
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


def DBG_PRINT(str):
    print(str + "\n")

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def sfp_base(self):
        return self.SFP_BASE

    @property
    def qsfp_ports(self):
        return ()

    @property
    def port_to_eeprom_mapping(self):
        return self.eeprom_mapping

    def is_logical_port(self, port_name):
        return True

    def get_eeprom_data(self, port):
        ret = None
        port_num = self.get_logical_to_physical(port)[0] + 1
        if port_num < self.port_start or port_num > self.port_end:
            return ret
        if port_num < self.sfp_base:
            return ret
        try:
            with open(self.eeprom_mapping[port_num], 'r') as eeprom_file:
                ret = eeprom_file.read()
        except IOError as e:
            DBG_PRINT(str(e))

        return ret

    def __init__(self):
        self.SONIC_PORT_NAME_PREFIX = "Ethernet"
        self.PORT_START = 0
        self.PORT_END = 51
        self.SFP_BASE = 0
        self.PORTS_IN_BLOCK = 52
        self.logical = []
        self.physical_to_logical = {}
        self.logical_to_physical = {}
        self.logical_to_asic = {}
        self.data = {'valid':0, 'last':0}

        self.eeprom_mapping = {}
        self.f_sfp_present = "/sys/class/sfp/sfp{}/sfp_presence"
        self.f_sfp_enable = "/sys/class/sfp/sfp{}/sfp_enable"
        for x in range(self.port_start, self.sfp_base):
            self.eeprom_mapping[x] = None
        for x in range(self.sfp_base, self.port_end + 1):
            self.eeprom_mapping[x] = "/sys/class/sfp/sfp{}/sfp_eeprom".format(x - self.sfp_base + 1)
        self.presence = {}
        for x in range(self.sfp_base, self.port_end + 1):
            self.presence[x] = False;

        SfpUtilBase.__init__(self)

        for x in range(self.sfp_base, self.port_end + 1):
            self.logical.append('Ethernet' + str(x))

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        if port_num < self.sfp_base:
            return False
        try:
            with open(self.f_sfp_present.format(port_num - self.sfp_base + 1), 'r') as sfp_file:
                return 1 == int(sfp_file.read())
        except IOError as e:
            DBG_PRINT(str(e))

        return False

    def get_low_power_mode(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        return False

    def set_low_power_mode(self, port_num, lpmode):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        return False

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False

        return False


    def read_porttab_mappings(self, porttabfile, asic_inst = 0):
        for x in range(self.sfp_base, self.port_end + 1):
            self.logical_to_physical['Ethernet' + str(x)] = [x]
            self.logical_to_asic['Ethernet' + str(x)] = 0
            self.physical_to_logical[x] = ['Ethernet' + str(x)]

    def get_transceiver_change_event(self, timeout=2000):
        now = time.time()
        port_dict = {}

        if timeout < 1000:
            timeout = 1000
        timeout = (timeout) / float(1000) # Convert to secs

        if now < (self.data['last'] + timeout) and self.data['valid']:
            return True, {}

        for x in range(self.sfp_base, self.port_end + 1):
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
