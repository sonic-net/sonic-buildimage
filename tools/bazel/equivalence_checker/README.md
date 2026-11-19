# Compare existing Make and Bazel OCI images

This is the existing sonic-net equivalence checker, imported from commit
`54eb0632ba9dc93e784665280ce4ec19c122cef1` and the earlier local PR 1 port.
Its comparator checks plain-file bytes, symlink targets and ELF differences,
including detached debug files paired by build ID. It does not replace installed
runtime, service, mode, ownership or complete source-closure validation.

## Comparison without a build

From a clean checker checkout, select the separate clean source checkout recorded
by both producers. Its revision may predate this checker-only change:

```sh
tools/bazel/test_equivalence.sh --existing-oci /absolute/path/pairs.json \
  --source-checkout /absolute/path/producer-checkout \
  --report /absolute/path/equivalence-report.json --jobs 1
```

This entry invokes no Make, Bazel, package installation, artifact download or
DEB extraction. It verifies existing files and then runs the declared comparison
tools. It requires both runtime and debug archives for every required image.
Supported inputs are gzip-compressed Docker-save archives with `manifest.json`,
the transport emitted by the current OCI image export targets. Raw OCI layouts
are not accepted by this inherited extractor; this port adds no format converter.
Missing artifacts, provenance or tool files, hash drift, a dirty checkout,
source/configuration/lock drift, or Make provenance with Bazel enabled fail
before comparison. Make and Bazel cannot point to the same physical file.
The report records the producer and checker revisions separately. Alternate
option ordering and `--existing-oci=PATH` use the same no-build route; malformed
arguments fail before provisioning.
The report path must not alias the manifest or any declared input. The report
records the SHA256 of the manifest bytes validated during preflight.

The manifest is schema 1. Paths are absolute or relative to the manifest;
each file record has `path` and lowercase hexadecimal `sha256` fields:

```json
{
  "schema": 1,
  "required_images": ["docker-example"],
  "make_provenance": {"path": "make/provenance.json", "sha256": "<SHA256>"},
  "bazel_provenance": {"path": "bazel/provenance.json", "sha256": "<SHA256>"},
  "pairs": [
    {"name": "docker-example.gz", "make": {"path": "make/docker-example.gz", "sha256": "<SHA256>"}, "bazel": {"path": "bazel/docker-example.gz", "sha256": "<SHA256>"}},
    {"name": "docker-example-dbg.gz", "make": {"path": "make/docker-example-dbg.gz", "sha256": "<SHA256>"}, "bazel": {"path": "bazel/docker-example-dbg.gz", "sha256": "<SHA256>"}}
  ],
  "tools": {
    "readelf": {"path": "/declared/readelf", "sha256": "<SHA256>", "origin": "<producer revision and label>", "closure": [{"path": "/declared/readelf", "sha256": "<SHA256>"}]},
    "abidiff": {"path": "/declared/abidiff", "sha256": "<SHA256>", "origin": "<locked tool origin>", "closure": [{"path": "/declared/abidiff", "sha256": "<SHA256>"}]},
    "elfcompare": {"path": "/declared/elfcompare", "sha256": "<SHA256>", "origin": "<pinned compareELF producer>", "closure": [{"path": "/declared/elfcompare", "sha256": "<SHA256>"}]}
  }
}
```

Each tool closure must list its actual interpreter, loaders, libraries and
runfiles as applicable, not just the example executable. Use the selected
execution environment and preserve the executable's invocation path for Bazel
runfiles. No PATH discovery or tool building occurs in this mode.

Each separately retained producer provenance file has this shape (use
`builder: "bazel"` for the Bazel side and omit the Make-only switch):

```json
{
  "schema": 1,
  "builder": "make",
  "buildimage_commit": "<40-character checkout commit>",
  "clean": true,
  "command": ["<original command and arguments>"],
  "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n",
  "configuration": {"distribution": "trixie", "architecture": "amd64", "platform": "vs", "features": {"INCLUDE_FIPS": "y"}},
  "sources": {"<component>": {"revision": "<40-character source commit>", "patches": ["<ordered patch SHA256>"]}},
  "locks": {"<lock path or producer identity>": "<SHA256>"},
  "artifacts": {"docker-example.gz": "<SHA256>", "docker-example-dbg.gz": "<SHA256>"}
}
```

The source, configuration and lock maps must match exactly. Populate them from
the complete actual producer evidence, including selected registry sources and
patches; matching repository names or filenames is insufficient. The checker
validates retained records and hashes but cannot authenticate an invented build
record or prove a reviewer omitted no dependency. Retain the original commands,
logs and producer evidence and independently review the complete closure before
acceptance. Never create a provenance file merely to relabel historical outputs
as products of the current checkout.

### Source archives and different lock formats

Schema 1 retains its exact source/configuration/lock equality checks. Use
`schema: 2` in the manifest and both producer records when the actual sources
include release archives or the builders use different lock formats. This
changes input validation only; it still runs the same OCI extraction, comparison,
required-pair checks and diagnostic rules.

Schema 2 keeps the original commands, clean checkout, configuration and artifact
hashes above. Each project source has one of these forms:

```json
{"kind": "git", "revision": "<actual Git commit>", "patches": []}
```

```json
{
  "kind": "archive",
  "release": "<actual source release>",
  "archives": {
    "component.orig.tar.gz": {"path": "<retained original archive>", "sha256": "<SHA256>", "origin": "<immutable source URL>"},
    "component.debian.tar.xz": {"path": "<retained Debian packaging archive>", "sha256": "<SHA256>", "origin": "<immutable source URL>"}
  },
  "patches": [
    {"path": "<retained patch>", "sha256": "<SHA256>", "origin": "<patch owner and revision>"}
  ]
}
```

