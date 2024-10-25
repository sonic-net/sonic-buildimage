import unittest
from unittest.mock import patch, MagicMock
import sys
import os
import subprocess
import re

# Import the dhcp_dos_logger module directly
sys.path.append(os.path.join(os.path.dirname(__file__), '../../files/image_config/dhcp_dos_logger'))
import dhcp_dos_logger


class TestDhcpDosLogger(unittest.TestCase):

    @patch('dhcp_dos_logger.ConfigDBConnector')
    @patch('dhcp_dos_logger.subprocess.run')
    def test_handler_logging_packet_drops(self, mock_subprocess_run, mock_config_db):
        """
        Test if handler logs packet drop events correctly
        """

        # Setup mocks for ConfigDB and ports
        mock_config_db.return_value.get_table.return_value = {'Ethernet0': None, 'Ethernet1': None}
        dhcp_dos_logger.drop_pkts = {'Ethernet0': 0, 'Ethernet1': 0}

        # Simulate output from subprocess for dropped packets on Ethernet0 and Ethernet1
        mock_subprocess_run.return_value.returncode = 0
        mock_subprocess_run.return_value.stdout = b'qdisc ffff: dev Ethernet0 root refcnt 2 limit 1000p dropped 5'

        # Patch the logger to capture log output
        with patch.object(dhcp_dos_logger.logger, 'log_warning') as mock_log_warning:
            dhcp_dos_logger.handler()
            # Check if the warning log is called correctly when packets drop
            mock_log_warning.assert_any_call("Port Ethernet0: Current DHCP drop counter is 5")

    @patch('dhcp_dos_logger.ConfigDBConnector')
    @patch('dhcp_dos_logger.subprocess.run')
    def test_handler_no_drops_logged(self, mock_subprocess_run, mock_config_db):
        """
        Test if handler does not log when no new packet drops occur
        """
        mock_config_db.return_value.get_table.return_value = {'Ethernet0': None}
        dhcp_dos_logger.drop_pkts = {'Ethernet0': 5}

        # Simulate output showing no additional packet drops on Ethernet0
        mock_subprocess_run.return_value.returncode = 0
        mock_subprocess_run.return_value.stdout = b'qdisc ffff: dev Ethernet0 root refcnt 2 limit 1000p dropped 5'

        with patch.object(dhcp_dos_logger.logger, 'log_warning') as mock_log_warning:
            dhcp_dos_logger.handler()
            # Check that the log warning is NOT called because drop count did not increase
            mock_log_warning.assert_not_called()

    @patch('dhcp_dos_logger.time.sleep', return_value=None)
    def test_main_handler_runs(self, mock_sleep):
        """
        Test that the main function runs the handler
        """
        with patch('dhcp_dos_logger.handler') as mock_handler:
            dhcp_dos_logger.main()
            mock_handler.assert_called_once()


if __name__ == "__main__":
    unittest.main()
