#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-


from msg_queue_common import EventType
from msg_queue_client import get_event


# Simple test
if __name__ == '__main__':
    for i in range(10):
        event = get_event(EventType.EVENT_TYPE_A, block=True)
        print("get eventA:", event)
        
        event = get_event(EventType.EVENT_TYPE_B, block=True)
        print("get eventB:", event)
    print("get finished")
