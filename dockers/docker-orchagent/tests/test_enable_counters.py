#!/usr/bin/env python3
"""
Unit tests for enable_counters.py
Tests RIF counter enablement functionality
"""

import sys
import os
from unittest import mock
from swsscommon import swsscommon

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import enable_counters


class TestEnableCounters:
    """Test cases for enable_counters module"""
    
    def setup_method(self):
        """Setup test fixtures"""
        self.mock_db = mock.MagicMock()
        self.mock_config_db = mock.MagicMock()
        
    @mock.patch('enable_counters.swsscommon.ConfigDBConnector')
    @mock.patch('enable_counters.device_info.get_platform_info')
    def test_enable_rif_counters(self, mock_platform_info, mock_config_db):
        """Test that RIF and RIF_RATES counters are enabled"""
        # Setup mocks
        mock_config_db_instance = mock.MagicMock()
        mock_config_db.return_value = mock_config_db_instance
        mock_config_db_instance.get_entry.return_value = {}  # No existing entry
        mock_platform_info.return_value = {'switch_type': 'switch'}
        
        # Call enable_counters
        enable_counters.enable_counters()
        
        # Verify RIF counter was enabled
        calls = mock_config_db_instance.mod_entry.call_args_list
        counter_names = [call[0][1] for call in calls]
        
        assert 'RIF' in counter_names, "RIF counter should be enabled"
        assert 'RIF_RATES' in counter_names, "RIF_RATES counter should be enabled"
        
    @mock.patch('enable_counters.swsscommon.ConfigDBConnector')
    @mock.patch('enable_counters.device_info.get_platform_info')
    def test_enable_rif_counters_existing_entry(self, mock_platform_info, mock_config_db):
        """Test that RIF counters are enabled when entry already exists"""
        # Setup mocks
        mock_config_db_instance = mock.MagicMock()
        mock_config_db.return_value = mock_config_db_instance
        mock_config_db_instance.get_entry.return_value = {'FLEX_COUNTER_STATUS': 'enable'}
        mock_platform_info.return_value = {'switch_type': 'switch'}
        
        # Call enable_counters
        enable_counters.enable_counters()
        
        # Verify mod_entry was called (even if entry exists, it should still be called)
        mock_config_db_instance.mod_entry.assert_called()
        
    @mock.patch('enable_counters.swsscommon.ConfigDBConnector')
    @mock.patch('enable_counters.device_info.get_platform_info')
    def test_enable_rif_counters_dpu_mode(self, mock_platform_info, mock_config_db):
        """Test that RIF counters are enabled in DPU mode"""
        # Setup mocks
        mock_config_db_instance = mock.MagicMock()
        mock_config_db.return_value = mock_config_db_instance
        mock_config_db_instance.get_entry.return_value = {}
        mock_platform_info.return_value = {'switch_type': 'dpu'}
        
        # Call enable_counters
        enable_counters.enable_counters()
        
        # Verify mod_entry was called for RIF counters
        calls = mock_config_db_instance.mod_entry.call_args_list
        counter_names = [call[0][1] for call in calls]
        
        assert 'RIF' in counter_names, "RIF counter should be enabled in DPU mode"
        assert 'RIF_RATES' in counter_names, "RIF_RATES counter should be enabled in DPU mode"
        
    @mock.patch('enable_counters.swsscommon.ConfigDBConnector')
    @mock.patch('enable_counters.device_info.get_platform_info')
    def test_enable_counter_group_new_entry(self, mock_platform_info, mock_config_db):
        """Test enable_counter_group creates new entry"""
        # Setup mocks
        mock_config_db_instance = mock.MagicMock()
        mock_config_db.return_value = mock_config_db_instance
        mock_config_db_instance.get_entry.return_value = {}  # No existing entry
        
        # Call enable_counter_group for RIF
        enable_counters.enable_counter_group(mock_config_db_instance, 'RIF')
        
        # Verify mod_entry was called with correct parameters
        mock_config_db_instance.mod_entry.assert_called_once_with(
            'FLEX_COUNTER_TABLE',
            'RIF',
            {'FLEX_COUNTER_STATUS': 'enable'}
        )
        
    @mock.patch('enable_counters.swsscommon.ConfigDBConnector')
    @mock.patch('enable_counters.device_info.get_platform_info')
    def test_enable_counter_group_existing_entry(self, mock_platform_info, mock_config_db):
        """Test enable_counter_group does not overwrite existing entry"""
        # Setup mocks
        mock_config_db_instance = mock.MagicMock()
        mock_config_db.return_value = mock_config_db_instance
        mock_config_db_instance.get_entry.return_value = {'FLEX_COUNTER_STATUS': 'enable'}
        
        # Call enable_counter_group for RIF
        enable_counters.enable_counter_group(mock_config_db_instance, 'RIF')
        
        # Verify mod_entry was NOT called (entry already exists)
        mock_config_db_instance.mod_entry.assert_not_called()
        
    @mock.patch('enable_counters.swsscommon.SonicV2Connector')
    def test_enable_rates(self, mock_sonic_v2):
        """Test that RIF rates are enabled"""
        # Setup mocks
        mock_db_instance = mock.MagicMock()
        mock_sonic_v2.return_value = mock_db_instance
        
        # Call enable_rates
        enable_counters.enable_rates()
        
        # Verify RIF rate settings were set
        calls = mock_db_instance.set.call_args_list
        keys = [call[0][1] + ':' + call[0][2] for call in calls]
        
        assert 'RATES:RIF' in keys, "RIF rates should be configured"
        
    @mock.patch('enable_counters.swsscommon.ConfigDBConnector')
    @mock.patch('enable_counters.device_info.get_platform_info')
    def test_all_counter_groups_enabled(self, mock_platform_info, mock_config_db):
        """Test that all expected counter groups are enabled"""
        # Setup mocks
        mock_config_db_instance = mock.MagicMock()
        mock_config_db.return_value = mock_config_db_instance
        mock_config_db_instance.get_entry.return_value = {}
        mock_platform_info.return_value = {'switch_type': 'switch'}
        
        # Call enable_counters
        enable_counters.enable_counters()
        
        # Verify all expected counter groups
        calls = mock_config_db_instance.mod_entry.call_args_list
        counter_names = [call[0][1] for call in calls]
        
        expected_counters = ['RIF', 'RIF_RATES']
        for counter in expected_counters:
            assert counter in counter_names, f"{counter} should be enabled"


if __name__ == '__main__':
    import pytest
    pytest.main([__file__, '-v'])