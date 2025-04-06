#!/usr/bin/env python3

"""
update_syslog_counter

This script will get recent 10 minutes syslog count and update to COUNTERS_DB
"""

import syslog
import os
from swsscommon import swsscommon


def count_syslog():
    try:
        result = os.popen(r'sudo journalctl --since "10 minutes ago" | wc -l').read()
        return int(result)
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR, "[update_syslog_counter] Failed to count syslog, exception: '{}'".format(e))

    return 0


def main():
    count = count_syslog()

    counters_db = swsscommon.SonicV2Connector()
    counters_db.connect('COUNTERS_DB')
    current = counters_db.get('COUNTERS_DB', 'SYSLOG_COUNTER', 'COUNT')

    if current != None:
        count = int(current) + count

    counters_db.set('COUNTERS_DB', 'SYSLOG_COUNTER', 'COUNT', count)


if __name__ == "__main__":
    main()
