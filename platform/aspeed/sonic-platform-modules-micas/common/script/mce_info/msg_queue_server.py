#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-

import logging
from queue import Queue
from msg_queue_common import EventType, QueueManager

logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(filename)s:%(lineno)d %(message)s')
logger = logging.getLogger(__name__)


# Create a global queue dictionary, key: event_type, value: Queue example
event_queues = {}

def get_queue(event_type):
    return event_queues.get(event_type, None)

def create_queues():
    for et in range(EventType.EVENT_TYPE_A, EventType.EVENT_TYPE_END):
        event_queues[et] = Queue(maxsize=128)
    logger.info("All event queues created.")

# BaseManager.register(typeid, callable=None, proxytype=None, exposed=None, method_to_typeid=None, create_method=True)
# Register classes or methods that can be shared among different processes
# typeid: The string name that identifies the shared object
# callable: The callable object that is invoked when the client requests this type of object
QueueManager.register('get_queue', callable=get_queue)

def start_server(address=('localhost', 50000), authkey=b'abc'):
    create_queues()
    manager = QueueManager(address=address, authkey=authkey)
    server = manager.get_server()
    logger.info(f"QueueManager server started at {address}")
    server.serve_forever()

if __name__ == '__main__':
    start_server()
