"""
Unit tests for PmonDaemonConfig: the shared resolver for pmon daemon tunables.

Precedence under test (highest wins):
  1. nested subsection keys in the daemon's section of pmon_daemon_control.json
     (hwsku file over platform file)
  2. legacy aliases - deprecated flat section keys and top-level file keys
  3. built-in dataclass defaults

The base is exercised through small test-only subclasses so it is covered
independently of any real daemon's schema.
"""
import json
import os
import sys

from dataclasses import dataclass, field
from typing import Optional

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

import pytest

# pmon_daemon_config imports device_info, which does a top-level
# `from swsscommon.swsscommon import ...`. Real swsscommon bindings are present
# in CI but absent in a plain venv, where their absence would fail collection of
# this whole module (and can take unrelated tests down with it). Stub the module
# with the local mock_swsscommon classes only when the real one is unavailable,
# matching the sibling tests (device_info_test, test_security_cipher).
try:
    import swsscommon.swsscommon  # noqa: F401
except ImportError:
    import types

    from .mock_swsscommon import ConfigDBConnector, SonicV2Connector

    _swss_inner = types.ModuleType("swsscommon.swsscommon")
    _swss_inner.ConfigDBConnector = ConfigDBConnector
    _swss_inner.SonicV2Connector = SonicV2Connector
    _swss_outer = types.ModuleType("swsscommon")
    _swss_outer.swsscommon = _swss_inner
    sys.modules.setdefault("swsscommon", _swss_outer)
    sys.modules.setdefault("swsscommon.swsscommon", _swss_inner)

from sonic_py_common import pmon_daemon_config
from sonic_py_common.pmon_daemon_config import (
    FieldSpec, LegacyAlias, PmonDaemonConfig, PMON_DAEMON_CONTROL_FILE, to_bool)

PATHS_FN = "sonic_py_common.pmon_daemon_config.device_info.get_paths_to_platform_and_hwsku_dirs"

# The quiet_logger fixture below patches get_config_logger for every test; keep a
# handle on the real one so its own caching can still be exercised.
REAL_GET_CONFIG_LOGGER = pmon_daemon_config.get_config_logger


@dataclass
class SampleConfig(PmonDaemonConfig):
    """A stand-in daemon schema covering every FieldSpec feature."""

    SECTION_NAME = 'sampled'
    FIELD_SPECS = {
        'interval': FieldSpec(caster=int, minimum=0, maximum=86400),
        'ratio': FieldSpec(caster=float, minimum=0.0),          # unbounded above
        'enabled': FieldSpec(caster=to_bool),                   # unbounded
        'mode': FieldSpec(choices=('fast', 'slow')),            # no caster
        'freeform': FieldSpec(),                                # neither
    }

    interval: Optional[int] = None
    ratio: Optional[float] = None
    enabled: bool = False
    mode: Optional[str] = None
    freeform: Optional[str] = None


@dataclass
class OtherConfig(PmonDaemonConfig):
    """A second schema, to prove sections do not bleed into each other."""

    SECTION_NAME = 'otherd'
    FIELD_SPECS = {'interval': FieldSpec(caster=int, minimum=0)}

    interval: Optional[int] = None


