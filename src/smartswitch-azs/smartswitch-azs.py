#!/usr/bin/env python3
import signal
import time
import sys
from sonic_py_common import logger


SYSLOG_IDENTIFIER = "smartswitch-azs"
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)

# Global flag
running = True

def signal_hanlder(sig, frame):
    """
    Handle signals gracefully
    """
    global running
    logger_helper.log_notice(f"Received signal {sig}, shutting down")
    running = False

def main():
    """
    The entry point of smartswitch-azs service
    """
    # Register signal handlers for graceful shutdown
    signal.signal(signal.SIGINT, signal_hanlder)
    signal.signal(signal.SIGTERM, signal_hanlder)

    logger_helper.log_notice("smartswitch-azs service is starting")
    print("Yay python sample script is running!")

    try:
        while running:
            # Keep the service alive to pass the tests
            time.sleep(1)
    except Exception as e:
        logger_helper.log_notice(f"Unhandled exception: {str(e)}")
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())