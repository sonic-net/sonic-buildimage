from unittest.mock import MagicMock, patch

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from copy import deepcopy
from . import swsscommon_test
import pytest
import subprocess

import sys
sys.modules["swsscommon"] = swsscommon_test

import bgpcfgd.managers_bfd
from bgpcfgd.managers_bfd import BfdMgr

@pytest.fixture
@patch('bgpcfgd.managers_bfd.run_command', return_value=(0, '{}', ''))
@patch('bgpcfgd.managers_bfd.subprocess.check_output')
def bfd_mgr(mocked_check_output, mocked_run_command):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }

    cmd = [
        'vtysh', '-c', 'show bfd peers json'
    ]
    mocked_check_output.return_value = b'bfdd running'
    m = BfdMgr(common_objs, "STATE_DB", "SOFTWARE_BFD_SESSION_TABLE")
    mocked_run_command.assert_called_once_with(cmd)

    return m

def test_constructor(bfd_mgr):
    assert len(bfd_mgr.bfd_sessions) == 0

@patch('subprocess.run', return_value=subprocess.CompletedProcess(args=[], returncode=0, stdout=''))
@patch('bgpcfgd.managers_bfd.subprocess.check_output', side_effect=subprocess.CalledProcessError(1, 'pgrep'))
@patch('bgpcfgd.managers_bfd.log_warn')
def test_check_and_start_bfdd_stopped(mocked_log_warn, mocked_check_output, mocked_run_command, bfd_mgr):
    cmd = [
        '/usr/lib/frr/bfdd', '-A', '127.0.0.1', '-d'
    ]
    assert bfd_mgr.check_and_start_bfdd() == True
    mocked_run_command.assert_called_once_with(cmd)
    mocked_log_warn.assert_called_with("bfdd process is not running, starting now...")

@patch('subprocess.run', return_value=subprocess.CompletedProcess(args=[], returncode=1, stdout=''))
@patch('bgpcfgd.managers_bfd.subprocess.check_output', side_effect=subprocess.CalledProcessError(1, 'pgrep'))
@patch('bgpcfgd.managers_bfd.log_err')
@patch('bgpcfgd.managers_bfd.log_warn')
def test_check_and_start_bfdd_error(mocked_log_warn, mocked_log_err, mocked_check_output,mocked_run_command,  bfd_mgr):
    cmd = [
        '/usr/lib/frr/bfdd', '-A', '127.0.0.1', '-d'
    ]
    assert bfd_mgr.check_and_start_bfdd() == False
    mocked_run_command.assert_called_once_with(cmd)
    mocked_log_warn.assert_called_with("bfdd process is not running, starting now...")
    mocked_log_err.assert_called_with("Can't start bfdd: 1")

def test_get_def_res_fields(bfd_mgr):
    result = bfd_mgr.get_def_res_fields()
    expected_result = {
        'multihop': False,
        'local': '',
        'detect-multiplier': 3,
        'receive-interval_ms': 200,
        'transmit-interval_ms': 200,
        'passive-mode': True,
    }
    assert result == expected_result

def test_redis_to_local_res(bfd_mgr):
    data = {
        "multihop": "false",
        "local_addr": "127.0.0.1",
        "multiplier": "5",
        "rx_interval": "300",
        "tx_interval": "300",
        "type": "async_active",
    }
    result = bfd_mgr.redis_to_local_res(data)
    expected_result = {
        'multihop': False,
        'local': '127.0.0.1',
        'detect-multiplier': 5,
        'receive-interval_ms': 300,
        'transmit-interval_ms': 300,
        'passive-mode': False,
    }
    assert result == expected_result

def test_redis_to_local_res_no_mh(bfd_mgr):
    data = {
        "local_addr": "127.0.0.1",
        "multiplier": "5",
        "rx_interval": "300",
        "tx_interval": "300",
        "type": "async_passive",
    }
    result = bfd_mgr.redis_to_local_res(data)
    expected_result = {
        'multihop': False,
        'local': '127.0.0.1',
        'detect-multiplier': 5,
        'receive-interval_ms': 300,
        'transmit-interval_ms': 300,
        'passive-mode': True,
    }
    assert result == expected_result

def test_redis_to_local_res_basic(bfd_mgr):
    data = {
        "local_addr": "127.0.0.1",
    }
    result = bfd_mgr.redis_to_local_res(data)
    expected_result = {
        'multihop': False,
        'local': '127.0.0.1',
        'detect-multiplier': 3,
        'receive-interval_ms': 200,
        'transmit-interval_ms': 200,
        'passive-mode': True,
    }
    assert result == expected_result

