#!/usr/bin/env python3

import unittest
from unittest import mock
import sys
import os
import subprocess
import signal

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

sys.modules['swsscommon'] = mock.MagicMock()
sys.modules['swsscommon.swsscommon'] = mock.MagicMock()
sys.modules['sonic_py_common'] = mock.MagicMock()
sys.modules['sonic_py_common.logger'] = mock.MagicMock()

import periodic_config_saver

class MockSwssCommon:
    APP_ROUTE_TABLE_NAME = "ROUTE_TABLE"
    
    class DBConnector:
        def __init__(self, db_name, timeout, use_unix_socket_path=False):
            self.db_name = db_name
            self.timeout = timeout
            self.use_unix_socket_path = use_unix_socket_path

    class Table:
        def __init__(self, db, table_name):
            self.db = db
            self.table_name = table_name
        
        def get(self, key):
            if key == "0.0.0.0/0":
                return True, []
            return False, []

class TestConfigSave(unittest.TestCase):
    
    def setUp(self):
        periodic_config_saver.running = True
    
    @mock.patch('periodic_config_saver.subprocess.run')
    @mock.patch('periodic_config_saver.swsscommon', MockSwssCommon)
    def test_run_config_save_single_cycle_with_route_success(self, mock_run):
        """Test single cycle when route exists and config save succeeds."""
        mock_process = mock.Mock()
        mock_process.returncode = 0
        mock_process.stdout = "Config saved"
        mock_process.stderr = ""
        mock_run.return_value = mock_process
        
        periodic_config_saver.run_config_save_single_cycle()
        
        mock_run.assert_called_once_with(
            ["sudo", "config", "save", "-y"],
            capture_output=True,
            text=True,
            timeout=60
        )
    
    @mock.patch('periodic_config_saver.subprocess.run')
    @mock.patch('periodic_config_saver.swsscommon', MockSwssCommon)
    def test_run_config_save_single_cycle_with_route_failure(self, mock_run):
        """Test single cycle when route exists but config save fails."""
        mock_process = mock.Mock()
        mock_process.returncode = 1
        mock_process.stdout = ""
        mock_process.stderr = "Error saving config"
        mock_run.return_value = mock_process
        
        periodic_config_saver.run_config_save_single_cycle()
        
        mock_run.assert_called_once_with(
            ["sudo", "config", "save", "-y"],
            capture_output=True,
            text=True,
            timeout=60
        )
    
    @mock.patch('periodic_config_saver.swsscommon')
    def test_run_config_save_single_cycle_no_route(self, mock_swsscommon):
        """Test single cycle when no default route exists."""
        mock_table = mock.Mock()
        mock_table.get.return_value = (False, [])  # No route exists
        mock_db = mock.Mock()
        
        mock_swsscommon.DBConnector.return_value = mock_db
        mock_swsscommon.Table.return_value = mock_table
        mock_swsscommon.APP_ROUTE_TABLE_NAME = "ROUTE_TABLE"
        
        with mock.patch('periodic_config_saver.subprocess.run') as mock_run:
            periodic_config_saver.run_config_save_single_cycle()
            
            # Should not call subprocess.run since no route exists
            mock_run.assert_not_called()
            mock_table.get.assert_called_once_with("0.0.0.0/0")
    
    @mock.patch('periodic_config_saver.subprocess.run')
    @mock.patch('periodic_config_saver.swsscommon', MockSwssCommon)
    def test_run_config_save_single_cycle_timeout(self, mock_run):
        """Test single cycle when config save times out."""
        mock_run.side_effect = subprocess.TimeoutExpired(
            cmd=["sudo", "config", "save", "-y"], 
            timeout=60
        )
        
        # Should not raise exception, just log error
        periodic_config_saver.run_config_save_single_cycle()
        
        mock_run.assert_called_once_with(
            ["sudo", "config", "save", "-y"],
            capture_output=True,
            text=True,
            timeout=60
        )
    
    @mock.patch('periodic_config_saver.swsscommon')
    def test_run_config_save_single_cycle_db_exception(self, mock_swsscommon):
        """Test single cycle when database connection fails."""
        mock_swsscommon.DBConnector.side_effect = Exception("Database connection failed")
        
        # Should not raise exception, just log error
        periodic_config_saver.run_config_save_single_cycle()
        
        mock_swsscommon.DBConnector.assert_called_once_with("APPL_DB", 0, True)
    
    def test_signal_handler(self):
        """Test signal handler sets running to False and exits."""
        periodic_config_saver.running = True
        
        with mock.patch('periodic_config_saver.sys.exit') as mock_exit:
            periodic_config_saver.signal_handler(signal.SIGTERM, None)
            
            self.assertFalse(periodic_config_saver.running)
            mock_exit.assert_called_once_with(0)
    
    @mock.patch('periodic_config_saver.time.sleep')
    @mock.patch('periodic_config_saver.run_config_save_single_cycle')
    @mock.patch('periodic_config_saver.signal.signal')
    def test_main_single_iteration(self, mock_signal, mock_cycle, mock_sleep):
        """Test main function runs one iteration and sleeps."""
        # Make the loop run only once
        def stop_after_first_sleep(*args):
            periodic_config_saver.running = False
        
        mock_sleep.side_effect = stop_after_first_sleep
        
        result = periodic_config_saver.main()
        
        # Should set up signal handlers
        self.assertEqual(mock_signal.call_count, 2)
        
        # Should run one cycle
        mock_cycle.assert_called_once()
        
        # Should sleep for 1 hour
        mock_sleep.assert_called_once_with(3600)
        
        # Should not return anything
        self.assertIsNone(result)
    
    @mock.patch('periodic_config_saver.time.sleep')
    @mock.patch('periodic_config_saver.run_config_save_single_cycle')
    @mock.patch('periodic_config_saver.signal.signal')
    def test_main_multiple_iterations(self, mock_signal, mock_cycle, mock_sleep):
        """Test main function runs multiple iterations."""
        call_count = 0
        
        def stop_after_three_sleeps(*args):
            nonlocal call_count
            call_count += 1
            if call_count >= 3:
                periodic_config_saver.running = False
        
        mock_sleep.side_effect = stop_after_three_sleeps
        
        periodic_config_saver.main()
        
        # Should run three cycles
        self.assertEqual(mock_cycle.call_count, 3)
        
        # Should sleep three times
        self.assertEqual(mock_sleep.call_count, 3)
    
    @mock.patch('periodic_config_saver.run_config_save_single_cycle')
    @mock.patch('periodic_config_saver.signal.signal')
    def test_main_stops_when_running_false(self, mock_signal, mock_cycle):
        """Test main function stops when running is set to False."""
        periodic_config_saver.running = False
        
        result = periodic_config_saver.main()
        
        # Should not run any cycles
        mock_cycle.assert_not_called()
        
        # Should not return anything
        self.assertIsNone(result)

if __name__ == '__main__':
    unittest.main()