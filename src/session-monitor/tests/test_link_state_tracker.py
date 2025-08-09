#!/usr/bin/env python3

import pytest
import unittest.mock as mock
import sys
import os
import time

# Add the parent directory to the path to import the link_state_tracker.py
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

sys.modules['sonic_py_common'] = mock.MagicMock()
sys.modules['sonic_py_common.logger'] = mock.MagicMock()
sys.modules['swsscommon'] = mock.MagicMock()
sys.modules['swsscommon.swsscommon'] = mock.MagicMock()

from tests.mock_sonic_py_common import Logger
from tests.mock_swsscommon import (
    DBConnector, Table, 
    SubscriberStateTable, Select
)

class TestLinkStateTracker:
    """Tests for LinkStateTracker class"""
    
    def setup_method(self):
        """Setup for each test method"""
        self.mock_config_db_connector = DBConnector("CONFIG_DB", 0, False)
        self.mock_config_db_loopback_table = Table(self.mock_config_db_connector, "LOOPBACK_INTERFACE")
        self.mock_state_db = DBConnector("STATE_DB", 0, False)
        self.mock_state_db_link_tracker_table = Table(self.mock_state_db, "LINK_STATE_TRACKER_TABLE")
        self.mock_state_db_lag_table = Table(self.mock_state_db, "LAG_TABLE")
        self.mock_appl_db = DBConnector("APPL_DB", 0, False)
        self.mock_appl_db_intf_table = Table(self.mock_appl_db, "INTF_TABLE")
        self.mock_select = Select()
        self.mock_lag_subscriber_table = SubscriberStateTable(self.mock_state_db, "LAG_TABLE")
        
        self.patcher = mock.patch('link_state_tracker.swsscommon')
        self.mock_swsscommon = self.patcher.start()
        
        self.mock_swsscommon.DBConnector.side_effect = self._db_connector_side_effect
        self.mock_swsscommon.Table.side_effect = self._table_side_effect
        self.mock_swsscommon.Select.return_value = self.mock_select
        self.mock_swsscommon.SubscriberStateTable.return_value = self.mock_lag_subscriber_table
        self.mock_swsscommon.Select.TIMEOUT = 0
        self.mock_swsscommon.Select.ERROR = 1
        self.mock_swsscommon.Select.OBJECT = 2
        
        self.mock_swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME = "LOOPBACK_INTERFACE"
        self.mock_swsscommon.STATE_LINK_STATE_TRACKER_TABLE_NAME = "LINK_STATE_TRACKER_TABLE"
        self.mock_swsscommon.STATE_LAG_TABLE_NAME = "LAG_TABLE"
        self.mock_swsscommon.APP_INTF_TABLE_NAME = "INTF_TABLE"

        from link_state_tracker import LinkStateTracker
        self.tracker = LinkStateTracker()
        
    def teardown_method(self):
        """Clean up after each test"""
        self.patcher.stop()
        
    def _db_connector_side_effect(self, db_name, timeout, use_unix_socket_path):
        """Side effect for DBConnector mock"""
        if db_name == "CONFIG_DB":
            return self.mock_config_db_connector
        elif db_name == "STATE_DB":
            return self.mock_state_db
        elif db_name == "APPL_DB":
            return self.mock_appl_db
        return DBConnector(db_name, timeout, use_unix_socket_path)

    def _table_side_effect(self, db, table_name):
        """Side effect for Table mock"""
        if table_name == self.mock_swsscommon.STATE_LINK_STATE_TRACKER_TABLE_NAME:
            return self.mock_state_db_link_tracker_table
        elif table_name == self.mock_swsscommon.STATE_LAG_TABLE_NAME:
            return self.mock_state_db_lag_table
        elif table_name == self.mock_swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME:
            return self.mock_config_db_loopback_table
        elif table_name == self.mock_swsscommon.APP_INTF_TABLE_NAME:
            return self.mock_appl_db_intf_table
        else:
            return Table(db, table_name)

    def test_initialization(self):
        """Test LinkStateTracker initialization"""
        assert self.tracker.should_monitor_run == False
        assert self.tracker.loopback_status is None
        assert self.tracker.southbound_portchannels == ["PortChannel1031", "PortChannel1032"]
        assert self.tracker.southbound_portchannels_cache == {}
        assert self.tracker.last_connectivity_status is None

    def test_setup_southbound_portchannels_cache_all_up(self):
        """Test initial setup when all portchannels are up"""
        # Setup: All southbound portchannels are up in LAG_TABLE
        self.mock_state_db_lag_table.set_data("PortChannel1031", {"oper_status": "up"})
        self.mock_state_db_lag_table.set_data("PortChannel1032", {"oper_status": "up"})
        
        # Execute
        self.tracker.setup_southbound_portchannels_cache()
        
        # Verify cache is populated correctly
        expected_cache = {
            "PortChannel1031": "up",
            "PortChannel1032": "up"
        }
        assert self.tracker.southbound_portchannels_cache == expected_cache

    def test_setup_southbound_portchannels_cache_all_down(self):
        """Test initial setup when all portchannels are down"""
        # Setup: All southbound portchannels are down
        self.mock_state_db_lag_table.set_data("PortChannel1031", {"oper_status": "down"})
        self.mock_state_db_lag_table.set_data("PortChannel1032", {"oper_status": "down"})
        
        # Execute
        self.tracker.setup_southbound_portchannels_cache()
        
        # Verify cache
        expected_cache = {
            "PortChannel1031": "down",
            "PortChannel1032": "down"
        }
        assert self.tracker.southbound_portchannels_cache == expected_cache

    def test_setup_southbound_portchannels_cache_missing_portchannels(self):
        """Test setup when some portchannels are missing from LAG_TABLE"""
        # Setup: Only one portchannel exists in LAG_TABLE
        self.mock_state_db_lag_table.set_data("PortChannel1031", {"oper_status": "up"})
        # PortChannel1032 is missing
        
        # Execute
        self.tracker.setup_southbound_portchannels_cache()
        
        # Verify only existing portchannel is cached
        expected_cache = {
            "PortChannel1031": "up"
        }
        assert self.tracker.southbound_portchannels_cache == expected_cache

    def test_should_update_southbound_portchannels_cache_status_change(self):
        """Test cache update when portchannel status changes"""
        # Setup initial cache
        self.tracker.southbound_portchannels_cache = {"PortChannel1031": "up"}
        
        # Simulate status change
        fvp = {"oper_status": "down"}
        changed = self.tracker.should_update_southbound_portchannels_cache("PortChannel1031", "SET", fvp)
        
        # Verify change detected and cache updated
        assert changed == True
        assert self.tracker.southbound_portchannels_cache["PortChannel1031"] == "down"

    def test_should_update_southbound_portchannels_cache_no_change(self):
        """Test cache update when status doesn't change"""
        # Setup initial cache
        self.tracker.southbound_portchannels_cache = {"PortChannel1031": "up"}
        
        # Simulate same status
        fvp = {"oper_status": "up"}
        changed = self.tracker.should_update_southbound_portchannels_cache("PortChannel1031", "SET", fvp)
        
        # Verify no change detected
        assert changed == False
        assert self.tracker.southbound_portchannels_cache["PortChannel1031"] == "up"

    def test_get_current_loopback_state_enabled(self):
        """Test reading current loopback state when enabled"""
        # Setup: Loopback is enabled in APPL_DB
        self.mock_appl_db_intf_table.set_data("Loopback6", {"admin_status": "up"})
        
        # Execute
        self.tracker.get_current_loopback_state()
        
        # Verify state is read correctly
        assert self.tracker.loopback_status == "enabled"

    def test_get_current_loopback_state_disabled(self):
        """Test reading current loopback state when disabled"""
        # Setup: Loopback is disabled in CONFIG_DB
        self.mock_appl_db_intf_table.set_data("Loopback6", {"admin_status": "down"})
        
        # Execute
        self.tracker.get_current_loopback_state()
        
        # Verify state is read correctly
        assert self.tracker.loopback_status == "disabled"

    def test_get_current_loopback_state_not_found(self):
        """Test reading current loopback state when interface not found"""
        # Setup: No loopback interface in APPL_DB
        
        # Execute
        self.tracker.get_current_loopback_state()
        
        # Verify defaults to disabled
        assert self.tracker.loopback_status is None

    def test_process_portchannel_status_update_connectivity_up(self):
        """Test processing when connectivity is available (at least one PC up)"""
        # Setup: One portchannel up, one down, and Loopback6 exists
        self.tracker.southbound_portchannels_cache = {
            "PortChannel1031": "up",
            "PortChannel1032": "down"
        }
        self.tracker.loopback_status = "disabled"
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "down")
        self.mock_appl_db_intf_table.set_data("Loopback6", {"admin_status": "down"})
        
        # Execute
        self.tracker.process_portchannel_status_update()
        
        # Verify loopback enabled and connectivity status updated
        assert self.tracker.loopback_status == "enabled"
        assert self.tracker.last_connectivity_status == "up"

    def test_process_portchannel_status_update_connectivity_down(self):
        """Test processing when no connectivity (all PCs down)"""
        # Setup: All portchannels down, and Loopback6 exists
        self.tracker.southbound_portchannels_cache = {
            "PortChannel1031": "down",
            "PortChannel1032": "down"
        }
        self.tracker.loopback_status = "enabled"
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "up")
        
        # Execute
        self.tracker.process_portchannel_status_update()
        
        # Verify loopback disabled and connectivity status updated
        assert self.tracker.loopback_status == "disabled"
        assert self.tracker.last_connectivity_status == "down"

    def test_process_portchannel_status_update_no_portchannels(self):
        """Test processing when no portchannels are cached"""
        # Setup: Empty cache (no portchannels available), and Loopback6 exists
        self.tracker.southbound_portchannels_cache = {}
        self.tracker.loopback_status = "enabled"
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "up")
        
        # Execute
        self.tracker.process_portchannel_status_update()
        
        # Verify loopback disabled when no portchannels available
        assert self.tracker.loopback_status == "disabled"
        assert self.tracker.last_connectivity_status == "down"

    def test_loopback_not_exists_still_database_writes(self):
        """Test that database writes occur when Loopback6 doesn't exist"""
        # Setup: Loopback6 does not exist in CONFIG_DB
        self.tracker.loopback_status = "disabled"
        
        # Clear databases to ensure clean test
        self.mock_config_db_loopback_table.data.clear()
        self.mock_state_db_link_tracker_table.data.clear()
        
        # Execute: Try to update loopback with connectivity up
        self.tracker.update_loopback_interface('up')
        
        # Verify: CONFIG_DB write occur even though interface doesn't exist
        loopback_data = self.mock_config_db_loopback_table.data.get("Loopback6", {})
        assert loopback_data.get("admin_status") == "up"
        
        # Verify: STATE_DB write occurred for loopback status
        tracker_data = self.mock_state_db_link_tracker_table.data.get("WLPoToLo6", {})
        assert tracker_data.get("loopback6_status") == "enabled"
        
        # Verify: Internal tracking was updated
        assert self.tracker.loopback_status == "enabled"

    def test_loopback_not_exists_warning_only_on_status_change(self):
        """Test that warning is only logged when loopback status would change"""
        # Setup: Start with disabled state, Loopback6 doesn't exist
        self.tracker.loopback_status = "disabled"
        
        # Mock the logger to capture warnings
        with mock.patch('link_state_tracker.logger_helper') as mock_logger:
            # First call with same status (down) - should NOT log warning
            self.tracker.update_loopback_interface('down')
            mock_logger.log_warning.assert_not_called()
            
            # Call with different status (up) - should log warning
            self.tracker.update_loopback_interface('up')
            mock_logger.log_warning.assert_called_once()
            assert "Loopback6 interface not configured" in str(mock_logger.log_warning.call_args)
            
            # Reset mock and call with same status again - should NOT log warning
            mock_logger.reset_mock()
            self.tracker.update_loopback_interface('up')
            mock_logger.log_warning.assert_not_called()
            
            # Verify internal state was updated correctly
            assert self.tracker.loopback_status == "enabled"

    def test_update_loopback_interface_enable(self):
        """Test enabling loopback interface"""
        # Setup: Loopback6 exists and is currently disabled
        self.tracker.loopback_status = "disabled"
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "down")
        
        # Execute
        self.tracker.update_loopback_interface('up')
        
        # Verify CONFIG_DB update
        loopback_data = self.mock_config_db_loopback_table.data.get("Loopback6", {})
        assert loopback_data.get("admin_status") == "up"
        
        # Verify internal state
        assert self.tracker.loopback_status == "enabled"

    def test_update_loopback_interface_disable(self):
        """Test disabling loopback interface"""
        # Setup: Loopback6 exists and is currently enabled
        self.tracker.loopback_status = "enabled"
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "up")
        
        # Execute
        self.tracker.update_loopback_interface('down')
        
        # Verify CONFIG_DB update
        loopback_data = self.mock_config_db_loopback_table.data.get("Loopback6", {})
        assert loopback_data.get("admin_status") == "down"
        
        # Verify internal state 
        assert self.tracker.loopback_status == "disabled"

    def test_update_loopback_interface_no_change(self):
        """Test that loopback interface isn't updated when status unchanged"""
        # Setup: loopback already enabled and exists in CONFIG_DB
        self.tracker.loopback_status = "enabled"
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "up")
        
        # Store initial state
        initial_data = self.mock_config_db_loopback_table.data.copy()
        
        # Execute (try to enable again)
        self.tracker.update_loopback_interface('up')
        
        # Verify no CONFIG_DB update occurred (data unchanged)
        assert self.mock_config_db_loopback_table.data == initial_data
    
    def test_perform_initial_setup(self):
        """Test complete initial setup process"""
        # Setup CONFIG_DB with loopback disabled
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "down")
        
        # Setup LAG_TABLE with mixed portchannel states
        self.mock_state_db_lag_table.set_data("PortChannel1031", {"oper_status": "up"})
        self.mock_state_db_lag_table.set_data("PortChannel1032", {"oper_status": "down"})
        
        # Execute
        self.tracker.perform_initial_setup()
        
        # Verify loopback state was read
        assert self.tracker.loopback_status is not None
        
        # Verify cache was populated
        assert self.tracker.southbound_portchannels_cache["PortChannel1031"] == "up"
        assert self.tracker.southbound_portchannels_cache["PortChannel1032"] == "down"
        
        # Verify STATE_DB entries were created
        state_data = self.mock_state_db_link_tracker_table.data["WLPoToLo6"]
        assert "is_link_state_tracker_enabled" in state_data
        assert "southbound_portchannels_status" in state_data
        assert "loopback6_status" in state_data

    def test_loopback_disabled_at_init_then_portchannels_up(self):
        """Test scenario where loopback is disabled at init but portchannels come up"""
        # Setup: Loopback is disabled in CONFIG_DB initially
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "down")
        
        # Setup: Portchannels are initially down
        self.mock_state_db_lag_table.set_data("PortChannel1031", {"oper_status": "down"})
        self.mock_state_db_lag_table.set_data("PortChannel1032", {"oper_status": "down"})
        
        # Perform initial setup
        self.tracker.perform_initial_setup()
        
        # Verify initial state - loopback should remain disabled
        assert self.tracker.loopback_status == "disabled"
        
        # Now simulate portchannel coming up
        self.tracker.southbound_portchannels_cache["PortChannel1031"] = "up"
        self.tracker.process_portchannel_status_update()
        
        # Verify loopback gets enabled when connectivity is restored
        assert self.tracker.loopback_status == "enabled"
        
        # Verify CONFIG_DB was updated
        loopback_data = self.mock_config_db_loopback_table.data.get("Loopback6", {})
        assert loopback_data.get("admin_status") == "up"

    def test_stop_cleanup(self):
        """Test stop method cleans up properly"""
        # Setup
        self.mock_state_db_link_tracker_table.hset("WLPoToLo6", "test", "data")
        self.tracker.should_monitor_run = True
        
        # Execute
        self.tracker.stop()
        
        # Verify cleanup
        assert self.tracker.should_monitor_run == False
        state_data = self.mock_state_db_link_tracker_table.data["WLPoToLo6"]
        assert state_data["is_link_state_tracker_enabled"] == "no"

