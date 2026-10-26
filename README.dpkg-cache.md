# DPKG build caching and cache-key validation

SONiC's build cache reuses previously built artifacts to reduce build time.
A correct cache key must capture the inputs that affect the artifact: source
and patch content, build flags, dependencies, and relevant build tooling.
Otherwise, changing an input can reuse an artifact built with the old input.

This guide complements the [SONiC Buildimage Guide](README.buildsystem.md).
The implementation is in [Makefile.cache](Makefile.cache), with package
declarations in `rules/*.dep` and `platform/**/*.dep`.

## What the PR gate does

The Azure Pipelines check **dpkg cache-key completeness (PR gate)** runs
[`scripts/audit_dep_completeness.sh`](scripts/audit_dep_completeness.sh).
It statically checks that build inputs are represented in cache dependency
declarations. Despite its name, the audit also covers Docker images, Python
packages, and the host root filesystem.

The gate does not build an image or compare binaries. It runs in parallel
with platform builds. In PR mode (`--base <target-branch-ref>`), it uses the
PR diff and finding provenance to identify blocking gaps. A pre-existing gap
can become blocking when a PR touches the relevant build-input file; a failure
does not necessarily mean the PR introduced the original omission.

| Result | Meaning |
| --- | --- |
| P0 | Blocking cache-key finding; audit exits with status 1. |
| P1/P2 | Non-blocking risk findings requiring investigation. |
| P3 | Informational finding, including documented waivers. |
| Script/setup error | Investigate checkout, dependencies, or script execution rather than adding a cache waiver. |

A successful static audit is not proof that all cached artifacts are equivalent
to fresh builds.

## Find and reproduce a failure

1. Open the PR's failed Azure Pipelines check.
2. Open **dpkg cache-key completeness (PR gate)**, then
   **Audit PR for untracked build-cache inputs**.
3. Read the P0 rows and their explanations and suggested fixes. Identify the
   target, the input being consumed, and the file that should track it.

Run the same audit from the repository root on your PR branch:

```bash
# Use the remote for sonic-net/sonic-buildimage, not an out-of-date fork.
# Substitute the PR's actual target branch when it is not master.
git fetch upstream master
git submodule update --init --recursive
bash scripts/audit_dep_completeness.sh --verbose --base upstream/master
```

Here `upstream` must point to `sonic-net/sonic-buildimage`; use `origin` if
that is your upstream remote. A full-history checkout and initialized submodules
are important, particularly for rules owned by submodules. Resolve missing-base
or checkout warnings before trusting the result.

PR-diff scoping compares committed changes through `HEAD`. Commit the proposed
fix before the final `--base` run so its changed-file scope matches CI.
For investigation, a whole-tree scan also reads current working-tree content:

```bash
bash scripts/audit_dep_completeness.sh --verbose
bash scripts/audit_dep_completeness.sh --json > /tmp/cache-key-audit.json
```

Without `--base`, the audit is a backlog report, not the PR gate. A zero exit
status in that mode does not mean all findings have been fixed.

## Fix the input tracking, not just the check

Trace the input from the build recipe to the cache key. Prefer the narrowest
existing package-specific mechanism:

| Finding | Typical fix |
| --- | --- |
| Build flag changes package content | Track its value in the owning target's `_DEP_FLAGS`. |
| Recipe, helper, or template is not hashed | Add its path to the appropriate `_DEP_FILES` list. |
| Package source is not hashed | Follow the existing `_SMDEP_FILES` and `_SMDEP_PATHS` pattern for that source tree. |
| Source-built package lacks cache declarations | Add a `.dep` file using a comparable package as the reference, and ensure the build includes it. |
| Derived package is missing from saved artifacts | Register it under the correct top-level package with `add_derived_package`. |
| Built artifact is consumed without a dependency edge | Declare the appropriate build dependency in the owning `.mk` file. |
| Host rootfs input is not tracked | Inspect `RFS_DEP_FILES` or the RFS-specific flag wiring in `Makefile.cache`. |

For example, if `MY_FEATURE` changes a package's build output, its owning
`.dep` file can include:

```make
$(MY_PACKAGE)_DEP_FLAGS += $(MY_FEATURE)
```

If a helper script affects that package's output:

```make
$(MY_PACKAGE)_DEP_FILES += scripts/my_package_helper.sh
```

These are illustrative declarations: use the actual package variable and check
that no later `:=` assignment overwrites them. Preserve existing common flags,
file lists, source tracking, and dependency declarations. A `.dep` file must be
wired into the build, not merely exist.

Keep flags package-specific unless they genuinely affect every target. Do not
put raw credentials in `_DEP_FLAGS`; use an existing secret-fingerprint mechanism
where applicable. Sibling in-repository quilt patch directories are tracked
generically by `Makefile.cache`; inspect that wiring before adding duplicate
package-specific enumeration.

After fixing the declaration, rerun the PR-mode audit and perform the relevant
package validation. For a cache-correctness change, verify that changing the input
changes the affected key, and that cache restore or rebuild produces the intended
artifact. Merely clearing the cache or building once with caching disabled does
not fix an incomplete key.

## Use an AI assistant to investigate

Give a repository-aware, organization-approved AI coding assistant the P0
finding, the PR diff, and the relevant `.mk`/`.dep` files. Do not include
credentials or other sensitive build values. For example:

```text
Investigate this dpkg cache-key completeness gate failure:
<paste the P0 finding and explanation>

Trace the reported input through the build recipe and Makefile.cache.
Check whether it changes the cached artifact and whether it is already
tracked directly or transitively. Propose the smallest correct fix using
existing package-specific declarations. Preserve existing dependencies.
Do not disable the gate or add a waiver just to make it pass.
Explain why the key now tracks the input, rerun the audit against the PR
target branch, and report the exact command and result.
```

Review the proposed change yourself. AI output and a passing static check are
not substitutes for understanding which artifact consumes the input.

## Suspected false positives and review

If the input is already keyed or cannot affect the cached artifact, document the
actual data flow. For example, an installer-only input consumed after cached
rootfs restoration may not belong in the first-stage rootfs key.

[`scripts/cache_key_export_waivers.tsv`](scripts/cache_key_export_waivers.tsv)
contains justified variable waivers. Waivers are not a general-purpose way to
suppress failures: check all uses covered by a waiver and explain why none create
an untracked cached-content input. If the audit fails to recognize existing
tracking, prefer a focused analyzer correction with a regression test.

Changes to the analyzer or waiver file should include the before/after findings
and evidence that genuine missing inputs are still reported. Separate broader
audit corrections from unrelated package changes when practical.

**Add `@aaronber0614` as a reviewer on PRs fixing failures from this gate,
including changes to the audit script or its waivers.** Include the finding,
the reason for the fix, and the audit command and result in the PR description.
