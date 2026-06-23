#!/usr/bin/env python3
import os
import time
import sys
from platform_util import exec_os_cmd, log_to_file, get_value, set_value, get_sysfs_value
from wbutil.baseutil import get_machine_info, get_platform_info, get_board_id

SUB_VERSION_FILE = "/etc/sonic/.subversion"

# Constants
LOG_DIRECTORY = '/var/log/bsp_tech'
NO_CONFIG_ERR_CODE = 0
READ_FAIL_ERR_CODE = -1
MAX_RETRY = 6

# Ensure log directory exists
os.makedirs(LOG_DIRECTORY, exist_ok=True)
LOG_FILE_PATH = os.path.join(LOG_DIRECTORY, 'platform_base_debug.log')
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB

def log_message(message):
    # Print the message and log it to the log file.
    log_to_file(message, LOG_FILE_PATH, LOG_WRITE_SIZE)


platform = get_platform_info(get_machine_info())
board_id = get_board_id(get_machine_info())
platform_productfile = (platform + "_base_config").replace("-", "_")
platformid_configfile = (platform + "_" + board_id + "_base_config").replace("-", "_")  # platfrom + board_id
configfile_pre = "/usr/local/bin/"
sys.path.append(configfile_pre)

############################################################################################
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
    var = getattr(module_product, name, default)
    return var

DRIVERLISTS = get_var("DRIVERLISTS", [])
DEVICE = get_var("DEVICE", [])
INIT_PARAM = get_var("INIT_PARAM", [])
FINISH_PARAM = get_var("FINISH_PARAM ", [])
SUBVERSION_CONFIG = get_var("SUBVERSION_CONFIG", {})
SECONDARY_SUBVERSION_CONFIG = get_var("SECONDARY_SUBVERSION_CONFIG", {})


def removeDev(bus, loc):
    cmd = "echo 0x%02x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (loc, bus)
    devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
    if os.path.exists(devpath):
        log_message("%%PLATFORM_BASE: removeDev, bus: %s, loc: 0x%02x" % (bus, loc))
        tmp_config = {
            "gettype": "cmd",
            "cmd": cmd
        }
        ret, log = set_value(tmp_config)
        if ret is False:
            log_message("%%PLATFORM_BASE: run %s error, msg: %s" % (cmd, log))
        else:
            log_message("%%PLATFORM_BASE: removeDev, bus: %s, loc: 0x%02x success" % (bus, loc))
    else:
        log_message("%%PLATFORM_BASE: %s not found, don't run cmd: %s" % (devpath, cmd))



def addDev(name, bus, loc):
    pdevpath = "/sys/bus/i2c/devices/i2c-%d/" % (bus)
    for i in range(1, 11):
        if os.path.exists(pdevpath) is True:
            break
        time.sleep(0.1)
        if i % 10 == 0:
            log_message("%%PLATFORM_BASE: %s not found ! i %d " % (pdevpath, i))
            return

    cmd = "echo %s 0x%02x > /sys/bus/i2c/devices/i2c-%d/new_device" % (name, loc, bus)
    devpath = "/sys/bus/i2c/devices/%d-%04x" % (bus, loc)
    if os.path.exists(devpath) is False:
        log_message("%%PLATFORM_BASE: addDev, name: %s, bus: %s, loc: 0x%02x" % (name, bus, loc))
        tmp_config = {
            "gettype": "cmd",
            "cmd": cmd
        }
        ret, log = set_value(tmp_config)
        if ret is False:
            log_message("%%PLATFORM_BASE: run %s error, msg: %s" % (cmd, log))
        else:
            log_message("%%PLATFORM_BASE: addDev, name: %s, bus: %s, loc: 0x%02x success" % (name, bus, loc))
    else:
        log_message("%%PLATFORM_BASE: %s already exist, don't run cmd: %s" % (devpath, cmd))


def removedevs():
    devs = DEVICE
    for index in range(len(devs) - 1, -1, -1):
        removeDev(devs[index]["bus"], devs[index]["loc"])


def adddevs():
    for dev in DEVICE:
        addDev(dev["name"], dev["bus"], dev["loc"])


def checksignaldriver(name):
    driver_path = "/sys/module/%s" % name
    if os.path.exists(driver_path) is True:
        return True
    return False


