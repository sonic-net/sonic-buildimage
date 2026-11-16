# SBOM Generation for SONiC Builds

With `ENABLE_SBOM=y`, every installer image (`.bin`, `.swi`, `.img.gz`,
one per ASIC variant) gets a CycloneDX 1.6 SBOM, an SPDX 2.3 conversion
and an unsigned SLSA v1.0 provenance alongside it. Default builds are
unaffected.

## Contents

- [Quick start](#quick-start)
- [Configuration](#configuration)
- [What the SBOM covers](#what-the-sbom-covers)
- [Architecture](#architecture)
- [How components are identified](#how-components-are-identified)
- [Licenses](#licenses)
- [Vulnerability scanning](#vulnerability-scanning)
- [Signing](#signing)
- [Reproducibility](#reproducibility)
- [Querying the SBOM](#querying-the-sbom)
- [Known limitations](#known-limitations)
- [File map](#file-map)

## Quick start

```bash
make ENABLE_SBOM=y target/sonic-broadcom.bin
```

| Output | Contents |
|---|---|
| `target/sonic-<machine>.bin.cdx.json` | CycloneDX 1.6 SBOM (primary) |
| `target/sonic-<machine>.bin.spdx.json` | SPDX 2.3, when `SBOM_FORMAT` is `spdx` or `both` |
| `target/sonic-<machine>.bin.intoto.json` | Unsigned SLSA v1.0 provenance |
| `target/<container>.gz.sbom.cdx.json` | Standalone SBOM for test containers in no `.bin` (`docker-ptf`, `docker-ptf-sai`, `docker-sonic-mgmt`) |

The SBOM is not embedded in the `.bin`; publish it alongside. The
`<artifact>.cdx.json` files next to each `.deb`, `.whl` and `.gz` are
build fragments, not SBOMs: don't publish them.

The provenance's subject is the image's SHA-256; it records the
buildimage commit, build parameters (`PLATFORM`, `INCLUDE_*`, mirror
snapshot) and the SBOM's `serialNumber`.

Tools (syft, grype, cyclonedx-cli) are downloaded on first use,
SHA-256 verified, and cached with the scan and license caches in
`target/sbom-tools/`; `make reset` clears them.

## Configuration

| Variable | Default | Meaning |
|---|---|---|
| `ENABLE_SBOM` | `n` | Master switch. Not supported on `armhf` (no syft/grype binaries); `ENABLE_SBOM=y SBOM_STRICT=n` there gives a partial, recipe-only SBOM. |
| `SBOM_FORMAT` | `both` | `cyclonedx`, `spdx` or `both`. The SPDX conversion has no dependency graph and no `formulation`. |
| `SBOM_SCAN_TOOL` | `syft` | Binary scanner. `trivy` is a placeholder: it works only if trivy is already installed. |
| `SBOM_INCLUDE_LICENSES` | `y` | Resolve SPDX licenses. |
| `SBOM_STRICT` | `y` | Fail the build on a missing input (host rootfs, a shipped container, the scanner, a shipped `.deb`'s fragment), a fragment that cannot be written, a schema failure or an unresolved dependency edge. `n` warns instead. SPDX, provenance and license failures only warn either way, so check those files exist. |

All five appear in the `Build Configuration` dump at the start of a build.

## What the SBOM covers

- **In the `.bin` SBOM:** the host root filesystem, kernel and modules,
  bootloader, and every container the `.bin` installs. Optional
  containers appear when their `INCLUDE_*` flag ships them. Only
  packages still installed in the finished image are listed.
- **Never in any SBOM:** `sonic-slave-*` build images,
  `docker-sonic-sdk-buildenv`, and build-only debs and wheels.
- **Build toolchain:** in `formulation[]`, not `components[]`: it
  built the image but is not in it, and grype does not scan it.
- **Test-container SBOMs** have no scanner pass unless
  `SBOM_PER_CONTAINER_SCAN_TOOL` is set: only observed packages and
  lockfiles.

## Architecture

```
 recipe fragments    observation        lockfiles          scanner
 (slave.mk, one per  (dpkg / pip in     (go.sum, npm,      (syft over the
  .deb/wheel/docker)  each scope)        pnpm, yarn)        rootfs and each
        |                  |                  |              container)
        +------------------+--------+---------+------------------+
                                    v
                    build_sbom.py: merge, first source wins
                                    v
             licenses, dependency graph, validation, SPDX, provenance
                                    v
                    target/sonic-<machine>.bin.cdx.json
```

| Source | Gives | When |
|---|---|---|
| Recipe fragments | What SONiC built: versions, submodule commits, patches, vendor hashes, Rust/Go/Python linked into each `.deb` | `sbom_fragment.py`, as each recipe finishes |
| Observation | Every installed apt and pip package, its distro and apt origin | Build hooks, at the end of each container and the host rootfs |
| Lockfiles | Go and JavaScript dependencies of in-tree builds | Build hooks, same time |
| Scanner | Anything the others miss | `build_sbom.py`, over the finished rootfs and containers |

Each source sees only what this build actually ran or installed, so
different platforms and options get different SBOMs without any list
to maintain.

## How components are identified

| What | Package URL | Notes |
|---|---|---|
| `.deb` built or fetched by a recipe | `pkg:deb/sonic/<name>@<version>` | Supplier SONiC, or the vendor for SDK downloads. |
| Its source repository | `pkg:github/<owner>/<repo>@<commit>` | The `.deb` depends on it. |
| Patched Debian or upstream source | `pkg:deb/sonic/...` | `pedigree.ancestors` names the upstream; `pedigree.patches[]` lists each patch with its SHA-256, and `resolves[]` the CVEs a patch says it fixes. |
| apt-installed package | `pkg:deb/<origin>/<name>@<version>?distro=<distro>` | `<origin>` is the apt repository: `debian`, `docker` (docker-ce), or `sonic` for a package installed from a local `.deb`. |
| pip-installed package | `pkg:pypi/<name>@<version>` | |
| Container | `pkg:oci/<name>@<version>` | Its packages hang off it in `dependencies[]`. |
| Rust / Go / Python linked into a SONiC `.deb` | `pkg:cargo/...`, `pkg:golang/...`, `pkg:pypi/...` | Read from the binaries; `sonic:source_deb` names the `.deb`. |
| Go module in a scanned program | `pkg:golang/...` | Hangs off an `application` component for the program. |
| Out-of-tree kernel module package | `pkg:deb/sonic/...` | Depends on the kernel image. |
| In-tree kernel module (`.ko`) | `pkg:generic/<module>` | From the scanner, no version; over half of all components. |
| Vendor SDK or HAL | `pkg:deb/sonic/...` | Supplier from the download URL, EULA name as license. |

## Licenses

In priority order:

1. `$(<artifact>)_LICENSE = "<SPDX expression>"` in the recipe.
2. DEP-5 `debian/copyright` files, mapped to SPDX through
   `scripts/sbom_license_map.json`. A package with no copyright file
   of its own (kmod) takes its source package's license from a
   sibling (libkmod2).
3. `licensecheck` for other copyright files.

The build log reports `License resolution: X/Y resolved; N NOASSERTION`.
To resolve one, find the `License:` keyword the resolver could not
match, look up its SPDX id, and add it to `sbom_license_map.json`. The
map is curated by hand: have someone who has read the license review
each entry.

## Vulnerability scanning

Scanning is a separate step, so an old SBOM can be re-scanned against
today's advisories.

```bash
python3 scripts/sbom_vuln_scan.py target/sonic-broadcom.bin.cdx.json
python3 scripts/sbom_vuln_scan.py --vex vex/ --fail-on critical target/sonic-broadcom.bin.cdx.json
python3 scripts/sbom_vuln_diff.py old.vuln.json new.vuln.json
```

It runs grype (downloading its vulnerability database) and writes
`target/sonic-<machine>.bin.vuln.json` with every finding and the SBOM's
serial number. The table lists findings with an upstream fix; the rest
are counted in its header. `--fail-on` gates only on findings with a
fix. Exit 1 means the gate tripped, 2 an error.

**VEX.** Patched components carry their upstream version, so scanners
report CVEs SONiC's patches fix. OpenVEX statements in `vex/` suppress
them:

- `vex/*.json` are curated and checked in.
- `vex/auto/` is regenerated each build from CVE markers in SONiC's
  own patches, with status `fixed`. Its purls name the source tree,
  so a statement applies only once copied into a curated file with
  the shipped package's purl.
- A suppressed finding stays in the report with an `analysis` block;
  `--fail-on` ignores it.

See `vex/README.md` for the schema and triage workflow.

## Signing

The build does not sign. Sign the published files with your own tool:

```bash
cosign sign-blob --yes --key cosign.key \
    --output-signature target/sonic-broadcom.bin.cdx.json.sig \
    target/sonic-broadcom.bin.cdx.json
```

## Reproducibility

Two builds that install the same packages produce byte-identical SBOMs: timestamps
come from `SOURCE_DATE_EPOCH` (the HEAD commit's time by default) and
`serialNumber` from the document's contents. To compare two:

```bash
python3 scripts/sbom_diff.py old.cdx.json new.cdx.json   # non-zero on drift; --quiet, --json out.json
```

## Querying the SBOM

The graph is rooted at the image. Its children are the containers and
one `sonic:host-image` component for the host filesystem. A
component's `sonic:scope` lists every scope it was seen in; one the
build could not place has no containment edge (the log counts them).
A `+` in a version is `%2B` in a purl.

```bash
SBOM=target/sonic-<machine>.bin.cdx.json

# Validate (without --fail-on-errors it exits 0 on an invalid file)
"$(scripts/install_sbom_tool.sh cyclonedx-cli)" validate \
   --input-file $SBOM --input-version v1_6 --fail-on-errors

# Containers the image installs
jq -r --arg root "$(jq -r '.metadata.component."bom-ref"' $SBOM)" \
   '.dependencies[] | select(.ref == $root) | .dependsOn[]
    | select(startswith("pkg:oci/"))' $SBOM

# Where a package runs
jq -r --arg pkg "pkg:deb/debian/libc6@2.41-12" \
   '.dependencies[] | select(.dependsOn[]? | startswith($pkg)) | .ref' $SBOM

# What swss depends on (debs, its repository, Rust crates)
jq --arg ref "pkg:deb/sonic/swss@" \
   '.dependencies[] | select(.ref | startswith($ref)) | .dependsOn' $SBOM

# Which .deb ships tokio
jq '.components[] | select((.purl // "") | startswith("pkg:cargo/tokio@"))
    | {name, version, source_deb: ([.properties[]?
      | select(.name=="sonic:source_deb") | .value] | first)}' $SBOM

# Locally patched components, and components without a license
jq '.components[] | select(.pedigree.patches) | .name' $SBOM
jq '.components[] | select((.licenses // []) == []) | .name' $SBOM
```

## Known limitations

- Two packages with the same name and version merge into one
  component, whatever their distro or origin.
- Libraries statically linked into a binary are not detected.
- `uv pip` installs are found only by the scanner.
- pip packages have no pinned hashes.
- Vendor EULA licenses are recorded by name, not as SPDX.
- Language dependencies are read only from SONiC-built `.deb`s; for
  Debian's, the scanner covers them.

## File map

| Path | Role |
|---|---|
| `rules/config`, `Makefile.work` | SBOM variables and their passing into the build container. |
| `slave.mk` | Emits a fragment from each recipe; runs `build_sbom.sh` after `build_image.sh`. |
| `scripts/build_sbom.py` | Merges fragments, observation, lockfiles and scans; resolves licenses; builds and validates the graph; writes the SBOM, SPDX and provenance. |
| `scripts/sbom_fragment.py` | Writes one recipe's fragment. |
| `scripts/sbom_purl.py`, `scripts/sbom_os_release.py`, `scripts/sbom_cve_refs.py` | Shared helpers: package URLs, distro, patches and their CVEs. |
| `scripts/sbom_resolve_licenses.py`, `scripts/sbom_license_map.json` | License resolution and the SPDX map. |
| `scripts/sbom_parse_lockfiles.py` | Lockfile parsing. |
| `scripts/sbom_emit_provenance.py` | Provenance document. |
| `scripts/install_sbom_tool.sh` | Fetches syft, grype and cyclonedx-cli, SHA-256 verified, into `target/sbom-tools/`. |
| `scripts/sbom_diff.py`, `scripts/sbom_vuln_scan.py`, `scripts/sbom_vuln_diff.py` | Comparison and scanning. |
| `scripts/sbom_extract_vex_from_patches.py`, `vex/` | VEX statements. |
| `scripts/sbom_upload_openpsirt.sh`, `.azure-pipelines/upload-sbom-openpsirt.yml` | Upload to OpenPSIRT. |
| `src/sonic-build-hooks/scripts/collect_version_files`, `post_run_cleanup` | Per-scope snapshots: copyrights, lockfiles, package origins, installed packages. |
| `scripts/prepare_docker_buildinfo.sh` | Passes `ENABLE_SBOM` into container builds. |
