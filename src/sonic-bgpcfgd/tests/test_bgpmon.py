import pytest
import json
from unittest.mock import MagicMock, patch, call
import syslog
import bgpmon.bgpmon
from bgpmon.bgpmon import BgpStateGet


@pytest.fixture
@patch('swsscommon.swsscommon.RedisPipeline')
@patch('swsscommon.swsscommon.SonicV2Connector')
def bgp_mon(mock_conn, mock_pipe):
    mock_db = mock_conn.return_value
    mock_db.STATE_DB = 'STATE_DB'
    m = BgpStateGet()
    return m


@patch('bgpmon.bgpmon.getstatusoutput_noshell', return_value=(1, ""))
@patch('os.system', return_value=1)
@patch('syslog.syslog')
def test_vtysh_fail_both_daemons_down(mocked_syslog, mocked_os_system, mocked_cmd, bgp_mon):
    """When vtysh fails and neither bgpd nor zebra is running, log WARNING."""
    bgp_mon.get_all_neigh_states()
    mocked_syslog.assert_called_once()
    level, msg = mocked_syslog.call_args[0]
    assert level == syslog.LOG_WARNING
    assert "bgpd or zebra not running" in msg
    assert "container may be shutting down" in msg


@patch('bgpmon.bgpmon.getstatusoutput_noshell', return_value=(1, ""))
@patch('os.system', return_value=0)
@patch('syslog.syslog')
def test_vtysh_fail_both_daemons_running(mocked_syslog, mocked_os_system, mocked_cmd, bgp_mon):
    """When vtysh fails but bgpd and zebra ARE running, log ERR."""
    bgp_mon.get_all_neigh_states()
    mocked_syslog.assert_called_once()
    level, msg = mocked_syslog.call_args[0]
    assert level == syslog.LOG_ERR
    assert "*ERROR* Failed with rc:1 when execute" in msg


@patch('bgpmon.bgpmon.getstatusoutput_noshell', return_value=(1, ""))
@patch('os.system', side_effect=[1, 0])
@patch('syslog.syslog')
def test_vtysh_fail_only_bgpd_down(mocked_syslog, mocked_os_system, mocked_cmd, bgp_mon):
    """When vtysh fails and only bgpd is down (first pgrep fails), log WARNING."""
    bgp_mon.get_all_neigh_states()
    mocked_syslog.assert_called_once()
    level, msg = mocked_syslog.call_args[0]
    assert level == syslog.LOG_WARNING
    assert "bgpd or zebra not running" in msg


@patch('bgpmon.bgpmon.getstatusoutput_noshell', return_value=(1, ""))
@patch('os.system', side_effect=[0, 1])
@patch('syslog.syslog')
def test_vtysh_fail_only_zebra_down(mocked_syslog, mocked_os_system, mocked_cmd, bgp_mon):
    """When vtysh fails and only zebra is down (second pgrep fails), log WARNING."""
    bgp_mon.get_all_neigh_states()
    mocked_syslog.assert_called_once()
    level, msg = mocked_syslog.call_args[0]
    assert level == syslog.LOG_WARNING
    assert "bgpd or zebra not running" in msg


@patch('bgpmon.bgpmon.getstatusoutput_noshell', return_value=(1, "some vtysh error output here"))
@patch('os.system', return_value=1)
@patch('syslog.syslog')
def test_vtysh_fail_warning_includes_output(mocked_syslog, mocked_os_system, mocked_cmd, bgp_mon):
    """WARNING message includes truncated vtysh output."""
    bgp_mon.get_all_neigh_states()
    level, msg = mocked_syslog.call_args[0]
    assert level == syslog.LOG_WARNING
    assert "some vtysh error output here" in msg