Git sources use the same retained-file records for their ordered patches.
Every archive and patch is rehashed. Archive identities are release names and
full SHA256 values; an archive hash must never be shortened into a Git revision.
The complete project source set, original identities and configuration must
match. Do not omit inherited project components or relabel them as external
packages.

`locks` contains each builder's original retained lock files, with `path`,
`sha256`, `origin` and the supported `format`. `external_dependencies` records
the resolved runtime packages, keyed by ecosystem/name/target, with their exact
version, artifact checksums, activated features, consuming archive names and
retained producer selection evidence. Checksums/features/images are sorted,
unique lists. Each selection must reference its original lock; every retained
lock must have selected runtime inputs. The checker verifies supported lock
contents and rejects unknown formats or selections the retained inputs cannot
establish. Preserve actual Make downloads and resolver logs, including any
fallback from its declared constraints. A constraints file alone cannot prove
which wheel was installed.

Different lock serialization does not establish a dependency difference. The
checker compares the resolved package identities. Any actual difference fails
unless `EXTERNAL_DEPENDENCY_DIFFERENCES` contains a reviewed rule binding the
exact dependency name, both complete selections, affected archive names and a
reason. That table is empty by default, and caller manifests cannot add rules.
An admission never accepts a file-content, ABI, ELF or service difference;
those still need their own evidence and narrowly named diagnostic rules.

For metadata-only patch differences, `SOURCE_PATCH_DIFFERENCES` must bind the
same original source identity, both exact ordered patch lists and a reason.
Both producers must retain their prepared source directories and a hashed
manifest with `schema: 1` and `entries`: each relative path maps to its type,
file SHA256 and mode, directory mode, or literal symlink target. The checker
rehashes and inventories the complete actual directories before comparing them.
Only the rule's exact `nonproduction_paths` may be excluded. Caller exclusions,
wildcards, stale exclusions, omitted files and changed production files fail.
Equal prepared files cannot excuse a different Git revision or source archive.
This rule table is also empty by default.

Schema 2 reports retain both complete producer records and every matched input
rule under `producer_inputs`. Original producer evidence still needs independent
review: a JSON receipt cannot authenticate a claimed build or establish that its
author included every runtime dependency.

Supported lock formats are `cargo-lock` (registry packages in Cargo.lock),
`requirements-lock` (exact Python requirements with SHA256 hashes), `apt-lock`
(the selected SONiC apt.lock.json), `debian-packages` (a retained Packages index),
and `versions-py3` (Make's exact Python constraints). The last format additionally
requires `artifacts`, a list of retained wheel file records. The checker reads
each wheel's METADATA and verifies its package, version and full archive hash.
It does not infer source identity from a wheel filename.

For example, a resolved external entry has this shape:

```json
{
  "cargo/example/x86_64-unknown-linux-gnu": {
    "lock": "owner-Cargo.lock",
    "version": "1.2.3",
    "checksums": ["<actual crate SHA256 from Cargo.lock>"],
    "features": ["default"],
    "images": ["docker-example-dbg.gz", "docker-example.gz"],
    "selection_evidence": [
      {"path": "selection.json", "sha256": "<SHA256>", "origin": "<original producer and collection command>"}
    ]
  }
}
```

The selection receipt has `schema: 1` and `dependencies` mapping the same key
to exactly `version`, `checksums`, `features` and `images` above. Retain the
original graph/resolver evidence behind that receipt. Lock parsing independently
checks the package identities; features and image consumers remain recorded
producer context and require review against the actual build graph.

Additional explicit OCI pairs are diagnostics. Their comparison findings remain
in the normal report, while `scope.required_status` and the exit status depend
on the required runtime/debug set. `scope.all_compared_status` retains the
aggregate verdict. A missing diagnostic artifact is reported by the original
extractor; a missing required artifact fails preflight. A required exclusion or
incomplete comparison cannot be accepted away. There are no accepted content or
version-drift rules by default.
Debug files are matched only within their runtime/debug image family. Malformed
archives are reported as unreadable for their own pair, and every comparable
file is checked even when both archives use the same layer name.
Layer operations through absolute rootfs symlinks may fail closed rather than
normalize the link's container-root semantics; literal symlink text is retained.
Inner layers reject device nodes, FIFOs and other special entries.

## Preserved legacy entry and validation limits

There is no implicit build mode. Both the shell and Python entry require the
deliberate `--build-all-pairs` opt-in for the retained all-pair collector and verdict.
The shell accepts that option alone; other invocation forms without
`--existing-oci` fail before any provisioning.
That route discovers DEBs and OCI labels, builds Make artifacts, resolves Bazel
tools, and may provision abidiff. It is **not** the no-build entry and must not be
used under a no-DEB or no-native-execution task. Its original discovery assumes
upstream `gzip rule` targets; this fork's `gzip_compress` artifacts must be supplied
explicitly through the existing-artifact entry until that discovery is separately
validated. Neither the opted-in legacy route nor a real image comparison was executed
for this port. No automatic CI/build trigger is added.

The existing OCI regression job runs the checker's pure Python test discovery.
The original rule-engine/reporter tests and focused manifest tests cover parsing,
provenance, pair coverage and scoped failure reporting using synthetic files.
Their success does not establish Make/Bazel image equivalence. Real comparisons
remain NOT_RUN until matching artifacts, complete provenance and a suitable
execution environment are available. Preserve all existing image/runtime checks.
