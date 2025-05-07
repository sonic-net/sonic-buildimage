#!/usr/bin/env python

import signal
import time
from datetime import datetime, timedelta
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
BGP_SESSION_TRACKER_INSTANCE_NAME = "T1toWL"
should_bgp_session_tracker_run = True

class BgpSessionTracker():
    """
    A class to monitor the db (APP_DB)
    """
    def __init__(self):
        self.should_monitor_run = False
        self.interfaces_down = False
        self.bgp_sessions_up = True
        self.downstream_portchannels = ['PortChannel1031', 'PortChannel1032']
        self.appl_db = daemon_base.db_connect(APPL_DB_NAME)
        self.sel = swsscommon.Select()
        self.tbl = swsscommon.SubscriberStateTable(self.appl_db, ROUTE_TABLE_NAME)
        self.sel.addSelectable(self.tbl)
        self.state_db = swsscommon.DBConnector("STATE_DB", 0, False)
        self.state_db_tbl = swsscommon.Table(self.state_db, BGP_SESSION_TRACKER_STATE_DB_TABLE_NAME)
        self.lag_tbl = swsscommon.Table(self.state_db, LAG_TABLE_NAME)
        self.route_table = swsscommon.Table(self.appl_db, ROUTE_TABLE_NAME)
        self.config_db = swsscommon.ConfigDBConnector()
        self.config_db.connect()
        self.last_t1_bgp_session_update_time = None
        self.t1_bgp_session_cache = set()

    def process_new_entry(self, key, op, fvp):
        """
        Process the new entry in the table
        """
        if not op in ['SET', 'DEL']:
            return
        if key == DEFAULT_ROUTE_KEY:
            logger_helper.log_notice("Changes to default route. key: %s, op: %s, fvp: %s" % (key, op, fvp))
            try:
                self.cache_t1_bgp_sessions()
                self.process_default_route_update()
            except Exception as e:
                logger_helper.log_warning("Exception in processing new entry: {}".format(e))

    def process_default_route_update(self):
        """
        Process the default route update
        """
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
                self.update_state_db("interfaces_are_shutdown", "no")
        else:
            logger_helper.log_notice("Default route with nexthop to T1 not found")
            self.update_state_db("bgp_sessions_up", "no")
            if self.interfaces_down == False:
                logger_helper.log_notice("Bringing down southbound interfaces")
                self.update_southbound_portchannels('down')
            else:
                logger_helper.log_notice("Southbound interfaces already down")
                self.update_state_db("interfaces_are_shutdown", "yes")

    def update_state_db(self, key, value):
        """
        Update the state db with the key and value
        """
        try:
            ret_code, current_fvs = self.state_db_tbl.get(BGP_SESSION_TRACKER_INSTANCE_NAME)
            if ret_code:
                fvs_dict = dict(current_fvs)
                fvs_dict[key] = value
                fvs = swsscommon.FieldValuePairs(list(fvs_dict.items()))
                self.state_db_tbl.set(BGP_SESSION_TRACKER_INSTANCE_NAME, fvs)
                logger_helper.log_notice("State DB updated with key: %s, value: %s" % (key, value))
            else:
                logger.helper.log_warning("BGP Session tracker instance %s not found when updating key: %s with value: %s" % (BGP_SESSION_TRACKER_INSTANCE_NAME, key, value))
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

    def cache_t1_bgp_sessions(self):
        """
        Get the BGP sessions to T1
        """
        current_time = datetime.now()
        if self.last_t1_bgp_session_update_time is None:
            self.last_t1_bgp_session_update_time = current_time
        else:
            elapsed_time = current_time - self.last_t1_bgp_session_update_time
            if elapsed_time < timedelta(hours=4):
                return
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
            logger_helper.log_warning("Exception in cache_t1_bgp_sessions: {}".format(e))

    def update_southbound_portchannels(self, status):
        """
        Update the admin status of southbound portchannels
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
                    pc_entry = self.lag_tbl.get(portchannel)
                    if pc_entry[0] == True:
                        pc_attr = self.lag_tbl.hget(portchannel, 'admin_status')
                        if pc_attr[0] == True:
                            intf_status = pc_attr[1]
                            if intf_status == status:
                                logger_helper.log_notice("Portchannel: %s updated to status: %s" % (portchannel, status))
                            else:
                                logger_helper.log_warning("Portchannel: %s could not update to status: %s" % (portchannel, status))
                                success = False
                    else:
                        logger_helper.log_warning("Portchannel: %s not found in LAG table when updating status to: %s" % (portchannel, status))
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

    def can_run_bgp_session_tracker(self):
        """
        Check if the feature is enabled
        """
        try:
            feature = self.config_db.get_table('FEATURE')
            ret_code, current_fvs = self.state_db_tbl.get(BGP_SESSION_TRACKER_INSTANCE_NAME)
            if 'AzSBgpSessionTracker' not in feature:
                if ret_code:
                    #clean up the state db if there is an entry but feature is not present in config db
                    self.cleanup_state_db()
                return False
            else:
                if feature['AzSBgpSessionTracker']['state'] != 'enabled':
                    if ret_code:
                        # if the feature is present in config db but not enabled and is_session_tracker_enabled is set to yes, set it to no
                        fvs_dict = dict(current_fvs)
                        if 'is_session_tracker_enabled' in fvs_dict and fvs_dict['is_session_tracker_enabled'] == 'yes':
                            self.update_state_db("is_session_tracker_enabled", "no")
                    else:
                        # if the feature is present in config db but not enabled and table for T1toWL is not present, create it.
                        self.state_db_tbl.set(BGP_SESSION_TRACKER_INSTANCE_NAME, swsscommon.FieldValuePairs([("is_session_tracker_enabled", "no")]))
                    return False
                else:
                    if ret_code:
                        # if the feature is present in config db and enabled, check if the state db entry is present and set to yes
                        fvs_dict = dict(current_fvs)
                        if 'is_session_tracker_enabled' in fvs_dict and fvs_dict['is_session_tracker_enabled'] == 'no':
                            self.update_state_db("is_session_tracker_enabled", "yes")
                    else:
                        # if the feature is present in config db and enabled and table for T1toWL is not present, create it.
                        self.state_db_tbl.set(BGP_SESSION_TRACKER_INSTANCE_NAME, swsscommon.FieldValuePairs([("is_session_tracker_enabled", "yes")]))
                    return True
        except Exception as e:
            logger_helper.log_warning("Exception when checking if AzSBgpSessionTracker feature can run: {}".format(e))
            return False

    def perform_initial_route_check(self):
        self.cache_t1_bgp_sessions()
        self.process_default_route_update(DEFAULT_ROUTE_KEY)

    def start(self):
        """
        Start monitoring the ROUTE_TABLE for updates.
        """
        self.should_monitor_run = True
        SELECT_TIMEOUT_MSECS = 1000

        if (self.can_run_bgp_session_tracker()):
            logger_helper.log_notice("Starting active monitoring for default route.")
            self.perform_initial_route_check()
            logger_helper.log_notice("Initial setup and checks completed.")
            while True:
                if not self.should_monitor_run:
                    break
                if not self.can_run_bgp_session_tracker():
                    break
                (state, selectableObj) = self.sel.select(SELECT_TIMEOUT_MSECS)
                if state == swsscommon.Select.OBJECT:
                    if selectableObj.getFd() == self.tbl.getFd():
                        (key, op, fvp) = self.tbl.pop()
                        self.process_new_entry(key, op, dict(fvp))
        else:
            return

    def stop(self):
        """
        Stop the select operation and exit
        """
        self.update_state_db("is_session_tracker_enabled", "no")
        self.should_monitor_run = False
        if self.sel:
            self.sel.interrupt()
    
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
    global should_bgp_session_tracker_run, bgp_session_tracker
    logger_helper.log_notice("BGP Session tracker going to quit")
    # Ignore any exception during quit
    try:
        if bgp_session_tracker:
            bgp_session_tracker.stop()
            bgp_session_tracker.cleanup_state_db()
        should_bgp_session_tracker_run = False
    except Exception as e:
        logger_helper.log_warning("Exception in signal handler: {}".format(e))
        #pass

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
    while True:
        if not should_bgp_session_tracker_run:
            break
        bgp_session_tracker.start()

if __name__ == "__main__":
    main()