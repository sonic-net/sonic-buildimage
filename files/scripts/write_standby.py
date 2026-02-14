#!/usr/bin/env python3

import argparse
import time
import sys

from sonic_py_common import logger as log
from swsscommon.swsscommon import ConfigDBConnector, DBConnector, FieldValuePairs, ProducerStateTable, SonicV2Connector, Table
from swsscommon.swsscommon import APPL_DB, STATE_DB 

logger = log.Logger('write_standby')

REDIS_SOCK_PATH = '/var/run/redis/redis.sock'


def create_fvs(**kwargs):
    return FieldValuePairs(list(kwargs.items()))

class MuxStateWriter(object):
    """
    Class used to write standby mux state to APP DB
    """

    def __init__(self, activeactive, activestandby, shutdown_module):
        self.config_db_connector = None
        self.appl_db_connector = None
        self.state_db_connector = None
        self.asic_db_connector = None
        self.default_active_active_state = activeactive
        self.default_active_standby_state = activestandby
        self.shutdown_module = shutdown_module
        self.is_shutdwon = (self.shutdown_module != None)

    @property
    def config_db(self):
        """
        Returns config DB connector.
        Initializes the connector during the first call
        """
        if self.config_db_connector is None:
            self.config_db_connector = ConfigDBConnector(use_unix_socket_path=True)
            self.config_db_connector.connect()

        return self.config_db_connector

    @property
    def appl_db(self):
        """
        Returns the app DB connector.
        Initializes the connector during the first call
        """
        if self.appl_db_connector is None:
            self.appl_db_connector = DBConnector(APPL_DB, REDIS_SOCK_PATH, True)
        return self.appl_db_connector
    
    @property
    def state_db(self):
        """
        Returns the state DB connector.
        Intializes the connector during the first call
        """
        if self.state_db_connector is None:
            self.state_db_connector = DBConnector(STATE_DB, REDIS_SOCK_PATH, True)
        return self.state_db_connector

    @property
    def asic_db(self):
        """
        Returns the ASIC DB connector.
        Initializes the connector during the first call
        """
        if self.asic_db_connector is None:
            self.asic_db_connector = SonicV2Connector(use_unix_socket_path=True)
            self.asic_db_connector.connect('ASIC_DB')

        return self.asic_db_connector

    @property
    def tunnel_name(self):
        """
        Returns the name of the IP-in-IP tunnel used for Dual ToR devices
        """
        return self.config_db.get_keys('TUNNEL')[0]

    @property
    def is_dualtor(self):
        """
        Checks if script is running on a dual ToR system
        """
        localhost_key = self.config_db.get_keys('DEVICE_METADATA')[0]
        metadata = self.config_db.get_entry('DEVICE_METADATA', localhost_key)

        return 'subtype' in metadata and 'dualtor' in metadata['subtype'].lower()

    @property
    def is_warmrestart(self):
        """
        Checks if a warmrestart is going on
        """
        tbl = Table(self.state_db, 'WARM_RESTART_ENABLE_TABLE')
        (status, value) = tbl.hget('system', 'enable')

        if status and value == 'true':
            return True

        if self.shutdown_module:
            (status, value) = tbl.hget(self.shutdown_module, 'enable')
            if status and value == 'true':
                return True

        return False

    def get_all_mux_intfs_modes(self):
        """
        Returns a list of all mux cable interfaces, with suggested modes
        Setting mux initial modes is crucial to kick off the statemachines,
        have to set the modes for all mux/gRPC ports.
        """
        intf_modes = {}
        all_intfs = self.config_db.get_table('MUX_CABLE')
        for intf, status in all_intfs.items():
            state = status['state'].lower()
            if state in ['active', 'standby']:
                intf_modes[intf] = state
            elif state in ['auto', 'manual']:
                if ('soc_ipv4' in status or 'soc_ipv6' in status or
                    ('cable_type' in status and status['cable_type'] == 'active-active')):
                    intf_modes[intf] = self.default_active_active_state
                else:
                    intf_modes[intf] = self.default_active_standby_state
        return intf_modes

    def tunnel_exists(self):
        """
        Checks if the IP-in-IP tunnel has been written to ASIC DB
        """
        tunnel_key_pattern = 'ASIC_STATE:SAI_OBJECT_TYPE_TUNNEL:*'
        return len(self.asic_db.keys('ASIC_DB', tunnel_key_pattern)) > 0

    def wait_for_tunnel(self, interval=1, timeout=160):
        """
        Waits until the IP-in-IP tunnel has been created

        Returns:
            (bool) True if the tunnel has been created
                   False if the timeout period is exceeded
        """
        logger.log_info("Waiting for tunnel {} with timeout {} seconds".format(self.tunnel_name, timeout))
        start = time.time()
        curr_time = time.time()

        while not self.tunnel_exists() and curr_time - start < timeout:
            time.sleep(interval) 
            curr_time = time.time()

        # If we timed out, return False else return True
        return curr_time - start < timeout

    def apply_mux_config(self):
        """
        Writes standby mux state to APP DB for all mux interfaces
        """
        if not self.is_dualtor:
            logger.log_warning("It is not a Dual-ToR system, do not start mux container")
            sys.exit(1)

        if self.is_warmrestart and self.is_shutdwon:
            # If in warmrestart context, take no action
            logger.log_warning("Skip setting mux state due to ongoing warmrestart.")
            return

        modes = self.get_all_mux_intfs_modes()
        if self.wait_for_tunnel():
            logger.log_warning("Applying state to interfaces {}".format(modes))
            producer_state_table = ProducerStateTable(self.appl_db, 'MUX_CABLE_TABLE')

            for intf, state in modes.items():
                fvs = create_fvs(state=state)
                producer_state_table.set(intf, fvs)
        else:
            logger.log_error("Timed out waiting for tunnel {}, mux state will not be written".format(self.tunnel_name))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Write initial mux state')
    parser.add_argument('-a', '--active_active',
                        help='state: intial state for "auto" and/or "manual" config in active-active mode, default "active"',
                        type=str, required=False, default='active')
    parser.add_argument('-s', '--active_standby',
                        help='state: intial state for "auto" and/or "manual" config in active-standby mode, default "standby"',
                        type=str, required=False, default='standby')
    parser.add_argument('--shutdown', help='write mux state after shutdown other services, supported: mux, bgp',
                        type=str, required=False, choices=['mux', 'bgp'], default=None)
    args = parser.parse_args()
    active_active_state = args.active_active
    active_standby_state = args.active_standby
    if args.shutdown in ['mux', 'bgp']:
        active_active_state = "standby"
    mux_writer = MuxStateWriter(activeactive=active_active_state, activestandby=active_standby_state, shutdown_module=args.shutdown)
    mux_writer.apply_mux_config()
