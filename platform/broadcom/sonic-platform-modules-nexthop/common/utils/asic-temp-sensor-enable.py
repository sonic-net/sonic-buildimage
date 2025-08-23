#!/usr/bin/python3

from swsscommon.swsscommon import SonicV2Connector
from sonic_py_common import logger

INTERVAL_SECONDS = '10'

LOG_TAG = "asic_temp_sensor_enable"
log = logger.Logger(LOG_TAG)
log.set_min_log_priority_info()

log.log_info("Enabling asic temperature sensor polling")

db = SonicV2Connector()
db.connect(db.CONFIG_DB)

if db.get(db.CONFIG_DB, "ASIC_SENSORS|ASIC_SENSORS_POLLER_STATUS", 'admin_status') != 'enable':
    log.log_info("Enabling asic temperature sensor polling")
    db.set(db.CONFIG_DB, "ASIC_SENSORS|ASIC_SENSORS_POLLER_STATUS", 'admin_status', 'enable')
else:
    log.log_warning("Asic temperature sensor polling is already enabled")

if db.get(db.CONFIG_DB, "ASIC_SENSORS|ASIC_SENSORS_POLLER_INTERVAL", 'interval') != INTERVAL_SECONDS:
    log.log_info(f"Setting asic temperature sensor polling interval to {INTERVAL_SECONDS} seconds")
    db.set(db.CONFIG_DB, "ASIC_SENSORS|ASIC_SENSORS_POLLER_INTERVAL", 'interval', INTERVAL_SECONDS)
else:
    log.log_info(f"Asic temperature sensor polling interval is already set to {INTERVAL_SECONDS} seconds")
