#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-


import logging
from queue import Empty, Full
from msg_queue_common import EventType, QueueManager
import os
import logging
import threading


# Default log shutdown
log_level_str = os.getenv('MSG_QUEUE_CLIENT_LOG_LEVEL', 'NOEXIST').upper()
log_level = getattr(logging, log_level_str, logging.CRITICAL + 1)

logging.basicConfig(level=log_level, format='%(asctime)s [%(levelname)s] %(filename)s:%(lineno)d %(message)s')
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

QueueManager.register('get_queue')

# Cache QueueManager instance
_manager_instance = None
_manager_lock = threading.Lock()  # Used to ensure thread safety of _manager_instance

def connect_manager(address=('localhost', 50000), authkey=b'abc'):
    """
    Obtain the QueueManager instance using the singleton pattern to avoid frequent connections
    """
    global _manager_instance
    if _manager_instance is None:
        with _manager_lock:  # Locking protection
            if _manager_instance is None:  # Double-checking
                manager = QueueManager(address=address, authkey=authkey)
                try:
                    manager.connect()
                    logger.info(f"Connected to QueueManager at {address}")
                    _manager_instance = manager
                except Exception as e:
                    logger.error(f"Failed to connect to QueueManager: {e}")
                    raise
    return _manager_instance

def put_event(event_type, event):
    """
    Put the event into the specific event type queue
    """
    try:
        manager = connect_manager()
        queue = manager.get_queue(event_type)
        if queue is None:
            logger.error(f"No queue for event_type={event_type}")
            return -1

        if queue.full():
            discarded = queue.get(block=False)
            logger.warning(f"Queue full for event_type={event_type}, discarded oldest event: {discarded}")
        queue.put(event, block=False)
        logger.info(f"Put event {event} to queue for event_type={event_type}")
        return 0
    except Full:
        logger.error("Queue full, failed to put event")
        return -2
    except Exception as e:
        logger.error(f"Exception putting event: {e}")
        return -2

def get_event(event_type, block=True, timeout=None):
    """
    Retrieve events from the queue of specific event types
    """
    try:
        manager = connect_manager()
        queue = manager.get_queue(event_type)
        if queue is None:
            logger.error(f"No queue for event_type={event_type}")
            return None

        event = queue.get(block=block, timeout=timeout)
        logger.info(f"Got event {event} from queue for event_type={event_type}")
        return event
    except Empty:
        logger.info("Queue empty on get_event")
        return None
    except Exception as e:
        logger.error(f"Exception getting event: {e}")
        return None

# Simple test
if __name__ == '__main__':
    put_event(EventType.EVENT_TYPE_A, "hello from client1")
    event = get_event(EventType.EVENT_TYPE_A, block=False)
    print("Received event:", event)
    put_event(EventType.EVENT_TYPE_B, "hello from client2")
    event = get_event(EventType.EVENT_TYPE_B, block=False)
    print("Received event:", event)