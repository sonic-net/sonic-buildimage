#!/usr/bin/env python3
import os
import time
import click
import glob
import sys
from platform_config import GLOBALCONFIG, WARM_UPGRADE_STARTED_FLAG, WARM_UPG_FLAG, FW_UPGRADE_STARTED_FLAG, S3IP_DEBUG_FILE_LIST, PLUGINS_DOCKER_STARTED_FLAG
from platform_util import check_value, exec_os_cmd, set_value
from platform_util import setup_logger, BSP_COMMON_LOG_DIR
CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}
MAC_TEMP_FILE = "/etc/sonic/highest_mac_temp"

# Constants
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_driver_debug.log"
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


fixed_files = [
    WARM_UPGRADE_STARTED_FLAG,
    WARM_UPG_FLAG,
    FW_UPGRADE_STARTED_FLAG,
    MAC_TEMP_FILE,
    PLUGINS_DOCKER_STARTED_FLAG,
]

def platform_process_file_check():
    all_file_list = [
        *fixed_files,
        *S3IP_DEBUG_FILE_LIST
    ]

    for file_list in all_file_list:
        for file_path in glob.glob(file_list):
            if os.path.exists(file_path):
                try:
                    os.remove(file_path)
                except OSError as e:
                    print("Error deleting file: %s" % str(e))


def startCommon_operation():
    platform_process_file_check()


def removeDev(bus, loc):
    cmd = "echo 0x%02x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (loc, bus)
    devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
    if os.path.exists(devpath):
        log_message("%%PLATFORM_DRIVER: removeDev, bus: %s, loc: 0x%02x" % (bus, loc))
        tmp_config = {
            "gettype": "cmd",
            "cmd": cmd
        }
        ret, log = set_value(tmp_config)
        if ret is False:
            log_message("%%PLATFORM_DRIVER: run %s error, msg: %s" % (cmd, log))
        else:
            log_message("%%PLATFORM_DRIVER: removeDev, bus: %s, loc: 0x%02x success" % (bus, loc))
    else:
        log_message("%%PLATFORM_DRIVER: %s not found, don't run cmd: %s" % (devpath, cmd))


def addDev(name, bus, loc, init=None):
    pdevpath = "/sys/bus/i2c/devices/i2c-%d/" % (bus)
    for i in range(1, 11):
        if os.path.exists(pdevpath) is True:
            break
        time.sleep(0.1)
        if i % 10 == 0:
            log_message("%%PLATFORM_DRIVER: %s not found ! i %d " % (pdevpath, i))
            return

    if init is not None:
        if isinstance(init, list):
            for init_cmd in init:
                ret, log = set_value(init_cmd)
                if ret is True:
                    log_message("%%PLATFORM_DRIVER: init for dev %s success, cmd: %s" % (name, init_cmd))
                else:
                    log_message("%%PLATFORM_DRIVER: init for dev %s failed, cmd: %s, msg: %s" % (name, init_cmd, str(log)))
        else:
            log_message("%%PLATFORM_DRIVER: init must be list (type is %s)" % type(init).__name__)

    cmd = "echo %s 0x%02x > /sys/bus/i2c/devices/i2c-%d/new_device" % (name, loc, bus)
    devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
    if os.path.exists(devpath) is False:
        log_message("%%PLATFORM_DRIVER: addDev, name: %s, bus: %s, loc: 0x%02x" % (name, bus, loc))
        tmp_config = {
            "gettype": "cmd",
            "cmd": cmd
        }
        ret, log = set_value(tmp_config)
        if ret is False:
            log_message("%%PLATFORM_DRIVER: run %s error, msg: %s" % (cmd, log))
        else:
            log_message("%%PLATFORM_DRIVER: addDev, name: %s, bus: %s, loc: 0x%02x success" % (name, bus, loc))
    else:
        log_message("%%PLATFORM_DRIVER: %s already exist, don't run cmd: %s" % (devpath, cmd))



def removeOPTOE(startbus, endbus):
    for bus in range(endbus, startbus - 1, -1):
        removeDev(bus, 0x50)


def addOPTOE(name, startbus, endbus):
    for bus in range(startbus, endbus + 1):
        addDev(name, bus, 0x50)


