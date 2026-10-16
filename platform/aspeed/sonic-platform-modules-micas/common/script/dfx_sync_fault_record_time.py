#!/usr/bin/env python3
import os
import logging


from platform_config import get_config_param
from platform_util import setup_logger, BSP_COMMON_LOG_DIR


DEBUG_FILE = "/etc/.dfx_sync_fault_record_time_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "dfx_sync_fault_record_time.log"
logger = setup_logger(LOG_FILE)


FAULT_RECORD_TIME_SYSFS = "/sys/bus/i2c/devices/%s/fault_date"


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)


def log_error(msg):
    logger.error(msg)


def log_info(msg):
    logger.info(msg)


def log_debug(msg):
    logger.debug(msg)


class SyncFaultRecordTime():

    def sync_fault_record_time(self):
        devices = get_config_param("DFX_SYNC_FAULT_RECORD_TIME", [])
        if not devices:
            log_info("No devices configured for fault record time sync.")
            return

        for device in devices:
            try:
                name = device.get("name", "unknown")
                loc = device.get("loc")
                if not loc:
                    log_error(f"Device {name} missing 'loc' parameter.")
                    continue

                path = FAULT_RECORD_TIME_SYSFS % loc
                log_debug(f"Syncing fault record time for device '{name}' at path: {path}")

                try:
                    with open(path, 'w') as f:
                        f.write('1\n')
                    log_info(f"{name} sync_fault_record_time: success")
                except Exception as e:
                    log_error(f"Failed to sync {name} fault_record_time: {e}")

            except Exception as e:
                log_error(f"Exception while processing device {device}: {e}")

    def run(self):
        self.sync_fault_record_time()


if __name__ == '__main__':
    debug_init()
    log_debug("Starting sync fault record time process.")
    sync_fault_record_time = SyncFaultRecordTime()
    sync_fault_record_time.run()
