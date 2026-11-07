import pytest
import json
import psutil
import sys
import time
from types import SimpleNamespace
from common_utils import MockProc, mock_get_config_db_table
from dhcp_utilities.common.utils import DhcpDbConnector
from dhcp_utilities.common.dhcp_db_monitor import DhcpServdDbMonitor
from dhcp_utilities.dhcpservd.dhcp_cfggen import DhcpServCfgGenerator
from dhcp_utilities.dhcpservd.dhcp_lease import LeaseManager
from dhcp_utilities.dhcpservd.dhcpservd import DhcpServd
from swsscommon import swsscommon
from unittest.mock import patch, call, MagicMock

AF_INET = 2
AF_INET6 = 10
PORT_MODE_CHECKER = ["DhcpServerTableCfgChangeEventChecker", "DhcpPortTableEventChecker", "DhcpRangeTableEventChecker",
                     "DhcpOptionTableEventChecker", "VlanTableEventChecker", "VlanIntfTableEventChecker",
                     "VlanMemberTableEventChecker"]


tested_config = """
{
    "key": "dummy_value\\\\,dummy_value"
}
"""


@pytest.mark.parametrize("enabled_checker", [None, set(PORT_MODE_CHECKER)])
def test_dump_dhcp4_config(mock_swsscommon_dbconnector_init, enabled_checker, tmp_path):
    new_enabled_checker = set(["VlanTableEventChecker"])
    generation_result = (
        tested_config,
        {"range1"},
        {"Vlan1000"},
        {"option223"},
        new_enabled_checker,
        {"Vlan1000"},
        set(),
        {"vendor-a"}
    )
    dhcp_cfg_generator = MagicMock()
    dhcp_cfg_generator.generate.return_value = generation_result
    monitor = MagicMock()
    config_path = tmp_path / "kea-dhcp4.conf"
    with patch("dhcp_utilities.dhcpservd.dhcpservd.DhcpServd._notify_kea_dhcp4_proc") \
            as mock_notify_kea_dhcp4_proc, \
         patch("dhcp_utilities.dhcpservd.dhcpservd.subprocess.run",
               return_value=SimpleNamespace(returncode=0, stdout="", stderr="")) as mock_validate:
        dhcpservd = DhcpServd(
            dhcp_cfg_generator,
            MagicMock(),
            monitor,
            kea_dhcp4_config_path=str(config_path),
            kea_dhcp4_binary="kea-dhcp4"
        )
        dhcpservd.enabled_checker = enabled_checker
        assert dhcpservd.dump_dhcp4_config()
        # Verfiy whether generate() func of dhcp_cfggen is called
        dhcp_cfg_generator.generate.assert_called_once_with()
        validate_args = mock_validate.call_args.args[0]
        assert validate_args[:2] == ["kea-dhcp4", "-t"]
        assert validate_args[2] != str(config_path)
        with open("tests/test_data/test_kea_config.conf", "r") as file, \
             open(config_path, "r") as output:
            expected_content = file.read()
            actual_content = output.read()
            assert json.loads(expected_content) == json.loads(actual_content)
        # Verify whether notify func of dhcpservd is called, which is expected to call after new config generated
        mock_notify_kea_dhcp4_proc.assert_called_once_with()
        if enabled_checker is None:
            monitor.enable_checkers.assert_not_called()
            monitor.disable_checkers.assert_not_called()
        else:
            monitor.disable_checkers.assert_called_once_with(enabled_checker - new_enabled_checker)
            monitor.enable_checkers.assert_not_called()
        assert dhcpservd.used_range == {"range1"}
        assert dhcpservd.enabled_port_interfaces == {"Vlan1000"}
        assert dhcpservd.enabled_match_interfaces == set()
        assert dhcpservd.used_matches == {"vendor-a"}


def test_dump_dhcp4_config_validation_failure_preserves_live_config(mock_swsscommon_dbconnector_init, tmp_path):
    config_path = tmp_path / "kea-dhcp4.conf"
    config_path.write_text("last-known-good")
    generator = MagicMock()
    generator.generate.return_value = (
        tested_config, {"new-range"}, {"Vlan2000"}, {"new-option"}, {"NewChecker"},
        set(), {"Vlan2000"}, {"new-match"}
    )
    monitor = MagicMock()
    with patch("dhcp_utilities.dhcpservd.dhcpservd.subprocess.run",
               return_value=SimpleNamespace(returncode=1, stdout="", stderr="invalid config")), \
         patch.object(DhcpServd, "_notify_kea_dhcp4_proc") as mock_notify:
        dhcpservd = DhcpServd(
            generator,
            MagicMock(),
            monitor,
            kea_dhcp4_config_path=str(config_path)
        )
        dhcpservd.enabled_checker = {"OldChecker"}
        dhcpservd.used_range = {"old-range"}

        assert not dhcpservd.dump_dhcp4_config()

        assert config_path.read_text() == "last-known-good"
        assert list(tmp_path.iterdir()) == [config_path]
        assert dhcpservd.enabled_checker == {"OldChecker"}
        assert dhcpservd.used_range == {"old-range"}
        monitor.disable_checkers.assert_not_called()
        monitor.enable_checkers.assert_called_once()
        recovery_checkers = monitor.enable_checkers.call_args.args[0]
        assert "MidPlaneTableEventChecker" in recovery_checkers
        assert "DpusTableEventChecker" in recovery_checkers
        mock_notify.assert_not_called()
        assert dhcpservd.reload_pending
        assert dhcpservd.recovery_checkers


