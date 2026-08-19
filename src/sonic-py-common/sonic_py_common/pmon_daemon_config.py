"""
Shared resolver for pmon daemon runtime tunables.

Historically every pmon tunable was plumbed end-to-end as a command-line flag:
the platform set it in pmon_daemon_control.json, sonic-cfggen loaded that file
when rendering docker-pmon.supervisord.conf.j2, the template flattened it into
"--flag value", argparse re-parsed it, and the daemon constructor grew another
parameter. Adding one knob meant editing four places, once per daemon.

This module removes that round trip. A daemon declares a dataclass subclass of
PmonDaemonConfig naming the section it owns and the fields it accepts; the base
locates pmon_daemon_control.json, extracts that section, and layers it over the
built-in defaults.

Precedence, highest wins:
  1. Per-platform / per-hwsku file - the daemon's section of pmon_daemon_control.json
  2. Built-in defaults            - the subclass's dataclass field defaults

The per-platform file is read from the same device directories (and with the
same hwsku-over-platform precedence) that docker_init.j2 uses and that xcvrd's
media_settings.json / optics_si_settings.json parsers already read.

Adding a tunable is one field plus one FIELD_SPECS entry declaring its type
coercion and valid range. Platform owners set it in the section they already
maintain; no template, argparse, or constructor change.

Example:

    @dataclass
    class XcvrdConfig(PmonDaemonConfig):
        SECTION_NAME = 'xcvrd'
        FIELD_SPECS = {
            'dom_update_interval': FieldSpec(caster=int, minimum=0, maximum=86400),
        }

        dom_update_interval: Optional[int] = None

    config = XcvrdConfig.resolve()

Nothing here raises on bad input: an unreadable file, a malformed section, an
uncoercible value, or an out-of-range value degrades to the built-in default
with a syslog warning, so a bad tunable can never keep a pmon daemon down.
"""

import json
import os

from dataclasses import dataclass, fields
from typing import Callable, ClassVar, Dict, Optional, Tuple, get_origin

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

        unknown = sorted(specced - declared)
        if unknown:
            raise TypeError(
                "{}.FIELD_SPECS has no matching field for: {}. A spec keyed by a "
                "name no field declares is never applied, leaving that tunable "
                "with no type coercion and no range check.".format(
                    cls.__name__, ", ".join(unknown)))

        unspecced = sorted(declared - specced)
        if unspecced:
            raise TypeError(
                "{}.FIELD_SPECS is missing an entry for: {}. Every tunable "
                "declares its coercion and bounds; a field that is genuinely "
                "unbounded declares FieldSpec() so that stays an explicit "
                "choice.".format(cls.__name__, ", ".join(unspecced)))

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
    def _logger(cls):
        identifier = cls.SYSLOG_IDENTIFIER or '{}_config'.format(
            cls.SECTION_NAME or 'pmon_daemon')
        return get_config_logger(identifier)

    @classmethod
    def _log_prefix(cls):
        return '{} config'.format(cls.SECTION_NAME or 'pmon daemon')

    @classmethod
    def resolve(cls, platform_section=None):
        """Build a config by layering the platform file over the built-in defaults.

        platform_section is exposed for tests so the merge logic can be exercised
        without touching the filesystem; in production it is read from disk.
        """
        cfg = cls()
        if platform_section is None:
            platform_section = cls._read_platform_section()
        cfg._merge(platform_section)
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

    @classmethod
    def _read_platform_section(cls):
        """Return this daemon's dict from pmon_daemon_control.json, or {} if absent.

        Mirrors docker_init.j2: the hwsku file takes precedence over the platform
        file, and only the first existing file is consulted (no cross-file merge).
        Any failure degrades to {} so the daemon starts on its built-in defaults.
        """
        logger = cls._logger()
        prefix = cls._log_prefix()
        try:
            platform_path, hwsku_path = device_info.get_paths_to_platform_and_hwsku_dirs()
        except Exception as exc:  # device_info can raise if platform is undetermined
            logger.log_warning(
                "{}: unable to determine platform/hwsku dirs: {}".format(prefix, exc))
            return {}

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
                return {}
            section = data.get(cls.SECTION_NAME, {})
            if not isinstance(section, dict):
                logger.log_warning(
                    "{}: '{}' section in {} is not an object; ignoring".format(
                        prefix, cls.SECTION_NAME, path))
                return {}
            return section
        return {}
