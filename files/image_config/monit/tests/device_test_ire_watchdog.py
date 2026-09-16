#!/usr/bin/env python3
"""On-device test harness for the monit ``ire_watchdog`` check.

Run this on an actual SONiC switch (a Broadcom DNX platform for the live path)
to validate the deployed ``/usr/bin/ire_watchdog`` one-shot monit check.

This is **not** a pytest unit test (it is named ``device_test_*`` so pytest does
not auto-collect it); it is a standalone diagnostic you copy onto a switch and
run by hand.

Two test groups:

* **Simulated** (default, always safe): imports the deployed script and drives
  ``main()`` / ``log_counters()`` / ``nonzero_cores()`` with *mocked* register
  reads. No ASIC access, no side effects -- safe on a production switch. This
  validates that the deployed script's clear-on-read alert logic is correct.

* **Live** (``--live``): exercises the real hardware path -- reads the actual
  ``IRE_DATA_PATH_CRC_ERROR_COUNTER`` register and runs the deployed script as
  monit would, then checks syslog / the JSON error log.
  NOTE: the register is **clear-on-read**, so a live read consumes any pending
  per-core error counts (the same side effect as a normal monit cycle).

Usage::

    sudo python3 device_test_ire_watchdog.py [--script /usr/bin/ire_watchdog]
                                             [--live] [--verbose]

Exit status is 0 if all selected tests pass, non-zero otherwise.
"""
import argparse
import importlib.machinery
import importlib.util
import json
import os
import subprocess
import sys
import tempfile
from unittest import mock

DEFAULT_SCRIPT = '/usr/bin/ire_watchdog'
SYSLOG_PATHS = ['/var/log/syslog', '/var/log/messages']
# A stable, unique fragment of INTERRUPT_LOG_MSG to grep for in syslog.
MSG_FRAGMENT = 'RQP_PACKET_REASSEMBLY_CdcPktSizeErrInt'


class Results:
    """Tiny pass/fail/skip tracker with printed output."""

    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.skipped = 0

    def ok(self, name, detail=''):
        self.passed += 1
        print('  [PASS] {}{}'.format(name, ' -- ' + detail if detail else ''))

    def fail(self, name, detail=''):
        self.failed += 1
        print('  [FAIL] {}{}'.format(name, ' -- ' + detail if detail else ''))

    def skip(self, name, detail=''):
        self.skipped += 1
        print('  [SKIP] {}{}'.format(name, ' -- ' + detail if detail else ''))


def section(title):
    print('\n=== {} ==='.format(title))


def load_module(path):
    """Load the extension-less deployed script as a module."""
    loader = importlib.machinery.SourceFileLoader('ire_watchdog', path)
    spec = importlib.util.spec_from_file_location('ire_watchdog', path,
                                                  loader=loader)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module.__name__] = module
    loader.exec_module(module)
    return module


def _grep_syslog(fragment=MSG_FRAGMENT, paths=SYSLOG_PATHS):
    """Return True if ``fragment`` appears in the tail of any syslog file."""
    for path in paths:
        if not os.path.exists(path):
            continue
        try:
            proc = subprocess.run(['tail', '-n', '1000', path],
                                  capture_output=True, text=True, timeout=10)
            if fragment in proc.stdout:
                return True
        except (OSError, subprocess.SubprocessError):
            continue
    return False


