# Component scenarios

The firmware component tests (`src/component_tests.rs`) run each group of
steps in `<group>.json` against the Rust components, beneath an io root that
`setup.sh` fills with fake firmware tools and hardware files. Each test then
compares every answer, and every call the fake tools logged, with what
Python's `component.py` gave over the same fakes. Python's answers are in
`<group>.rec` and its tool calls in `<group>.calls`.

| Group | Platform | What it covers |
|---|---|---|
| `generic` | a switch | configparser, ONIE, SSD, BIOS and CPLD, with their tools failing every way the steps make them |
| `sn2201` | the platform of the same name | its own BIOS and CPLD classes |
| `sn4280` | a SmartSwitch | its CPLDs and DPU FPGAs |
| `simx` | a modular chassis under SimX | no CPLDs |
| `bmc` | a switch with a BMC | a CPLD update that ends in an aux power cycle |

## Regenerating

1. `scenarios.py <group> <group>.json > <group>.plat` writes the steps.
2. `golden.sh` runs Python's `component.py` on them through `driver.py`.
   - It runs in the pmon image, as root, with `/g` this directory.
   - `PLATFORM_COMMON_DIR` names a sonic-platform-common checkout, and
     `MLNX_PLATFORM_API_DIR` the `platform/mellanox/mlnx-platform-api` of the
     sonic-buildimage this crate follows, both as the container sees them.
   - `setup.sh` is called with an empty `R`, so the fakes go in the real
     paths.
3. Copy `out/<group>.rec` and `out/<group>.calls` here.

The MPFAs' temporary directory names are random, and the tests normalize
them.
