#!/usr/bin/env python3
try:
    import signal
    import time
    import threading
    from queue import PriorityQueue, Empty
    from sonic_py_common import daemon_base, logger
    from swsscommon import swsscommon
    import configutil
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


# Set the syslog identifier for this container
SYSLOG_IDENTIFIER = "smartswitch-azs"
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)


# Global variables to hold service instances
g_bgp_session_tracker = None
g_link_state_tracker = None


# Signal Handler pattern for graceful shutdown
def signal_handler(signal, frame):
    """
    Gracefully exit
    """
    logger_helper.log_notice("Smartswitch-azs going to quit")
    # Ignore any exception during quit
    try:
        # Stop services
        if g_bgp_session_tracker:
            g_bgp_session_tracker.stop()
        if g_link_state_tracker:
            g_link_state_tracker.stop()
    except:
        logger_helper.log_warning(f"Exception during shutdown: {str(e)}")


class BGPSessionTracker:
    """
    Track BGP sessions and track their state
    """
    def __init__(self):
        self.working = False
        # TODO: Initialization DB connections
    
    def start(self):
        self.working = True
        logger_helper.log_notice("BGP session tracker started")
    
    def stop(self):
        self.working = False
        logger_helper.log_notice("BGP session tracker stopped")

class LinkStateTracker:
    """
    Track link states
    """
    def __init__(self):
        self.working = False
        # TODO: Initialization DB conections
    
    def start(self):
        self.working = True
        # Start tracking logic
        logger_helper.log_notice("Link state tracker started")
    
    def stop(self):
        self.working = False
        # Cleanup
        logger_helper.log_notice("Link state tracker stopped")


def main():
    """
    The entry of smartswitch-azs
    """
    # Install signal handler
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    try:
        global g_bgp_tracker, g_link_state_tracker

        # Initialize BGP session tracker
        g_bgp_session_tracker = BGPSessionTracker()
        g_bgp_session_tracker.start()

        # Initialize link state tracker
        g_link_state_tracker = LinkStateTracker()
        g_link_state_tracker.start()

        logger_helper.log_notice("Smartswitch-azs started")

    except Exception as e:
        logger_helper.log_error(f"Unhandled exception: {str(e)}")
        return 1
    finally:
        # Cleanup handled by signal handler
        logger_helper.log_notice("Smartswitch-azs exited")
    
    return 0
