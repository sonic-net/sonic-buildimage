# Bazel package acceptance

The existing `sonic_bgpcfgd_wheel` and `runtime` targets build this owner's
installation payload. `upstream_tests` runs the complete original suite with
declared cfggen and template fixtures.

`installed_wheel_test` adds native AMD64/Python 3.13 acceptance. It installs the
wheel into a private environment, checks its files and metadata against
`setup.py`, resolves all four console entry points from installed files, and
exercises directory notifications and identifier validation. It uses the real
SWSS Common native runtime and declared Python dependencies. It does not start
Redis/FRR services or prove a standalone dependency installation or container.

From the repository root in native AMD64/Trixie (CI uses a pinned container):

```sh
bazel test //src/sonic-bgpcfgd:upstream_tests //src/sonic-bgpcfgd:installed_wheel_test
python3 tools/bazel/ci/frr_python_rebuild.py --output=/tmp/frr-python-rebuild.json
```

The FRR Python workflow retains both owners' wheels and runtime TARs, test
counts, exact package hashes, and tested revision/parents. On AMD64 it also
checks that the installed-wheel receipt names those same wheel bytes. The
rebuild probe requires stable no-op outputs, changes one source/template at a
time, requires changes to exactly that owner's wheel and TAR, and restores the
original source and package hashes. It preserves the current output base and
caches; it does not claim a cold build or a remote cache hit. No DEBs are built.
