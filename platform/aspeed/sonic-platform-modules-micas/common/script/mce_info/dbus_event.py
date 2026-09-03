#!/usr/bin/python3
# -*- coding: UTF-8 -*-


import dbus
import dbus.service
import dbus.mainloop.glib
import threading
import logging
import os


# Default log shutdown
log_level_str = os.getenv('DBUS_EVENT_LOG_LEVEL', 'NOEXIST').upper()
log_level = getattr(logging, log_level_str, logging.CRITICAL + 1)

logging.basicConfig(level=log_level, format='%(asctime)s [%(levelname)s] %(filename)s:%(lineno)d %(message)s')
logger = logging.getLogger(__name__)


# Used to determine Dbus objects
BUS_NAME = 'xyz.openbmc_project.BspService'
OBJECT_PATH = '/xyz/openbmc_project/BspService'
INTERFACE_NAME= 'xyz.openbmc_project.BspService'

_publisher = None
_publisher_lock = threading.Lock()

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

# Publisher class, used for sending signals
class Publisher(dbus.service.Object):
    def __init__(self, bus, object_path):
        super().__init__(bus, object_path)

    @dbus.service.signal(dbus_interface=INTERFACE_NAME, signature='i')
    def xmit_int_msg(self, int_num):
        pass

    @dbus.service.signal(dbus_interface=INTERFACE_NAME, signature='s')
    def xmit_str_msg(self, str_msg):
        pass

    @dbus.service.signal(dbus_interface=INTERFACE_NAME, signature='a(iiiis)')
    def BspSignal(self, list_of_structs):
        pass


def _init_publisher():
    global _publisher
    if _publisher is None:
        with _publisher_lock:
            if _publisher is None:
                bus = dbus.SystemBus()
                name = dbus.service.BusName(BUS_NAME, bus)
                try:
                    _publisher = Publisher(bus, OBJECT_PATH)
                except Exception as e:
                    logger.error(f"Failed to create Publisher object: {e}")
                    return None
                return _publisher
    return _publisher


def event_notify_by_dbus(event):
    """By D-Bus send event"""
    publisher = _init_publisher()
    if publisher is None:
        logger.error("publisher is none")
        return -1

    try:
        if isinstance(event, int):
            publisher.xmit_int_msg(event)
            logger.info(f"Published int event: {event}")
        elif isinstance(event, str):
            publisher.xmit_str_msg(event)
            logger.info(f"Published str event: {event}")
        elif isinstance(event, list):
            struct_list = []
            for d in event:
                signal_type = int(d.get('signal_type', 0))
                event_type = int(d.get('event_type', 0))
                event_level = int(d.get('event_level', 0))
                assert_type = int(d.get('assert_type', 0))
                event_detail = str(d.get('event_detail', ''))
                struct_list.append((signal_type, event_type, event_level, assert_type, event_detail))
            publisher.BspSignal(struct_list)
            logger.info(f"Published list of dict event: {struct_list}")
        else:
            logger.error("event is not int, str or list type, its type:%s" % (type(event)))
            return -1
    except Exception as e:
        logger.error(f"Failed to publish event: {e}")
        return -1
    return 0


if __name__ == '__main__':
    event_notify_by_dbus(1)
    event_notify_by_dbus(2)
    event_notify_by_dbus("abcd")
    event_notify_by_dbus("hello")

    # Test the dictionary data of the sending list
    test_data = [
        {
            'signal_type': 1,
            'event_type': 100,
            'event_level': 3,
            'assert_type': 0,
            'event_detail': 'detail A'
        },
        {
            'signal_type': 2,
            'event_type': 200,
            'event_level': 5,
            'assert_type': 1,
            'event_detail': 'detail B'
        }
    ]
    event_notify_by_dbus(test_data)
