#!/usr/bin/env python

import syslog
import logging
from logging.handlers import RotatingFileHandler

DIAG_LOGGER_NAME = "sysdiag"
DIAG_LOG_DIR = "/var/log"
DIAG_LOG_SIZE = 20*1024*1024
DIAG_LOG_BACKUP = 5

# RAW syslog facilities
def sys_log_warn(module_name, ctn):
    syslog.openlog(module_name)
    syslog.syslog(syslog.LOG_WARNING, ctn)
    syslog.closelog()


def sys_log_err(module_name, ctn):
    syslog.openlog(module_name)
    syslog.syslog(syslog.LOG_ERR, ctn)
    syslog.closelog()


def sys_log_crit(module_name, ctn):
    syslog.openlog(module_name)
    syslog.syslog(syslog.LOG_CRIT, ctn)
    syslog.closelog()

# sysdiag logger class
class SysLogger():
    def __init__(self,
                 module_name,
                 log_file=None,
                 size_bytes=DIAG_LOG_SIZE,
                 backup_count=DIAG_LOG_BACKUP
                 ):
        self.module_name = module_name
        self.logger = None
        self.also_print_console = True
        if log_file:
            log_fname = log_file
        else:
            log_fname = "/".join([DIAG_LOG_DIR, module_name.lower() + ".log"])

        self.logger = logging.getLogger(module_name)
        if self.logger is None:
            err_msg = "Failed to get logger for module %s" % module_name
            sys_log_err(self.module_name, err_msg)
            raise IOError(err_msg)

        self.logger.setLevel(logging.DEBUG)
        
        fmt_str = '%(asctime)s - %(name)s - %(levelname)s: %(message)s'
        formatter = logging.Formatter(fmt_str)
        if not formatter:
            err_msg = "Failed to create formatter for %s" % fmt_str
            sys_log_err(self.module_name, err_msg)
            raise IOError(err_msg)
        
        handler = RotatingFileHandler(log_fname, maxBytes=size_bytes, backupCount=backup_count)
        handler.setFormatter(formatter)
        
        #console = logging.StreamHandler()
        #console.setFormatter(formatter)
    
        self.logger.addHandler(handler)
        #self.logger.addHandler(console)
    
    def get_logger(self):
        return self.logger
    
    def log_dbg(self, ctn, also_print_console=False):
        self.logger.debug(ctn)
        if also_print_console and self.also_print_console:
            print(ctn)


    def log_info(self, ctn, also_print_console=False):
        self.logger.info(ctn)
        if also_print_console and self.also_print_console:
            print(ctn)

    def log_warn(self, ctn, also_print_console=False):
        self.logger.warning(ctn)
        #sys_log_warn(self.module_name, ctn)
        if also_print_console and self.also_print_console:
            print(ctn)

    def log_err(self, ctn, also_print_console=False):
        self.logger.error(ctn)
        #sys_log_err(self.module_name, ctn)
        if also_print_console and self.also_print_console:
            print(ctn)

    def log_crit(self, ctn, also_print_console=False):
        self.logger.critical(ctn)
        #sys_log_crit(self.module_name, ctn)
        if also_print_console and self.also_print_console:
            print(ctn)

    def set_logger_also_print_console(self, also_print_console):
        self.also_print_console = also_print_console

# Initialize logger
def sys_log_init_logger(module_name):
    sys_logger = None

    try:
        sys_logger = SysLogger(module_name=module_name)
    except IOError as e:
        sys_log_err(module_name, "Create logger exception: %s" % str(e))

    return sys_logger