def test_dump_dhcp4_config_generation_failure_preserves_live_config(mock_swsscommon_dbconnector_init, tmp_path):
    config_path = tmp_path / "kea-dhcp4.conf"
    config_path.write_text("last-known-good")
    generator = MagicMock()
    generator.generate.side_effect = ValueError("invalid binding")
    monitor = MagicMock()
    with patch.object(DhcpServd, "_notify_kea_dhcp4_proc") as mock_notify:
        dhcpservd = DhcpServd(generator, MagicMock(), monitor, kea_dhcp4_config_path=str(config_path))
        dhcpservd.enabled_checker = {"DhcpServerTableCfgChangeEventChecker"}

        assert not dhcpservd.dump_dhcp4_config()

        assert config_path.read_text() == "last-known-good"
        assert dhcpservd.enabled_checker == {"DhcpServerTableCfgChangeEventChecker"}
        assert dhcpservd.reload_pending
        monitor.enable_checkers.assert_called_once()
        mock_notify.assert_not_called()


def test_dump_dhcp4_config_validation_execution_failure_preserves_live_config(
        mock_swsscommon_dbconnector_init, tmp_path):
    config_path = tmp_path / "kea-dhcp4.conf"
    config_path.write_text("last-known-good")
    generator = MagicMock()
    generator.generate.return_value = (
        tested_config, set(), {"Vlan1000"}, set(), {"NewChecker"},
        set(), {"Vlan1000"}, {"new-match"}
    )
    monitor = MagicMock()
    with patch("dhcp_utilities.dhcpservd.dhcpservd.subprocess.run",
               side_effect=FileNotFoundError("kea-dhcp4")), \
         patch.object(DhcpServd, "_notify_kea_dhcp4_proc") as mock_notify:
        dhcpservd = DhcpServd(
            generator,
            MagicMock(),
            monitor,
            kea_dhcp4_config_path=str(config_path)
        )
        dhcpservd.enabled_checker = {"OldChecker"}

        assert not dhcpservd.dump_dhcp4_config()

    assert config_path.read_text() == "last-known-good"
    assert list(tmp_path.iterdir()) == [config_path]
    assert dhcpservd.enabled_checker == {"OldChecker"}
    assert dhcpservd.reload_pending
    monitor.enable_checkers.assert_called_once()
    mock_notify.assert_not_called()


def test_dump_dhcp4_config_success_clears_recovery_subscriptions(mock_swsscommon_dbconnector_init, tmp_path):
    config_path = tmp_path / "kea-dhcp4.conf"
    generator = MagicMock()
    generator.generate.return_value = (
        tested_config,
        set(),
        {"Vlan1000"},
        set(),
        {"DhcpServerBindingTableCfgChangeEventChecker"},
        set(),
        {"Vlan1000"},
        {"match-a"}
    )
    monitor = MagicMock()
    dhcpservd = DhcpServd(generator, MagicMock(), monitor, kea_dhcp4_config_path=str(config_path))
    dhcpservd.enabled_checker = {"DhcpServerTableCfgChangeEventChecker"}
    dhcpservd.recovery_checkers = {
        "DhcpServerTableCfgChangeEventChecker",
        "DhcpServerBindingTableCfgChangeEventChecker",
        "DhcpServerMatchTableCfgChangeEventChecker"
    }
    dhcpservd.reload_pending = True

    with patch("dhcp_utilities.dhcpservd.dhcpservd.subprocess.run",
               return_value=SimpleNamespace(returncode=0, stdout="", stderr="")), \
         patch.object(DhcpServd, "_notify_kea_dhcp4_proc"):
        assert dhcpservd.dump_dhcp4_config()

    monitor.disable_checkers.assert_called_once_with({
        "DhcpServerTableCfgChangeEventChecker",
        "DhcpServerMatchTableCfgChangeEventChecker"
    })
    monitor.enable_checkers.assert_not_called()
    assert dhcpservd.enabled_checker == {"DhcpServerBindingTableCfgChangeEventChecker"}
    assert dhcpservd.recovery_checkers == set()
    assert dhcpservd.reload_pending is False
    assert dhcpservd.enabled_match_interfaces == {"Vlan1000"}
    assert dhcpservd.used_matches == {"match-a"}


