"""
Shared resolver for pmon daemon runtime tunables.

A daemon declares a dataclass subclass of PmonDaemonConfig that names the
section it owns in pmon_daemon_control.json and the fields it accepts. The base
locates the file, extracts that section, and layers it over the built-in field
defaults, coercing and range-checking each value.

Schema options on a subclass:
  - FIELD_SPECS     per-field type coercion and valid range (see FieldSpec).
  - SUBSECTIONS     group related fields one level deep, each a nested
                    PmonDaemonConfig subclass with its own FIELD_SPECS.
  - LEGACY_ALIASES  route a deprecated key to its new (possibly nested) target
                    for a compatibility window; an explicit nested value wins.

Precedence, highest first:
  1. The daemon's section of pmon_daemon_control.json.
  2. The subclass's built-in field defaults.

The file is read from the same device directories, with the same
hwsku-over-platform precedence, that docker_init.j2 uses.

Nothing here raises on bad input: an unreadable file, a malformed section, an
uncoercible value, or an out-of-range value degrades to the built-in default
with a syslog warning, so a bad tunable can never keep a pmon daemon down.

Example:

    @dataclass
    class DomConfig(PmonDaemonConfig):
        SECTION_NAME = 'dom'
        FIELD_SPECS = {
            'update_interval': FieldSpec(caster=int, minimum=0, maximum=86400),
        }
        update_interval: Optional[int] = None

    @dataclass
    class XcvrdConfig(PmonDaemonConfig):
        SECTION_NAME = 'xcvrd'
        SUBSECTIONS = {'dom': DomConfig}
        dom: DomConfig = field(default_factory=DomConfig)

    config = XcvrdConfig.resolve()   # -> config.dom.update_interval
"""

import copy
import json
import os

from dataclasses import dataclass, fields
from typing import Callable, ClassVar, Dict, Optional, Tuple, Type, get_origin

from . import device_info
from .syslogger import SysLogger

# Per-platform / per-hwsku file. Each daemon's tunables live under its own key,
# alongside the top-level skip_<daemon> / delay_<daemon> capability keys.
PMON_DAEMON_CONTROL_FILE = "pmon_daemon_control.json"

# One SysLogger per identifier: a daemon's config schema module and the base
# class both log under the same identifier and should share one instance.
_LOGGERS = {}


def get_config_logger(identifier):
    """Return the SysLogger for identifier, creating it on first use."""
    logger = _LOGGERS.get(identifier)
    if logger is None:
        logger = SysLogger(identifier, enable_runtime_config=True)
        _LOGGERS[identifier] = logger
    return logger


def to_bool(value):
    """Coerce a platform-file value to a bool.

    Boolean tunables cannot use bool() as their caster: bool("false") is True,
    so a platform writing the string "false" would silently enable the feature.
    Accepts real booleans, 0/1, and the usual textual spellings; anything else
    raises ValueError so the caller keeps the built-in default.
    """
    if isinstance(value, bool):
        return value
    if isinstance(value, int):
        if value in (0, 1):
            return bool(value)
        raise ValueError("cannot interpret {!r} as a boolean".format(value))
    if isinstance(value, str):
        lowered = value.strip().lower()
        if lowered in ('true', 'yes', 'on', '1'):
            return True
        if lowered in ('false', 'no', 'off', '0'):
            return False
    raise ValueError("cannot interpret {!r} as a boolean".format(value))


def _is_class_var(annotation):
    """True if an annotation declares class-level config rather than a tunable."""
    if annotation is ClassVar or get_origin(annotation) is ClassVar:
        return True
    # PEP 563 string annotations never reach get_origin.
    if isinstance(annotation, str):
        return annotation.split('[')[0].split('.')[-1] == 'ClassVar'
    return False


