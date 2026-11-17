# AMD64 MUX source image

The shared `Container archive (AMD64)` job builds both MUX images from the
source-built Trixie config-engine parent and Linkmgrd's published TARs. It
checks the ordered layers, matching debug symbols and incremental rebuilds.
`Run installed MUX images (AMD64)` downloads the archives for that revision
and starts both containers. [Bazel OCI workflow](../../../.github/workflows/bazel-oci.yml)

The selected profile is native AMD64 VS, Trixie, FIPS enabled, ASAN disabled,
`INCLUDE_MUX=y`, and the standard `dbg` suffix. Other profiles keep the existing
Make producer. The original install selection, config-engine dependency,
read-only mounts, and warm/fast shutdown ordering before SWSS are retained.

Linkmgrd's owning repository supplies the daemon, compressed changelog, matching
Build-ID symbols and runtime package metadata. The parent supplies Common,
Python bindings, libnl, supervisor, the Rust exit listener and rsyslog. Only the
four required Boost 1.83 runtime packages are added from pinned APT inputs;
no project DEB is used as a substitute for its source producer.

The runtime exports `//dockers/docker-mux:docker-mux.gz`; the debug variant is
`//dockers/docker-mux:docker-mux-dbg.gz`. They preserve the original entrypoint,
supervisor configuration, critical-process file and Dockerfile template copied
by the legacy `*.j2` instruction.

Build, static validation, incremental checks and loaded-service validation are
separate commands in `tools/bazel/ci/mux_image.py` and `mux_loaded.py`. The build
audits selected actions before execution and rejects DEB outputs. Validation
binds receipts to the committed checkout, checks the actual loaded image config
and layer digests, preserves parent and owner file hashes/permissions, and checks
source/debug ELF pairs. No-op and restored config builds must produce identical
archive hashes; a supervisor-config edit must leave parent/source outputs alone.

The installed probe starts the original entrypoint and Linkmgrd through
supervisor, with a private Redis instance and isolated virtual MUX port. It
requires Linkmgrd's actual state publication, reads it through the installed
SONiC database CLI, and checks positive/negative debug-symbol lookup. The probe
uses neither host networking nor host services and does not simulate a physical
MUX response or claim dual-ToR hardware validation.

The runtime selects its four Boost roots and their transitive dependencies
from the shared build-infra Trixie APT lock. The debug image uses
`sysmgr_debug_debian`, and the installed-service fixture uses the shared test
Redis package. Change the runtime selection in the image module when its
dependencies change. New Debian package records belong in the shared lock.

## Compare with the Make images

The required pairs are `target/docker-mux.gz` and `target/docker-mux-dbg.gz`
from legacy Make against the corresponding Bazel exports above. Use the
same AMD64 Trixie VS/FIPS configuration, source revisions, patches and
package selections for both producers. Build the Make pair with
`BUILD_WITH_BAZEL_WHEN_AVAILABLE=n` and retain the archive hashes, producer
records and file comparison report. The installed-image and incremental
checks exercise the Bazel archives; Make equivalence requires both archive
pairs to pass the shared checker. [Comparison procedure](https://github.com/securely1g/SONiC/blob/master/doc/bazel/sonic-bazel-migration/references/make-bazel-equivalence.md)

Receipts record the tested commit and its immutable parent order. CI binds the
head/base to the checkout before building or accepting downloaded archives.
Both runtime and debug variants and their exact recorded hashes are required.
Generated evidence belongs under `artifacts/<name>`; untracked source files
and tracked source edits cause acceptance to fail.

The image version is passed through the `image_version` build setting to the
Make `Tag` label. Inherited component labels use the installed package receipt,
including Common and the Rust supervisor listener.
