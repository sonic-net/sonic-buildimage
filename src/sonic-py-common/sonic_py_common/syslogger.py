import logging
from logging.handlers import SysLogHandler
import os
import socket
import sys

CONFIG_DB = 'CONFIG_DB'
STATE_DB = 'STATE_DB'
# STATE_DB table of every registered component's effective log setting. It is
# shared with the swss-common C++ logger, which publishes LOGLEVEL and LOGOUTPUT
# there; this module also publishes require_manual_refresh. The name must match
# STATE_LOGGER_TABLE_NAME in swss-common's schema.h; it is not imported because
# older swsscommon bindings do not export it.
STATE_LOGGER_TABLE = 'LOGGER_TABLE'
FIELD_LOG_LEVEL = 'LOGLEVEL'
FIELD_LOG_OUTPUT = 'LOGOUTPUT'
FIELD_REQUIRE_REFRESH = 'require_manual_refresh'


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

    def __init__(self, log_identifier=None, log_facility=DEFAULT_LOG_FACILITY, log_level=DEFAULT_LOG_LEVEL, enable_runtime_config=False):
        self.log_identifier = log_identifier if log_identifier else os.path.basename(sys.argv[0]) 

        # Initialize SysLogger
        self.logger = logging.getLogger(self.log_identifier)

        # Reset all existing handlers
        for handler in self.logger.handlers[:]:
            self.logger.removeHandler(handler)

        handler = SysLogHandler(address="/dev/log", facility=log_facility, socktype=socket.SOCK_DGRAM)
        formatter = logging.Formatter('%(name)s[%(process)d]: %(message)s')
        handler.setFormatter(formatter)
        self.logger.addHandler(handler)

        self._default_log_level = log_level
        self.set_min_log_priority(log_level)
        
        if enable_runtime_config:
            ret, msg = self.update_log_level()
            if not ret:
                self.log_error(msg)
                            
    def update_log_level(self):
        """Refresh log level.

        The CONFIG_DB LOGGER row is operator intent and is only read: an absent
        row or LOGLEVEL field means the level this logger was created with. The
        effective level is published to STATE_DB, with require_manual_refresh
        telling the CLI that this logger does not pick up CONFIG_DB changes on
        its own. The level is applied even if the publish fails, so the
        operator's CONFIG_DB setting still takes effect.

        Returns:
            tuple: (refresh result, fail reason)
        """        
        from swsscommon import swsscommon
        try:
            db = swsscommon.SonicV2Connector(use_unix_socket_path=True)
            db.connect(CONFIG_DB)
            log_level_in_db = db.get(CONFIG_DB, f'{swsscommon.CFG_LOGGER_TABLE_NAME}|{self.log_identifier}', FIELD_LOG_LEVEL)
            if log_level_in_db:
                self.set_min_log_priority(self.log_priority_from_str(log_level_in_db))
            else:
                self.set_min_log_priority(self._default_log_level)

            db.connect(STATE_DB)
            data = {
                FIELD_LOG_LEVEL: self.log_priority_to_str(self._min_log_level),
                FIELD_LOG_OUTPUT: 'SYSLOG',
                FIELD_REQUIRE_REFRESH: 'true'
            }
            db.hmset(STATE_DB, f'{STATE_LOGGER_TABLE}|{self.log_identifier}', data)
            return True, ''
        except Exception as e:
            return False, f'Failed to refresh log configuration - {e}'

    def log_priority_to_str(self, priority):
        """Convert log priority to string.
        Args:
            priority (int): log priority.
        Returns:
            str: log priority in string.
        """
        if priority == logging.INFO:
            return 'INFO'
        elif priority == logging.NOTICE:
            return 'NOTICE'
        elif priority == logging.DEBUG:
            return 'DEBUG'
        elif priority == logging.WARNING:
            return 'WARN'
        elif priority == logging.ERROR:
            return 'ERROR'
        else:
            self.log_error(f'Invalid log priority: {priority}')
            return 'WARN'

    def log_priority_from_str(self, priority_in_str):
        """Convert log priority from string.
        Args:
            priority_in_str (str): log priority in string.
        Returns:
            _type_: log priority.
        """
        if priority_in_str == 'DEBUG':
            return logging.DEBUG
        elif priority_in_str == 'INFO':
            return logging.INFO
        elif priority_in_str == 'NOTICE':
            return logging.NOTICE
        elif priority_in_str == 'WARN':
            return logging.WARNING
        elif priority_in_str == 'ERROR':
            return logging.ERROR
        else:
            self.log_error(f'Invalid log priority string: {priority_in_str}')
            return logging.WARNING

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
    