import os
import sys
import syslog

"""
Logging functionality for SONiC Python applications
"""


class Logger(object):
    """
    Logger class for SONiC Python applications
    """
    LOG_FACILITY_DAEMON = syslog.LOG_DAEMON
    LOG_FACILITY_USER = syslog.LOG_USER

    LOG_OPTION_NDELAY = syslog.LOG_NDELAY
    LOG_OPTION_PID = syslog.LOG_PID

    LOG_PRIORITY_ERROR = syslog.LOG_ERR
    LOG_PRIORITY_WARNING = syslog.LOG_WARNING
    LOG_PRIORITY_NOTICE = syslog.LOG_NOTICE
    LOG_PRIORITY_INFO = syslog.LOG_INFO
    LOG_PRIORITY_DEBUG = syslog.LOG_DEBUG

    DEFAULT_LOG_FACILITY = LOG_FACILITY_USER
    DEFAULT_LOG_OPTION = LOG_OPTION_NDELAY

    def __init__(self, log_identifier=None, log_facility=DEFAULT_LOG_FACILITY, log_option=DEFAULT_LOG_OPTION,
                 enable_set_log_level_on_fly=False, db_name=None):
        if log_identifier is None:
            log_identifier = os.path.basename(sys.argv[0])
        if db_name is None:
            self.db_name = log_identifier
        else:
            self.db_name = db_name
        self.enable_set_log_level_on_fly = enable_set_log_level_on_fly

        # Initialize syslog
        syslog.openlog(ident=log_identifier, logoption=log_option, facility=log_facility)

        self._logger = None

    def __del__(self):
        syslog.closelog()
        
    @property
    def logger(self):
        if self._logger is None:
            try:
                from swsscommon.swsscommon import Logger as SwssLogger
            except ImportError:
                # Workaround for unit test. In some SONiC Python package, it mocked
                # swsscommon lib for unit test purpose, but it does not contain Logger
                # class. To make those unit test happy, here provides a MagicMock object.
                if sys.version_info.major == 3:
                    from unittest import mock
                else:
                    # Expect the 'mock' package for python 2
                    # https://pypi.python.org/pypi/mock
                    import mock
                SwssLogger = mock.MagicMock()
                instance = mock.MagicMock()
                SwssLogger.getInstance.return_value = instance
                instance.getMinPrio.return_value = syslog.LOG_NOTICE
            self._logger = SwssLogger.getInstance()
            if self.enable_set_log_level_on_fly:
                # Performance warning: linkToDbNative will potentially create a new thread.
                # The thread listens to CONFIG DB for log level changes.
                self._logger.linkToDbNative(self.db_name, 'NOTICE')
        return self._logger

    #
    # Methods for setting minimum log priority
    #

    def set_min_log_priority(self, priority):
        """
        Sets the minimum log priority level to <priority>. All log messages
        with a priority lower than <priority> will not be logged

        Args:
            priority: The minimum priority at which to log messages
        """
        self.logger.setMinPrio(priority)

    def set_min_log_priority_error(self):
        """
        Convenience function to set minimum log priority to LOG_PRIORITY_ERROR
        """
        self.set_min_log_priority(self.LOG_PRIORITY_ERROR)

    def set_min_log_priority_warning(self):
        """
        Convenience function to set minimum log priority to LOG_PRIORITY_WARNING
        """
        self.set_min_log_priority(self.LOG_PRIORITY_WARNING)

    def set_min_log_priority_notice(self):
        """
        Convenience function to set minimum log priority to LOG_PRIORITY_NOTICE
        """
        self.set_min_log_priority(self.LOG_PRIORITY_NOTICE)

    def set_min_log_priority_info(self):
        """
        Convenience function to set minimum log priority to LOG_PRIORITY_INFO
        """
        self.set_min_log_priority(self.LOG_PRIORITY_INFO)

    def set_min_log_priority_debug(self):
        """
        Convenience function to set minimum log priority to LOG_PRIORITY_DEBUG
        """
        self.set_min_log_priority(self.LOG_PRIORITY_DEBUG)

    #
    # Methods for logging messages
    #

    def log(self, priority, msg, also_print_to_console=False):
        if self.logger.getMinPrio() >= priority:
            # Send message to syslog
            self.logger.write(priority, msg)

            # Send message to console
            if also_print_to_console:
                print(msg)

    def log_error(self, msg, also_print_to_console=False):
        self.log(self.LOG_PRIORITY_ERROR, msg, also_print_to_console)

    def log_warning(self, msg, also_print_to_console=False):
        self.log(self.LOG_PRIORITY_WARNING, msg, also_print_to_console)

    def log_notice(self, msg, also_print_to_console=False):
        self.log(self.LOG_PRIORITY_NOTICE, msg, also_print_to_console)

    def log_info(self, msg, also_print_to_console=False):
        self.log(self.LOG_PRIORITY_INFO, msg, also_print_to_console)

    def log_debug(self, msg, also_print_to_console=False):
        self.log(self.LOG_PRIORITY_DEBUG, msg, also_print_to_console)
