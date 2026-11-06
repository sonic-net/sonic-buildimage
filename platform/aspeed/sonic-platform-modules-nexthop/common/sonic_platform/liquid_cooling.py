#!/usr/bin/env python3
#
# Author: Chinmoy Dey <chinmoy@nexthop.ai>
#
# Liquid Cooling implementation for the Nexthop BMC (Aspeed).
#
# Each sensor reads its leak state from a BMC GPIO line (see leak_detect.py for
# the sensor inventory and the line decoding). On top of that hardware state,
# a leak can be injected through the vendor-independent leak test API
# (LeakageSensorTestBase): an injected leak is reported like any other leak,
# but is flagged as a test leak so no mitigation action is taken. A leak
# reported by the hardware always takes precedence and is never flagged as a
# test leak.
#
# Injected state is held in marker files under LEAK_TEST_DIR, one per sensor,
# containing the severity. That directory lives under /run, which is tmpfs on
# the BMC host, so a simulation never writes to the eMMC and never survives a
# reboot. The pmon container, where thermalctld polls the sensors, sees the
# same directory through the /run/bmc bind mount added for the aspeed platform
# in files/build_templates/docker_image_ctl.j2. Because the sensor re-reads the
# line and the marker on every is_leak() poll, a separate process holding its
# own LiquidCooling object observes the injection, and withdrawing it returns
# the sensor to its hardware state rather than to a snapshot taken at
# injection time.
#

try:
    import os
    import subprocess

    from sonic_platform_base.leakage_sensor_test_base import LeakageSensorTestBase
    from sonic_platform_base.liquid_cooling_base import (
        LeakageSensorBase,
        LeakSensorProfileBase,
        LeakSeverity,
        LiquidCoolingBase,
    )
    from sonic_py_common.syslogger import SysLogger

    from sonic_platform import leak_detect
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

# Directory holding the injected test-leak markers. tmpfs on the host, shared
# with pmon through a bind mount of /run/bmc; created on demand by
# set_test_leak().
LEAK_TEST_DIR = "/run/bmc/leak_sim"

# Marker suffix written by the leak test API.
TEST_LEAK_SUFFIX = "test_leak"

# Seconds a MINOR leak may persist before it is escalated.
DEFAULT_MAX_MINOR_DURATION_SEC = 300

# Severity reported when an injected marker holds an unrecognised severity, so
# a malformed injection is never reported as the least severe leak.
INVALID_INJECTION_SEVERITY = LeakSeverity.CRITICAL

# Shelling out rather than binding libgpiod: the other GPIO consumers on this
# platform do the same, and no Python libgpiod bindings ship in the image.
GPIOGET = "gpioget"
GPIO_CONSUMER = "leak-detect"
GPIOGET_TIMEOUT_SEC = 5

logger = SysLogger("nexthop-liquid-cooling")


def _marker_path(leak_test_dir, sensor_name):
    return os.path.join(leak_test_dir,
                        "{}.{}".format(sensor_name, TEST_LEAK_SUFFIX))


def read_gpio_line(name):
    """
    Read one GPIO line by its device-tree gpio-line-names entry.

    --strict makes a duplicate line name an error rather than a coin flip over
    which line gets read.

    Returns:
        str: raw gpioget stdout.

    Raises:
        leak_detect.LeakLineError: the line is absent from this DTB, gpioget is
            missing, the line is claimed by another consumer such as a
            gpio-hog, or the read timed out.
    """
    cmd = [GPIOGET, "--strict", "--numeric", "--consumer", GPIO_CONSUMER, name]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True,
                              timeout=GPIOGET_TIMEOUT_SEC, check=False)
    except FileNotFoundError as e:
        raise leak_detect.LeakLineError("{} not installed: {}".format(GPIOGET, e))
    except subprocess.TimeoutExpired:
        raise leak_detect.LeakLineError(
            "{} timed out reading {}".format(GPIOGET, name))

    if proc.returncode != 0:
        raise leak_detect.LeakLineError(
            "{} failed for line {!r} (rc={}): {}".format(
                GPIOGET, name, proc.returncode, (proc.stderr or "").strip()))
    return proc.stdout


class NexthopLeakProfile(LeakSensorProfileBase):
    """Leak profile for one type of Nexthop BMC leak sensor."""

    def __init__(self, type,
                 max_minor_duration_sec=DEFAULT_MAX_MINOR_DURATION_SEC):
        self._type = type
        self._max_minor_duration_sec = max_minor_duration_sec

    def get_type(self):
        return self._type

    def get_leak_max_minor_duration_sec(self):
        return self._max_minor_duration_sec


