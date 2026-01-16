#!/usr/bin/python


import syslog
import traceback
import logging.handlers


DEF_LOG_DIR = "/var/log"
DEF_LOG_BYTES = 20 * 1024 * 1024
DEF_LOG_BACKUP = 5


# RAW syslog facilities
def sys_log_info(module_name, ctn):
    syslog.openlog(module_name)
    syslog.syslog(syslog.LOG_WARNING, ctn)
    print(ctn)
    syslog.closelog()

def sys_log_warn(module_name, ctn):
    syslog.openlog(module_name)
    syslog.syslog(syslog.LOG_WARNING, ctn)
    print(ctn)
    syslog.closelog()


def sys_log_err(module_name, ctn):
    syslog.openlog(module_name)
    syslog.syslog(syslog.LOG_ERR, ctn)
    print(ctn)
    syslog.closelog()


def sys_log_crit(module_name, ctn):
    syslog.openlog(module_name)
    syslog.syslog(syslog.LOG_CRIT, ctn)
    print(ctn)
    syslog.closelog()


# SysLogger
class SysLogger(object):
    def __init__(self, module_name, size_bytes=DEF_LOG_BYTES,
                 backup_count=DEF_LOG_BACKUP, log_file=None):
        self.module_name = module_name
        self.logger = None
        self.also_print_console = True
        if log_file:
            log_fname = log_file
        else:
            log_fname = "/".join([DEF_LOG_DIR, module_name.lower() + ".log"])

        self.logger = logging.getLogger(module_name)
        if self.logger is None:
            err_msg = "Failed to get logger for module %s" % module_name
            sys_log_err(self.module_name, err_msg)
            raise IOError(err_msg)

        self.logger.setLevel(logging.INFO)
        fh = logging.handlers.RotatingFileHandler(log_fname,
                                                  maxBytes=size_bytes,
                                                  backupCount=backup_count)
        if not fh:
            err_msg = "Failed to create RotatingFileHandler for log file"
            sys_log_err(self.module_name, err_msg)
            raise IOError(err_msg)

        fmt_str = '%(asctime)s - %(name)s - %(levelname)s: %(message)s'
        formatter = logging.Formatter(fmt_str)
        if not formatter:
            err_msg = "Failed to create formatter for %s" % fmt_str
            sys_log_err(self.module_name, err_msg)
            raise IOError(err_msg)

        fh.setFormatter(formatter)
        self.logger.addHandler(fh)

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
        # sys_log_warn(self.module_name, ctn)
        if also_print_console and self.also_print_console:
            print(ctn)

    def log_err(self, ctn, also_print_console=False):
        self.logger.error(ctn)
        # sys_log_err(self.module_name, ctn)
        if also_print_console and self.also_print_console:
            print(ctn)

    def log_crit(self, ctn, also_print_console=False):
        self.logger.critical(ctn)
        #sys_log_crit(self.module_name, ctn)
        if also_print_console and self.also_print_console:
            print(ctn)

    def set_logger(self, also_print_console):
        self.also_print_console = also_print_console

# Initialize logger
def sys_log_init_logger(module_name):
    sys_logger = None

    try:
        sys_logger = SysLogger(module_name)
    except IOError as e:
        sys_log_err(module_name, "{}".format(traceback.format_exc()))
        sys_log_err(module_name, "Create logger exception: %s" % str(e))

    return sys_logger
