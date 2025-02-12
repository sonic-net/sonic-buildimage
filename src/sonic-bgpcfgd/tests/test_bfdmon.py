import pytest
import json
from unittest.mock import MagicMock, patch
from swsscommon import swsscommon
import syslog
import bfdmon.bfdmon
from bfdmon.bfdmon import BfdFrrMon

@pytest.fixture
@patch('swsscommon.swsscommon.Table')
@patch('swsscommon.swsscommon.DBConnector', autospec=True)
@patch('swsscommon.swsscommon.SonicV2Connector')
def bfd_mon(mock_conn, mock_db, mock_tbl):
    #mock_conn.return_value.get_db_list.return_value = ['STATE_DB']
    m = BfdFrrMon()
    return m

def test_constructor(bfd_mon):
    assert len(bfd_mon.local_v4_peers) == 0
    assert len(bfd_mon.local_v6_peers) == 0

@patch('bfdmon.bfdmon.getstatusoutput_noshell', return_value=(0, "14211"))
def test_check_bfdd(mocked_getstatusoutput, bfd_mon):
    result = bfd_mon.check_bfdd()
    assert result == True

@patch('bfdmon.bfdmon.getstatusoutput_noshell', return_value=(1, ""))
def test_check_bfdd_failure(mocked_getstatusoutput, bfd_mon):
    # Command to check if bfdd is running returns invalid rc value
    result = bfd_mon.check_bfdd()
    assert result == False

@patch('bfdmon.bfdmon.getstatusoutput_noshell', side_effect=Exception("test e"))
def test_check_bfdd_exception(mocked_getstatusoutput, bfd_mon):
    # Command to check if bfdd is running returns exception
    result = bfd_mon.check_bfdd()
    assert result == False

@patch('bfdmon.bfdmon.getstatusoutput_noshell')
def test_get_bfd_sessions(mocked_getstatusoutput, bfd_mon):
    bfd_frr_json = '''
    [
        {
            "peer": "192.168.1.1",
            "local": "180.1.1.1",
            "status": "up"
        },
        {
            "peer": "192.168.1.2",
            "local": "181.1.1.1",
            "vrf": "default",
            "status": "up"
        },
        {
            "peer": "192.168.1.3",
            "local": "180.1.1.2",
            "status": "down"
        },
        {
            "peer": "30ab::2",
            "status": "up"
        },
        {
            "peer": "30ab::3",
            "status": "down"
        },
        {
            "peer": "30ab::4",
            "status": "up"
        }
    ]
    '''
    required_v4_peers = ["192.168.1.1", "192.168.1.2"]
    required_v6_peers = ["30ab::2", "30ab::4"]
    bfd_mon.bfdd_running = True
    mocked_getstatusoutput.return_value = (0, bfd_frr_json)
    result = bfd_mon.get_bfd_sessions()
    mocked_getstatusoutput.assert_called_once_with(["vtysh", "-c", "show bfd peers json"])
    assert result == True
    assert all(value in bfd_mon.frr_v4_peers for value in required_v4_peers), f"Expected {required_v4_peers} to be in {bfd_mon.frr_v4_peers}"
    assert all(value in bfd_mon.frr_v6_peers for value in required_v6_peers), f"Expected {required_v6_peers} to be in {bfd_mon.frr_v6_peers}"

@patch('bfdmon.bfdmon.getstatusoutput_noshell', return_value=(1, ""))
@patch('syslog.syslog')
def test_get_bfd_sessions_nobfdd(mocked_syslog, mocked_getstatusoutput, bfd_mon):
    # Check for bfdd fails so returns early
    result = bfd_mon.get_bfd_sessions()
    mocked_syslog.call_args[0][1]
    assert "bfdd not currently running" in mocked_syslog.call_args[0][1]
    assert result == False

@patch('bfdmon.bfdmon.getstatusoutput_noshell', return_value=(1, ""))
@patch('syslog.syslog')
def test_get_bfd_sessions_failure2(mocked_syslog, mocked_getstatusoutput, bfd_mon):
    # Command to get bfd peers json fails because rc is not valid
    bfd_mon.bfdd_running = True
    result = bfd_mon.get_bfd_sessions()
    assert "Failed with rc:1 when execute" in mocked_syslog.call_args[0][1]
    assert result == False

