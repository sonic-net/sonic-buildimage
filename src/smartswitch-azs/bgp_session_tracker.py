#!/usr/bin/env python3

from bgp_session_tracker_base import *
from sonic_py_common import daemon_base, logger
from swsscommon import swsscommon
import signal

SYSLOG_IDENTIFIER = "bgp-session-tracker"
logger_help = logger.Logger(SYSLOG_IDENTIFIER)
CONFIG_DB_BGP_DEVICE_GLOBAL_NAME = "BGP_DEVICE_GLOBAL"

bgp_session_tracker = None

def signal_handler(signal, frame):
    """
    Gracefully exit
    """
    logger_help.log_notice("BGP Session tracker going to quit")
    # Ignore any exception during quit
    try:
        if bgp_session_tracker:
            bgp_session_tracker.stop()
    except:
        pass

def main():
    """
    The entry of bgp_session_tracker
    """
    logger_help.log_notice("BGP Session tracker starting")
    # Install signal handler
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    bgp_session_tracker = BgpSessionTracker()
    bgp_session_tracker.start()

if __name__ == "__main__":
    main()
