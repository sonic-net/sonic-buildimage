#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import sys
import os
from logging.handlers import RotatingFileHandler
from platform_config import BSP_COMMON_LOG_DIR
from platform_util import logging

'''
This script sets up logging for debug and record purposes, 
with rotation of log files to manage their size. 
It also includes functions to log error, info, debug, and record messages, 
with the debug messages being conditional based on a debug level flag.
'''

def logger_init(type):
    if not os.path.exists(BSP_COMMON_LOG_DIR):
        os.system("mkdir -p %s" % BSP_COMMON_LOG_DIR)
        os.system("sync")

    logger_name = f"{type}_debug_logger"
    globals()[logger_name] = logging.getLogger(f"{type}_debug")
    globals()[logger_name].setLevel(logging.DEBUG)
    handler = RotatingFileHandler(filename = BSP_COMMON_LOG_DIR + f"/{type}_info_debug.log", maxBytes = 1*1024*1024, backupCount = 1)
    handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
    handler.setLevel(logging.DEBUG)
    globals()[logger_name].addHandler(handler)

    logger_name = f"{type}_record_logger"
    globals()[logger_name] = logging.getLogger(f"{type}_record")
    globals()[logger_name].setLevel(logging.DEBUG)
    handler = RotatingFileHandler(filename = BSP_COMMON_LOG_DIR + f"/{type}_info_record.log", maxBytes = 1*1024*1024, backupCount = 1)
    globals()[logger_name].setLevel(logging.DEBUG)
    handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
    handler.setLevel(logging.DEBUG)
    globals()[logger_name].addHandler(handler)


def debug_level_init(type):
    DEBUG_FILE = f"/etc/.{type}_debug_flag"
    debuglevel = f"{type}_debuglevel"
    if os.path.exists(DEBUG_FILE):
        globals()[debuglevel] = 1
    else:
        globals()[debuglevel] = 0


def error_log(type, s):
    #s = s.decode('utf-8').encode('gb2312')
    logger_name = f"{type}_debug_logger"
    globals()[logger_name].error("LINE:%s, %s" % (sys._getframe(1).f_lineno, s))


def info_log(type, s):
    #s = s.decode('utf-8').encode('gb2312')
    logger_name = f"{type}_debug_logger"
    globals()[logger_name].info("LINE:%s, %s" % (sys._getframe(1).f_lineno, s))


def debug_log(type, s):
    #s = s.decode('utf-8').encode('gb2312')
    debuglevel = f"{type}_debuglevel"
    if globals()[debuglevel] == 1:
        logger_name = f"{type}_debug_logger"
        globals()[logger_name].debug("LINE:%s, %s" % (sys._getframe(1).f_lineno, s))


def record_log(type, s):
    #s = s.decode('utf-8').encode('gb2312')
    logger_name = f"{type}_record_logger"
    globals()[logger_name].info("%s" % (s))