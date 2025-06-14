#!/usr/bin/env python3

import pytest
import unittest.mock as mock
import sys
import os

# Add the parent directory to the path to import the link_state_tracker.py
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

sys.modules['sonic_py_common'] = mock.MagicMock()
sys.modules['sonic_py_common.logger'] = mock.MagicMock()
sys.modules['swsscommon'] = mock.MagicMock()
import link_state_tracker
from link_state_tracker import *

# Mock constants for testing
TEST_LOOPBACK_INTERFACE = "loopback6"
TEST_FEATURE_NAME = "AzSLinkStateTracker"
TEST_RESOURCE_TYPE = "AZS_SMARTSWITCH_T0"

class TestDBHandler:
    """
    Tests for the DBHandler class
    """

    def setup_method(self):
        """
        Setup for each test method
        """
        # Create mocks for database connectors and tables
        self.mock_config_table = MockConfigTable("mock_db", "config_table")
        self.mock_state_table = MockStateTable("mock_db", "state_table")
        self.mock_lag_table = MockLagTable(mock.MagicMock(), "lag_table")

        # Configure mock return value
        link_state_tracker.swsscommon.DBConnector = mock.MagicMock(return_value=mock.MagicMock())
        link_state_tracker.swsscommon.Table = mock.MagicMock(return_value=self.mock_config_table)

        # Create DBHandler instance with mocked dependency
        self.db_handler = DBHandler()
        self.db_handler.config_db = mock.MagicMock()
        self.db_handler.state_db = mock.MagicMock()
        
        # Add mock for link_state_tracker_table
        self.db_handler.link_state_tracker_table = mock.MagicMock()

        # Create PortChannelHandler instance with mocked dependency
        self.port_channel_handler = PortChannelHandler()

        # Configure southbound portchannels
        self.port_channel_handler.southbound_portchannels = ["Po1031", "Po1032"]
    
    def test_is_feature_enabled_true(self):
        """Test feature_enabled when feature is enabled"""
        # Setup feature as enabled
        self.db_handler.get_config_value = mock.MagicMock(return_value="enabled")
        
        result = self.db_handler.is_feature_enabled(TEST_FEATURE_NAME)
        self.db_handler.get_config_value.assert_called_with(CONFIG_DB_FEATURE_TABLE, TEST_FEATURE_NAME, "state", "disabled")
        assert result == True
    
    def test_is_feature_enabled_false(self):
        """Test is_feature_enabled when feature is disabled"""
        # Setup feature as disabled
        self.mock_config_table.set(TEST_FEATURE_NAME, [("state", "disabled")])
        
        result = self.db_handler.is_feature_enabled(TEST_FEATURE_NAME)
        assert result == False
    
    def test_is_azs_smart_switch_t0_true(self):
        """Test is_azs_smart_switch_t0 when device is an AzS smart switch T0"""
        # Setup device metadata
        self.db_handler.get_config_value = mock.MagicMock(return_value=AZS_SMARTSWITCH_RESOURCE_TYPE)
        
        result = self.db_handler.is_azs_smart_switch_t0()
        assert result == True
    
    def test_is_azs_smart_switch_t0_false(self):
        """Test is_azs_smart_switch_t0 when device is not an AzS smart switch T0"""
        # Setup device metadata
        self.db_handler.get_config_value = mock.MagicMock(return_value="OTHER_DEVICE")
        
        result = self.db_handler.is_azs_smart_switch_t0()
        assert result == False

class TestPortChannelHandler:
    """
    Tests for the PortChannelHandler class
    """
    def setup_method(self):
        """
        Setup for each test method
        """
        # Create mock for lag table
        self.mock_lag_table = MockLagTable(mock.MagicMock(), "lag_table")

        link_state_tracker.swsscommon.DBConnector = mock.MagicMock(return_value=mock.MagicMock())
        link_state_tracker.swsscommon.Table = mock.MagicMock(return_value=self.mock_lag_table)

        # Create PortChannelHandler instance with mocked dependency
        self.portchannel_handler = PortChannelHandler()

        # Configure southbound portchannels
        self.portchannel_handler.southbound_portchannels = ["Po1031", "Po1032"]

    def test_get_up_portchannels_all_up(self):
        """
        Test get_up_portchannels when all portchannels are up
        """
        # Setup: All portchannels are up
        self.mock_lag_table.add("Po1031", "up")
        self.mock_lag_table.add("Po1032", "up")

        result = self.portchannel_handler.get_up_portchannels(["Po1031", "Po1032"])
        assert sorted(result) == ["Po1031", "Po1032"]
    
    def test_get_up_portchannels_some_up(self):
        """
        Test get_up_portchannels when some portchannels are up
        """
        # Setup one portchannel as up, one as down
        self.mock_lag_table.add_portchannel("Po1031", "up")
        self.mock_lag_table.add_portchannel("Po1032", "down")
        
        result = self.portchannel_handler.get_up_portchannels(["Po1031", "Po1032"])
        assert result == ["Po1031"]
    
    def test_get_up_portchannels_all_down(self):
        """
        Test get_up_portchannels when all portchannels are down
        """
        # Setup all portchannels as down
        self.mock_lag_table.add_portchannel("Po1031", "down")
        self.mock_lag_table.add_portchannel("Po1032", "down")
        
        result = self.portchannel_handler.get_up_portchannels(["Po1031", "Po1032"])
        assert result == []

