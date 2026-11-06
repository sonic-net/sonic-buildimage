"""
sonic_alarm.statedb_poller -- Polls STATE_DB tables and evaluates conditions.

Contains:
  - evaluate_condition()  -- module-level function for field comparison
  - StateDBPoller         -- iterates tables, reads fields, calls EventPublisher

Logging: module-level SysLogger (identifier 'alarmd#poller'), following
the same pattern as system-health/service_checker.py.
"""

import logging
import operator as _op

from logging.handlers import SysLogHandler
from sonic_py_common.syslogger import SysLogger
from swsscommon.swsscommon import Table

logger = SysLogger(
    log_identifier='alarmd#poller',
    log_facility=SysLogHandler.LOG_DAEMON,
    log_level=logging.INFO,
    enable_runtime_config=False
)

# ---------------------------------------------------------------------------
# Condition evaluator
# ---------------------------------------------------------------------------

# String-comparison dispatch (case-sensitive per HLD §7.7)
_STRING_OPS = {
    '==': _op.eq,
    '!=': _op.ne,
}

# Numeric-comparison dispatch
_NUMERIC_OPS = {
    '<':  _op.lt,
    '>':  _op.gt,
    '<=': _op.le,
    '>=': _op.ge,
}

# All recognised operators
ALL_OPS = set(_STRING_OPS) | set(_NUMERIC_OPS)


def evaluate_condition(field_value, operator, expected):
    """Evaluate a condition from alarm_defs against a STATE_DB field value.

    Both field_value and expected are strings (STATE_DB stores everything
    as strings).

    Supports: ==, !=, <, >, <=, >=
      - == and != use case-sensitive string comparison (HLD §7.7).
      - < > <= >= try numeric comparison first; on ValueError, fall back
        to case-sensitive string comparison (HLD §7.7).
    Returns False on any error (fail-safe: don't raise spurious alarms).
    """
    if field_value is None:
        return False

    try:
        fv = field_value.strip() if isinstance(field_value, str) else str(field_value)
        ex = expected.strip() if isinstance(expected, str) else str(expected)
    except Exception:
        return False

    # String equality / inequality (case-sensitive)
    cmp_fn = _STRING_OPS.get(operator)
    if cmp_fn is not None:
        try:
            return cmp_fn(fv, ex)
        except Exception:
            return False

    # Numeric comparisons with string fallback
    cmp_fn = _NUMERIC_OPS.get(operator)
    if cmp_fn is not None:
        try:
            return cmp_fn(float(fv), float(ex))
        except (ValueError, TypeError):
            # String fallback for relational operators
            try:
                return cmp_fn(fv, ex)
            except Exception:
                return False

    # Unknown operator -> fail safe
    return False


# ---------------------------------------------------------------------------
# StateDB Poller
# ---------------------------------------------------------------------------

class StateDBPoller:
    """Polls STATE_DB tables defined in alarm_defs (type=statedb),
    evaluates conditions, and raises/clears alarms via EventPublisher.

    Parameters
    ----------
    tables : list[dict]
        The statedb-type alarm_tables from the merged alarm definitions.
    store : EventPublisher
        The eventd output path to raise/clear alarms on
        (set_alarm / raise_alarm / clear_alarm).
    state_db : DBConnector
        A swsscommon DBConnector for STATE_DB.
    """

    def __init__(self, tables, store, state_db):
        self._tables = tables
        self._store = store
        self._db = state_db

    def poll(self):
        """One full poll cycle across all statedb tables."""
        for table_def in self._tables:
            table_name = table_def.get('table_name', '')
            exclude = set(table_def.get('exclude_keys', []))
            checks = table_def.get('checks', [])

            if not table_name or not checks:
                continue

            try:
                tbl = Table(self._db, table_name)
                keys = tbl.getKeys()
            except Exception as exc:
                logger.log_error(
                    f"Table({table_name}).getKeys() failed: {exc}")
                return

            # Filter out excluded keys
            filtered_keys = [k for k in keys if k not in exclude
                             and f"{table_name}|{k}" not in exclude]

            if not filtered_keys:
                continue

            for key in filtered_keys:
                try:
                    status, fvs = tbl.get(key)
                except Exception as exc:
                    logger.log_error(
                        f"Table({table_name}).get({key}) failed: {exc}")
                    continue

                if not status:
                    continue

                fields = dict(fvs)
                object_name = key

                # Evaluate all checks, collect results per (alarm_id, object).
                # Multiple checks can share the same alarm_id (OR logic).
                fault_map = {}   # alarm_id -> bool

                for check in checks:
                    alarm_id = check.get('alarm_id')
                    cond = check.get('condition', {})
                    field_name = cond.get('field', '')
                    operator = cond.get('operator', '==')
                    expected = cond.get('value', '')

                    if not alarm_id:
                        logger.log_warning(
                            f"Table({table_name}).{check.get('check_name', 'unnamed')}: "
                            f"Skipping check with missing alarm_id")
                        continue

                    if not field_name:
                        logger.log_warning(
                            f"Table({table_name}).{check.get('check_name', alarm_id)}: "
                            f"Skipping check with missing condition.field")
                        continue

                    field_value = fields.get(field_name)

                    try:
                        is_fault = evaluate_condition(
                            field_value, operator, expected)
                    except Exception as exc:
                        logger.log_error(
                            f"Table({table_name}).{key}.{alarm_id}: "
                            f"Condition evaluation error: {exc}")
                        is_fault = False

                    # OR: once faulted, stays faulted for this alarm_id
                    if alarm_id not in fault_map:
                        fault_map[alarm_id] = is_fault
                    elif is_fault:
                        fault_map[alarm_id] = True

                # Apply the aggregated results
                for alarm_id, is_fault in fault_map.items():
                    try:
                        self._store.set_alarm(
                            alarm_id, object_name, is_fault=is_fault)
                    except Exception as exc:
                        logger.log_error(
                            f"Failed to set_alarm({alarm_id}, "
                            f"{object_name}, {is_fault}): {exc}")
