import unittest
from unittest.mock import patch, MagicMock, Mock
import sys
import subprocess
import os

# Mock dependencies that may not be available in test environment
sys.modules['docker'] = MagicMock()
sys.modules['swsscommon'] = MagicMock()
sys.modules['swsscommon.swsscommon'] = MagicMock()

# Add parent directory to path to import memory_checker
test_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(test_dir)
sys.path.insert(0, parent_dir)

# Import memory_checker using importlib
import importlib.util
memory_checker_path = os.path.join(parent_dir, 'memory_checker')
spec = importlib.util.spec_from_loader(
    'memory_checker',
    importlib.machinery.SourceFileLoader('memory_checker', memory_checker_path)
)
memory_checker = importlib.util.module_from_spec(spec)
sys.modules['memory_checker'] = memory_checker
spec.loader.exec_module(memory_checker)


class TestMemoryChecker(unittest.TestCase):

    @patch('subprocess.Popen')
    def test_get_command_result(self, mock_popen):
        command = 'your command'
        stdout = 'Command output'
        returncode = 0
        mock_popen.return_value.communicate.return_value = (stdout, None)
        mock_popen.return_value.returncode = returncode

        result = memory_checker.get_command_result(command)

        self.assertEqual(result, stdout.strip())
        mock_popen.assert_called_once_with(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                           universal_newlines=True)
        mock_popen.return_value.communicate.assert_called_once()
        mock_popen.return_value.communicate.assert_called_with()
        self.assertEqual(mock_popen.return_value.returncode, returncode)

    @patch('memory_checker.get_command_result')
    def test_get_container_id(self, mock_get_command_result):
        container_name = 'your_container'
        command = ['docker', 'ps', '--no-trunc', '--filter', 'name=your_container']
        mock_get_command_result.return_value = ''

        with self.assertRaises(SystemExit) as cm:
            memory_checker.get_container_id(container_name)
        self.assertEqual(cm.exception.code, 1)
        mock_get_command_result.assert_called_once_with(command)

    @patch('memory_checker.open', side_effect=FileNotFoundError)
    def test_get_memory_usage(self, mock_open):
        container_id = 'your_container_id'
        with self.assertRaises(SystemExit) as cm:
            memory_checker.get_memory_usage(container_id)
        self.assertEqual(cm.exception.code, 1)

    @patch('memory_checker.open', side_effect=FileNotFoundError)
    def test_get_memory_usage_invalid(self, mock_open):
        container_id = '../..'
        with self.assertRaises(SystemExit) as cm:
            memory_checker.get_memory_usage(container_id)
        self.assertEqual(cm.exception.code, 1)

    @patch('builtins.open', side_effect=FileNotFoundError)
    def test_get_inactive_cache_usage(self, mock_open):
        container_id = 'your_container_id'
        with self.assertRaises(SystemExit) as cm:
            memory_checker.get_inactive_cache_usage(container_id)
        self.assertEqual(cm.exception.code, 1)

    @patch('syslog.syslog')
    @patch('memory_checker.get_container_id')
    @patch('memory_checker.get_memory_usage')
    @patch('memory_checker.get_inactive_cache_usage')
    def test_check_memory_usage(self, mock_get_inactive_cache_usage, mock_get_memory_usage, mock_get_container_id, mock_syslog):
        container_name = 'your_container'
        threshold_value = 1024
        container_id = 'your_container_id'
        memory_usage = 2048
        cache_usage = 512
        mock_get_container_id.return_value = container_id
        mock_get_memory_usage.return_value = str(memory_usage)
        mock_get_inactive_cache_usage.return_value = str(cache_usage)

        with self.assertRaises(SystemExit) as cm:
            memory_checker.check_memory_usage(container_name, threshold_value)

        self.assertEqual(cm.exception.code, 3)
        mock_get_memory_usage.assert_called_once_with(container_id)

    @patch('memory_checker.ConfigDBConnector')
    def test_is_feature_enabled_when_enabled(self, mock_config_db_connector):
        """Test is_feature_enabled returns True when feature state is 'enabled'"""
        mock_db = MagicMock()
        mock_config_db_connector.return_value = mock_db
        mock_db.get_table.return_value = {
            'telemetry': {'state': 'enabled'}
        }

        result = memory_checker.is_feature_enabled('telemetry')

        self.assertTrue(result)
        mock_db.connect.assert_called_once()
        mock_db.get_table.assert_called_once_with('FEATURE')

    @patch('memory_checker.ConfigDBConnector')
    def test_is_feature_enabled_when_always_enabled(self, mock_config_db_connector):
        """Test is_feature_enabled returns True when feature state is 'always_enabled'"""
        mock_db = MagicMock()
        mock_config_db_connector.return_value = mock_db
        mock_db.get_table.return_value = {
            'database': {'state': 'always_enabled'}
        }

        result = memory_checker.is_feature_enabled('database')

        self.assertTrue(result)

    @patch('memory_checker.ConfigDBConnector')
    def test_is_feature_enabled_when_disabled(self, mock_config_db_connector):
        """Test is_feature_enabled returns False when feature state is 'disabled'"""
        mock_db = MagicMock()
        mock_config_db_connector.return_value = mock_db
        mock_db.get_table.return_value = {
            'snmp': {'state': 'disabled'}
        }

        result = memory_checker.is_feature_enabled('snmp')

        self.assertFalse(result)

    @patch('memory_checker.ConfigDBConnector')
    def test_is_feature_enabled_when_not_in_table(self, mock_config_db_connector):
        """Test is_feature_enabled returns True when feature not found in table (fail-safe)"""
        mock_db = MagicMock()
        mock_config_db_connector.return_value = mock_db
        mock_db.get_table.return_value = {}

        result = memory_checker.is_feature_enabled('unknown_feature')

        self.assertTrue(result)

    @patch('syslog.syslog')
    @patch('memory_checker.ConfigDBConnector')
    def test_is_feature_enabled_on_exception(self, mock_config_db_connector, mock_syslog):
        """Test is_feature_enabled returns True on exception (fail-safe)"""
        mock_config_db_connector.side_effect = Exception('DB connection failed')

        result = memory_checker.is_feature_enabled('telemetry')

        self.assertTrue(result)
        mock_syslog.assert_called_once()

if __name__ == '__main__':
    unittest.main()