class TestLinkStateTracker:
    """
    Tests for the LinkStateTracker class
    """

    def setup_method(self):
        """
        Setup for each test method
        """
        # Mock dependencies
        link_state_tracker.subprocess = mock.MagicMock()
        link_state_tracker.threading = mock.MagicMock()

        # Create mock objects for select and subscriber
        self.mock_select = MockSelect()
        self.mock_lag_table = MockLagTable(mock.MagicMock(), "lag_table")
        self.mock_subscriber = MockLagTable(mock.MagicMock(), "subscriber_table")

        # Configure mock return value
        link_state_tracker.swsscommon.DBConnector = mock.MagicMock(return_value=mock.MagicMock())
        link_state_tracker.swsscommon.SubscriberStateTable = mock.MagicMock(return_value=self.mock_lag_table)
        link_state_tracker.swsscommon.Select = mock.MagicMock(return_value=self.mock_select)
        link_state_tracker.swsscommon.Select.OBJECT = MockSelect.OBJECT
        link_state_tracker.swsscommon.Select.TIMEOUT = MockSelect.TIMEOUT
        link_state_tracker.swsscommon.Select.ERROR = MockSelect.ERROR

        # Create LinkStateTracker instance with mocked dependency
        self.link_state_tracker = LinkStateTracker()

        # Mock DBHandler and PortChannelHandler
        self.link_state_tracker.db_handler = mock.MagicMock()
        self.link_state_tracker.portchannel_handler = mock.MagicMock()

        # Mock southbound portchannels
        self.link_state_tracker.portchannel_handler.southbound_portchannels = ["Po1031", "Po1032"]

    def test_set_loopback_interface_status(self):
        """Test the unified loopback interface state setting function"""
        link_state_tracker.subprocess.run.reset_mock()
        
        # Test enabling
        self.link_state_tracker.set_loopback_interface_status(enabled=True)
        link_state_tracker.subprocess.run.assert_called_with(
            ["sudo", "config", "interface", "startup", LOOPBACK_INTERFACE],
            check=True, text=True, capture_output=True
        )
        assert self.link_state_tracker.loopback_enabled == True
        self.link_state_tracker.db_handler.update_status.assert_called_with(
            LINKSTATETRACKER_LOOPBACK_STATUS_KEY, "enabled"
        )
        
        # Reset mocks
        link_state_tracker.subprocess.run.reset_mock()
        self.link_state_tracker.db_handler.update_status.reset_mock()
        
        # Test disabling
        self.link_state_tracker.set_loopback_interface_status(enabled=False)
        link_state_tracker.subprocess.run.assert_called_with(
            ["sudo", "config", "interface", "shutdown", LOOPBACK_INTERFACE],
            check=True, text=True, capture_output=True
        )
        assert self.link_state_tracker.loopback_enabled == False
        self.link_state_tracker.db_handler.update_status.assert_called_with(
            LINKSTATETRACKER_LOOPBACK_STATUS_KEY, "disabled"
        )
    
    def test_check_portchannel_states_all_up(self):
        """Test checking portchannel states when all are up"""
        # Configure portchannel handler to return all up
        self.link_state_tracker.portchannel_handler.get_up_portchannels.return_value = ["Po1031", "Po1032"]
        
        # Initial state: loopback is disabled
        self.link_state_tracker.loopback_enabled = False
        
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface()
        
        # Verify state was updated and loopback was enabled
        self.link_state_tracker.db_handler.update_status.assert_has_calls([
            mock.call(LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, "up"),
            mock.call(LINKSTATETRACKER_LOOPBACK_STATUS_KEY, "enabled")
        ], any_order=True)
    
    def test_check_portchannel_states_some_up(self):
        """Test checking portchannel states when some are up"""
        # Configure portchannel handler to return some up
        self.link_state_tracker.portchannel_handler.get_up_portchannels.return_value = ["Po1031"]
        
        # Initial state: loopback is disabled
        self.link_state_tracker.loopback_enabled = False
        
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface()
        
        self.link_state_tracker.db_handler.update_status.assert_has_calls([
            mock.call(LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, "up"),
            mock.call(LINKSTATETRACKER_LOOPBACK_STATUS_KEY, "enabled")
        ], any_order=True)
    
    def test_check_portchannel_states_all_down(self):
        """Test checking portchannel states when all are down"""
        # Configure portchannel handler to return none up
        self.link_state_tracker.portchannel_handler.get_up_portchannels.return_value = []
        
        # Initial state: loopback is enabled
        self.link_state_tracker.loopback_enabled = True
        
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface()
        
        self.link_state_tracker.db_handler.update_status.assert_has_calls([
            mock.call(LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, "down"),
            mock.call(LINKSTATETRACKER_LOOPBACK_STATUS_KEY, "disabled")
        ], any_order=True)
    
    def test_run_monitor_portchannel_status(self):
        """Test monitoring portchannel status with mock events"""
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface = mock.MagicMock()

        # Setup mock monitor method to run only once
        original_run = self.link_state_tracker.run_monitor_portchannel_status
        run_count = [0]
        def mock_run():
            if run_count[0] < 1:
                run_count[0] += 1
                # Call original but break after first iteration
                self.link_state_tracker.running = True
                self.mock_select.add_return_value(MockSelect.OBJECT)
                self.mock_lag_table.add_event("Po1031", "SET", {"oper_status": "down"})
                # Run one iteration - will exit after processing event
                self.link_state_tracker.running = False
        
        self.link_state_tracker.run_monitor_portchannel_status = mock_run
        
        # Start and verify
        self.link_state_tracker.start()
        
        # Verify checker method was called
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface.assert_called()

