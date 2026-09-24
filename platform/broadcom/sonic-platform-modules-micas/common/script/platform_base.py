#!/usr/bin/env python3
import os
import time
import sys
from platform_util import exec_os_cmd, log_to_file, get_value, set_value
from wbutil.baseutil import get_machine_info, get_platform_info, get_board_id

SUB_VERSION_FILE = "/etc/sonic/.subversion"

LOG_DIRECTORY = '/var/log/bsp_tech'
os.makedirs(LOG_DIRECTORY, exist_ok=True)
LOG_FILE_PATH = os.path.join(LOG_DIRECTORY, 'platform_base_debug.log')
LOG_WRITE_SIZE = 1 * 1024 * 1024


def log_message(message):
    log_to_file(message, LOG_FILE_PATH, LOG_WRITE_SIZE)


platform = get_platform_info(get_machine_info())
board_id = get_board_id(get_machine_info())
platform_productfile = (platform + "_base_config").replace("-", "_")
platformid_configfile = (platform + "_" + board_id + "_base_config").replace("-", "_")
configfile_pre = "/usr/local/bin/"
sys.path.append(configfile_pre)

global module_product
if os.path.exists(configfile_pre + platformid_configfile + ".py"):
    module_product = __import__(platformid_configfile, globals(), locals(), [], 0)
elif os.path.exists(configfile_pre + platform_productfile + ".py"):
    module_product = __import__(platform_productfile, globals(), locals(), [], 0)
else:
    log_message("platform base config file not exist, do nothing")
    sys.exit(0)


def get_var(name, default):
    global module_product
    return getattr(module_product, name, default)


DRIVERLISTS = get_var("DRIVERLISTS", [])
DEVICE = get_var("DEVICE", [])
INIT_PARAM = get_var("INIT_PARAM", [])
FINISH_PARAM = get_var("FINISH_PARAM ", [])
SUBVERSION_CONFIG = get_var("SUBVERSION_CONFIG", {})


def removeDev(bus, loc):
    cmd = "echo 0x%02x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (loc, bus)
    devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
    if os.path.exists(devpath):
        log_message("%%PLATFORM_BASE: removeDev, bus: %s, loc: 0x%02x" % (bus, loc))
        ret, log = set_value({"gettype": "cmd", "cmd": cmd})
        if ret is False:
            log_message("%%PLATFORM_BASE: run %s error, msg: %s" % (cmd, log))
        else:
            log_message("%%PLATFORM_BASE: removeDev, bus: %s, loc: 0x%02x success" % (bus, loc))
    else:
        log_message("%%PLATFORM_BASE: %s not found, don't run cmd: %s" % (devpath, cmd))


def addDev(name, bus, loc):
    pdevpath = "/sys/bus/i2c/devices/i2c-%d/" % bus
    for i in range(1, 11):
        if os.path.exists(pdevpath):
            break
        time.sleep(0.1)
        if i % 10 == 0:
            log_message("%%PLATFORM_BASE: %s not found ! i %d " % (pdevpath, i))
            return

    cmd = "echo %s 0x%02x > /sys/bus/i2c/devices/i2c-%d/new_device" % (name, loc, bus)
    devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
    if not os.path.exists(devpath):
        log_message("%%PLATFORM_BASE: addDev, name: %s, bus: %s, loc: 0x%02x" % (name, bus, loc))
        ret, log = set_value({"gettype": "cmd", "cmd": cmd})
        if ret is False:
            log_message("%%PLATFORM_BASE: run %s error, msg: %s" % (cmd, log))
        else:
            log_message("%%PLATFORM_BASE: addDev, name: %s, bus: %s, loc: 0x%02x success" % (name, bus, loc))
    else:
        log_message("%%PLATFORM_BASE: %s already exist, don't run cmd: %s" % (devpath, cmd))


def removedevs():
    for index in range(len(DEVICE) - 1, -1, -1):
        removeDev(DEVICE[index]["bus"], DEVICE[index]["loc"])


def adddevs():
    for dev in DEVICE:
        addDev(dev["name"], dev["bus"], dev["loc"])


def checksignaldriver(name):
    return os.path.exists("/sys/module/%s" % name)