@dataclass(frozen=True)
class FieldSpec:
    """Type coercion and validation policy for one tunable.

    caster  - applied first, e.g. int / float / to_bool. A TypeError or
              ValueError from it rejects the value.
    minimum - inclusive lower bound, or None for unbounded below.
    maximum - inclusive upper bound, or None for unbounded above.
    choices - permitted values, or None if not an enumeration.

    Coercion alone is not validation: a negative interval coerces to a perfectly
    good int but is not a valid cadence, and downstream consumers differ in
    whether they notice. Declaring the range here gives every tunable one
    enforced, testable contract. A field that is genuinely unbounded declares
    FieldSpec(caster=...) with no bounds, so "unbounded" stays an explicit
    choice rather than an omission.
    """

    caster: Optional[Callable] = None
    minimum: Optional[float] = None
    maximum: Optional[float] = None
    choices: Optional[Tuple] = None

    def coerce(self, value):
        """Apply caster. Raises TypeError/ValueError if the value is unusable."""
        if self.caster is None:
            return value
        return self.caster(value)

    def describe_range(self):
        """Human-readable bounds, for log messages."""
        if self.choices is not None:
            return "one of {}".format(sorted(self.choices, key=repr))
        low = "-inf" if self.minimum is None else self.minimum
        high = "+inf" if self.maximum is None else self.maximum
        return "[{}, {}]".format(low, high)

    def rejection_reason(self, value):
        """Return None if value is acceptable, else why it is not."""
        if self.choices is not None and value not in self.choices:
            return "expected {}".format(self.describe_range())
        if self.minimum is None and self.maximum is None:
            return None
        try:
            below = self.minimum is not None and value < self.minimum
            above = self.maximum is not None and value > self.maximum
        except TypeError:
            # Not comparable to the bounds at all (e.g. a dict where an int was
            # declared) - treat as out of range rather than raising.
            return "expected a value in {}".format(self.describe_range())
        if below or above:
            return "expected a value in {}".format(self.describe_range())
        return None


@dataclass(frozen=True)
class LegacyAlias:
    """Maps a deprecated key to a field on the new (possibly nested) schema.

    A schema keeps working for platform files that still use an old flat key or
    an old top-level key after the tunable has moved under a subsection. The
    alias is applied before the nested form is merged and never overwrites an
    explicit nested value, so the nested key always wins when both are present.

    target    - dotted path to the destination field, e.g. "cmis_mgr.enabled"
                (at most one level: "<field>" or "<subsection>.<field>").
    scope     - 'section' reads the deprecated key from the daemon's own section;
                'file' reads it from the top level of pmon_daemon_control.json
                (a sibling of the daemon section).
    transform - optional rewrite of the raw value before it is routed to target,
                e.g. inverting a legacy skip_* flag into an enabled toggle. A
                TypeError/ValueError from it drops the value (default kept).
    """

    target: str
    scope: str = 'section'
    transform: Optional[Callable] = None


