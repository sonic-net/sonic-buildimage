#!/usr/bin/env python

import signal
import time
import sys
from sonic_py_common import daemon_base, logger
from swsscommon import swsscommon

SYSLOG_IDENTIFIER = "bgp-session-tracker"
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)
DEFAULT_ROUTE_KEY = "0.0.0.0/0"
APPL_DB_NAME = "APPL_DB"
ROUTE_TABLE_NAME = "ROUTE_TABLE"
PORTCHANNEL_TABLE_NAME = "PORTCHANNEL"
BGP_NEIGHBOR_TABLE_NAME = "BGP_NEIGHBOR"
CONFIG_DB_BGP_DEVICE_GLOBAL_NAME = "BGP_DEVICE_GLOBAL"
BGP_SESSION_TRACKER_STATE_DB_TABLE_NAME = "BGP_SESSION_TRACKER"
LAG_TABLE_NAME = "LAG_TABLE"
FEATURE_TABLE_NAME = "FEATURE"
BGP_SESSION_TRACKER_INSTANCE_NAME = "T1toWL"

class BgpSessionTracker():
    """
    A class to monitor the db (APP_DB)
    """
    def __init__(self):
        self.should_monitor_run = False
        self.interfaces_down = False
        self.bgp_sessions_up = True
        self.feature_enabled = False
        self.downstream_portchannels = ['PortChannel1031', 'PortChannel1032']
        self.last_t1_bgp_session_update_time = None
        self.t1_bgp_session_cache = set()

        self.config_db = swsscommon.ConfigDBConnector()
        self.config_db.connect()
        self.config_db_connector = swsscommon.DBConnector("CONFIG_DB", 0, False)
        self.state_db = swsscommon.DBConnector("STATE_DB", 0, False)
        self.appl_db = daemon_base.db_connect(APPL_DB_NAME)
        self.state_db_tbl = swsscommon.Table(self.state_db, BGP_SESSION_TRACKER_STATE_DB_TABLE_NAME)
        self.lag_tbl = swsscommon.Table(self.state_db, LAG_TABLE_NAME)
        self.route_table = swsscommon.Table(self.appl_db, ROUTE_TABLE_NAME)

        self.sel = swsscommon.Select()
        self.tbl = swsscommon.SubscriberStateTable(self.appl_db, ROUTE_TABLE_NAME)
        self.sel.addSelectable(self.tbl)
        self.feature_tbl = swsscommon.SubscriberStateTable(self.config_db_connector, FEATURE_TABLE_NAME)
        self.sel.addSelectable(self.feature_tbl)
        self.nbr_tbl = swsscommon.SubscriberStateTable(self.config_db_connector, BGP_NEIGHBOR_TABLE_NAME)
        self.sel.addSelectable(self.nbr_tbl)

    def process_default_route_update(self):
        """
        Process the default route update
        """
        try:
            t1_nexthop_present = False
            nexthops = self.get_nexthops(DEFAULT_ROUTE_KEY)
            if nexthops:
                for nexthop in nexthops:
                    if nexthop in self.t1_bgp_session_cache:
                        logger_helper.log_notice("Nexthop: %s is a BGP session to T1" % (nexthop))
                        t1_nexthop_present = True
                        break
            if t1_nexthop_present:
                logger_helper.log_notice("Default route with nexthop to T1 found")
                self.update_state_db("bgp_sessions_up", "yes")
                if self.interfaces_down:
                    logger_helper.log_notice("Bringing up southbound interfaces")
                    self.update_southbound_portchannels('up')
                else:
                    logger_helper.log_notice("Southbound interfaces already up")
            else:
                logger_helper.log_notice("Default route with nexthop to T1 not found")
                self.update_state_db("bgp_sessions_up", "no")
                if self.interfaces_down == False:
                    logger_helper.log_notice("Bringing down southbound interfaces")
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
        logger_helper.log_notice("Fetching nexthops for route key: %s" % (route_key))
        try:
            route_entry = self.route_table.get(route_key)
            if route_entry[0] == True:
                logger_helper.log_notice("Route: %s found in route table. Route entry: %s" % (route_key, route_entry))
                nexthops = self.route_table.hget(route_key, 'nexthop')
                if nexthops[0] == True:
                    nexthop_list = nexthops[1].split(',')
                    logger_helper.log_notice("List of nexthops: %s" % (nexthop_list))
                else:
                    logger_helper.log_notice("Nexthops not found for route: %s" % (route_key))
            else:
                logger_helper.log_notice("Route: %s not found in route table" % (route_key))
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
        logger_helper.log_notice("Fetching BGP sessions to T1")
        try:
            data = self.config_db.get_table(BGP_NEIGHBOR_TABLE_NAME)
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
            data = self.config_db.get_table(PORTCHANNEL_TABLE_NAME)
            for portchannel in self.downstream_portchannels:
                if portchannel in data.keys():
                    portchannel_data = data[portchannel]
                    portchannel_data['admin_status'] = status
                    self.config_db.set_entry(PORTCHANNEL_TABLE_NAME, portchannel, portchannel_data)
                    time.sleep(5)  # Wait for the config to be applied
                    pc_admin_status = self.lag_tbl.hget(portchannel, 'admin_status')
                    if pc_admin_status[0] == True and pc_admin_status[1] == status:
                        logger_helper.log_notice("Portchannel: %s updated to status: %s" % (portchannel, status))
                    else:
                        logger_helper.log_warning("Portchannel: %s could not update to status: %s" % (portchannel, status))
                        success = False
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

    def southbound_interfaces_down(self):
        """
        Get the status of southbound interfaces.
        Returns False if any interface is up, True otherwise.
        """
        logger_helper.log_notice("Checking southbound interface status")
        try:
            data = self.config_db.get_table(PORTCHANNEL_TABLE_NAME)
            for portchannel in self.downstream_portchannels:
                if portchannel in data.keys():
                    if data[portchannel].get('admin_status') == 'up':
                        logger_helper.log_notice("Portchannel: %s is up" % (portchannel))
                        return False
            logger_helper.log_notice("All southbound interfaces are down")
            return True
        except Exception as e:
            logger_helper.log_warning("Exception in get_interface_status: {}".format(e))
            return False

    def enable_feature(self):
        """
        Enable the AzSBgpSessionTracker feature.
        Cache the T1 BGP sessions, run through the default route processing
        and update the state db accordingly.
        """
        logger_helper.log_notice("Feature AzSBgpSessionTracker is enabled")
        self.feature_enabled = True
        self.update_state_db("is_session_tracker_enabled", "yes")
        self.setup_t1_bgp_sessions_cache()
        self.process_default_route_update()

    def perform_initial_setup(self):
        """
        Perform initial setup to check if the feature is enabled and process the default route.
        """
        try:
            feature_table = self.config_db.get_table(FEATURE_TABLE_NAME)
            if 'AzSBgpSessionTracker' in feature_table:
                state = feature_table.get('AzSBgpSessionTracker', {}).get('state')
                if state == 'enabled':
                    self.enable_feature()
                    self.update_state_db("interfaces_are_shutdown", "yes" if self.interfaces_down else "no")
                else:
                    logger_helper.log_notice("Feature AzSBgpSessionTracker is not enabled")
        except Exception as e:
            logger_helper.log_warning("Exception in perform_initial_setup: {}".format(e))
            self.feature_enabled = False

    def process_feature_update(self, key, op, fvp):
        """
        Process the feature update for AzSBgpSessionTracker.
        """
        try:
            if op == 'SET':
                if fvp.get('state') == 'enabled':
                    if not self.feature_enabled:
                        self.interfaces_down = self.southbound_interfaces_down()
                        self.update_state_db("interfaces_are_shutdown", "yes" if self.interfaces_down else "no")
                        self.enable_feature()
                elif fvp.get('state') == 'disabled':
                    if self.feature_enabled:
                        logger_helper.log_notice("Feature AzSBgpSessionTracker disabled")
                        self.feature_enabled = False
                        self.update_state_db("is_session_tracker_enabled", "no")
            elif op == 'DEL':
                logger_helper.log_notice("Feature AzSBgpSessionTracker deleted")
                self.cleanup_state_db()
                self.feature_enabled = False
            else:
                logger_helper.log_warning("Unknown operation %s for feature %s" % (op, key))
        except Exception as e:
            logger_helper.log_warning("Exception in process_feature_update: {}".format(e))

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
                if selectableObj.getFd() == self.feature_tbl.getFd():
                    key, op, fvp = self.feature_tbl.pop()
                    if key == 'AzSBgpSessionTracker' and op in ['SET', 'DEL']:
                        logger_helper.log_notice("Feature AzSBgpSessionTracker changed. Key: %s, Op: %s, FVP: %s" % (key, op, fvp))
                        self.process_feature_update(key, op, dict(fvp))
                elif self.feature_enabled and selectableObj.getFd() == self.tbl.getFd():
                    key, op, fvp = self.tbl.pop()
                    if key == DEFAULT_ROUTE_KEY and op in ['SET', 'DEL']:
                        logger_helper.log_notice("Default route changed. Key: %s, Op: %s, FVP: %s" % (key, op, fvp))
                        self.process_default_route_update()
                elif self.feature_enabled and selectableObj.getFd() == self.nbr_tbl.getFd():
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
        self.cleanup_state_db()
        self.should_monitor_run = False

    def cleanup_state_db(self):
        """
        Clean up the state db
        """
        try:
            self.state_db_tbl.delete(BGP_SESSION_TRACKER_INSTANCE_NAME)
            logger_helper.log_notice("State DB cleaned up and deleted the entry for %s" % (BGP_SESSION_TRACKER_INSTANCE_NAME))
        except Exception as e:
            logger_helper.log_warning("Exception in cleanup_state_db: {}".format(e))

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