# Leak test interface of the NVIDIA BMC

The NVIDIA AST2700 BMC platform implements the leak test interface of `sonic-platform-common` (`LeakageSensorTestBase`). A test leak makes a leak sensor report a leak that its hardware does not see. You can then check the leak reporting path without wetting a detector. The sensor flags such a leak as a test leak: `is_test_leak()` returns `True`.

Read "What a test leak triggers" before you inject a test leak.

## Who can use it

- Only root can use the interface, on the BMC host or in the platform monitor container. The interface is the platform API. The package installs no command and no script for it. There is no build option and no step that enables it.
- No sensor reports a test leak until `set_test_leak()` stores one, or root writes a marker file by hand (see "Rules and limits").
- A request for one sensor writes one notice to syslog. The notice starts with `leak test:` and names the sensor. For an injection, it also names the severity. `clear_test_leaks()` writes one notice for the whole request.

## Example

Run the example as root in a Python interpreter, in the platform monitor container or on the BMC host. The example injects a MINOR test leak on one sensor. The default leak policy only logs one MINOR leak. Before you start, make sure that no sensor reports a leak: `show platform leak status` on the BMC host shows no leak, and `sonic-db-cli STATE_DB hget 'SYSTEM_LEAK_STATUS|system' device_leak_status` prints `None`. A second leak can change what the daemons do.

```python
from sonic_platform.platform import Platform
from sonic_platform_base.liquid_cooling_base import LeakSeverity

cooling = Platform().get_chassis().get_liquid_cooling()
leak_test = cooling.get_leak_sensor_test()
sensors = cooling.get_all_leak_sensors()
names = [sensor.get_name() for sensor in sensors]
print(names)            # empty on a system with no leak sensor: nothing to inject

# Choose a sensor that reads healthy and dry: a test leak shows only there.
index = next(i for i, s in enumerate(sensors) if s.is_leak_sensor_ok() and not s.is_leak())
sensor, name = sensors[index], names[index]
leak_test.is_leak_test_supported()               # True

# Inject. The severity is MINOR when you give none.
leak_test.set_test_leak(name, True)              # True
leak_test.set_test_leak(name, True, severity=LeakSeverity.MINOR)
leak_test.is_test_leak_enabled(name)             # True: the stored state

# What the sensor reports. Call is_leak() first: the two other methods
# report the reading that the last is_leak() took.
sensor.is_leak()                                 # True
sensor.get_leak_severity()                       # LeakSeverity.MINOR
sensor.is_test_leak()                            # True

# Withdraw one test leak, then all of them.
leak_test.set_test_leak(name, False)             # True
leak_test.clear_test_leaks()                     # True: none is left
sensor.is_leak()                                 # False on a dry sensor
```

Each request returns `True` when it leaves the sensor in the state you asked for. A request for an unknown sensor name, or for a name that two channels carry, returns `False` and changes nothing.

The leak daemon polls the sensors about twice a second and reports a test leak at its next poll. While a sensor carries a test leak, look at these items on the BMC host. The `show` and `config` commands are not in the platform monitor container.

- `show platform leak status`
- the keys `LIQUID_COOLING_INFO|<sensor>` and `SYSTEM_LEAK_STATUS|system` in STATE_DB
- syslog

## What a test leak triggers

The platform reports the test leak and its test flag. It decides no action. What happens next depends on the leak daemons of the image.

- **The daemons do not read the test flag.** They handle a test leak as a physical leak of the same severity, under the leak policy of the system. With the default policy, a CRITICAL test leak powers the switch host off and blocks its power-on until you withdraw the test leak. The daemons only log one MINOR test leak. A policy that sets a power action for a lower severity applies to a test leak of that severity too.
- **Two or more leaks at the same time.** On some images, the daemons handle two or more leaks at the same time as one CRITICAL leak. On such an image, MINOR test leaks on two sensors have the same effect as a CRITICAL test leak. A MINOR test leak beside a physical leak has that effect too. On other images, the leak policy can set a number of sensors at which MINOR leaks count as one MAJOR leak. `show platform leak control-policy` shows the policy.
- **The daemons read the test flag.** They report the leak as a test leak and dispatch no action. To exercise the protection action on such an image, use a leak that the daemons cannot distinguish from a physical leak. The leak mock of the test suite gives such a leak.

