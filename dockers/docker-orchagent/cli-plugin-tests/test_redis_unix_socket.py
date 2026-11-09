import importlib.util
import re
import sys
import types
from pathlib import Path


TEST_DIR = Path(__file__).resolve().parent
DOCKER_DIR = TEST_DIR.parent
REPO_ROOT = TEST_DIR.parents[2]

_UNSET = object()


def _load_module(name, path, fake_modules):
    """Load a production module with temporary dependency stubs."""
    saved_modules = {
        module_name: sys.modules.get(module_name, _UNSET)
        for module_name in fake_modules
    }
    sys.modules.update(fake_modules)

    try:
        spec = importlib.util.spec_from_file_location(name, path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    finally:
        for module_name, saved_module in saved_modules.items():
            if saved_module is _UNSET:
                sys.modules.pop(module_name, None)
            else:
                sys.modules[module_name] = saved_module


def test_enable_counters_uses_unix_socket():
    constructor_calls = []
    connect_calls = []

    class FakeSonicV2Connector:
        def __init__(self, *args, **kwargs):
            constructor_calls.append((args, kwargs))

        def connect(self, *args):
            connect_calls.append(args)

        def set(self, *args):
            pass

    swss_package = types.ModuleType("swsscommon")
    swss_module = types.ModuleType("swsscommon.swsscommon")
    swss_module.SonicV2Connector = FakeSonicV2Connector
    swss_package.swsscommon = swss_module

    enable_counters = _load_module(
        "orchagent_enable_counters_test",
        DOCKER_DIR / "enable_counters.py",
        {
            "swsscommon": swss_package,
            "swsscommon.swsscommon": swss_module,
        },
    )

    enable_counters.enable_rates()

    assert constructor_calls == [
        ((), {"use_unix_socket_path": True}),
    ]
    assert connect_calls == [("COUNTERS_DB",)]


def test_tunnel_packet_handler_uses_unix_socket():
    constructor_calls = []
    connect_calls = []

    class FakeConfigDBConnector:
        def __init__(self, *args, **kwargs):
            constructor_calls.append(("ConfigDBConnector", args, kwargs))

        def connect(self, *args):
            connect_calls.append(("CONFIG_DB", args))

        def get_keys(self, table):
            return []

    class FakeSonicV2Connector:
        def __init__(self, *args, **kwargs):
            constructor_calls.append(("SonicV2Connector", args, kwargs))

        def connect(self, *args):
            connect_calls.append(("SonicV2Connector", args))

        def get_db_separator(self, db_name):
            return ":"

    class FakeLogger:
        def log_info(self, message):
            pass

        def log_notice(self, message):
            pass

        def log_warning(self, message):
            pass

        def log_error(self, message):
            pass

    class FakeSelect:
        TIMEOUT = 0
        ERROR = -1

    class FakeNetlinkError(Exception):
        pass

    swss_package = types.ModuleType("swsscommon")
    swss_module = types.ModuleType("swsscommon.swsscommon")
    swss_module.ConfigDBConnector = FakeConfigDBConnector
    swss_module.SonicV2Connector = FakeSonicV2Connector
    swss_module.DBConnector = object
    swss_module.Select = FakeSelect
    swss_module.SubscriberStateTable = object
    swss_package.swsscommon = swss_module

    logger_module = types.ModuleType("sonic_py_common.logger")
    logger_module.Logger = FakeLogger
    sonic_package = types.ModuleType("sonic_py_common")
    sonic_package.logger = logger_module

    pyroute2_package = types.ModuleType("pyroute2")
    pyroute2_package.IPRoute = object
    netlink_package = types.ModuleType("pyroute2.netlink")
    netlink_exceptions = types.ModuleType("pyroute2.netlink.exceptions")
    netlink_exceptions.NetlinkError = FakeNetlinkError

    scapy_package = types.ModuleType("scapy")
    scapy_layers = types.ModuleType("scapy.layers")
    scapy_inet = types.ModuleType("scapy.layers.inet")
    scapy_inet6 = types.ModuleType("scapy.layers.inet6")
    scapy_sendrecv = types.ModuleType("scapy.sendrecv")
    scapy_inet.IP = type("IP", (), {})
    scapy_inet6.IPv6 = type("IPv6", (), {})
    scapy_sendrecv.AsyncSniffer = object

    tunnel_packet_handler = _load_module(
        "orchagent_tunnel_packet_handler_test",
        DOCKER_DIR / "tunnel_packet_handler.py",
        {
            "swsscommon": swss_package,
            "swsscommon.swsscommon": swss_module,
            "sonic_py_common": sonic_package,
            "sonic_py_common.logger": logger_module,
            "pyroute2": pyroute2_package,
            "pyroute2.netlink": netlink_package,
            "pyroute2.netlink.exceptions": netlink_exceptions,
            "scapy": scapy_package,
            "scapy.layers": scapy_layers,
            "scapy.layers.inet": scapy_inet,
            "scapy.layers.inet6": scapy_inet6,
            "scapy.sendrecv": scapy_sendrecv,
        },
    )

    tunnel_packet_handler.TunnelPacketHandler()

    assert constructor_calls == [
        ("ConfigDBConnector", (), {"use_unix_socket_path": True}),
        ("SonicV2Connector", (), {"use_unix_socket_path": True}),
        ("SonicV2Connector", (), {"use_unix_socket_path": True}),
    ]
    assert connect_calls == [
        ("CONFIG_DB", ()),
        ("SonicV2Connector", ("STATE_DB",)),
        ("SonicV2Connector", ("COUNTERS_DB",)),
    ]


CLI_CALL_PATTERN = re.compile(
    r"\bsonic-db-cli(?P<socket>\s+-s)?\s+(?P<db>[A-Z][A-Z0-9_]*_DB)"
)

LOCAL_CLI_CALLS = {
    DOCKER_DIR / "buffermgrd.sh": ["CONFIG_DB"],
    DOCKER_DIR / "orchagent.sh": ["CONFIG_DB"] * 7,
    DOCKER_DIR / "swssconfig.sh": ["STATE_DB"] * 2,
    DOCKER_DIR / "wait_for_link.sh.j2": ["STATE_DB"],
    DOCKER_DIR / "docker-init.j2": ["CONFIG_DB"] * 3,
    REPO_ROOT / "files/scripts/arp_update": [
        "APPL_DB",
        "ASIC_DB",
        "APPL_DB",
        "CONFIG_DB",
        "APPL_DB",
        "CONFIG_DB",
    ],
}


def _find_cli_calls(path):
    return [
        (match.group("db"), match.group("socket") is not None)
        for match in CLI_CALL_PATTERN.finditer(path.read_text())
    ]


def test_all_local_shell_clients_request_unix_socket():
    chassis_calls = []

    for path, expected_databases in LOCAL_CLI_CALLS.items():
        calls = _find_cli_calls(path)
        chassis_calls.extend(
            call for call in calls if call[0] == "CHASSIS_STATE_DB"
        )
        local_calls = [
            call for call in calls if call[0] != "CHASSIS_STATE_DB"
        ]

        assert [database for database, _ in local_calls] == expected_databases
        assert all(use_socket for _, use_socket in local_calls)

        # Do not regress to the raw redis-cli TCP default.
        assert "redis-cli" not in path.read_text().replace("sonic-db-cli", "")

    # CHASSIS_STATE_DB can be remote. Its endpoint policy is owned separately
    # and is deliberately excluded from this local-client migration.
    assert len(chassis_calls) == 1
