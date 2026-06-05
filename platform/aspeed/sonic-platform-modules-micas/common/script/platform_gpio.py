#!/usr/bin/env python_nos
import sys
import os
import time
import syslog
import click
import logging
from platform_config import get_config_param
from platform_util import exec_os_cmd, setup_logger, BSP_COMMON_LOG_DIR, AliasedGroup, CONTEXT_SETTINGS

PLATFORM_GPIO_CONFIG = get_config_param("PLATFORM_GPIO_CONFIG", [])
DEBUG_FILE = "/etc/.platform_gpio_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_gpio_debug.log"
logger = setup_logger(LOG_FILE)

'''
Example 
PLATFORM_GPIO_CONFIG = [
    {"num": 1, "init": "high"},
    {"num": 2, "init": "low"},
    {"num": 3, "init": "input"}
]
'''


def platform_gpio_debug(s):
    logger.debug(s)


def platform_gpio_error(s):
    logger.error(s)


def platform_gpio_info(s):
    logger.info(s)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def is_gpio_exported(gpio_num):
    """Check if a GPIO is exported by verifying the existence of its sysfs directory."""
    gpio_path = "/sys/class/gpio/gpio%d" % gpio_num
    return os.path.exists(gpio_path)


def export_gpio(gpio_num):
    if is_gpio_exported(gpio_num):
        return True, f"GPIO {gpio_num} is exported."
    command = "echo %d > /sys/class/gpio/export" % gpio_num
    platform_gpio_info("Exporting GPIO %d" % gpio_num)
    status, output = exec_os_cmd(command)
    if status == 0:
        return True, output
    else:
        return False, output


def unexport_gpio(gpio_num):
    if not is_gpio_exported(gpio_num):
        return True, f"GPIO {gpio_num} not exported, skip."
    command = "echo %d > /sys/class/gpio/unexport" % gpio_num
    platform_gpio_info("Unexporting GPIO %d" % gpio_num)
    status, output = exec_os_cmd(command)
    if status == 0:
        return True, output
    else:
        return False, output


def set_gpio_direction_and_level(gpio_num, init):
    """
    set GPIO direction
    - init : "high" or "low" or "in"
    """
    if init == "high":
        command = "echo high > /sys/class/gpio/gpio%d/direction" % gpio_num
        platform_gpio_info("Setting GPIO %d direction to 'out' with level 'high'" % gpio_num)
    elif init == "low":
        command = "echo low > /sys/class/gpio/gpio%d/direction" % gpio_num
        platform_gpio_info("Setting GPIO %d direction to 'out' with level 'low'" % gpio_num)
    elif init == "input":
        command = "echo in > /sys/class/gpio/gpio%d/direction" % gpio_num
        platform_gpio_info("Setting GPIO %d direction to 'in'" % gpio_num)
    else:
        platform_gpio_error("Invalid init value for GPIO %d: %s" % (gpio_num, init))
        return True, "Invalid init value"
    status, output = exec_os_cmd(command)
    if status == 0:
        return True, output
    else:
        return False, output


def initialize_gpio(gpio_config):
    """configutation GPIO"""
    for gpio in gpio_config:
        gpio_num = gpio.get("num")
        init = gpio.get("init")

        if gpio_num is None or init is None:
            platform_gpio_error("Invalid GPIO configuration: %s" % gpio)
            platform_gpio_error("action uninitialize_gpio")
            uninitialize_gpio(gpio_config)
            platform_gpio_error("uninitialize_gpio end")
            exit(1)

        # export GPIO
        status, log = export_gpio(gpio_num)
        if status == False:
            platform_gpio_error("    GPIO %d export fail. log: %s " % (gpio_num, log))
            continue
        # Set GPIO direction and level
        status, log = set_gpio_direction_and_level(gpio_num, init)
        if status == False:
            platform_gpio_error(f"GPIO {gpio_num} direction setup failed. Log: {log}")

    platform_gpio_info("GPIO initialization completed.")


def uninitialize_gpio(gpio_config):
    """unexport GPIO"""
    for gpio in gpio_config:
        gpio_num = gpio.get("num")
        if gpio_num is None:
            platform_gpio_error("Invalid GPIO configuration: %s" % gpio)
            continue

        status, log = unexport_gpio(gpio_num)
        if status == False:
            platform_gpio_error("    GPIO %d unexport fail. log: %s " % (gpio_num, log))

    platform_gpio_info("GPIO uninitialization completed.")


def check_gpio_status(gpio_config):
    """Check if GPIOs are exported or unexported."""
    for gpio in gpio_config:
        gpio_num = gpio.get("num")
        if gpio_num is None:
            platform_gpio_error(f"Invalid GPIO configuration: {gpio}")
            continue

        if is_gpio_exported(gpio_num):
            platform_gpio_info(f"GPIO {gpio_num} is exported.")
        else:
            platform_gpio_info(f"GPIO {gpio_num} is not exported.")


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''device operator'''
    debug_init()

@main.command()
def start():
    '''start gpio configuration'''
    platform_gpio_info("GPIO initialization start:")
    initialize_gpio(PLATFORM_GPIO_CONFIG)
    time.sleep(5)
    sys.exit(0)


@main.command()
def stop():
    '''stop drivers device '''
    platform_gpio_info("GPIO uninitialization:")
    uninitialize_gpio(PLATFORM_GPIO_CONFIG)
    time.sleep(5)
    sys.exit(0)


@main.command()
def check():
    """Check GPIO status."""
    check_gpio_status(PLATFORM_GPIO_CONFIG)


if __name__ == '__main__':
    main()
