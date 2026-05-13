#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import subprocess
import time
import click
from platform_config import GLOBALCONFIG, WARM_UPGRADE_STARTED_FLAG, WARM_UPG_FLAG, FW_UPGRADE_STARTED_FLAG
from platform_util import exec_os_cmd, set_value, setup_logger, BSP_COMMON_LOG_DIR


CONTEXT_SETTINGS = {"help_option_names": ['-h', '--help']}


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

def log_os_system(cmd):
    status, output = subprocess.getstatusoutput(cmd)
    if status:
        print(output)
    return status, output

LOG_WRITE_SIZE = 1 * 1024 * 1024
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_driver_debug.log"
logger = setup_logger(LOG_FILE, LOG_WRITE_SIZE)


def log_message(message):
    logger.info(message)


def platform_process_file_check():
    # WARM_UPGRADE_STARTED_FLAG is used as warm_upgrade.py process start flag
    if os.path.exists(WARM_UPGRADE_STARTED_FLAG):
        os.remove(WARM_UPGRADE_STARTED_FLAG)

    # WARM_UPG_FLAG is used as port related service judgment flag
    if os.path.exists(WARM_UPG_FLAG):
        os.remove(WARM_UPG_FLAG)

    # FW_UPGRADE_STARTED_FLAG is used as upgrade.py process start flag
    if os.path.exists(FW_UPGRADE_STARTED_FLAG):
        os.remove(FW_UPGRADE_STARTED_FLAG)


def startCommon_operation():
    platform_process_file_check()

def removeDev(bus, loc):
    cmd = "echo 0x%02x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (loc, bus)
    devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
    if os.path.exists(devpath):
        log_message("%%PLATFORM_DRIVER: removeDev, bus: %s, loc: 0x%02x" % (bus, loc))
        ret, log = set_value({"gettype": "cmd", "cmd": cmd})
        if ret is False:
            log_message("%%PLATFORM_DRIVER: run %s error, msg: %s" % (cmd, log))
        else:
            log_message("%%PLATFORM_DRIVER: removeDev, bus: %s, loc: 0x%02x success" % (bus, loc))
    else:
        log_message("%%PLATFORM_DRIVER: %s not found, don't run cmd: %s" % (devpath, cmd))


def addDev(name, bus, loc):
    if name == "lm75":
        time.sleep(0.1)
    pdevpath = "/sys/bus/i2c/devices/i2c-%d/" % (bus)
    for i in range(1, 11):
        if os.path.exists(pdevpath) is True:
            break
        time.sleep(0.1)
        if i % 10 == 0:
            log_message("%%PLATFORM_DRIVER: %s not found ! i %d " % (pdevpath, i))
            return

    cmd = "echo %s 0x%02x > /sys/bus/i2c/devices/i2c-%d/new_device" % (name, loc, bus)
    devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
    if os.path.exists(devpath) is False:
        log_message("%%PLATFORM_DRIVER: addDev, name: %s, bus: %s, loc: 0x%02x" % (name, bus, loc))
        ret, log = set_value({"gettype": "cmd", "cmd": cmd})
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
        addDev(dev["name"], dev["bus"], dev["loc"])


def checksignaldriver(name):
    return os.path.exists("/sys/module/%s" % name)


def adddriver(name, delay):
    realname = name.lstrip().split(" ")[0]
    if delay > 0:
        time.sleep(delay)
    if checksignaldriver(realname):
        log_message("%%PLATFORM_DRIVER: WARN: %s driver already loaded, skip to modprobe" % realname)
        return
    cmd = "modprobe %s" % name
    log_message("%%PLATFORM_DRIVER: adddriver cmd: %s, delay: %s" % (cmd, delay))
    for i in range(6):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            if checksignaldriver(realname):
                log_message("%%PLATFORM_DRIVER: add driver %s success" % realname)
                return
            log_message("%%PLATFORM_DRIVER: run %s success, but driver %s not load, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_DRIVER: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_DRIVER: load %s driver failed, exit!" % realname)
    raise SystemExit(1)


def removedriver(name, delay, removeable=1):
    realname = name.lstrip().split(" ")[0]
    if not removeable:
        log_message("%%PLATFORM_DRIVER: driver name: %s not removeable" % realname)
        return
    if not checksignaldriver(realname):
        log_message("%%PLATFORM_DRIVER: WARN: %s driver not loaded, skip to rmmod" % realname)
        return
    cmd = "rmmod %s" % realname
    log_message("%%PLATFORM_DRIVER: removedriver, driver name: %s, delay: %s" % (realname, delay))
    for i in range(6):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            if not checksignaldriver(realname):
                log_message("%%PLATFORM_DRIVER: remove driver %s success" % realname)
                if delay > 0:
                    time.sleep(delay)
                return
            log_message("%%PLATFORM_DRIVER: run %s success, but driver %s is loaded, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_DRIVER: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_DRIVER: remove %s driver failed, exit!" % realname)
    raise SystemExit(1)


def removedrivers():
    if GLOBALCONFIG is None:
        log_message("%%WB_PLATFORM_DRIVER-INIT: load global config failed.")
        return
    drivers = GLOBALCONFIG.get("DRIVERLISTS", None)
    if drivers is None:
        log_message("%%WB_PLATFORM_DRIVER-INIT: load driver list failed.")
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
        log_message("%%WB_PLATFORM_DRIVER-INIT: load global config failed.")
        return
    drivers = GLOBALCONFIG.get("DRIVERLISTS", None)
    if drivers is None:
        log_message("%%WB_PLATFORM_DRIVER-INIT: load driver list failed.")
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
        log_message("%%WB_PLATFORM_DRIVER-INIT: load global config failed.")
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


def i2c_check(bus, retrytime=6):
    try:
        i2cpath = "/sys/bus/i2c/devices/" + bus
        while retrytime and not os.path.exists(i2cpath):
            click.echo("%%WB_PLATFORM_DRIVER-HA: i2c bus abnormal, last bus %s is not exist." % i2cpath)
            reload_driver()
            retrytime -= 1
            time.sleep(1)
    except Exception as e:
        click.echo("%%WB_PLATFORM_DRIVER-HA: %s" % str(e))


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
