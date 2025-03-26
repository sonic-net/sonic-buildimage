#!/usr/bin/env python

from sonic_py_common import daemon_base, logger
from swsscommon import swsscommon

SYSLOG_IDENTIFIER = "bgp-session-tracker-base"
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)
DEFAULT_ROUTE_KEY = "0.0.0.0/0"
APPL_DB_NAME = "APPL_DB"
ROUTE_TABLE_NAME = "ROUTE_TABLE"
PORTCHANNEL_TABLE_NAME = "PORTCHANNEL"
BGP_NEIGHBOR_TABLE_NAME = "BGP_NEIGHBOR"

class BgpSessionTracker():
    """
    A class to monitor the db (APP_DB)
    """
    def __init__(self):
        self.working = False
        self.interfaces_down = False
        self.bgp_sessions_up = True
        self.downstream_portchannels = ['Po1031', 'Po1032']
        self.appl_db = daemon_base.db_connect(APPL_DB_NAME)
        self.sel = swsscommon.Select()
        self.tbl = swsscommon.SubscriberStateTable(self.appl_db, ROUTE_TABLE_NAME)
        self.sel.addSelectable(self.tbl)

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
            t1_bgp_sessions = self.get_t1_bgp_sessions(config_db)
            for nexthop in nexthops:
                if nexthop in t1_bgp_sessions:
                    logger_helper.log_notice("Nexthop: %s is a BGP session to T1" % (nexthop))
                    t1_nexthop_present = True
                    break
        if t1_nexthop_present:
            logger_helper.log_notice("Default route with nexthop to T1 found")
            if self.interfaces_down:
                self.interfaces_down = False
                logger_helper.log_notice("Bringing up southbound interfaces")
                self.update_southbound_portchannels('up', config_db)
            else:
                logger_helper.log_notice("Southbound interfaces already up")
        else:
            logger_helper.log_notice("Default route with nexthop to T1 not found, Bringing down southbound interfaces")
            self.update_southbound_portchannels('down', config_db)
            self.interfaces_down = True

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
    
    def get_t1_bgp_sessions(self, config_db):
        """
        Get the BGP sessions to T1
        """
        bgp_sessions = []
        logger_helper.log_notice("Fetching BGP sessions to T1")
        try:
            data = config_db.get_table(BGP_NEIGHBOR_TABLE_NAME)
            for peerip, value in data.items():
                if 'T1' in value['name']:
                    bgp_sessions.append(peerip)
            logger_helper.log_notice("Found bgp sessions: to T1 %s" % (bgp_sessions))
        except Exception as e:
            logger_helper.log_warning("Exception in get_t1_bgp_sessions: {}".format(e))
        return bgp_sessions
    
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
                logger_helper.log_notice("Device is not an AzS ToRRouter. BGP session tracker not supported")
                return False
            feature = config_db.get_table('FEATURE')
            if 'AzSBgpSessionTracker' not in feature or feature['AzSBgpSessionTracker']['state'] != 'enabled':
                logger_helper.log_notice("Feature: AzSBgpSessionTracker not enabled")
                return False
        except Exception as e:
            logger_helper.log_warning("Exception when checking if AzSBgpSessionTracker feature can run: {}".format(e))
            return False
        return True

    def start(self):
        """
        Start monitoring the table
        """
        self.working = True
        SELECT_TIMEOUT_MSECS = 1000
        while True:
            self.working = self.can_run_bgp_session_tracker()
            (state, selectableObj) = self.sel.select(SELECT_TIMEOUT_MSECS)
            if not self.working:
                break
            if state == swsscommon.Select.OBJECT:
                if selectableObj.getFd() == self.tbl.getFd():
                    (key, op, fvp) = self.tbl.pop()
                    self.process_new_entry(key, op, dict(fvp))

    def stop(self):
        """
        Stop the select operation and exit
        """
        self.working = False