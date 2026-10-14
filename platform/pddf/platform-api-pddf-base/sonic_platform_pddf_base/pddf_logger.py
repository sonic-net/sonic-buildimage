#!/usr/bin/env python3

from functools import partialmethod
import logging
import time
from sonic_py_common.syslogger import SysLogger


class PddfLogger(SysLogger):
    """
    Extension on top of SysLogger to provide rate limited logging
    """

    def __init__(self, log_identifier=None, log_facility=SysLogger.DEFAULT_LOG_FACILITY,
                 log_level=SysLogger.DEFAULT_LOG_LEVEL, enable_runtime_config=False, log_interval=30):
        super().__init__(log_identifier, log_facility, log_level, enable_runtime_config)
        self._last_log_time = {}
        self._log_count = {}
        self._rate_limit_log_interval = log_interval

    def log(self, priority, msg, also_print_to_console=False, rate_limit=False, limit_key=None, max_log_count=None):
        """Method for logging rate limited messages
        Args:
            priority: Log priority
            msg: Message to log
            rate_limit: Whether to rate limit this log or not
            limit_key: Key to rate limit on
            max_log_count: Max number of times to log the message
        """
        if rate_limit and limit_key:
            now = time.monotonic()
            last_log_time = self._last_log_time.get(limit_key, None)
            if last_log_time and now - last_log_time < self._rate_limit_log_interval:
                return
            self._last_log_time[limit_key] = now
        if max_log_count:
            log_count = self._log_count.get(limit_key, 0)
            if log_count >= max_log_count:
                return
            self._log_count[limit_key] = log_count + 1
        super().log(priority, msg, also_print_to_console)

    log_error = partialmethod(log, logging.ERROR)

    log_warning = partialmethod(log, logging.WARNING)

    log_notice = partialmethod(log, logging.NOTICE)

    log_info = partialmethod(log, logging.INFO)

    log_debug = partialmethod(log, logging.DEBUG)


logger = PddfLogger("pddf", enable_runtime_config=True)
