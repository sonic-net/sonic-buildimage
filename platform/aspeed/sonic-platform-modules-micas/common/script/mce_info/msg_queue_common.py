#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-


from multiprocessing.managers import BaseManager
from enum import IntEnum


# Event Type Enumeration
class EventType(IntEnum):
    EVENT_TYPE_A = 1
    EVENT_TYPE_B = 2
    EVENT_TYPE_C = 3
    EVENT_TYPE_D = 4
    EVENT_TYPE_E = 5
    EVENT_TYPE_F = 6
    EVENT_TYPE_END = 7

# Manager class definition
class QueueManager(BaseManager):
    pass

