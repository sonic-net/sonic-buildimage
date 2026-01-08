#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import threading
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
_thread_local = threading.local()


def get_db_table_helper():
    if not hasattr(_thread_local, 'helper'):
        _thread_local.helper = DbTableHelper()
    return _thread_local.helper
