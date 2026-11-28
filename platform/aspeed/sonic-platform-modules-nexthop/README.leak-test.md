# Leak test API — how to test it

Author: Chinmoy Dey <chinmoy@nexthop.ai>

The leak test API lets a simulated leak be injected through the platform API so
the leak reporting path can be exercised without wetting hardware.

Injection is **non-destructive**: an injected leak is reported like any other
leak and is additionally flagged as a test leak (`is_test_leak()`), so consumers
must not take a mitigation action on it.

## Interfaces

Defined in `sonic-platform-common`
(`sonic_platform_base/liquid_cooling_base.py` and
`sonic_platform_base/leakage_sensor_test_base.py`):

| API | Purpose |
| --- | --- |
| `LiquidCoolingBase.get_leak_sensor_test()` | Returns the platform leak test object, or `None` if unsupported |
| `LeakageSensorTestBase.is_leak_test_supported()` | Whether injection is supported (defaults to `True`; unsupported platforms return `None` from `get_leak_sensor_test()`) |
| `LeakageSensorTestBase.set_test_leak(name, enable, *, severity=MINOR)` | Inject/withdraw a test leak. Idempotent; `False` for an unknown sensor |
| `LeakageSensorTestBase.is_test_leak_enabled(name)` | Whether a test leak is injected |
| `LeakageSensorTestBase.clear_test_leaks()` | Withdraw all injected test leaks |
| `LeakageSensorBase.is_test_leak()` | Whether the reported leak came from injection |

`severity` is a `LeakSeverity` enum: `MINOR`, `CRITICAL`. It is keyword-only
and defaults to `MINOR`, so an injection that omits it never selects the
severity mapped to the most destructive mitigation action.

The Nexthop implementation lives in
`common/sonic_platform/liquid_cooling.py`. Injected state is held in marker
files under `/run/bmc/leak_sim/<sensor>.test_leak` containing the severity.
`/run` is tmpfs on the BMC host, so a simulation never writes to the eMMC and
is cleared by a reboot. pmon sees the same directory through the `/run/bmc`
bind mount added for the aspeed platform in
`files/build_templates/docker_image_ctl.j2`. The sensor re-reads its GPIO line and
marker on every poll, so the state is visible to any process holding its own
`LiquidCooling` object, and withdrawing an injection returns the sensor to its
hardware state. `is_leak_sensor_ok()` and `is_leak()` called back to back share
one read, and `get_leak_severity()` and `is_test_leak()` report that same read,
so a poll never mixes health from one read with leak state from another.

## Unit tests

`debian/rules` runs both platform test directories at package build time:
`tests/` (watchdog) and `common/tests/` (leak sensors, leak test API, liquid
cooling detection, switch host module). To run the same set by hand:

```
cd platform/aspeed/sonic-platform-modules-nexthop
PYTHONDONTWRITEBYTECODE=1 python3 -m pytest tests/ common/tests/
```

The leak test coverage is in `common/tests/test_liquid_cooling.py`. It redirects
the marker directory to a tmp path and stubs syslog, so no BMC hardware is
required.

`TestCoherentSample` checks that health, leak state, severity and test
provenance come from one GPIO read when polled in thermalctld's order
(`is_leak_sensor_ok()`, then `is_leak()`), including a line that fails or
asserts between the two calls.

`TestLeakTestApi` in that file mirrors the expectations of the base test suite
in `src/sonic-platform-common/tests/leakage_sensor_test_base_test.py`
(keyword-only severity defaulting to `MINOR`, idempotent `set_test_leak()`,
`False` for unknown sensors, injected leaks flagged via `is_test_leak()`), so
the Nexthop implementation is validated against the shared leak test API
contract. `TestMarkerFiles` covers the marker-file backing store.

Base class coverage is in the `sonic-platform-common` submodule:

```
cd src/sonic-platform-common
PYTHONPATH=../sonic-py-common python3 -m pytest \
    tests/liquid_cooling_base_test.py tests/leakage_sensor_test_base_test.py
```

## Runtime test on the BMC

There is no CLI for leak injection yet. Two scripts in `common/scripts`,
installed to `/usr/local/bin` by the platform package, fill that gap. Both go
through `Chassis().get_liquid_cooling()`, the same wiring thermalctld uses.

`leak_injection_sim.py` injects, withdraws and shows simulated leaks:

```
leak_injection_sim.py list
leak_injection_sim.py status [--json]
leak_injection_sim.py inject coldplate_leak [--severity MINOR|CRITICAL]   # default MINOR
leak_injection_sim.py inject all --severity CRITICAL
leak_injection_sim.py clear coldplate_leak
leak_injection_sim.py clear all
```

