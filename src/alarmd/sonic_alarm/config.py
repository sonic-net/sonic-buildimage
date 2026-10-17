"""
sonic_alarm.config -- Alarm definition loading, merging, and validation.

Handles the full configuration pipeline:
  - resolve_paths()             : find platform-specific alarm_defs path
  - _load_common_catalog()      : load the shipped common_alarm_defs.json
  - _merge_common()             : merge platform alarm_tables on top of common
  - _expand_shorthand()         : expand checks_short arrays into full check dicts
  - load_alarm_defs()           : orchestrate the full pipeline
  - validate()                  : semantic validation of the merged config
"""

import os
import re
import json
import copy
import logging

from sonic_alarm.constants import (
    MACHINE_CONF,
    DEVICE_BASE,
    ALARM_DEFS_FILENAME,
    COMMON_ALARM_DEFS_FILENAME,
    EVENT_PROFILE_FILENAME,
    SONIC_VERSION_FILE,
    SCRIPT_TIMEOUT,
    MAX_SCRIPT_TIMEOUT,
    VALID_SEVERITIES,
    VALID_OPERATORS,
)

_log = logging.getLogger('alarmd')

# Path to the common alarm catalog shipped inside the sonic_alarm package
COMMON_ALARM_DEFS = os.path.join(
    os.path.dirname(__file__), COMMON_ALARM_DEFS_FILENAME)

# Path to the event-profile fragment shipped inside the sonic_alarm package
EVENT_PROFILE = os.path.join(
    os.path.dirname(__file__), EVENT_PROFILE_FILENAME)


# ---------------------------------------------------------------------------
# Path resolution
# ---------------------------------------------------------------------------

def resolve_paths():
    """Resolve platform-specific alarm_defs.json path.

    Reads /host/machine.conf to determine onie_platform, then checks
    /usr/share/sonic/device/<platform>/ for alarm_defs.json.

    Returns (file_path_or_None, platform_dir_or_None).
    """
    file_path = None

    platform = None
    try:
        with open(MACHINE_CONF, 'r') as f:
            for line in f:
                for prefix in ('onie_platform=', 'aboot_platform='):
                    if line.startswith(prefix):
                        platform = line.strip().split('=', 1)[1]
                        break
                if platform:
                    break
    except (IOError, OSError) as e:
        _log.error("FATAL: Cannot read %s: %s", MACHINE_CONF, e)
        return None, None

    if not platform:
        _log.error(
            "FATAL: Cannot determine platform — no onie_platform or "
            "aboot_platform in %s", MACHINE_CONF)
        return None, None

    platform_dir = os.path.join(DEVICE_BASE, platform)
    candidate_file = os.path.join(platform_dir, ALARM_DEFS_FILENAME)

    if os.path.isfile(candidate_file):
        file_path = candidate_file

    if not file_path:
        _log.warning(
            "No platform alarm definitions found for '%s'. "
            "Falling back to common catalog only.", platform)

    return file_path, platform_dir


# ---------------------------------------------------------------------------
# JSON file loading
# ---------------------------------------------------------------------------

def _load_json_file(path):
    """Load and parse a single JSON file."""
    _log.info("Loading alarm definitions from %s", path)
    try:
        with open(path, 'r') as f:
            return json.load(f)
    except Exception as exc:
        _log.error("FATAL: Failed to load %s: %s", path, exc)
        return None


# ---------------------------------------------------------------------------
# Common catalog and merge
# ---------------------------------------------------------------------------

def _load_common_catalog():
    """Load the common alarm catalog shipped with the sonic_alarm package.

    Returns the parsed dict, or None on error.
    """
    return _load_json_file(COMMON_ALARM_DEFS)