To inject a CRITICAL test leak, give the severity:

```python
leak_test.set_test_leak(name, True, severity=LeakSeverity.CRITICAL)
```

If the daemons do not read the test flag, the switch host stays off after you withdraw the test leak. The daemons do not power it on again when a leak clears. To power it on, do these steps on the BMC host:

1. Withdraw the test leak.
2. Wait until `show platform leak status` shows no leak and `show chassis modules status` shows `SWITCH-HOST` offline.

   The daemons read the system leak status before they power the switch host on. Make sure that `sonic-db-cli STATE_DB hget 'SYSTEM_LEAK_STATUS|system' device_leak_status` prints `None`. The daemons drop a power-on request that comes while that status is still CRITICAL. On some images, they also drop it while the status is a lower severity for which the leak policy sets a power action. A second `startup` does not repeat the request.

   A CRITICAL alert of the Rack Manager also blocks power-on. On some images, a lower alert also blocks power-on when the leak policy sets a power action for it. Make sure that `show platform leak rack-manager alerts` shows no alert.
3. Set the admin state of the switch host to down:

   ```
   sudo config chassis modules shutdown SWITCH-HOST
   ```

4. Wait 5 seconds, so that the daemons see the down state. Then set the admin state to up:

   ```
   sudo config chassis modules startup SWITCH-HOST
   ```

5. Wait 2 minutes. Then make sure that `show chassis modules status` shows `SWITCH-HOST` online. If it stays offline, read `/host/bmc/event.log`. The daemons write a line there when they block or skip a power-on. Remove that cause before you do steps 3 and 4 again.

## Rules and limits

- You choose a sensor by the name that the platform reports. You choose the severity MINOR or CRITICAL. You can inject on any number of sensors at the same time, but read "What a test leak triggers" first. You cannot simulate a sensor fault.
- A physical leak always has priority. While the hardware of a sensor reads a leak, the sensor reports that leak with its hardware severity and without the test flag. An injection on that sensor does not change this. `is_test_leak_enabled()` still returns `True`, because the marker file of the injection is still there.
- A test leak shows only on a sensor that reads healthy and dry. A sensor with a faulty reading reports the fault, not the test leak.
- `is_leak()` takes a reading. `get_leak_severity()` and `is_test_leak()` do not read the sensor. They report the reading of the last `is_leak()`. Thus a poll gets the leak, its severity and its test flag from one reading. `is_leak_sensor_ok()` reads the sensor at each call. Before the first `is_leak()`, `get_leak_severity()` and `is_test_leak()` report no leak. After you inject or withdraw a test leak, call `is_leak()` before you call the two other methods. After you withdraw a test leak, the next `is_leak()` reports what the hardware reads at that moment.
- A test leak stays until you withdraw it or the BMC reboots. It survives a restart of the leak daemon, of the platform monitor container and of the process that injected it. It has no expiry time. Finish every session with `clear_test_leaks()`.
- The state is one file for each sensor in `/run/bmc/leak_sim/`. The file name is `<a2d>_<channel>.test_leak`, from the two hw-management indices of the sensor. The file holds `MINOR` or `CRITICAL`. If root writes a file of that form by hand, the sensor reports a test leak. Such a file bypasses the interface and its notice. `clear_test_leaks()` removes every file in that directory.
- If the system reports a leak that you cannot explain, look in `/run/bmc/leak_sim/` first. A marker file there that holds `MINOR` or `CRITICAL` is a test leak that nobody withdrew. The file `/var/run/hw-management/leakage/<a2d>/<channel>/channel_name` gives the name of its sensor. Run `clear_test_leaks()`. Look again after the next poll of the leak daemon. A leak that the system still reports does not come from this interface. A physical leak hides a test leak on the same sensor. Thus a marker file does not show that a reported leak is a test leak.
- The leak daemons can log a test leak with the same lines as a physical leak, with no test mark. This includes the persistent event log of critical events.
- Redfish and its subscribers see a test leak as a leak.
