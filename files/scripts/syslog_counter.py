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
from swsscommon import swsscommon
import time

endedWithError = False
counters_db = None
syslog_count = 0

# initialize rsyslog plugin syslog, log message will write to the fine defined by output parameter
logging.basicConfig(stream=sys.stderr,
                    level=logging.WARNING,
                    format='%(asctime)s %(levelname)s %(message)s')

logging.debug("syslog counter plugin start initialize")

try:
    # initialize db connection and counter
    counters_db = swsscommon.SonicV2Connector()
    counters_db.connect('COUNTERS_DB')

    counter_value = counters_db.get('COUNTERS_DB', 'SYSLOG_COUNTER', 'COUNT')
    if counter_value == None:
        syslog_count = 0
    else:
        syslog_count = int(counter_value)
except Exception as e:
    logging.exception("syslog counter plugin initialization error, exiting program")
    # rsyslog will restart plugin when exit with 1
    sys.exit(1)

# plugin initialzied, send OK to rsyslog
print("OK", flush=True)

last_update_time = time.time()
try:
    # receive syslog message from rsyslog and count
    line = sys.stdin.readline()
    while line:
        syslog_count += 1

        # update counter every 1 minutes
        current_time = time.time()
        if current_time - last_update_time >= 60:
            counters_db.set('COUNTERS_DB', 'SYSLOG_COUNTER', 'COUNT', syslog_count)

        # Send the status code to rsyslog:
        print("OK", flush=True)
        line = sys.stdin.readline()
except Exception:
    logging.exception("syslog counter plugin unrecoverable error, exiting program")
    endedWithError = True

logging.debug("syslog counter plugin exit")

if endedWithError:
    sys.exit(1)
else:
    sys.exit(0)
