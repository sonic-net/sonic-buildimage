# tests/test_snmp_yml_to_configdb.py
#
# Regression + security tests for dockers/docker-snmp/snmp_yml_to_configdb.py
# (F082: unsafe yaml.load(Loader=yaml.FullLoader) replaced with
# yaml.safe_load).
#
# The production script connects a real ConfigDBConnector (via swsscommon)
# at import/main() time, and imports sonic_py_common.logger.Logger. Neither
# dependency is installed in this test environment, so both are stubbed out
# with fake modules -- following the same pattern used in
# dockers/docker-telemetry-sidecar/cli-plugin-tests/test_systemd_stub.py --
# before the module under test is imported. The module is loaded via
# importlib so tests exercise the actual production yaml_snmp_info parsing
# call site (load_snmp_yaml -> yaml.safe_load), not a reimplementation of it.

import importlib.util
import os
import sys
import types

import pytest
import yaml

TESTS_DIR = os.path.dirname(os.path.realpath(__file__))
SCRIPT_PATH = os.path.join(TESTS_DIR, "..", "snmp_yml_to_configdb.py")


def _setup_fakes():
    """Create fake swsscommon/sonic_py_common.logger modules before import."""
    swss_pkg = types.ModuleType("swsscommon")
    swss_common_mod = types.ModuleType("swsscommon.swsscommon")

    class _DummyConfigDBConnector:
        def __init__(self, *_, **__):
            pass

        def connect(self, *_, **__):
            pass

        def get_table(self, *_, **__):
            return {}

        def set_entry(self, *_, **__):
            pass

    swss_common_mod.ConfigDBConnector = _DummyConfigDBConnector
    swss_pkg.swsscommon = swss_common_mod
    sys.modules["swsscommon"] = swss_pkg
    sys.modules["swsscommon.swsscommon"] = swss_common_mod

    logger_mod = types.ModuleType("sonic_py_common.logger")

    class _Logger:
        def __init__(self, *_, **__):
            self.messages = []

        def set_min_log_priority_info(self):
            pass

        def _log(self, level, msg):
            self.messages.append((level, msg))

        def log_debug(self, msg):
            self._log("DEBUG", msg)

        def log_info(self, msg):
            self._log("INFO", msg)

        def log_error(self, msg):
            self._log("ERROR", msg)

        def log_notice(self, msg):
            self._log("NOTICE", msg)

        def log_warning(self, msg):
            self._log("WARNING", msg)

        def log_critical(self, msg):
            self._log("CRITICAL", msg)

    logger_mod.Logger = _Logger
    if "sonic_py_common" not in sys.modules:
        sonic_py_common_pkg = types.ModuleType("sonic_py_common")
        sonic_py_common_pkg.__path__ = []
        sys.modules["sonic_py_common"] = sonic_py_common_pkg
    sys.modules["sonic_py_common.logger"] = logger_mod


_setup_fakes()

_spec = importlib.util.spec_from_file_location("snmp_yml_to_configdb", SCRIPT_PATH)
snmp_yml_to_configdb = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(snmp_yml_to_configdb)


class _FakeConfigDB:
    """Minimal in-memory ConfigDBConnector double for asserting set_entry calls."""

    def __init__(self, existing_communities=None, existing_general_keys=None):
        self.set_entry_calls = []
        self._existing_communities = existing_communities or {}
        self._existing_general_keys = existing_general_keys or {}

    def set_entry(self, table, key, data):
        self.set_entry_calls.append((table, key, data))


def _write_yaml(tmp_path, content):
    yml_path = tmp_path / "snmp.yml"
    yml_path.write_text(content)
    return str(yml_path)


