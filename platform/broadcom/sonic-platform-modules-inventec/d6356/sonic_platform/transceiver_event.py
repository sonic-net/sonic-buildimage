#!/usr/bin/env python
#
# Name: transceiver_event.py, version: 1.0
#

try:
    import time
    import socket
    import re
    import os
    from collections import OrderedDict
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


class NetlinkEventMonitor(object):
    __instance = None

    def __new__(cls, *args, **kwargs):
        if not cls.__instance:
            # print(cls)
            cls.__instance = super(NetlinkEventMonitor, cls).__new__(cls)
            cls.__instance.__recieved_events = OrderedDict()
        return cls.__instance

    def __init__(self, timeout):
        # print('__init__', self)
        NETLINK_KOBJECT_UEVENT = 15
        self.__socket  = socket.socket(socket.AF_NETLINK, socket.SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)
        self.__timeout = timeout

    def start(self):
        # print('start', self.__timeout)
        self.__socket.bind((os.getpid(), -1))
        if 0 == self.__timeout:
            self.__socket.settimeout(None)
        else:
            self.__socket.settimeout(self.__timeout/1000.0)

    def stop(self):
        self.__socket.close()

    def __enter__(self):
        # print('__enter__', self)
        self.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        # print('__exit__', self)
        self.stop()

    def __iter__(self):
        # print('__iter__', self)
        while True:
            for item in self.next_events():
                yield item

    def next_events(self):
        try:
            data = self.__socket.recv(16384)
            event = {}
            for item in data.split(b'\x00'):
                if not item:
                    # check if we have an event and if we already received it
                    if event and 'SEQNUM' in event:
                        event_seqnum = event['SEQNUM']
                        if event_seqnum in self.__recieved_events:
                            pass
                        else:
                            # print("=", event_seqnum)
                            self.__recieved_events[event_seqnum] = event
                            length = len(self.__recieved_events)
                            # print("=", length)
                            if (length > 100):
                                self.__recieved_events.popitem(last=False)
                            yield event
                    event = {}
                else:
                    try:
                        k, v = item.split(b'=', 1)
                        event[k.decode('ascii')] = v.decode('ascii')
                        # print("=",k,v)
                    except ValueError:
                        pass
        except Exception:
            yield {}

class TransceiverEvent(object):

    def __init__(self):
        pass

    def get_transceiver_change_event(self, timeout=0):
        port_dict = {}
        with NetlinkEventMonitor(timeout) as netlink_monitor:
            for event in netlink_monitor:
                if event and 'SUBSYSTEM' in event:
                    if event['SUBSYSTEM'] == 'swps':
                        #print('SWPS event. From %s, ACTION %s, IF_TYPE %s, IF_LANE %s' % (event['DEVPATH'], event['ACTION'], event['IF_TYPE'], event['IF_LANE']))
                        portname = event['DEVPATH'].split("/")[-1]
                        rc = re.match(r"port(?P<num>\d+)",portname)
                        if rc is not None:
                            if event['ACTION'] == "remove":
                                remove_num = int(rc.group("num"))
                                port_dict[remove_num] = "0"
                            elif event['ACTION'] == "add":
                                add_num = int(rc.group("num"))
                                port_dict[add_num] = "1"
                            return True, port_dict
                        else:
                            return False, {}
                    else:
                        pass
                else:
                    return True, {}

