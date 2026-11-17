# Source-built config-engine base

`//dockers/docker-config-engine-trixie:docker-config-engine-trixie` remains the
public OCI image. `.debug_symbols` exports matching native symbols and
`:docker-config-engine-trixie.gz` preserves the existing Make archive handoff.
This AMD64/Trixie acceptance work reuses the merged source producers and does
not introduce DEB generation.

| Installed component | Existing producer |
| --- | --- |
| Common library, DB CLI and Python bindings | `@sonic_swss_common//dist:libswsscommon_pkg`, `:sonic-db-cli_pkg`, `@sonic_swss_common//pyext:swsscommon_pkg` |
| libyang and its Python/CFFI binding | `@libyang//:libyang_pkg`, `@libyang_python//:libyang_pkg` |
| libnl runtime family | Existing `@libnl3` runtime-file targets listed in `BUILD.bazel` |
| Event services and Rust supervisor utility | `//src/sonic-eventd:eventd_runtime`, `//src/sonic-supervisord-utilities-rs:runtime` |
| Config-engine, py-common, YANG management, containercfgd and Python supervisor utility | Five owner wheel targets listed in `:python_runtime` |
| YANG models and package metadata | `@sonic_yang_models//:yang_models_pkg` |
| Runtime Python dependencies | `@config_engine_pip`, using the committed source lock |
| Startup files and SWSS template | `//dockers/docker-config-engine-trixie/config:files` |

The pip input for redis-dump-load is the same immutable source commit as Make's
submodule: `758549795174dc6b3be70810e0e4d6308f80f1a3`, archive SHA256
`aece61c694f887af8307e8e82d128ca08f2768017d98ef606ca1380f6ceb593e`.
`MODULE.bazel` applies the existing pipeline-reader patch to its built wheel.
Make orders sonic-py-common after SWSSSDK; the current py-common `setup.py`
requires redis-dump-load, natsort, YAML and the selected gRPC/protobuf versions,
not SWSSSDK. That build-order declaration does not add SWSSSDK to this image.

Pinned distribution packages remain external inputs. The config-engine APT
lock retains `apt-utils` 3.0.3; `build-essential` and `python3-dev` are execution
tools removed by the legacy Dockerfile, not additional runtime requirements.
The Dockerfile installs three explicit copied files: `swss_vars.j2`,
`readiness_probe.sh` and `container_startup.py`. Other Make `_FILES` entries are
available to the Docker context; that alone does not install them. Inherited
rsyslog configuration remains owned by docker-base-trixie.

The existing base-image workflow builds the native packages, source wheels,
models, runtime images and collected symbols. It checks installed ELF hashes,
architecture, loader dependencies, build IDs, debuglink CRC and DWARF. AMD64
also compares every non-directory entry of `:python_and_models` and
`:config_files` with the final OCI filesystem, rejecting changed bytes,
ownership, regular-file modes and link targets. Ordinary symlink modes follow
Linux's installed `0777` representation.

The runtime job loads the exact archive from that build and checks its config,
rootfs and hashes before executing the installed-image probe. That probe checks
the owner files again inside the actual image, runs cfggen and cfg-help with
valid/invalid YANG data, imports native bindings, round-trips generated gNOI
messages, exercises the patched empty Redis pipeline reader and DB CLI help,
and retains the existing FIPS/provider and supervisor checks. It uses no source
checkout and does not claim a complete service daemon or network topology test.

`tools/bazel/ci/config_engine_rebuild.py` records a cached no-op, adds a comment
to the declared SWSS template, and requires only the startup TAR and config-engine
archive to change. The shared base archive, native TAR and Python/model TAR must
remain identical. It restores the exact source bytes even on failure, then
requires all baseline hashes again. This is same-output-base incremental
evidence; it does not prove a cold rebuild or remote cache hit.

Run the existing `tools/bazel/ci/base_images.py --architecture amd64
--config-engine-acceptance --artifacts <directory>` and the rebuild helper in
the workflow's pinned native Debian Trixie container. The first command prepares
the normal Make manifests and audits its Bazel action graph. The new checks are
AMD64-only; inherited ARM jobs remain unchanged. The VS installer job and the
kernel DEB fixture suites in `bazel-oci.yml` run unchanged alongside these checks.

### Validate a reviewed source and registry together

The `Bazel source base images` workflow runs both native architectures for
pull requests and pushes. To reproduce a reviewed pair, dispatch the workflow
on its controller branch, choose `amd64`, and provide full commit IDs for
both `source_revision` and `registry_revision`. Each commit must be reachable
from a branch of its own repository. With both inputs blank the workflow
validates its own revision against registry main.

The build uses the selected source's base-image drivers and Bazel targets.
The controller supplies the mirror configuration and selects one SONiC registry
endpoint. The runtime job loads the resulting artifacts at that same source
revision and checks the registry recorded by Bazel. The source-base artifact
includes `review-selection.json` with the controller, source and registry
commits for manual pairs.
