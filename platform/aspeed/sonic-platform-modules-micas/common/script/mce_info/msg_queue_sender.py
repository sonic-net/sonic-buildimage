#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-


from msg_queue_common import EventType
from msg_queue_client import put_event


# Simple test
if __name__ == '__main__':
    for i in range(5):
        put_event(EventType.EVENT_TYPE_A, f"hello from client{i}")
        put_event(EventType.EVENT_TYPE_A, i)
        put_event(EventType.EVENT_TYPE_B, f"hello from client{i}")
        put_event(EventType.EVENT_TYPE_B, i)
    print("put finished")