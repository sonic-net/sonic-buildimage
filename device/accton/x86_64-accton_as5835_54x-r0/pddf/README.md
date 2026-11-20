# AS5835 environmental API migration to PDDF

This port uses PDDF fan, fan-drawer, PSU and thermal APIs over the existing
AS5835 kernel drivers. `drv_attr_name` maps the native sysfs names into PDDF.
The native platform service still creates the I2C clients and owns the fan
policy; these JSON files are API mappings, not a replacement device-creation
topology. Do not enable the generic PDDF device-creation service for this port.

The native chassis preserves EEPROM, LED, CPLD/BIOS inventory, watchdog, SFP
handling and port events. Four board thermals retain their inventory order,
and the existing sparse coretemp discovery appends CPU sensors. Existing PSU
and fan names are preserved for health-check exclusions. Both rotors per tray
are represented, including their RPM and fault state. Missing fault readings
remain unhealthy.

The native YM-1401A driver exports mV, mA and mW. PSU APIs convert these to V,
A and W; chassis output sensors use the shared PDDF adapters to expose mV/mA.
Capacity and voltage limits come from the existing hardware attributes.
Input telemetry and PSU fan target speed remain unsupported. Existing PSU
fan-speed comparison and PSU-temperature-threshold exclusions are preserved.

The native fan and PSU status drivers need the companion hwmon-registration
fix. Their previous `hwmon_device_register_with_info(..., NULL, NULL, NULL)`
calls fail with EINVAL on the test image's Linux 6.12 kernel, removing their
sysfs attributes. `hwmon_device_register_with_groups` supports this existing
client-attribute layout without a chip-info table.

## Host startup

Initialize `PddfApi` before opening `pd-plugin.json`. Native AS5835 device
initialization does not create `/usr/share/sonic/platform` on the host;
the PDDF API creates that link to the canonical device directory. PMON has
an existing platform directory. Both layouts use the same API mappings.

This ordering makes EEPROM decoding available to ZTP, system health and
watchdog startup without enabling generic PDDF device creation.

## PSU airflow health policy

This AS5835 policy assumes the installed PSU airflow matches the chassis.
Exclude only `PSU-1 FAN-1.direction` and `PSU-2 FAN-1.direction` comparisons
from system health. Raw PMBus/CPLD direction values remain visible through
the platform API and CLI. This is a policy assumption, not a measured
confirmation of physical airflow.

PSU fan presence/status checks, chassis fan direction/speed checks and PSU
voltage checks remain enabled. The existing unsupported PSU target-speed
and PSU temperature-threshold exclusions are unchanged.

## Validation

- Regression tests exercise missing/existing host platform links, the PMON
  directory layout, initialization errors, native sysfs path resolution,
  both PSUs, sensor units and unavailable/faulted readings.
- The built Linux 6.12 image runs the corrected fan and PSU status drivers
  on AS5835 hardware. All ten chassis rotors and both PSUs are present and
  healthy; the original 40% fan policy is retained at normal temperatures.
- Board, sparse CPU and PSU temperatures, output voltage/current and 400 W
  PSU capacity are reported through the platform APIs.
- The startup fix was applied live to the master-derived community image.
  EEPROM serial/product/MAC decoding succeeds, and a provisioning request
  with the ZTP identity headers receives HTTP 200 through the management VRF.
- System-health and watchdog-control services start successfully. The wdat
  watchdog passed an arm/disarm test at its configured 30-second timeout.
- With the narrow PSU direction exclusions, the live AS5835 system-health
  result is Services OK / Hardware OK and the physical status LED is green.
