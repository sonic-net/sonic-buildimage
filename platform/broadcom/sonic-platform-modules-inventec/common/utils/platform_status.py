#!/usr/bin/env python

import os
import socket
import subprocess
from collections import OrderedDict

# Purpose:  Shutdown DUT upon receiving thermaltrip event from kernel (inv_pthread)

NETLINK_KOBJECT_UEVENT = 15

class KernelEventMonitor(object):

    def __init__(self):
        self.received_events = OrderedDict()
        self.socket = socket.socket(socket.AF_NETLINK, socket.SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)

    def start(self):
        self.socket.bind((os.getpid(), -1))

    def stop(self):
        self.socket.close()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.stop()

    def __iter__(self):
        while True:
          for item in monitor.next_events():
              yield item

    def next_events(self):
        data = self.socket.recv(16384)
        event = {}
        for item in data.split(b'\x00'):
            if not item:        
                #check if we have an event and if we already received it
                if event and event['SEQNUM'] not in self.received_events:
                    self.received_events[event['SEQNUM']] = None
                    if (len(self.received_events) > 100):
                        self.received_events.popitem(last=False)
                    yield event
                event = {}
            else:
                try:
                    k, v = item.split(b'=', 1)
                    event[k.decode('ascii')] = v.decode('ascii')
                except ValueError:
                    pass

if __name__ == '__main__':
    with KernelEventMonitor() as monitor:
        for event in monitor:
            if event['SUBSYSTEM'] == 'platform_status':
                print('subsystem is platform_status')

            # Receive thermaltrip event
            if event['ACTION'] == 'remove' and event['DEVPATH'] == '/kernel/platform_status/fan':
                subprocess.call(["shutdown", "-h", "now"])
                

