#!/usr/bin/python3
#
# Author: Chinmoy Dey <chinmoy@nexthop.ai>
#
# End-to-end runtime check of simulated leak injection on the Nexthop BMC (Aspeed).
#
# Drives the leak test API exactly as a CLI would (the "injector") and observes
# the result through a second, independent LiquidCooling object polled the way
# thermalctld polls (the "poller"), so the check covers the shared marker store
# and not just in-memory state. Also runs the sibling leak_injection_sim.py
# script, and optionally verifies that the running thermalctld published the
# injected leak to STATE_DB.
#
# Usage (on the BMC host, or anywhere sonic_platform is importable):
#   test_leak_injection_sim.py                 # API + CLI checks
#   test_leak_injection_sim.py --verify-db     # also wait for thermalctld/STATE_DB
#   test_leak_injection_sim.py --leak-test-dir /tmp/leak_sim   # development host
#
# The run refuses to start if an injection is already present (use --force),
# and always withdraws every injection it made unless --keep is given.
#
# On a live system thermalctld logs the injected leak like a real one, and a
# CRITICAL injection is written to /host/bmc/event.log.
#
# Note for pytest: this is a runtime script, not a unit test. It only imports
# the standard library at module level and defines no test_* functions, so
# being collected by the package's pytest run is harmless.
#

import argparse
import json
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import leak_injection_sim as sim  # noqa: E402

PASS, FAIL, SKIP = "PASS", "FAIL", "SKIP"


class Report:
    def __init__(self):
        self.results = []

    def record(self, status, name, detail=""):
        self.results.append((status, name, detail))
        line = "[{}] {}".format(status, name)
        if detail:
            line += ": {}".format(detail)
        print(line)

    def check(self, name, condition, detail=""):
        self.record(PASS if condition else FAIL, name, "" if condition else detail)
        return bool(condition)

    def failed(self):
        return any(s == FAIL for s, _, _ in self.results)

    def summary(self):
        counts = {s: sum(1 for r in self.results if r[0] == s) for s in (PASS, FAIL, SKIP)}
        return "{} passed, {} failed, {} skipped".format(counts[PASS], counts[FAIL], counts[SKIP])


def poll(lc, name):
    """Poll one sensor the way thermalctld does: is_leak() first, then the
    accessors that report that poll."""
    sensor = next(s for s in lc.get_all_leak_sensors() if s.get_name() == name)
    return {
        "leaking": sensor.is_leak(),
        "test_leak": sensor.is_test_leak(),
        "severity": sensor.get_leak_severity(),
        "ok": sensor.is_leak_sensor_ok(),
    }


def marker(module, leak_test_dir, name):
    return os.path.join(leak_test_dir, "{}.{}".format(name, module.TEST_LEAK_SUFFIX))


def check_interface(report, injector, poller):
    leak_test = injector.get_leak_sensor_test()
    if not report.check("leak test interface is exposed", leak_test is not None,
                        "get_leak_sensor_test() returned None"):
        return None
    report.check("leak test is supported", leak_test.is_leak_test_supported())
    report.check("same sensors seen by injector and poller",
                 sim.sensor_names(injector) == sim.sensor_names(poller),
                 "{} vs {}".format(sim.sensor_names(injector), sim.sensor_names(poller)))
    return leak_test


def check_baseline(report, poller, leak_test, names):
    clean = all(not poll(poller, n)["leaking"] for n in names)
    report.check("no leak reported before injection", clean)
    report.check("no injection present before test",
                 not any(leak_test.is_test_leak_enabled(n) for n in names))
    report.check("severity is None when not leaking",
                 all(poll(poller, n)["severity"] is None for n in names))


def check_contract(report, leak_test, names):
    LeakSeverity = sim.severity_from_name("MINOR").__class__

    report.check("unknown sensor: set_test_leak returns False",
                 not leak_test.set_test_leak("no_such_sensor", True))
    report.check("unknown sensor: is_test_leak_enabled returns False",
                 not leak_test.is_test_leak_enabled("no_such_sensor"))
    try:
        leak_test.set_test_leak(names[0], True, LeakSeverity.CRITICAL)
        positional_rejected = False
        leak_test.set_test_leak(names[0], False)
    except TypeError:
        positional_rejected = True
    report.check("severity is keyword-only", positional_rejected)
    report.check("clear_test_leaks with nothing injected returns True",
                 leak_test.clear_test_leaks())


