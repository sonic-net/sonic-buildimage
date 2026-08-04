# -*- coding: UTF-8 -*-

import logging
from syslog import (
    syslog,
    openlog,
    LOG_WARNING,
    LOG_CRIT,
    LOG_DEBUG,
    LOG_ERR,
    LOG_PID,
    LOG_INFO,
)

class Logger():
    def __init__(self, prefix, filepath=None, syslog=False, dbg_mask=0x0):
        self.logger = None
        if syslog is False:
            if filepath is None:
                raise AttributeError("filepath needed")

            # init logging
            formatter = logging.Formatter( "%(asctime)s %(levelname)s %(filename)s[%(funcName)s][%(lineno)s]: %(message)s")
            handler = logging.FileHandler(self.filepath)
            handler.setFormatter(formatter)
            self.logger = logging.getLogger(__name__)
            self.logger.setLevel(logging.DEBUG)
            self.logger.addHandler(handler)

        self.prefix = prefix
        self.use_syslog = syslog
        self.dbg_mask = dbg_mask

    def info(self, s):
        if self.use_syslog:
            self._syslog(s, LOG_INFO)
        else:
            self.logger.info(s)

    def debug(self, dbg_lvl, s):
        if dbg_lvl & self.dbg_mask:
            if self.use_syslog:
                self._syslog(s, LOG_DEBUG)
            else:
                self.logger.debug(s)

    def warn(self, s):
        if self.use_syslog:
            self._syslog(s, LOG_WARNING)
        else:
            self.logger.warning(s)

    def error(self, s):
        if self.use_syslog:
            self._syslog(s, LOG_ERR)
        else:
            self.logger.error(s)

    def crit(self, s):
        if self.use_syslog:
            self._syslog(s, LOG_CRIT)
        else:
            self.logger.critical(s)

    def _syslog(self, s, t):
        openlog(self.prefix, LOG_PID)
        syslog(t, s)