class NexthopLeakageSensor(LeakageSensorBase):
    """Nexthop BMC leak sensor, backed by one BMC GPIO line.

    Severity is fixed per sensor rather than derived from the reading: the line
    is a discrete wet/dry level, and the classification belongs to the sensor
    position. Escalation of a persistent MINOR leak is thermalctld's job.

    No debounce is applied here. thermalctld polls on its own cadence while a
    show command calls the same accessor ad hoc, so a filter here would hand
    one of them half-formed state. Debounce belongs on the GPIO line itself.

    Only is_leak() reads the line and the marker. get_leak_severity() and
    is_test_leak() are inherited from LeakageSensorBase and report the state
    recorded by the last is_leak() call, so one poll gives a consistent view of
    the sensor.
    """

    def __init__(self, spec, profile=None, leak_test_dir=LEAK_TEST_DIR,
                 read_line=None):
        """
        Args:
            spec: one entry of leak_detect.LEAK_SENSORS.
            profile: the NexthopLeakProfile for this sensor's type.
            leak_test_dir: directory holding the injected test-leak markers.
            read_line: callable taking a gpio-line-name and returning raw
                       gpioget output. Defaults to running gpioget.
        """
        self._hw_severity = LeakSeverity(spec["severity"])
        super().__init__(spec["name"], type=spec["type"],
                         location=spec["location"], severity=self._hw_severity)
        self.line = spec["line"]
        self._profile = profile
        self._leak_test_dir = leak_test_dir
        self._read_line = read_line or read_gpio_line
        self._read_error = None

    def _marker(self):
        return _marker_path(self._leak_test_dir, self.name)

    def _hw_leaking(self):
        """
        Returns:
            bool or None: the line's wet/dry state, or None when it could not
            be read. An unreadable line is a faulty sensor, never a clean dry
            reading, so an absent DTB entry or a line held by a gpio-hog stays
            visible.
        """
        try:
            leaking = leak_detect.is_leaking(self._read_line(self.line))
        except Exception as ex:  # noqa: BLE001
            # Logged once per distinct error, not on every poll.
            if str(ex) != self._read_error:
                self._read_error = str(ex)
                logger.log_warning("sensor '{}': cannot read line '{}': {}".format(
                    self.name, self.line, ex))
            return None
        self._read_error = None
        return leaking

    def _injected_severity(self):
        """
        Returns:
            LeakSeverity or None: the injected severity, or None when no leak
            is injected on this sensor.
        """
        try:
            with open(self._marker()) as f:
                severity = f.read().strip()
        except OSError:
            return None
        try:
            return LeakSeverity(severity)
        except ValueError:
            logger.log_warning(
                "sensor '{}': invalid injected severity '{}'; reporting {}".format(
                    self.name, severity, INVALID_INJECTION_SEVERITY.value))
            return INVALID_INJECTION_SEVERITY

    def is_leak(self):
        """Re-derive the reported state: the hardware state of the line,
        overlaid with any injection present in the marker file."""
        hw = self._hw_leaking()
        self.leak_sensor_ok = hw is not None
        injected = self._injected_severity()

        if hw:
            self.leaking = True
            self.test_leak = False
            self.leak_severity = self._hw_severity
        elif injected is not None:
            self.leaking = True
            self.test_leak = True
            self.leak_severity = injected
        else:
            self.leaking = False
            self.test_leak = False
            self.leak_severity = self._hw_severity
        return self.leaking

    def is_leak_sensor_ok(self):
        """
        Whether the sensor can be trusted.

        The GPIO line carries no sensor-health information, so a readable line
        is reported as OK. This detects an unreadable line, not a damaged
        sensor.
        """
        self.leak_sensor_ok = self._hw_leaking() is not None
        return self.leak_sensor_ok

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

    def __init__(self, leak_test_dir=LEAK_TEST_DIR, read_line=None):
        """
        Args:
            leak_test_dir: directory holding the injected test-leak markers.
            read_line: callable taking a gpio-line-name and returning raw
                       gpioget output. For testing; defaults to running
                       gpioget.
        """
        # One profile per sensor type, so the MINOR-persistence escalation
        # applies uniformly to sensors sharing a type.
        profiles = {}
        sensors = []
        for spec in leak_detect.LEAK_SENSORS:
            profile = profiles.setdefault(spec["type"],
                                          NexthopLeakProfile(spec["type"]))
            sensors.append(NexthopLeakageSensor(
                spec, profile=profile, leak_test_dir=leak_test_dir,
                read_line=read_line))
        super().__init__(len(sensors), sensors, profiles=list(profiles.values()))
        self._leak_test = NexthopLeakSensorTest(sensors,
                                                leak_test_dir=leak_test_dir)

    def get_leak_sensor_test(self):
        return self._leak_test