def _merge_common(platform_defs):
    """Merge platform alarm_tables on top of the common catalog.

    Merge rules (simple, deterministic):
      1. Start with common catalog's alarm_tables as the baseline.
      2. For each table in platform's alarm_tables:
         - If table_name matches a common table, append platform checks.
           If a platform check has the same check_name as a common check,
           the platform check replaces it (last-writer-wins).
         - If table_name does not exist in common, add it as a new table.
      3. If platform file contains a "disable" list, remove those checks.
         Format: ["TABLE_NAME.check_name", ...]

    Returns a fully-merged alarm definition dict, or None on error.
    """
    common = _load_common_catalog()
    if common is None:
        _log.error("FATAL: Cannot load common alarm catalog from %s",
                   COMMON_ALARM_DEFS)
        return None

    # Start with a deep copy of common tables
    common_tables = copy.deepcopy(common.get('alarm_tables', []))

    # Index common tables by table_name for fast lookup
    table_index = {}
    for table in common_tables:
        tname = table.get('table_name', '') or table.get('group_name', '')
        if tname:
            table_index[tname] = table

    # Merge platform tables on top
    platform_tables = platform_defs.get('alarm_tables', [])
    for ptable in platform_tables:
        tname = (ptable.get('table_name', '') or ptable.get('group_name', '')
                 or ptable.get('event_source', ''))
        if tname in table_index:
            # Existing common table — merge checks
            existing = table_index[tname]
            existing_checks = existing.get('checks', [])
            platform_checks = ptable.get('checks', [])

            # Build index of existing check_names for replacement
            check_index = {c.get('check_name'): i
                           for i, c in enumerate(existing_checks)}

            for pcheck in platform_checks:
                cname = pcheck.get('check_name', '')
                if cname in check_index:
                    # Replace common check with platform check
                    existing_checks[check_index[cname]] = pcheck
                    _log.info("Merge: platform replaces check %s.%s",
                              tname, cname)
                else:
                    # Append new check
                    existing_checks.append(pcheck)

            # Merge shorthand too
            if 'checks_short' in ptable:
                existing.setdefault('checks_short', []).extend(
                    ptable['checks_short'])
        else:
            # New table — add it
            common_tables.append(copy.deepcopy(ptable))
            table_index[tname] = common_tables[-1]
            _log.info("Merge: added new table %s from platform", tname)

    # Apply disable list
    disable_list = set(platform_defs.get('disable', []))
    if disable_list:
        for table in common_tables:
            tname = table.get('table_name', '') or table.get('group_name', '')
            before = len(table.get('checks', []))
            table['checks'] = [
                c for c in table.get('checks', [])
                if f"{tname}.{c.get('check_name', '')}" not in disable_list
            ]
            removed = before - len(table['checks'])
            if removed:
                _log.info("Merge: disabled %d check(s) from %s",
                          removed, tname)

    # Build result
    result = {
        'alarm_tables': common_tables,
    }

    # Carry over settings from platform file
    if 'alarmd_settings' in platform_defs:
        result['alarmd_settings'] = platform_defs['alarmd_settings']
    if 'sonic_version' in platform_defs:
        result['sonic_version'] = platform_defs['sonic_version']

    total_checks = sum(len(t.get('checks', [])) for t in common_tables)
    _log.info("Merge complete: %d table(s), %d total check(s)",
              len(common_tables), total_checks)

    return result
# ---------------------------------------------------------------------------
# Shorthand expansion
# ---------------------------------------------------------------------------

# Operators to match, ordered longest-first to avoid '<' matching '<='
_SHORTHAND_OPS = ['<=', '>=', '!=', '==', '<', '>']


def _parse_condition_expr(expr):
    """Parse a shorthand condition expression like 'presence == false'.

    Returns (field, operator, value) or None on parse failure.
    """
    for op in _SHORTHAND_OPS:
        if op in expr:
            parts = expr.split(op, 1)
            if len(parts) == 2:
                field = parts[0].strip()
                value = parts[1].strip()
                if field and value:
                    return field, op, value
    _log.error("Cannot parse shorthand condition: '%s'", expr)
    return None


