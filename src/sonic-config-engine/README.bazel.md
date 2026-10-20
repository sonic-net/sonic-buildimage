# Bazel build interface

`//src/sonic-config-engine:sonic_cfggen` executes the same `sonic-cfggen` script
installed by `setup.py`. Bazel copies it unchanged to a `.py` entry point for
`rules_python`; there is no second renderer or replacement CLI.

`//src/sonic-config-engine:sonic_config_engine` exposes all Python 3 modules from
`setup.py`, including the optional `minigraph_custom.py` when present. It uses
the local `sonic-py-common` target, Common's source-built native Python binding,
the registered `sonic-yang-mgmt` library and its libyang binding, and pinned
Python packages. These targets use Python 3.13 on native AMD64 and ARM64
with a Debian Trixie execution environment for the native dependencies.
Install `build-essential` for the native GCC/G++ execution tools: the pinned
bitarray 2.8.1 dependency builds a Python 3.13 wheel from source, and the pip
hub explicitly selects these compilers.
`//src/sonic-config-engine:templates` exposes the existing `data/*` templates.

Use the maintained [registry configuration](../../tools/bazel/README.md#registry-selection).
For example, generate an image startup script using the existing CLI:

```sh
PLATFORM=sonic-bazel-build NAMESPACE_ID= bazel run //src/sonic-config-engine:sonic_cfggen -- \
  -a '{"ENABLE_ASAN":"n"}' \
  -t /absolute/path/to/docker-init.j2
```

Build actions must select this binary as an execution tool and declare each
template and input file. Use the source-owned macro to share that action setup:

```starlark
load("//src/sonic-config-engine:template.bzl", "sonic_cfggen_template")

sonic_cfggen_template(
    name = "startup",
    template = ":startup.j2",
    additional_data = {"ENABLE_ASAN": "n"},
    out = "startup.sh",
)
```

The macro uses standard `run_binary`, JSON-encodes `additional_data`, declares
supporting files through `srcs`, and asks the real CLI to write its declared
output with `-t template,output`. A failed command fails the build action.
The action supplies an explicit build-only
`PLATFORM` value and an empty `NAMESPACE_ID` to avoid inheriting these settings
from the host. The SWSS startup template consumes only `ENABLE_ASAN`; cfggen's
other runtime discovery features retain their platform-file requirements.
The resulting startup script still reads live switch metadata and
renders device configuration when the container starts.

The macro preserves cfggen's existing template lookup rules, including
`/usr/share/sonic/templates`. Use the clean Trixie execution environment used by
CI: an installed template with the same basename can otherwise take precedence
over the declared template. Declaring inputs does not isolate cfggen's runtime
discovery or template search behavior.

Build the complete Python 3 wheel and run both the source suite and installed
package checks from the repository root:

```sh
bazel build //src/sonic-config-engine:sonic_config_engine_wheel
bazel test \
  //tools/bazel/tests:sonic_config_engine_full_test \
  //tools/bazel/tests:sonic_python_wheels_test
```

The wheel is `sonic_config_engine-1.0-py3-none-any.whl`. It contains the modules,
`sonic-cfggen` script and `data/*` templates declared by `setup.py`. The installed
wheel test also builds and installs the complete `sonic-py-common` wheel. It
checks package metadata, installed module and script coverage, template bytes,
and the real installed command using the declared external dependencies.
Templates preserve `setup.py`'s existing wheel layout and install beneath
`site-packages/usr/share/sonic/templates`. These wheels do not change the image's
global template installation.

Run the focused command regression tests with:

```sh
bazel test //src/sonic-config-engine:sonic_cfggen_cli_test
```

The root defaults select native AMD64 Trixie for both target code and execution
tools. On a native ARM64 Trixie host, select both platforms explicitly:

```sh
bazel test --platforms=@sonic_build_infra//platforms:aarch64_trixie \
  --host_platform=@sonic_build_infra//platforms:aarch64_trixie \
  //tools/bazel/tests:sonic_config_engine_full_test \
  //tools/bazel/tests:sonic_py_common_full_test \
  //tools/bazel/tests:sonic_python_wheels_test \
  //src/sonic-config-engine:sonic_cfggen_cli_test
```

The execution platform supplies Common's declared C++ toolchain and libraries;
the host's installed Boost libraries are not build inputs.

The tests invoke the real command in a child process, load the native Common
and YANG imports, and check input merging, SONiC filters, template includes,
multiple output destinations, runtime namespace data, and invalid inputs.
They require neither Redis nor installed SONiC packages. These tests cover
the build-tool use case; live ConfigDB, hardware/minigraph discovery, and
`--yang` with installed model directories retain their existing runtime
requirements.

`//tools/bazel/tests:sonic_config_engine_full_test` discovers the whole existing
`tests/` tree, including its fixtures and conftest setup. The runner uses writable
copies of declared sources and fixtures, runs child commands with the selected
Python interpreter, and preserves the suite's existing explicit skips. It does
not substitute a smaller set of CLI cases for the component suite.

The native AMD64 and ARM64 archive jobs in
[CI](../../.github/workflows/bazel-swss-oci.yml) run the full source suite,
focused command tests, installed-wheel checks and SWSS startup rendering.
The `container-archive-amd64` and `container-archive-arm64` artifacts retain both
wheels, SHA-256 hashes, test logs and XML, collected test inventories, the
installation receipt, and the tested revision. Collection fails if a required
wheel, test result or receipt is absent or the installed wheel hashes differ.

This packages the component for native Python 3.13 on AMD64 and ARM64 Trixie.
It does not validate live Redis, switch hardware or service health. The SWSS
image continues to use its existing Make-built config-engine base; the new
wheels are independently built and tested artifacts. These targets create no
Debian packages.

When adding an installed module to `setup.py`, add it to the library and wheel
source declarations in `BUILD.bazel`; the installed-wheel check detects package
contract drift. Declare new imports in `deps`; shared SONiC libraries belong to
their owning targets.

## Python dependencies

Each component owns its direct Python requirements. Core py-common dependencies
and its test runner belong in `../sonic-py-common/requirements.in`. Config-engine
includes that file with `-r` and adds its own packages in this directory's
`requirements.in`.

The Python supervisor utility owns its direct dependency in
`../sonic-supervisord-utilities/requirements.in`, which this input also includes.
It uses the same lock and pip hub, including the Trixie image's tracked
Supervisor 4.3.0 version.

Keep one checked-in `requirements_lock.txt` here for both components. It records
their resolved package versions and hashes; `MODULE.bazel` loads it once as
`@config_engine_pip`, which both targets use. Normal builds read this lock without
resolving the requirements again.

The source suites use `swsssdk` from the exact revision recorded by Make's
submodule. Its Python 3 metadata requires `redis>=4.5.4`, so this lock selects
4.5.4 even though the legacy Trixie builder initially installs 3.5.3.
`redis-dump-load` also uses Make's exact source revision and existing pipeline
patch; the PyPI 1.1 release omits two fixes present in that source revision.

After editing either input, regenerate the shared lock from the repository root
with uv 0.12.18, for the native Linux AMD64/ARM64 Python 3.13 builds:

```sh
uv pip compile src/sonic-config-engine/requirements.in \
  --python-version 3.13 --generate-hashes --no-emit-index-url \
  --output-file src/sonic-config-engine/requirements_lock.txt
```

Review the resulting version and hash changes and validate both native
architectures. Do not add a separate py-common lock: both components must use
the same resolved packages.
