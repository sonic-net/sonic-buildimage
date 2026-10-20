# Build containercfgd with Bazel

The wheel target builds this directory's Python source and preserves `setup.py`'s
package contents, version, dependency and `containercfgd` command. The daemon
uses py-common and Common's source-built native Python binding.

From the repository root on native AMD64 Debian Trixie:

```sh
bazel build //src/sonic-containercfgd:sonic_containercfgd_wheel
bazel test \
  //tools/bazel/tests:containercfgd_full_test \
  //tools/bazel/tests:containercfgd_wheel_test
```

On native ARM64 Trixie, add both platform selections:

```sh
bazel test --platforms=@sonic_build_infra//platforms:aarch64_trixie \
  --host_platform=@sonic_build_infra//platforms:aarch64_trixie \
  //tools/bazel/tests:containercfgd_full_test \
  //tools/bazel/tests:containercfgd_wheel_test
```

The wheel is `sonic_containercfgd-1.0-py3-none-any.whl`. Its Python source is
architecture independent; tests use the native Common binding and CPython 3.13.

`containercfgd_full_test` runs the entire existing `tests/` directory from a
writable copy of declared inputs. The adapter supplies an empty system rsyslog
configuration instead of reading the build machine's `/etc/rsyslog.conf`.
The suite's own configuration fixtures and assertions remain unchanged.

`containercfgd_wheel_test` uses the shared wheel-contract checker to compare
metadata and file bytes against `setup.py`,
installs this wheel and py-common into a fresh environment, then runs the
installed `containercfgd` command. Both packages must import from that
environment. The check substitutes the boot wait and Redis event loop so it
does not require a running switch, and makes no system configuration changes.
It does not validate live Redis or service health.

The native AMD64 and ARM64 archive jobs in
[CI](../../.github/workflows/bazel-swss-oci.yml) run both tests. Archive artifacts
publish the wheel, its SHA-256 hash and test counts/status. The job workspace
also retains the full pytest inventory and installed-wheel receipt, which are
required before publication. Collection rejects missing evidence or a wheel
whose bytes differ from those installed by the test.

Python dependencies reuse the checked-in config-engine/py-common lock. There
are no new third-party runtime requirements. Container-image composition
consumes this wheel separately; these targets produce no Debian packages.
