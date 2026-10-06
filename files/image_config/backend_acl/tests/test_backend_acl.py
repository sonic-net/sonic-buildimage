import importlib.util
import sys
import types
from pathlib import Path
from unittest import mock


class FakeSonicV2Connector:
    STATE_DB = 6


swsscommon_package = types.ModuleType("swsscommon")
swsscommon_module = types.ModuleType("swsscommon.swsscommon")
swsscommon_module.SonicV2Connector = FakeSonicV2Connector
swsscommon_package.swsscommon = swsscommon_module
sys.modules.setdefault("swsscommon", swsscommon_package)
sys.modules.setdefault("swsscommon.swsscommon", swsscommon_module)

syslog_module = types.ModuleType("syslog")
syslog_module.LOG_INFO = 6
syslog_module.openlog = mock.Mock()
syslog_module.syslog = mock.Mock()
syslog_module.closelog = mock.Mock()
sys.modules.setdefault("syslog", syslog_module)

module_path = Path(__file__).resolve().parents[1] / "backend_acl.py"
spec = importlib.util.spec_from_file_location("backend_acl", module_path)
backend_acl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(backend_acl)


def test_run_command_uses_exit_status_not_stderr():
    completed = mock.Mock(returncode=0, stdout="ToRRouter\n", stderr="warning\n")
    with mock.patch.object(backend_acl.subprocess, "run", return_value=completed):
        success, output = backend_acl.run_command(["command"])

    assert success is True
    assert output == "ToRRouter"


def test_device_type_is_read_from_config_db():
    with mock.patch.object(
            backend_acl,
            "run_command",
            return_value=(True, "BackEndToRRouter")) as run_command:
        assert backend_acl._get_device_type() == "BackEndToRRouter"

    command = run_command.call_args.args[0]
    assert command == [
        backend_acl.SONIC_CFGGEN_PATH,
        "-d",
        "-v",
        "DEVICE_METADATA.localhost.type"
    ]
    assert "-m" not in command


def test_main_fails_when_device_type_is_unavailable():
    with mock.patch.object(backend_acl, "_get_device_type", return_value=None), \
            mock.patch.object(backend_acl, "load_backend_acl") as load_backend_acl:
        assert backend_acl.main() == 1

    load_backend_acl.assert_not_called()


def test_main_skips_non_backend_device():
    with mock.patch.object(backend_acl, "_get_device_type", return_value="ToRRouter"), \
            mock.patch.object(backend_acl, "load_backend_acl") as load_backend_acl:
        assert backend_acl.main() == 0

    load_backend_acl.assert_not_called()


def test_backend_prerequisite_lookup_failure_is_fatal():
    with mock.patch.object(backend_acl, "_is_storage_device", return_value=None):
        assert backend_acl.load_backend_acl("BackEndToRRouter") is False


def test_backend_acl_load_runs_only_after_all_prerequisites():
    template_file = backend_acl.os.path.join(
        "/", "usr", "share", "sonic", "templates", "backend_acl.j2")
    acl_file = backend_acl.os.path.join("/", "etc", "sonic", "backend_acl.json")

    with mock.patch.object(backend_acl, "_is_storage_device", return_value=True), \
            mock.patch.object(backend_acl, "_is_acl_table_present", return_value=True), \
            mock.patch.object(backend_acl, "_is_switch_table_present", return_value=True), \
            mock.patch.object(backend_acl.os.path, "isfile", return_value=True), \
            mock.patch.object(
                backend_acl,
                "run_command",
                side_effect=[(True, ""), (True, "")]) as run_command:
        assert backend_acl.load_backend_acl("BackEndToRRouter") is True

    assert run_command.call_args_list == [
        mock.call([
            "sudo",
            backend_acl.SONIC_CFGGEN_PATH,
            "-d",
            "-t",
            "{},{}".format(template_file, acl_file)
        ]),
        mock.call([
            "acl-loader",
            "update",
            "full",
            acl_file,
            "--table_name",
            "DATAACL"
        ])
    ]


def test_backend_acl_command_failure_is_fatal():
    with mock.patch.object(backend_acl, "_is_storage_device", return_value=True), \
            mock.patch.object(backend_acl, "_is_acl_table_present", return_value=True), \
            mock.patch.object(backend_acl, "_is_switch_table_present", return_value=True), \
            mock.patch.object(backend_acl.os.path, "isfile", return_value=True), \
            mock.patch.object(backend_acl, "run_command", return_value=(False, "")):
        assert backend_acl.load_backend_acl("BackEndToRRouter") is False