@patch('bfdmon.bfdmon.getstatusoutput_noshell', return_value=(0, ""))
@patch('syslog.syslog')
def test_get_bfd_sessions_failure3(mocked_syslog, mocked_getstatusoutput, bfd_mon):
    # Command to get bfd peers fails because output is empty
    bfd_mon.bfdd_running = True
    result = bfd_mon.get_bfd_sessions()
    assert "output none when execute" in mocked_syslog.call_args[0][1]
    assert result == False

@patch('bfdmon.bfdmon.getstatusoutput_noshell', side_effect=json.JSONDecodeError("wrong value", "doc", 0))
@patch('syslog.syslog')
def test_get_bfd_sessions_failure_json(mocked_syslog, mocked_getstatusoutput, bfd_mon):
    # Command to get bfd peers returns JSONDecodeError exception
    bfd_mon.bfdd_running = True
    bfd_mon.MAX_RETRY_ATTEMPTS=1
    result = bfd_mon.get_bfd_sessions()
    assert mocked_syslog.call_count == 2
    called_message = mocked_syslog.call_args_list[0][0][1]
    assert "JSONDecodeError" in called_message
    called_message = mocked_syslog.call_args_list[1][0][1]
    assert "Maximum retry attempts reached" in called_message
    assert result == False

@patch('bfdmon.bfdmon.getstatusoutput_noshell', side_effect=Exception("test e"))
@patch('syslog.syslog')
def test_get_bfd_sessions_failure_exp(mocked_syslog, mocked_getstatusoutput, bfd_mon):
    # Command to get bfd peers returns unexpected exception
    bfd_mon.bfdd_running = True
    result = bfd_mon.get_bfd_sessions()
    assert mocked_syslog.call_count == 4
    for i in range(3):    
        called_message = mocked_syslog.call_args_list[i][0][1]
        assert "An unexpected error occurred" in called_message

    called_message = mocked_syslog.call_args_list[3][0][1]
    assert "Maximum retry attempts reached" in called_message
    assert result == False

@patch('syslog.syslog')
def test_update_state_db_empty_no_init(mocked_syslog, bfd_mon):
    # Update if new peer is empty (same as initial value) and init is not done yet
    bfd_mon.frr_v4_peers = set()
    bfd_mon.frr_v6_peers = set()
    result = bfd_mon.update_state_db()
    assert mocked_syslog.call_count == 1
    assert all(value in bfd_mon.local_v4_peers for value in set()), f"Expected {set()} to be in {bfd_mon.local_v4_peers}"
    assert all(value in bfd_mon.local_v6_peers for value in set()), f"Expected {set()} to be in {bfd_mon.local_v6_peers}"

@patch('syslog.syslog')
def test_update_state_db_empty(mocked_syslog, bfd_mon):
    # Init is done, don't update if new peer set is empty
    bfd_mon.frr_v4_peers = set()
    bfd_mon.frr_v6_peers = set()
    bfd_mon.init_done = True
    result = bfd_mon.update_state_db()
    assert mocked_syslog.call_count == 0
    assert all(value in bfd_mon.local_v4_peers for value in set()), f"Expected {set()} to be in {bfd_mon.local_v4_peers}"
    assert all(value in bfd_mon.local_v6_peers for value in set()), f"Expected {set()} to be in {bfd_mon.local_v6_peers}"

@patch('syslog.syslog')
def test_update_state_db_same(mocked_syslog, bfd_mon):
    # Exact same peer list as locally stored so don't update
    bfd_mon.local_v4_peers = {"192.168.1.2", "192.168.1.1"}
    bfd_mon.local_v6_peers = {"30ab::2", "30ab::4"}
    bfd_mon.frr_v4_peers = {"192.168.1.1", "192.168.1.2"}
    bfd_mon.frr_v6_peers = {"30ab::2", "30ab::4"}
    bfd_mon.init_done = True
    result = bfd_mon.update_state_db()
    assert mocked_syslog.call_count == 0
    assert all(value in bfd_mon.local_v4_peers for value in bfd_mon.frr_v4_peers), f"Expected {bfd_mon.frr_v4_peers} to be in {bfd_mon.local_v4_peers}"
    assert all(value in bfd_mon.local_v6_peers for value in bfd_mon.frr_v6_peers), f"Expected {bfd_mon.frr_v6_peers} to be in {bfd_mon.local_v6_peers}"

