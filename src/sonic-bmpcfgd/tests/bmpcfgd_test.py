import importlib.machinery
import importlib.util
import filecmp
import json
import shutil
import os
import sys
import signal
from swsscommon import swsscommon

from parameterized import parameterized
from unittest import TestCase, mock
from tests.common.mock_configdb import MockConfigDb, MockDBConnector
from tests.common.mock_bootloader import MockBootloader
from sonic_py_common.general import getstatusoutput_noshell
from .mock_connector import MockConnector
from sonic_py_common.general import load_module_from_source
from mock import patch

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
scripts_path = os.path.join(modules_path, "scripts")
sys.path.insert(0, modules_path)

# Load the file under test
bmpcfgd_path = os.path.join(scripts_path, 'bmpcfgd')
bmpcfgd = load_module_from_source('bmpcfgd', bmpcfgd_path)


from bmpcfgd import signal_handler

original_syslog = bmpcfgd.syslog

# Mock swsscommon classes
bmpcfgd.ConfigDBConnector = MockConfigDb
bmpcfgd.DBConnector = MockDBConnector
bmpcfgd.Table = mock.Mock()
swsscommon.SonicV2Connector = MockConnector

class TestBMPCfgDaemon(TestCase):
    """
        Test bmpcfgd daemon
    """
    def setUp(self):
        self.test_data = {}
        self.test_data['BMP'] = {}
        self.test_data['BMP']['table'] = {'bgp_neighbor_table': 'false', 'bgp_rib_in_table': 'false', 'bgp_rib_out_table': 'false'}

    @mock.patch('sonic_installer.bootloader.get_bootloader', side_effect=[MockBootloader()])
    @mock.patch('syslog.syslog')
    @mock.patch('subprocess.call')
    def test_bmpcfgd_neighbor_enable(self, mock_check_call, mock_syslog, mock_get_bootloader):
        self.test_data['BMP']['table']['bgp_neighbor_table'] = 'true'
        MockConfigDb.set_config_db(self.test_data)
        bmp_config_daemon = bmpcfgd.BMPCfgDaemon()
        bmp_config_daemon.register_callbacks()
        bmp_config_daemon.bmp_handler("BMP", '', self.test_data)
        expected_calls = [
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: update : True, False, False'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: stop bmp daemon'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: Reset bmp table from state_db'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: start bmp daemon'),
        ]
        mock_syslog.assert_has_calls(expected_calls)

    @mock.patch('sonic_installer.bootloader.get_bootloader', side_effect=[MockBootloader()])
    @mock.patch('syslog.syslog')
    @mock.patch('subprocess.check_call')
    def test_bmpcfgd_bgp_rib_in_enable(self, mock_check_call, mock_syslog, mock_get_bootloader):
        self.test_data['BMP']['table']['bgp_rib_in_table'] = 'true'
        MockConfigDb.set_config_db(self.test_data)
        bmp_config_daemon = bmpcfgd.BMPCfgDaemon()
        bmp_config_daemon.bmp_handler("BMP", '', self.test_data)
        expected_calls = [
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: update : False, True, False'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: stop bmp daemon'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: Reset bmp table from state_db'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: start bmp daemon'),
        ]
        mock_syslog.assert_has_calls(expected_calls)

    @mock.patch('sonic_installer.bootloader.get_bootloader', side_effect=[MockBootloader()])
    @mock.patch('syslog.syslog')
    @mock.patch('subprocess.check_call')
    def test_bmpcfgd_bgp_rib_out_enable(self, mock_check_call, mock_syslog, mock_get_bootloader):
        self.test_data['BMP']['table']['bgp_rib_out_table'] = 'true'
        MockConfigDb.set_config_db(self.test_data)
        bmp_config_daemon = bmpcfgd.BMPCfgDaemon()
        bmp_config_daemon.bmp_handler("BMP", '', self.test_data)
        expected_calls = [
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: update : False, False, True'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: stop bmp daemon'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: Reset bmp table from state_db'),
            mock.call(original_syslog.LOG_INFO, 'BMPCfg: start bmp daemon'),
        ]
        mock_syslog.assert_has_calls(expected_calls)


    @mock.patch('syslog.syslog')
    @mock.patch.object(sys, 'exit')
    def test_signal_handler(self, mock_exit, mock_syslog):
        # Test SIGHUP signal
        signal_handler(signal.SIGHUP, None)
        mock_syslog.assert_called_with(original_syslog.LOG_INFO, "bmpcfgd: signal 'SIGHUP' is caught and ignoring..")
        mock_exit.assert_not_called()
        # Test SIGINT signal
        signal_handler(signal.SIGINT, None)
        mock_syslog.assert_called_with(original_syslog.LOG_INFO, "bmpcfgd: signal 'SIGINT' is caught and exiting...")
        mock_exit.assert_called_once_with(128 + signal.SIGINT)
        # Test SIGTERM signal
        signal_handler(signal.SIGTERM, None)
        mock_syslog.assert_called_with(original_syslog.LOG_INFO, "bmpcfgd: signal 'SIGTERM' is caught and exiting...")
        mock_exit.assert_called_with(128 + signal.SIGTERM)
        # Test invalid signal
        signal_handler(999, None)
        mock_syslog.assert_called_with(original_syslog.LOG_INFO, "bmpcfgd: invalid signal - ignoring..")
