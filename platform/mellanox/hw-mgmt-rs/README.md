# hw-mgmt: hw-management's Python helpers in Rust

The NVIDIA platform API (`sonic_platform`) imports Python modules that
hw-management installs. `platform-native` (`../mlnx-platform-api-rs`) runs
without Python, so it uses these Rust versions of the two that pmon has:

| Module | hw-management installs it as | Used by |
|---|---|---|
| `independent_mode_update` | `/run/hw-management/bin/hw_management_independent_mode_update.py` | imported by `sfp` and the thermal manager; the thermal updater writes ASIC and module temperatures, thresholds and vendor data under `/var/run/hw-management/{thermal,eeprom,config}` through it |
| `dpu_thermal_update` | `/run/hw-management/bin/hw_management_dpu_thermal_update.py` | the SmartSwitch thermal updater: DPU temperatures under `/var/run/hw-management/dpu<N>/thermal` |

This is hw-management's code, not the platform API's. Each module follows the
Python one line by line: what it writes, what it prints, which commands it
runs. The Python file itself is never run. Only its presence is checked,
because that is what decides whether Python's import succeeds.

The third module, `hw_management_redfish_client.py` (the BMC's NOS account:
its password from the TPM, and the login that provisions it), is not ported.
hw-management installs it in the host's `/usr/bin`, which pmon does not
have, so Python's import of it fails in pmon too; `bmc` in `platform-native`
answers what that import does. None of the pmon daemons reaches the BMC.

## The version followed

Both follow hw-mgmt `V.7.0070.1935`. hw-mgmt `V.7.0070.1932` carries the
same two files.

| File | sha256 |
|---|---|
| `usr/usr/bin/hw_management_independent_mode_update.py` | `44f2a2ce18fcff0ddaf0d8f1358074a5d7c3a62c3e8ba09cbb85aa62f45c3662` |
| `usr/usr/bin/hw_management_dpu_thermal_update.py` | `ced3be956203e1beb1fcef6a0dd14702b1e40a89674a57a43370d18ca46e976c` |

When the image takes a hw-management whose copy of one of these files has a
different hash, that module has to be ported again. A different
hw-management version with the same two hashes needs nothing. Compare
against the hw-mgmt checkout the image is built from, not just the version
in `hw-management.mk`:

    sha256sum ../hw-management/hw-mgmt/usr/usr/bin/hw_management_{independent_mode_update,dpu_thermal_update}.py

## Tests

    cargo test --manifest-path ../platform-api-rs-crates/hw-mgmt/Cargo.toml

The `testing` feature exports the test root, for `platform-native`'s tests
of the code that calls into this crate.

Like `platform-native`, the crate is built as
`../platform-api-rs-crates/hw-mgmt`. That way its path dependencies name the
same siblings, and both crates resolve to one `platform-base`.