def adddriver(name, delay):
    realname = name.lstrip().split(" ")[0]
    if delay > 0:
        time.sleep(delay)
    if checksignaldriver(realname):
        log_message("%%PLATFORM_BASE: WARN: %s driver already loaded, skip to modprobe" % realname)
        return
    cmd = "modprobe %s" % name
    log_message("%%PLATFORM_BASE: adddriver cmd: %s, delay: %s" % (cmd, delay))
    for i in range(6):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            if checksignaldriver(realname):
                log_message("%%PLATFORM_BASE: add driver %s success" % realname)
                return
            log_message("%%PLATFORM_BASE: run %s success, but driver %s not load, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_BASE: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_BASE: load %s driver failed, exit!" % realname)
    sys.exit(1)


def removedriver(name, delay, removeable=1):
    realname = name.lstrip().split(" ")[0]
    if not removeable:
        log_message("%%PLATFORM_BASE: driver name: %s not removeable" % realname)
        return
    if not checksignaldriver(realname):
        log_message("%%PLATFORM_BASE: WARN: %s driver not loaded, skip to rmmod" % realname)
        return

    cmd = "rmmod %s" % realname
    log_message("%%PLATFORM_BASE: removedriver, driver name: %s, delay: %s" % (realname, delay))
    for i in range(6):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            if not checksignaldriver(realname):
                log_message("%%PLATFORM_BASE: remove driver %s success" % realname)
                if delay > 0:
                    time.sleep(delay)
                return
            log_message("%%PLATFORM_BASE: run %s success, but driver %s is loaded, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_BASE: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_BASE: remove %s driver failed, exit!" % realname)
    sys.exit(1)


def removedrivers():
    for index in range(len(DRIVERLISTS) - 1, -1, -1):
        delay = 0
        removeable = DRIVERLISTS[index].get("removable", 1)
        if isinstance(DRIVERLISTS[index], dict):
            name = DRIVERLISTS[index].get("name")
            delay = DRIVERLISTS[index].get("delay")
        else:
            name = DRIVERLISTS[index]
        removedriver(name, delay, removeable)


def adddrivers():
    for driver in DRIVERLISTS:
        delay = 0
        if isinstance(driver, dict):
            name = driver.get("name")
            delay = driver.get("delay", 0)
        else:
            name = driver
        adddriver(name, delay)


def platform_base_init():
    for item in INIT_PARAM:
        status, log = set_value(item)
        if status is False:
            log_message("%%PLATFORM_BASE: init set value failed, config: %s, log: %s" % (item, log))
            return False
        log_message("%%PLATFORM_BASE: init set value success, config: %s" % item)
    return True


def platform_base_finish():
    for item in FINISH_PARAM:
        status, log = set_value(item)
        if status is False:
            log_message("%%PLATFORM_BASE: finish set value failed, config: %s, log: %s" % (item, log))
        else:
            log_message("%%PLATFORM_BASE: finish set value success, config: %s" % item)


def unload_driver():
    removedevs()
    removedrivers()


def load_driver():
    adddrivers()
    adddevs()


def generate_sub_version():
    if not SUBVERSION_CONFIG:
        log_message("%%PLATFORM_BASE: SUBVERSION_CONFIG is empty, do nothing")
        return

    val_config = SUBVERSION_CONFIG["get_value"]
    ret, value = get_value(val_config)
    if ret is False:
        log_message("%%PLATFORM_BASE: get value failed, config: %s, log: %s" % (val_config, value))
        return

    log_message("%%PLATFORM_BASE: get value success, value: 0x%02x" % value)

    val_mask = val_config.get("mask")
    if val_mask is not None:
        origin_value = value
        value = origin_value & val_mask
        log_message("%%PLATFORM_BASE: origin value: 0x%02x, mask: 0x%02x, mask_value: 0x%02x" %
                    (origin_value, val_mask, value))

    decode_config = SUBVERSION_CONFIG.get("decode_value")
    if decode_config is not None:
        origin_value = value
        value = decode_config.get(origin_value, origin_value)
        log_message("%%PLATFORM_BASE: origin_value: 0x%02x, decode value: 0x%02x" % (origin_value, value))

    out_file_dir = os.path.dirname(SUB_VERSION_FILE)
    if len(out_file_dir) != 0:
        exec_os_cmd("mkdir -p %s" % out_file_dir)
    with open(SUB_VERSION_FILE, "w") as fd:
        fd.write("v%02x\n" % value)
    exec_os_cmd("sync")


def run():
    ret = platform_base_init()
    if ret is False:
        platform_base_finish()
        return
    load_driver()
    generate_sub_version()
    unload_driver()


if __name__ == '__main__':
    run()
