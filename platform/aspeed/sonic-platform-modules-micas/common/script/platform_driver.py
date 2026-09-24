#!/usr/bin/env python_nos
import os
import time
import click
import glob
import sys
import fnmatch
from platform_config import GLOBALCONFIG, WARM_UPGRADE_STARTED_FLAG, WARM_UPG_FLAG, FW_UPGRADE_STARTED_FLAG, S3IP_DEBUG_FILE_LIST, PLUGINS_DOCKER_STARTED_FLAG, STARTMODULE
from platform_config import get_config_param
from platform_util import AliasedGroup, CONTEXT_SETTINGS, check_value, exec_os_cmd, set_value, PLATFORM_I2C_RETRY_TIME
from platform_util import setup_logger, BSP_COMMON_LOG_DIR
from public.platform_common_config import S3IP_STANDARD_PATTERN, S3IP_LINK_TO_EXTEND, S3IP_LINK_TO_SYS_SWITCH, SET_S3IP_ROOT_MODE, CUSTOM_DEFINED_S3IP_PATTERN, S3IP_PRESENT_DEBUG_LINK_MAP,\
CUSTOM_SENSOR_S3IP_DEBUG_BATH_PATH, CUSTOM_S3IP_PRESENT_DEBUG_BASE_PATH, CUSTOM_SENSOR_S3IP_DEBUG_MAP, CUSTOM_ONE2ONE_S3IP_DEBUG_MAP, S3IP_DEBUG_LINK, S3IP_SENSOR_DEBUG_LINK_MAP
from public.platform_common_config import CUSTOM_DEFINE_EXTEND_S3IP_PATTERN, EXTEND_S3IP_PATH, S3IP_SYSFS_NAME, PLATFORM_INFO_DIR
from public.platform_diff_util import platform_process_other_init, platform_process_other_init_pre
MAC_TEMP_FILE = PLATFORM_INFO_DIR + "highest_mac_temp"

# Constants
LOG_WRITE_SIZE = 1 * 1024 * 1024  # 1 MB
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_driver_debug.log"
logger = setup_logger(LOG_FILE, LOG_WRITE_SIZE)


def log_message(message):
    logger.info(message)

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


def process_init_commands(init):
    if init is not None:
        if isinstance(init, list):
            for init_cmd in init:
                ret, log = set_value(init_cmd)
                if ret is True:
                    log_message("%%PLATFORM_DRIVER: init success, cmd: %s" % init_cmd)
                else:
                    log_message("%%PLATFORM_DRIVER: init failed, cmd: %s, msg: %s" % (init_cmd, str(log)))
                    return
        else:
            log_message("%%PLATFORM_DRIVER: init must be list (type is %s)" % type(init).__name__)
            return


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

    process_init_commands(init)
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


def get_number_from_file(filepath):
    try:
        with open(filepath, "r") as f:
            n = int(f.read().strip())
        return n
    except Exception:
        log_message("%%PLATFORM_DRIVER:Error reading %s" % (filepath))
        return 0


def ensure_parent_dir(filepath):
    dirpath = os.path.dirname(filepath)
    if not os.path.isdir(dirpath):
        os.makedirs(dirpath, mode=0o777)


def safe_symlink(src, dst):
    if os.path.exists(src):
        ensure_parent_dir(dst)
        try:
            if os.path.islink(dst) or os.path.exists(dst):
                os.remove(dst)
            os.symlink(src, dst)
        except Exception as e:
            log_message("%%PLATFORM_DRIVER: Symlinked %s -> %s fail" % (src, dst))
    else:
        log_message("%%PLATFORM_DRIVER: Source %s does not exist, skipping." % (src))


def s3ip_present_debug_link(link_map):
    number_file = f"/sys/{S3IP_SYSFS_NAME}/{link_map[0]}/number"
    count = get_number_from_file(number_file)
    if (link_map[0] != "transceiver"):
        single_dev_name = link_map[0]
    if (link_map[0] == "transceiver"):
        single_dev_name = "eth"

    for i in range(1, count + 1):
        src = f"/sys/{S3IP_SYSFS_NAME}/{link_map[0]}/{single_dev_name}{i}/present"
        dst = CUSTOM_S3IP_PRESENT_DEBUG_BASE_PATH + f"{link_map[1]}/{i}"
        safe_symlink(src, dst)


def s3ip_sensor_debug_links(sensor_type):
    sensor_base = f"/sys/{S3IP_SYSFS_NAME}/{sensor_type}"
    number_path = os.path.join(sensor_base, "number")
    if not os.path.exists(sensor_base) or not os.path.exists(number_path):
        log_message("%%PLATFORM_DRIVER: Skip s3ip_sensor_debug_links, %s folder or number file does not exist." % sensor_type)
        return
    num = get_number_from_file(number_path)

    for i in range(1, num + 1):
        # sensor_type[:-7] = vol or curr 
        sensor_dir = os.path.join(sensor_base, f"{sensor_type[:-7]}{i}")
        # e.g., /sys/s3ip/vol_sensor/vol1 or /sys/s3ip/curr_sensor/curr1
        for src_name, dst_name in CUSTOM_SENSOR_S3IP_DEBUG_MAP:
            src = os.path.join(sensor_dir, src_name)
            dst = f"{CUSTOM_SENSOR_S3IP_DEBUG_BATH_PATH}/{sensor_type[:-7]}{i}/{dst_name}"
            safe_symlink(src, dst)


