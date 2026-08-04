#
# event_monitor.py
# Description: module to minitor events
#

try:
    import socket
    from collections import OrderedDict
    import os
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))


NETLINK_KOBJECT_UEVENT = 15


class EventMonitor:

    def __init__(self, timeout):
        self.recieved_events = OrderedDict()
        self.socket = socket.socket(
            socket.AF_NETLINK, socket.SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)
        self.timeout = timeout

    def start(self):
        self.socket.bind((os.getpid(), -1))

        if self.timeout == 0:
            self.socket.settimeout(None)
        else:
            self.socket.settimeout(self.timeout/1000.0)

    def stop(self):
        self.socket.close()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.stop()

    def __iter__(self):
        while True:
            for item in self.next_events():
                yield item

    def next_events(self):
        try:
            data = self.socket.recv(16384)
            event = {}
            for item in data.split(b'\x00'):
                if not item:
                    # check if we have an event and if we already received it
                    if event and event['SEQNUM'] not in self.recieved_events:
                        self.recieved_events[event['SEQNUM']] = None
                        if len(self.recieved_events) > 100:
                            self.recieved_events.popitem(last=False)
                        yield event
                    event = {}
                else:
                    try:
                        k, v = item.split(b'=', 1)
                        event[k.decode('ascii')] = v.decode('ascii')
                    except ValueError:
                        pass
        except socket.timeout:
            yield event

    def get_events(self):
        event = {}
        while True:
            try:
                data = self.socket.recv(16384)

                for item in data.split(b'\x00'):
                    if not item:
                        # check if we have an event and if we already received it
                        # if no item and event empty, means received garbled
                        if bool(event):
                            if event['SEQNUM'] not in self.recieved_events:
                                self.recieved_events[event['SEQNUM']] = None
                                if len(self.recieved_events) > 100:
                                    self.recieved_events.popitem(last=False)
                                return event
                            else:
                                event = {}
                    else:
                        try:
                            k, v = item.split(b'=', 1)
                            event[k] = v
                        except ValueError:
                            pass
            except socket.timeout:
                return event
