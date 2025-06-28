#!/usr/bin/env python3

import subprocess
import sys
import time
import signal
from swsscommon import swsscommon
from sonic_py_common import logger

SYSLOG_IDENTIFIER = CONFIG_SAVE_FEATURE = "PeriodicConfigSave"
DEFAULT_ROUTE_KEY = "0.0.0.0/0"
TIMEOUT_SECONDS = 60
SLEEP_INTERVAL = 3600  # 1 hour

logger_helper = logger.Logger(SYSLOG_IDENTIFIER)

running = True

def signal_handler(signum, frame):
    """Gracefully exit"""
    global running
    logger_helper.log_notice(f"Received signal {signum}, {CONFIG_SAVE_FEATURE} going to quit")
    running = False
    sys.exit(0)

def run_config_save_single_cycle():
    try:
        appl_db = swsscommon.DBConnector("APPL_DB", 0, True)
        route_table = swsscommon.Table(appl_db, swsscommon.APP_ROUTE_TABLE_NAME)
        route_exists, _ = route_table.get(DEFAULT_ROUTE_KEY)
        if route_exists:
            logger_helper.log_notice("Default route present. Performing config save...")
            result = subprocess.run(
                ["sudo", "config", "save", "-y"],
                capture_output=True, text=True, timeout=TIMEOUT_SECONDS
            )
            if result.returncode == 0:
                logger_helper.log_notice("Config save operation is successful")
            else:
                logger_helper.log_error(f"Config save failed with return code {result.returncode}: {result.stderr}")
        else:
            logger_helper.log_warning("Default route not present. Skipping config save operation...")
                
    except subprocess.TimeoutExpired:
        logger_helper.log_error(f"Config save timed out after {TIMEOUT_SECONDS} seconds")
        
    except Exception as e:
        logger_helper.log_error(f"Exception in run_config_save_single_cycle: {str(e)}")

def main():
    global running

    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    logger_helper.log_notice(f"{CONFIG_SAVE_FEATURE} starting")
    
    while running:
        run_config_save_single_cycle()
        logger_helper.log_notice(f"Config save single cycle completed. Sleeping for 1 hour before next config save cycle...")
        time.sleep(SLEEP_INTERVAL)

if __name__ == "__main__":
    sys.exit(main())