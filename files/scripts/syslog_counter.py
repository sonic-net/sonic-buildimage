#!/usr/bin/env python3
"""
syslog counter update plugin

this plugin need load with omprog:
action(type="omprog" binary="/usr/bin/syslog_counter.py" output="/var/log/syslog_counter.log" confirmMessages="on")

For more information:
https://www.rsyslog.com/doc/configuration/modules/omprog.html
"""
import sys
import logging
import threading
import time
from swsscommon import swsscommon

syslog_count = 0

def update_counter_db():
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
    global syslog_count

    # initialize rsyslog plugin syslog, log message will write to the fine defined by output parameter
    logging.basicConfig(stream=sys.stderr,
                        level=logging.WARNING,
                        format='%(asctime)s %(levelname)s %(message)s')

    logging.debug("syslog counter plugin start initialize")

    # Start background thread
    update_thread = threading.Thread(target=update_counter_db, daemon=True)
    update_thread.start()

    # plugin initialzied, send OK to rsyslog
    print("OK", flush=True)

    last_update_time = time.time()
    try:
        # receive syslog message from rsyslog and count
        line = sys.stdin.readline()
        while line:
            syslog_count += 1

            # Send the status code to rsyslog after every syslog
            print("OK", flush=True)
            line = sys.stdin.readline()
    except Exception as e:
        logging.exception("syslog counter plugin unrecoverable error: {}, exiting program".format(e))
        sys.exit(1)

    logging.debug("Syslog counter plugin exiting.")
    sys.exit(0)

if __name__ == "__main__":
    main()