class TestDBHandlerStateTable:
    """
    Test class for link_state_tracker_table in DBHandler
    """
    def setup_method(self):
        """
        Setup for test method
        """
        # self.mock_link_state_tracker_table = MockStateTable("mock_db", STATE_DB_LINK_STATE_TRACKER_TABLE)
        # self.mock_link_state_tracker_table.set = mock.MagicMock()
        self.mock_link_state_tracker_table = mock.MagicMock()

        link_state_tracker.swsscommon.DBConnector = mock.MagicMock(return_value=mock.MagicMock())
        link_state_tracker.swsscommon.Table = mock.MagicMock(return_value=self.mock_link_state_tracker_table)

        self.db_handler = DBHandler()
        self.db_handler.link_state_tracker_table = self.mock_link_state_tracker_table
    
    def test_update_status_propagates_to_table(self):
        """Test update_status function can properly propagates to the link_state_tracker_table"""
        self.db_handler.update_status(LINKSTATETRACKER_ENABLED_KEY, "yes")
        self.mock_link_state_tracker_table.set.assert_called_once_with(LINKSTATETRACKER_ENABLED_KEY, "yes")
    
    def test_update_status_multiple_values(self):
        """Test update_status function can handle multiple updates"""
        test_data = [
            (LINKSTATETRACKER_ENABLED_KEY, "yes"),
            (LINKSTATETRACKER_LOOPBACK_STATUS_KEY, "up"),
            (LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, "enabled")
        ]

        for key, value in test_data:
            self.db_handler.update_status(key, value)
        
        # Verify each key-value pair
        calls = [mock.call(key, value) for key, value in test_data]
        self.mock_link_state_tracker_table.set.assert_has_calls(calls)
        assert self.mock_link_state_tracker_table.set.call_count == len(test_data)
    
    def test_update_status_exception_handle(self):
        """Test that update_status can properly handle exceptions"""
        key = LINKSTATETRACKER_LOOPBACK_STATUS_KEY
        invalid_value = "yes"
        self.mock_link_state_tracker_table.set.side_effect = Exception("Test exception")

        orginal_log_error = link_state_tracker.logger_helper.log_error
        link_state_tracker.logger_helper.log_error = mock.MagicMock()

        try:
            self.db_handler.update_status(key, invalid_value)
            link_state_tracker.logger_helper.log_error.assert_called_once()

            error_message = link_state_tracker.logger_helper.log_error.call_args[0][0]
            assert "Error updating" in error_message
            assert STATE_DB_LINK_STATE_TRACKER_TABLE in error_message
            assert key in error_message
        finally:
            link_state_tracker.logger_helper.log_error = orginal_log_error
    
