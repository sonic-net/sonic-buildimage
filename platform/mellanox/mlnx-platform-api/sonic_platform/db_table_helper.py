from swsscommon.swsscommon import DBConnector, Table
from sonic_py_common import multi_asic

TRANSCEIVER_DOM_TEMPERATURE_TABLE = 'TRANSCEIVER_DOM_TEMPERATURE'
TRANSCEIVER_DOM_THRESHOLD_TABLE = 'TRANSCEIVER_DOM_THRESHOLD'
TRANSCEIVER_INFO_TABLE = 'TRANSCEIVER_INFO'
STATE_DB = 'STATE_DB'
APPL_DB = 'APPL_DB'


class DbTableHelper:    
    def __init__(self):
        self.state_dbs = {}
        self.appl_dbs = {}
        self.temperature_tables = {}
        self.threshold_tables = {}
        self.info_tables = {}
        self._initialized = False
        
    def initialize(self):
        if not self._initialized:
            for namespace in multi_asic.get_front_end_namespaces():
                self.state_dbs[namespace] = DBConnector(STATE_DB, 0, True, namespace)
                self.appl_dbs[namespace] = DBConnector(APPL_DB, 0, True, namespace)
                self.temperature_tables[namespace] = Table(self.state_dbs[namespace], TRANSCEIVER_DOM_TEMPERATURE_TABLE)
                self.threshold_tables[namespace] = Table(self.state_dbs[namespace], TRANSCEIVER_DOM_THRESHOLD_TABLE)
                self.info_tables[namespace] = Table(self.state_dbs[namespace], TRANSCEIVER_INFO_TABLE)
            self._initialized = True
    
    def get_temperature_table(self, namespace):
        self.initialize()
        return self.temperature_tables[namespace]

    def get_threshold_table(self, namespace):
        self.initialize()
        return self.threshold_tables[namespace]

    def get_info_table(self, namespace):
        self.initialize()
        return self.info_tables[namespace]
    
    def get_appl_db(self, namespace):
        self.initialize()
        return self.appl_dbs[namespace]
    
    def get_state_db(self, namespace):
        self.initialize()
        return self.state_dbs[namespace]

# Global instance of DbTableHelper
db_table_helper = DbTableHelper()
