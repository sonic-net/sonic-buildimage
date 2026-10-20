# Bazel helper tests

This directory holds tests for the Bazel helpers, Make container integration,
OCI publication and source-built Python dependency checks. Existing component
behavior tests remain with `src/sonic-config-engine` and `src/sonic-py-common`.
The containercfgd suite remains with `src/sonic-containercfgd` and uses the
same central runner for its complete source suite.

Run the self-contained unit tests from the repository root:

```sh
python3 -B -m unittest discover -s tools/bazel/tests -p '*_test.py'
```

The suite uses Python 3, Make, Bash, Git, `j2` from `j2cli`, `jq`, a C compiler,
`readelf` and `objcopy`. The Make tests use a controlled Bazel executable and small local
archives. The AMD64 P4RT split-debug checks also require GNU `dwp` and GDB;
Trixie supplies `dwp` through `binutils-gold`. Fixture generators also live here so CI's real Bazel cache and Make
export checks use the same inputs.

Two unrelated container families exercise independent runtime/debug builder
switches with caching off and with the real Make package-cache loader. A corrupt
cache must preserve the previous archive and allow a successful retry. Shared
CI tests collect unrelated Bazel targets and inspect native ELF/debug pairs;
SWSS-specific tests retain its selection and package-contract expectations.
Manifest tests run Make's existing generator with unrelated containers, computed
and conditional metadata, service discovery and manifest overlays. They check
runtime/debug isolation, stable timestamps and publication failures before Bazel.

The shared APT suite runs as `//tools/bazel/tests:apt_selection_test`. It checks
OCI layer ordering, retained package state, platform selection and the common
CLI with unrelated AMD64 and ARM64 container fixtures. It rejects incomplete policy declarations and missing/unexpected Make manifests,
and checks that failed selection or staging cannot
publish a new success receipt. These checks create TARs, not Debian packages.

The Orchagent APT suite lives in `integration/select_apt_payloads_test.py` and
runs as `//tools/bazel/tests:select_apt_payloads_test`. It consumes the generated
package mapping, image-owned lock and generated BUILD policy through declared
inputs. It verifies the exact Orchagent policy, checks retained-version
requirements and carries runtime package metadata into
debug selection, rejecting absent, stale or altered inherited inventories.

The checks under `integration/` require declared Bazel dependencies or generated
build outputs and rendering tools.
They run through Bazel, separately from Python unit-test discovery. On native
AMD64 Debian Trixie, run the explicit helper and artifact checks with:

```sh
make -f tools/bazel/prepare_manifests.mk \
  MANIFEST_METADATA=rules/docker-orchagent.mk \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y BLDENV=trixie CONFIGURED_PLATFORM=vs \
  CONFIGURED_ARCH=amd64 ENABLE_ASAN=n DBG_IMAGE_MARK=dbg DOCKERS_PATH=dockers
bazel test \
  //tools/bazel/tests:sonic_py_common_test \
  //tools/bazel/tests:sonic_py_common_full_test \
  //tools/bazel/tests:sonic_config_engine_full_test \
  //tools/bazel/tests:sonic_python_wheels_test \
  //tools/bazel/tests:containercfgd_full_test \
  //tools/bazel/tests:containercfgd_wheel_test \
  //src/sonic-config-engine:sonic_cfggen_cli_test \
  //tools/bazel/tests:manifest_labels_test \
  //tools/bazel/tests:prepare_oci_base_test \
  //tools/bazel/tests:apt_selection_test \
  //tools/bazel/tests:oci_base_layout_test \
  //tools/bazel/tests:oci_base_consumer_test \
  //tools/bazel/tests:sonic_docker_archive_test \
  //tools/bazel/tests:manifest_labels_action_test \
  //tools/bazel/tests:cfggen_template_test \
  //tools/bazel/tests:swss_render_test \
  //tools/bazel/tests:test_dpkg_patterns_up_to_date
```

