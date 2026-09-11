#!/usr/bin/env python_nos
import sys
import os
import time
import syslog
import click
import logging
from platform_config import get_config_param
from platform_util import exec_os_cmd, setup_logger, BSP_COMMON_LOG_DIR, AliasedGroup, CONTEXT_SETTINGS
from public.platform_common_config import (PRODUCT_STRATEGY, PRODUCT_STRATEGY_2)

WATCHDOG_CONFIG = get_config_param("WATCHDOG_CONFIG", "watchdog0")
DEBUG_FILE = "/etc/.platform_watchdog_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_watchdog.log"
logger = setup_logger(LOG_FILE)

WDT_DEV_PATH = f"/dev/{WATCHDOG_CONFIG}"
WDT_STOP_PATH = f"/sys/class/watchdog/{WATCHDOG_CONFIG}/device/no_feed_wdt"


def platform_watchdog_debug(s):
    logger.debug(s)


def platform_watchdog_error(s):
    logger.error(s)


def platform_watchdog_info(s):
    logger.info(s)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def is_watchdog_exist():
    return os.path.exists(WDT_DEV_PATH)


def is_watchdog_stoppable():
    return os.path.exists(WDT_STOP_PATH)

def feed_watchdog():
    if not is_watchdog_stoppable():
        platform_watchdog_error(f"Stop watchdog node {WDT_STOP_PATH} not found!")
        return False, "Stop node not exists"

    command = f"echo 0 > {WDT_STOP_PATH}"
    platform_watchdog_info(f"starting watchdog via {WDT_STOP_PATH}")
    status, output = exec_os_cmd(command)

    if status == 0:
        platform_watchdog_info(f"Watchdog {WATCHDOG_CONFIG} started successfully")
        return True, output
    else:
        platform_watchdog_error(f"Failed to start watchdog. Command: {command}. Log: {output}")
        return False, output

def start_watchdog():

    result, output = feed_watchdog()
    if not result:
        platform_watchdog_error(f"Failed to feed watchdog. Log: {output}")
        return False, output

    if PRODUCT_STRATEGY == PRODUCT_STRATEGY_2:
        platform_watchdog_info("Watchdog do not need enable in product strategy 2, skipping start.")
        return True, "Watchdog not need enable in this product strategy"

    if not is_watchdog_exist():
        platform_watchdog_error(f"Watchdog device {WDT_DEV_PATH} not found!")
        return False, "Device not exists"
    command = f"echo 1 > {WDT_DEV_PATH}"
    platform_watchdog_info(f"Starting watchdog via {WDT_DEV_PATH}")
    status, output = exec_os_cmd(command)
    
    if status == 0:
        platform_watchdog_info(f"Watchdog {WATCHDOG_CONFIG} started successfully")
        return True, output
    else:
        platform_watchdog_error(f"Failed to start watchdog. Command: {command}. Log: {output}")
        return False, output


def stop_watchdog():
    if not is_watchdog_stoppable():
        platform_watchdog_error(f"Stop watchdog node {WDT_STOP_PATH} not found!")
        return False, "Stop node not exists"

    command = f"echo 1 > {WDT_STOP_PATH}"
    platform_watchdog_info(f"Stopping watchdog via {WDT_STOP_PATH}")
    status, output = exec_os_cmd(command)
    
    if status == 0:
        platform_watchdog_info(f"Watchdog {WATCHDOG_CONFIG} stopped successfully")
        return True, output
    else:
        platform_watchdog_error(f"Failed to stop watchdog. Command: {command}. Log: {output}")
        return False, output


@click.group(cls=AliasedGroup, context_settings=CONTEXT_SETTINGS)
def main():
    '''Watchdog management tool'''
    debug_init()


@main.command()
def start():
    '''Start watchdog service'''
    platform_watchdog_info("=== Starting watchdog process ===")
    status, log = start_watchdog()
    sys.exit(0 if status else 1)


@main.command()
def stop():
    '''Stop watchdog service'''
    platform_watchdog_info("=== Stopping watchdog process ===")
    status, log = stop_watchdog()
    sys.exit(0 if status else 1)


@main.command()
def status():
    '''Check watchdog status'''
    print(f"=== Watchdog {WATCHDOG_CONFIG} status ===")
    
    if is_watchdog_exist():
        print(f"Device {WDT_DEV_PATH}: Exists")
    else:
        print(f"Device {WDT_DEV_PATH}: Not found")
    
    if is_watchdog_stoppable():
        print(f"Stop node {WDT_STOP_PATH}: Exists")
    else:
        print(f"Stop node {WDT_STOP_PATH}: Not found")
    
    state_path = f"/sys/class/watchdog/{WATCHDOG_CONFIG}/state"
    if os.path.exists(state_path):
        with open(state_path, 'r') as f:
            state = f.read().strip()
        print(f"Watchdog state: {state}")
    else:
        print("Watchdog state: Not available")

    state_path = f"/sys/class/watchdog/{WATCHDOG_CONFIG}/device/wdt_status"
    if os.path.exists(state_path):
        with open(state_path, 'r') as f:
            state = f.read().strip()
        print(f"Watchdog wdt_status: {state}")
    else:
        print("Watchdog wdt_status: Not available")

    timeout_path = f"/sys/class/watchdog/{WATCHDOG_CONFIG}/device/wdt_timeout_value"
    if os.path.exists(timeout_path):
        with open(timeout_path, 'r') as f:
            timeout_value = f.read().strip()
        print(f"Watchdog timeout: {timeout_value}")
    else:
        print("Watchdog timeout: Not available")

    sys.exit(0)


if __name__ == '__main__':
    main()