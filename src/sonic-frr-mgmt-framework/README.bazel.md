# Bazel package acceptance

Reuse `sonic_frr_mgmt_framework_wheel`, `runtime`, and `upstream_tests` for this
owner's wheel, runtime TAR and complete original Python suite.

`installed_wheel_test` installs the generated wheel away from the checkout and
checks its metadata, source files, installed commands and all template bytes
against `setup.py`. The native AMD64/Python 3.13 probe loads the installed
`frrcfgd` entry point using real SWSS Common dependencies, checks command-daemon
parsing and cached operations, and renders a BFD peer from the installed
template. Service startup, FRR forwarding and container acceptance remain
separate. Run these tests in native AMD64/Trixie, matching the pinned CI container.

```sh
bazel test //src/sonic-frr-mgmt-framework:upstream_tests //src/sonic-frr-mgmt-framework:installed_wheel_test
```

The shared workflow and source/template rebuild evidence are described in
[the bgpcfgd package acceptance guide](../sonic-bgpcfgd/README.bazel.md). Existing
ARM64 source CI remains, while these new acceptance checks are AMD64 only.
