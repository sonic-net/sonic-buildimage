#!/usr/bin/python3
#
# Author: Chinmoy Dey <chinmoy@nexthop.ai>
#
# Inject, withdraw and show simulated leaks on the Nexthop BMC (Aspeed) through
# the vendor-independent leak test API (LeakageSensorTestBase).
#
# An injected leak is reported by the platform like any other leak, so
# thermalctld publishes it to STATE_DB and logs it, but the sensor flags it as a
# test leak so no mitigation action is taken. A CRITICAL injection is still
# written to /host/bmc/event.log by thermalctld as a leak event.
#
# Usage (on the BMC host, or anywhere sonic_platform is importable):
#   leak_injection_sim.py list
#   leak_injection_sim.py status [--json]
#   leak_injection_sim.py inject leakage1 [--severity MINOR|CRITICAL]
#   leak_injection_sim.py inject all --severity CRITICAL
#   leak_injection_sim.py clear leakage1
#   leak_injection_sim.py clear all
#
# --leak-test-dir DIR redirects the marker directory (default
# /run/bmc/leak_sim) and bypasses the Chassis object, which is useful on a
# development host or to inspect a copied marker directory.
#

import argparse
import importlib.util
import json
import os
import sys
import types

EXIT_OK = 0
EXIT_FAIL = 1
EXIT_UNSUPPORTED = 2

SEVERITIES = ("MINOR", "CRITICAL")