@patch('syslog.syslog')
def test_update_state_db(mocked_syslog, bfd_mon):
    # Update in state_db
    bfd_mon.frr_v4_peers = {"192.168.1.1", "192.168.1.2"}
    bfd_mon.frr_v6_peers = {"30ab::2", "30ab::4"}
    result = bfd_mon.update_state_db()
    assert mocked_syslog.call_count == 1
    assert "DPU_BFD_PROBE_STATE table in STATE_DB updated" in mocked_syslog.call_args[0][1]
    assert all(value in bfd_mon.local_v4_peers for value in bfd_mon.frr_v4_peers), f"Expected {bfd_mon.frr_v4_peers} to be in {bfd_mon.local_v4_peers}"
    assert all(value in bfd_mon.local_v6_peers for value in bfd_mon.frr_v6_peers), f"Expected {bfd_mon.frr_v6_peers} to be in {bfd_mon.local_v6_peers}"

@patch('syslog.syslog')
def test_update_state_db_add(mocked_syslog, bfd_mon):
    # Update in state_db because new peers were added to existing list
    bfd_mon.local_v4_peers = {"192.168.1.2", "192.168.1.1"}
    bfd_mon.local_v6_peers = {"30ab::2", "30ab::4"}
    bfd_mon.frr_v4_peers = {"192.168.1.2", "192.168.1.1", "192.168.1.3"}
    bfd_mon.frr_v6_peers = {"30ab::2", "30ab::4", "30ab::5"}
    result = bfd_mon.update_state_db()
    assert mocked_syslog.call_count == 1
    assert "DPU_BFD_PROBE_STATE table in STATE_DB updated" in mocked_syslog.call_args[0][1]
    assert all(value in bfd_mon.local_v4_peers for value in bfd_mon.frr_v4_peers), f"Expected {bfd_mon.frr_v4_peers} to be in {bfd_mon.local_v4_peers}"
    assert all(value in bfd_mon.local_v6_peers for value in bfd_mon.frr_v6_peers), f"Expected {bfd_mon.frr_v6_peers} to be in {bfd_mon.local_v6_peers}"

@patch('syslog.syslog')
def test_update_state_db_del(mocked_syslog, bfd_mon):
    # Update in state_db because some peers were deleted compared to existing list
    bfd_mon.local_v4_peers = {"192.168.1.2", "192.168.1.1"}
    bfd_mon.local_v6_peers = {"30ab::2", "30ab::4"}
    bfd_mon.frr_v4_peers = {"192.168.1.1"}
    bfd_mon.frr_v6_peers = {"30ab::2"}
    result = bfd_mon.update_state_db()
    assert mocked_syslog.call_count == 1
    assert "DPU_BFD_PROBE_STATE table in STATE_DB updated" in mocked_syslog.call_args[0][1]
    assert all(value in bfd_mon.local_v4_peers for value in bfd_mon.frr_v4_peers), f"Expected {bfd_mon.frr_v4_peers} to be in {bfd_mon.local_v4_peers}"
    assert all(value in bfd_mon.local_v6_peers for value in bfd_mon.frr_v6_peers), f"Expected {bfd_mon.frr_v6_peers} to be in {bfd_mon.local_v6_peers}"

@patch('syslog.syslog')
def test_update_state_db_del_all(mocked_syslog, bfd_mon):
    # Update in state_db because no peers compared to existing list
    bfd_mon.local_v4_peers = {"192.168.1.2", "192.168.1.1"}
    bfd_mon.local_v6_peers = {"30ab::2", "30ab::4"}
    bfd_mon.frr_v4_peers = set()
    bfd_mon.frr_v6_peers = set()
    result = bfd_mon.update_state_db()
    assert mocked_syslog.call_count == 1
    assert "DPU_BFD_PROBE_STATE table in STATE_DB updated" in mocked_syslog.call_args[0][1]
    assert all(value in bfd_mon.local_v4_peers for value in bfd_mon.frr_v4_peers), f"Expected {bfd_mon.frr_v4_peers} to be in {bfd_mon.local_v4_peers}"
    assert all(value in bfd_mon.local_v6_peers for value in bfd_mon.frr_v6_peers), f"Expected {bfd_mon.frr_v6_peers} to be in {bfd_mon.local_v6_peers}"
