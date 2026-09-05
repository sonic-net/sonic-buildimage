# led_control.py
#
# Platform-specific LED control functionality for SONiC
#

try:
    from sonic_led.led_control_base import LedControlBase
    import threading
    import os
    import logging
    import struct
    import time
    import syslog
    from socket import *
    from select import *
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


def DBG_PRINT(str):
    syslog.openlog("centec-led")
    syslog.syslog(syslog.LOG_INFO, str)
    syslog.closelog()


class LedControl(LedControlBase):
    """Platform specific LED control class"""
    SONIC_PORT_NAME_PREFIX = "Ethernet"
    LED_MODE_UP = [11, 1]
    LED_MODE_DOWN = [7, 2]

    def _initSystemLed(self):
        try:
            with open(self.f_led.format("system"), 'w') as led_file:
                led_file.write("5")
            DBG_PRINT("init system led to normal")
            with open(self.f_led.format("idn"), 'w') as led_file:
                led_file.write("1")
            DBG_PRINT("init idn led to off")
        except IOError as e:
            DBG_PRINT(str(e))

    def _initPanelLed(self):
        with open(self.f_led.format("port1"), 'r') as led_file:
            shouldInit = (int(led_file.read()) == 0)

        if shouldInit == True:
            for (port, ctlid, defmode) in self.led_mapping[1:59]:
                data = struct.pack('=HHHBBH', 0, 7, 4, ctlid, defmode, port)
                self.udpClient.sendto(data, ('localhost', 8101))

            data = struct.pack('=HHHBB34B', 0, 3, 36, 34, 0, *[x[0] for x in self.led_mapping[1:35]])
            self.udpClient.sendto(data, ('localhost', 8101))
            data = struct.pack('=HHHBB24B', 0, 3, 26, 24, 1, *[x[0] for x in self.led_mapping[35:59]])
            self.udpClient.sendto(data, ('localhost', 8101))

            data = struct.pack('=HHHB', 0, 5, 1, 1)
            self.udpClient.sendto(data, ('localhost', 8101))

            for idx in range(1, 55):
                (port, ctlid, defmode) = self.led_mapping[idx]
                with open(self.f_led.format("port{}".format(idx)), 'w') as led_file:
                    led_file.write(str(defmode))
                    DBG_PRINT("init port{} led to mode={}".format(idx, defmode))

        for idx in range(1, 55):
            (port, ctlid, defmode) = self.led_mapping[idx]
            with open(self.f_led.format("port{}".format(idx)), 'r') as led_file:
                defmode = int(led_file.read())
                data = struct.pack('=HHHBBH', 0, 7, 4, ctlid, defmode, port)
                self.udpClient.sendto(data, ('localhost', 8101))
                DBG_PRINT("init port{} led to mode={}".format(idx, defmode))

    def _initDefaultConfig(self):
        DBG_PRINT("start init led")
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

        self._initSystemLed()
        self._initPanelLed()

        DBG_PRINT("init led done")

    # Helper method to map SONiC port name to index

    def _port_name_to_index(self, port_name):
        # Strip "Ethernet" off port name
        if not port_name.startswith(self.SONIC_PORT_NAME_PREFIX):
            return -1

        port_idx = int(port_name[len(self.SONIC_PORT_NAME_PREFIX):])
        return port_idx

    def _port_state_to_mode(self, port_idx, state):
        if state == "up":
            return self.LED_MODE_UP[0] if (port_idx < 49) else self.LED_MODE_UP[1]
        else:
            return self.LED_MODE_DOWN[0] if (port_idx < 49) else self.LED_MODE_DOWN[1]

    def _port_led_mode_update(self, port_idx, ledMode):
        with open(self.f_led.format("port{}".format(port_idx)), 'w') as led_file:
            led_file.write(str(ledMode))
        (port, ctlid) = (self.led_mapping[port_idx][0], self.led_mapping[port_idx][1])
        data = struct.pack('=HHHBBH', 0, 7, 4, ctlid, ledMode, port)
        self.udpClient.sendto(data, ('localhost', 8101))

    # Concrete implementation of port_link_state_change() method
    def port_link_state_change(self, portname, state):
        port_idx = self._port_name_to_index(portname)
        ledMode = self._port_state_to_mode(port_idx, state)
        with open(self.f_led.format("port{}".format(port_idx)), 'r') as led_file:
            saveMode = int(led_file.read())

        if ledMode == saveMode:
            return

        self._port_led_mode_update(port_idx, ledMode)
        DBG_PRINT("update {} led mode from {} to {}".format(portname, saveMode, ledMode))

    # Constructor
    def __init__(self):
        # [macid, ctlid, defaultmode]
        self.led_mapping = [(0, 0, 0)]  # resv
        self.led_mapping.extend([(4,  0, 7), (5,  0, 7), (6,  0, 7), (8,  0, 7), (9,  0, 7),
                                 (10, 0, 7), (12, 0, 7), (13, 0, 7)])  # panel port 1~8
        self.led_mapping.extend([(14, 0, 7), (16, 0, 7), (17, 0, 7), (18, 0, 7), (20, 0, 7),
                                 (21, 0, 7), (22, 0, 7), (24, 0, 7)])  # panel port 9~16
        self.led_mapping.extend([(25, 0, 7), (26, 0, 7), (28, 0, 7), (30, 0, 7), (31, 0, 7),
                                 (32, 0, 7), (34, 0, 7), (35, 0, 7)])  # panel port 17~24
        self.led_mapping.extend([(48, 0, 7), (49, 0, 7), (51, 0, 7), (36, 0, 7), (37, 0, 7),
                                 (39, 0, 7), (55, 0, 7), (54, 0, 7)])  # panel port 25~32
        self.led_mapping.extend([(53, 0, 7), (52, 0, 7), (52, 1, 7), (53, 1, 7), (54, 1, 7),
                                 (55, 1, 7), (38, 1, 7), (37, 1, 7)])  # panel port 33~40
        self.led_mapping.extend([(36, 1, 7), (51, 1, 7), (50, 1, 7), (49, 1, 7), (48, 1, 7),
                                 (34, 1, 7), (33, 1, 7), (32, 1, 7)])  # panel port 41~48
        self.led_mapping.extend([(28, 1, 2), (24, 1, 2), (20, 1, 2), (12, 1, 2), (8,  1, 2),
                                 (4,  1, 2)])                          # panel port 49~54
        self.led_mapping.extend([(0,  1, 2), (0,  1, 2), (0,  1, 2), (0,  1, 2), (0,  1, 2), (0,  1, 2)])

        self.f_led = "/sys/class/leds/{}/brightness"

        self.udpClient = socket(AF_INET, SOCK_DGRAM)

        self._initDefaultConfig()