def _expand_shorthand(alarm_tables):
    """Expand checks_short arrays into full check dicts on each table.

    Shorthand format: [alarm_id, "field op value", severity]
    checks_short is concatenated with any existing 'checks' array.

    Mutates the tables in-place.
    """
    for table in alarm_tables:
        short_checks = table.get('checks_short', [])
        if not short_checks:
            continue

        expanded = []
        for entry in short_checks:
            if not isinstance(entry, (list, tuple)) or len(entry) < 3:
                _log.warning("Skipping malformed shorthand entry: %s", entry)
                continue

            alarm_id = str(entry[0])
            condition_expr = str(entry[1])
            severity = str(entry[2])

            parsed = _parse_condition_expr(condition_expr)
            if parsed is None:
                continue

            field, operator, value = parsed

            expanded.append({
                'check_name': alarm_id,
                'alarm_id': alarm_id,
                'severity': severity,
                'category': 'Hardware',
                'description_template': '{object_name} ' + alarm_id,
                'condition': {
                    'field': field,
                    'operator': operator,
                    'value': value,
                },
            })

        if expanded:
            existing = table.get('checks', [])
            table['checks'] = existing + expanded
            _log.info("Shorthand: expanded %d entries for table %s",
                      len(expanded),
                      table.get('table_name', table.get('group_name', '?')))

        # Remove checks_short after expansion
        table.pop('checks_short', None)


# ---------------------------------------------------------------------------
# SONiC version check
# ---------------------------------------------------------------------------

