#!/usr/bin/env python

#############################################################################
# Centec
#
# Module contains an implementation of sfp presence scan logic
#
#############################################################################

try:
    import os
    import os.path
    import threading
    import time
    import logging
    import struct
    import syslog
    from swsscommon import swsscommon
    from socket import *
    from select import *
except ImportError, e:
    raise ImportError(str(e) + " - required module not found")


def DBG_PRINT(str):
    syslog.openlog("centec-pmon")
    syslog.syslog(syslog.LOG_INFO, str)
    syslog.closelog()

PORT_NUMBER = (48+6)

class PlatformMonitor:

    """init board platform default config"""
    def __init__(self):
        """[ctlid, slavedevid]"""
        self.fiber_mapping = [(0, 0)]  # res
        self.fiber_mapping.extend([(0,  0), (0,  1), (0,  2), (0,  3), (0,  4), (0,  5), (0,  6), (0,  7)])  # panel port 1~8
        self.fiber_mapping.extend([(0, 14), (0, 13), (0, 15), (0, 12), (0,  8), (0, 11), (0,  9), (0, 10)])  # panel port 9~16
        self.fiber_mapping.extend([(0, 22), (0, 21), (0, 23), (0, 20), (0, 16), (0, 19), (0, 17), (0, 18)])  # panel port 17~24
        self.fiber_mapping.extend([(1,  4), (1,  3), (1,  5), (1,  2), (1,  6), (1,  1), (1,  7), (1,  0)])  # panel port 25~32
        self.fiber_mapping.extend([(1,  8), (1, 15), (1,  9), (1, 14), (1, 10), (1, 13), (1, 11), (1, 12)])  # panel port 33~40
        self.fiber_mapping.extend([(1, 22), (1, 21), (1, 23), (1, 20), (1, 16), (1, 19), (1, 17), (1, 18)])  # panel port 41~48
        self.fiber_mapping.extend([(1, 28), (1, 29), (1, 26), (1, 27), (1, 24), (1, 25)])                    # panel port 49~54

        self.udpClient = socket(AF_INET, SOCK_DGRAM)
        self.sfp_present = [0]*(PORT_NUMBER+1)
        self.sfp_enable = [0]*(PORT_NUMBER+1)
        self.f_sfp_present = "/sys/class/sfp/sfp{}/sfp_presence"
        self.f_sfp_enable = "/sys/class/sfp/sfp{}/sfp_enable"
        self.sfp_scan_timer = 0

    def is_qsfp(self, port):
        if port <= 48:
            return False
        else:
            return True

    def get_sfp_present(self, port):
        with open(self.f_sfp_present.format(port), 'r') as sfp_file:
            return int(sfp_file.read())

    def set_sfp_present(self, port, present):
        self.sfp_present[port] = present

    def set_sfp_enable(self, port, enable):
        if self.is_qsfp(port):
            if enable:
                with open(self.f_sfp_enable.format(port), 'w') as sfp_file:
                    sfp_file.write("1")
                self.sfp_enable[port] = 1
            else:
                with open(self.f_sfp_enable.format(port), 'w') as sfp_file:
                    sfp_file.write("0")
                self.sfp_enable[port] = 0
        else:
            (ctlid, devid) = self.fiber_mapping[port]
            req = struct.pack('=HHHBBHIBBBBI', 0, 9, 16, ctlid, devid, 0x50, 0, 0x56, 1, 0xf, 0, 1)
            self.udpClient.sendto(req, ('localhost', 8101))
            rsp, addr = self.udpClient.recvfrom(1024)
            rsp_data = struct.unpack('=HHHBBHIBBBBIi512B', rsp)
            enable_v = rsp_data[13]
            if enable:
                enable_v &= 0xf0
            else:
                enable_v |= 0x0f
            data = struct.pack('=HHHBBHBBBB', 0, 11, 8, ctlid, 0x56, 0x50, devid, enable_v, 0xf, 0)
            self.udpClient.sendto(data, ('localhost', 8101))
        DBG_PRINT("set sfp{} to {}".format(port, ("enable" if enable else "disable")))

    def initialize_configdb(self):
        try:
            f_mac = os.popen('ip link show eth0 | grep ether | awk \'{print $2}\'')
            mac_addr = f_mac.read(17)
            last_byte = mac_addr[-2:]
            aligned_last_byte = format(int(int(str(last_byte), 16) + 1), '02x')
            mac_addr = mac_addr[:-2] + aligned_last_byte
            DBG_PRINT("start connect swss config-db to set device mac-address")
            swss = swsscommon.SonicV2Connector()
            swss.connect(swss.CONFIG_DB)
            swss.set(swss.CONFIG_DB, "DEVICE_METADATA|localhost", 'mac', mac_addr)
            mac_addr = swss.get(swss.CONFIG_DB, "DEVICE_METADATA|localhost", 'mac')
            DBG_PRINT("set device mac-address: %s" % mac_addr)
        except IOError as e:
            DBG_PRINT(str(e))

    def initialize_rpc(self):
        while True:
            try:
                r_sel = [self.udpClient]
                echo_req = struct.pack('=HHH', 0, 1, 0)
                self.udpClient.sendto(echo_req, ('localhost', 8101))
                result = select(r_sel, [], [], 1)
                if self.udpClient in result[0]:
                    echo_rsp, srv_addr = self.udpClient.recvfrom(1024)
                    if echo_rsp:
                        break
                DBG_PRINT("connect to sdk rpc server timeout, try again.")
            except IOError as e:
                DBG_PRINT(str(e))

        DBG_PRINT("connect to sdk rpc server success.")

    def initialize_gpio(self):
        # set gpio 1,2,3,4,5,6,7,8 output mode
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 1, 1, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 1, 2, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 1, 3, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 1, 4, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 1, 5, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 1, 6, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 1, 7, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 1, 8, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        # set gpio 1,2,3,4,5,6,7,8 output 0 to reset i2c bridge
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 1, 0)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 2, 0)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 3, 0)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 4, 0)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 5, 0)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 6, 0)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 7, 0)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 8, 0)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        # set gpio 1,2,3,4,5,6,7,8 output 1 to release i2c bridge
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 1, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 2, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 3, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 4, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 5, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 6, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 7, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        gpio_init = struct.pack('=HHHBBB', 0, 13, 3, 2, 8, 1)
        self.udpClient.sendto(gpio_init, ('localhost', 8101))
        DBG_PRINT("config ctc chip gpio done.")

    def initialize_sfp(self):
        try:
            for port in range(1, PORT_NUMBER+1):
                if self.get_sfp_present(port):
                    self.set_sfp_present(port, 1)
                    self.set_sfp_enable(port, 1)
                else:
                    self.set_sfp_present(port, 0)
                    self.set_sfp_enable(port, 0)
        except IOError as e:
            DBG_PRINT(str(e))

    def initialize(self):
        DBG_PRINT("start connect to sdk rpc server.")

        self.initialize_configdb()
        self.initialize_rpc()
        self.initialize_gpio()
        self.initialize_sfp()

    def sfp_scan(self):
        try:
            for port in range(1, PORT_NUMBER+1):
                cur_present = self.get_sfp_present(port)
                if self.sfp_present[port] != cur_present:
                    self.set_sfp_present(port, cur_present)
                    self.set_sfp_enable(port, cur_present)
        except IOError as e:
            DBG_PRINT(str(e))

    def start(self):
        while True:
            self.sfp_scan()
            time.sleep(1)

if __name__ == "__main__":
    monitor = PlatformMonitor()
    monitor.initialize()
    monitor.start()