@pytest.mark.parametrize("process_list", [["proc1", "proc2", "kea-dhcp4"], ["proc1", "proc2"]])
def test_notify_kea_dhcp4_proc(process_list, mock_swsscommon_dbconnector_init, mock_get_render_template,
                               mock_parse_port_map_alias):
    proc_list = [MockProc(process_name) for process_name in process_list]
    proc_list.append(MockProc("exited_proc", exited=True))
    with patch.object(psutil, "process_iter", return_value=proc_list), \
         patch.object(MockProc, "send_signal", MagicMock()) as mock_send_signal, \
         patch("dhcp_utilities.dhcpservd.dhcpservd.signal.SIGHUP", 1, create=True):
        dhcp_db_connector = DhcpDbConnector()
        dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
        dhcpservd = DhcpServd(dhcp_cfg_generator, dhcp_db_connector, None)
        dhcpservd._notify_kea_dhcp4_proc()
        if "kea-dhcp4" in process_list:
            mock_send_signal.assert_has_calls([
                call(1)
            ])
        else:
            mock_send_signal.assert_not_called()


@pytest.mark.parametrize("mock_intf", [True, False])
def test_update_dhcp_server_ip(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, mock_get_render_template,
                               mock_intf):
    mock_interface = {} if not mock_intf else {
        "eth0": [
            MockIntf(AF_INET6, "fd00::2"),
            MockIntf(AF_INET, "240.127.1.2")
        ]
    }
    with patch.object(psutil, "net_if_addrs", return_value=mock_interface), \
         patch.object(swsscommon.DBConnector, "hset") as mock_hset, \
         patch.object(time, "sleep") as mock_sleep, \
         patch.object(sys, "exit") as mock_exit:
        dhcp_db_connector = DhcpDbConnector()
        dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
        dhcpservd = DhcpServd(dhcp_cfg_generator, dhcp_db_connector, None)
        dhcpservd._update_dhcp_server_ip()
        if mock_intf:
            mock_hset.assert_has_calls([
                call("DHCP_SERVER_IPV4_SERVER_IP|eth0", "ip", "240.127.1.2")
            ])
        else:
            mock_hset.assert_not_called()
            mock_exit.assert_called_once_with(1)
            mock_sleep.assert_has_calls([call(5) for _ in range(10)])


def test_start(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, mock_get_render_template):
    with patch.object(DhcpServd, "dump_dhcp4_config") as mock_dump, \
         patch.object(DhcpServd, "_update_dhcp_server_ip") as mock_update_dhcp_server_ip, \
         patch.object(LeaseManager, "start"), \
         patch.object(DhcpServdDbMonitor, "enable_checkers"), \
         patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table):
        dhcp_db_connector = DhcpDbConnector()
        dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
        dhcpservd = DhcpServd(dhcp_cfg_generator, dhcp_db_connector, MagicMock())
        dhcpservd.start()
        mock_dump.assert_called_once_with()
        mock_update_dhcp_server_ip.assert_called_once_with()


def test_start_exits_when_initial_candidate_is_invalid(mock_swsscommon_dbconnector_init):
    with patch.object(DhcpServd, "dump_dhcp4_config", return_value=False), \
         patch.object(sys, "exit", side_effect=SystemExit(1)) as mock_exit:
        dhcpservd = DhcpServd(MagicMock(), MagicMock(), MagicMock())
        with pytest.raises(SystemExit):
            dhcpservd.start()
        mock_exit.assert_called_once_with(1)


def test_signal_readiness(mock_swsscommon_dbconnector_init):
    with patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table), \
         patch("tempfile.NamedTemporaryFile"):
        import tempfile, os
        tmpdir = tempfile.mkdtemp()
        test_flag = os.path.join(tmpdir, "dhcpservd_ready")
        try:
            dhcp_db_connector = DhcpDbConnector()
            dhcpservd = DhcpServd(MagicMock(), dhcp_db_connector, MagicMock())
            with patch("dhcp_utilities.dhcpservd.dhcpservd.DHCPSERVD_READY_FLAG", test_flag):
                dhcpservd._signal_readiness()
            assert os.path.exists(test_flag), "readiness flag file was not created"
            with open(test_flag) as f:
                assert f.read() == str(os.getpid())
        finally:
            if os.path.exists(test_flag):
                os.remove(test_flag)
            os.rmdir(tmpdir)


class MockIntf(object):
    def __init__(self, family, address):
        self.family = family
        self.address = address
