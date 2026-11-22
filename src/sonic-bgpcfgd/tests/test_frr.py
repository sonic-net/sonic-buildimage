import os
import tempfile
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

def test_write(tmp_path, monkeypatch):
    original_mkstemp = tempfile.mkstemp
    monkeypatch.setattr(
        bgpcfgd.frr.tempfile,
        "mkstemp",
        lambda prefix, dir: original_mkstemp(prefix=prefix, dir=str(tmp_path))
    )

    def run_command(cmd):
        assert os.path.exists(cmd[2])
        return 0, "some output", ""

    bgpcfgd.frr.run_command = run_command
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    res = f.write("config context")
    assert res, "Expect True return value"
    assert list(tmp_path.iterdir()) == []

@pytest.mark.parametrize(
    "ret_code, expected_result, remove_called",
    [
        (0, True, True),
        (1, False, False),
    ]
)
@patch('bgpcfgd.frr.log_err')
def test_write_cleanup_error(
    mocked_log_err, ret_code, expected_result, remove_called, tmp_path, monkeypatch
):
    original_mkstemp = tempfile.mkstemp
    monkeypatch.setattr(
        bgpcfgd.frr.tempfile,
        "mkstemp",
        lambda prefix, dir: original_mkstemp(prefix=prefix, dir=str(tmp_path))
    )
    bgpcfgd.frr.run_command = lambda cmd: (ret_code, "some output", "some error")

    f = bgpcfgd.frr.FRR(["abc", "cde"])
    with patch('bgpcfgd.frr.os.remove', side_effect=OSError("remove error")) as mocked_remove:
        res = f.write("config context")

    assert res is expected_result
    assert mocked_remove.called is remove_called
    mocked_log_err.assert_called_once()
    if remove_called:
        assert "can't remove temporary file=" in mocked_log_err.call_args.args[0]
        assert "remove error" in mocked_log_err.call_args.args[0]
    else:
        assert len(list(tmp_path.iterdir())) == 1
        assert "can't push configuration from file=" in mocked_log_err.call_args.args[0]

def test_write_fail(tmp_path, monkeypatch):
    original_mkstemp = tempfile.mkstemp
    monkeypatch.setattr(
        bgpcfgd.frr.tempfile,
        "mkstemp",
        lambda prefix, dir: original_mkstemp(prefix=prefix, dir=str(tmp_path))
    )

    def run_command(cmd):
        assert os.path.exists(cmd[2])
        return 1, "some output", "some error"

    bgpcfgd.frr.run_command = run_command
    f = bgpcfgd.frr.FRR(["abc", "cde"])
    res = f.write("config context")
    assert not res, "Expect False return value"
    failed_config_files = list(tmp_path.iterdir())
    assert len(failed_config_files) == 1
    assert failed_config_files[0].name.startswith("bgpcfgd_failed_config.")
    assert failed_config_files[0].read_text() == "config context\n"

    res = f.write("new config context")
    assert not res, "Expect False return value"
    failed_config_files = list(tmp_path.iterdir())
    assert len(failed_config_files) == 2
    assert sorted(path.read_text() for path in failed_config_files) == [
        "config context\n",
        "new config context\n",
    ]

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