class TestEndToEndStateTableUpdates:
    """
    End-to-End tests for updates of link_state_tracker table in STATE_DB
    """
    def setup_method(self):
        """Set up the test methods"""
        # self.mock_link_state_tracker_table = MockStateTable("mock_db", STATE_DB_LINK_STATE_TRACKER_TABLE)
        # self.mock_lag_table = MockLagTable(mock.MagicMock(), "lag_table")
        # self.mock_select = MockSelect()
        self.mock_link_state_tracker_table = mock.MagicMock()
        self.mock_lag_table = mock.MagicMock()
        self.mock_select = mock.MagicMock()

        link_state_tracker.subprocess = mock.MagicMock()
        link_state_tracker.threading = mock.MagicMock()

        link_state_tracker.swsscommon.DBConnector = mock.MagicMock(return_value=mock.MagicMock())
        link_state_tracker.swsscommon.Table = mock.MagicMock(return_valie=self.mock_link_state_tracker_table)
        link_state_tracker.swsscommon.SubscriberStateTable = mock.MagicMock(return_value=self.mock_lag_table)
        link_state_tracker.swsscommon.Select = mock.MagicMock(return_value=self.mock_select)
        
        link_state_tracker.swsscommon.Select.OBJECT = MockSelect.OBJECT
        link_state_tracker.swsscommon.Select.TIMEOUT = MockSelect.TIMEOUT
        link_state_tracker.swsscommon.Select.ERROR = MockSelect.ERROR

        self.link_state_tracker = LinkStateTracker()
        self.link_state_tracker.db_handler = mock.MagicMock()
        
        # self.link_state_tracker.db_handler.link_state_tracker_table = self.mock_link_state_tracker_table
        self.original_update_status = self.link_state_tracker.db_handler.update_status
        # self.link_state_tracker.db_handler.update_status = mock.MagicMock()

        self.link_state_tracker.portchannel_handler = mock.MagicMock()
        self.link_state_tracker.portchannel_handler.southbound_portchannels = ["Po1031", "Po1032"]
        self.link_state_tracker.portchannel_handler.get_southbound_portchannels.return_value =  ["Po1031", "Po1032"]

    def test_state_updates_during_startup(self):
        """Test link_state_tracker table can be updated during the startup"""
        # Mock only one up portchannel for initial status check
        self.link_state_tracker.portchannel_handler.get_up_portchannels.return_value = ["Po1031"]
        self.link_state_tracker.monitor_thread = mock.MagicMock()
        
        # Start the service and verify
        self.link_state_tracker.start()
        self.link_state_tracker.db_handler.update_status.assert_any_call(
            LINKSTATETRACKER_ENABLED_KEY, "yes")
        self.link_state_tracker.db_handler.update_status.assert_any_call(
            LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, "up")

    def test_state_updates_with_portchannel_state_changes(self):
        """Test that state table is updated when portchannel states change"""
        # Reset mock to clear any previous calls
        self.link_state_tracker.db_handler.update_status.reset_mock()

        # All portchannels are up and loopback enabled
        self.link_state_tracker.portchannel_handler.get_up_portchannels.return_value = ["Po1031", "Po1032"]
        self.link_state_tracker.loopback_enabled = True
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface()

        self.link_state_tracker.db_handler.update_status.assert_called_with(
            LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, "up")
        self.link_state_tracker.db_handler.reset_mock()

        # All portchannels are down
        self.link_state_tracker.portchannel_handler.get_up_portchannels.return_value = []
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface()

        self.link_state_tracker.db_handler.update_status.assert_any_call(
            LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, "down")
        self.link_state_tracker.db_handler.update_status.assert_any_call(
            LINKSTATETRACKER_LOOPBACK_STATUS_KEY, "disabled")
        self.link_state_tracker.db_handler.reset_mock()

        # Some portchannels are up again, loopback should be enabled from disabled
        self.link_state_tracker.portchannel_handler.get_up_portchannels.return_value = ["Po1031"]
        self.link_state_tracker.loopback_enabled = False
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface()

        self.link_state_tracker.db_handler.update_status.assert_any_call(
            LINKSTATETRACKER_SOUTHBOUND_PORTCHANNELS_STATE_KEY, "up")
        self.link_state_tracker.db_handler.update_status.assert_any_call(
            LINKSTATETRACKER_LOOPBACK_STATUS_KEY, "enabled")

    def test_direct_state_table_updates(self):
        """Test that updates actually made to the state table itself"""
        # Create a new DBHandler instance with a mock table
        db_handler = DBHandler()
        mock_table = mock.MagicMock()
        db_handler.link_state_tracker_table = mock_table
        
        # Call the method on this instance
        db_handler.update_status(LINKSTATETRACKER_ENABLED_KEY, "yes")
        
        # Verify the mock table's set method was called
        mock_table.set.assert_called_once_with(LINKSTATETRACKER_ENABLED_KEY, "yes")

    def test_monitor_thread_updates_state_on_portchannel_changes(self):
        """Test that the monitor thread updates state table when portchannel states changes"""
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface = mock.MagicMock()

        # A mock for the monitor method to run just one iteration
        def mock_run_monitor():
            self.link_state_tracker.running = True

            self.mock_select.add_return_value(MockSelect.OBJECT, self.mock_lag_table)
            self.mock_lag_table.add_event("Po1031", "SET", {"oper_status": "down"})
            select_status, _ = self.mock_select(1000) # return OBJECT

            if select_status == MockSelect.OBJECT:
                lag, oper, fvs = self.mock_lag_table.pop()
                if lag in self.link_state_tracker.portchannel_handler.southbound_portchannels:
                    self.link_state_tracker.check_portchannel_states_and_update_loopback_interface()

            self.link_state_tracker.running = False

        self.link_state_tracker.run_monitor_portchannel_status = mock_run_monitor
        self.link_state_tracker.start()
        self.link_state_tracker.check_portchannel_states_and_update_loopback_interface.assert_called_once()

    def test_exception_handling_in_update_status(self):
        """Test exceptions can be properly handled during updating the state table"""
        # Create a new DBHandler instance
        db_handler = DBHandler()
        
        # Mock the logger
        mock_log_error = mock.MagicMock()
        original_log_error = link_state_tracker.logger_helper.log_error
        link_state_tracker.logger_helper.log_error = mock_log_error
        
        try:
            # Create a mock table that raises an exception
            mock_table = mock.MagicMock()
            mock_table.set.side_effect = Exception("Test exception")
            db_handler.link_state_tracker_table = mock_table
            
            # Call the method - this should catch the exception internally
            db_handler.update_status(LINKSTATETRACKER_ENABLED_KEY, "yes")
            
            # Verify logger was called
            mock_log_error.assert_called_once()
            
            # Check the error message
            error_message = mock_log_error.call_args[0][0]
            assert "Error updating" in error_message
            assert STATE_DB_LINK_STATE_TRACKER_TABLE in error_message
            assert LINKSTATETRACKER_ENABLED_KEY in error_message
        finally:
            # Restore original logger
            link_state_tracker.logger_helper.log_error = original_log_error