@patch('bgpcfgd.managers_bfd.run_command', return_value=(0, '', ''))
def test_add_frr_session_success(mocked_run_command, bfd_mgr):
    session_key = {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}
    data = {
        "multihop": "true",
        "local_addr": "127.0.0.1",
        "type": "async_active",
    }
    add_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd', '-c',
        'peer 10.0.0.1 multihop local-address 127.0.0.1 vrf default',
        '-c', 'detect-multiplier 3', '-c', 'receive-interval 200',
        '-c', 'transmit-interval 200', '-c', 'minimum-ttl 1',
        '-c', 'no passive-mode'
    ]
    assert bfd_mgr.add_frr_session(session_key, data) == True
    mocked_run_command.assert_called_once_with(add_cmd)
    assert bfd_mgr.bfd_sessions[bfd_mgr.dict_to_fs(session_key)] == \
        bfd_mgr.redis_to_local_res(data)

@patch('bgpcfgd.managers_bfd.run_command', return_value=(1, '', 'Error'))
@patch('bgpcfgd.managers_bfd.log_err')
def test_add_frr_session_failure(mocked_log_err, mocked_run_command, bfd_mgr):
    session_key = {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}
    data = {
        "multihop": "true",
        "local_addr": "127.0.0.1",
        "type": "async_active",
    }
    assert bfd_mgr.add_frr_session(session_key, data) == False
    mocked_log_err.assert_called_with("Can't add bfd session: {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}, err: Error")
    assert len(bfd_mgr.bfd_sessions) == 0

@patch('bgpcfgd.managers_bfd.run_command', return_value=(0, '', ''))
def test_del_frr_session_success(mocked_run_command, bfd_mgr):
    session_key = {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}
    local_res_data = {
        'multihop': True,
        'local': '127.0.0.1',
        'detect-multiplier': 3,
        'receive-interval_ms': 200,
        'transmit-interval_ms': 200,
        'passive-mode': False,
    }
    bfd_mgr.bfd_sessions[bfd_mgr.dict_to_fs(session_key)] = local_res_data

    del_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd', '-c',
        'no peer 10.0.0.1 multihop local-address 127.0.0.1 vrf default'
    ]
    assert bfd_mgr.del_frr_session(session_key) == True
    mocked_run_command.assert_called_once_with(del_cmd)
    assert len(bfd_mgr.bfd_sessions) == 0

@patch('bgpcfgd.managers_bfd.run_command', return_value=(1, '', 'Error'))
@patch('bgpcfgd.managers_bfd.log_err')
def test_del_frr_session_failure(mocked_log_err, mocked_run_command, bfd_mgr):
    session_key = {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}
    local_res_data = {
        'multihop': True,
        'local': '127.0.0.1',
        'detect-multiplier': 3,
        'receive-interval_ms': 200,
        'transmit-interval_ms': 200,
        'passive-mode': False,
    }
    bfd_mgr.bfd_sessions[bfd_mgr.dict_to_fs(session_key)] = local_res_data
    assert bfd_mgr.del_frr_session(session_key) == False
    mocked_log_err.assert_called_with("Can't delete bfd session: Error")
    assert len(bfd_mgr.bfd_sessions) == 1

@patch('bgpcfgd.managers_bfd.run_command')
@patch('bgpcfgd.managers_bfd.log_err')
def test_load_bfd_sessions(mocked_log_err, mocked_run_command, bfd_mgr):
    bfd_frr_json = '''
    [
        {
            "peer": "192.168.1.1",
            "local": "180.1.1.1",
            "vrf": "default",
            "detect-multiplier": 5,
            "transmit-interval_ms": 500,
            "receive-interval_ms": 200,
            "passive-mode": true
        },
        {
            "local": "181.1.1.1",
            "vrf": "default"
        },
        {
            "peer": "192.168.1.2",
            "local": "180.1.1.2",
            "vrf": "default",
            "detect-multiplier": 6,
            "transmit-interval_ms": 100,
            "receive-interval_ms": 300,
            "passive-mode": false
        },
        {
            "peer": "105::109",
            "local": "180::220",
            "vrf": "default",
            "detect-multiplier": 4,
            "transmit-interval_ms": 900,
            "receive-interval_ms": 700,
            "passive-mode": false
        },
        {
            "peer": "110::101",
            "local": "190::57",
            "vrf": "default",
            "detect-multiplier": 3,
            "transmit-interval_ms": 100,
            "receive-interval_ms": 100,
            "passive-mode": true
        }
    ]
    '''
    expected_sessions = {
        frozenset({('vrf', 'default'), ('peer', '192.168.1.1'), ('interface', 'default')}): {
            'detect-multiplier': 5,
            'local': '180.1.1.1',
            'multihop': False,
            'passive-mode': True,
            'receive-interval_ms': 200,
            'transmit-interval_ms': 500
        },
        frozenset({('peer', '192.168.1.2'), ('vrf', 'default'), ('interface', 'default')}): {
            'detect-multiplier': 6,
            'local': '180.1.1.2',
            'multihop': False,
            'passive-mode': False,
            'receive-interval_ms': 300,
            'transmit-interval_ms': 100
        },
        frozenset({('peer', '105::109'), ('vrf', 'default'), ('interface', 'default')}): {
            'detect-multiplier': 4,
            'local': '180::220',
            'multihop': False,
            'passive-mode': False,
            'receive-interval_ms': 700,
            'transmit-interval_ms': 900
        },
        frozenset({('peer', '110::101'), ('vrf', 'default'), ('interface', 'default')}): {
            'detect-multiplier': 3,
            'local': '190::57',
            'multihop': False,
            'passive-mode': True,
            'receive-interval_ms': 100,
            'transmit-interval_ms': 100
        },
    }
    mocked_run_command.return_value = (0, bfd_frr_json, "")
    bfd_sessions = bfd_mgr.load_bfd_sessions()
    mocked_log_err.assert_called_with("Peer is not set in frr bfd session, skipping")
    mocked_run_command.assert_called_once_with(["vtysh", "-c", "show bfd peers json"])
    assert len(bfd_sessions) == 4
    assert bfd_sessions == expected_sessions

