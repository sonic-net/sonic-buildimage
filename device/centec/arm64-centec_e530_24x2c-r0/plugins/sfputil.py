#!/usr/bin/env python

# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import os
    import re
    import time
    import collections
    from sonic_sfp.sfputilbase import SfpUtilBase
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'
USR_SHARE_SONIC_PATH = "/usr/share/sonic"
HOST_DEVICE_PATH = USR_SHARE_SONIC_PATH + "/device"
CONTAINER_PLATFORM_PATH = USR_SHARE_SONIC_PATH + "/platform"

class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""

    def __init__(self):
        self.mac_to_sfp = {
            0:  1,
            1:  2,
            2:  3,
            3:  4,
            8:  5,
            9:  6,
            10: 7,
            11: 8,
            20: 9,
            21:10,
            22:11,
            23:12,
            12:13,
            13:14,
            14:15,
            15:16,
            24:17,
            25:18,
            26:19,
            27:20,
            28:21,
            29:22,
            30:23,
            31:24,
            61:25,
            60:25,
            63:25,
            62:25,
            45:26,
            44:26,
            47:26,
            46:26,
        }
        self.logical = []
        self.physical_to_logical = {}
        self.logical_to_physical = {}
        self.logical_to_asic = {}
        self.data = {'valid':0, 'last':0}
        self.f_sfp_present = "/sys/class/sfp/sfp{}/sfp_presence"
        self.f_sfp_enable = "/sys/class/sfp/sfp{}/sfp_enable"

        if os.path.isdir(CONTAINER_PLATFORM_PATH):
            platform_path = CONTAINER_PLATFORM_PATH
        else:
            platform = device_info.get_platform()
            if platform is None:
                raise
            platform_path = os.path.join(HOST_DEVICE_PATH, platform)

        port_config_file = "/".join([platform_path, "E530-24x2c", "port_config.ini"])
        try:
            f = open(port_config_file)
        except:
            raise
        for line in f:
            line.strip()
            if re.search('^#', line) is not None:
                Port_cfg = collections.namedtuple('Port_cfg', line.split()[1:])
                break
        f.close()
        f = open(port_config_file)
        self._port_cfgs = [Port_cfg(*tuple((line.strip().split())))
                           for line in f if re.search('^#', line) is None]
        f.close()

        self.PORT_START = 256
        self.PORT_END = 0
        for port_cfg in self._port_cfgs:
            if int(port_cfg.index) <= self.PORT_START:
                self.PORT_START = int(port_cfg.index)
            elif int(port_cfg.index) >= self.PORT_END:
                self.PORT_END = int(port_cfg.index)

        self.eeprom_mapping = {}
        self.presence = {}
        for port_cfg in self._port_cfgs:
            sfp_idx = self.mac_to_sfp[int(port_cfg.lanes.split(',')[0])]
            if sfp_idx > 0:
                self.eeprom_mapping[int(port_cfg.index)] = "/sys/class/sfp/sfp{}/sfp_eeprom".format(sfp_idx)
                self.logical.append(port_cfg.name)
            else:
                self.eeprom_mapping[int(port_cfg.index)] = None
            self.presence[int(port_cfg.index)] = False

        SfpUtilBase.__init__(self)

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def sfp_base(self):
        return self.PORT_START

    @property
    def qsfp_ports(self):
        start = 256
        end = 0
        for port_cfg in self._port_cfgs:
            sfp_idx = self.mac_to_sfp[int(port_cfg.lanes.split(',')[0])]
            if sfp_idx == 25 or sfp_idx == 26:
                if int(port_cfg.index) <= start:
                    start = int(port_cfg.index)
                elif int(port_cfg.index) >= end:
                    end = int(port_cfg.index)
        return range(start, end + 1)

    @property
    def port_to_eeprom_mapping(self):
        return self.eeprom_mapping

    def is_logical_port(self, port_name):
        return True

    def get_presence(self, port_num):
        for port_cfg in self._port_cfgs:
            if int(port_cfg.index) == port_num:
                sfp_idx = self.mac_to_sfp[int(port_cfg.lanes.split(',')[0])]
                if sfp_idx >= 0:
                    try:
                        with open(self.f_sfp_present.format(sfp_idx), 'r') as sfp_file:
                            return 1 == int(sfp_file.read())
                    except IOError as e:
                        DBG_PRINT(str(e))
        return False

    def get_low_power_mode(self, port_num):
        return False

    def set_low_power_mode(self, port_num, lpmode):
        return False

    def reset(self, port_num):
        return False

    def read_porttab_mappings(self, porttabfile, asic_inst = 0):
        for port_cfg in self._port_cfgs:
            self.logical_to_physical[port_cfg.name] = [int(port_cfg.index)]
            self.logical_to_asic[port_cfg.name] = 0
            self.physical_to_logical[int(port_cfg.index)] = [port_cfg.name]

    def get_transceiver_change_event(self, timeout=2000):
        now = time.time()
        port_dict = {}

        if timeout < 1000:
            timeout = 1000
        timeout = (timeout) / float(1000) # Convert to secs

        if now < (self.data['last'] + timeout) and self.data['valid']:
            return True, {}

        for port_cfg in self._port_cfgs:
            presence = self.get_presence(int(port_cfg.index))
            if presence != self.presence[int(port_cfg.index)]:
                self.presence[int(port_cfg.index)] = presence
                if presence:
                    port_dict[int(port_cfg.index)] = SFP_STATUS_INSERTED
                else:
                    port_dict[int(port_cfg.index)] = SFP_STATUS_REMOVED

        if bool(port_dict):
            self.data['last'] = now
            self.data['valid'] = 1
            return True, port_dict
        else:
            time.sleep(0.5)
            return True, {}
