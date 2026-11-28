# Serial console configuration

`rc.local` derives the serial console baud rate from the first matching kernel
`console=ttyS<n>,<baud>` or `console=ttyAMA<n>,<baud>` argument, defaulting to 9600
when neither is present. It preserves the packaged serial-getty login options
and writes the existing template-wide `10-sonic-console-baud.conf` drop-in.

## Hardware flow control

Hardware RTS/CTS is opt-in through the detected platform's
`/usr/share/sonic/device/<platform>/platform_env.conf`:

```sh
console_hardware_flow_control=ttyS12
```

The value names exactly one serial-getty instance (`ttyS<n>` or `ttyAMA<n>`).
Only enable it after verifying RTS/CTS wiring and configuring the connected
console server for hardware flow control. Missing or empty settings leave getty
behavior unchanged; invalid values are reported and do not enable flow control.
There is no default based on architecture, BMC role, or UART number.

The sole platform opt-in is `arm64-arista_goldfinch-r0`, whose `installer.conf`
identifies the external BMC console as `ttyS12` at 115200 baud. This setting does
not affect the separate Switch-CPU console described by
`switch_cpu_console.conf`, or any other serial-getty instance.

On every boot, `rc.local` regenerates
`/etc/systemd/system/serial-getty@<tty>.service.d/20-sonic-console-flow-control.conf`
from the same baud-adjusted packaged `ExecStart`, adding agetty's
`--flow-control` option. Unlike a one-time `stty crtscts`, the option reapplies
RTS/CTS whenever getty starts after a logout or TTY reset. Packaged units are not
edited, and the generated file is covered by the secure-boot writable-path
allowlist. SONiC-owned flow-control drop-ins are removed before regeneration so
an opt-out or UART change does not retain a stale override. Administrator
drop-ins, including `override.conf`, are not removed.

## Verification

Run the isolated shell regression tests from the repository root on Linux:

```sh
python3 -m pytest -q files/image_config/platform/tests/test_console_config.py
```

On a Goldfinch image containing this change, inspect
`systemctl cat serial-getty@ttyS12.service` and
`stty -F /dev/ttyS12 -a` for `--flow-control` and `crtscts`, respectively.
Run the unchanged `tests/dut_console/test_console_stress.py` from the console
test repository: its 100,000-digit input must produce MD5
`13572e9e296cff52b79c52148313c3a5`, with no new UART input overruns.
Verify the settings again after logout/login, repeated test runs, reboot, and
image upgrade. A manual getty override passing the stress test is not evidence
that a new image has passed these persistence checks.
