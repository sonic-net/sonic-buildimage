#!/usr/bin/env python3
#
# Author: Chinmoy Dey <chinmoy@nexthop.ai>
#
# Liquid Cooling implementation for the Nexthop BMC (Aspeed).
#
# Leak-detection hardware is not yet integrated into the platform API, so the
# sensors currently report only leaks injected through the vendor-independent
# leak test API (LeakageSensorTestBase): a leak injected through it is
# reported like any other leak, but is flagged as a test leak so no mitigation
# action is taken.
#
# Injected state is held in marker files under LEAK_TEST_DIR, one per sensor,
# containing the severity. That directory lives under /run, which is tmpfs on
# the BMC host, so a simulation never writes to the eMMC and never survives a
# reboot. The pmon container, where thermalctld polls the sensors, sees the
# same directory through the /run/bmc bind mount added for the aspeed platform
# in files/build_templates/docker_image_ctl.j2. Because the sensor re-reads the
# marker on every is_leak() poll, a separate process holding its own
# LiquidCooling object observes the injection, and withdrawing it returns the
# sensor to its hardware state rather than to a snapshot taken at injection
# time.
#

try:
    import os
    from sonic_platform_base.leakage_sensor_test_base import LeakageSensorTestBase
    from sonic_platform_base.liquid_cooling_base import (
        LeakageSensorBase,
        LeakSensorProfileBase,
        LeakSeverity,
        LiquidCoolingBase,
    )
    from sonic_py_common.syslogger import SysLogger
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

# Directory holding the injected test-leak markers. tmpfs on the host, shared
# with pmon through a bind mount of /run/bmc; created on demand by
# set_test_leak().
LEAK_TEST_DIR = "/run/bmc/leak_sim"

# Marker suffix written by the leak test API.
TEST_LEAK_SUFFIX = "test_leak"

# Seconds a MINOR leak may persist before it is escalated to CRITICAL.
DEFAULT_MAX_MINOR_DURATION_SEC = 300

SENSOR_TYPE = "Moisture"

# Severity reported for a hardware-detected leak; also used when an injected
# marker holds an unrecognised severity, so a malformed injection is never
# reported as the least severe leak.
HW_SEVERITY = LeakSeverity.CRITICAL

logger = SysLogger("nexthop-liquid-cooling")


def _marker_path(leak_test_dir, sensor_name):
    return os.path.join(leak_test_dir,
                        "{}.{}".format(sensor_name, TEST_LEAK_SUFFIX))


class NexthopLeakProfile(LeakSensorProfileBase):
    """Leak profile for the Nexthop BMC leak sensors."""

    def __init__(self, type=SENSOR_TYPE,
                 max_minor_duration_sec=DEFAULT_MAX_MINOR_DURATION_SEC):
        self._type = type
        self._max_minor_duration_sec = max_minor_duration_sec

    def get_type(self):
        return self._type

    def get_leak_max_minor_duration_sec(self):
        return self._max_minor_duration_sec


class NexthopLeakageSensor(LeakageSensorBase):
    """Nexthop BMC leak sensor.

    Leak-detection hardware is not yet integrated, so the only leak this sensor
    currently reports is one injected through NexthopLeakSensorTest.

    Only is_leak() reads the marker. get_leak_severity() and is_test_leak()
    are inherited from LeakageSensorBase and report the state recorded by the
    last is_leak() call, so one poll gives a consistent view of the sensor.
    """

    def __init__(self, name, location=None, profile=None,
                 leak_test_dir=LEAK_TEST_DIR):
        super().__init__(name, type=SENSOR_TYPE, location=location,
                         severity=HW_SEVERITY)
        self._profile = profile
        self._leak_test_dir = leak_test_dir

    def _marker(self):
        return _marker_path(self._leak_test_dir, self.name)

    def is_leak(self):
        """Re-derive the reported state: the hardware state (no hardware read
        path yet, so not leaking) overlaid with any injection present in the
        marker file."""
        try:
            with open(self._marker()) as f:
                severity = f.read().strip()
        except OSError:
            self.leaking = False
            self.test_leak = False
            self.leak_severity = HW_SEVERITY
            return self.leaking

        self.leaking = True
        self.test_leak = True
        try:
            self.leak_severity = LeakSeverity(severity)
        except ValueError:
            logger.log_warning(
                "sensor '{}': invalid injected severity '{}'; reporting {}".format(
                    self.name, severity, HW_SEVERITY.value))
            self.leak_severity = HW_SEVERITY
        return self.leaking

    def is_leak_sensor_ok(self):
        return True

    def get_leak_profile(self):
        return self._profile


class NexthopLeakSensorTest(LeakageSensorTestBase):
    """Leak test injection for the Nexthop BMC (Aspeed).

    Injection is non-destructive: the leak is reported and logged, and the
    sensor flags it as a test leak so no mitigation action is taken.

    is_leak_test_supported() is inherited: reaching this object through
    LiquidCooling.get_leak_sensor_test() already implies support.
    """

    def __init__(self, sensors, leak_test_dir=LEAK_TEST_DIR):
        self._sensors = {s.get_name(): s for s in sensors}
        self._leak_test_dir = leak_test_dir

    def _marker(self, sensor_name):
        return _marker_path(self._leak_test_dir, sensor_name)

    def set_test_leak(self, sensor_name, enable, *,
                      severity=LeakSeverity.MINOR):
        # Idempotent: the return value reports whether the sensor is left in
        # the requested state. Re-injecting rewrites the marker, withdrawing
        # an absent injection is a no-op, both return True.
        if sensor_name not in self._sensors:
            logger.log_error("leak test: unknown sensor '{}'".format(sensor_name))
            return False

        try:
            if enable:
                os.makedirs(self._leak_test_dir, exist_ok=True)
                with open(self._marker(sensor_name), "w") as f:
                    f.write(severity.value)
            else:
                try:
                    os.remove(self._marker(sensor_name))
                except FileNotFoundError:
                    pass
        except OSError as ex:
            logger.log_error("leak test: sensor '{}': {}".format(sensor_name, ex))
            return False

        if enable:
            logger.log_notice("leak test: sensor '{}' test leak injected (severity={})".format(
                sensor_name, severity.value))
        else:
            logger.log_notice("leak test: sensor '{}' test leak cleared".format(
                sensor_name))
        return True

    def is_test_leak_enabled(self, sensor_name):
        if sensor_name not in self._sensors:
            return False
        return os.path.exists(self._marker(sensor_name))

    def clear_test_leaks(self):
        # A list comprehension rather than a generator, so a failed withdrawal
        # does not stop the remaining sensors from being attempted.
        return all([self.set_test_leak(name, False)
                    for name in self._sensors
                    if self.is_test_leak_enabled(name)])


class LiquidCooling(LiquidCoolingBase):
    """Platform-specific Liquid Cooling class for the Nexthop BMC (Aspeed)."""

    def __init__(self, leak_test_dir=LEAK_TEST_DIR):
        profile = NexthopLeakProfile()
        sensors = [
            NexthopLeakageSensor("leakage1", location="Intake",
                                 profile=profile, leak_test_dir=leak_test_dir),
            NexthopLeakageSensor("leakage2", location="Exhaust",
                                 profile=profile, leak_test_dir=leak_test_dir),
        ]
        super().__init__(len(sensors), sensors, profiles=[profile])
        self._leak_test = NexthopLeakSensorTest(sensors,
                                                leak_test_dir=leak_test_dir)

    def get_leak_sensor_test(self):
        return self._leak_test