def s3ip_debug_link():
    if not S3IP_DEBUG_LINK:
        return
    for link_map in S3IP_PRESENT_DEBUG_LINK_MAP:
        s3ip_present_debug_link(link_map)
    for sensor in S3IP_SENSOR_DEBUG_LINK_MAP:
        s3ip_sensor_debug_links(sensor)
    for src, dst in CUSTOM_ONE2ONE_S3IP_DEBUG_MAP:
        safe_symlink(src, dst)

#find s3ip standard node and debug mode
def is_s3ip_standard_debug_node(node_path, pattern_list):
    for p in pattern_list:
        if fnmatch.fnmatch(node_path, p):
            return True
    return False


def get_all_s3ip_nodes(root=f'/sys/{S3IP_SYSFS_NAME}'):
    all_nodes = []
    for dirpath, dirnames, filenames in os.walk(root):
        for f in filenames:
            all_nodes.append(os.path.join(dirpath, f))
    return all_nodes


def add_s3ip_link():
    try:
        if SET_S3IP_ROOT_MODE == True:
            exec_os_cmd(f"sudo chmod 700 /sys/{S3IP_SYSFS_NAME}")
        if S3IP_LINK_TO_SYS_SWITCH == True:
            exec_os_cmd("sudo rm -rf /sys_switch")

            if S3IP_LINK_TO_EXTEND == False :
                safe_symlink(f'/sys/{S3IP_SYSFS_NAME}',  '/sys_switch')
            else :
                all_node_paths = get_all_s3ip_nodes()
                combined_pattern = S3IP_STANDARD_PATTERN + CUSTOM_DEFINED_S3IP_PATTERN
                for node_path in all_node_paths:
                    is_standard_node = is_s3ip_standard_debug_node(node_path, combined_pattern)
                    if is_standard_node:
                        dst_path = node_path.replace(f'/sys/{S3IP_SYSFS_NAME}', '/sys_switch')
                    else:
                        if CUSTOM_DEFINE_EXTEND_S3IP_PATTERN:
                            is_custom_node = is_s3ip_standard_debug_node(node_path, CUSTOM_DEFINE_EXTEND_S3IP_PATTERN)
                            if is_custom_node == False:
                                continue
                        #e.g  '/sys/s3ip/psu/psu1/fan_ratio' => '/sys_switch/extend/psu/psu1/fan_ratio'
                        relative_path = node_path[len(f'/sys/{S3IP_SYSFS_NAME}/'):]
                        dst_path = os.path.join(EXTEND_S3IP_PATH, relative_path)
                    parent_dir = os.path.dirname(dst_path)
                    os.makedirs(parent_dir, mode=0o777, exist_ok=True)
                    safe_symlink(node_path, dst_path)

    except Exception as e:
        log_message("%%PLATFORM_DRIVER: load s3ip link failed, reason: %s" % e)

def remove_s3ip_link():
    if S3IP_LINK_TO_SYS_SWITCH == True:
        exec_os_cmd("sudo rm -rf /sys_switch")


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


def adddriver(name, delay, init=None):
    realname = name.lstrip().split(" ")[0]
    if delay > 0:
        time.sleep(delay)

    ret = checksignaldriver(realname)
    if ret is True:
        log_message("%%PLATFORM_DRIVER: WARN: %s driver already loaded, skip to modprobe" % realname)
        return

    cmd = "modprobe %s" % name
    log_message("%%PLATFORM_DRIVER: adddriver cmd: %s, delay: %s" % (cmd, delay))
    retrytime = PLATFORM_I2C_RETRY_TIME
    for i in range(retrytime):
        process_init_commands(init)
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


def removedriver(name, delay, removeable=1, remove_init=None):
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
    retrytime = PLATFORM_I2C_RETRY_TIME
    for i in range(retrytime):
        process_init_commands(remove_init)
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
        remove_init = None
        removeable = drivers[index].get("removable", 1)
        if isinstance(drivers[index], dict) and "delay" in drivers[index]:
            name = drivers[index].get("name")
            delay = drivers[index]["delay"]
            remove_init = drivers[index].get("remove_init", None)
        else:
            name = drivers[index]
        removedriver(name, delay, removeable, remove_init)


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
        init = None
        if isinstance(driver, dict) and "delay" in driver:
            name = driver.get("name", "")
            delay = driver["delay"]
            init = driver.get("init", None)
        else:
            name = driver
        adddriver(name, delay, init)


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

def drv_otherinit():
    drv_init_para = get_config_param("DRV_INIT_PARAM", [])
    drv_init_cmd  = get_config_param("DRV_INIT_COMMAND", [])
    platform_process_other_init(drv_init_para, drv_init_cmd)

def drv_otherinit_pre():
    drv_init_para_pre = get_config_param("DRV_INIT_PARAM_PRE", [])
    drv_init_cmd_pre  = get_config_param("DRV_INIT_COMMAND_PRE", [])
    platform_process_other_init_pre(drv_init_para_pre, drv_init_cmd_pre)

def unload_driver():
    remove_s3ip_link()
    removeoptoes()
    removedevs()
    removedrivers()


def reload_driver():
    remove_s3ip_link()
    removedevs()
    removedrivers()
    time.sleep(1)
    adddrivers()
    adddevs()


def load_driver():
    drv_otherinit_pre()
    startCommon_operation()
    adddrivers()
    adddevs()
    addoptoes()
    drv_otherinit()
    add_s3ip_link()
    s3ip_debug_link()


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
