# Native FRR build and OCI installation archives

The pinned FRR source and SONiC patches are compiled with native Bazel C/C++
rules. This implementation exports installation TARs rather than final Debian
packages. Existing Make package builds retain their upstream implementation.

Public installation targets:

| Target | Contents |
| --- | --- |
| `:frr_layout` | Routing daemons, modules, shared libraries, configuration, service resources and runtime documentation |
| `:frr_elf.debug_symbols` | Debug symbols matching the deployed FRR ELF files |
| `:frr_snmp_layout` | FRR AgentX shared library and associated resources |
| `:frr_snmp_elf.debug_symbols` | Debug symbols matching the AgentX library |
| `:pythontools_layout` | FRR Python administration tools |

Use `--platforms=@sonic_build_infra//platforms:x86_64_trixie` or the corresponding
`aarch64_trixie`, `x86_64_bookworm`, or `aarch64_bookworm` platform. Execution tools
use the selected host platform. `--//src/sonic-frr:tcmalloc` and
`--//src/sonic-frr:snmp` preserve the component build options; both default to true.
The image consumer selects applicable TARs and supplies required system packages,
accounts, writable directories, service startup and cache generation. The TARs
can be layered directly; no intermediate DEB assembly or maintainer-script
execution is provided by this component build.

`:check_programs` builds the upstream check binaries. `:upstream_native_check`
runs the ordinary upstream checks with the default tcmalloc configuration.
`:upstream_check` retains the complete suite, including all six Valgrind checks;
run it with `--//src/sonic-frr:tcmalloc=false` so Valgrind owns allocation
instrumentation. Both suites support native AMD64 and ARM64 execution.
CI keeps the production TARs and ordinary tests on the default allocator and
runs the complete memory-check suite separately on native AMD64/Trixie. Cross-compiling the check programs does not execute them.
This avoids the observed tcmalloc initialization errors under Bookworm Valgrind
and preserves strict memory-error exit codes without suppressions.

## Native source CI

The `Bazel FRR and sonic-fib` workflow builds the installation TARs, matching
symbols and upstream check programs on native AMD64/Trixie. This matches the
current migration scope; the retained ARM64 and Bookworm target definitions are
not current CI coverage. It runs the original FRR checks, including Valgrind,
both sonic-fib tests, and the installed SDK/runtime and schema-invalidation
checks documented in `../libraries/sonic-fib/README.bazel.md`. A failing upstream
check remains a failing job. Full VS installer acceptance remains separate from these native component
results.

`verify_runtime.py` checks the deployed FRR and SNMP ELF modes, architecture,
build IDs, debug-link CRCs and matching DWARF. It preserves the owner debug
documentation symlinks and verifies isolated BGPD source lookup succeeds only
with the matching symbol TAR. The receipt records actual SONAME/DT_NEEDED
entries. In the selected source, the SONiC FPM module includes libFIB's public
header but makes no libFIB API call, so the linker drops its unused shared
library. The existing Make build and runtime metadata declarations are retained;
a recipe dependency alone is not proof of an ELF loader dependency.

Native builds use the maintained registry `main`, including Net-SNMP
`5.9.3.sonic.1`, libyang `3.12.2.sonic.6` and libnl3 `3.7.0.sonic.7`.
The root selects published infrastructure
`0.0.30-c8e5486df0147c5652d015bb5ddd49b6dce6b655` from merged registry
PR #305. Its source is the maintained-master merge of infra PR #97, including
the libFIB collections missing from `.29`. All modules use canonical registry
`main`; the temporary source override and frozen candidate catalog are removed.
The source and ordered
SONiC patches remain unchanged. Installation targets retain
`preserve_install_rpath = True` for FRR's private shared libraries.

For local validation, use the commands and explicit platform flags in
`.github/workflows/bazel-frr.yml`. The CI entry point
`tools/bazel/ci/libteam_draft_bazel.sh` also uses canonical `main` and preserves
explicit shared Bazel settings across fresh checkouts. The workflow retains
checkout inputs, resolution lock, build/test events and named output files
with SHA-256 checksums. Component results establish the tested source build;
complete SONiC image, installer, boot and forwarding acceptance are separate.