Exit status is 0 on success, 1 if an injection or withdrawal failed (e.g. an
unknown sensor), 2 if the platform reports no leak test support.

`test_leak_injection_sim.py` is an end-to-end check. It drives the API as an
injector and observes the result through a second, independent `LiquidCooling`
object polled the way thermalctld polls, so the shared marker store is covered.
It also exercises `leak_injection_sim.py` itself. It refuses to run if an
injection is already present (`--force` clears it first) and withdraws every
injection it made at the end (`--keep` to leave them):

```
test_leak_injection_sim.py               # API + CLI checks, prints PASS/FAIL per check
test_leak_injection_sim.py --verify-db   # also wait for thermalctld to publish to STATE_DB
```

On the platform marker directory, which thermalctld watches, the script
injects nothing until it has confirmed that thermalctld marks injected leaks
as test leaks: every `LIQUID_COOLING_INFO|<sensor>` row must carry `test_leak`
and `leak_source`, and `SYSTEM_LEAK_STATUS|system` must carry `test_leak`.
That check runs before the first injection of the run. If `swsscommon` is not
importable, STATE_DB is unreachable, thermalctld is not publishing liquid
cooling, or it predates sonic-platform-daemons#943, the script exits 2
without writing a marker. An older thermalctld reports an injected CRITICAL
leak in `device_leak_status` without marking it as a test leak, and bmcctld's
default policy powers the switch off on that.

With `--verify-db` it also waits (default 60 s, `--db-timeout`) for
`LIQUID_COOLING_INFO|<sensor>` to show `leaking=Yes`, `test_leak=Enabled`,
`leak_source=test` and the injected `test_leak_severity`, and
`SYSTEM_LEAK_STATUS|system` to show `device_leak_status=CRITICAL` with
`test_leak=Enabled`, then for both to clear after withdrawal.

Both scripts accept `--leak-test-dir DIR` to operate on another marker
directory. No daemon watches it, so the verifier skips the consumer check and
the STATE_DB verification there. The sensors still read their GPIO lines, so
this is not a hardware-free mode on the BMC. It bypasses `Chassis()` and, on a
development host without the platform package installed, loads the module
from the source tree the way the unit tests do:

```
cd platform/aspeed/sonic-platform-modules-nexthop/common
python3 scripts/test_leak_injection_sim.py --leak-test-dir /tmp/leak_sim
```

## How thermalctld handles a test leak

A test leak is reported like a real one but never triggers a power action
(sonic-net/sonic-platform-daemons#943). The sensor's `LIQUID_COOLING_INFO` row
shows `leaking=Yes`, `test_leak=Enabled`, `leak_source=test` and the injected
`test_leak_severity`. `SYSTEM_LEAK_STATUS|system` carries the leak in
`device_leak_status` and sets `test_leak=Enabled` only while every active leak
is a test leak. bmcctld, seeing `test_leak=Enabled`, logs the action it would
have taken and dispatches nothing. A hardware leak is never flagged as a test
leak and keeps the system `test_leak` at `Disabled`, so it is acted on even
while an injection is present.

This needs thermalctld and bmcctld with test-leak support
(sonic-platform-daemons#943). Older daemons ignore `is_test_leak()` and treat an
injected leak like a real one, so do not inject a CRITICAL leak on a system
running them.

## Manual test on the BMC

From the pmon container (or anywhere `sonic_platform` is importable):

```python
from sonic_platform.chassis import Chassis
from sonic_platform_base.liquid_cooling_base import LeakSeverity

lc = Chassis().get_liquid_cooling()
leak_test = lc.get_leak_sensor_test()

# Inject (severity is keyword-only and defaults to MINOR)
leak_test.set_test_leak("coldplate_leak", True, severity=LeakSeverity.CRITICAL)

# Observe
for s in lc.get_leak_sensor_status():
    print(s.get_name(), s.get_leak_severity(), s.is_test_leak())

# Withdraw
leak_test.clear_test_leaks()
```

Expected result:

- `get_leak_sensor_status()` returns `coldplate_leak` while the injection is active.
- `get_leak_severity()` returns the injected severity (`None` once cleared).
- `is_test_leak()` returns `True`, so no mitigation action is taken.
- `is_leak_sensor_ok()` stays `True` — the injection does not fault the sensor.
- After `clear_test_leaks()`, `get_leak_sensor_status()` is empty again.

The injection can also be inspected or cleared directly on the BMC host:

```
ls /run/bmc/leak_sim/
cat /run/bmc/leak_sim/coldplate_leak.test_leak   # prints the injected severity
rm -f /run/bmc/leak_sim/*.test_leak
```
