import syslog

from .vars import g_debug

def log_debug(msg):
    """ Send a message msg to the syslog as DEBUG """
    if g_debug:
        syslog.syslog(syslog.LOG_DEBUG, msg)


def log_notice(msg):
    """ Send a message msg to the syslog as NOTICE """
    syslog.syslog(syslog.LOG_NOTICE, msg)


def log_info(msg):
    """ Send a message msg to the syslog as INFO """
    syslog.syslog(syslog.LOG_INFO, msg)


def log_warn(msg):
    """ Send a message msg to the syslog as WARNING """
    syslog.syslog(syslog.LOG_WARNING, msg)


def log_err(msg):
    """ Send a message msg to the syslog as ERR """
    syslog.syslog(syslog.LOG_ERR, msg)


def log_crit(msg):
    """ Send a message msg to the syslog as CRIT """
    syslog.syslog(syslog.LOG_CRIT, msg)