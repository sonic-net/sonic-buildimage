from sonic_py_common import device_info
from sonic_platform_base.chassis_base import ChassisBase
from swsscommon import swsscommon

from .health_checker import HealthChecker

STATE_DB_ID = 6
REDIS_PORT = 6379
CONNECT_TIMEOUT_MSECS = 5000
SYSTEM_HEALTH_TABLE = 'SYSTEM_HEALTH_INFO'


class BmcChecker(HealthChecker):

    def __init__(self):
        HealthChecker.__init__(self)
        self._bmc_addr = device_info.get_bmc_address()
        self._chassis = ChassisBase()

    def get_category(self):
        return 'BMC'

    def reset(self):
        self._info = {}

    def check(self, config):
        self.reset()

        if not self._bmc_addr:
            self.set_object_not_ok('BMC', 'BMC', 'BMC address not configured')
            return

        link_state, message = self._chassis.get_bmc_if_link_status()
        if not link_state:
            self.set_object_not_ok('BMC', 'BMC-Connectivity', message)
            return

        try:
            db = swsscommon.DBConnector(STATE_DB_ID, self._bmc_addr,
                                        REDIS_PORT, CONNECT_TIMEOUT_MSECS)
            tbl = swsscommon.Table(db, SYSTEM_HEALTH_TABLE)
        except Exception as e:
            self.set_object_not_ok('BMC', 'BMC-Connectivity',
                                  'Cannot connect to BMC Redis: {}'.format(e))
            return

        try:
            keys = tbl.getKeys()
        except Exception as e:
            self.set_object_not_ok('BMC', 'BMC-Connectivity',
                                  'Error reading BMC health data: {}'.format(e))
            return

        if not keys:
            self.set_object_not_ok('BMC', 'BMC', 'No BMC health data available')
            return

        for key in keys:
            ok, fvs = tbl.get(key)
            if not ok:
                continue
            data = dict(fvs)
            if key == '_meta':
                last_updated = data.get('last_updated', 'unknown')
                polling_interval = data.get('polling_interval', 'unknown')
                self.add_info('BMC-LastUpdate', 'type', 'BMC')
                self.add_info('BMC-LastUpdate', 'status', HealthChecker.STATUS_OK)
                self.add_info('BMC-LastUpdate', 'message',
                             'Last updated: {}'.format(last_updated))
                self.add_info('BMC-RefreshInterval', 'type', 'BMC')
                self.add_info('BMC-RefreshInterval', 'status', HealthChecker.STATUS_OK)
                self.add_info('BMC-RefreshInterval', 'message',
                             'BMC Refresh Interval: {}s'.format(polling_interval))
                continue

            obj_status = data.get('status', 'Unknown')
            obj_message = data.get('message', '')
            if obj_status != HealthChecker.STATUS_OK:
                self.set_object_not_ok('BMC', key, obj_message)
            else:
                self.set_object_ok('BMC', key)
