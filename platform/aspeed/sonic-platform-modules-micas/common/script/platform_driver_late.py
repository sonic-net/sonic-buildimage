#!/usr/bin/env python_nos
import os
import time
import click
import copy
from platform_config import get_config_param
from platform_util import check_value, exec_os_cmd, set_value
from platform_util import setup_logger, BSP_COMMON_LOG_DIR
from public.platform_common_config import COMMON_DRIVER_CHECK_CONFIGS
CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}


# Constants
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_driver_late.log"
logger = setup_logger(LOG_FILE, LOG_WRITE_SIZE)

def log_message(message):
    logger.info(message)

class AliasedGroup(click.Group):
    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv
        matches = [x for x in self.list_commands(ctx)
                   if x.startswith(cmd_name)]
        if not matches:
            return None
        if len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))
        return None


def checksignaldriver(name):
    driver_path = "/sys/module/%s" % name.replace('-', '_')
    if os.path.exists(driver_path) is True:
        return True
    return False


def adddriver(name, delay):
    realname = name.lstrip().split(" ")[0]
    if delay > 0:
        time.sleep(delay)

    ret = checksignaldriver(realname)
    if ret is True:
        log_message("%%PLATFORM_DRIVER_LATE: WARN: %s driver already loaded, skip to modprobe" % realname)
        return

    cmd = "modprobe %s" % name
    log_message("%%PLATFORM_DRIVER_LATE: adddriver cmd: %s, delay: %s" % (cmd, delay))
    retrytime = 6
    for i in range(retrytime):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            ret = checksignaldriver(realname)
            if ret is True:
                log_message("%%PLATFORM_DRIVER_LATE: add driver %s success" % realname)
                return
            log_message("%%PLATFORM_DRIVER_LATE: run %s success, but driver %s not load, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_DRIVER_LATE: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_DRIVER_LATE: load %s driver failed, exit!" % realname)
    sys.exit(1)


def removedriver(name, delay, removeable=1):
    realname = name.lstrip().split(" ")[0]
    if not removeable:
        log_message("%%PLATFORM_DRIVER_LATE: driver name: %s not removeable" % realname)
        return

    ret = checksignaldriver(realname)
    if ret is False:
        log_message("%%PLATFORM_DRIVER_LATE: WARN: %s driver not loaded, skip to rmmod" % realname)
        return

    cmd = "rmmod %s" % realname
    log_message("%%PLATFORM_DRIVER_LATE: removedriver, driver name: %s, delay: %s" % (realname, delay))
    retrytime = 6
    for i in range(retrytime):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            ret = checksignaldriver(realname)
            if ret is False:
                log_message("%%PLATFORM_DRIVER_LATE: remove driver %s success" % realname)
                if delay > 0:
                    time.sleep(delay)
                return
            log_message("%%PLATFORM_DRIVER_LATE: run %s success, but driver %s is loaded, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_DRIVER_LATE: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_DRIVER_LATE: remove %s driver failed, exit!" % realname)
    sys.exit(1)


def add_driver_cycle_check():
    drvs_common_copied = copy.deepcopy(COMMON_DRIVER_CHECK_CONFIGS)
    drvs_check_copied  = copy.deepcopy(get_config_param("DRIVERLISTS_CHECK", []))
    all_drvs = drvs_common_copied + drvs_check_copied

    while all_drvs:
        driver = all_drvs[0]
        name = driver.get("name", "")
        pre_check_conf = driver.get("pre_check", None)
        if pre_check_conf is not None:
            status, msg = check_value(pre_check_conf)
            if status is False:
                time.sleep(1)
                log_message("%%PLATFORM_DRIVER_LATE: %s cycle check_value failed log is %s " % (name, msg))
                all_drvs.append(all_drvs.pop(0))
                continue

        delay = driver.get("delay", 0)
        adddriver(name, delay)
        all_drvs.pop(0)


def remove_driver_cycle_check():
    drvs_common_copied = copy.deepcopy(COMMON_DRIVER_CHECK_CONFIGS)
    drvs_check_copied  = copy.deepcopy(get_config_param("DRIVERLISTS_CHECK", []))
    drivers = drvs_common_copied + drvs_check_copied

    for index in range(len(drivers) - 1, -1, -1):
        delay = 0
        name = ""
        removeable = drivers[index].get("removable", 1)
        if isinstance(drivers[index], dict) and "delay" in drivers[index]:
            name = drivers[index].get("name")
            delay = drivers[index]["delay"]
        else:
            name = drivers[index]
        removedriver(name, delay, removeable)


def unload_driver():
    remove_driver_cycle_check()


def reload_driver():
    remove_driver_cycle_check()
    time.sleep(1)
    add_driver_cycle_check()


def load_driver():
    add_driver_cycle_check()


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''


@main.command()
def start():
    load_driver()


@main.command()
def stop():
    '''stop drivers device '''
    unload_driver()


@main.command()
def restart():
    '''restart drivers and device'''
    unload_driver()
    load_driver()


if __name__ == '__main__':
    main()