@dataclass
class PmonDaemonConfig:
    """Base class for a pmon daemon's resolved configuration.

    Subclasses are dataclasses that set SECTION_NAME, populate FIELD_SPECS, and
    declare one field per tunable whose default is the built-in default. The
    base carries no fields of its own, so subclass field ordering is unaffected.
    """

    # Key in pmon_daemon_control.json that holds this daemon's tunables.
    SECTION_NAME: ClassVar[str] = ''
    # field name -> FieldSpec. A field with no entry is stored as-is.
    FIELD_SPECS: ClassVar[Dict[str, FieldSpec]] = {}
    # Defaults to "<section>_config" when unset.
    SYSLOG_IDENTIFIER: ClassVar[Optional[str]] = None
    # field name -> subsection schema (a PmonDaemonConfig subclass). Subsections
    # are exactly one level deep: a subsection schema may not declare its own.
    SUBSECTIONS: ClassVar[Dict[str, Type['PmonDaemonConfig']]] = {}
    # deprecated key -> LegacyAlias describing where its value now lives.
    LEGACY_ALIASES: ClassVar[Dict[str, 'LegacyAlias']] = {}

    def __init_subclass__(cls, **kwargs):
        """Reject a schema whose fields and FIELD_SPECS disagree.

        FIELD_SPECS is keyed by field name, so a key matching no field is
        silently ignored - and that field then gets neither coercion nor a range
        check, which is precisely the failure the specs exist to prevent. A field
        with no spec is likewise stored exactly as the platform wrote it.

        Both are static errors in the schema, not bad input, so they raise here
        and surface the first time the module is imported. Bad values in
        pmon_daemon_control.json stay non-fatal; see _merge.

        Runs before @dataclass processes the subclass, so fields are read from
        annotations rather than dataclasses.fields().
        """
        super().__init_subclass__(**kwargs)
        declared = cls._declared_tunables()
        specced = set(cls.FIELD_SPECS)
        subsectioned = set(cls.SUBSECTIONS)

        unknown = sorted(specced - declared)
        if unknown:
            raise TypeError(
                "{}.FIELD_SPECS has no matching field for: {}. A spec keyed by a "
                "name no field declares is never applied, leaving that tunable "
                "with no type coercion and no range check.".format(
                    cls.__name__, ", ".join(unknown)))

        unknown_subs = sorted(subsectioned - declared)
        if unknown_subs:
            raise TypeError(
                "{}.SUBSECTIONS names field(s) that do not exist: {}.".format(
                    cls.__name__, ", ".join(unknown_subs)))

        both = sorted(specced & subsectioned)
        if both:
            raise TypeError(
                "{}: field(s) {} appear in both FIELD_SPECS and SUBSECTIONS; a "
                "field is either a scalar tunable or a nested group, not both.".format(
                    cls.__name__, ", ".join(both)))

        uncovered = sorted(declared - specced - subsectioned)
        if uncovered:
            raise TypeError(
                "{}.FIELD_SPECS is missing an entry for: {}. Every tunable "
                "declares its coercion and bounds (or is a SUBSECTIONS group); a "
                "field that is genuinely unbounded declares FieldSpec() so that "
                "stays an explicit choice.".format(
                    cls.__name__, ", ".join(uncovered)))

        for name, subcls in cls.SUBSECTIONS.items():
            if not (isinstance(subcls, type) and issubclass(subcls, PmonDaemonConfig)):
                raise TypeError(
                    "{}.SUBSECTIONS['{}'] must be a PmonDaemonConfig subclass.".format(
                        cls.__name__, name))
            if subcls.SUBSECTIONS:
                raise TypeError(
                    "{}.SUBSECTIONS['{}'] ({}) declares its own SUBSECTIONS; "
                    "subsections are exactly one level deep.".format(
                        cls.__name__, name, subcls.__name__))

        cls._validate_legacy_aliases(declared)

    @classmethod
    def _declared_tunables(cls):
        """Field names across the MRO, excluding ClassVar class-level config."""
        names = set()
        for klass in reversed(cls.__mro__):
            # vars() rather than cls.__annotations__: the latter falls through to
            # a base class's annotations when a class declares none of its own.
            for name, annotation in vars(klass).get('__annotations__', {}).items():
                if _is_class_var(annotation):
                    names.discard(name)
                else:
                    names.add(name)
        return names

    @classmethod
    def _validate_legacy_aliases(cls, declared):
        """Reject a LEGACY_ALIASES target that names no real field.

        Like the FIELD_SPECS guard, this is a static schema error caught at
        import rather than a bad platform value.
        """
        for old_key, alias in cls.LEGACY_ALIASES.items():
            if alias.scope not in ('section', 'file'):
                raise TypeError(
                    "{}.LEGACY_ALIASES['{}'] has invalid scope {!r}; expected "
                    "'section' or 'file'.".format(cls.__name__, old_key, alias.scope))
            parts = alias.target.split('.')
            if len(parts) == 1:
                if parts[0] not in declared:
                    raise TypeError(
                        "{}.LEGACY_ALIASES['{}'] target '{}' names no field.".format(
                            cls.__name__, old_key, alias.target))
            elif len(parts) == 2:
                sub, field_name = parts
                subcls = cls.SUBSECTIONS.get(sub)
                if subcls is None:
                    raise TypeError(
                        "{}.LEGACY_ALIASES['{}'] target '{}' does not name a "
                        "subsection.".format(cls.__name__, old_key, alias.target))
                if field_name not in subcls._declared_tunables():
                    raise TypeError(
                        "{}.LEGACY_ALIASES['{}'] target '{}' names no field on "
                        "subsection '{}'.".format(
                            cls.__name__, old_key, alias.target, sub))
            else:
                raise TypeError(
                    "{}.LEGACY_ALIASES['{}'] target '{}' is more than one level "
                    "deep.".format(cls.__name__, old_key, alias.target))

    @classmethod
    def _logger(cls):
        identifier = cls.SYSLOG_IDENTIFIER or '{}_config'.format(
            cls.SECTION_NAME or 'pmon_daemon')
        return get_config_logger(identifier)

    @classmethod
    def _log_prefix(cls):
        return '{} config'.format(cls.SECTION_NAME or 'pmon daemon')

    @classmethod
    def resolve(cls, platform_section=None, platform_file=None):
        """Build a config by layering the platform file over the built-in defaults.

        platform_section (and, for scope='file' legacy aliases, platform_file)
        are exposed for tests so the merge and alias logic can be exercised
        without touching the filesystem; in production both are read from disk.
        """
        cfg = cls()
        if platform_section is None:
            platform_section, platform_file = cls._read_platform_control()
        overrides = cfg._with_legacy_aliases(platform_section, platform_file)
        cfg._merge(overrides)
        cfg._post_merge()
        return cfg

    def _merge(self, overrides):
        """Apply the platform section: unknown keys and unusable values are dropped."""
        logger = self._logger()
        prefix = self._log_prefix()
        valid = {f.name for f in fields(self)}
        for key, value in overrides.items():
            if key not in valid:
                logger.log_notice(
                    "{}: ignoring unknown key '{}' in {}".format(
                        prefix, key, PMON_DAEMON_CONTROL_FILE))
                continue
            if value is None:
                # An absent override never clobbers the default.
                continue
            if key in self.SUBSECTIONS:
                self._merge_subsection(key, value)
                continue
            spec = self.FIELD_SPECS.get(key)
            if spec is not None:
                try:
                    value = spec.coerce(value)
                except (TypeError, ValueError):
                    logger.log_warning(
                        "{}: invalid value {!r} for '{}' in {}; keeping default".format(
                            prefix, value, key, PMON_DAEMON_CONTROL_FILE))
                    continue
                reason = spec.rejection_reason(value)
                if reason is not None:
                    logger.log_warning(
                        "{}: out-of-range value {!r} for '{}' in {}; {}; keeping "
                        "default".format(prefix, value, key,
                                         PMON_DAEMON_CONTROL_FILE, reason))
                    continue
            setattr(self, key, value)

    def _post_merge(self):
        """Hook for constraints that span more than one field.

        FieldSpec validates each field in isolation; a subclass overrides this to
        enforce (or clamp) relationships between fields once every override has
        been applied. Default: nothing to do.
        """
        return

    def _merge_subsection(self, key, value):
        """Merge a nested dict into the subsection instance at self.<key>.

        The subsection keeps the parent's built-in defaults for any field the
        nested dict omits; a non-dict value is rejected and the subsection is
        left untouched. Coercion and validation inside the subsection use its
        own FIELD_SPECS, identical to the top level.
        """
        current = getattr(self, key)
        if not isinstance(value, dict):
            self._logger().log_warning(
                "{}: '{}' in {} must be an object; keeping subsection defaults".format(
                    self._log_prefix(), key, PMON_DAEMON_CONTROL_FILE))
            return
        current._merge(value)
        current._post_merge()

    def _with_legacy_aliases(self, section, platform_file):
        """Return a copy of section with deprecated keys routed to their targets.

        For each LEGACY_ALIASES entry whose deprecated key is present (in the
        daemon section for scope='section', or the whole file for scope='file'),
        the raw value is optionally transformed and written to the alias target
        only if the nested form is absent, so an explicit nested value always
        wins. A used alias logs a deprecation warning. Section-scoped legacy keys
        are consumed so _merge does not also report them as unknown.
        """
        if not self.LEGACY_ALIASES:
            return section
        section = section if isinstance(section, dict) else {}
        platform_file = platform_file or {}
        overrides = copy.deepcopy(section)
        logger = self._logger()
        prefix = self._log_prefix()
        for old_key, alias in self.LEGACY_ALIASES.items():
            source = platform_file if alias.scope == 'file' else overrides
            if not isinstance(source, dict) or old_key not in source:
                continue
            raw = source.get(old_key)
            if alias.scope == 'section':
                # Consume the flat key so _merge does not also flag it unknown.
                overrides.pop(old_key, None)
            if raw is None:
                continue
            logger.log_warning(
                "{}: '{}' in {} is deprecated; set '{}' instead".format(
                    prefix, old_key, PMON_DAEMON_CONTROL_FILE, alias.target))
            value = raw
            if alias.transform is not None:
                try:
                    value = alias.transform(raw)
                except (TypeError, ValueError):
                    logger.log_warning(
                        "{}: invalid value {!r} for deprecated '{}' in {}; keeping "
                        "default".format(prefix, raw, old_key, PMON_DAEMON_CONTROL_FILE))
                    continue
            self._inject_alias_value(overrides, alias.target, value)
        return overrides

    @staticmethod
    def _inject_alias_value(overrides, target, value):
        """Write value at the dotted target in overrides, never overwriting.

        setdefault gives the nested form precedence: if the destination key is
        already present (an explicit nested value), the alias value is dropped.
        """
        parts = target.split('.')
        if len(parts) == 1:
            overrides.setdefault(parts[0], value)
            return
        sub, field_name = parts[0], parts[1]
        subdict = overrides.get(sub)
        if not isinstance(subdict, dict):
            subdict = {}
            overrides[sub] = subdict
        subdict.setdefault(field_name, value)

    @classmethod
    def _read_platform_section(cls):
        """Return this daemon's dict from pmon_daemon_control.json, or {} if absent.

        Thin wrapper over _read_platform_control for callers that only need the
        daemon's own section and not the sibling top-level keys.
        """
        return cls._read_platform_control()[0]

    @classmethod
    def _read_platform_control(cls):
        """Return (section, whole_file) from pmon_daemon_control.json.

        section is this daemon's own object (or {} if absent/malformed);
        whole_file is the entire parsed file, needed so a scope='file' legacy
        alias can read a top-level key that lives outside the daemon section.

        Mirrors docker_init.j2: the hwsku file takes precedence over the platform
        file, and only the first existing file is consulted (no cross-file merge).
        Any failure degrades to ({}, {}) so the daemon starts on its defaults.
        """
        logger = cls._logger()
        prefix = cls._log_prefix()
        try:
            platform_path, hwsku_path = device_info.get_paths_to_platform_and_hwsku_dirs()
        except Exception as exc:  # device_info can raise if platform is undetermined
            logger.log_warning(
                "{}: unable to determine platform/hwsku dirs: {}".format(prefix, exc))
            return {}, {}

        for directory in (hwsku_path, platform_path):
            if not directory:
                continue
            path = os.path.join(directory, PMON_DAEMON_CONTROL_FILE)
            if not os.path.isfile(path):
                continue
            try:
                with open(path) as control_file:
                    data = json.load(control_file)
            except (OSError, ValueError) as exc:
                logger.log_warning(
                    "{}: failed to read {}: {}".format(prefix, path, exc))
                return {}, {}
            if not isinstance(data, dict):
                logger.log_warning(
                    "{}: {} is not an object; ignoring".format(prefix, path))
                return {}, {}
            section = data.get(cls.SECTION_NAME, {})
            if not isinstance(section, dict):
                logger.log_warning(
                    "{}: '{}' section in {} is not an object; ignoring".format(
                        prefix, cls.SECTION_NAME, path))
                return {}, data
            return section, data
        return {}, {}
