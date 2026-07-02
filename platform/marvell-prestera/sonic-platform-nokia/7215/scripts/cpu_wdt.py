#!/usr/bin/python

from sonic_platform.chassis import Chassis
from sonic_py_common import logger
import time
import os
import signal
import sys


TIMEOUT=170
KEEPALIVE=55
sonic_logger = logger.Logger('Watchdog')
sonic_logger.set_min_log_priority_info()
time.sleep(60)
chassis = Chassis()
watchdog = chassis.get_watchdog()

def stopWdtService(signal, frame):
    watchdog._disablewatchdog()
    sonic_logger.log_notice("CPUWDT Disabled: watchdog armed=%s" % watchdog.is_armed() )
    sys.exit()

def main():

    signal.signal(signal.SIGHUP, signal.SIG_IGN)
    signal.signal(signal.SIGINT, stopWdtService)
    signal.signal(signal.SIGTERM, stopWdtService)
    
    watchdog.arm(TIMEOUT)
    sonic_logger.log_notice("CPUWDT Enabled: watchdog armed=%s" % watchdog.is_armed() )


    while True:
        time.sleep(KEEPALIVE)
        watchdog._keepalive()
        sonic_logger.log_info("CPUWDT keepalive")
    done

    stopWdtService

    return


if __name__ == '__main__':
    main()
