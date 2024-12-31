import unittest
from unittest.mock import patch, MagicMock
import subprocess
import syslog
import sys
import mgmt_oper_status

class TestMgmtOperStatusCheck(unittest.TestCase):

    @patch('mgmt_oper_status.SonicV2Connector')
    @patch('mgmt_oper_status.subprocess.run')
    @patch('mgmt_oper_status.syslog.syslog')
    def test_main_no_mgmt_ports(self, mock_syslog, mock_subprocess, mock_SonicV2Connector):
        mock_db = MagicMock()
        mock_SonicV2Connector.return_value = mock_db
        mock_db.keys.return_value = [] 

        mgmt_oper_status.main()

        mock_syslog.assert_called_with(syslog.LOG_DEBUG, 'No management interface found')

    @patch('mgmt_oper_status.SonicV2Connector')
    @patch('mgmt_oper_status.subprocess.run')
    @patch('mgmt_oper_status.syslog.syslog')
    def test_main_with_mgmt_ports(self, mock_syslog, mock_subprocess, mock_SonicV2Connector):
        mock_db = MagicMock()
        mock_SonicV2Connector.return_value = mock_db
        mgmt_ports_keys = ['MGMT_PORT|eth0', 'MGMT_PORT|eth1']
        mock_db.keys.return_value = mgmt_ports_keys
        mock_db.set.return_value = None

        mock_subprocess.return_value = subprocess.CompletedProcess(args=['cat', '/sys/class/net/eth0/operstate'], returncode=0, stdout='up', stderr='')

        mgmt_oper_status.main()

        mock_syslog.assert_any_call(syslog.LOG_INFO, 'mgmt_oper_status: up')
        mock_syslog.assert_any_call(syslog.LOG_INFO, 'mgmt_oper_status: up')

        mock_db.set.assert_any_call(mock_db.STATE_DB, 'MGMT_PORT_TABLE|eth0', 'oper_status', 'up')
        mock_db.set.assert_any_call(mock_db.STATE_DB, 'MGMT_PORT_TABLE|eth1', 'oper_status', 'up')

    @patch('mgmt_oper_status.SonicV2Connector')
    @patch('mgmt_oper_status.subprocess.run')
    @patch('mgmt_oper_status.syslog.syslog')
    def test_main_exception_handling(self, mock_syslog, mock_subprocess, mock_SonicV2Connector):
        mock_db = MagicMock()
        mock_SonicV2Connector.return_value = mock_db
        mgmt_ports_keys = ['MGMT_PORT|eth0']
        mock_db.keys.return_value = mgmt_ports_keys
        mock_db.set.return_value = None

        mock_subprocess.side_effect = Exception("File not found")

        mgmt_oper_status.main()

        mock_syslog.assert_called_with(syslog.LOG_ERR, "mgmt_oper_status exception : File not found")
        mock_db.set.assert_any_call(mock_db.STATE_DB, 'MGMT_PORT_TABLE|eth0', 'oper_status', 'unknown')

if __name__ == '__main__':
    unittest.main()
