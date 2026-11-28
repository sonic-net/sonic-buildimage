# Build the Python supervisor utility with Bazel

The owner wheel target packages the existing `supervisor-proc-exit-listener`
script. It preserves `setup.py`'s name, version and dependencies. The script
body is unchanged; the wheel uses the standard `#!python` convention so an
installer selects the interpreter.

On native AMD64 Debian Trixie, from the repository root:

```sh
bazel build //src/sonic-supervisord-utilities:sonic_supervisord_utilities_wheel
bazel test \
  //tools/bazel/tests:supervisor_python_full_test \
  //tools/bazel/tests:supervisor_python_wheel_test
```

On native ARM64 Trixie, add
`--platforms=@sonic_build_infra//platforms:aarch64_trixie` and
`--host_platform=@sonic_build_infra//platforms:aarch64_trixie`.
Both platforms use CPython 3.13 and the native Common binding.

The full-suite target runs every existing pytest case with declared writable
fixtures. It stages config-engine's shared Redis fixtures beside this component,
matching the paths used by the original tests. Existing tests substitute process
termination and Redis connections; no running switch is required.

The wheel check installs this package and py-common into a fresh environment.
The [shared wheel checks](../../tools/bazel/tests/README.md#shared-wheel-checks)
compare metadata and script bytes in the wheel and installation with the source
contract. The component probe tests critical-process parsing through the
installed script and checks the command's rejection of a missing container name. Dependencies come from declared
Bazel runfiles; py-common must import from the installed wheel.

`requirements.in` pins Supervisor 4.3.0, the version already tracked by the
Make-built Trixie base image. Config-engine includes this file in its single
checked-in `requirements_lock.txt`. Normal builds consume that lock; do not add
a second supervisor lock. This package adds no independent runtime library.

The native AMD64/ARM64 archive jobs publish the wheel, hashes and test
counts/status. The runner also retains the pytest inventory and installation
receipt and requires those records before publishing. Container-image assembly
consumes the wheel separately. These targets do not produce Debian packages or
validate a live supervisord service.
