#!/usr/bin/env python
#
# SPDX-License-Identifier: MIT
#
# Copyright (c) 2024 Xsightlabs Ltd.

try:
    import os
    import os.path
    import sys
    import logging
    import time
    import signal
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

FUNCTION_NAME = 'es9618xx_monitor'
SHUTDOWN_FILE = '/usr/share/sonic/firmware/pmon_system_shutdown'
SYSTEM_HALT_ON_OVERHEAT = 1
SYSTEM_WARN_ON_OVERHEAT = 2

global logger

def handler(signum, frame):
    logging.info('Cause signal %d.', signum)
    sys.exit(0)

def main(argv):
    logger = logging.getLogger(FUNCTION_NAME)
    logging.basicConfig(level=logging.INFO)

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)

    logger.info("Started endless loop.")
    while True:
        if os.path.exists(SHUTDOWN_FILE) and os.path.isfile(SHUTDOWN_FILE):
            try:
                file = open(SHUTDOWN_FILE)
                file_content = file.readline().split('-')
                ext_string = ""
                if 1 < len(file_content):
                    ext_string = file_content[1]
                if SYSTEM_HALT_ON_OVERHEAT == int(file_content[0]):
                    os.remove(SHUTDOWN_FILE)
                    os.system("/opt/xplt/utils/es9618x_reset_x2.sh")
                    logger.warning("System HALT due to high temperature of: {} !".format(ext_string))
                    os.system("/usr/sbin/shutdown -H now")
                if SYSTEM_WARN_ON_OVERHEAT == int(file_content[0]):
                    logger.warning("System warning due to overheat of: {} !".format(ext_string))
                    os.remove(SHUTDOWN_FILE)
            except:
                time.sleep(1)
        else:
            time.sleep(2)

if __name__ == '__main__':
    main(sys.argv[1:])
