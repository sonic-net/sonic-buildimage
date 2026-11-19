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

## Validation

- 149 platform/PDDF tests pass, including native sysfs path resolution,
  both PSUs, missing/malformed readings, fan faults, RPM and child inventory.
- Read-only PDDF path checks on AS5835 hardware resolve both PSUs' electrical
  measurements, hardware voltage limits, 400 W capacity, RPM and temperature,
  plus all four board temperature/80 C threshold pairs.
- Fan, PSU and CPLD modules compile and link against Debian Linux 6.12.117
  headers in a disposable Linux container. They have not been loaded.
- Live fan and PSU-presence validation is pending corrected modules compiled
  for the exact SONiC kernel. The switch has no compiler or kernel headers.
- Its older image also lacks the shared `pddf_psu_sensor` module; that module
  is already included in the source branch this port is based on.

A complete image or matching platform package is required before activating
this migration. The changes have not been installed on the test switch.
