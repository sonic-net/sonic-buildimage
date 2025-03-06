#!/usr/bin/env python3
try:
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

SYSLOG_IDENTIFIER = "smartswitch-azs"
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)


def main():
    """
    The entry of smartswitch-azs
    """
    logger_helper.log_notice("Python script is running.")
    print("Yay python sample script is running!")
    
    return 0
