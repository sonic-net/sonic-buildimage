#!/usr/bin/python
# -*- coding: UTF-8 -*-
import os
import syslog
import logging
import subprocess
import shlex

SYSLOG_IDENTIFIER = "LOGUTIL"

# ========================== Syslog wrappers ==========================

def log_info(msg,log_type=SYSLOG_IDENTIFIER, also_print_to_console=False):
    syslog.openlog(log_type)
    syslog.syslog(syslog.LOG_INFO, msg)
    syslog.closelog()

    if also_print_to_console:
        click.echo(msg)


def log_debug(msg, log_type=SYSLOG_IDENTIFIER, also_print_to_console=False):
    try:
        syslog.openlog(log_type)
        syslog.syslog(syslog.LOG_DEBUG, msg)
        syslog.closelog()

        if also_print_to_console:
            click.echo(msg)
    except Exception as e:
        print(str(e))


def log_warning(msg, log_type=SYSLOG_IDENTIFIER, also_print_to_console=False):
    syslog.openlog(log_type)
    syslog.syslog(syslog.LOG_WARNING, msg)
    syslog.closelog()

    if also_print_to_console:
        click.echo(msg)

def log_error(msg, log_type=SYSLOG_IDENTIFIER, also_print_to_console=False):
    syslog.openlog(log_type)
    syslog.syslog(syslog.LOG_ERR, msg)
    syslog.closelog()

    if also_print_to_console:
        click.echo(msg)

restful_logger_path = "/var/log/bmc_restful.log"
def bmc_restful_logger(message):
    if not os.path.isfile(restful_logger_path):
        cmd = "sudo touch %s && sudo chmod +x %s" % (
            restful_logger_path, restful_logger_path)
        subprocess.Popen(
            shlex.split(cmd), shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    logging.basicConfig(filename=restful_logger_path,
                        filemode='a',
                        format='%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s',
                        datefmt='%H:%M:%S',
                        level=logging.INFO)

    logging.info(message)
