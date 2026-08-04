#!/usr/bin/env python3
'''
bmpcfgd
Daemon which monitors bmp relevant table enablement from CONFIG_DB, and reset BMP states
'''

import os
import sys
import subprocess
import syslog
import signal
from shutil import copy2
from datetime import datetime
from sonic_py_common import logger
from sonic_py_common.general import check_output_pipe
from swsscommon.swsscommon import ConfigDBConnector, DBConnector, Table
from swsscommon import swsscommon
from sonic_py_common.daemon_base import DaemonBase

CFG_DB = "CONFIG_DB"
BMP_STATE_DB = "BMP_STATE_DB"
REDIS_HOSTIP = "127.0.0.1"
BMP_TABLE = "BMP"
SYSLOG_IDENTIFIER = "bmpcfgd"
logger = logger.Logger(SYSLOG_IDENTIFIER)

def is_true(val):
    return str(val).lower() == 'true'

class BMPCfg(DaemonBase):
    def __init__(self, state_db_conn):
        DaemonBase.__init__(self, SYSLOG_IDENTIFIER)
        self.bgp_neighbor_table  = False
        self.bgp_rib_in_table  = False
        self.bgp_rib_out_table  = False
        self.state_db_conn = state_db_conn


    def load(self, data={}):
        common_config = data.get('table', {})
        self.bgp_neighbor_table = is_true(common_config.get('bgp_neighbor_table', 'false'))
        self.bgp_rib_in_table = is_true(common_config.get('bgp_rib_in_table', 'false'))
        self.bgp_rib_out_table = is_true(common_config.get('bgp_rib_out_table', 'false'))
        logger.log_notice(f'bmpcfgd: config update : {self.bgp_neighbor_table}, {self.bgp_rib_in_table}, {self.bgp_rib_out_table}')

        # reset bmp table state once config is changed.
        self.stop_bmp()
        self.reset_bmp_table()
        self.start_bmp()


    def cfg_handler(self, data):
        self.load(data)


    def stop_bmp(self):
        logger.log_notice('bmpcfgd: stop bmp daemon')
        subprocess.call(["supervisorctl", "stop", "openbmpd"])


    def reset_bmp_table(self):
        logger.log_notice('bmpcfgd: Reset bmp table from state_db')
        self.state_db_conn.delete_all_by_pattern(BMP_STATE_DB, 'BGP_NEIGHBOR*')
        self.state_db_conn.delete_all_by_pattern(BMP_STATE_DB, 'BGP_RIB_IN_TABLE*')
        self.state_db_conn.delete_all_by_pattern(BMP_STATE_DB, 'BGP_RIB_OUT_TABLE*')


    def start_bmp(self):
        logger.log_notice('bmpcfgd: start bmp daemon')
        subprocess.call(["supervisorctl", "start", "openbmpd"])


class BMPCfgDaemon:
    def __init__(self):
        self.state_db_conn = swsscommon.SonicV2Connector(host=REDIS_HOSTIP)
        self.state_db_conn.connect(BMP_STATE_DB)
        self.config_db = ConfigDBConnector()
        self.config_db.connect(wait_for_init=True, retry_on=True)
        self.bmpcfg = BMPCfg(self.state_db_conn)

    def bmp_handler(self, key, data):
        data = self.config_db.get_table(BMP_TABLE)
        self.bmpcfg.cfg_handler(data)

    def register_callbacks(self):
        self.config_db.subscribe(BMP_TABLE,
                                 lambda table, key, data:
                                     self.bmp_handler(key, data))
        self.config_db.listen(init_data_handler=self.bmpcfg.load)

def main():
    daemon = BMPCfgDaemon()
    daemon.register_callbacks()


if __name__ == "__main__":
    main()
