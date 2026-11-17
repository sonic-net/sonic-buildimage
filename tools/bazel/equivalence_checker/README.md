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