def _bind_source_tree():
    """Make the platform module importable when running from the source tree.

    On the BMC the sonic_platform wheel is installed and this is never needed.
    On a development host, bind sonic_platform_base to the sonic-platform-common
    submodule and stub the syslogger, the same way the unit tests do.
    """
    here = os.path.dirname(os.path.abspath(__file__))
    common_dir = os.path.dirname(here)
    repo_root = os.path.abspath(os.path.join(common_dir, *([os.pardir] * 4)))
    base_dir = os.path.join(repo_root, "src", "sonic-platform-common",
                            "sonic_platform_base")
    module_path = os.path.join(common_dir, "sonic_platform", "liquid_cooling.py")
    if not (os.path.isdir(base_dir) and os.path.isfile(module_path)):
        return None

    try:
        import sonic_platform_base  # noqa: F401
    except ImportError:
        base = types.ModuleType("sonic_platform_base")
        base.__path__ = [base_dir]
        sys.modules["sonic_platform_base"] = base

    try:
        import sonic_py_common.syslogger  # noqa: F401
    except ImportError:
        class SysLogger:
            def __init__(self, ident="leak_injection_sim", *args, **kwargs):
                self._ident = ident

            def _emit(self, level, msg):
                sys.stderr.write("{} {}: {}\n".format(self._ident, level, msg))

            def log_error(self, msg):
                self._emit("ERR", msg)

            def log_warning(self, msg):
                self._emit("WARNING", msg)

            def log_notice(self, msg):
                self._emit("NOTICE", msg)

            def log_info(self, msg):
                self._emit("INFO", msg)

        py_common = types.ModuleType("sonic_py_common")
        syslogger = types.ModuleType("sonic_py_common.syslogger")
        syslogger.SysLogger = SysLogger
        py_common.syslogger = syslogger
        sys.modules["sonic_py_common"] = py_common
        sys.modules["sonic_py_common.syslogger"] = syslogger

    spec = importlib.util.spec_from_file_location("nexthop_liquid_cooling",
                                                  module_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_liquid_cooling_module():
    """Return the platform liquid_cooling module, installed or from source."""
    try:
        from sonic_platform import liquid_cooling
        return liquid_cooling
    except ImportError as installed_err:
        module = _bind_source_tree()
        if module is None:
            raise SystemExit(
                "sonic_platform is not importable ({}) and no source tree was "
                "found next to this script".format(installed_err))
        return module


def get_liquid_cooling(leak_test_dir=None):
    """The LiquidCooling object to operate on.

    Without --leak-test-dir this goes through Chassis().get_liquid_cooling(),
    exercising the same wiring thermalctld uses. With it, a LiquidCooling is
    built directly on the given marker directory.
    """
    if leak_test_dir is None:
        try:
            from sonic_platform.chassis import Chassis
            return Chassis().get_liquid_cooling()
        except ImportError:
            # Source tree without an installed platform package: fall back to
            # the platform default directory.
            leak_test_dir = load_liquid_cooling_module().LEAK_TEST_DIR
    return load_liquid_cooling_module().LiquidCooling(leak_test_dir=leak_test_dir)


def get_leak_test(lc):
    leak_test = lc.get_leak_sensor_test()
    if leak_test is None or not leak_test.is_leak_test_supported():
        raise SystemExit(EXIT_UNSUPPORTED)
    return leak_test


def severity_from_name(name):
    from sonic_platform_base.liquid_cooling_base import LeakSeverity
    return LeakSeverity(name.upper())


def sensor_names(lc):
    """Names of all leak sensors, in platform order."""
    return [s.get_name() for s in lc.get_all_leak_sensors()]


def resolve_sensors(lc, selector):
    """Expand a command-line sensor selector into sensor names.

    'all' selects every sensor; anything else must be a known sensor name.
    Exits with EXIT_FAIL, listing the known sensors, when it is not.
    """
    names = sensor_names(lc)
    if selector == "all":
        return names
    if selector not in names:
        sys.stderr.write("unknown sensor '{}'; known sensors: {}\n".format(
            selector, ", ".join(names)))
        raise SystemExit(EXIT_FAIL)
    return [selector]


def severity_name(severity):
    """The string form of a LeakSeverity, or None when there is no leak."""
    return severity.value if severity is not None else None


def yes_no(flag):
    """Table cell for a boolean."""
    return "Yes" if flag else "No"


def sensor_status(lc, leak_test, leak_test_dir):
    """One dict per sensor, filled in the order thermalctld polls: is_leak()
    first, so the accessors after it report the same poll."""
    suffix = load_liquid_cooling_module().TEST_LEAK_SUFFIX
    return [{
        "name": sensor.get_name(),
        "location": sensor.get_leak_sensor_location(),
        "type": sensor.get_leak_sensor_type(),
        "sensor_ok": sensor.is_leak_sensor_ok(),
        "leaking": sensor.is_leak(),
        "test_leak": sensor.is_test_leak(),
        "severity": severity_name(sensor.get_leak_severity()),
        "injected": leak_test.is_test_leak_enabled(sensor.get_name()),
        "marker": os.path.join(leak_test_dir, "{}.{}".format(sensor.get_name(), suffix)),
    } for sensor in lc.get_all_leak_sensors()]


def print_status(rows, as_json):
    if as_json:
        print(json.dumps(rows, indent=2))
        return
    header = ("SENSOR", "LOCATION", "OK", "LEAKING", "TEST", "SEVERITY", "INJECTED")
    table = [header] + [(
        r["name"],
        r["location"],
        yes_no(r["sensor_ok"]),
        yes_no(r["leaking"]),
        yes_no(r["test_leak"]),
        r["severity"] or "-",
        yes_no(r["injected"]),
    ) for r in rows]
    widths = [max(len(row[i]) for row in table) for i in range(len(header))]
    for row in table:
        print("  ".join(cell.ljust(widths[i]) for i, cell in enumerate(row)))


def cmd_list(args, lc, leak_test):
    for name in sensor_names(lc):
        print(name)
    return EXIT_OK


def cmd_status(args, lc, leak_test):
    rows = sensor_status(lc, leak_test, args.leak_test_dir)
    print_status(rows, args.json)
    return EXIT_OK


def cmd_inject(args, lc, leak_test):
    severity = severity_from_name(args.severity)
    ok = True
    for name in resolve_sensors(lc, args.sensor):
        applied = leak_test.set_test_leak(name, True, severity=severity)
        print("{}: test leak {} (severity={})".format(
            name, "injected" if applied else "INJECTION FAILED", severity.value))
        ok = ok and applied
    return EXIT_OK if ok else EXIT_FAIL


def cmd_clear(args, lc, leak_test):
    if args.sensor == "all":
        ok = leak_test.clear_test_leaks()
        print("all test leaks {}".format("cleared" if ok else "NOT cleared"))
        return EXIT_OK if ok else EXIT_FAIL
    ok = True
    for name in resolve_sensors(lc, args.sensor):
        applied = leak_test.set_test_leak(name, False)
        print("{}: test leak {}".format(name, "cleared" if applied else "NOT cleared"))
        ok = ok and applied
    return EXIT_OK if ok else EXIT_FAIL


def build_parser():
    parser = argparse.ArgumentParser(
        description="Inject and withdraw simulated leaks through the Nexthop BMC leak test API.")
    parser.add_argument("--leak-test-dir", metavar="DIR",
                        help="Marker directory to operate on instead of the platform "
                             "default; bypasses Chassis().")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("list", help="List the leak sensor names.").set_defaults(func=cmd_list)

    p = sub.add_parser("status", help="Show per-sensor leak and injection state.")
    p.add_argument("--json", action="store_true", help="Machine-readable output.")
    p.set_defaults(func=cmd_status)

    p = sub.add_parser("inject", help="Inject a test leak.")
    p.add_argument("sensor", help="Sensor name, or 'all'.")
    p.add_argument("--severity", choices=SEVERITIES, default="MINOR",
                   type=str.upper,
                   help="Severity the injected leak is reported with (default MINOR).")
    p.set_defaults(func=cmd_inject)

    p = sub.add_parser("clear", help="Withdraw a test leak.")
    p.add_argument("sensor", help="Sensor name, or 'all'.")
    p.set_defaults(func=cmd_clear)
    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)
    lc = get_liquid_cooling(args.leak_test_dir)
    if args.leak_test_dir is None:
        args.leak_test_dir = load_liquid_cooling_module().LEAK_TEST_DIR
    try:
        leak_test = get_leak_test(lc)
    except SystemExit as e:
        if e.code == EXIT_UNSUPPORTED:
            sys.stderr.write("this platform does not support leak test injection\n")
        raise
    return args.func(args, lc, leak_test)


if __name__ == "__main__":
    sys.exit(main())