@dataclass
class ClampedConfig(PmonDaemonConfig):
    """Exercises the _post_merge hook for a cross-field constraint."""

    SECTION_NAME = 'clampedd'
    FIELD_SPECS = {
        'window': FieldSpec(caster=int, minimum=1),
        'heartbeat': FieldSpec(caster=int, minimum=1),
    }

    window: int = 300
    heartbeat: int = 30

    def _post_merge(self):
        if self.heartbeat >= self.window:
            self.heartbeat = max(1, self.window // 2)


@dataclass
class LeafConfig(PmonDaemonConfig):
    """A subsection schema: it declares no SUBSECTIONS of its own."""

    FIELD_SPECS = {
        'interval': FieldSpec(caster=int, minimum=0, maximum=86400),
        'enabled': FieldSpec(caster=to_bool),
    }

    interval: Optional[int] = None
    enabled: Optional[bool] = None


@dataclass
class NestedConfig(PmonDaemonConfig):
    """A schema with one-level subsections and legacy aliases routing into them."""

    SECTION_NAME = 'nestedd'
    SUBSECTIONS = {'group': LeafConfig, 'mgr': LeafConfig}
    LEGACY_ALIASES = {
        'group_interval': LegacyAlias('group.interval'),
        'skip_mgr': LegacyAlias('mgr.enabled', scope='file',
                                transform=lambda v: not to_bool(v)),
    }

    group: LeafConfig = field(default_factory=LeafConfig)
    mgr: LeafConfig = field(default_factory=lambda: LeafConfig(enabled=True))


@pytest.fixture(autouse=True)
def quiet_logger():
    """Keep tests off rsyslogd; the unit test environment has none."""
    with mock.patch.object(pmon_daemon_config, 'get_config_logger',
                           return_value=mock.MagicMock()) as logger_fn:
        yield logger_fn


def write_control_file(directory, payload):
    """Write a pmon_daemon_control.json with the given dict into directory."""
    os.makedirs(directory, exist_ok=True)
    path = os.path.join(directory, PMON_DAEMON_CONTROL_FILE)
    with open(path, "w") as f:
        json.dump(payload, f)
    return path


class TestToBool:
    @pytest.mark.parametrize("value", [True, "true", "True", " TRUE ", "yes", "on", "1", 1])
    def test_truthy_spellings(self, value):
        assert to_bool(value) is True

    @pytest.mark.parametrize("value", [False, "false", "False", " FALSE ", "no", "off", "0", 0])
    def test_falsy_spellings(self, value):
        # The case bool() gets wrong: bool("false") is True.
        assert to_bool(value) is False

    @pytest.mark.parametrize("value", ["maybe", "", 2, -1, 1.5, None, [], {}])
    def test_uninterpretable_raises(self, value):
        with pytest.raises(ValueError):
            to_bool(value)


class TestFieldSpec:
    def test_no_caster_passes_value_through(self):
        assert FieldSpec().coerce("as-is") == "as-is"

    def test_caster_failure_propagates(self):
        with pytest.raises(ValueError):
            FieldSpec(caster=int).coerce("not-a-number")

    def test_unbounded_spec_accepts_anything(self):
        assert FieldSpec(caster=int).rejection_reason(-999999) is None

    def test_bounds_are_inclusive(self):
        spec = FieldSpec(caster=int, minimum=0, maximum=10)
        assert spec.rejection_reason(0) is None
        assert spec.rejection_reason(10) is None
        assert spec.rejection_reason(-1) is not None
        assert spec.rejection_reason(11) is not None

    def test_incomparable_value_is_rejected_not_raised(self):
        # A dict where an int was declared must not blow up the comparison.
        assert FieldSpec(minimum=0).rejection_reason({}) is not None

    def test_choices_enforced(self):
        spec = FieldSpec(choices=('fast', 'slow'))
        assert spec.rejection_reason('fast') is None
        assert spec.rejection_reason('turbo') is not None

    def test_describe_range_reports_open_bounds(self):
        assert FieldSpec(minimum=0).describe_range() == "[0, +inf]"
        assert FieldSpec(maximum=9).describe_range() == "[-inf, 9]"


class TestDefaults:
    def test_defaults_when_no_overrides(self):
        cfg = SampleConfig.resolve(platform_section={})
        assert cfg.interval is None
        assert cfg.ratio is None
        assert cfg.enabled is False
        assert cfg.mode is None

    def test_bare_construction_matches_defaults(self):
        assert SampleConfig() == SampleConfig.resolve(platform_section={})


class TestMerge:
    def test_section_overrides_defaults(self):
        cfg = SampleConfig.resolve(platform_section={"interval": 5, "mode": "fast"})
        assert cfg.interval == 5
        assert cfg.mode == "fast"

    def test_partial_section_leaves_other_fields_at_default(self):
        cfg = SampleConfig.resolve(platform_section={"interval": 5})
        assert cfg.interval == 5
        assert cfg.ratio is None

    def test_none_value_does_not_override(self):
        cfg = SampleConfig.resolve(platform_section={"enabled": None})
        assert cfg.enabled is False

    def test_string_value_is_coerced(self):
        cfg = SampleConfig.resolve(platform_section={"interval": "30"})
        assert cfg.interval == 30
        assert isinstance(cfg.interval, int)

    def test_uncoercible_value_keeps_default(self):
        cfg = SampleConfig.resolve(platform_section={"interval": "not-a-number"})
        assert cfg.interval is None

    def test_unknown_key_is_ignored(self):
        cfg = SampleConfig.resolve(platform_section={
            "interval": 5, "some_future_unknown_key": 99})
        assert cfg.interval == 5
        assert not hasattr(cfg, "some_future_unknown_key")

    def test_field_without_spec_is_stored_as_is(self):
        cfg = SampleConfig.resolve(platform_section={"freeform": {"anything": 1}})
        assert cfg.freeform == {"anything": 1}

    def test_bool_string_false_disables(self):
        # Without to_bool this would store the truthy string "false".
        cfg = SampleConfig.resolve(platform_section={"enabled": "false"})
        assert cfg.enabled is False

    def test_bool_string_true_enables(self):
        cfg = SampleConfig.resolve(platform_section={"enabled": "true"})
        assert cfg.enabled is True


class TestRangeValidation:
    def test_below_minimum_keeps_default(self):
        cfg = SampleConfig.resolve(platform_section={"interval": -1})
        assert cfg.interval is None

    def test_above_maximum_keeps_default(self):
        cfg = SampleConfig.resolve(platform_section={"interval": 86401})
        assert cfg.interval is None

    def test_boundaries_are_accepted(self):
        assert SampleConfig.resolve(platform_section={"interval": 0}).interval == 0
        assert SampleConfig.resolve(platform_section={"interval": 86400}).interval == 86400

    def test_validation_runs_after_coercion(self):
        # "-5" coerces to int fine, then fails the range check.
        cfg = SampleConfig.resolve(platform_section={"interval": "-5"})
        assert cfg.interval is None

    def test_unbounded_above_accepts_large_value(self):
        cfg = SampleConfig.resolve(platform_section={"ratio": "1e9"})
        assert cfg.ratio == 1e9

    def test_choices_violation_keeps_default(self):
        cfg = SampleConfig.resolve(platform_section={"mode": "turbo"})
        assert cfg.mode is None

    def test_rejection_is_logged_as_warning(self):
        logger = mock.MagicMock()
        with mock.patch.object(pmon_daemon_config, 'get_config_logger', return_value=logger):
            SampleConfig.resolve(platform_section={"interval": -1})
        assert logger.log_warning.called
        message = logger.log_warning.call_args[0][0]
        assert "interval" in message and "keeping default" in message

    def test_one_bad_field_does_not_drop_the_others(self):
        cfg = SampleConfig.resolve(platform_section={"interval": -1, "mode": "slow"})
        assert cfg.interval is None
        assert cfg.mode == "slow"


class TestPostMergeHook:
    def test_hook_is_a_noop_by_default(self):
        assert SampleConfig.resolve(platform_section={"interval": 5}).interval == 5

    def test_cross_field_constraint_applied_after_merge(self):
        cfg = ClampedConfig.resolve(platform_section={"window": 20, "heartbeat": 50})
        assert cfg.heartbeat == 10

    def test_constraint_sees_defaults_too(self):
        # Lowering only window must still pull the default heartbeat under it.
        cfg = ClampedConfig.resolve(platform_section={"window": 10})
        assert cfg.heartbeat == 5

    def test_in_range_values_untouched(self):
        cfg = ClampedConfig.resolve(platform_section={"window": 100, "heartbeat": 40})
        assert cfg.heartbeat == 40


class TestSubsections:
    def test_nested_dict_merges_into_subsection(self):
        cfg = NestedConfig.resolve(platform_section={"group": {"interval": 5, "enabled": "true"}})
        assert cfg.group.interval == 5
        assert cfg.group.enabled is True

    def test_partial_nested_dict_keeps_sibling_defaults(self):
        cfg = NestedConfig.resolve(platform_section={"group": {"interval": 5}})
        assert cfg.group.interval == 5
        assert cfg.group.enabled is None

    def test_subsection_default_factory_is_preserved(self):
        # mgr's default_factory sets enabled=True; an untouched subsection keeps it.
        cfg = NestedConfig.resolve(platform_section={})
        assert cfg.mgr.enabled is True
        assert cfg.group.enabled is None

    def test_subsection_range_validation_applies(self):
        cfg = NestedConfig.resolve(platform_section={"group": {"interval": -1}})
        assert cfg.group.interval is None

    def test_non_dict_subsection_keeps_defaults(self):
        cfg = NestedConfig.resolve(platform_section={"mgr": "oops-not-an-object"})
        assert cfg.mgr.enabled is True

    def test_unknown_key_in_subsection_is_ignored(self):
        cfg = NestedConfig.resolve(platform_section={"group": {"interval": 5, "bogus": 1}})
        assert cfg.group.interval == 5
        assert not hasattr(cfg.group, "bogus")


class TestLegacyAliases:
    def test_section_alias_routes_into_subsection(self):
        cfg = NestedConfig.resolve(platform_section={"group_interval": 5})
        assert cfg.group.interval == 5

    def test_file_alias_reads_top_level_and_transforms(self):
        # skip_mgr lives at the file top level, not in the daemon section, and
        # inverts into mgr.enabled.
        cfg = NestedConfig.resolve(platform_section={}, platform_file={"skip_mgr": True})
        assert cfg.mgr.enabled is False

    def test_nested_form_wins_over_section_alias(self):
        cfg = NestedConfig.resolve(platform_section={
            "group_interval": 5, "group": {"interval": 9}})
        assert cfg.group.interval == 9

    def test_nested_form_wins_over_file_alias(self):
        cfg = NestedConfig.resolve(platform_section={"mgr": {"enabled": True}},
                                   platform_file={"skip_mgr": True})
        assert cfg.mgr.enabled is True

    def test_section_alias_out_of_range_keeps_default(self):
        cfg = NestedConfig.resolve(platform_section={"group_interval": -1})
        assert cfg.group.interval is None

    def test_consumed_alias_key_is_not_reported_as_unknown(self):
        cfg = NestedConfig.resolve(platform_section={"group_interval": 5})
        assert cfg.group.interval == 5
        assert not hasattr(cfg, "group_interval")

    def test_using_an_alias_logs_a_deprecation_warning(self):
        logger = mock.MagicMock()
        with mock.patch.object(pmon_daemon_config, 'get_config_logger', return_value=logger):
            NestedConfig.resolve(platform_section={"group_interval": 5})
        assert logger.log_warning.called
        assert any("deprecated" in c[0][0] for c in logger.log_warning.call_args_list)


class TestReadPlatformSection:
    def test_missing_files_yield_empty(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        hwsku_dir = str(tmp_path / "hwsku")
        with mock.patch(PATHS_FN, return_value=(platform_dir, hwsku_dir)):
            assert SampleConfig._read_platform_section() == {}

    def test_reads_platform_file_when_no_hwsku_file(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        hwsku_dir = str(tmp_path / "hwsku")
        write_control_file(platform_dir, {"sampled": {"interval": 30}})
        with mock.patch(PATHS_FN, return_value=(platform_dir, hwsku_dir)):
            assert SampleConfig._read_platform_section() == {"interval": 30}

    def test_hwsku_file_takes_precedence_over_platform_file(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        hwsku_dir = str(tmp_path / "hwsku")
        write_control_file(platform_dir, {"sampled": {"interval": 30}})
        write_control_file(hwsku_dir, {"sampled": {"interval": 99}})
        with mock.patch(PATHS_FN, return_value=(platform_dir, hwsku_dir)):
            # Mirrors docker_init: the hwsku file wins; no cross-file merge.
            assert SampleConfig._read_platform_section() == {"interval": 99}

    def test_hwsku_file_without_section_does_not_fall_back(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        hwsku_dir = str(tmp_path / "hwsku")
        write_control_file(platform_dir, {"sampled": {"interval": 30}})
        write_control_file(hwsku_dir, {"skip_sampled": False})
        with mock.patch(PATHS_FN, return_value=(platform_dir, hwsku_dir)):
            assert SampleConfig._read_platform_section() == {}

    def test_each_subclass_reads_its_own_section(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        write_control_file(platform_dir, {
            "sampled": {"interval": 30},
            "otherd": {"interval": 99},
        })
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            assert SampleConfig.resolve().interval == 30
            assert OtherConfig.resolve().interval == 99

    def test_no_section_yields_empty(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        write_control_file(platform_dir, {"skip_ledd": True})
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            assert SampleConfig._read_platform_section() == {}

    def test_malformed_json_yields_empty(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        os.makedirs(platform_dir)
        with open(os.path.join(platform_dir, PMON_DAEMON_CONTROL_FILE), "w") as f:
            f.write("{ this is not valid json")
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            assert SampleConfig._read_platform_section() == {}

    def test_non_dict_section_yields_empty(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        write_control_file(platform_dir, {"sampled": "oops-not-an-object"})
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            assert SampleConfig._read_platform_section() == {}

    def test_device_info_failure_yields_empty(self):
        with mock.patch(PATHS_FN, side_effect=RuntimeError("platform undetermined")):
            assert SampleConfig._read_platform_section() == {}

    def test_empty_dir_path_is_skipped(self, tmp_path):
        # get_paths_to_platform_and_hwsku_dirs may return an empty hwsku path;
        # that entry is skipped rather than joined into a bogus path.
        platform_dir = str(tmp_path / "platform")
        write_control_file(platform_dir, {"sampled": {"interval": 30}})
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            assert SampleConfig._read_platform_section() == {"interval": 30}

    def test_read_platform_control_returns_section_and_whole_file(self, tmp_path):
        # A scope='file' alias needs the sibling top-level keys, so the control
        # read returns the whole file alongside the daemon's own section.
        platform_dir = str(tmp_path / "platform")
        write_control_file(platform_dir, {"skip_mgr": True, "nestedd": {"group": {"interval": 5}}})
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            section, whole = NestedConfig._read_platform_control()
        assert section == {"group": {"interval": 5}}
        assert whole["skip_mgr"] is True

    def test_read_platform_control_whole_file_available_without_section(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        write_control_file(platform_dir, {"skip_mgr": True})
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            section, whole = NestedConfig._read_platform_control()
        assert section == {}
        assert whole["skip_mgr"] is True


class TestResolveEndToEnd:
    def test_resolve_reads_from_disk(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        write_control_file(platform_dir, {"sampled": {"interval": 5, "enabled": "true"}})
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            cfg = SampleConfig.resolve()
        assert cfg.interval == 5
        assert cfg.enabled is True

    def test_resolve_defaults_when_nothing_on_disk(self, tmp_path):
        with mock.patch(PATHS_FN, return_value=(str(tmp_path / "platform"), "")):
            cfg = SampleConfig.resolve()
        assert cfg.interval is None
        assert cfg.enabled is False

    def test_resolve_applies_file_scope_alias_from_disk(self, tmp_path):
        platform_dir = str(tmp_path / "platform")
        write_control_file(platform_dir, {"skip_mgr": True, "nestedd": {}})
        with mock.patch(PATHS_FN, return_value=(platform_dir, "")):
            cfg = NestedConfig.resolve()
        assert cfg.mgr.enabled is False


class TestSchemaGuard:
    """A spec keyed by the wrong name is inert, so the schema must not compile.

    These are developer errors in static code, caught at import; bad values in
    the platform file stay non-fatal.
    """

    def test_spec_for_nonexistent_field_is_rejected(self):
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class Typo(PmonDaemonConfig):
                SECTION_NAME = 'typod'
                FIELD_SPECS = {'intervl': FieldSpec(caster=int, minimum=0)}

                interval: Optional[int] = None

        assert 'intervl' in str(excinfo.value)

    def test_field_without_a_spec_is_rejected(self):
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class Unspecced(PmonDaemonConfig):
                SECTION_NAME = 'unspeccedd'
                FIELD_SPECS = {'covered': FieldSpec(caster=int)}

                covered: Optional[int] = None
                forgotten: Optional[int] = None

        assert 'forgotten' in str(excinfo.value)

    def test_explicitly_unbounded_field_is_accepted(self):
        @dataclass
        class Unbounded(PmonDaemonConfig):
            SECTION_NAME = 'unboundedd'
            FIELD_SPECS = {'anything': FieldSpec()}

            anything: Optional[str] = None

        assert Unbounded.resolve(platform_section={"anything": "x"}).anything == "x"

    def test_classvars_are_not_treated_as_tunables(self):
        # SECTION_NAME/FIELD_SPECS/SYSLOG_IDENTIFIER are ClassVars on the base;
        # a schema declaring no fields at all must still be legal.
        @dataclass
        class NoTunables(PmonDaemonConfig):
            SECTION_NAME = 'notunablesd'

        assert NoTunables._declared_tunables() == set()

    def test_inherited_fields_need_inherited_specs(self):
        # Extending a schema: the new field needs its own spec, the inherited
        # ones are already covered.
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class Extended(SampleConfig):
                SECTION_NAME = 'extendedd'

                extra: Optional[int] = None

        assert 'extra' in str(excinfo.value)

    def test_extending_a_schema_with_a_spec_is_accepted(self):
        @dataclass
        class Extended(SampleConfig):
            SECTION_NAME = 'extended2d'
            FIELD_SPECS = dict(SampleConfig.FIELD_SPECS,
                               extra=FieldSpec(caster=int, minimum=0))

            extra: Optional[int] = None

        cfg = Extended.resolve(platform_section={"extra": "7", "interval": 5})
        assert cfg.extra == 7
        assert cfg.interval == 5

    def test_field_in_both_field_specs_and_subsections_is_rejected(self):
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class Both(PmonDaemonConfig):
                SECTION_NAME = 'bothd'
                FIELD_SPECS = {'group': FieldSpec()}
                SUBSECTIONS = {'group': LeafConfig}

                group: Optional[LeafConfig] = None

        assert 'group' in str(excinfo.value)

    def test_subsection_naming_no_field_is_rejected(self):
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class Ghost(PmonDaemonConfig):
                SECTION_NAME = 'ghostd'
                SUBSECTIONS = {'ghost': LeafConfig}

        assert 'ghost' in str(excinfo.value)

    def test_field_that_is_neither_specced_nor_subsectioned_is_rejected(self):
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class Uncovered(PmonDaemonConfig):
                SECTION_NAME = 'uncoveredd'

                group: Optional[LeafConfig] = None

        assert 'group' in str(excinfo.value)

    def test_two_level_nesting_is_rejected(self):
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class TwoLevel(PmonDaemonConfig):
                SECTION_NAME = 'twoleveld'
                SUBSECTIONS = {'outer': NestedConfig}  # NestedConfig has SUBSECTIONS

                outer: Optional[NestedConfig] = None

        assert 'one level' in str(excinfo.value)

    def test_alias_target_naming_no_field_is_rejected(self):
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class BadAlias(PmonDaemonConfig):
                SECTION_NAME = 'badaliasd'
                SUBSECTIONS = {'group': LeafConfig}
                LEGACY_ALIASES = {'x': LegacyAlias('group.nope')}

                group: LeafConfig = field(default_factory=LeafConfig)

        assert 'group.nope' in str(excinfo.value)

    def test_alias_target_naming_no_subsection_is_rejected(self):
        with pytest.raises(TypeError) as excinfo:
            @dataclass
            class BadAlias(PmonDaemonConfig):
                SECTION_NAME = 'badalias2d'
                LEGACY_ALIASES = {'x': LegacyAlias('missing.field')}

        assert 'missing.field' in str(excinfo.value)


class TestLoggerIdentifier:
    def test_identifier_defaults_to_section_name(self, quiet_logger):
        SampleConfig._logger()
        quiet_logger.assert_called_with('sampled_config')

    def test_explicit_identifier_wins(self, quiet_logger):
        @dataclass
        class Named(PmonDaemonConfig):
            SECTION_NAME = 'named'
            SYSLOG_IDENTIFIER = 'custom_identifier'

        Named._logger()
        quiet_logger.assert_called_with('custom_identifier')

    def test_one_logger_instance_per_identifier(self):
        # Real factory: repeated lookups must not build a new SysLogger each time.
        with mock.patch.object(pmon_daemon_config, 'SysLogger') as syslogger_cls:
            pmon_daemon_config._LOGGERS.clear()
            try:
                first = REAL_GET_CONFIG_LOGGER('dedupe_test')
                second = REAL_GET_CONFIG_LOGGER('dedupe_test')
            finally:
                pmon_daemon_config._LOGGERS.clear()
        assert first is second
        assert syslogger_cls.call_count == 1