class TestLinkStateTrackerIntegration:
    """Integration tests for LinkStateTracker monitoring loop"""
    
    def setup_method(self):
        """Setup for integration tests"""
        self.mock_config_db_connector = DBConnector("CONFIG_DB", 0, False)
        self.mock_state_db = DBConnector("STATE_DB", 0, False)
        self.mock_appl_db = DBConnector("APPL_DB", 0, False)
        self.mock_select = Select()
        self.mock_lag_subscriber_table = SubscriberStateTable(self.mock_state_db, "LAG_TABLE")
        
        self.mock_config_db_loopback_table = Table(self.mock_config_db_connector, "LOOPBACK_INTERFACE")
        self.mock_state_db_link_tracker_table = Table(self.mock_state_db, "LINK_STATE_TRACKER_TABLE")
        self.mock_state_db_lag_table = Table(self.mock_state_db, "LAG_TABLE")
        self.mock_appl_db_intf_table = Table(self.mock_appl_db, "INTF_TABLE")
        
        self.patcher = mock.patch('link_state_tracker.swsscommon')
        self.mock_swsscommon = self.patcher.start()
        
        self.mock_swsscommon.DBConnector.side_effect = self._db_connector_side_effect
        self.mock_swsscommon.Table.side_effect = self._table_side_effect
        self.mock_swsscommon.Select.return_value = self.mock_select
        self.mock_swsscommon.SubscriberStateTable.return_value = self.mock_lag_subscriber_table
        self.mock_swsscommon.Select.TIMEOUT = 0
        self.mock_swsscommon.Select.ERROR = 1
        self.mock_swsscommon.Select.OBJECT = 2
        
        self.mock_swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME = "LOOPBACK_INTERFACE"
        self.mock_swsscommon.STATE_LINK_STATE_TRACKER_TABLE_NAME = "LINK_STATE_TRACKER_TABLE"
        self.mock_swsscommon.STATE_LAG_TABLE_NAME = "LAG_TABLE"
        
        from link_state_tracker import LinkStateTracker
        self.tracker = LinkStateTracker()
        
    def teardown_method(self):
        self.patcher.stop()
        
    def _db_connector_side_effect(self, db_name, timeout, use_unix_socket_path):
        """Side effect for DBConnector mock"""
        if db_name == "CONFIG_DB":
            return self.mock_config_db_connector
        elif db_name == "STATE_DB":
            return self.mock_state_db
        elif db_name == "APPL_DB":
            return self.mock_appl_db
        return DBConnector(db_name, timeout, use_unix_socket_path)
        
    def _table_side_effect(self, db, table_name):
        """Side effect for Table mock"""
        if table_name == self.mock_swsscommon.STATE_LINK_STATE_TRACKER_TABLE_NAME:
            return self.mock_state_db_link_tracker_table
        elif table_name == self.mock_swsscommon.STATE_LAG_TABLE_NAME:
            return self.mock_state_db_lag_table
        elif table_name == self.mock_swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME:
            return self.mock_config_db_loopback_table
        elif table_name == self.mock_swsscommon.APP_INTF_TABLE_NAME:
            return self.mock_appl_db_intf_table
        return Table(db, table_name)

    def test_monitoring_loop_portchannel_event_processing(self):
        """Test that monitoring loop processes portchannel events correctly"""
        # Setup initial state with Loopback6 existing
        self.tracker.southbound_portchannels_cache = {"PortChannel1031": "up"}
        self.tracker.loopback_status = "enabled"
        self.mock_config_db_loopback_table.hset("Loopback6", "admin_status", "up")
        
        # Add a portchannel status change event
        self.mock_lag_subscriber_table.add_update(
            "PortChannel1031", "SET", [("oper_status", "down")]
        )
        
        # Configure select to return the event
        self.mock_select.return_values.append((Select.OBJECT, self.mock_lag_subscriber_table))
        
        # Simulate one iteration of the monitoring loop
        select_status, _ = self.mock_select.select(1000)
        if select_status == Select.OBJECT:
            lag, op, fvp = self.mock_lag_subscriber_table.pop()
            if lag in self.tracker.southbound_portchannels:
                fvp_dict = dict(fvp)
                if self.tracker.should_update_southbound_portchannels_cache(lag, op, fvp_dict):
                    self.tracker.process_portchannel_status_update()
        
        # Verify the event was processed correctly
        assert self.tracker.southbound_portchannels_cache["PortChannel1031"] == "down"
        assert self.tracker.loopback_status == "disabled"

    def test_monitoring_loop_ignores_non_monitored_portchannels(self):
        """Test that monitoring loop ignores non-monitored portchannels"""
        # Setup initial state
        original_cache = self.tracker.southbound_portchannels_cache.copy()
        
        # Add event for non-monitored portchannel
        self.mock_lag_subscriber_table.add_update(
            "PortChannel9999", "SET", [("oper_status", "down")]
        )
        
        # Configure select to return the event
        self.mock_select.return_values.append((Select.OBJECT, self.mock_lag_subscriber_table))
        
        # Simulate monitoring loop iteration
        select_status, _ = self.mock_select.select(1000)
        if select_status == Select.OBJECT:
            lag, op, fvp = self.mock_lag_subscriber_table.pop()
            # This should be ignored since PortChannel9999 is not monitored
            assert lag not in self.tracker.southbound_portchannels
        
        # Verify cache unchanged
        assert self.tracker.southbound_portchannels_cache == original_cache

if __name__ == '__main__':
    pytest.main(["-v", __file__])