def check_sensor_cycle(report, module, leak_test, poller, leak_test_dir, name):
    LeakSeverity = sim.severity_from_name("MINOR").__class__
    tag = "{}: ".format(name)

    report.check(tag + "inject with default severity returns True",
                 leak_test.set_test_leak(name, True))
    report.check(tag + "is_test_leak_enabled after inject",
                 leak_test.is_test_leak_enabled(name))
    path = marker(module, leak_test_dir, name)
    report.check(tag + "marker file holds MINOR",
                 os.path.isfile(path) and open(path).read().strip() == "MINOR",
                 "marker {} missing or wrong".format(path))

    seen = poll(poller, name)
    report.check(tag + "poller reports leaking", seen["leaking"])
    report.check(tag + "poller flags it as a test leak", seen["test_leak"])
    report.check(tag + "poller reports MINOR", seen["severity"] is LeakSeverity.MINOR,
                 "got {}".format(seen["severity"]))
    report.check(tag + "sensor stays healthy while injected", seen["ok"])
    leaking = [s.get_name() for s in poller.get_leak_sensor_status()]
    report.check(tag + "only this sensor is in get_leak_sensor_status()",
                 leaking == [name], "got {}".format(leaking))

    report.check(tag + "re-inject as CRITICAL returns True",
                 leak_test.set_test_leak(name, True, severity=LeakSeverity.CRITICAL))
    seen = poll(poller, name)
    report.check(tag + "poller reports CRITICAL after re-inject",
                 seen["severity"] is LeakSeverity.CRITICAL, "got {}".format(seen["severity"]))

    report.check(tag + "withdraw returns True", leak_test.set_test_leak(name, False))
    report.check(tag + "withdraw again (idempotent) returns True",
                 leak_test.set_test_leak(name, False))
    report.check(tag + "marker removed", not os.path.exists(path))
    seen = poll(poller, name)
    report.check(tag + "poller reports no leak after withdraw", not seen["leaking"])
    report.check(tag + "test flag cleared after withdraw", not seen["test_leak"])
    report.check(tag + "severity None after withdraw", seen["severity"] is None)


def check_clear_all(report, leak_test, poller, names):
    for name in names:
        leak_test.set_test_leak(name, True)
    report.check("all sensors leaking after injecting all",
                 sorted(s.get_name() for s in poller.get_leak_sensor_status()) == sorted(names))
    report.check("clear_test_leaks returns True", leak_test.clear_test_leaks())
    report.check("no injection remains after clear_test_leaks",
                 not any(leak_test.is_test_leak_enabled(n) for n in names))
    report.check("no leak reported after clear_test_leaks",
                 poller.get_leak_sensor_status() == [])


def check_cli(report, poller, leak_test_dir, names):
    script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "leak_injection_sim.py")
    if not os.path.isfile(script):
        report.record(SKIP, "CLI script", "{} not found".format(script))
        return
    base = [sys.executable, script, "--leak-test-dir", leak_test_dir]
    name = names[0]

    def run(*args):
        return subprocess.run(base + list(args), capture_output=True, text=True)

    r = run("list")
    report.check("CLI list prints the sensors",
                 r.returncode == 0 and r.stdout.split() == names, r.stdout + r.stderr)

    r = run("inject", name, "--severity", "critical")
    report.check("CLI inject exits 0", r.returncode == 0, r.stdout + r.stderr)
    seen = poll(poller, name)
    report.check("CLI injection observed by poller as CRITICAL test leak",
                 seen["leaking"] and seen["test_leak"]
                 and seen["severity"] is sim.severity_from_name("CRITICAL"))

    r = run("status", "--json")
    try:
        rows = {row["name"]: row for row in json.loads(r.stdout)}
        row = rows[name]
        status_ok = (row["injected"] and row["leaking"] and row["test_leak"]
                     and row["severity"] == "CRITICAL")
    except Exception as ex:  # noqa: BLE001
        status_ok, row = False, repr(ex)
    report.check("CLI status --json reflects the injection", status_ok, str(row))

    r = run("inject", "no_such_sensor")
    report.check("CLI inject on unknown sensor exits non-zero", r.returncode != 0)

    r = run("clear", "all")
    report.check("CLI clear all exits 0", r.returncode == 0, r.stdout + r.stderr)
    report.check("CLI clear observed by poller", poller.get_leak_sensor_status() == [])


