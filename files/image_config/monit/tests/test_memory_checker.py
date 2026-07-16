import unittest
from unittest.mock import patch, MagicMock
import sys
import subprocess

import memory_checker


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

    @patch('memory_checker.exit_if_container_stopped')
    @patch('memory_checker.get_command_result')
    def test_get_container_id(self, mock_get_command_result, mock_exit_check):
        container_name = 'your_container'
        command = ['docker', 'ps', '--no-trunc', '--filter', 'name=your_container']
        mock_get_command_result.return_value = ''

        with self.assertRaises(SystemExit) as cm:
            memory_checker.get_container_id(container_name)
        self.assertEqual(cm.exception.code, 1)
        mock_get_command_result.assert_called_once_with(command)

    @patch('memory_checker.exit_if_container_stopped')
    @patch('memory_checker.open', side_effect=FileNotFoundError)
    def test_get_memory_usage(self, mock_open, mock_exit_check):
        container_id = 'your_container_id'
        container_name = 'your_container'
        with self.assertRaises(SystemExit) as cm:
            memory_checker.get_memory_usage(container_id, container_name)
        self.assertEqual(cm.exception.code, 1)

    @patch('memory_checker.open', side_effect=FileNotFoundError)
    def test_get_memory_usage_invalid(self, mock_open):
        container_id = '../..'
        container_name = 'your_container'
        with self.assertRaises(SystemExit) as cm:
            memory_checker.get_memory_usage(container_id, container_name)
        self.assertEqual(cm.exception.code, 1)

    @patch('memory_checker.exit_if_container_stopped')
    @patch('builtins.open', side_effect=FileNotFoundError)
    def test_get_inactive_cache_usage(self, mock_open, mock_exit_check):
        container_id = 'your_container_id'
        container_name = 'your_container'
        with self.assertRaises(SystemExit) as cm:
            memory_checker.get_inactive_cache_usage(container_id, container_name)
        self.assertEqual(cm.exception.code, 1)

    @patch('memory_checker.get_running_container_names', return_value=[])
    def test_exit_if_container_stopped(self, mock_running):
        """Container not running - should exit gracefully."""
        with self.assertRaises(SystemExit) as cm:
            memory_checker.exit_if_container_stopped('gnmi')
        self.assertEqual(cm.exception.code, 0)

    @patch('memory_checker.get_running_container_names', return_value=['gnmi'])
    def test_exit_if_container_stopped_still_running(self, mock_running):
        """Container still running - should not exit."""
        memory_checker.exit_if_container_stopped('gnmi')

    @patch('syslog.syslog')
    @patch('memory_checker.get_container_id')
    @patch('memory_checker.get_memory_usage')
    @patch('memory_checker.get_inactive_cache_usage')
    def test_check_memory_usage(self, mock_get_inactive_cache_usage, mock_get_memory_usage, mock_get_container_id, mock_syslog):
        container_name = 'your_container'
        threshold_value = 1024
        container_id = 'your_container'
        memory_usage = 2048
        cache_usage = 512
        mock_get_container_id.return_value = container_id
        mock_get_memory_usage.return_value = str(memory_usage)
        mock_get_inactive_cache_usage.return_value = str(cache_usage)

        with self.assertRaises(SystemExit) as cm:
            memory_checker.check_memory_usage(container_name, threshold_value)

        self.assertEqual(cm.exception.code, 3)
        mock_get_memory_usage.assert_called_once_with(container_id, container_name)

if __name__ == '__main__':
    unittest.main()
