# AMD64 VS gearbox images

The gearbox image now has independent runtime/debug OCI targets and exports the
existing `docker-gbsyncd-vs{,-dbg}.gz` archives. The supported profile is native
AMD64 Debian Trixie VS with FIPS and DASH enabled, without ASAN or Syncd RPC.
Other Make profiles retain their existing producer and dependency lists.

The original gearbox and Syncd Dockerfiles request the same package set. The
image therefore shares `platform/vs/docker-syncd-vs/source_packages.bzl`, its
locked Debian inputs, source layer composition and debug symbols. The owner
`@sonic_sairedis//syncd:syncd_pkg` already contains `gbsyncdmgrd`; no second
manager, Syncd or P4 producer is introduced. The selected DASH Make profile
installs ordinary VS Syncd and `syncd_dash`, plus the PI/BMv2/p4c/DASH packages.
Gearbox keeps those installed contents even though its manager launches ordinary
VS Syncd with a nonzero gearbox context.

The parent remains the source-built config-engine image. Gearbox owns only its
service configuration, package/image receipts, manifests, environment and final
image composition. The BUILD files consume public source TARs directly; they do
not produce or import project DEBs. Pinned Debian packages remain external inputs.
The installed-image test uses the Redis server already pinned in the shared
FRR test collection. The runtime and debug images share the Syncd collections.

Build on native AMD64 Debian Trixie with the maintained SONiC registry and BCR.

```sh
python3 tools/bazel/ci/gbsyncd_image.py build \
  --artifacts artifacts/gbsyncd
python3 tools/bazel/ci/gbsyncd_image.py validate --artifacts artifacts/gbsyncd
python3 tools/bazel/ci/gbsyncd_image.py incremental \
  --artifacts artifacts/gbsyncd
python3 tools/bazel/ci/gbsyncd_loaded.py --artifacts artifacts/gbsyncd
```

Run build/static checks on native AMD64 Trixie with the execution tools used by
Bazel OCI CI; the final command additionally needs Docker. Commit source inputs
before acceptance. The build audits its full action graph before execution.
Validation and no-op/edit/restoration checks run separately after archive creation.
Do not substitute a successful graph audit for a successful image build.

The installed test uses the exact original
`sonic-sairedis@5e765149371aeca207254641873791db76a95f5f`
`unittest/lib/files/context_config_gearbox.json`. It starts private Redis, exercises
the original profile copy/Jinja renderer, launches the shipped supervisor and
`gbsyncdmgrd`, and checks that its actual Syncd child selects context1 and the GB
ASIC/FLEX databases while leaving a seeded main ASIC database untouched. It also
checks the manager's missing-platform exit and child-exit handling. All source
ELFs are checked with the installed loader; the debug image uses real GDB
positive/negative checks, including the PI launcher's declared symbol forwarding.

This is software VS gearbox acceptance. Physical PHY behavior, forwarding,
installer construction and booting the resulting full VS image are separate
acceptance steps and are not claimed by the component checks.
