#!/usr/bin/env python_nos
# -*- coding: UTF-8 -*-

import sys
import os
from logging.handlers import RotatingFileHandler
from platform_config import BSP_COMMON_LOG_DIR
from platform_util import logging

LOGGERS = {}
DEBUG_LEVELS = {}

'''
This script sets up logging for debug and record purposes, 
with rotation of log files to manage their size. 
It also includes functions to log error, info, debug, and record messages, 
with the debug messages being conditional based on a debug level flag.
'''

def logger_init(type):
    if not os.path.exists(BSP_COMMON_LOG_DIR):
        os.makedirs(BSP_COMMON_LOG_DIR, exist_ok=True)
        os.sync()

    logger_name = f"{type}_debug_logger"
    LOGGERS[logger_name] = logging.getLogger(f"{type}_debug")
    LOGGERS[logger_name].setLevel(logging.DEBUG)
    handler = RotatingFileHandler(filename = BSP_COMMON_LOG_DIR + f"/{type}_info_debug.log", maxBytes = 1*1024*1024, backupCount = 1)
    handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
    handler.setLevel(logging.DEBUG)
    LOGGERS[logger_name].addHandler(handler)

    logger_name = f"{type}_record_logger"
    LOGGERS[logger_name] = logging.getLogger(f"{type}_record")
    LOGGERS[logger_name].setLevel(logging.DEBUG)
    handler = RotatingFileHandler(filename = BSP_COMMON_LOG_DIR + f"/{type}_info_record.log", maxBytes = 1*1024*1024, backupCount = 1)
    LOGGERS[logger_name].setLevel(logging.DEBUG)
    handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
    handler.setLevel(logging.DEBUG)
    LOGGERS[logger_name].addHandler(handler)


def debug_level_init(type):
    DEBUG_FILE = f"/etc/.{type}_debug_flag"
    debuglevel = f"{type}_debuglevel"
    if os.path.exists(DEBUG_FILE):
        DEBUG_LEVELS[debuglevel] = 1
    else:
        DEBUG_LEVELS[debuglevel] = 0


def error_log(type, s):
    #s = s.decode('utf-8').encode('gb2312')
    logger_name = f"{type}_debug_logger"
    LOGGERS[logger_name].error("LINE:%s, %s" % (sys._getframe(1).f_lineno, s))


def info_log(type, s):
    #s = s.decode('utf-8').encode('gb2312')
    logger_name = f"{type}_debug_logger"
    LOGGERS[logger_name].info("LINE:%s, %s" % (sys._getframe(1).f_lineno, s))


def debug_log(type, s):
    #s = s.decode('utf-8').encode('gb2312')
    debuglevel = f"{type}_debuglevel"
    if DEBUG_LEVELS.get(debuglevel, 0) == 1:
        logger_name = f"{type}_debug_logger"
        LOGGERS[logger_name].debug("LINE:%s, %s" % (sys._getframe(1).f_lineno, s))


def record_log(type, s):
    #s = s.decode('utf-8').encode('gb2312')
    logger_name = f"{type}_record_logger"
    LOGGERS[logger_name].info("%s" % (s))
