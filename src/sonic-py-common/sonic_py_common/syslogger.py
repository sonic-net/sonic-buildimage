import os
import sys
import logging
from logging.handlers import SysLogHandler


class SysLogger:
    """
    SysLogger class for Python applications using SysLogHandler
    """

    DEFAULT_LOG_FACILITY = SysLogHandler.LOG_USER
    DEFAULT_LOG_LEVEL = SysLogHandler.LOG_INFO

    def __init__(self, log_identifier=None, log_facility=DEFAULT_LOG_FACILITY, log_level=DEFAULT_LOG_LEVEL):
        if log_identifier is None:
            log_identifier = os.path.basename(sys.argv[0])

        # Initialize SysLogger
        self.logger = logging.getLogger(log_identifier)

        # Reset all existing handlers
        for handler in self.logger.handlers[:]:
            self.logger.removeHandler(handler)

        handler = SysLogHandler(facility=log_facility)
        formatter = logging.Formatter("%(name)s: %(message)s")
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
        self.log(SysLogHandler.LOG_ERR, msg, also_print_to_console)

    def log_warning(self, msg, also_print_to_console=False):
        self.log(SysLogHandler.LOG_WARNING, msg, also_print_to_console)

    def log_notice(self, msg, also_print_to_console=False):
        self.log(SysLogHandler.LOG_NOTICE, msg, also_print_to_console)

    def log_info(self, msg, also_print_to_console=False):
        self.log(SysLogHandler.LOG_INFO, msg, also_print_to_console)

    def log_debug(self, msg, also_print_to_console=False):
        self.log(SysLogHandler.LOG_DEBUG, msg, also_print_to_console)
