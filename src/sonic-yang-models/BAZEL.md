# Build and install the YANG model distribution

Run Bazel from this directory. This standalone module builds the existing
`setup.py` wheel from the checked-in model sources and Jinja templates. It
retains setup.py's model completeness checks and both Python and CVL variants.

- `//:yang_models` exposes the prepared Python model tree.
- `//:yang_models_wheel` produces the `sonic-yang-models` 1.0 wheel.
- `//:yang_models_pkg` installs that wheel into a TAR for SONiC's Python 3.13
  container: metadata in `/usr/local/lib/python3.13/dist-packages` and data in
  `/usr/local/yang-models` and `/usr/local/cvlyang-models`.
- `//:yang_models_package_test` compares installed files with the source wheel
  and checks that Python discovers the installed distribution and version.

```sh
registry=https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/main
bazel test --registry="$registry" --registry=https://bcr.bazel.build \
  --lockfile_mode=update //:yang_models_package_test
bazel build --registry="$registry" --registry=https://bcr.bazel.build \
  --lockfile_mode=update //:yang_models_wheel //:yang_models_pkg
```

The source CI matrix checks native AMD64 and ARM64 with Bazel 8.5.1 and declared
Python execution tools. These targets create a wheel and TAR, with no Debian
package actions. Dependencies use immutable module versions and package hashes;
CI generates its ignored module lock and uploads it with the matching artifacts.