class TestLoadSnmpYaml:
    """load_snmp_yaml() is the production call site that used to invoke
    yaml.load(Loader=yaml.FullLoader); it now uses yaml.safe_load."""

    def test_missing_file_returns_none(self, tmp_path):
        missing_path = str(tmp_path / "does-not-exist.yml")
        assert snmp_yml_to_configdb.load_snmp_yaml(missing_path) is None

    def test_valid_scalar_config_parses(self, tmp_path):
        path = _write_yaml(tmp_path, "snmp_rocommunity: public\nsnmp_location: lab1\n")
        result = snmp_yml_to_configdb.load_snmp_yaml(path)
        assert result == {"snmp_rocommunity": "public", "snmp_location": "lab1"}

    def test_valid_list_config_parses(self, tmp_path):
        path = _write_yaml(
            tmp_path,
            "snmp_rocommunities:\n  - public\n  - public2\n"
            "snmp_rwcommunities:\n  - private\n",
        )
        result = snmp_yml_to_configdb.load_snmp_yaml(path)
        assert result["snmp_rocommunities"] == ["public", "public2"]
        assert result["snmp_rwcommunities"] == ["private"]

    def test_unsafe_python_tag_is_rejected(self, tmp_path):
        # Same class of payload flagged by F082: a YAML type tag that would
        # instruct an unsafe loader (yaml.FullLoader) to construct an
        # arbitrary Python object / invoke a callable.
        malicious_yaml = (
            "snmp_rocommunity: !!python/object/apply:os.system [\"id\"]\n"
            "snmp_location: lab1\n"
        )
        path = _write_yaml(tmp_path, malicious_yaml)

        with pytest.raises(yaml.YAMLError):
            snmp_yml_to_configdb.load_snmp_yaml(path)

    def test_unsafe_tag_in_location_is_rejected(self, tmp_path):
        malicious_yaml = (
            "snmp_rocommunity: public\n"
            "snmp_location: !!python/object/apply:os.system [\"id\"]\n"
        )
        path = _write_yaml(tmp_path, malicious_yaml)

        with pytest.raises(yaml.YAMLError):
            snmp_yml_to_configdb.load_snmp_yaml(path)


class TestApplySnmpCommunities:
    def test_scalar_rocommunity_written(self):
        db = _FakeConfigDB()
        yaml_snmp_info = {"snmp_rocommunity": "public"}
        snmp_yml_to_configdb.apply_snmp_communities(db, yaml_snmp_info, {})
        assert db.set_entry_calls == [("SNMP_COMMUNITY", "public", {"TYPE": "RO"})]

    def test_scalar_rwcommunity_written(self):
        db = _FakeConfigDB()
        yaml_snmp_info = {"snmp_rwcommunity": "private"}
        snmp_yml_to_configdb.apply_snmp_communities(db, yaml_snmp_info, {})
        assert db.set_entry_calls == [("SNMP_COMMUNITY", "private", {"TYPE": "RW"})]

    def test_list_rocommunities_written(self):
        db = _FakeConfigDB()
        yaml_snmp_info = {"snmp_rocommunities": ["public", "public2"]}
        snmp_yml_to_configdb.apply_snmp_communities(db, yaml_snmp_info, {})
        assert db.set_entry_calls == [
            ("SNMP_COMMUNITY", "public", {"TYPE": "RO"}),
            ("SNMP_COMMUNITY", "public2", {"TYPE": "RO"}),
        ]

    def test_list_rwcommunities_written(self):
        db = _FakeConfigDB()
        yaml_snmp_info = {"snmp_rwcommunities": ["private", "private2"]}
        snmp_yml_to_configdb.apply_snmp_communities(db, yaml_snmp_info, {})
        assert db.set_entry_calls == [
            ("SNMP_COMMUNITY", "private", {"TYPE": "RW"}),
            ("SNMP_COMMUNITY", "private2", {"TYPE": "RW"}),
        ]

    def test_existing_communities_are_not_rewritten(self):
        db = _FakeConfigDB()
        yaml_snmp_info = {"snmp_rocommunity": "public"}
        snmp_yml_to_configdb.apply_snmp_communities(db, yaml_snmp_info, {"public"})
        assert db.set_entry_calls == []

    def test_no_community_keys_present_is_a_no_op(self):
        db = _FakeConfigDB()
        snmp_yml_to_configdb.apply_snmp_communities(db, {"snmp_location": "lab1"}, {})
        assert db.set_entry_calls == []


class TestApplySnmpLocation:
    def test_location_present_and_not_yet_set(self):
        db = _FakeConfigDB()
        result = snmp_yml_to_configdb.apply_snmp_location(db, {"snmp_location": "lab1"}, {})
        assert result is True
        assert db.set_entry_calls == [("SNMP", "LOCATION", {"Location": "lab1"})]

    def test_location_present_but_already_set(self):
        db = _FakeConfigDB()
        result = snmp_yml_to_configdb.apply_snmp_location(
            db, {"snmp_location": "lab1"}, {"LOCATION"}
        )
        assert result is True
        assert db.set_entry_calls == []

    def test_location_missing_returns_false(self):
        db = _FakeConfigDB()
        result = snmp_yml_to_configdb.apply_snmp_location(db, {}, {})
        assert result is False
        assert db.set_entry_calls == []
