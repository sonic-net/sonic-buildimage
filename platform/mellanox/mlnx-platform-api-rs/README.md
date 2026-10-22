# platform-native: the NVIDIA (Mellanox) platform API in Rust

This crate is `sonic_platform` (platform/mellanox/mlnx-platform-api) rewritten
in Rust behind the `PlatformApi` trait the pmon daemons use. A daemon started
with `--platform-api native` runs it in-process. `--platform-api pyo3`
embeds Python and runs the original package. With the native API, the daemon
carries no interpreter.

The two must behave the same: every answer, every exception's class and text,
every syslog line (ident, level and words), every file written, every command
run and every redis write. Python's quirks and bugs are reproduced, not
fixed. Each one is commented where it is reproduced.

The native API is linked in-process by Rust consumers of the platform API,
today the six pmon daemons. Everything else that uses the platform keeps the
Python `sonic_platform`: healthd, the CLI tools, xcvrd and ledd. Both run on
one switch, against the same hw-management files, redis and locks, which is
the other reason the Rust has to do what the Python does.

## Scope

- All of `sonic_platform` that the `PlatformApi` trait reaches:
  - chassis (fixed, modular, SmartSwitch), PSUs, PDBs, fans and drawers, thermals,
    liquid cooling, LEDs, VPD, EEPROM, PCIe, DPU modules and line cards,
    SFP presence and DB readings, the thermal manager and updaters;
  - firmware components on the host, the watchdog, SED, and the BMC over
    Redfish (in pmon, without hw-management's client: see below);
  - `get_change_event` in legacy mode and module host management mode.
- hw-management's Python helpers that `sonic_platform` imports
  (`hw_management_independent_mode_update.py` and
  `hw_management_dpu_thermal_update.py`) are hw-management's code, not
  `sonic_platform`'s. They are ported in `../hw-mgmt-rs`, which names the
  hw-management version it follows. The third,
  `hw_management_redfish_client.py`, is not: it is in the host's `/usr/bin`,
  which pmon does not have, so its import fails in pmon in Python too, and
  `bmc` answers what that import does -- no TPM password, no NOS account
  provisioning. None of the pmon daemons reaches the BMC.
- `sonic_platform_base` code (the base classes) lives in sonic-platform-common's
  `platform-base` crate.
- **sonic_xcvr is not ported.** In module host management mode, four
  state-machine steps read the module through sonic_xcvr's `XcvrApiFactory`.
  Each of them answers a loud `not_ported` error:
  - deciding software or firmware control;
  - checking the module's power class against the cage limit;
  - setting the I2C frequency;
  - disabling an SFF optic's TX.

  A module reaches the first of these once it is powered and its power is
  good.

`open()` accepts any daemon: every `PlatformApi` method is implemented.

## Known differences from the PyO3 path

Each of these was examined and has no effect on what a daemon publishes.

| What | Python | Here | Why it is left |
|---|---|---|---|
| `print()` in hw-management helpers and components | The embedded interpreter buffers stdout in 8 KB blocks and never flushes it at exit. | Written line by line. | pmon runs the daemons with `stdout_logfile=NONE`. |
| Order of a `set` of LED colour names | String-hash order, random per process. | The order of the capability file. | It only shows when two colour files are non-zero at once, which real LEDs never are. |
| Exception from the thermal updater's `utils.Timer` thread | Full traceback on stderr. | First and last lines only. | Stderr only. |
| Order of two threads' syslog lines | Decided by the race. | Decided by the race. | Same race on both sides. |
| `watch_boot_prog` lines | Their order follows the forked child's timing. | First value always logged; no "Received termination signal" line for operations under 5 ms. | Timing-dependent on the Python side. |
| `watch_boot_prog` CPU use | Busy-polls sysfs for the whole DPU operation. | Same. | Reproduced. |
| YAML block scalars (`|`, `>`) | PyYAML supports them. | Unsupported. | None of the 30 NVIDIA `pcie.yaml` files uses one; a nexthop file does. |
| JSON strings with a lone surrogate (`"\ud800"`) | A `str` with a surrogate. | U+FFFD. | A Rust `String` cannot hold one. |
| Integers beyond ±1.7e38 | Exact. | i128; a JSON number this large becomes a float. | No platform value is that large. |
| Order of `_asic_modules_dict` sets of SFP objects | Hash of each object's address. | Order the ports were built. | Addresses cannot be reproduced. In Python this also makes `initialize_sfp_modules` index ready ports by list position, not port number, whenever the orders differ. |
| Order of `InotifyEventHelper.wait_for_events` paths | String-hash order. | Order first seen. | Only differs with several ASICs changing at once. |
| `fd` numbers in `inotify.adapters` debug lines | The process's fd. | The process's fd. | Only reach stderr after the liquid_cooling import's `basicConfig`. Differ between any two processes. |
| `str(xcvr_api)` in one error line | `<... object at 0x...>` | — | Its path needs sonic_xcvr, which is not ported. |

The platform-base crate reproduces CPython behaviour that callers can see.
Each item below has a differential test against Python 3.13:

- the locale codec of an embedded interpreter (ASCII in pmon, which sets no `LANG`);
- `json.loads` and its error messages (3411 cases);
- `shlex.split` (14980 cases);
- int `set` iteration order (20000 cases);
- PyYAML 1.1 plain scalars (all 304 device yaml files).

The first three run on a sample in the tree (427, 833 and 200 cases); set
`JSON_CORPUS`, `SHLEX_CORPUS` or `INTSET_CORPUS` to run a whole corpus (see
platform-base's `tests/data/README.md`).

## Validation

- **Sandbox:** the same plan runs through `platform-api-diff --impl pyo3` and
  `--impl native` in the pmon image, and the two runs are compared on:
  - results;
  - syslog, stderr and stdout (stdout as a subsequence);
  - hw-management files and redis;
  - shim logs (ioctl, curl/Redfish requests, TPM, firmware tools, poll triggers).

  It covers seven platforms: air- and liquid-cooled switches, a SmartSwitch
  and a modular chassis.
  `MODE=daemon` runs whole daemons the same way.
- **Hardware:** a SmartSwitch (A/B of all six daemons in both orders) and a
  liquid-cooled switch with a BMC (read-only API subset).