def _get_running_sonic_version():
    """Read the running SONiC image version from sonic_version.yml.

    Returns the branch string (e.g. 'master', '202505', '202605') or None
    if the file is unreadable or the field is missing.
    """
    try:
        with open(SONIC_VERSION_FILE, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('branch:'):
                    # branch: 'master'  or  branch: master
                    val = line.split(':', 1)[1].strip().strip("'\"")
                    return val if val else None
    except (IOError, OSError):
        return None
    return None


def _check_sonic_version(defs):
    """Compare alarm_defs sonic_version against the running SONiC branch.

    Logs a WARNING if there is a mismatch, but does NOT block loading.
    This is advisory — a version-targeted alarm_defs file still works on
    other branches, it just might monitor conditions that don't exist or
    miss conditions that are new.
    """
    declared = defs.get('sonic_version')
    if not declared:
        _log.debug("No sonic_version declared in alarm_defs — skipping "
                   "version check")
        return

    running = _get_running_sonic_version()
    if not running:
        _log.debug("Cannot determine running SONiC version — skipping "
                   "version check")
        return

    if declared != running:
        _log.warning(
            "SONIC VERSION MISMATCH: alarm_defs declares "
            "sonic_version='%s' but running SONiC branch is '%s'. "
            "Some alarm checks may reference STATE_DB fields or scripts "
            "that do not exist on this version, or may miss checks "
            "added in a newer version.",
            declared, running)
    else:
        _log.info("sonic_version check passed: alarm_defs and running "
                  "image both target '%s'", declared)


# ---------------------------------------------------------------------------
# Event profile check
# ---------------------------------------------------------------------------

def load_event_profile():
    """Load the shipped event-profile fragment (sonic_events_alarmd.json).

    Returns the set of profiled alarm_ids (each entry's ``name``), or None if
    the fragment is missing/unreadable (the profile check is skipped then).
    """
    try:
        with open(EVENT_PROFILE, 'r') as f:
            data = json.load(f)
    except (IOError, OSError, ValueError):
        return None
    names = set()
    for ev in data.get('events', []):
        name = ev.get('name')
        if name:
            names.add(name)
    return names


def _check_event_profile(defs):
    """Warn for any alarm_id that has no event-profile entry (HLD §7.12, §9.3).

    eventd DROPS events whose type-id is absent from the profile, so an
    unprofiled alarm_id would never reach the ALARM table.  Advisory here
    (warn-only); the build's YANG/event-profile CI is the hard gate.
    """
    profiled = load_event_profile()
    if profiled is None:
        _log.debug("Event profile fragment unreadable — skipping profile check")
        return
    unprofiled = set()
    for table in defs.get('alarm_tables', []):
        for check in table.get('checks', []):
            aid = check.get('alarm_id')
            if aid and aid not in profiled:
                unprofiled.add(aid)
    if unprofiled:
        _log.warning(
            "UNPROFILED ALARM IDS: %s — these have no entry in %s and would "
            "be DROPPED by eventd. Add them to the event profile (HLD §9.3).",
            ', '.join(sorted(unprofiled)), EVENT_PROFILE_FILENAME)


# ---------------------------------------------------------------------------
# Main loading pipeline
# ---------------------------------------------------------------------------

def load_alarm_defs():
    """Load alarm definitions through the full configuration pipeline.

    Pipeline:
      1. Resolve platform path (alarm_defs.json)
      2. If platform file exists, load it and merge with common catalog
      3. If no platform file exists, fall back to common catalog directly
      4. Expand shorthand entries (checks_short)
      5. Validate the merged config
      6. Advisory sonic_version check

    Returns the merged dict, or None on error.
    """
    file_path, _platform_dir = resolve_paths()

    merged = None

    # Load platform file and merge with common
    if file_path:
        platform_defs = _load_json_file(file_path)
        if platform_defs is None:
            return None

        merged = _merge_common(platform_defs)
        if merged is None:
            return None

    # Fallback: if no platform file, load common catalog directly
    if merged is None:
        _log.info("No platform alarm definitions — falling back to "
                  "common catalog")
        merged = _load_common_catalog()
        if merged is None:
            _log.error("FATAL: Cannot load common alarm catalog from %s",
                       COMMON_ALARM_DEFS)
            return None
        merged = copy.deepcopy(merged)

    # Expand shorthand entries
    _expand_shorthand(merged.get('alarm_tables', []))

    # Validate
    is_valid, errors = validate(merged)
    if not is_valid:
        _log.error(
            "FATAL: Config validation failed with %d error(s). "
            "Fix the alarm definitions and reload. Errors: %s",
            len(errors), '; '.join(errors[:5]))
        return None

    # Advisory sonic_version check (warn-only, does not block loading)
    _check_sonic_version(merged)

    # Advisory event-profile check (warn-only): flag unprofiled alarm_ids
    _check_event_profile(merged)

    return merged


# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

def validate(defs):
    """Validate alarm definitions for consistency and correctness.

    Returns (is_valid: bool, error_messages: list[str]).
    """
    errors = []
    warnings = []

    if not defs or 'alarm_tables' not in defs:
        errors.append("Config missing 'alarm_tables' key")
        return False, errors

    alarm_registry = {}
    check_names_seen = set()

    for table_idx, table in enumerate(defs.get('alarm_tables', [])):
        table_type = table.get('type', 'statedb')
        table_name = table.get('table_name', '')
        group_name = table.get('group_name', '')
        source_name = (table_name or group_name
                       or table.get('event_source') or f'table_{table_idx}')

        if table_type not in ('statedb', 'script', 'event'):
            errors.append(
                f"{source_name}: Invalid type '{table_type}' "
                f"(must be 'statedb', 'script', or 'event')")
            continue

        if table_type == 'statedb' and not table_name:
            errors.append(
                f"{source_name}: Missing 'table_name' for statedb table")
            continue

        if table_type == 'event' and not (
                table.get('event_source') and table.get('event_tag')):
            errors.append(
                f"{source_name}: 'event' table requires 'event_source' "
                f"and 'event_tag'")
            continue

        if table_type == 'script' and not group_name:
            warnings.append(
                f"{source_name}: Missing 'group_name' for script table")

        checks = table.get('checks', [])
        if not checks:
            warnings.append(
                f"{source_name}: No checks defined (table will be ignored)")
            continue

        for check_idx, check in enumerate(checks):
            check_name = check.get('check_name', f'unnamed_check_{check_idx}')
            alarm_id = check.get('alarm_id', '')

            if not alarm_id:
                errors.append(
                    f"{source_name}.{check_name}: "
                    f"Missing required field 'alarm_id'")
                continue

            full_check_id = f"{source_name}.{check_name}"
            if full_check_id in check_names_seen:
                warnings.append(
                    f"Duplicate check_name: {full_check_id}")
            check_names_seen.add(full_check_id)

            severity = check.get('severity', 'Minor')
            if severity not in VALID_SEVERITIES:
                warnings.append(
                    f"{full_check_id}: Invalid severity '{severity}' "
                    f"(valid: {', '.join(VALID_SEVERITIES)})")

            if table_type in ('statedb', 'event'):
                condition = check.get('condition', {})
                if not condition:
                    errors.append(
                        f"{full_check_id}: Missing 'condition' block")
                    continue

                field = condition.get('field', '')
                operator = condition.get('operator', '==')
                value = condition.get('value')

                if not field:
                    errors.append(
                        f"{full_check_id}: Missing condition.field")

                if operator not in VALID_OPERATORS:
                    errors.append(
                        f"{full_check_id}: Invalid operator '{operator}'")

                if value is None or value == '':
                    warnings.append(
                        f"{full_check_id}: condition.value is empty")

                if alarm_id not in alarm_registry:
                    alarm_registry[alarm_id] = []
                alarm_registry[alarm_id].append(
                    (check_name, source_name, 'dynamic'))

            elif table_type == 'script':
                command = check.get('command', '')
                object_name = check.get('object_name', 'SYSTEM')
                timeout = check.get('timeout', SCRIPT_TIMEOUT)
                condition = check.get('condition', {})

                if not command:
                    errors.append(
                        f"{full_check_id}: Missing 'command' field")
                    continue

                script_path = command.split()[0]
                if not os.path.isfile(script_path):
                    errors.append(
                        f"{full_check_id}: Script not found: {script_path}")
                elif not os.access(script_path, os.X_OK):
                    errors.append(
                        f"{full_check_id}: Script not executable: "
                        f"{script_path}")

                if not isinstance(timeout, (int, float)) or timeout <= 0:
                    warnings.append(
                        f"{full_check_id}: Invalid timeout {timeout}")
                elif timeout > MAX_SCRIPT_TIMEOUT:
                    warnings.append(
                        f"{full_check_id}: Timeout {timeout}s exceeds "
                        f"max {MAX_SCRIPT_TIMEOUT}s (will be clamped)")

                if condition and 'exit_code' in condition:
                    op = condition.get('exit_code')
                    if op not in VALID_OPERATORS:
                        errors.append(
                            f"{full_check_id}: Invalid exit_code "
                            f"operator '{op}'")

                if condition and 'stdout_regex' in condition:
                    try:
                        re.compile(condition['stdout_regex'])
                    except re.error as e:
                        errors.append(
                            f"{full_check_id}: Invalid regex: {e}")

                alarm_key = f"{alarm_id}|{object_name}"
                if alarm_key not in alarm_registry:
                    alarm_registry[alarm_key] = []
                alarm_registry[alarm_key].append(
                    (check_name, source_name, object_name))

    # Cross-table conflict detection
    for alarm_key, sources in alarm_registry.items():
        if len(sources) > 1:
            table_sources = set(src[1] for src in sources)
            if len(table_sources) > 1:
                check_list = ', '.join(
                    f"{s[0]}@{s[1]}" for s in sources)
                warnings.append(
                    f"alarm_id '{alarm_key}' produced by multiple "
                    f"tables: {check_list}")

    for w in warnings:
        _log.warning("CONFIG VALIDATION: %s", w)

    if errors:
        for e in errors:
            _log.error("CONFIG VALIDATION ERROR: %s", e)
        return False, errors

    _log.info(
        "Config validation passed: %d alarm table(s), %d check(s), "
        "%d warning(s)",
        len(defs.get('alarm_tables', [])),
        sum(len(t.get('checks', [])) for t in defs.get('alarm_tables', [])),
        len(warnings))
    return True, []
