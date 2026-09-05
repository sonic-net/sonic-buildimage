from unittest.mock import patch
import bgpcfgd.frr
import pytest

def test_constructor():
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    assert f.daemons == ["abc", "cde"]

def test_wait_for_daemons():
    bgpcfgd.frr.run_command = lambda cmd, **kwargs: (0, ["abc", "cde"], "")
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    f.wait_for_daemons(5)

def test_wait_for_daemons_fail():
    bgpcfgd.frr.run_command = lambda cmd, **kwargs: (0, ["abc", "non_expected"], "")
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    with pytest.raises(Exception):
        assert f.wait_for_daemons(5)

def test_wait_for_daemons_error():
    bgpcfgd.frr.run_command = lambda cmd, **kwargs: (1, ["abc", "cde"], "some error")
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    with pytest.raises(Exception):
        assert f.wait_for_daemons(5)

def test_get_config():
    bgpcfgd.frr.run_command = lambda cmd: (0, "expected config", "")
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    out = f.get_config()
    assert out == "expected config"

@patch('bgpcfgd.frr.log_crit')
def test_get_config_fail(mocked_log_crit):
    bgpcfgd.frr.run_command = lambda cmd: (1, "some config", "some error")
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    out = f.get_config()
    assert out == ""
    mocked_log_crit.assert_called_with("can't update running config: rc=1 out='some config' err='some error'")

def test_write():
    bgpcfgd.frr.run_command = lambda cmd: (0, "some output", "")
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    res = f.write("config context")
    assert res, "Expect True return value"

def test_write_fail():
    bgpcfgd.frr.run_command = lambda cmd: (1, "some output", "some error")
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    res = f.write("config context")
    assert not res, "Expect False return value"

def test_restart_peer_groups():
    bgpcfgd.frr.run_command = lambda cmd: (0, "some output", "")
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    res = f.restart_peer_groups(["pg_1", "pg_2"])
    assert res, "Expect True return value"

@patch('bgpcfgd.frr.log_crit')
def test_restart_peer_groups_fail(mocked_log_crit):
    return_value_map = {
        "['vtysh', '-c', 'clear bgp peer-group pg_1 soft in']": (0, "", ""),
        "['vtysh', '-c', 'clear bgp peer-group pg_2 soft in']": (1, "some output", "some error")
    }
    bgpcfgd.frr.run_command = lambda cmd: return_value_map[str(cmd)]
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    res = f.restart_peer_groups(["pg_1", "pg_2"])
    assert not res, "Expect False return value"
    mocked_log_crit.assert_called_with("Can't restart bgp peer-group 'pg_2'. rc='1', out='some output', err='some error'")