def check_state_db(report, leak_test, names, timeout):
    """Verify the running thermalctld publishes the injection to STATE_DB.

    thermalctld does not yet publish is_test_leak, so only the leaking flag,
    the severity and the system status are checked.
    """
    try:
        from swsscommon.swsscommon import SonicV2Connector
    except ImportError:
        report.record(SKIP, "STATE_DB verification", "swsscommon not importable here")
        return
    try:
        db = SonicV2Connector(use_unix_socket_path=True)
        db.connect(db.STATE_DB)
    except Exception as ex:  # noqa: BLE001
        report.record(SKIP, "STATE_DB verification", "cannot connect: {}".format(ex))
        return

    name = names[0]

    def wait_for(predicate):
        deadline = time.monotonic() + timeout
        while True:
            entry = db.get_all(db.STATE_DB, "LIQUID_COOLING_INFO|{}".format(name)) or {}
            system = db.get_all(db.STATE_DB, "SYSTEM_LEAK_STATUS|system") or {}
            if predicate(entry, system):
                return True, entry, system
            if time.monotonic() >= deadline:
                return False, entry, system
            time.sleep(1)

    if not db.exists(db.STATE_DB, "LIQUID_COOLING_INFO|{}".format(name)):
        report.record(SKIP, "STATE_DB verification",
                      "LIQUID_COOLING_INFO|{} absent; is thermalctld running with "
                      "liquid cooling enabled?".format(name))
        return

    leak_test.set_test_leak(name, True, severity=sim.severity_from_name("CRITICAL"))
    ok, entry, system = wait_for(
        lambda e, s: e.get("leaking") == "Yes" and "CRITICAL" in e.get("leak_severity", "")
        and s.get("device_leak_status") == "CRITICAL")
    report.check("STATE_DB shows CRITICAL leak on {} within {}s".format(name, timeout), ok,
                 "sensor={} system={}".format(entry, system))

    leak_test.set_test_leak(name, False)
    ok, entry, system = wait_for(
        lambda e, s: e.get("leaking") == "No" and s.get("device_leak_status") == "None")
    report.check("STATE_DB clears within {}s after withdraw".format(timeout), ok,
                 "sensor={} system={}".format(entry, system))


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="End-to-end check of simulated leak injection on the Nexthop BMC.")
    parser.add_argument("--leak-test-dir", metavar="DIR",
                        help="Marker directory to test against instead of the platform "
                             "default; bypasses Chassis().")
    parser.add_argument("--verify-db", action="store_true",
                        help="Also wait for the running thermalctld to publish the "
                             "injection to STATE_DB.")
    parser.add_argument("--db-timeout", type=int, default=60, metavar="SEC",
                        help="How long to wait for STATE_DB updates (default 60).")
    parser.add_argument("--force", action="store_true",
                        help="Run even if an injection is already present (it is cleared).")
    parser.add_argument("--keep", action="store_true",
                        help="Do not withdraw injections at the end.")
    args = parser.parse_args(argv)

    module = sim.load_liquid_cooling_module()
    leak_test_dir = args.leak_test_dir or module.LEAK_TEST_DIR
    injector = sim.get_liquid_cooling(args.leak_test_dir)
    poller = module.LiquidCooling(leak_test_dir=leak_test_dir)
    names = sim.sensor_names(injector)
    report = Report()
    print("leak test dir: {}".format(leak_test_dir))
    print("sensors: {}".format(", ".join(names)))

    leak_test = check_interface(report, injector, poller)
    if leak_test is None:
        print(report.summary())
        return 2

    if any(leak_test.is_test_leak_enabled(n) for n in names):
        if not args.force:
            print("an injection is already present; clear it or pass --force")
            return 2
        leak_test.clear_test_leaks()

    try:
        check_baseline(report, poller, leak_test, names)
        check_contract(report, leak_test, names)
        for name in names:
            check_sensor_cycle(report, module, leak_test, poller, leak_test_dir, name)
        check_clear_all(report, leak_test, poller, names)
        check_cli(report, poller, leak_test_dir, names)
        if args.verify_db:
            check_state_db(report, leak_test, names, args.db_timeout)
        else:
            report.record(SKIP, "STATE_DB verification", "pass --verify-db to enable")
    finally:
        if not args.keep:
            leak_test.clear_test_leaks()

    print(report.summary())
    return 1 if report.failed() else 0


if __name__ == "__main__":
    sys.exit(main())