`BUILD.bazel` declares the production helpers and generated files each check
needs. CI selects the same tests explicitly and retains raw logs and cache
records in the job workspace. The [workflow](../../../.github/workflows/bazel-swss-oci.yml)
supplies the supported native AMD64/ARM64 platform flags and execution dependencies.
The Python checks run the complete config-engine, py-common and containercfgd
test directories and install their source-built wheels. Public archive artifacts retain the wheels, hashes and
extracted test counts/status. Collection still requires test logs/XML, collected
test inventories and the installed-wheel receipt locally. Missing evidence or
mismatched installed wheel hashes fail collection.
The SWSS source job also selects these tests.
The Make command prepares the SWSS JSON inputs consumed by `swss_render_test`.
The same metadata preparation works on an ARM64 execution host; this helper
coverage does not build a production ARM64 image. Label fixtures check JSON
preservation and validation; the cfggen fixture checks declared includes,
literal JSON quoting and failed output generation.
The workspace rc selects the [maintained registry](../README.md#registry-selection)
with pinned dependency versions.

## What each suite protects

Each Python test case has a docstring describing the behavior or regression it
checks. The unit suites below run with local fixtures or controlled command
replacements; they do not build or boot a VS image.

| Unit suite | Purpose |
| --- | --- |
| [artifact_validation_test.py](artifact_validation_test.py) | Check archive paths, ownership, modes, content hashes and links, then validate real ELF architecture and matching split-debug files. Reject unsafe extraction and undeclared symbol gaps. |
| [build_helpers_test.py](build_helpers_test.py) | Protect an existing published archive when copying fails, preserve timestamps for identical bytes, and keep shared caching optional. |
| [build_test.py](build_test.py) | Exercise the shared CI build/query/export helper with unrelated target declarations. Check artifact receipts, source lookup and failure diagnostics without replacing a valid publication on failure. |
| [cache_mount_test.py](cache_mount_test.py) | Check the shared host cache default, explicit overrides and empty-value opt-out, quoted paths, and writable Docker mounts. The mounted system rc supplies repository and disk caches without sharing Bazel output directories. Invalid cache paths must fail before Docker starts. |
| [command_log_test.py](command_log_test.py) | Keep stderr diagnostics separate from stdout artifact paths while retaining command output and exit status as evidence. |
| [docker_switch_test.py](docker_switch_test.py) | Switch unrelated runtime/debug container families independently between Make and Bazel, including real Make cache restores. A corrupt cache must preserve the prior archive and permit a retry. |
| [docker_test.py](docker_test.py) | Exercise the shared Make-to-Bazel bridge: deferred prerequisites, target registration, runtime/debug exports, options and failure handling for unrelated containers. |
| [make_forwarding_test.py](make_forwarding_test.py) | Run the real outer Makefile with a controlled inner build. Existing SWSS archives and VS images must reach the builder on repeated requests and Make/Bazel switches; preserve output timestamps, failure status, distribution phases, the default goal and unrelated targets. |
| [make_manifests_test.py](make_manifests_test.py) | Run Make's existing generator for unrelated containers in parallel, including computed and conditional metadata, service discovery, debug suffixes and fragment merges. Check stable timestamps, failed publication and compatibility with the legacy output directory. |
| [manifest_labels_test.py](manifest_labels_test.py) | Preserve nested values, unknown fields and quoting when converting a JSON manifest to one OCI label line. Reject malformed JSON, non-object manifests and non-JSON numbers. |
| [oci_base_layout_test.py](oci_base_layout_test.py) | Validate an OCI base's platform, index, descriptors and blob contents before consumption. Reject ambiguous, missing, corrupted or unsafe entries. |
| [oci_base_make_test.py](oci_base_make_test.py) | Ensure Make prepares and revalidates each declared OCI base before its consumers, including parallel runtime/debug requests and changed archives with misleading timestamps. |
| [p4rt_debug_test.py](p4rt_debug_test.py) | Require a populated AMD64 DWP that independently supplies matching function types and source lines in GDB. Reject empty, unrelated or malformed symbols and loose-DWO fallback. Fixtures compile ELF files without generating packages. |
| [prepare_oci_base_test.py](prepare_oci_base_test.py) | Publish native OCI entries from Docker-save fixtures without changing source archives. Protect stable timestamps, prior readers, corruption recovery and concurrent publication. |
| [python_packages_test.py](python_packages_test.py) | Retain direct or zipped Python test receipts and reject missing XML/inventories, wheel hashes that differ from the installation test, or results from another architecture. |
| [resolution_test.py](resolution_test.py) | Require complete module-graph evidence and recognize only the explicitly supported extension diagnostics. Unknown, incomplete or mismatched failures remain fatal. |
| [swss_ci_test.py](swss_ci_test.py) | Keep SWSS source selection in its CI caller and require the resolved source to provide its install declarations. |
| [swss_make_integration_test.py](swss_make_integration_test.py) | Check the SWSS opt-in, supported configurations, installer archive contract and Make base/manifest prerequisites. A debug-only request must prepare both manifests before Bazel; other Make phases retain their expected selection. |
| [swss_package_contract_test.py](swss_package_contract_test.py) | Match the SWSS payload to source-declared programs and Lua aliases. Detect missing or extra programs and aliases containing the wrong implementation. |
| [verify_agent_cache_test.py](verify_agent_cache_test.py) | Validate cache evidence: shared disk hits must be distinguished from local action reuse, and changed source must invalidate the cached action. Also check safe probe cleanup. |

The integration suites consume declared Make inputs and files generated by Bazel.
Their assertions inspect generated artifacts, exercise source modules and
installed wheels, or invoke label and template tools. They do not establish
container boot, live service health or forwarding behavior.

| Integration check | Purpose |
| --- | --- |
| [apt_selection_test.py](integration/apt_selection_test.py) | Exercise shared OCI inspection, package retention and CLI staging with unrelated AMD64/ARM64 fixtures. Reject incomplete policies and undeclared Make handoffs, and prevent selection or staging failures from publishing success evidence. |
| [cfggen_template_test.py](integration/cfggen_template_test.py) | Check literal JSON values, declared template includes and build namespace isolation in generated output. A failing cfggen invocation must not publish an output file. |
| [oci_base_consumer_test.py](integration/oci_base_consumer_test.py) | Follow timestamp-varied Make OCI bases through layered Bazel images and Docker exports, checking content, configuration, whiteouts and reproducibility. The fixture uses AMD64 metadata even on an ARM64 host. |
| [manifest_labels_action_test.py](integration/manifest_labels_action_test.py) | Check that separate Bazel actions encode their supplied runtime/debug JSON without changing its values. Exercise changed inputs, missing or invalid manifests, and failure handling that preserves an existing label. |
| [`sonic_config_engine_full_test`](integration/component_pytest_runner.py) | Discover the complete config-engine `tests/` tree using declared sources, fixtures and its existing conftest setup. Run CLI subprocesses with the selected Python interpreter and preserve existing explicit skips. |
| [`sonic_py_common_full_test`](integration/component_pytest_runner.py) | Discover the complete py-common `tests/` tree, including DB and generated gRPC behavior. Use writable copies for existing source-generating hooks; preserve the original assertions, mocks and explicit skips. |
| [`sonic_python_wheels_test`](integration/sonic_python_wheels_test.py) | Install both generated wheels in a temporary scheme, check installed module and script paths, package metadata, generated gRPC exports and template bytes, then execute installed cfggen and DB command entry points with declared external dependencies. Retain the installation receipt in `wheels.json`. |
| [`sonic_cfggen_cli_test`](../../../src/sonic-config-engine/README.bazel.md) | Run the real config-engine command with Common/YANG imports and check input merging, filters, includes, multiple output destinations, namespace input and failure exits. |
| [sonic_py_common_native_test.py](integration/sonic_py_common_native_test.py) | Import every selected Python helper and exercise the real source-built Common extension. The shared [pytest runner](integration/pytest_runner.py) also runs 44 existing interface, BGP and EEPROM cases from the component tree, for 46 cases total; it disables legacy source-mutating conftest hooks and coverage defaults. |
| [sonic_docker_archive_test.py](integration/sonic_docker_archive_test.py) | Check Docker archive metadata and payload for the selected AMD64/ARM64 fixture and require reproducible gzip exports despite source names or requested timestamps changing. |
| [swss_render_test.py](integration/swss_render_test.py) | Check Make's prepared SWSS runtime/debug manifests against the service contract and corresponding OCI labels, and preserve required runtime commands in the generated non-ASAN startup script. |
| [`test_dpkg_patterns_up_to_date`](BUILD.bazel) | Compare the generated dpkg **include** patterns with the checked-in `PATH_INCLUDES` list so they cannot drift silently. This comparison does not build Debian packages. |

[archive_fixture.py](archive_fixture.py) supplies small archive inputs for cache
and Make export checks. [oci_base_fixture.py](oci_base_fixture.py) supplies native
OCI/Docker-save inputs for base preparation and consumer checks. These are fixture
generators, not additional test suites.

## Shared wheel checks

Installed-wheel tests depend on the test-only `:wheel_test_helpers` library:

- `integration/wheel_contract.py` compares the wheel and installed files with
  the package's `setup.py`: metadata, dependency versions, extras, Python files,
  data, licenses, scripts and command entry points.
- `integration/installed_wheel_fixture.py` creates a private installation,
  excludes checkout paths and records the exact tested wheel hashes. Its shared
  regression tests also preserve failed/partial results.

Each component keeps its own import, command and API probes. For example,
containercfgd checks daemon dispatch with Redis/boot interactions replaced;
config-engine checks its installed CLI and py-common checks generated gRPC
exports. The complete original pytest suites remain separate targets.

To add a wheel check, declare its wheel and runtime dependencies in its Bazel
`py_test`, depend on `:wheel_test_helpers`, and call `setup_contract` and
`assert_wheel_contract` inside `installed_wheels`. Add only the component's
behavior probes and write its wheel receipt after success. Register the target
and wheel filenames in `tools/bazel/ci/python_packages.py`'s `TESTS` and
`WHEEL_CHECKS`, and select the target in both architecture jobs. CI requires
successful logs/XML, complete source-suite inventories and matching hashes;
every exported wheel must have exactly one installed-wheel check.

The fixture can borrow declared third-party runfiles; it does not claim an
independently installed dependency closure. `installed_wheel_fixture_test` and
`wheel_contract_test` cover isolation, failure reporting and rejection of
incorrect package contents once for all consumers.