@patch('bgpcfgd.managers_bfd.run_command', return_value=(1, '', 'Error'))
@patch('bgpcfgd.managers_bfd.log_err')
def test_load_bfd_sessions_failure(mocked_log_err, mocked_run_command, bfd_mgr):
    bfd_sessions = bfd_mgr.load_bfd_sessions()
    assert len(bfd_sessions) == 0
    mocked_log_err.assert_called_with("Can't read bfd peers: Error")

@patch('bgpcfgd.managers_bfd.run_command', return_value=(0, '', ''))
def test_update_frr_session_success(mocked_run_command, bfd_mgr):
    session_key = {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}
    local_res_data = {
        'multihop': True,
        'local': '127.0.0.1',
        'detect-multiplier': 3,
        'receive-interval_ms': 200,
        'transmit-interval_ms': 200,
        'passive-mode': False,
    }
    data = {
        "multihop": "true",
        "local_addr": "127.0.0.2",
        "multiplier": "5",
        "rx_interval": "300",
        "tx_interval": "400",
        "type": "async_passive",
    }

    bfd_mgr.bfd_sessions[bfd_mgr.dict_to_fs(session_key)] = local_res_data

    #New data has different local addr so frr delete and then add command
    #should be sent.
    update_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd', '-c',
        'no peer 10.0.0.1 multihop local-address 127.0.0.1 vrf default',
        '-c', 'peer 10.0.0.1 multihop local-address 127.0.0.2 vrf default',
        '-c', 'detect-multiplier 5', '-c', 'receive-interval 300',
        '-c','transmit-interval 400', '-c', 'minimum-ttl 1',
        '-c', 'passive-mode'
    ]
    assert bfd_mgr.update_frr_session(session_key, data) == True
    mocked_run_command.assert_called_with(update_cmd)

@patch('bgpcfgd.managers_bfd.run_command', return_value=(1, '', 'Error'))
@patch('bgpcfgd.managers_bfd.log_err')
@patch('bgpcfgd.managers_bfd.log_warn')
def test_update_frr_session_failure(mocked_log_warn, mocked_log_err, mocked_run_command, bfd_mgr):
    session_key = {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}
    local_res_data = {
        'multihop': True,
        'local': '127.0.0.1',
        'passive-mode': True,
    }
    data = {
        "multihop": "true",
        "local_addr": "127.0.0.1",
        "type": "async_passive",
    }
    #No difference in existing bfd session data and new data
    bfd_mgr.bfd_sessions[bfd_mgr.dict_to_fs(session_key)] = \
        bfd_mgr.redis_to_local_res(data)
    assert bfd_mgr.update_frr_session(session_key, data) == True
    mocked_log_warn.assert_called_with("bfd session fields are same, skipping update in frr")

    #Error while running command
    data["local_addr"] = "127.0.0.2"
    assert bfd_mgr.update_frr_session(session_key, data) == False
    mocked_log_err.assert_called_with("Can't update bfd session: {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}, err: Error")

