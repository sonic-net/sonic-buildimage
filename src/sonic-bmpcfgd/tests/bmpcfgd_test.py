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
scripts_path = os.path.join(modules_path, "bmpcfgd")
sys.path.insert(0, modules_path)

# Load the file under test
bmpcfgd_path = os.path.join(scripts_path, 'bmpcfgd.py')
bmpcfgd = load_module_from_source('bmpcfgd', bmpcfgd_path)


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

    def test_database_connectors_use_unix_sockets(self):
        MockConfigDb.set_config_db(self.test_data)

        daemon = bmpcfgd.BMPCfgDaemon()

        self.assertEqual(daemon.state_db_conn.init_args, ())
        self.assertEqual(daemon.state_db_conn.init_kwargs, {'use_unix_socket_path': True})
        self.assertEqual(daemon.state_db_conn.connected_db, bmpcfgd.BMP_STATE_DB)
        self.assertEqual(daemon.config_db.init_kwargs, {'use_unix_socket_path': True})
        self.assertEqual(daemon.config_db.connect_kwargs, {
            'wait_for_init': True,
            'retry_on': True,
        })

    @mock.patch('subprocess.call')
    @mock.patch('bmpcfgd.BMPCfg.log_info')
    def test_bmpcfgd_neighbor_enable(self, mock_call, mock_log_info):
        self.test_data['BMP']['table']['bgp_neighbor_table'] = 'true'
        MockConfigDb.set_config_db(self.test_data)
        bmp_config_daemon = bmpcfgd.BMPCfgDaemon()
        bmp_config_daemon.register_callbacks()
        bmp_config_daemon.bmp_handler("BMP", self.test_data)
        expected_calls = [
            mock.call(["supervisorctl", "stop", "openbmpd"]),
            mock.call(["supervisorctl", "start", "openbmpd"])
        ]
        mock_log_info.assert_has_calls(expected_calls)

    @mock.patch('subprocess.call')
    @mock.patch('bmpcfgd.BMPCfg.log_info')
    def test_bmpcfgd_bgp_rib_in_enable(self, mock_call, mock_log_info):
        self.test_data['BMP']['table']['bgp_rib_in_table'] = 'true'
        MockConfigDb.set_config_db(self.test_data)
        bmp_config_daemon = bmpcfgd.BMPCfgDaemon()
        bmp_config_daemon.bmp_handler("BMP", self.test_data)
        expected_calls = [
            mock.call(["supervisorctl", "stop", "openbmpd"]),
            mock.call(["supervisorctl", "start", "openbmpd"])
        ]
        mock_log_info.assert_has_calls(expected_calls)

    @mock.patch('subprocess.call')
    @mock.patch('bmpcfgd.BMPCfg.log_info')
    def test_bmpcfgd_bgp_rib_out_enable(self, mock_call, mock_log_info):
        self.test_data['BMP']['table']['bgp_rib_out_table'] = 'true'
        MockConfigDb.set_config_db(self.test_data)
        bmp_config_daemon = bmpcfgd.BMPCfgDaemon()
        bmp_config_daemon.bmp_handler("BMP", self.test_data)
        expected_calls = [
            mock.call(["supervisorctl", "stop", "openbmpd"]),
            mock.call(["supervisorctl", "start", "openbmpd"])
        ]
        mock_log_info.assert_has_calls(expected_calls)
