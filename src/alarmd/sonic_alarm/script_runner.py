"""
sonic_alarm.script_runner -- Executes external health-check scripts.

Scripts are run via ThreadPoolExecutor for parallel execution.
Exit code evaluation follows the same raise/clear pattern as StateDBPoller.

Logging: module-level SysLogger (identifier 'alarmd#runner'), following
the same pattern as system-health/utils.py.
"""

import os
import re
import logging
import operator as _op
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed

from logging.handlers import SysLogHandler
from sonic_py_common.syslogger import SysLogger
from sonic_alarm.constants import SCRIPT_TIMEOUT, MAX_SCRIPT_TIMEOUT

logger = SysLogger(
    log_identifier='alarmd#runner',
    log_facility=SysLogHandler.LOG_DAEMON,
    log_level=logging.INFO,
    enable_runtime_config=False
)


class ScriptRunner:
    """Runs external scripts defined in alarm_defs (type=script),
    checks exit codes and/or stdout, raises/clears alarms.

    Parameters
    ----------
    script_groups : list[dict]
        The script-type alarm_tables from the merged alarm definitions.
    store : EventPublisher
        The eventd output path to raise/clear alarms on
        (set_alarm / raise_alarm / clear_alarm).
    max_workers : int
        Maximum concurrent script executions (default 4).
    """

    # Exit-code comparison dispatch
    _EXIT_CODE_OPS = {
        '!=': _op.ne,
        '==': _op.eq,
        '>':  _op.gt,
        '<':  _op.lt,
        '>=': _op.ge,
        '<=': _op.le,
    }

    def __init__(self, script_groups, store, max_workers=4):
        self._script_groups = script_groups
        self._store = store
        self._max_workers = max_workers
        self._missing_scripts = set()
        # Snapshot mtime of every script at config-load time.
        # If a script is modified between reloads, we log a warning
        # but still execute it.
        self._baseline_mtimes = self._snapshot_mtimes()

    def _snapshot_mtimes(self):
        """Record the mtime of every script at config-load time."""
        mtimes = {}
        for group in self._script_groups:
            for check in group.get('checks', []):
                command = check.get('command', '')
                if not command:
                    continue
                try:
                    script_path = command.split()[0]
                except Exception:
                    continue
                try:
                    mtimes[script_path] = os.path.getmtime(script_path)
                except OSError:
                    pass  # missing scripts handled elsewhere
        return mtimes

    def run_all(self):
        """Run all script checks using a thread pool for parallelism."""
        checks_to_run = []
        for group in self._script_groups:
            for check in group.get('checks', []):
                checks_to_run.append(check)

        if not checks_to_run:
            return

        with ThreadPoolExecutor(max_workers=self._max_workers) as pool:
            futures = {
                pool.submit(self._run_check, check): check
                for check in checks_to_run
            }
            for future in as_completed(futures):
                check = futures[future]
                try:
                    future.result()
                except Exception as exc:
                    check_name = check.get('check_name', '?')
                    logger.log_error(
                        f"Script check '{check_name}' raised exception: {exc}")

    def _run_check(self, check):
        """Run a single script check and raise/clear accordingly."""
        alarm_id = check.get('alarm_id', '')
        command = check.get('command', '')
        object_name = check.get('object_name', 'SYSTEM')
        check_name = check.get('check_name', alarm_id)
        cond = check.get('condition', {})
        timeout = check.get('timeout', SCRIPT_TIMEOUT)

        if not alarm_id:
            logger.log_warning(
                f"Script check '{check_name}': Missing alarm_id, skipping")
            return

        if not command:
            logger.log_warning(
                f"Script check '{check_name}': Missing command, skipping")
            return

        # Validate and clamp timeout
        try:
            timeout = float(timeout)
            if timeout <= 0:
                logger.log_warning(
                    f"Script check '{check_name}': Invalid timeout "
                    f"{timeout}, using default {SCRIPT_TIMEOUT}s")
                timeout = SCRIPT_TIMEOUT
            elif timeout > MAX_SCRIPT_TIMEOUT:
                timeout = MAX_SCRIPT_TIMEOUT
        except (ValueError, TypeError):
            logger.log_warning(
                f"Script check '{check_name}': Non-numeric timeout, "
                f"using default {SCRIPT_TIMEOUT}s")
            timeout = SCRIPT_TIMEOUT

        # Guard: if the script binary doesn't exist, skip.
        try:
            script_path = command.split()[0]
        except Exception:
            logger.log_error(
                f"Script check '{check_name}': Cannot parse command "
                f"'{command}', skipping")
            return

        if not os.path.isfile(script_path):
            if script_path not in self._missing_scripts:
                logger.log_warning(
                    f"Script not found, skipping: {script_path} "
                    f"(alarm_id: {alarm_id})")
                self._missing_scripts.add(script_path)
            return

        # Modification-aware logging: if the script's mtime changed since
        # config was loaded, log a warning but still execute.
        baseline = self._baseline_mtimes.get(script_path)
        if baseline is not None:
            try:
                current_mtime = os.path.getmtime(script_path)
                if current_mtime != baseline:
                    logger.log_warning(
                        f"MODIFIED SCRIPT: {script_path} "
                        f"(alarm_id: {alarm_id}) has changed since config "
                        f"was loaded — executing anyway, results may be "
                        f"unreliable. Reload config (SIGHUP) to re-baseline.")
            except OSError:
                pass  # race with deletion — "not found" guard catches next

        try:
            result = subprocess.run(
                command, shell=True,
                capture_output=True, text=True,
                timeout=timeout
            )
            exit_code = result.returncode
            stdout = result.stdout.strip() if result.stdout else ''
        except subprocess.TimeoutExpired:
            logger.log_warning(
                f"Script timed out ({timeout}s): {command}")
            try:
                self._store.raise_alarm(
                    alarm_id, object_name,
                    description=f"{alarm_id}: script timed out after {timeout}s")
            except Exception as exc:
                logger.log_error(
                    f"Failed to raise timeout alarm for {alarm_id}: {exc}")
            return
        except Exception as exc:
            logger.log_error(
                f"Script execution failed: {command} — {exc}")
            return

        try:
            is_fault = self._evaluate_script_condition(cond, exit_code, stdout)
        except Exception as exc:
            logger.log_error(
                f"Script check '{check_name}': "
                f"Condition evaluation error: {exc}")
            return

        try:
            if is_fault:
                tmpl = check.get('description_template', alarm_id)
                desc = tmpl.replace('{object_name}', object_name)
                if stdout:
                    snippet = stdout[:200]
                    desc = f"{desc} — {snippet}"
                self._store.raise_alarm(
                    alarm_id, object_name, description=desc)
            else:
                self._store.clear_alarm(alarm_id, object_name)
        except Exception as exc:
            logger.log_error(
                f"Script check '{check_name}': "
                f"Failed to set alarm state: {exc}")

    @staticmethod
    def _evaluate_script_condition(condition, exit_code, stdout):
        """Evaluate whether a script result indicates a fault.

        Condition types:
          {"exit_code": "!=", "value": 0}        -- fault if exit code != 0
          {"stdout_contains": "FAIL"}             -- fault if stdout has "FAIL"
          {"stdout_regex": "ERROR.*critical"}     -- fault if stdout matches

        Returns False on any error (fail-safe).
        """
        if not condition:
            return exit_code != 0

        try:
            if 'exit_code' in condition:
                op = condition['exit_code']
                expected = int(condition.get('value', 0))
                cmp_fn = ScriptRunner._EXIT_CODE_OPS.get(op)
                if cmp_fn is not None:
                    return cmp_fn(exit_code, expected)
                return False

            if 'stdout_contains' in condition:
                search_str = condition['stdout_contains']
                if not isinstance(search_str, str):
                    return False
                return search_str in stdout

            if 'stdout_regex' in condition:
                pattern = condition['stdout_regex']
                if not isinstance(pattern, str):
                    return False
                return bool(re.search(pattern, stdout))

        except (ValueError, TypeError, re.error, AttributeError):
            return False

        return exit_code != 0
