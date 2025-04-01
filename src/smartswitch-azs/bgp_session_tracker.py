#!/usr/bin/env python

import signal
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
        self.last_t1_bgp_session_update_time = None
        self.t1_bgp_session_cache = []

    def process_new_entry(self, key, op, fvp):
        """
        Process the new entry in the table
        """
        if not op in ['SET', 'DEL']:
            return
        if key == DEFAULT_ROUTE_KEY:
            logger_helper.log_notice("Changes to default route. key: %s, op: %s, fvp: %s" % (key, op, fvp))
            try:
                app_db = swsscommon.DBConnector(APPL_DB_NAME, 0)
                route_tbl = swsscommon.Table(app_db, ROUTE_TABLE_NAME)
                config_db = swsscommon.ConfigDBConnector()
                config_db.connect()
                self.cache_t1_bgp_sessions(config_db)
                self.process_default_route_update(key, route_tbl, config_db)
            except Exception as e:
                logger_helper.log_warning("Exception in processing new entry: {}".format(e))

    def process_default_route_update(self, key, route_tbl, config_db):
        """
        Process the default route update
        """
        t1_nexthop_present = False
        nexthops = self.get_nexthops(key, route_tbl)
        if nexthops:
            for nexthop in nexthops:
                if nexthop in self.t1_bgp_session_cache:
                    logger_helper.log_notice("Nexthop: %s is a BGP session to T1" % (nexthop))
                    t1_nexthop_present = True
                    break
        if t1_nexthop_present:
            logger_helper.log_notice("Default route with nexthop to T1 found")
            if self.interfaces_down:
                logger_helper.log_notice("Bringing up southbound interfaces")
                self.update_southbound_portchannels('up', config_db)
                intf_status = self.validate_southbound_portchannels_state('up', config_db)
                if intf_status:
                    self.interfaces_down = False
                else:
                    logger_helper.log_warning("Could not bring up southbound portchannels in bgp session tracker.")
            else:
                logger_helper.log_notice("Southbound interfaces already up")
        else:
            logger_helper.log_notice("Default route with nexthop to T1 not found, Bringing down southbound interfaces")
            self.update_southbound_portchannels('down', config_db)
            intf_status = self.validate_southbound_portchannels_state('down', config_db)
            if intf_status:
                self.interfaces_down = True
            else:
                logger_helper.log_warning("Could not bring down southbound portchannels in bgp session tracker.")

    def validate_southbound_portchannels_state(self, state, config_db):
        """
        Validate that the southbound portchannels are in expected state.
        """
        data = config_db.get_table(PORTCHANNEL_TABLE_NAME)
        for portchannel in self.downstream_portchannels:
            if portchannel in data.keys():
                if data[portchannel]['admin_status'] != state:
                    logger_helper.log_warning("Portchannel %s is not in expected state: %s" % (portchannel, state))
                    return False
        return True

    def get_nexthops(self, route_key, route_tbl):
        """
        Get the nexthops for a route key
        """
        nexthop_list = []
        logger_helper.log_notice("Fetching nexthops for route key: %s" % (route_key))
        try:
            route_entry = route_tbl.get(route_key)
            if route_entry[0] == True:
                logger_helper.log_notice("Route: %s found in route table. Route entry: %s" % (route_key, route_entry))
                nexthops = route_tbl.hget(route_key, 'nexthop')
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

    def cache_t1_bgp_sessions(self, config_db):
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
            data = config_db.get_table(BGP_NEIGHBOR_TABLE_NAME)
            if data:
                for peerip, value in data.items():
                    if 'T1' in value['name']:
                        self.t1_bgp_session_cache.append(peerip)
                logger_helper.log_notice("Cached bgp sessions to T1: %s" % (self.t1_bgp_session_cache))
            else:
                logger_helper.log_warning("No BGP sessions to T1 found.")
        except Exception as e:
            logger_helper.log_warning("Exception in cache_t1_bgp_sessions: {}".format(e))

    def update_southbound_portchannels(self, status, config_db):
        """
        Update the admin status of southbound portchannels
        """
        logger_helper.log_notice("Updating southbound portchannels with status: %s" % (status))
        try:
            data = config_db.get_table(PORTCHANNEL_TABLE_NAME)
            for portchannel in self.downstream_portchannels:
                if portchannel in data.keys():
                    portchannel_data = data[portchannel]
                    portchannel_data['admin_status'] = status
                    config_db.set_entry(PORTCHANNEL_TABLE_NAME, portchannel, portchannel_data)
                    logger_helper.log_notice("Portchannel: %s updated with admin status: %s" % (portchannel, status))
                else:
                    logger_helper.log_notice("Portchannel: %s not found for updating status to: %s" % (portchannel, status))
        except Exception as e:
            logger_helper.log_warning("Exception in update_southbound_portchannels: {}".format(e))

    def can_run_bgp_session_tracker(self):
        """
        Check if the feature is enabled
        """
        try:
            config_db = swsscommon.ConfigDBConnector()
            config_db.connect()
            device_metadata = config_db.get_table('DEVICE_METADATA')
            if 'localhost' not in device_metadata or 'resource_type' not in device_metadata['localhost'] or device_metadata['localhost']['resource_type'] != '<placeholder>':
                return False
            feature = config_db.get_table('FEATURE')
            if 'AzSBgpSessionTracker' not in feature or feature['AzSBgpSessionTracker']['state'] != 'enabled':
                return False
        except Exception as e:
            logger_helper.log_warning("Exception when checking if AzSBgpSessionTracker feature can run: {}".format(e))
            return False
        return True

    def perform_initial_setup(self):
        app_db = swsscommon.DBConnector(APPL_DB_NAME, 0)
        route_tbl = swsscommon.Table(app_db, ROUTE_TABLE_NAME)
        config_db = swsscommon.ConfigDBConnector()
        config_db.connect()
        self.cache_t1_bgp_sessions(config_db)
        self.process_default_route_update(DEFAULT_ROUTE_KEY, route_tbl, config_db)

    def start(self):
        """
        Start monitoring the table
        """
        self.should_monitor_run = True
        SELECT_TIMEOUT_MSECS = 1000

        if (self.can_run_bgp_session_tracker()):
            logger_helper.log_notice("Starting active monitoring for default route.")
            self.perform_initial_setup()
            logger_helper.log_notice("Initial setup and checks completed.")
            while True:
                self.should_monitor_run = self.can_run_bgp_session_tracker()
                if not self.should_monitor_run:
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
        self.should_monitor_run = False

def signal_handler(signal, frame):
    """
    Gracefully exit
    """
    logger_helper.log_notice("BGP Session tracker going to quit")
    # Ignore any exception during quit
    try:
        if bgp_session_tracker:
            bgp_session_tracker.stop()
        should_bgp_session_tracker_run = False
    except:
        pass

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