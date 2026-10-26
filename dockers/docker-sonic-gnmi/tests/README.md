# gNMI and telemetry launcher tests

Run from the repository root:

```bash
bats --tap dockers/docker-sonic-gnmi/tests
```

These tests require Bash, Bats 1.10.0 and jq. The `LauncherTests` stage in
`azure-pipelines.yml` provisions `bats=1.10.0-1` and jq on Ubuntu 24.04, matching
the Bats package used by `docker-sonic-mgmt`. Local execution needs no Docker,
root access or sudo once these dependencies are installed.

- `test_gnmi_native_user_auth.bats` checks secure defaults and explicit
  authentication modes.
- `test_dialout_transport.bats` checks both images' TLS defaults, trust
  environment, argument isolation and client exit status.
- `test_tls_fallbacks.bats` checks both launchers' configured/legacy
  credentials, rejection before telemetry starts, loopback fallback, and both
  DPU metadata forms' TLS and client-authentication behavior.

Bats owns a fresh temporary directory for each test and removes it afterward.
The tests run temporary copies of the production scripts with only fixed
template/executable paths redirected to fixtures. Guards fail if those paths
change. ConfigDB and telemetry executables are stubs; the tests do not access
devices, services or production paths.

A failed assertion or fixture setup returns nonzero from Bats, which fails
the pipeline step. There is no separate shell suite to run.
