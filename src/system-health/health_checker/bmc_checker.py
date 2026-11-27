from sonic_py_common import device_info
from swsscommon import swsscommon

from .health_checker import HealthChecker

STATE_DB_ID = 6
REDIS_PORT = 6379
CONNECT_TIMEOUT_MSECS = 5000
READ_TIMEOUT_MSECS = 5000
SYSTEM_HEALTH_TABLE = 'SYSTEM_HEALTH_INFO'
META_FIELDS = ('summary', 'last_updated', 'polling_interval')


class BmcChecker(HealthChecker):

    def __init__(self):
        HealthChecker.__init__(self)
        self._bmc_addr = device_info.get_bmc_address()

    def get_category(self):
        return 'BMC'

    def reset(self):
        self._info = {}

    def check(self, config, chassis=None):
        self.reset()

        if not self._bmc_addr:
            self.set_object_not_ok('BMC', 'BMC', 'BMC address not configured')
            return

        link_state, message = chassis.get_bmc_if_link_status()
        if not link_state:
            self.set_object_not_ok('BMC', 'BMC-Connectivity', message)
            return

        try:
            db = swsscommon.DBConnector(STATE_DB_ID, self._bmc_addr,
                                        REDIS_PORT, CONNECT_TIMEOUT_MSECS)
            db.setTimeout(READ_TIMEOUT_MSECS)
        except Exception as e:
            self.set_object_not_ok('BMC', 'BMC-Connectivity',
                                  'Cannot connect to BMC Redis: {}'.format(e))
            return

        try:
            data = dict(db.hgetall(SYSTEM_HEALTH_TABLE))
        except Exception as e:
            self.set_object_not_ok('BMC', 'BMC-Connectivity',
                                  'Error reading BMC health data: {}'.format(e))
            return

        if not data:
            self.set_object_not_ok('BMC', 'BMC', 'No BMC health data available')
            return

        last_updated = data.get('last_updated', 'unknown')
        polling_interval = data.get('polling_interval', 'unknown')
        self.add_info('BMC-LastUpdate', 'type', 'BMC')
        self.add_info('BMC-LastUpdate', 'status', HealthChecker.STATUS_OK)
        self.add_info('BMC-LastUpdate', 'message',
                     'Last updated: {}'.format(last_updated))
        self.add_info('BMC-RefreshInterval', 'type', 'BMC')
        self.add_info('BMC-RefreshInterval', 'status', HealthChecker.STATUS_OK)
        self.add_info('BMC-RefreshInterval', 'message',
                     'BMC Refresh Interval: {}'.format(
                         '{}s'.format(polling_interval) if polling_interval != 'unknown' else polling_interval))

        for field, value in data.items():
            if field in META_FIELDS:
                continue
            if value:
                self.set_object_not_ok('BMC', field, value)
            else:
                self.set_object_ok('BMC', field)