def adddriver(name, delay):
    realname = name.lstrip().split(" ")[0]
    if delay > 0:
        time.sleep(delay)

    ret = checksignaldriver(realname)
    if ret is True:
        log_message("%%PLATFORM_BASE: WARN: %s driver already loaded, skip to modprobe" % realname)
        return

    cmd = "modprobe %s" % name
    log_message("%%PLATFORM_BASE: adddriver cmd: %s, delay: %s" % (cmd, delay))
    retrytime = 6
    for i in range(retrytime):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            ret = checksignaldriver(realname)
            if ret is True:
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
    ret = checksignaldriver(realname)
    if ret is False:
        log_message("%%PLATFORM_BASE: WARN: %s driver not loaded, skip to rmmod" % realname)
        return

    cmd = "rmmod %s" % realname
    log_message("%%PLATFORM_BASE: removedriver, driver name: %s, delay: %s" % (realname, delay))
    retrytime = 6
    for i in range(retrytime):
        status, log = exec_os_cmd(cmd)
        if status == 0:
            ret = checksignaldriver(realname)
            if ret is False:
                log_message("%%PLATFORM_BASE: remove driver %s success" % realname)
                if delay > 0:
                    time.sleep(delay)
                return
            log_message("%%PLATFORM_BASE: run %s success, but driver %s is load, retry: %d" % (cmd, realname, i))
        else:
            log_message("%%PLATFORM_BASE: run %s error, status: %s, msg: %s, retry: %d" % (cmd, status, log, i))
        time.sleep(0.1)
    log_message("%%PLATFORM_BASE: remove %s driver failed, exit!" % realname)
    sys.exit(1)


def removedrivers():
    drivers = DRIVERLISTS
    for index in range(len(drivers) - 1, -1, -1):
        delay = 0
        name = ""
        removeable = drivers[index].get("removable", 1)
        if isinstance(drivers[index], dict):
            name = drivers[index].get("name")
            delay = drivers[index].get("delay")
        else:
            name = drivers[index]
        removedriver(name, delay, removeable)


def adddrivers():
    for driver in DRIVERLISTS:
        delay = 0
        name = ""
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

def get_main_sub_version():
    if not SUBVERSION_CONFIG:
        log_message("%%PLATFORM_BASE: SUBVERSION_CONFIG is empty, do nothing")
        return NO_CONFIG_ERR_CODE

    val_config = SUBVERSION_CONFIG["get_value"]
    ret, value = get_value(val_config)
    if ret is False:
        log_message("%%PLATFORM_BASE: get value failed, config: %s, log: %s" % (val_config, value))
        return READ_FAIL_ERR_CODE

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

    return "v%02x" % value

def get_secondary_sub_version():
    if not SECONDARY_SUBVERSION_CONFIG:
        log_message("%%PLATFORM_BASE: SECONDARY_SUBVERSION_CONFIG is empty, do nothing")
        return NO_CONFIG_ERR_CODE

    val_config = SECONDARY_SUBVERSION_CONFIG.get("get_value", {}).get("loc")
    val_configs = val_config if isinstance(val_config, list) else [val_config]
    origin_value = ""
    matched_config = None

    for i in range(MAX_RETRY):
        for candidate in val_configs:
            origin_value = get_sysfs_value(candidate)
            if not (origin_value.startswith("ERR") or origin_value == ""):
                matched_config = candidate
                break
        if matched_config is not None:
            break
        log_message("%%PLATFORM_BASE: get value failed, config: %s, log: %s, read count: %s" % (val_configs, origin_value, i))
        time.sleep(0.1)

    if matched_config is None:
        return SECONDARY_SUBVERSION_CONFIG.get("default", READ_FAIL_ERR_CODE)

    log_message("%%PLATFORM_BASE: get value success, config: %s, value: %s" % (matched_config, origin_value))
    decode_config = SECONDARY_SUBVERSION_CONFIG.get("decode_value")
    if decode_config is not None:
        if origin_value in decode_config.keys():
            value = decode_config.get(origin_value)
        elif "default" in decode_config.keys():
            value = decode_config.get("default")
        else:
            value = origin_value
    else:
        value = origin_value
    return value

def generate_sub_version():
    main_sub_version_str = get_main_sub_version()
    if main_sub_version_str == NO_CONFIG_ERR_CODE or main_sub_version_str == READ_FAIL_ERR_CODE:
        return
    
    secondary_sub_version_str = get_secondary_sub_version()
    if secondary_sub_version_str == NO_CONFIG_ERR_CODE or secondary_sub_version_str == READ_FAIL_ERR_CODE:
        sub_version_str = main_sub_version_str
    else:
        sub_version_str = "{}-{}".format(main_sub_version_str, secondary_sub_version_str)

    out_file_dir = os.path.dirname(SUB_VERSION_FILE)
    if len(out_file_dir) != 0:
        cmd = "mkdir -p %s" % out_file_dir
        exec_os_cmd(cmd)
    with open(SUB_VERSION_FILE, "w") as fd:
        fd.write(sub_version_str.lower())
    exec_os_cmd("sync")


def run():
    ret = platform_base_init()
    if ret is False:
        platform_base_finish()
        return
    load_driver()
    generate_sub_version()
    unload_driver()
    platform_base_finish()


if __name__ == '__main__':
    log_message("enter platform base main")
    run()
