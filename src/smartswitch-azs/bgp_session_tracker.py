#!/usr/bin/env python3

import signal
import time
import sys
from sonic_py_common import daemon_base, logger
from swsscommon import swsscommon

SYSLOG_IDENTIFIER = "bgp-session-tracker"
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)
DEFAULT_ROUTE_KEY = "0.0.0.0/0"
BGP_SESSION_TRACKER_INSTANCE_NAME = "T1toWL"

class BgpSessionTracker():
    """
    A class to monitor the db (APP_DB)
    """
    def __init__(self):
        self.should_monitor_run = False
        self.interfaces_down = None
        self.downstream_portchannels = ['PortChannel1031', 'PortChannel1032']
        self.t1_bgp_session_cache = set()

        self.config_db = swsscommon.ConfigDBConnector()
        self.config_db.connect()
        self.config_db_connector = swsscommon.DBConnector("CONFIG_DB", 0, False)
        self.state_db = swsscommon.DBConnector("STATE_DB", 0, False)
        self.appl_db = daemon_base.db_connect("APPL_DB")
        self.state_db_tbl = swsscommon.Table(self.state_db, swsscommon.STATE_BGP_SESSION_TRACKER_TABLE_NAME)
        self.lag_tbl = swsscommon.Table(self.state_db, swsscommon.STATE_LAG_TABLE_NAME)
        self.route_table = swsscommon.Table(self.appl_db, swsscommon.APP_ROUTE_TABLE_NAME)

        self.sel = swsscommon.Select()
        self.tbl = swsscommon.SubscriberStateTable(self.appl_db, swsscommon.APP_ROUTE_TABLE_NAME)
        self.sel.addSelectable(self.tbl)
        self.nbr_tbl = swsscommon.SubscriberStateTable(self.config_db_connector, swsscommon.CFG_BGP_NEIGHBOR_TABLE_NAME)
        self.sel.addSelectable(self.nbr_tbl)

    def process_default_route_update(self):
        """
        Process the default route update
        """
        try:
            t1_nexthop_present = False
            nexthops = self.get_nexthops(DEFAULT_ROUTE_KEY)
            for nexthop in nexthops:
                if nexthop in self.t1_bgp_session_cache:
                    logger_helper.log_notice("Nexthop: %s is a BGP session to T1" % (nexthop))
                    t1_nexthop_present = True
                    break
            if t1_nexthop_present:
                self.update_state_db("bgp_sessions_up", "yes")
                if self.interfaces_down is not False:
                    self.update_southbound_portchannels('up')
                else:
                    logger_helper.log_notice("Southbound interfaces already up")
            else:
                logger_helper.log_notice("Default route with nexthop to T1 not found")
                self.update_state_db("bgp_sessions_up", "no")
                if self.interfaces_down is not True:
                    self.update_southbound_portchannels('down')
                else:
                    logger_helper.log_notice("Southbound interfaces already down")
        except Exception as e:
            logger_helper.log_warning("Exception in process_default_route_update: {}".format(e))

    def update_state_db(self, key, value):
        """
        Update the state db with the key and value
        """
        try:
            self.state_db_tbl.hset(BGP_SESSION_TRACKER_INSTANCE_NAME, key, value)
            logger_helper.log_notice("State DB updated with key: %s, value: %s" % (key, value))
        except Exception as e:
            logger_helper.log_warning("Exception in updating state db: {}".format(e))

    def get_nexthops(self, route_key):
        """
        Get the nexthops for a route key
        """
        nexthop_list = []
        try:
            nexthops = self.route_table.hget(route_key, 'nexthop')
            if nexthops[0] == True:
                nexthop_list = nexthops[1].split(',')
            else:
                logger_helper.log_notice("Nexthops not found for route: %s" % (route_key))
        except Exception as e:
            logger_helper.log_warning("Exception in get_nexthops: {}".format(e))
        return nexthop_list

    def update_t1_bgp_sessions_cache(self, key, op, fvp):
        """
        Update the T1 BGP sessions cache based on the key, operation and field value pairs.
        """
        if op == 'SET':
            if 'T1' in fvp.get('name', ''):
                self.t1_bgp_session_cache.add(key)
                logger_helper.log_notice("Added BGP session to T1: %s. Updated cache: %s" % (key, self.t1_bgp_session_cache))
        elif op == 'DEL':
            if key in self.t1_bgp_session_cache:
                self.t1_bgp_session_cache.remove(key)
                logger_helper.log_notice("Removed BGP session to T1: %s. Updated cache: %s" % (key, self.t1_bgp_session_cache))

    def setup_t1_bgp_sessions_cache(self):
        """
        Get the BGP sessions to T1
        """
        try:
            data = self.config_db.get_table(swsscommon.CFG_BGP_NEIGHBOR_TABLE_NAME)
            if data:
                for peerip, value in data.items():
                    if 'T1' in value['name']:
                        self.t1_bgp_session_cache.add(peerip)
                logger_helper.log_notice("Cached bgp sessions to T1: %s" % (self.t1_bgp_session_cache))
            else:
                logger_helper.log_warning("No BGP sessions to T1 found.")
        except Exception as e:
            logger_helper.log_warning("Exception in setup_t1_bgp_sessions_cache: {}".format(e))

    def update_southbound_portchannels(self, status):
        """
        Update the admin status of southbound portchannels in config db PORTCHANNEL table.
        Validate their status in state db LAG_TABLE.
        """
        logger_helper.log_notice("Updating southbound portchannels with status: %s" % (status))
        success = True
        try:
            data = self.config_db.get_table(swsscommon.CFG_LAG_TABLE_NAME)
            for portchannel in self.downstream_portchannels:
                if portchannel in data.keys():
                    portchannel_data = data[portchannel]
                    portchannel_data['admin_status'] = status
                    self.config_db.set_entry(swsscommon.CFG_LAG_TABLE_NAME, portchannel, portchannel_data)
                else:
                    logger_helper.log_notice("Portchannel: %s not found for updating status to: %s" % (portchannel, status))
                    success = False
            if success:
                if status == 'up':
                    self.interfaces_down = False
                    self.update_state_db("interfaces_are_shutdown", "no")
                else:
                    self.interfaces_down = True
                    self.update_state_db("interfaces_are_shutdown", "yes")
        except Exception as e:
            logger_helper.log_warning("Exception in update_southbound_portchannels: {}".format(e))

    def perform_initial_setup(self):
        """
        Perform initial setup and process the default route.
        """
        try:
            self.update_state_db("is_session_tracker_enabled", "yes")
            self.setup_t1_bgp_sessions_cache()
            self.process_default_route_update()
        except Exception as e:
            logger_helper.log_warning("Exception in perform_initial_setup: {}".format(e))

    def start(self):
        """
        Start monitoring the ROUTE_TABLE for updates.
        """
        self.should_monitor_run = True
        SELECT_TIMEOUT_MSECS = 1000

        try:
            self.perform_initial_setup()
            while self.should_monitor_run:
                state, selectableObj = self.sel.select(SELECT_TIMEOUT_MSECS)
                if state != swsscommon.Select.OBJECT:
                    continue
                if selectableObj.getFd() == self.tbl.getFd():
                    key, op, fvp = self.tbl.pop()
                    if key == DEFAULT_ROUTE_KEY and op in ['SET', 'DEL']:
                        logger_helper.log_notice("Default route changed. Key: %s, Op: %s, FVP: %s" % (key, op, fvp))
                        self.process_default_route_update()
                elif selectableObj.getFd() == self.nbr_tbl.getFd():
                    key, op, fvp = self.nbr_tbl.pop()
                    if '|' not in key and op in ['SET', 'DEL']:
                        logger_helper.log_notice("T1 BGP neighbor changed. Key: %s, Op: %s, FVP: %s" % (key, op, fvp))
                        self.update_t1_bgp_sessions_cache(key, op, dict(fvp))
        except Exception as e:
            logger_helper.log_warning("Exception in BgpSessionTracker start: {}".format(e))

    def stop(self):
        """
        Stop the select operation and exit
        """
        self.update_state_db("is_session_tracker_enabled", "no")
        self.should_monitor_run = False

def signal_handler(signal, frame):
    """
    Gracefully exit
    """
    global bgp_session_tracker
    logger_helper.log_notice("BGP Session tracker going to quit")
    # Ignore any exception during quit
    try:
        if bgp_session_tracker:
            bgp_session_tracker.stop()
            bgp_session_tracker = None
    except Exception as e:
        logger_helper.log_warning("Exception in signal handler: {}".format(e))
        #pass
    sys.exit(0)

def main():
    """
    The entry of bgp_session_tracker
    """
    logger_helper.log_notice("BGP Session tracker starting")
    # Install signal handler
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    global bgp_session_tracker
    bgp_session_tracker = BgpSessionTracker()
    bgp_session_tracker.start()

if __name__ == "__main__":
    main()
