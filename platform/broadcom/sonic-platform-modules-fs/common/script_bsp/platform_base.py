#!/usr/bin/env python3
import os
import time
import sys
from platform_util import exec_os_cmd, get_value, set_value
from platform_util import setup_logger, BSP_COMMON_LOG_DIR
from wbutil.baseutil import get_machine_info
from public.platform_common_config import SUB_VERSION_FILE
from public.platform_common_config import BOARD_ID_PATH, PRODUCT_NAME_PATH
from public.platform_common_config import PLATFORM_DEBUG_INIT_PATH, HOST_MACHINE

platform = "NA"
board_id = "NA"

# Constants
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_base_debug.log"
logger = setup_logger(LOG_FILE, LOG_WRITE_SIZE)


def log_message(message):
    logger.info(message)


def platform_debug_init():
    if os.path.isfile(PLATFORM_DEBUG_INIT_PATH):
        os.system(PLATFORM_DEBUG_INIT_PATH)
    return


def get_val_from_env(val):
    cmd = "fw_printenv %s" % val
    status, log = exec_os_cmd(cmd)
    if status != 0:
        msg = "Run %s error, status: %s, log: %s" % (cmd, status, log)
        return False, msg
    val_t = log.split("=")
    if len(val_t) == 2:
        if len(val_t[1].strip()) == 0:
            return False, "%s is empty in env" % val
        return True, val_t[1].strip().replace("-","_").lower()
    msg = "Run %s error, status: %s, log: %s" % (cmd, status, log)
    return False, msg


def get_platform_name_from_env():
    ret, product_name = get_val_from_env("productname")
    if ret is False:
        msg = "get product name error, reason: %s" % product_name
        return False, msg
    return True, product_name


def get_product_board_id_from_env():
    ret, board_id = get_val_from_env("board_id")
    if ret is False:
        msg = "get board id error, reason: %s" % board_id
        return False, msg
    return True, board_id


def get_sonic_platform_info():
    machine_info = get_machine_info()

    if machine_info is not None:
        if 'onie_platform' in machine_info:
            platform = machine_info['onie_platform'].replace("-","_").lower()
            if len(platform) == 0:
                return False, "onie_platform is empty"
            return True, platform

        if 'aboot_platform' in machine_info:
            platform = machine_info['aboot_platform'].replace("-","_").lower()
            if len(platform) == 0:
                return False, "aboot_platform is empty"
            return True, platform
        return False, "onie_platform and aboot_platform not found"
    return False, "machine_info is None"


def get_bmc_platform_info():
    return get_platform_name_from_env()


def get_platform_info():
    if os.path.isfile(HOST_MACHINE):
        # sonic get platform info
        return get_sonic_platform_info()
    else:
        # bmc get platform info
        return get_bmc_platform_info()


def get_sonic_board_id(machine_info=None):
    if machine_info is None:
        machine_info = get_machine_info()

    if machine_info is not None:
        if 'onie_board_id' in machine_info:
            board_id = machine_info['onie_board_id'].replace("-","_").lower()
            if len(board_id) == 0:
                return False, "onie_board_id is empty"
            return True, board_id
        return False, "onie_board_id not found"
    return False, "machine_info is None"


def get_bmc_board_id():
    return get_product_board_id_from_env()


def get_board_id():
    if os.path.isfile(HOST_MACHINE):
        # sonic get board id
        return get_sonic_board_id()
    else:
        # bmc get board id
        return get_bmc_board_id()


def get_product_info():
    global platform
    global board_id

    retry = 10
    for i in range(1, retry + 1):
        status, val_tmp = get_platform_info()
        if status is True:
            platform = val_tmp
            log_message("get_platform_info success, platform: %s, loop: %d" % (platform, i))
            break
        log_message("get_platform_info fail, msg: %s, loop: %d" % (val_tmp, i))
        time.sleep(1)

    for i in range(1, retry + 1):
        status, val_tmp = get_board_id()
        if status is True:
            board_id = val_tmp
            log_message("get_board_id success, board_id: %s, loop: %d" % (board_id, i))
            break
        log_message("get_board_id fail, msg: %s, loop: %d" % (val_tmp, i))
        time.sleep(1)
    return


def generate_platform():
    global platform
    if platform == "NA":
        log_message("get_platform_info fail, not write to %s" % PRODUCT_NAME_PATH)
        return

    out_file_dir = os.path.dirname(PRODUCT_NAME_PATH)
    if len(out_file_dir) != 0:
        cmd = "mkdir -p %s" % out_file_dir
        exec_os_cmd(cmd)
    platform_str = "%s\n" % platform
    with open(PRODUCT_NAME_PATH, "w") as fd:
        fd.write(platform_str)
    exec_os_cmd("sync")
    return


def generate_board_id():
    global board_id
    if board_id == "NA":
        log_message("get_board_id fail, not write to %s" % BOARD_ID_PATH)
        return

    out_file_dir = os.path.dirname(BOARD_ID_PATH)
    if len(out_file_dir) != 0:
        cmd = "mkdir -p %s" % out_file_dir
        exec_os_cmd(cmd)
    board_id_str = "%s\n" % board_id
    with open(BOARD_ID_PATH, "w") as fd:
        fd.write(board_id_str)
    exec_os_cmd("sync")
    return

platform_debug_init()
get_product_info()
generate_platform()
generate_board_id()
platform_productfile = (platform + "_base_config")
platformid_configfile = (platform + "_" + board_id + "_base_config")  # platfrom + board_id
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
            log_message("%%PLATFORM_BASE: run %s success, but driver %s is loaded, retry: %d" % (cmd, realname, i))
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
        cmd = "mkdir -p %s" % out_file_dir
        exec_os_cmd(cmd)
    sub_version_str = "v%02x\n" % value
    with open(SUB_VERSION_FILE, "w") as fd:
        fd.write(sub_version_str)
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

