#!/usr/bin/env python_nos
import os
import time
import logging
import errno
from platform_util import *
from platform_config import get_config_param


LOGIC_MON_CTRL_FILE = "/tmp/.logic_monitor_factest_mode_en"
DEBUG_FILE = "/etc/.platform_logic_monitor_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_logic_monitor_debug.log"
logger = setup_logger(LOG_FILE)
CHECK_OK = "NORMAL"
CHECK_NOT_OK = "ABNORMAL"
LOGIC_DEV_FILE = "/sys/logic_dev"
LOG_LAST_TIME = {}
LOGITC_STATUS_CHECK_VALUE_OK = 0
'''
Example
PLATFORM_LOGIC_MONITOR_CONFIG = {
    "logic_dev_sysfs_file": "xxxxx",
    "polling_time": 60,
    "dev_config": [
        {"name": "xxx", "gettype": "sysfs", "loc": "/sys/xxxx", "okval": 0x1, "mask": 0x01},
        {"name": "xxx", "gettype": "sysfs", "loc": "/sys/xxxx", "okval": [0x1, 0x2], "mask": 0x03},
        {"name": "xxx", "gettype": "devfile", "path": "/dev/cpld0", "offset": 0x1, "okval": 0x1, "mask": 0x01},
        {"name": "xxx", "gettype": "sysfs", "loc": "/sys/xxxx", "okval": [0x1, 0x2], "mask": 0x03},
    ],
}
'''
# Log initialization and interface
def platform_logic_monitor_debug(s):
    logger.debug(s)


def platform_logic_monitor_error(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, s)
    if print_flag:
        logger.error(s)


def platform_logic_monitor_info(s, additional_key = ""):
    global LOG_LAST_TIME
    print_flag, LOG_LAST_TIME = allow_syslog(LOG_LAST_TIME, s)
    if print_flag:
        logger.info(s)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


class DeviceMonitor:
    def __init__(self, name, config):
        self.name = name
        self.config = config
        self.skip_forever = False
        self.first_run = True
        self.last_status = None
        self.last_raw_val = None

    def direct_sysfs_status(self):
        path = self.config.get("loc")
        try:
            with open(path, "r") as f:
                val_str = f.read().strip()
                platform_logic_monitor_debug(f"[{self.name}] sysfs direct value: {val_str}")
                return True, val_str
        except OSError as e:
            # check EOPNOTSUPP(operation not supported)
            if hasattr(e, 'errno') and e.errno == errno.EOPNOTSUPP:
                platform_logic_monitor_info(f"[{self.name}] sysfs status not supported (EOPNOTSUPP)")
                return False, errno.EOPNOTSUPP
            else:
                platform_logic_monitor_error(f"[{self.name}] sysfs read error: {e}")
                return False, None
        except Exception as e:
            platform_logic_monitor_error(f"[{self.name}] sysfs read unknown error: {e}")
            return False, None

    def clear_status(self, status_value):
        command = "echo %d > %s" % (status_value, self.config.get("loc"))
        platform_logic_monitor_debug(f"[{self.name}] clear status")
        status, output = exec_os_cmd(command)
        if status == 0:
            return True, output
        else:
            platform_logic_monitor_error(f"[{self.name}] clear status fail: {output}")
            return False, output

    def monitor(self):
        if self.skip_forever:
            platform_logic_monitor_debug(f"[{self.name}] Skipped permanently due to EOPNOTSUPP")
            return

        #check support
        if self.first_run and (self.config.get("gettype") == "sysfs") and self.config.get("loc"):
            ret, val = self.direct_sysfs_status()
            if ret is False:
                if val == errno.EOPNOTSUPP:    #-EOPNOTSUPP
                    platform_logic_monitor_info(f"[{self.name}] status == -EOPNOTSUPP (-95). Not supported, skip future.")
                    self.skip_forever = True
                    return
                platform_logic_monitor_error(f"[{self.name}] Failed to read sysfs status.")
                return

            platform_logic_monitor_info(f"[{self.name}] Initial sysfs value: {val}")
            self.first_run = False

        ret, status, raw_val = check_value_and_get_value(self.config)
        platform_logic_monitor_debug(f"[{self.name}] check_value_and_get_value: ret={ret}, status={status}, raw_val={raw_val:#0x}")
        if ret is False:
            platform_logic_monitor_error(f"[{self.name}] Failed to check value. {status}")
            return

        if status == CHECK_VALUE_OK:
            cur_status = CHECK_OK
        else:
            cur_status = CHECK_NOT_OK

        if self.last_status is None:
            platform_logic_monitor_info(f"[{self.name}] Initial status by check_value: {cur_status}. raw: {raw_val:#0x}")
            self.last_status = cur_status
        else:
            if cur_status != self.last_status:
                platform_logic_monitor_info(f"[{self.name}] Status changed from {self.last_status} to {cur_status} ({raw_val:#0x})")
                self.last_status = cur_status
            else:
                platform_logic_monitor_debug(f"[{self.name}] Status: {cur_status} ({raw_val:#0x})")

        if cur_status == CHECK_NOT_OK:
            #When the status is not OK and the raw_val changes, perform the action of clearing the status
            if self.last_raw_val is None or self.last_raw_val != raw_val:
                last_val_str = f"{self.last_raw_val:#0x}" if self.last_raw_val is not None else "None"
                platform_logic_monitor_info(f"[{self.name}] [{cur_status}] raw_val changed from {last_val_str} to {raw_val:#0x}")
                self.clear_status(raw_val)

        self.last_raw_val = raw_val

