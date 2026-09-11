#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import logging
import time

from platform_util import get_value, set_value, setup_logger, BSP_COMMON_LOG_DIR
from platform_config import MGMT_LED_CONFIG

DEBUG_FILE = "/etc/.mgmt_ledcontrol_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "mgmt_ledctrl_debug.log"
logger = setup_logger(LOG_FILE)


class PortConfig(object):
    def __init__(self, name, link_path, led_path):
        self.name = name
        self.link_path = link_path
        self.led_path = led_path


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def mgmt_led_debug(msg):
    logger.debug(msg)


def mgmt_led_error(msg):
    logger.error(msg)


def read_link_status(path):
    config = {
        "gettype": "sysfs",
        "loc": path,
        "int_decode": 16,
    }
    ret, value = get_value(config)
    if ret is not True:
        raise RuntimeError("read link status failed, path:{}, msg:{}".format(path, value))
    if value not in (0, 1):
        raise ValueError("unsupported link_status value:{} path:{}".format(value, path))
    mgmt_led_debug("read link status path:{} value:{}".format(path, value))
    return value


def write_led_status(path, value):
    config = {
        "gettype": "sysfs",
        "loc": path,
        "value": int(value),
        "mask": 0,
    }
    ret, msg = set_value(config)
    mgmt_led_debug("write led status path:{} value:{} ret:{} msg:{}".format(path, value, ret, msg))
    return ret, msg


def load_runtime_config_from_product():
    if MGMT_LED_CONFIG is None or not isinstance(MGMT_LED_CONFIG, dict):
        raise ValueError("MGMT_LED_CONFIG error: %s" % MGMT_LED_CONFIG)

    interval = float(MGMT_LED_CONFIG.get("interval", 3))
    if interval <= 0:
        raise ValueError("MGMT_LED_CONFIG.interval must be > 0")

    led_list = MGMT_LED_CONFIG.get("mgmt_led_list", [])
    if not isinstance(led_list, list) or len(led_list) == 0:
        raise ValueError("MGMT_LED_CONFIG.mgmt_led_list must be non-empty list")

    ports = []
    for idx, item in enumerate(led_list):
        if not isinstance(item, dict):
            raise ValueError("MGMT_LED_CONFIG.mgmt_led_list[{}] must be dict".format(idx))

        name = item.get("name", "mgmt_led_{}".format(idx))
        link_path = item.get("link_path")
        led_path = item.get("led_path")

        if not link_path:
            raise ValueError("MGMT_LED_CONFIG.mgmt_led_list[{}].link_path is required".format(idx))
        if not led_path:
            raise ValueError("MGMT_LED_CONFIG.mgmt_led_list[{}].led_path is required".format(idx))

        ports.append(PortConfig(name, link_path, led_path))

    mgmt_led_debug("load MGMT_LED_CONFIG success interval:{} port_num:{}".format(interval, len(ports)))
    return ports, interval


def monitor_once(port):
    link_status = read_link_status(port.link_path)
    if link_status == 1:
        led_status = 1
    else:
        led_status = 0
    ret, msg = write_led_status(port.led_path, led_status)
    if ret is not True:
        raise RuntimeError("set led failed, path={}, value={}, msg={}"
                           .format(port.led_path, led_status, msg))
    mgmt_led_debug("monitor {} done link:{} led:{}".format(port.name, link_status, led_status))


def main():
    debug_init()
    ports, interval = load_runtime_config_from_product()

    while True:
        debug_init()
        for port in ports:
            try:
                monitor_once(port)
            except Exception as exc:
                mgmt_led_error("[{}] monitor error: {}".format(port.name, str(exc)))
        time.sleep(interval)


if __name__ == "__main__":
    main()
