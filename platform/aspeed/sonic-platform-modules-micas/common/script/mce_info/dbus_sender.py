#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import time
from dbus_event import event_notify_by_dbus

if __name__ == '__main__':
    for i in range(9):
        event_notify_by_dbus(i)
        event_notify_by_dbus("hello" + str(i))
        time.sleep(1)