def run_simulated_tests(mod, res):
    section('Simulated logic tests (no ASIC access -- safe on production)')

    # 1. nonzero_cores() pure logic.
    try:
        assert mod.nonzero_cores({0: 0, 1: 3, 2: 0}) == [1]
        assert mod.nonzero_cores({0: 5, 1: 2}) == [0, 1]
        assert mod.nonzero_cores({0: 0, 1: 0}) == []
        res.ok('nonzero_cores() reports only non-zero cores (clear-on-read)')
    except AssertionError as e:
        res.fail('nonzero_cores()', repr(e))

    # 2. Any non-zero core -> exit 1, LOG_ERR, timestamped log entry.
    tmpdir = tempfile.mkdtemp()
    logpath = os.path.join(tmpdir, 'ire_crc_error_count.json')
    try:
        with mock.patch.object(mod, 'ERROR_LOG', logpath), \
                mock.patch.object(mod, 'read_counters',
                                  return_value={0: 0, 1: 4}), \
                mock.patch.object(mod.syslog, 'syslog') as msys:
            rc = mod.main()
        assert rc == 1, 'exit {} != 1'.format(rc)
        assert msys.call_args == mock.call(mod.syslog.LOG_ERR,
                                           mod.INTERRUPT_LOG_MSG), \
            'syslog call mismatch: {}'.format(msys.call_args)
        with open(logpath) as f:
            entries = json.load(f)
        assert len(entries) == 1 and 'timestamp' in entries[0]
        assert entries[0]['counters'] == {'0': 0, '1': 4}
        res.ok('non-zero read -> exit 1, LOG_ERR, timestamped log',
               'logged counters={}'.format(entries[0]['counters']))
    except AssertionError as e:
        res.fail('non-zero alert path', repr(e))

    # 3. All-zero read -> exit 0, no alert, no log.
    tmpdir = tempfile.mkdtemp()
    logpath = os.path.join(tmpdir, 'ire_crc_error_count.json')
    try:
        with mock.patch.object(mod, 'ERROR_LOG', logpath), \
                mock.patch.object(mod, 'read_counters',
                                  return_value={0: 0, 1: 0}), \
                mock.patch.object(mod.syslog, 'syslog') as msys:
            rc = mod.main()
        assert rc == 0, 'exit {} != 0'.format(rc)
        assert not msys.called, 'syslog called on all-zero read'
        assert not os.path.exists(logpath), 'log written on all-zero read'
        res.ok('all-zero read -> exit 0, no alert, no log')
    except AssertionError as e:
        res.fail('all-zero path', repr(e))

    # 4. Non-DNX / syncd-down / transient -> exit 0, no alert, no log.
    for label, exc in (
        ('non-DNX (PlatformNotSupported)', mod.PlatformNotSupported('not dnx')),
        ('syncd down (SyncdUnavailable)', mod.SyncdUnavailable('syncd down')),
        ('transient error (RuntimeError)', RuntimeError('boom')),
    ):
        tmpdir = tempfile.mkdtemp()
        logpath = os.path.join(tmpdir, 'ire_crc_error_count.json')
        try:
            with mock.patch.object(mod, 'ERROR_LOG', logpath), \
                    mock.patch.object(mod, 'read_counters', side_effect=exc), \
                    mock.patch.object(mod.syslog, 'syslog') as msys:
                rc = mod.main()
            assert rc == 0, 'exit {} != 0'.format(rc)
            for call in msys.call_args_list:
                assert call.args[0] != mod.syslog.LOG_ERR, 'false LOG_ERR alert'
            assert not os.path.exists(logpath), 'log written on no-op path'
            res.ok('{} -> exit 0, no alert, no log'.format(label))
        except AssertionError as e:
            res.fail(label, repr(e))


def run_live_tests(mod, script_path, res):
    section('Live hardware tests (touches ASIC -- register is CLEAR-ON-READ)')
    print('  WARNING: a live read consumes pending IRE CRC error counts.')

    print('  syncd running: {}'.format(mod.syncd_running()))

    # 1. Real register read.
    try:
        counters = mod.read_counters()
        res.ok('read_counters() succeeded',
               '{} cores, non-zero={}'.format(len(counters),
                                              mod.nonzero_cores(counters)))
        print('    counters: {}'.format(counters))
    except mod.PlatformNotSupported as e:
        res.skip('read_counters() -- not a DNX platform', str(e))
    except mod.SyncdUnavailable as e:
        res.skip('read_counters() -- syncd unavailable', str(e))
    except Exception as e:  # noqa: BLE001 - diagnostic harness
        res.fail('read_counters()', repr(e))

    # 2. Run the deployed script exactly as monit invokes it.
    try:
        proc = subprocess.run([script_path], capture_output=True, text=True,
                              timeout=30)
        rc = proc.returncode
        if rc in (0, 1):
            res.ok('deployed script ran', 'exit={}'.format(rc))
        else:
            res.fail('deployed script unexpected exit',
                     'exit={} stderr={!r}'.format(rc, proc.stderr.strip()))

        if rc == 1:
            # A real increment was seen this cycle: verify the side effects.
            if _grep_syslog():
                res.ok('LOG_ERR interrupt message found in syslog')
            else:
                res.fail('LOG_ERR interrupt message NOT found in syslog')
            if os.path.exists(mod.ERROR_LOG):
                with open(mod.ERROR_LOG) as f:
                    entries = json.load(f)
                res.ok('error log updated', '{} ({} entries)'.format(
                    mod.ERROR_LOG, len(entries)))
            else:
                res.fail('error log NOT written', mod.ERROR_LOG)
        else:
            res.skip('alert side-effects (no non-zero counters this cycle)')
    except Exception as e:  # noqa: BLE001 - diagnostic harness
        res.fail('deployed script run', repr(e))


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--script', default=DEFAULT_SCRIPT,
                        help='path to the deployed ire_watchdog (default: %(default)s)')
    parser.add_argument('--live', action='store_true',
                        help='also run live hardware tests (reads+clears the register)')
    parser.add_argument('--verbose', action='store_true', help='verbose output')
    args = parser.parse_args()

    print('ire_watchdog device test harness')
    print('script under test: {}'.format(args.script))

    if not os.path.exists(args.script):
        print('ERROR: {} not found -- is the image with ire_watchdog installed?'
              .format(args.script))
        return 2

    try:
        mod = load_module(args.script)
    except Exception as e:  # noqa: BLE001 - diagnostic harness
        print('ERROR: could not load {}: {!r}'.format(args.script, e))
        return 2

    res = Results()
    run_simulated_tests(mod, res)
    if args.live:
        run_live_tests(mod, args.script, res)
    else:
        section('Live hardware tests')
        print('  (skipped -- pass --live to read the real register; '
              'note it is clear-on-read)')

    section('Summary')
    print('  passed={} failed={} skipped={}'.format(
        res.passed, res.failed, res.skipped))
    return 0 if res.failed == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
