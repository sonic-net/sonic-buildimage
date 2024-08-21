import logging
from logging.handlers import SysLogHandler
import os
import socket
import sys

# customize python logging to support notice logger
logging.NOTICE = logging.INFO + 1
logging.addLevelName(logging.NOTICE, "NOTICE")
SysLogHandler.priority_map['NOTICE'] = 'notice'


class SysLogger:
    """
    SysLogger class for Python applications using SysLogHandler
    """

    DEFAULT_LOG_FACILITY = SysLogHandler.LOG_USER
    DEFAULT_LOG_LEVEL = logging.NOTICE

    def __init__(self, log_identifier=None, log_facility=DEFAULT_LOG_FACILITY, log_level=DEFAULT_LOG_LEVEL):
        if log_identifier is None:
            log_identifier = os.path.basename(sys.argv[0])

        # Initialize SysLogger
        self.logger = logging.getLogger(log_identifier)

        # Reset all existing handlers
        for handler in self.logger.handlers[:]:
            self.logger.removeHandler(handler)

        handler = SysLogHandler(address="/dev/log", facility=log_facility, socktype=socket.SOCK_DGRAM)
        formatter = logging.Formatter('%(name)s[%(process)d]: %(message)s')
        handler.setFormatter(formatter)
        self.logger.addHandler(handler)

        self.set_min_log_priority(log_level)

    def set_min_log_priority(self, priority):
        """
        Sets the minimum log priority level. All log messages
        with a priority lower than this will not be logged
        """
        self._min_log_level = priority
        self.logger.setLevel(priority)

    # Methods for logging messages
    def log(self, priority, msg, also_print_to_console=False):
        self.logger.log(priority, msg)

        if also_print_to_console:
            print(msg)

    # Convenience methods
    def log_error(self, msg, also_print_to_console=False):
        self.log(logging.ERROR, msg, also_print_to_console)

    def log_warning(self, msg, also_print_to_console=False):
        self.log(logging.WARNING, msg, also_print_to_console)

    def log_notice(self, msg, also_print_to_console=False):
        self.log(logging.NOTICE, msg, also_print_to_console)

    def log_info(self, msg, also_print_to_console=False):
        self.log(logging.INFO, msg, also_print_to_console)

    def log_debug(self, msg, also_print_to_console=False):
        self.log(logging.DEBUG, msg, also_print_to_console)
    