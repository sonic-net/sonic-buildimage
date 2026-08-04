# sfputil.py
#
# Platform-specific SFP transceiver interface for SONiC
#

try:
    import time
    import os
    import logging
    import struct
    import syslog
    from socket import *
    from select import *
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


def DBG_PRINT(str):
    print(str + "\n")


class SfpUtil(SfpUtilBase):
    """Platform-specific SfpUtil class"""
    SONIC_PORT_NAME_PREFIX = "Ethernet"
    PORT_START = 1
    PORT_END = 54
    PORTS_IN_BLOCK = 54

    @property
    def port_start(self):
        return self.PORT_START

    @property
    def port_end(self):
        return self.PORT_END

    @property
    def qsfp_ports(self):
        return list(range(49, self.PORTS_IN_BLOCK + 1))

    @property
    def port_to_eeprom_mapping(self):
        return self.eeprom_mapping

    def is_logical_port(self, port_name):
        return True

    def get_logical_to_physical(self, port_name):
        if not port_name.startswith(self.SONIC_PORT_NAME_PREFIX):
            return None

        port_idx = int(port_name[len(self.SONIC_PORT_NAME_PREFIX):])

        return [port_idx]

    def get_eeprom_data(self, port):
        (ctlid, devid) = self.fiber_mapping[port]
        offset = (128 if port in self.qsfp_ports else 0)
        r_sel = [self.udpClient]
        req = struct.pack('=HHHBBHIBBBBI',
                          0, 9, 16,  # lchip/msgtype/msglen
                          ctlid,   # uint8  ctl_id
                          devid,  # uint8  slave_dev_id
                          0x50,   # uint16 dev_addr
                          (1 << devid),   # uint32 slave_bitmap
                          offset,   # uint8  offset
                          95,   # uint8  length
                          0xf,   # uint8  i2c_switch_id
                          0,    # uint8  access_switch
                          95     # uint32  buf_length
                          )
        self.udpClient.sendto(req, ('localhost', 8101))
        result = select(r_sel, [], [], 1)
        if self.udpClient in result[0]:
            rsp, addr = self.udpClient.recvfrom(1024)
            if rsp:
                rsp_data = struct.unpack('=HHHBBHIBBBBIi512B', rsp)
                if rsp_data[12] != 0:
                    return None
                if port in self.qsfp_ports:
                    return buffer(bytearray([0]*128), 0, 128) + buffer(rsp, 26, 512)
                return buffer(rsp, 26, 512)
        return None

    def __init__(self):
        """[ctlid, slavedevid]"""
        self.fiber_mapping = [(0, 0)]  # res
        self.fiber_mapping.extend([(0,  0), (0,  1), (0,  2), (0,  3), (0,  4),
                                   (0,  5), (0,  6), (0,  7)])  # panel port 1~8
        self.fiber_mapping.extend([(0, 14), (0, 13), (0, 15), (0, 12), (0,  8),
                                   (0, 11), (0,  9), (0, 10)])  # panel port 9~16
        self.fiber_mapping.extend([(0, 22), (0, 21), (0, 23), (0, 20), (0, 16),
                                   (0, 19), (0, 17), (0, 18)])  # panel port 17~24
        self.fiber_mapping.extend([(1,  4), (1,  3), (1,  5), (1,  2), (1,  6),
                                   (1,  1), (1,  7), (1,  0)])  # panel port 25~32
        self.fiber_mapping.extend([(1,  8), (1, 15), (1,  9), (1, 14), (1, 10),
                                   (1, 13), (1, 11), (1, 12)])  # panel port 33~40
        self.fiber_mapping.extend([(1, 22), (1, 21), (1, 23), (1, 20), (1, 16),
                                   (1, 19), (1, 17), (1, 18)])  # panel port 41~48
        self.fiber_mapping.extend([(1, 28), (1, 29), (1, 26), (1, 27), (1, 24), (1, 25)]
                                  )                    # panel port 49~54

        self.udpClient = socket(AF_INET, SOCK_DGRAM)
        self.eeprom_mapping = {}
        self.f_sfp_present = "/sys/class/sfp/sfp{}/sfp_presence"
        self.f_sfp_enable = "/sys/class/sfp/sfp{}/sfp_enable"

        for x in range(1, self.port_end + 1):
            self.eeprom_mapping[x] = "/var/cache/sonic/sfp/sfp{}_eeprom".format(x)

        try:
            if not os.path.exists("/var/cache/sonic/sfp"):
                os.makedirs("/var/cache/sonic/sfp", 0o777)
            for x in range(1, self.port_end + 1):
                if not self.get_presence(x):
                    if os.path.exists(self.eeprom_mapping[x]):
                        os.remove(self.eeprom_mapping[x])
                    continue
                data = self.get_eeprom_data(x)
                if data:
                    with open(self.eeprom_mapping[x], 'w') as sfp_eeprom:
                        sfp_eeprom.write(data)
                else:
                    DBG_PRINT("get sfp{} eeprom data failed.".format(x))
                    break
        except IOError as e:
            DBG_PRINT(str(e))

        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self.port_start or port_num > self.port_end:
            return False
        try:
            with open(self.f_sfp_present.format(port_num), 'r') as sfp_file:
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

    def get_transceiver_change_event(self, timeout=0):
        return False, {}
