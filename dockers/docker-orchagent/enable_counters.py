#!/usr/bin/env python3

import time
from swsscommon import swsscommon

# ALPHA defines the size of the window over which we calculate the average value. ALPHA is 2/(N+1) where N is the interval(window size)
# In this case we configure the window to be 10s. This way if we have a huge 1s spike in traffic,
# the average rate value will show a curve descending from the spike to the usual rate over approximately 10s.
DEFAULT_SMOOTH_INTERVAL = '10'
DEFAULT_ALPHA = '0.18'


def enable_counter_group(db, name):
    entry_info = db.get_entry("FLEX_COUNTER_TABLE", name)

    if not entry_info:
        info = {}
        info['FLEX_COUNTER_STATUS'] = 'enable'
        db.mod_entry("FLEX_COUNTER_TABLE", name, info)
    else:
        entry_info.update({"FLEX_COUNTER_DELAY_STATUS":"false"})
        db.mod_entry("FLEX_COUNTER_TABLE", name, entry_info)

def enable_rates():
    # set the default interval for rates
    counters_db = swsscommon.SonicV2Connector()
    counters_db.connect('COUNTERS_DB')
    counters_db.set('COUNTERS_DB', 'RATES:PORT', 'PORT_SMOOTH_INTERVAL', DEFAULT_SMOOTH_INTERVAL)
    counters_db.set('COUNTERS_DB', 'RATES:PORT', 'PORT_ALPHA', DEFAULT_ALPHA)
    counters_db.set('COUNTERS_DB', 'RATES:RIF', 'RIF_SMOOTH_INTERVAL', DEFAULT_SMOOTH_INTERVAL)
    counters_db.set('COUNTERS_DB', 'RATES:RIF', 'RIF_ALPHA', DEFAULT_ALPHA)
    counters_db.set('COUNTERS_DB', 'RATES:TRAP', 'TRAP_SMOOTH_INTERVAL', DEFAULT_SMOOTH_INTERVAL)
    counters_db.set('COUNTERS_DB', 'RATES:TRAP', 'TRAP_ALPHA', DEFAULT_ALPHA)
    counters_db.set('COUNTERS_DB', 'RATES:TUNNEL', 'TUNNEL_SMOOTH_INTERVAL', DEFAULT_SMOOTH_INTERVAL)
    counters_db.set('COUNTERS_DB', 'RATES:TUNNEL', 'TUNNEL_ALPHA', DEFAULT_ALPHA)


def enable_counters():
    db = swsscommon.ConfigDBConnector()
    db.connect()
    default_enabled_counters = ['PORT', 'RIF', 'QUEUE', 'PFCWD', 'PG_WATERMARK', 'PG_DROP', 
                                'QUEUE_WATERMARK', 'BUFFER_POOL_WATERMARK', 'PORT_BUFFER_DROP', 'ACL']
    
    # Enable those default counters
    for key in default_enabled_counters:
        enable_counter_group(db, key)

    # Set FLEX_COUNTER_DELAY_STATUS to false for those non-default counters
    keys = db.get_keys('FLEX_COUNTER_TABLE')
    for key in keys:
        if key not in default_enabled_counters:
            enable_counter_group(db, key)
    enable_rates()


def get_uptime():
    with open('/proc/uptime') as fp:
        return float(fp.read().split(' ')[0])


def main():
    # If the switch was just started (uptime less than 5 minutes),
    # wait for 3 minutes and enable counters
    # otherwise wait for 60 seconds and enable counters
    uptime = get_uptime()
    if uptime < 300:
        time.sleep(180)
    else:
        time.sleep(60)
    enable_counters()


if __name__ == '__main__':
    main()