def removeoptoes():
    optoes = GLOBALCONFIG["OPTOE"]
    for index in range(len(optoes) - 1, -1, -1):
        removeOPTOE(optoes[index]["startbus"], optoes[index]["endbus"])


def addoptoes():
    optoes = GLOBALCONFIG["OPTOE"]
    for optoe in optoes:
        addOPTOE(optoe["name"], optoe["startbus"], optoe["endbus"])


def removedevs():
    devs = GLOBALCONFIG["DEVS"]
    for index in range(len(devs) - 1, -1, -1):
        removeDev(devs[index]["bus"], devs[index]["loc"])


def adddevs():
    devs = GLOBALCONFIG["DEVS"]
    for dev in devs:
        addDev(dev["name"], dev["bus"], dev["loc"], dev.get("init", None))


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
        log_message("%%PLATFORM_DRIVER: WARN: %s driver already loaded, skip to modprobe" % realname)
        return

    cmd = "modprobe %s" % name
    log_message("%%PLATFORM_DRIVER: adddriver cmd: %s, delay: %s" % (cmd, delay))
    retrytime = 6
    for i in range(retrytime):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            ret = checksignaldriver(realname)
            if ret is True:
                log_message("%%PLATFORM_DRIVER: add driver %s success" % realname)
                return
            log_message("%%PLATFORM_DRIVER: run %s success, but driver %s not load, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_DRIVER: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_DRIVER: load %s driver failed, exit!" % realname)
    sys.exit(1)


def removedriver(name, delay, removeable=1):
    realname = name.lstrip().split(" ")[0]
    if not removeable:
        log_message("%%PLATFORM_DRIVER: driver name: %s not removeable" % realname)
        return

    ret = checksignaldriver(realname)
    if ret is False:
        log_message("%%PLATFORM_DRIVER: WARN: %s driver not loaded, skip to rmmod" % realname)
        return

    cmd = "rmmod %s" % realname
    log_message("%%PLATFORM_DRIVER: removedriver, driver name: %s, delay: %s" % (realname, delay))
    retrytime = 6
    for i in range(retrytime):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            ret = checksignaldriver(realname)
            if ret is False:
                log_message("%%PLATFORM_DRIVER: remove driver %s success" % realname)
                if delay > 0:
                    time.sleep(delay)
                return
            log_message("%%PLATFORM_DRIVER: run %s success, but driver %s is loaded, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_DRIVER: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_DRIVER: remove %s driver failed, exit!" % realname)
    sys.exit(1)


def removedrivers():
    if GLOBALCONFIG is None:
        log_message("%%PLATFORM_DRIVER-INIT: load global config failed.")
        return
    drivers = GLOBALCONFIG.get("DRIVERLISTS", None)
    if drivers is None:
        log_message("%%PLATFORM_DRIVER-INIT: load driver list failed.")
        return
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


def adddrivers():
    if GLOBALCONFIG is None:
        log_message("%%PLATFORM_DRIVER-INIT: load global config failed.")
        return
    drivers = GLOBALCONFIG.get("DRIVERLISTS", None)
    if drivers is None:
        log_message("%%PLATFORM_DRIVER-INIT: load driver list failed.")
        return
    for driver in drivers:
        delay = 0
        name = ""
        if isinstance(driver, dict) and "delay" in driver:
            name = driver.get("name")
            delay = driver["delay"]
        else:
            name = driver
        adddriver(name, delay)


def blacklist_driver_remove():
    if GLOBALCONFIG is None:
        log_message("%%PLATFORM_DRIVER-INIT: load global config failed.")
        return
    blacklist_drivers = GLOBALCONFIG.get("BLACKLIST_DRIVERS", [])
    for driver in blacklist_drivers:
        delay = 0
        name = ""
        if isinstance(driver, dict) and "delay" in driver:
            name = driver.get("name")
            delay = driver["delay"]
        else:
            name = driver
        removedriver(name, delay)


def unload_driver():
    removeoptoes()
    removedevs()
    removedrivers()


def reload_driver():
    removedevs()
    removedrivers()
    time.sleep(1)
    adddrivers()
    adddevs()


def load_driver():
    startCommon_operation()
    adddrivers()
    adddevs()
    addoptoes()


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''


@main.command()
def start():
    '''load drivers and device '''
    blacklist_driver_remove()
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
