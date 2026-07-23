# AGENTS.md - SONiC Buildimage Contributor Guidance

## Scope and Purpose

This file contains repository-wide instructions for AI models. 
Keep it limited to durable build, editing, and review practices.
Project plans, progress tracking, design rationale, and migration reports must
not be duplicated here.

Before changing a component, read the nearest applicable `AGENTS.md`, its build
rule, and the associated template or source file. Do not infer the active build
environment, package version, or platform support from this file; verify it in
the checked-out branch and relevant build configuration.

## Overview

This repository is Canonical's maintained fork of the upstream
[`sonic-net/sonic-buildimage`](https://github.com/sonic-net/sonic-buildimage)
build infrastructure for SONiC, an open-source network operating system. It
builds ONIE-compatible switch installer images and service container images.
See [`README.md`](README.md) for general SONiC usage, supported platforms, and
build prerequisites.

## Branch and Origin

- **Canonical repository:** `canonical/sonic-buildimage`
- **Upstream source:** `sonic-net/sonic-buildimage`
- **Upstream release model:** Date-named release branches such as `202405` are
  maintained after their branch point; they are not immutable release tags.

The `feature_noble_build` branch is the reliable Ubuntu Noble (24.04) SONiC
reference implementation. It was migrated from the upstream Debian Bookworm
based `202405` branch.

The `202605_resolute` branch is the Ubuntu Resolute (26.04) SONiC implementation 
(still WIP on July 13, 2026). It was migrated from the upstream Debian Trixie
based `202605` branch.

## Build System

- The top-level `Makefile` dispatches to `Makefile.work`; build-environment
  selection is controlled by `BLDENV`.
- `slave.mk` defines the common build graph, package paths, and Docker targets.
  Make package or image changes in the relevant `rules/*.mk` file rather than
  bypassing that graph with ad hoc build commands.

Run the smallest relevant target first. Do not run destructive cleanup targets
unless the task requires them.

## Editing Rules

- Treat Jinja2 templates (`*.j2`) as source files. Generated Dockerfiles,
  scripts, manifests, and build artifacts are not the source of truth.
- Preserve the existing Docker variant chain. When adding or changing a variant,
  update its corresponding rules, base image relationship, and template context
  together.
- Pin package versions where a rule already pins them. Do not replace a pinned
  dependency with a rolling `latest`, `stable`, or meta-package without an
  explicit compatibility decision.
- Preserve source builds declared in `rules/*.mk` unless the change is explicitly
  approved and includes the corresponding build-graph update.
- Do not directly modify source code downloaded from external projects during a
  build. If a change is necessary, add a patch file and apply it
  explicitly from the relevant build rule or script.
- Prefer build flags or local compatibility patches for generated code and
  third-party headers. Do not directly edit generated output.
- Make a minimal, scoped change. Avoid unrelated formatting, generated-file
  updates, or broad dependency upgrades.

## Submodules

- Changes to submodule source must be committed in that submodule's repository
  and branch first; then update this repository's gitlink deliberately.
- Do not treat an uncommitted submodule worktree as a parent-repository source
  patch.
- Before changing a gitlink, verify its remote, branch, and intended commit.
- A submodule gitlink must be reachable on whatever remote its `.gitmodules`
  URL points to, or a clone of this repository cannot initialize that
  submodule. On `202605_resolute` a gitlink commit can be in one of three
  states:
  - **Upstream commit, upstream URL.** Canonical has not modified the
    submodule; the gitlink points at an upstream commit and the URL stays
    upstream (e.g. `sonic-net/`). No canonical fork or branch is involved.
  - **Canonical commit already pushed to `canonical/<submodule>`.** Canonical
    has modified the submodule; the gitlink commit lives on the `202605_resolute`
    branch of `canonical/<submodule>` and does not exist on `sonic-net/`. The
    URL must point at canonical for the clone to resolve.
  - **Canonical commit not yet pushed.** The gitlink names a local commit that
    exists only in the contributor's submodule worktree — on no remote at all,
    so any clone of this repository will fail to initialize the submodule until
    it is pushed.
  The state is not determined by the URL alone — a submodule can carry
  Canonical commits while its URL still names an upstream remote, or carry a
  commit that has not been pushed at all. When the gitlink commit is not an
  upstream commit, push it to `canonical/<submodule>:202605_resolute` — never to
  `sonic-net/`, which is upstream and not ours to write. Before committing a
  gitlink bump, confirm the commit actually exists on the remote the URL names;
  if the commit is still local-only, push it to
  `canonical/<submodule>:202605_resolute` first, and point the URL at
  `canonical/<submodule>` if it does not already, before the parent-repository
  commit lands.
- To pick up upstream changes in a submodule Canonical has modified, fetch the
  upstream remote and rebase the submodule's `202605_resolute` branch onto the
  upstream branch, then bump this repository's gitlink to the rebased commit.
  Do not fast-forward over the resolute-specific commits.

## Resolute Migration Work

For work specifically targeting the Debian Trixie to Ubuntu Resolute migration:

- Use Ubuntu Resolute (26.04) only. Do not substitute an earlier Ubuntu release.
- Preserve the procured kernel ABI unless the task explicitly approves changing
  it.
- Keep Docker and containerd packages pinned to the versions selected by the
  relevant rule; do not use rolling package channels.
- Treat explicitly documented skipped components and accepted workarounds as
  intentional. Do not "fix" them without a task that changes the documented
  decision.
- Follow the established Resolute Docker variant naming and template flow rather
  than altering the Trixie path as a shortcut.
- When an uncertain migration issue arises, inspect the equivalent Debian
  Bookworm-to-Noble migration in `feature_noble_build`. Determine whether its
  solution applies to Resolute, then provide developers with a concise summary
  of the approach, feasibility, and trade-offs before implementing it.
- Do not let a non-critical feature block the overall migration indefinitely.
  When its migration cost is disproportionate, assess the impact of disabling
  it and present that assessment to developers for a decision. The Noble
  migration's disabled FIPS and DASH engine functionality are examples of this
  type of explicit scope decision.

The migration documentation directory, `docs/`, exists only on the
`202605_resolute_doc` branch. It is intentionally absent from the migration
work branch, `202605_resolute`. The following documents on
`202605_resolute_doc` are authoritative for migration design, implementation
plans, current status, compatibility decisions, verification, and
Resolute-specific behavior. Do not duplicate or edit these documents unless the
task explicitly requests documentation work. The English documents are the
source of truth.

- [Migration design](https://github.com/canonical/sonic-buildimage/blob/202605_resolute_doc/docs/superpowers/specs/2026-07-03-sonic-202605-resolute-migration-design-en.md)
- [Migration implementation plan](https://github.com/canonical/sonic-buildimage/blob/202605_resolute_doc/docs/superpowers/plans/2026-07-03-sonic-202605-resolute-migration-plan-en.md)
- [VS migration report](https://github.com/canonical/sonic-buildimage/blob/202605_resolute_doc/docs/superpowers/resolute-vs-migration-report-en.md)
- [Modification catalog](https://github.com/canonical/sonic-buildimage/blob/202605_resolute_doc/docs/superpowers/resolute-modification-catalog-en.md)
- [Migration code review](https://github.com/canonical/sonic-buildimage/blob/202605_resolute_doc/docs/superpowers/resolute-migration-code-review-en.md)

## Change Verification

- Run the narrowest build, lint, or test target that covers the change. State
  clearly if verification cannot be run and why.
- For build-environment, base-image, package, or Docker changes, verify the
  selected `BLDENV` and inspect the rendered inputs or build logs as appropriate.
- Inspect `git diff` and `git status` before handing off changes. Preserve
  unrelated worktree changes.

## Git Hygiene

- Use a concise commit prefix appropriate to the change, such as `build:`,
  `fix:`, `docs:`, or `test:`.
- Do not commit local configuration, build output, generated artifacts, editor
  settings, or presentation files covered by `.gitignore`.
- In `.gitmodules`, submodule URLs must be `https`, not `ssh`, so anonymous
  and CI clones work without credentials. When changing a URL, edit the
  existing `[submodule "<short-name>"]` section in place; never append a
  duplicate `[submodule "<path>"]` section or omit `path =`, since git silently
  ignores sections without `path =` and the old URL stays effective.
- Upstream date-named branches can be force-rewritten with replayed history
  but an identical tree. A compare showing thousands of changes and an
  implausibly old merge-base usually means a force-rewrite, not a local
  mistake; confirm with `git merge-base` and `git diff` before rebasing.