def test_main_function():
    """Test the main function"""
    # Mock dependencies
    link_state_tracker.DBHandler = mock.MagicMock()
    link_state_tracker.LinkStateTracker = mock.MagicMock()
    link_state_tracker.signal = mock.MagicMock()
    link_state_tracker.sys.exit = mock.MagicMock()
    
    # Configure mock return values
    db_handler_mock = mock.MagicMock()
    link_state_tracker.DBHandler.return_value = db_handler_mock
    
    # Test case: feature enabled and correct device type
    db_handler_mock.is_feature_enabled.return_value = True
    db_handler_mock.is_azs_smart_switch_t0.return_value = True
    
    link_state_tracker.main()
    
    # Verify LinkStateTracker was created and started
    link_state_tracker.LinkStateTracker.assert_called_once()
    link_state_tracker.LinkStateTracker.return_value.start.assert_called_once()
    
    # Test case: feature disabled
    link_state_tracker.LinkStateTracker.reset_mock()
    db_handler_mock.is_feature_enabled.return_value = False
    
    link_state_tracker.main()
    
    # Verify LinkStateTracker was not created
    link_state_tracker.LinkStateTracker.assert_not_called()
    
    # Test case: wrong device type
    db_handler_mock.is_feature_enabled.return_value = True
    db_handler_mock.is_azs_smart_switch_t0.return_value = False
    
    link_state_tracker.main()
    
    # Verify LinkStateTracker was not created
    link_state_tracker.LinkStateTracker.assert_not_called()


if __name__ == "__main__":
    pytest.main(["-xvs", __file__])
