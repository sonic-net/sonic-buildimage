# SBOM Generation for SONiC Builds

A CycloneDX 1.6 Software Bill of Materials, an SPDX 2.3 conversion,
and an unsigned SLSA v1.0 provenance attestation are emitted alongside
every `target/sonic-<machine>.bin` when the build is invoked with
`ENABLE_SBOM=y`. Default builds are unaffected.

## Contents

- [Quick start](#quick-start)
- [Configuration](#configuration)
- [Scope](#scope)
- [Architecture](#architecture)
- [Component categories](#component-categories)
- [License resolution](#license-resolution)
- [Tools](#tools)
- [Reproducibility](#reproducibility)
- [Attestation and signing](#attestation-and-signing)
- [Vulnerability scanning](#vulnerability-scanning)
- [Verification](#verification)
- [Querying dependencies](#querying-dependencies)
- [Known limitations](#known-limitations)
- [File map](#file-map)

## Quick start

Enable for one build:

```bash
make ENABLE_SBOM=y target/sonic-broadcom.bin
```

Output, alongside the `.bin`:

```
target/sonic-<machine>.bin                  installer image
target/sonic-<machine>.bin.cdx.json         CycloneDX 1.6 SBOM (primary)
target/sonic-<machine>.bin.spdx.json        SPDX 2.3 (when SBOM_FORMAT in spdx, both)
target/sonic-<machine>.bin.intoto.json      unsigned SLSA v1.0 provenance
```

Test containers that get built but don't ship in any `.bin`
(`docker-ptf`, `docker-ptf-sai`, `docker-sonic-mgmt`) each get a
standalone sidecar SBOM:

```
target/docker-ptf.gz.sbom.cdx.json
target/docker-ptf-sai.gz.sbom.cdx.json
target/docker-sonic-mgmt.gz.sbom.cdx.json
```

Generate a vulnerability report from the SBOM (a standalone post-build
step, not part of the build):

```bash
python3 scripts/sbom_vuln_scan.py target/sonic-broadcom.bin.cdx.json
# Writes target/sonic-broadcom.bin.vuln.json + prints a table.
# Add --vex vex/ to apply suppressions. --fail-on critical for CI gating.
```

The SBOM is *not* embedded inside the `.bin`. Release engineering
publishes the sibling files alongside it.

## Configuration

| Variable | Default | Meaning |
|---|---|---|
| `ENABLE_SBOM` | `n` | Master switch. When `n`, no SBOM hooks fire and no extra tools are installed. |
| `SBOM_FORMAT` | `both` | `cyclonedx`, `spdx`, or `both`. SPDX is a downstream conversion via `cyclonedx-cli` (auto-fetched), so emitting both is essentially free. |
| `SBOM_SCAN_TOOL` | `syft` | Binary scanner for transitive deps: `syft` or `trivy`. |
| `SBOM_INCLUDE_LICENSES` | `y` | Whether to harvest copyrights and resolve SPDX licenses. |
| `SBOM_STRICT` | `y` | When `y`, the build fails if a critical SBOM input is missing — host rootfs (`fsroot-<machine>/`), any `.gz` in `SBOM_INSTALLER_DOCKERS`, or the scanner binary. Set to `n` for debugging or one-off partial emits. Soft optional features (SPDX conversion, provenance, license resolution) always warn-and-continue regardless. |

These variables are surfaced in the `Build Configuration` dump that
SONiC prints at the start of every build, so you can confirm the state.

## Scope

Two SBOM kinds are emitted, with different scopes:

**The aggregate `.bin` SBOM** (`target/sonic-<machine>.bin.cdx.json`)
covers what ships on the DUT inside the installer payload: the host
root filesystem, the kernel + every shipped module, the bootloader,
and every container the build packaged into the `.bin`. Build slaves,
the SDK build environment, and any container gated by an `INCLUDE_*`
flag set to `n` are automatically excluded — the aggregator reads the
docker list `slave.mk` itself computed for the active `PLATFORM`, no
hand-curated allow-list.

**Per-container sidecar SBOMs** (`target/<container>.gz.sbom.cdx.json`)
are emitted for test containers that get built but don't ship in any
`.bin` and so wouldn't otherwise be represented:

- `docker-ptf`, `docker-ptf-sai` — PTF test harness
- `docker-sonic-mgmt` — automation harness

These exist so security tooling has a CycloneDX surface for them
distinct from the production `.bin` SBOM. The other ~30 in-scope
containers don't get a sidecar — they're already covered by the
aggregate `.bin` SBOM.

When you enable an optional container (`INCLUDE_PDE=y`, etc.) it
ships, so it appears in the `.bin` SBOM. The filter tracks the
build's own opt-ins.

Always out-of-scope (never gets any SBOM):

- `sonic-slave-*` build slave images.
- `docker-sonic-sdk-buildenv` (SDK build environment).
- Build-time-only debs and wheels (`_TEST = y` recipes, build deps
  that `apt autoremove` strips before `fs.squashfs` is sealed).

## Architecture

Four independent sources contribute components to the final SBOM,
merged in priority order (first source wins for any given component):

```
+----------------+      +----------------+      +----------------+      +----------------+
|  Recipe-emit   |  >>  |  Observation   |  >>  |    Lockfile    |  >>  |    Scanner     |
|                |      |                |      |    parsing     |      |  (syft/trivy)  |
| Authoritative  |      | dpkg/pip in    |      | go.sum, npm /  |      | Catch-all over |
| for ~250 SONiC |      | the assembled  |      | pnpm / yarn    |      | the assembled  |
| .debs + per-   |      | rootfs and     |      | locks from in- |      | rootfs and     |
| .deb language  |      | containers.    |      | tree builds.   |      | each container.|
| deps (Rust /   |      |                |      | (Rust is via   |      |                |
| Go / Python).  |      |                |      | recipe-emit,   |      |                |
|                |      |                |      | not lockfile.) |      |                |
+----------------+      +----------------+      +----------------+      +----------------+
        |                       |                       |                       |
        +-----------------------+-----------------------+-----------------------+
                                            v
                                +-----------------------+
                                |  Merge (purl + nva +  |
                                |  normalized-version)  |
                                +-----------------------+
                                            v
                            target/sonic-<machine>.bin.cdx.json
```

**Why hybrid.** Recipe-driven emit owns the locally-built artifacts
(versions, submodule SHAs, patch series, per-.deb Rust/Go/Python
language-dep attribution) that a binary scanner has no way to recover.
Observation owns transitive apt/pip deps. Lockfile parsing adds the
language-ecosystem transitive deps in ecosystems the per-.deb
introspection above doesn't already cover. Scanner is the wide net
for anything else. None of the four alone is complete; the union is.

**Dedupe by `(purl, name+version+arch, name+normalized-version+arch)`.**
The normalized-version key strips Debian epochs (`1:`) and downstream
suffixes (`+fips`, `+sonic`, `+bN`, `+debNuM`) so recipe-emit's
a recipe-emit `<pkg> <upstream-ver>` matches observation's `<epoch>:<upstream-ver>+fips`.
Legitimately distinct upstream versions of the same package across
different Debian releases stay distinct.

**Dynamic, not enumerated.** The recipe layer only fires when make
actually invokes that recipe; observation only sees what got
installed. Builds for different `PLATFORM` values produce different
SBOMs automatically because the build invokes different recipes.
`SONIC_VERSION_CONTROL_COMPONENTS` overrides (where pins float to
mirror-latest) are reflected too — observation records what was
actually installed.

## Component categories

How each pattern is captured. Concrete CycloneDX shapes are visible in
`target/.../*.cdx.json` after a build; one representative example is
shown after this list.

- **SONiC-native code** (sonic-net submodule). Primary PURL:
  `pkg:github/sonic-net/<repo>@<commit>`. `externalReferences` records
  the submodule URL and pinned commit. No pedigree unless a sibling
  `src/<pkg>.patch/` exists.

- **Patched upstream Debian sources** (`dget` + sidecar patches in
  `src/<pkg>/patch/`). Primary: `pkg:deb/sonic/<name>@<version>`.
  `pedigree.ancestors[0]` records the upstream Debian `.dsc` URL +
  MD5 from `versions-web`; `pedigree.patches[]` enumerates each
  applied patch with its SHA-256.

- **Forked upstream with nested submodule** (the FRR pattern:
  `src/sonic-frr` is a sonic-net wrapper holding patches; the nested
  `src/sonic-frr/frr` is FRRouting upstream). Primary:
  `pkg:deb/sonic/frr@<version>`. Ancestor:
  `pkg:github/FRRouting/frr@<git-describe>`. Patches + aggregate
  patch-set SHA-1 in pedigree. Same handling for sonic-p4rt,
  sonic-sysmgr (gnoi), wpasupplicant.

- **Direct upstream submodule with sidecar patches** (scapy, ptf,
  ptf-py3, supervisor, redis-dump-load). Primary derived from the
  artifact filename (PyPI for wheels, deb for debs). Ancestor:
  `pkg:github/<upstream-org>/<repo>@<git-describe>`. Patches from the
  sidecar directory.

- **Upstream apt/dpkg packages** (everything apt-installed into the
  rootfs or a container, ~1000+ debs). Captured by observation:
  `dpkg-query` against each scope. PURL form
  `pkg:deb/debian/<name>@<version>?arch=<arch>`. Mirror snapshot
  timestamp attached via `externalReferences.comment`.

- **Python packages.** Locally-built wheels and stdeb debs follow the
  SONiC-native or direct-upstream rules. PyPI installs in
  containers/rootfs are observation
  (`pkg:pypi/<name>@<version>`) from `pip3 freeze`. `uv pip` installs
  are caught by the scanner pass since uv writes standard
  `dist-info/` directories.

- **Docker container images.** One `type: container` component per
  shipped container with image digest. Per-container observation
  components nested under it via `dependencies[]`. Base image SHAs
  recorded from `files/build/versions/default/versions-docker`.

- **Kernel and out-of-tree modules.** Linux kernel built from Debian
  source + ~200 SONiC patches enumerated in pedigree. Each kernel
  module (`opennsl-modules`, `sx-kernel`, `ionic-modules`, every
  `sonic-platform-modules-*`) has a `dependencies[]` edge pointing
  at the kernel image's `bom-ref` so consumers can reason about
  kernel-ABI risk.

- **Vendor SDKs and HALs** (Broadcom SAI, Mellanox SDK, NVIDIA
  BlueField, Marvell Teralynx, Pensando, Arista PHY Credo). Recipe
  emits `type: library` with `supplier.name` derived from URL
  pattern, the EULA name as license, and a build-time-computed
  SHA-256.

- **Language-ecosystem transitive deps.** Per-scope `go.sum`,
  `package-lock.json`, `pnpm-lock.yaml`, `yarn.lock` are harvested by
  the build hook into `lockfiles.tar.gz`. Each entry becomes a
  `pkg:golang/` or `pkg:npm/` component with the SHA-256 from the
  lockfile preserved. Rust crate inventory is *not* harvested this
  way — see the next bullet.

- **Per-.deb language dependency attribution.** Every SONiC-built
  `.deb` is introspected at fragment-emit time
  (`scripts/sbom_fragment.py`). A single `dpkg-deb -x` extraction
  feeds three harvesters:

  - **Rust** — for each ELF, `rust-audit-info` reads the `.dep-v0`
    section that `cargo-auditable` embedded. Emits each crate as
    `purl=pkg:cargo/<name>@<version>` with
    `sonic:fragment_kind=recipe-emit-rust`.
  - **Go** — for each ELF, `go version -m` parses the
    `runtime/debug.BuildInfo` table from the `.go.buildinfo` section.
    Emits each module as `purl=pkg:golang/<module>@<version>` with
    `sonic:fragment_kind=recipe-emit-go`. Replacement directives
    (`=>`) are honored so the recorded version is what actually
    shipped.
  - **Python** — walk for `*.dist-info/METADATA` files and parse
    Name + Version. Emits each as `purl=pkg:pypi/<normalized>@<version>`
    with `sonic:fragment_kind=recipe-emit-python`. SONiC `.debs`
    today don't ship dist-info content (Python is staged via dpkg
    into `/usr/lib/python3/dist-packages/` without metadata), so
    Python attribution here is currently a no-op — but the harvester
    activates automatically as soon as a `.deb` recipe starts
    bundling a venv or installing wheels into `debian/<pkg>/`.

  Every emitted component carries `sonic:source_deb=<deb filename>`.
  `build_sbom.py:build_dependency_graph()` reverses
  `sonic:source_deb` into `dependsOn` edges from the `.deb`'s
  `bom-ref` to each language-dep component so a consumer can walk
  `swss_*.deb -> tokio@1.x` (Rust) or `sonic-gnmi_*.deb ->
  github.com/openconfig/gnmi@v0.10` (Go) directly without parsing
  properties.

### Example component (forked-upstream pattern after pedigree resolution)

```json
{
  "bom-ref": "pkg:deb/sonic/frr@<sonic-version>?arch=<arch>",
  "type": "library",
  "name": "frr",
  "version": "<sonic-version>",
  "purl": "pkg:deb/sonic/frr@<sonic-version>?arch=<arch>",
  "supplier": {"name": "SONiC build (forked upstream)"},
  "pedigree": {
    "ancestors": [{
      "name": "frr",
      "version": "<upstream-tag>",
      "purl": "pkg:github/FRRouting/frr@<upstream-tag>",
      "externalReferences": [{"type": "vcs", "url": "https://github.com/FRRouting/frr"}]
    }],
    "patches": [
      {"type": "unofficial",
       "diff": {"url": "file://src/sonic-frr/patch/<patch-name>.patch",
                "hashes": [{"alg": "SHA-256", "content": "<hex>"}]}}
    ],
    "notes": "patch-set sha1: <hex>"
  }
}
```

## License resolution

Three sources, applied in priority order:

1. **Per-recipe override.** Recipes can declare
   `$(<artifact>)_LICENSE = "<SPDX expression>"` in `rules/*.mk`. The
   build helper passes it through to the fragment generator as
   `ARTIFACT_LICENSE`. Authoritative for SONiC-native packages where
   no `debian/copyright` ships.

2. **DEP-5 `debian/copyright`.** The build hook tars every
   `/usr/share/doc/*/copyright` from each scope into a sidecar
   tarball (before `apt autoremove` and the doc-strip at the end of
   the rootfs build wipe it). The resolver parses any file declaring
   `Format: ...copyright-format/1.0/` and maps `License:` stanzas
   to SPDX via `scripts/sbom_license_map.json` (~100 mappings).

3. **`licensecheck` fallback** (from `devscripts`) for non-DEP-5
   copyright files; output goes through the same SPDX table.

Components ending up `NOASSERTION` are counted at end-of-build:

```
[build_sbom.py] License resolution: 1182/1266 resolved (93.4%); 84 NOASSERTION
```

### Maintaining `scripts/sbom_license_map.json`

The license-header → SPDX-identifier map is **manually curated**. Each
entry maps a free-form license declaration string (as it appears in
`License:` stanzas of Debian DEP-5 `debian/copyright` files, or as
`licensecheck` reports them) to a valid SPDX-2.3 identifier.

To add a new mapping when a previously-unmapped `License:` keyword
shows up in a build's `NOASSERTION` set:

1. Run a build with `ENABLE_SBOM=y`. The aggregator logs the count
   of unresolved components: `License resolution: X/Y resolved …; N
   NOASSERTION`.
2. Drill into the generated `target/<artifact>.bin.cdx.json` and
   grep for components with no `licenses[]` entry; check their
   `pedigree.notes` or external references to find the source
   `License:` keyword the resolver couldn't match.
3. Look up the corresponding SPDX identifier at
   <https://spdx.org/licenses/>.
4. Add an entry to `scripts/sbom_license_map.json`:
   ```json
   "BSD-3-Clause variant from foo project": "BSD-3-Clause"
   ```
5. Re-run the build (or just the aggregator) and verify the count
   of resolved components increases.

The map is intentionally not auto-generated: false-positive risk is
too high for license declarations that differ in legally-meaningful
ways. A new mapping should be reviewed by someone who has read the
underlying license text at least once.

## Tools

When `ENABLE_SBOM=y`, the build invokes `scripts/install_sbom_tool.sh`
to fetch `syft`, `cyclonedx-cli` (if SPDX requested), and `grype` (for
post-build vuln scanning). The script wgets a pinned upstream release,
verifies its SHA-256 against a value hardcoded in the script, and
caches the binary under `target/sbom-tools/`. Subsequent builds use the
cache.

Because the `wget` runs through SONiC's existing build-hook shim, the
URL + MD5 lands in `versions-web` like every other web-fetched
artifact — no manual pin editing required.

For standalone vuln scanning outside a build, the same script
auto-fetches into `~/.cache/sonic-sbom/`.

The aggregator additionally caches per-file scanner output under
`target/sbom-tools/syft-cache/<tool>-<sha256>.json`, keyed by SHA-256
of the input archive. A SONiC platform with multiple ASIC variants
(e.g. broadcom, broadcom-dnx, broadcom-legacy-th) invokes
`build_sbom.py` once per variant; without the cache, syft would
re-scan the ~28 docker `.gz` files that are identical across variants
three times. The cache short-circuits 2nd and 3rd hits. `dir:` scans
(host rootfs) are not cached because `fsroot-<machine>/` differs per
variant. `make reset` wipes `target/` and therefore invalidates the
cache automatically — no stale entries across resets.

The license resolver's output is similarly memoized so it is not
recomputed once per ASIC variant. Same `make reset` invalidation
as the scanner cache; transparent to operators.

## Reproducibility

Two byte-identical builds of the same source produce byte-identical
SBOMs. `Makefile.work` sets `SOURCE_DATE_EPOCH` automatically from
the HEAD git commit's author timestamp (falling back to wall-clock
`date +%s` only when the source tree isn't a git checkout), and
exports it through to the slave container, so all SBOM timestamps
are derived from source state rather than build wall-clock. Component
ordering is sorted. License resolution depends only on on-disk files
+ the static translation table. Override is supported by passing
`SOURCE_DATE_EPOCH=<unix-ts>` explicitly to `make`.

Cross-host reproducibility check:

```bash
python3 scripts/sbom_diff.py old-host.cdx.json new-host.cdx.json
# Exit 0 if equivalent at the component level; non-zero on drift.
# --quiet for one-line summary; --json out.json for tooling.
```

The diff ignores timestamps and aggregator-internal metadata. It
compares version, hashes, licenses, pedigree.ancestors, and patch
hashes.

## Attestation and signing

The build emits an **unsigned** SLSA v1.0 / in-toto v1 provenance at
`target/sonic-<machine>.bin.intoto.json` capturing the `.bin`
SHA-256, the `sonic-buildimage` git commit, the active build slave
image digests, the build's external parameters (`PLATFORM`,
`CONFIGURED_ARCH`, `INCLUDE_*`, etc.), and a content-hash reference to
the SBOM. The document is reproducible.

**No signing happens inside the build.** Two reasons: (1) SONiC's
`SECURE_UPGRADE_*` keys answer a different question ("permitted to
boot on this hardware?"); reusing them for attestation expands their
blast radius; (2) production signing often requires per-invocation
human approval — release engineering needs to batch all artifacts
under one key access in their own toolchain.

Recommended post-build workflow (operator chooses the tool):

```bash
# Sigstore cosign
cosign sign-blob --yes --key cosign.key \
    --output-signature target/sonic-broadcom.bin.cdx.json.sig \
    target/sonic-broadcom.bin.cdx.json

# Or OpenSSL PKCS#7
openssl cms -sign -binary -in target/sonic-broadcom.bin.cdx.json \
    -signer attestation.crt -inkey attestation.key \
    -outform DER -out target/sonic-broadcom.bin.cdx.json.p7s
```

## Vulnerability scanning

Decoupled from the build by design: the SBOM is reproducible, a CVE
report cannot be (new CVEs are disclosed daily). A six-month-old SBOM
can be re-scanned against today's CVE data without rebuilding.

Standalone Python scripts, not make targets:

```bash
# Default report (table + sibling .vuln.json)
python3 scripts/sbom_vuln_scan.py target/sonic-broadcom.bin.cdx.json

# With VEX suppression and CI gating
python3 scripts/sbom_vuln_scan.py --vex vex/ --fail-on critical \
    target/sonic-broadcom.bin.cdx.json

# Drift between two reports
python3 scripts/sbom_vuln_diff.py release-202503.vuln.json release-202504.vuln.json
```

The scanner (`grype` by default) reads the SBOM directly. Without
VEX, every locally-patched component flags upstream CVEs (since
`pedigree.ancestors` records the unpatched version). VEX suppresses
those that the SONiC patches actually fix.

The table written to stdout has one row per finding, sorted by
severity descending:

```
SEVERITY  ID                 ECOSYS  PKG                        VERSION                  STATE      FIX
------------------------------------------------------------------------------------------------------------
Critical  CVE-2024-3094      deb     xz-utils                   5.4.1-1                  fixed      5.6.1+really5.4.5-1
Critical  CVE-2024-45337     go      golang.org/x/crypto        v0.24.0                  fixed      0.31.0
Critical  CVE-2021-3711      rust    openssl-src                111.10.2+1.1.1g          fixed      111.16.0
Critical  CVE-2021-44906     npm     minimist                   1.2.5                    fixed      1.2.6
Critical  CVE-2026-7210      py      python3.13                 3.13.5-2+deb13u2         fixed      ...
High      CVE-2026-34040     go      github.com/docker/docker   v28.5.2+incompatible     not-fixed  -
```

`ECOSYS` is derived from the PURL type (`deb`, `rust`, `go`, `npm`,
`py`, `gem`, `java`, `oci`, `github`, `generic`). `FIX` is the
suggested upgrade version when grype's advisory data names one, or
`-` for `not-fixed` advisories. GHSA aliases for CVE-primary findings
are preserved in the sidecar `<report>.cdx.json` as
`vulnerabilities[].references[]` per the CycloneDX 1.6 schema.

### VEX

VEX files live in `vex/` as OpenVEX JSON (grype's `--vex` flag
rejects YAML). `vex/README.md` documents the schema, status taxonomy,
and triage workflow. Curated statements live at the top level and are
checked in; the `vex/auto/` subdirectory is **gitignored** and
regenerated on every SBOM build by `scripts/sbom_extract_vex_from_patches.py`,
which scans `src/**/*.patch` for CVE markers and emits `not_affected`
statements per patch-mentioned CVE. The regeneration is wired into
`scripts/build_sbom.sh` so the auto-VEX set always reflects the
current state of `src/*/patches/` — a patch that introduces or
removes a CVE marker is reflected on the very next build.
Re-invocations against an unchanged patch set short-circuit;
changing any tracked patch forces a rescan on the next build.

```json
{
  "@context": "https://openvex.dev/ns/v0.2.0",
  "@id": "https://github.com/sonic-net/sonic-buildimage/vex/<id>",
  "author": "<email>",
  "timestamp": "<ISO-8601>",
  "version": 1,
  "statements": [{
    "vulnerability": {"name": "CVE-YYYY-NNNNN"},
    "products": [{"@id": "pkg:deb/sonic/<name>@<version>"}],
    "status": "not_affected",
    "justification": "vulnerable_code_not_in_execute_path",
    "impact_statement": "Fixed by SONiC patch <path>",
    "references": ["<upstream-commit-or-advisory-URL>"]
  }]
}
```

Auto-extracted entries use a generic `pkg:generic/<source-tree>` PURL
because the extractor doesn't know which downstream debs the patch
ships in; promoting them to curated files with concrete PURLs
improves suppression rate.

## Verification

```bash
# Schema-validate the SBOM
cyclonedx-cli validate --input-file target/sonic-<machine>.bin.cdx.json

# Component count
jq '.components | length' target/sonic-<machine>.bin.cdx.json

# Locally-patched components
jq '.components[] | select(.pedigree.patches) | .name' \
    target/sonic-<machine>.bin.cdx.json

# Components without resolved licenses
jq '.components[] | select(.licenses == null or .licenses == []) | .name' \
    target/sonic-<machine>.bin.cdx.json
```

## Querying dependencies

The CycloneDX `dependencies[]` graph supports direct walks without
parsing string properties. The bom-ref of a SONiC-built `.deb`
matches its source identity — `pkg:github/sonic-net/<repo>@<commit>`
for components built from a sonic-net submodule,
`pkg:deb/sonic/<name>@<version>?arch=<arch>` for patched-upstream
Debian sources.

```bash
SBOM=target/sonic-<machine>.bin.cdx.json

# What does sonic-swss depend on?
#   Returns 10 sibling SONiC fragments (libteam-*, libnexthopgroup-*,
#   sonic-sairedis, sonic-dash-api, sonic-stp, sonic-swss-common)
#   plus every Rust crate linked into swss's binaries (~190 today).
jq --arg ref "pkg:github/sonic-net/sonic-swss@<commit-prefix>" \
   '.dependencies[] | select(.ref | startswith($ref)) | .dependsOn' \
   "$SBOM"

# Which .deb ships tokio@1.x ?
#   recipe-emit-rust components carry sonic:source_deb=<filename>;
#   the same attribution is mirrored as a dependsOn edge from the
#   .deb to the crate, so either property or graph walk works.
jq '.components[] | select(.purl | startswith("pkg:cargo/tokio@")) |
    {name, version,
     source_deb: ([.properties[]? | select(.name=="sonic:source_deb") | .value]
                  | first)}' "$SBOM"

# Reverse: list every Rust crate that swss ships
jq --arg ref "pkg:github/sonic-net/sonic-swss@<commit-prefix>" \
   '.dependencies[] | select(.ref | startswith($ref)) |
    .dependsOn | map(select(startswith("pkg:cargo/")))' "$SBOM"

# Same idea for Go modules in sonic-gnmi
jq --arg ref "pkg:github/sonic-net/sonic-gnmi@<commit-prefix>" \
   '.dependencies[] | select(.ref | startswith($ref)) |
    .dependsOn | map(select(startswith("pkg:golang/")))' "$SBOM"

# What declared build/runtime .deb deps did a recipe make?
#   These remain as space-separated string properties for analytics
#   that need the build-vs-runtime split (CycloneDX dependencies[] is
#   one unscoped graph and doesn't distinguish the two).
jq --arg ref "pkg:github/sonic-net/sonic-swss@<commit-prefix>" \
   '.components[] | select(.["bom-ref"] | startswith($ref)) |
    {build_depends:   [.properties[]? | select(.name=="sonic:build_depends")   | .value],
     runtime_depends: [.properties[]? | select(.name=="sonic:runtime_depends") | .value],
     unresolved_deps: [.properties[]? | select(.name=="sonic:unresolved_deps") | .value]}' \
   "$SBOM"

# Walk the kernel-module → kernel-image edges (out-of-tree modules
# whose runtime ABI is pinned to the kernel)
jq '.dependencies[]
    | select(.dependsOn[] | startswith("pkg:deb/sonic/linux-image-"))
    | .ref' "$SBOM"

# Cross-reference: which SONiC .debs declared an unresolvable dep?
#   Useful to audit gaps between $(pkg)_DEPENDS declarations and the
#   set of recipe-emit fragments we actually produced.
jq '.components[]
    | select(.properties[]? | select(.name=="sonic:unresolved_deps"))
    | {name, version,
       unresolved: ([.properties[] | select(.name=="sonic:unresolved_deps") | .value] | first)}' \
   "$SBOM"
```

Tools that understand CycloneDX `dependencies[]` natively work
without any of these `jq` queries — e.g. `cyclonedx-cli analyze`,
or piping the SBOM into a graph visualizer. The examples above
exist for ad-hoc analysis from a shell.

## Known limitations

- **Per-.deb language dependency attribution is scoped to SONiC-
  built `.debs`.** `scripts/sbom_fragment.py` introspects each
  SONiC-built `.deb` and harvests Rust crates (from cargo-auditable
  `.dep-v0` ELF sections via `rust-audit-info`), Go modules (from
  `runtime/debug.BuildInfo` via `go version -m`), and Python
  distributions (from `*.dist-info/METADATA` walk). Components are
  scoped at the `.deb` level via `sonic:source_deb`, *not* per-binary
  within the `.deb`. Upstream Debian-archive `.debs` are intentionally
  out of scope — their language-dep metadata is captured by the syft
  scanner pass plus the observation manifests, not by `.deb`
  introspection.

- **`uv pip` skips the `pip3` build-hook shim**, so its installs are
  absent from the observation manifest. They still reach the SBOM via
  the scanner pass — uv writes standard `dist-info/` directories — but
  their `sonic:fragment_kind` property reads `scanner` rather than
  `observation`.

- **Vendored static libraries** statically linked into binaries (a
  `.deb` that bundles libcurl instead of dynamic-linking) are
  undetectable without source-code scanning. Not closed by this
  design.

- **Pip artifacts lack hashes** from pin time because SONiC's
  Python version-control doesn't use `--require-hashes`. The scanner
  may compute installation-time hashes from `RECORD` as a best-effort
  substitute.

- **Broadcom XGS SAI** historically used `_SKIP_VERSION=y`. The SBOM
  computes SHA-256 of the downloaded artifact independently and
  ignores the recipe flag.

- **Vendor EULA license names** are recorded as declared (e.g.,
  "Broadcom Inc. proprietary (EULA)") but not normalised to SPDX
  expressions, since most are not SPDX-listed.

## File map

Files that exist for this design.

| Path | Role |
|---|---|
| `rules/config` | Defines the four SBOM configuration variables. |
| `Makefile.work` | Threads SBOM variables into the slave-container environment. |
| `slave.mk` | Defines the `sbom_emit_fragment` helper, calls it from each `SONIC_*` artifact recipe, invokes `build_sbom.sh` between rootfs assembly and `.bin` wrap, and exposes the recipe context (installer docker/deb/wheel lists) to the aggregator. |
| `build_image.sh` | Emits the `.cdx.json` sibling after the `.bin` is wrapped. |
| `scripts/install_sbom_tool.sh` | Auto-fetches syft, grype, cyclonedx-cli with SHA-256 verify into `target/sbom-tools/`. |
| `scripts/build_sbom.py` | Aggregator. Walks fragments, runs the scanner, parses lockfiles, resolves licenses, dedupes, builds the CycloneDX `dependencies[]` graph (kernel-module → kernel-image edges, declared `.deb` build/runtime deps, recipe-emit-{rust,go,python} → owning `.deb` edges), and writes the SBOM + SPDX + provenance. |
| `scripts/build_sbom.sh` | Thin shim that execs `build_sbom.py`. |
| `scripts/sbom_fragment.py` | Per-recipe fragment generator. Knows the four ancestor patterns, the vendor-supplier URL table, and the per-`.deb` language-dep harvesters (Rust via `rust-audit-info`, Go via `go version -m`, Python via `*.dist-info/METADATA` walk). |
| `scripts/sbom_resolve_licenses.py` | DEP-5 parser + licensecheck fallback + SPDX translation. |
| `scripts/sbom_license_map.json` | ~100 Debian License header strings mapped to SPDX. |
| `scripts/sbom_parse_lockfiles.py` | Parses `go.sum`, `package-lock.json`, `pnpm-lock.yaml`, `yarn.lock` from harvested archives. Also parses `Cargo.lock` defensively for legacy archives — but Rust attribution comes from `sbom_fragment.py`'s `.dep-v0` introspection, not from this parser. |
| `scripts/sbom_emit_provenance.py` | Emits the unsigned SLSA v1.0 / in-toto provenance document. |
| `scripts/sbom_diff.py` | Reproducibility comparison between two SBOMs. |
| `scripts/sbom_vuln_scan.py` | Standalone CVE scanner (invokes grype, applies VEX). |
| `scripts/sbom_vuln_diff.py` | Standalone drift analysis between two vuln reports. |
| `scripts/sbom_extract_vex_from_patches.py` | Auto-VEX from CVE markers in patch metadata. |
| `vex/` | OpenVEX statements (curated at top level, auto in `vex/auto/`). |
| `vex/README.md` | VEX schema and triage workflow. |
| `src/sonic-build-hooks/scripts/collect_version_files` | Extended to harvest `copyrights.tar.gz` and `lockfiles.tar.gz` per scope when `ENABLE_SBOM=y`. |
| `scripts/prepare_docker_buildinfo.sh` | Injects `ENV ENABLE_SBOM` into each container's Dockerfile so the harvest hook sees the right value at install time. |