@patch('bgpcfgd.managers_bfd.run_command', return_value=(0, '', ''))
@patch('bgpcfgd.managers_bfd.log_warn')
def test_set_handler_add(mocked_log_warn, mocked_run_command, bfd_mgr):
    key = "default|default|10.0.0.1"
    data = {
        "multihop": "true",
        "local_addr": "127.0.0.1",
        "type": "async_passive",
    }
    add_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd', '-c', 
        'peer 10.0.0.1 multihop local-address 127.0.0.1 vrf default', '-c',
        'detect-multiplier 3', '-c', 'receive-interval 200', '-c',
        'transmit-interval 200', '-c', 'minimum-ttl 1', '-c', 'passive-mode'
    ]
    assert bfd_mgr.set_handler(key, data) == True
    mocked_run_command.assert_called_once_with(add_cmd)
    mocked_log_warn.assert_called_with("add a new bfd session to frr")
    assert len(bfd_mgr.bfd_sessions) == 1

@patch('bgpcfgd.managers_bfd.run_command', return_value=(0, '', ''))
@patch('bgpcfgd.managers_bfd.log_warn')
def test_set_handler_add_sh(mocked_log_warn, mocked_run_command, bfd_mgr):
    key = "default|eth0|10.0.0.1"
    data = {
        "local_addr": "127.0.0.1",
        "type": "async_active",
    }
    add_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd', '-c', 
        'peer 10.0.0.1 local-address 127.0.0.1 interface eth0 vrf default',
        '-c', 'detect-multiplier 3', '-c', 'receive-interval 200',
        '-c', 'transmit-interval 200', '-c', 'no passive-mode'
    ]
    assert bfd_mgr.set_handler(key, data) == True
    mocked_run_command.assert_called_once_with(add_cmd)
    mocked_log_warn.assert_called_with("add a new bfd session to frr")
    assert len(bfd_mgr.bfd_sessions) == 1

@patch('bgpcfgd.managers_bfd.run_command', return_value=(0, '', ''))
@patch('bgpcfgd.managers_bfd.log_warn')
def test_set_handler_upd(mocked_log_warn, mocked_run_command, bfd_mgr):
    key = "default|default|10.0.0.1"
    session_key = {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}
    local_res_data = {
        'multihop': True,
        'local': '127.0.0.1',
        'detect-multiplier': 3,
        'receive-interval_ms': 200,
        'transmit-interval_ms': 200,
        'passive-mode': False,
    }
    data = {
        "multihop": "true",
        "local_addr": "127.0.0.1",
        "type": "async_async",
    }
    bfd_mgr.bfd_sessions[bfd_mgr.dict_to_fs(session_key)] = local_res_data

    #New data has no change in multihop or local_addr so only update command
    #should be sent.
    upd_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd', '-c', 
        'peer 10.0.0.1 multihop local-address 127.0.0.1 vrf default', '-c',
        'detect-multiplier 3', '-c', 'receive-interval 200', '-c',
        'transmit-interval 200', '-c', 'minimum-ttl 1', '-c', 'passive-mode'
    ]
    assert bfd_mgr.set_handler(key, data) == True
    mocked_run_command.assert_called_once_with(upd_cmd)
    mocked_log_warn.assert_called_with("found existing session, update if needed")
    assert len(bfd_mgr.bfd_sessions) == 1

@patch('bgpcfgd.managers_bfd.run_command', return_value=(0, '', ''))
@patch('bgpcfgd.managers_bfd.log_warn')
def test_del_handler(mocked_log_warn, mocked_run_command, bfd_mgr):
    key = "default|default|10.0.0.1"
    session_key = {'vrf': 'default', 'interface': 'default', 'peer': '10.0.0.1'}
    local_res_data = {
        'multihop': True,
        'local': '127.0.0.1',
        'detect-multiplier': 3,
        'receive-interval_ms': 200,
        'transmit-interval_ms': 200,
        'passive-mode': False,
    }
    data = {
        "multihop": "true",
        "local_addr": "127.0.0.1",
        "type": "async_async",
    }
    bfd_mgr.bfd_sessions[bfd_mgr.dict_to_fs(session_key)] = local_res_data

    del_cmd = [
        'vtysh', '-c', 'conf t', '-c', 'bfd', '-c',
        'no peer 10.0.0.1 multihop local-address 127.0.0.1 vrf default'
    ]
    assert bfd_mgr.del_handler(key) == True
    mocked_run_command.assert_called_once_with(del_cmd)
    warn_msg = "found existing bfd session to delete"
    assert any(call.args[0].startswith(warn_msg) for call in mocked_log_warn.call_args_list)
    assert len(bfd_mgr.bfd_sessions) == 0

@patch('bgpcfgd.managers_bfd.log_err')
def test_del_handler_no_exist(mocked_log_err, bfd_mgr):
    # Try to delete non-existing session
    key = "default|default|11.0.0.1"
    assert bfd_mgr.del_handler(key) == True
    err_msg = "no existing bfd session found, key: "
    assert any(call.args[0].startswith(err_msg) for call in mocked_log_err.call_args_list)