def get_all_sysfs_devices(sysfs_root, default_okval=LOGITC_STATUS_CHECK_VALUE_OK):
    devices = []
    if not os.path.isdir(sysfs_root):
        platform_logic_monitor_error(f"Sysfs path not found: {sysfs_root}")
        return devices
    for dev in os.listdir(sysfs_root):
        dev_path = os.path.join(sysfs_root, dev)
        status_path = os.path.join(dev_path, "status")
        if os.path.isfile(status_path):
            devices.append({
                "name": dev,
                "gettype": "sysfs",
                "loc": status_path,
                "okval": default_okval
            })
            platform_logic_monitor_debug(f"Discovered sysfs device: {dev} @ {status_path}")
    return devices

def main():
    debug_init()
    platform_logic_monitor_config = get_config_param("PLATFORM_LOGIC_MONITOR_CONFIG", {})
    sysfs_root = platform_logic_monitor_config.get("logic_dev_sysfs_file", LOGIC_DEV_FILE)
    poll_interval = platform_logic_monitor_config.get("polling_time", 30)
    dev_config = platform_logic_monitor_config.get("dev_config", [])

    #add custm monitor config
    device_configs = {}
    for cfg in dev_config:
        name = cfg.get("name") or cfg.get("loc") or cfg.get("path")
        device_configs[name] = cfg

    #auto add monitor config
    sysfs_devs = get_all_sysfs_devices(sysfs_root)
    for cfg in sysfs_devs:
        name = cfg["name"]
        if name not in device_configs:
            device_configs[name] = cfg

    if not device_configs:
        platform_logic_monitor_error("No logical device config")
        return

    monitors = []
    for name, cfg in device_configs.items():
        monitors.append(DeviceMonitor(name, cfg))

    platform_logic_monitor_debug(f"Total devices to monitor: {len(monitors)}")

    try:
        while True:
            debug_init()
            if os.path.exists(LOGIC_MON_CTRL_FILE) is True:
                platform_logic_monitor_debug("file exists, do nothing!")
                time.sleep(5)
                continue

            for dev in monitors:
                dev.monitor()
            time.sleep(poll_interval)
    except KeyboardInterrupt:
        platform_logic_monitor_info("Monitor stopped by user.")
    except Exception as e:
        platform_logic_monitor_error(f"Fatal error: {e}")

############################################
if __name__ == "__main__":
    main()