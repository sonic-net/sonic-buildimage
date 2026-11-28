# libFIB source and installed interfaces

The native targets compile the three owner C++ sources and generate their C/C++
headers from `schema/NextHopGroupFull.json` and the existing Jinja templates.
FRR consumes `:nexthopgroup`; its public C interface and runtime library are
tested together. These targets produce TARs, not Debian packages.

| Target | Contract |
| --- | --- |
| `:nexthopgroup` | Shared library plus public installed-style header paths |
| `:runtime` | Versioned `libnexthopgroup.so.0.0.0`, SONAME link and runtime documentation |
| `:development` | Generated/public headers and the `libnexthopgroup.so` link |
| `:runtime.debug_symbols` | Detached symbols from the exact deployed ELF |
| `:runtime_packages.json` | Recipe-derived runtime metadata for image consumers |
| `:tests` | Original C API, JSON round-trip and debug callback tests |
| `:public_c_api_test` | Native consumer of the public C headers and shared ABI |

Current migration validation is native AMD64/Trixie. Existing other-platform
definitions are retained; this scoped CI run does not validate them. The normal
registry is maintained `main`. Use the shared CI entry point:

```sh
tools/bazel/ci/libteam_draft_bazel.sh build \
  //src/libraries/sonic-fib:runtime \
  //src/libraries/sonic-fib:development \
  //src/libraries/sonic-fib:runtime.debug_symbols \
  //src/libraries/sonic-fib:runtime_packages.json
tools/bazel/ci/libteam_draft_bazel.sh test \
  //src/libraries/sonic-fib:tests \
  //src/libraries/sonic-fib:public_c_api_test
```

CI also extracts the delivered runtime/SDK/symbol TARs, compiles the existing
FRR-facing C API probe against only the installed owner headers and library,
and checks its actual loaded library. The existing shared artifact validator
checks build IDs, debug-link CRCs and DWARF. Isolated GDB checks must resolve
`nexthopgroup_version` with packaged symbols and fail without them.

`verify_rebuild.py` checks an unchanged build, changes the schema's generated C
dependency-array capacity from `MULTIPATH_NUM + 1` to `MULTIPATH_NUM + 2`, and
requires a different runtime TAR. It restores
the exact source bytes and requires the original TAR hash again. Its timings
describe a same-output-base incremental check, not a cold build or independent
output-base cache reuse. CI retains the build events and receipt.

Add new C++ files and public headers directly to the maintained source/header
lists in `BUILD.bazel`, alongside their legacy Automake definitions. Add required
schema outputs to the existing generation targets. Keep source/ABI tests with
their owner and run the installed-interface check after changing exported paths
or symbols. SWSS use of this interface is tracked separately by issue #111;
its legacy Make dependency alone does not establish selected Bazel linkage.
