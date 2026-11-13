Platform Driver Development Framework (PDDF) is part of SONiC Platform Development Kit (PDK) which optimizes the platform development.

SONiC PDDF (Platform driver development framework) supports the following HW devices on a given platform:

- Fan
- PSU
- System EEPROM
- CPLD
- CPLDMUX
- GPIO
- Optic Transceivers
- System LED control via CPLD
- System Status Registers in CPLD
- Temp Sensors

This folder for the PDDF consists of the following:

- PDDF python utility scripts
- Generic PDDF HW device drivers in kernel space

## Fan conversion configuration

Fan PWM and duty-cycle conversions in `pd-plugin.json` are declarative objects.
Python expressions are not supported. Each conversion uses these fields:

```json
"duty_cycle_to_pwm": {
    "input_offset": 0,
    "multiplier": 255,
    "pre_divide_offset": 0,
    "divisor": 100,
    "output_offset": 0,
    "divide_before_multiply": false,
    "force_float": false
}
```

The default calculation uses this fixed pipeline:

```text
((value + input_offset) * multiplier + pre_divide_offset) / divisor
    + output_offset
```

Set `divide_before_multiply` to `true` only when preserving an established
division-before-multiplication order. Set `force_float` when the previous
formula explicitly used a floating-point constant. These fields preserve
existing calculation semantics across supported Python runtimes. The divisor
must be a positive integer. All other numeric fields must be integers.

### Migrating an existing platform

Earlier releases stored these conversions as Python lambda strings that were
passed to `eval()`. Convert each formula into the fields above:

| Legacy formula | Declarative fields |
| --- | --- |
| `lambda pwm: pwm * 100 / 255` | `multiplier: 100, divisor: 255` |
| `lambda dc: ((dc - 10) / 6)` | `input_offset: -10, divisor: 6` |
| `lambda pwm: ((pwm + 1) * 625 + 75) / 100` | `input_offset: 1, multiplier: 625, pre_divide_offset: 75, divisor: 100` |
| `lambda dc: dc * 100 / 625 - 1` | `multiplier: 100, divisor: 625, output_offset: -1` |
| `lambda pwm: ((pwm / 18000) * 100)` | `multiplier: 100, divisor: 18000, divide_before_multiply: true` |

Unconverted string values are not executed. A fan using one still constructs so
that the rest of the platform keeps working, and reports a configuration error
when a fan speed is read or set.
