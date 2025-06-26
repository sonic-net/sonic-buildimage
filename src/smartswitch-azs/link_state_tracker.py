#!/usr/bin/env python

import sys
import signal
import time
import inspect
from swsscommon import swsscommon
from sonic_py_common import logger
import threading

LINKSTATETRACKER_FEATURE_NAME = "LinkStateTracker"
SYSLOG_IDENTIFIER = LINKSTATETRACKER_FEATURE_NAME
logger_helper = logger.Logger(SYSLOG_IDENTIFIER)

LOOPBACK_INTERFACE = "Loopback6"
SOUTHBOUND_PORTCHANNELS = ["PortChannel1031", "PortChannel1032"]

LINKSTATETRACKER_INSTANCE_NAME = "WLPoToLo6"                                          # Instance name for the LinkStateTracker entry in STATE_DB                 
LINKSTATETRACKER_ENABLED_KEY = "is_link_state_tracker_enabled"                        # "yes/no"
LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY = "southbound_portchannels_status" # "up/down"
LINKSTATETRACKER_LOOPBACK_STATUS_KEY = "loopback6_status"                             # "enabled/disabled"

SELECT_TIMEOUT_MSECS = 1000
REDIS_TIMEOUT_MS = 0

g_link_state_tracker = None

class LinkStateTracker:
    """
    Main class to track link state of southbound portchannels and manage the loopback6 interface status.
    """
    def __init__(self):
        """Initilalize the LinkStateTracker"""
        self.should_monitor_run = False
        self.southbound_portchannels = SOUTHBOUND_PORTCHANNELS
        self.southbound_portchannels_cache = {}                 # Dict to store portchannel -> oper_status mapping
        self.loopback_status = None                             # "enabled"/"disabled"/None - Loopback status
        self.last_connectivity_status = None                    # "up"/"down"/None - whether we have connectivity through southbound portchannels / ANY portchannel is up

        self.config_db_connector = swsscommon.DBConnector("CONFIG_DB", REDIS_TIMEOUT_MS, True)
        self.config_db_loopback_tbl = swsscommon.Table(self.config_db_connector, swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME)
        self.state_db = swsscommon.DBConnector("STATE_DB", REDIS_TIMEOUT_MS, True)
        self.state_db_link_state_tracker_tbl = swsscommon.Table(self.state_db, swsscommon.STATE_LINK_STATE_TRACKER_TABLE_NAME)
        self.state_db_lag_table_read = swsscommon.Table(self.state_db, swsscommon.STATE_LAG_TABLE_NAME)
        self.appl_db = swsscommon.DBConnector("APPL_DB", REDIS_TIMEOUT_MS, True)
        self.appl_db_intf_tbl = swsscommon.Table(self.appl_db, swsscommon.APP_INTF_TABLE_NAME)

        self.sel = swsscommon.Select()
        self.lag_table = swsscommon.SubscriberStateTable(self.state_db, swsscommon.STATE_LAG_TABLE_NAME)
        self.sel.addSelectable(self.lag_table)

    def setup_southbound_portchannels_cache(self):
        """
        @summary: Full GET to initialize the portchannels cache
        @return:
            None (Updates self.southbound_portchannels_cache)
        """
        try:
            if not self.southbound_portchannels:
                logger_helper.log_warning("No southbound portchannels configured")
                return
    
            missing_portchannels = []

            for portchannel_name in self.southbound_portchannels:
                entry_exists, portchannel_data = self.state_db_lag_table_read.get(portchannel_name)
                if entry_exists:
                    portchannel_data_dict = dict(portchannel_data)
                    oper_status = portchannel_data_dict.get("oper_status", "unknown")
                    self.southbound_portchannels_cache[portchannel_name] = oper_status
                else:
                    missing_portchannels.append(portchannel_name)

            if self.southbound_portchannels_cache:
                logger_helper.log_notice(f"Cached southbound portchannel states: {self.southbound_portchannels_cache}")
            
            if missing_portchannels:
                logger_helper.log_warning(f"Configured southbound portchannels not found in LAG_TABLE: {missing_portchannels}")
        
        except Exception as e:
            logger_helper.log_error("Exception in setup_southbound_portchannels_cache: {}".format(e))
            self.southbound_portchannels_cache = {}  # Reset cache on error

    def should_update_southbound_portchannels_cache(self, key, op, fvp):
        """
        Update the southbound portchannels cache based on notification
        - DEL: Treat as portchannel going DOWN
        """
        try:
            old_status = self.southbound_portchannels_cache.get(key, "unknown")
            new_status = None
            if op == 'SET' and 'oper_status' in fvp:
                new_status = fvp['oper_status']
            elif op == 'DEL':
                new_status = "down"
            
            if new_status and old_status != new_status:
                self.southbound_portchannels_cache[key] = new_status
                logger_helper.log_notice(f"Portchannel {key} status changed: {old_status} -> {new_status}. op = {op}")
                return True   # status changed
        except Exception as e:
            logger_helper.log_warning(f"Exception in should_update_southbound_portchannels_cache: {e}")
        return False   # no change
    
    def process_portchannel_status_update(self):
        """
        Process portchannel status updates based on cached states
        This method is called when a portchannel status change is detected.
        """
        try:
            # Initialize connectivity flag - if any southbound portchannel is up based on cache
            # If no southbound portchannels are cached (none configured/available), disable loopback6
            current_connectivity_status = "down"

            # Check each portchannel status in cache
            for portchannel, status in self.southbound_portchannels_cache.items():
                if status == "up":
                    logger_helper.log_info(f"Portchannel: {portchannel} is UP (Connectivity present)")
                    current_connectivity_status = "up" 
                    break
            
            # 1. Update STATE_DB if connectivity status has changed and tracking variable
            if self.last_connectivity_status != current_connectivity_status:
                self.state_db_link_state_tracker_tbl.hset(LINKSTATETRACKER_INSTANCE_NAME, 
                    LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, current_connectivity_status)
                logger_helper.log_notice(f"Connectivity status changed: {self.last_connectivity_status} "
                    f"-> {current_connectivity_status}")
                self.last_connectivity_status = current_connectivity_status
            
            # 2. Update loopback6 interface based on connectivity status
            self.update_loopback_interface(current_connectivity_status)
        
        except Exception as e:
            logger_helper.log_warning(f"Exception in process_portchannel_status_update: {e}")

    def get_current_loopback_state(self):
        """Read the current loopback interface admin_status from APPL_DB"""
        try:
            exists, loopback_data = self.appl_db_intf_tbl.get(LOOPBACK_INTERFACE)
            if not exists:
                self.loopback_status = None  # Interface doesn't exist at all
                logger_helper.log_warning("Loopback6 interface not found in APPL_DB - Loopback6 not configured")
                return
            
            loopback_data_dict = dict(loopback_data)
            
            if "admin_status" in loopback_data_dict:
                admin_status = loopback_data_dict["admin_status"]
                self.loopback_status = "enabled" if admin_status == "up" else "disabled"
                logger_helper.log_info(f"Current Loopback6 admin_status from APPL_DB: {admin_status}, status={self.loopback_status}")
            else:
                logger_helper.log_warning("admin_status not found in APPL_DB INTF_TABLE for Loopback6")
                self.loopback_status = "disabled"  # Default to disabled if no admin_status
        
        except Exception as e:
            logger_helper.log_warning(f"Exception reading loopback state: {e}")
            self.loopback_status = None

    def update_loopback_interface(self, status):
        """Update the Loopback6 interface admin_status"""
        try:
            expected_loopback_status = "enabled" if status == 'up' else "disabled"
            exists, loopback_data = self.config_db_loopback_tbl.get(LOOPBACK_INTERFACE)

            # Only update if the status has actually changed
            if self.loopback_status != expected_loopback_status:
                # Update CONFIG_DB, STATE_DB and tracking variable
                self.config_db_loopback_tbl.hset(LOOPBACK_INTERFACE, 'admin_status', status)
                self.state_db_link_state_tracker_tbl.hset(LINKSTATETRACKER_INSTANCE_NAME, 
                    LINKSTATETRACKER_LOOPBACK_STATUS_KEY, expected_loopback_status)
                self.loopback_status = expected_loopback_status

                if not exists:
                    logger_helper.log_warning(f"Loopback6 interface not configured")
                else:
                    connectivity_reason = "detecting southbound portchannels UP" if status == 'up' else "detecting southbound portchannels DOWN"
                    logger_helper.log_notice(f"Having Loopback6 {expected_loopback_status} because of {connectivity_reason}. CONFIG_DB: admin_status={status}, STATE_DB: status={expected_loopback_status}")
            else:
                logger_helper.log_info(f"Loopback6 interface status is already {expected_loopback_status}, no update needed")

        except Exception as e:
            logger_helper.log_warning(f"Exception in update_loopback_interface: {e}")
    
    def perform_initial_setup(self):
        """
        Perform initial setup and process the portchannel states
        """
        try:
            self.state_db_link_state_tracker_tbl.hset(LINKSTATETRACKER_INSTANCE_NAME, 
                LINKSTATETRACKER_ENABLED_KEY, "yes")
            self.get_current_loopback_state()
            self.setup_southbound_portchannels_cache()
            self.process_portchannel_status_update()
        except Exception as e:
            logger_helper.log_error(f"Exception in perform_initial_setup: {e}")
    
    def start(self):
        """
        Start monitoring the LAG_table for portchannel status updates.
        """
        self.should_monitor_run = True

        try:
            self.perform_initial_setup()
            while self.should_monitor_run:
                select_status, _ = self.sel.select(SELECT_TIMEOUT_MSECS)
                if select_status == swsscommon.Select.TIMEOUT:
                    continue
                elif select_status == swsscommon.Select.ERROR:
                    logger_helper.log_error(f"Error while selecting from LAG_TABLE, retrying...")
                    time.sleep(0.25)
                    continue
                else:  # swsscommon.Select.OBJECT
                    lag, op, fvp = self.lag_table.pop()
                    if lag in self.southbound_portchannels:
                        logger_helper.log_info(f"LAG_TABLE event for monitored portchannel: {lag}, operation: {op}")
                        fvp_dict = dict(fvp)
                        # Update cache and process if there was a change
                        if self.should_update_southbound_portchannels_cache(lag, op, fvp_dict):
                            self.process_portchannel_status_update()
                    else:
                        logger_helper.log_info(f"Received event for non-monitored portchannel: {lag}")

        except Exception as e:
            logger_helper.log_error(f"Exception in {LINKSTATETRACKER_FEATURE_NAME} start: {e}")
        
    def stop(self):
        """
        Stop the LinkStateTracker service
        """
        logger_helper.log_notice(f"Stopping service...")
        self.should_monitor_run = False
        self.state_db_link_state_tracker_tbl.hset(LINKSTATETRACKER_INSTANCE_NAME, 
            LINKSTATETRACKER_ENABLED_KEY, "no")
        logger_helper.log_warning(f"{LINKSTATETRACKER_FEATURE_NAME} service stopped. {LINKSTATETRACKER_ENABLED_KEY} is set to 'no' in STATE_DB"
            f" {LOOPBACK_INTERFACE} status is {'enabled' if self.loopback_status else 'disabled'}")


def signal_handler(signum, frame):
    """
    Gracefully exit
    """
    global g_link_state_tracker
    logger_helper.log_notice(f"Received signal {signum}, {LINKSTATETRACKER_FEATURE_NAME} going to quit")
    try:
        if g_link_state_tracker:
            g_link_state_tracker.stop()
            g_link_state_tracker = None
    except Exception as e:
        logger_helper.log_warning(f"Exception in signal handler: {e}")
    sys.exit(0)

def main():
    """
    Entry point for the LinkStateTracker service.
    """
    logger_helper.log_notice(f"Starting service...")
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    global g_link_state_tracker
    g_link_state_tracker = LinkStateTracker()
    g_link_state_tracker.start()

if __name__ == "__main__":
    main()