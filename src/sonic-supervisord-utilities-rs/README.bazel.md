# Rust supervisor listener with Bazel

The public `:runtime` and `:runtime.debug_symbols` targets install the
source-built listener at `/usr/bin/supervisor-proc-exit-listener-rs` and its
matching detached build-ID symbols. This matches `debian/install`. Both native
AMD64 and ARM64 Debian Trixie are supported; Bazel never produces a DEB here.

`:supervisor_library` owns this component's Rust code. It depends directly on
`@sonic_swss_common//crates/swss-common:swss_common`; Common retains its source,
C bindings and native library. All application Rust dependencies use the single
`sonic-rust-deps` graph. The buildimage root also selects landed SWSS commit
`252a506ae5a94e36a4e330b736a3781a9f7bcf9b` and registered Common 0.0.2, which
already use that graph. No local Common source checkout is needed by Bazel.

```sh
bazel build //src/sonic-supervisord-utilities-rs:runtime \
  //src/sonic-supervisord-utilities-rs:runtime.debug_symbols
bazel test //src/sonic-supervisord-utilities-rs:unit_tests \
  //src/sonic-supervisord-utilities-rs:integration_tests \
  //src/sonic-supervisord-utilities-rs:listener_tests \
  //src/sonic-supervisord-utilities-rs:package_test
```

The original Rust tests run serially because they replace process-wide
functions and environment values. The package test executes the installed,
stripped program's `--help` using its declared runtime libraries and checks the
install path, ownership/mode, native ELF architecture, Common SONAME, build ID,
DWARF/debuglink CRC and GDB source-line lookup. It needs native binutils and GDB.
The shared dependency tests additionally exercise pipe readiness, signal-zero
probing, the pinned test injector and syslog initialization on both CPUs.

The public runtime targets automatically select the native Cargo release
profile: optimized code, full LTO and one code-generation unit. A component-owned
Bazel transition applies these settings to the listener and its complete Rust
dependency graph. It reuses the same shared sources and lock without changing
other image targets or requiring image-wide Rust flags. The executable uses
ordinary rustc linking with `-Cpanic=abort`; original test targets keep unwinding.
Source targets retain DWARF and disable rustc's early stripping, then the shared
packaging rules split the runtime binary and matching detached symbols.
CI tests the installed release package under ordinary image build flags. The
shared linking regression also verifies abort versus unwind behavior with real
C++ shared libraries and a Rust library dependency.

Different compatible dependency versions in the shared lock mean this is not
byte-for-byte Cargo artifact identity. Final container service and watchdog
behavior remain image-level checks beyond the installed CLI and original tests.

## Cargo and Bazel locks

`Cargo.toml` and `Cargo.lock` remain unchanged for the native Make/Cargo path.
Bazel's authoritative third-party lock is the shared `sonic-rust-deps` module.
The following direct dependencies differ between those two existing selections;
all Bazel versions satisfy the unchanged component manifest's version ranges.
Required features are retained and combined with the other shared consumers.

| Dependency | Native Cargo lock | Shared Bazel lock | Manifest range |
| --- | --- | --- | --- |
| clap | 4.5.42 | 4.5.53 | 4.0 |
| log | 0.4.27 | 0.4.28 | 0.4 |
| tracing-subscriber | 0.3.19 | 0.3.20 | 0.3 |
| serde_json | 1.0.141 | 1.0.145 | 1.0 |
| tempfile | 3.20.0 | 3.23.0 | 3.0 |

Mio 0.8.11, nix 0.27.1, syslog-tracing 0.3.1, thiserror 1.0.69,
tracing 0.1.41, tracing-log 0.2.0 and scopeguard 1.2.0 match both locks.
The test injector remains pinned to upstream Git commit
`c7317906a1d514ef99bf2fbd4c943908c265d280` (injectorpp 0.4.0).
The shared graph's established release logging cap remains in effect.

## Shared dependency validation

Local commands and native CI select `sonic-rust-deps` version
`0.0.4-6d9c70f708d7420e9e2b2cdc906b80cee6b140f1` from SONiC registry `main`.
This registration uses landed `sonic-build-infra` source
`6d9c70f708d7420e9e2b2cdc906b80cee6b140f1` for the supervisor aliases
and native linking support. Both paths use ordinary module resolution without
a local source checkout or module override.
