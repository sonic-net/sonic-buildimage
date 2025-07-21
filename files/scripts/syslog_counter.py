#!/usr/bin/env python3
"""
syslog counter update plugin

Usage with rsyslog:
action(type="omprog" binary="/usr/bin/syslog_counter.py" output="/var/log/syslog_counter.log" confirmMessages="on")

More information:
https://www.rsyslog.com/doc/configuration/modules/omprog.html
"""

import sys
import logging
import threading
import time
from swsscommon import swsscommon

syslog_count = 0

# initialize rsyslog plugin syslog, log message will write to the fine defined by output parameter
logging.basicConfig(stream=sys.stderr,
                    level=logging.DEBUG,
                    format='%(asctime)s %(levelname)s %(message)s')

logging.debug("syslog counter plugin start initialize")


def update_counter_db():
    # Background thread function to periodically update COUNTERS_DB with the current syslog count.
    global syslog_count

    try:
        counters_db = swsscommon.SonicV2Connector()
        counters_db.connect('COUNTERS_DB')

        initial_value = counters_db.get('COUNTERS_DB', 'SYSLOG_COUNTER', 'COUNT')
        syslog_count = int(initial_value) if initial_value is not None else 0
    except Exception as e:
        logging.exception("Initialization error: {}, exiting.".format(e))
        # rsyslog will restart plugin when exit with 1
        sys.exit(1)

    while True:
        try:
            counters_db.set('COUNTERS_DB', 'SYSLOG_COUNTER', 'COUNT', syslog_count)
        except Exception as e:
            logging.exception("Database update error: {}, exiting.".format(e))
            sys.exit(1)

        time.sleep(60)


def main():
    # Main thread function to read syslog messages from stdin and increment the counter.
    global syslog_count
    ended_with_error = False

    # Start background thread
    update_thread = threading.Thread(target=update_counter_db, daemon=True)
    update_thread.start()

    # Notify rsyslog that plugin is ready
    print("OK", flush=True)

    try:
        # receive syslog message from rsyslog and count
        stdin_count = sum(1 for _ in sys.stdin)
        while stdin_count > 0:
            syslog_count += stdin_count

            # Send the status code to rsyslog:
            print("OK", flush=True)
            time.sleep(1)

            stdin_count = sum(1 for _ in sys.stdin)
    except Exception as e:
        logging.exception("syslog counter plugin unrecoverable error: {}, exiting program".format(e))
        ended_with_error = True

    logging.debug("Syslog counter plugin exiting.")
    sys.exit(1 if ended_with_error else 0)

if __name__ == "__main__":
    main()
