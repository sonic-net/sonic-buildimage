# mega-generic: generic mega-container merge generator

**Status: implemented, iterating on correctness.** Feature auto-discovery
(`mega_gen/auto_registry.py`), per-feature asset discovery
(`mega_gen/discovery.py`), the supervisord merge engine
(`mega_gen/supervisord_merge.py`), `rules/docker-mega.mk` generation
(`mega_gen/gen_mega_mk.py`), `dockers/docker-mega/Dockerfile.j2` +
`docker-mega-init.sh` generation (`mega_gen/gen_dockerfile.py` +
`mega_gen/gen_docker_init.py`), the shared build-template patches
(`mega_gen/patch_templates.py` -- `docker_image_ctl.j2`, `init_cfg.json.j2`,
`sonic_debian_extension.j2`, `per_namespace/syncd.service.j2`, `rules/config`,
each folded feature's own `.mk`, plus `mega.service.j2` generation), and the
dry-run summary/validation report (`mega_gen/report.py`) are all implemented
and wired together by `mega_gen_cli.py --apply`. `ctl_parser.py` (a
1000+-line parser for `docker_image_ctl.j2`'s per-container-name branches)
exists but is currently unused by `patch_templates.py`, which instead patches
the shared template with targeted, anchor-based text edits; see
`mega_gen/report.py`'s `anchor_*` helpers for the idempotency pattern those
edits follow.

For any chosen feature subset, this generator will read each feature's
original per-container source files in a sonic-buildimage tree and
mechanically compose `dockers/docker-mega/*` plus the build-template edits
needed to fold them into one container. See the plan for the full design.

## Layout

```
optimizations/mega-generic/
├── README.md
├── mega_gen_cli.py               # CLI entry point (--list-registry / --list-assets / --apply)
├── features.default.yaml         # default feature list (used when --features omitted)
├── test_supervisord_merge.py     # validation harness for supervisord_merge.py
├── test_gen_mega_mk.py           # validation harness for gen_mega_mk.py
├── test_gen_dockerfile.py        # validation harness for gen_dockerfile.py + gen_docker_init.py
├── test_patch_templates.py       # renders the patched docker_image_ctl.j2 with Jinja2 and inspects `docker create`
├── test_sonic_debian_extension.py  # validation harness for the sonic_debian_extension.j2 service filter
└── mega_gen/
    ├── __init__.py
    ├── auto_registry.py      # auto-discovers feature -> docker-dir/mk/include-flag
    ├── discovery.py           # per-feature asset discovery -> FeatureSpec
    ├── supervisord_merge.py   # generic supervisord config merge engine
    ├── gen_mega_mk.py         # generates rules/docker-mega.mk
    ├── gen_docker_init.py     # generates per-feature preinit scripts + docker-mega-init.sh
    ├── gen_dockerfile.py      # generates dockers/docker-mega/Dockerfile.j2 (hybrid strategy)
    ├── patch_templates.py     # patches the shared build templates (docker_image_ctl.j2 etc.)
    ├── ctl_parser.py          # standalone parser for docker_image_ctl.j2 (currently unused)
    └── report.py              # dry-run summary, anchor_* idempotent-patch helpers, validate_dry_run
```

## Where this runs

The generator is meant to run on the **ARM build VM**, against that VM's
`sonic-buildimage` tree (e.g. `/build-sonic-arm/sonic-buildimage`), per the
`sonic-code-changes-on-vm` workspace rule. Discovery itself (`auto_registry`)
is read-only and works against any sonic-buildimage checkout, including the
local reference tree at `sonic-buildimage/` in this repo -- useful for
poking at the registry without touching the VM.

## Usage

```bash
# Show everything the generator can currently see in a tree:
python3 mega_gen_cli.py --sonic-root /path/to/sonic-buildimage --list-registry

# Resolve the default feature list (features.default.yaml) against it:
python3 mega_gen_cli.py --sonic-root /path/to/sonic-buildimage

# Or an explicit feature list:
python3 mega_gen_cli.py --sonic-root /path/to/sonic-buildimage \
    --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr

# Show the per-feature asset discovery (mega_gen.discovery) for the
# resolved feature list -- supervisord conf(s), critical_processes,
# base_image_files/, init script, .common.j2 variants:
python3 mega_gen_cli.py --sonic-root /path/to/sonic-buildimage --list-assets

# Actually generate dockers/docker-mega/*, rules/docker-mega.mk, and the
# shared-template patches, and write them under --out-dir (mirroring the
# sonic-buildimage tree layout so the output can be diffed/copied straight
# in):
python3 mega_gen_cli.py --sonic-root /path/to/sonic-buildimage \
    --apply --out-dir /tmp/mega-out

# Without --apply, the CLI performs a dry run: it computes everything
# --apply would write and prints report.validate_dry_run()'s summary
# (per-feature asset coverage, warnings, and any structural problems found)
# without touching any file.
```

## Feature registry

`mega_gen.auto_registry.discover_registry()` parses every
`rules/docker-*.mk` that defines `_CONTAINER_NAME` and derives, per feature:
the docker dir stem, the defining `.mk` path, whether it's gated behind an
`ifeq ($(INCLUDE_*), y)` block (and which flag), or installed unconditionally
("core"). A couple of features (currently just `bgp` / `docker-fpm-frr.mk`)
have no install line in their own file -- those are resolved via a wrapper-file
fallback pass (e.g. `docker-fpm.mk`'s `SONIC_ROUTING_STACK` switch).

The feature universe is **open**: any container discovered this way can be
requested. The 11 features listed in the plan
(`database, swss, bgp, teamd, lldp, snmp, gnmi, radv, eventd, sysmgr, pmon`)
are first-class/validated (`PROVEN_FEATURES`); anything else is accepted but
flagged `unproven`, never silently refused. `syncd` is always hard-excluded.

## Per-feature asset discovery

`mega_gen.discovery.discover_features(sonic_root, resolved_features, registry)`
takes the registry entries above and, for each feature, **recursively**
scans its docker dir for the assets the rest of the pipeline needs -- again
with no hand-maintained per-feature table, since naming varies a lot across
containers (verified against the tree, see the module docstring):

- **supervisord.conf** -- a `*supervisord.conf.j2` template (most
  containers; some prefix it, e.g. `docker-pmon.supervisord.conf.j2`), a
  static `supervisord.conf` (gnmi/eventd/sysmgr/nat/...), and it isn't
  always at the top level -- bgp's lives under `frr/supervisord/`.
- **supervisord.conf.common.j2** -- the reusable per-process fragment the
  main conf `{% include %}`s (only swss/bgp/teamd/lldp/nat/sflow have one
  today).
- **critical_processes** -- static or `.j2`-templated; several tiny
  watchdog/sidecar containers genuinely have none at all (flagged as a
  warning, not guessed).
- **base_image_files/** -- host CLI wrapper dir, if present.
- **init script** -- resolved from the feature's own `Dockerfile.j2`
  `ENTRYPOINT`: either a literal `.sh` copied verbatim, a `.j2` template
  rendered into the init script at build time (swss, pmon), or nothing at
  all when the container's entrypoint is `/usr/local/bin/supervisord`
  directly (gnmi, eventd, sysmgr, ...).
- **Dockerfile.common.j2** and every other `*.common.j2` file under the
  docker dir (generic catch-all).

Every `FeatureSpec` carries a `warnings` list for anything discovery
couldn't confidently resolve (missing file, multiple candidates) instead of
silently guessing.

## `rules/docker-mega.mk` generation

`mega_gen.gen_mega_mk.generate_mega_mk(specs, sonic_root)` takes the
discovered `FeatureSpec`s and produces the generated `rules/docker-mega.mk`
(+ `.dep` companion) text, covering everything `_INCLUDE_DOCKER`'s automatic
`_DEPENDS`/`_PYTHON_WHEELS`/`_FILES` union does **not** already handle:

- **`_INCLUDE_DOCKER`** -- one line per selected feature (dep union + build
  contexts, via the existing `add_docker_feature`/`process_include_dockers`
  in `rules/functions`).
- **`_LOAD_DOCKERS`** -- the base layer (`*_SWSS_LAYER*` if `swss` is
  selected, else `*_CONFIG_ENGINE*`), scanned out of the actual `_LOAD_DOCKERS`
  line(s) in each feature's own `.mk` (not hardcoded to a distro suffix like
  `TRIXIE`), **plus** one entry per selected feature with no
  `Dockerfile.common.j2` so `gen_dockerfile.py` can load its already-built
  image as an rsync source stage. Verified against this tree: **no**
  `Dockerfile.common.j2` currently exists anywhere under `dockers/` (not even
  for `docker-sonic-vs`'s "proven" precedent), so today every selected
  feature falls into this bucket -- the check is still done generically, not
  hardcoded, so it adapts if that ever changes.
- **`_INSTALL_PYTHON_WHEELS` / `_INSTALL_DEBS`** -- unioned from whichever
  selected features define their own (most don't; `eventd` does).
- **`_BASE_IMAGE_FILES`** -- unioned `name:dest` pairs from every selected
  feature, with the underlying host-side CLI wrapper / monit-config file
  contents **retargeted**: literal `docker exec <feature> ...`,
  `memory_checker <feature> ...`, `restart_service <feature>` references get
  rewritten to the merged container's name (`mega` by default). Collisions
  (two features shipping a `_BASE_IMAGE_FILES` entry with the same source
  name) and anything the retargeter can't confidently rewrite are reported
  in `MegaMkResult.warnings`, never silently dropped/guessed.
- **`_WARM_SHUTDOWN_BEFORE`/`_AFTER`, `_FAST_SHUTDOWN_BEFORE`/`_AFTER`** --
  unioned from whichever selected features define their own, **filtering
  out self-references**: e.g. `lldp`'s own `WARM_SHUTDOWN_BEFORE = swss`
  becomes meaningless once `lldp` and `swss` are both folded into mega, so
  it's dropped; `swss`'s own `WARM_SHUTDOWN_BEFORE = syncd` still targets a
  container outside mega (`syncd` is always hard-excluded) so it's kept.
  Best-effort: no actual runtime consumer of these fields was found
  anywhere in this tree (only the `manifest.json` producer in
  `rules/functions`) -- carried through for correctness, flagged as
  unconfirmed rather than asserted as load-bearing.
- **`_RUN_OPT`** (the `docker create` flags Makefile variable) -- unioned
  from every selected feature's own `_RUN_OPT`, with real collision
  handling rather than naive concatenation (`mega_gen.gen_mega_mk.merge_run_opt`).
  Verified against **all 29** `docker-*.mk` files that define `_RUN_OPT`
  (not just the 10 proven features): every shared `-v` bind-mount
  destination across the whole registry uses an identical source+mode
  *except one* -- `p4rt` mounts `/etc/sonic` as `rw` while the other 26
  features mount it `ro`. Per-project policy:
  - **Bind-mount mode conflicts** (`ro` vs `rw` on the same destination):
    `ro` always wins (assume best practice); only ONE `-v` line is ever
    emitted per destination, never both -- verified against the real
    `swss`+`p4rt` case.
  - **Bind-mount source conflicts** (different host paths to the same
    destination -- no current real case): first-seen source wins, loud
    warning naming everyone dropped.
  - **Host-port publishing** (`-p`/`--publish`) is treated as a genuinely
    scarce, unshareable resource: two selected features claiming the same
    host port produce an unmistakable `ERROR:`-prefixed warning rather
    than a silent clobber; distinct ports are assumed correct and kept.
  - Everything else (`-t`, `--cap-add=X`, `--security-opt VALUE`,
    `--privileged`) is a plain exact-string union/dedup -- `--privileged`
    gets no special treatment and overlapping-but-not-identical
    destinations are not detected, both by explicit scope decision.
  This only covers the Makefile-variable half of `docker create`'s flags --
  the ~15 `{%- if docker_container_name == "X" %}` blocks hardcoded
  directly into `docker_image_ctl.j2` (swss's `-e ASIC_VENDOR=...`, bgp's
  frr mount, database's `$DB_OPT`, etc.) still need `ctl_parser.py` to
  compose a new `mega` branch.

Out of scope here (owned by other to-dos): the `docker_image_ctl.j2`
per-container-name template blocks (`ctl_parser.py`'s job, see above), the
Dockerfile/init-script/supervisord content itself
(`gen_dockerfile.py`/`gen_docker_init.py`/`supervisord_merge.py`), and debug
(`_DBG`) image generation (omitted entirely for this PoC).

```bash
# Print what generate_mega_mk() would produce for a feature list, and
# optionally write the .mk/.dep/base_image_files/ to disk:
python3 -m mega_gen.gen_mega_mk --sonic-root /path/to/sonic-buildimage \
    --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr \
    --out-mk /tmp/docker-mega.mk --out-dep /tmp/docker-mega.dep \
    --out-base-image-files-dir /tmp/mega-base-image-files

# Validation harness (8 scenarios + synthetic RUN_OPT collision unit tests:
# default 10, no-swss, swss-layer subset, single-feature edge cases,
# shutdown-order self-reference filtering, the real p4rt /etc/sonic mode
# conflict, plus synthetic source-path/host-port collision cases):
python3 test_gen_mega_mk.py --sonic-root /path/to/sonic-buildimage
```

## `dockers/docker-mega/Dockerfile.j2` + `docker-mega-init.sh` generation

Two modules, split per the `gen-dockerfile-init` to-do (the plan's own
section 2/5 file list bundles both into one `gen_dockerfile.py` -- this repo
splits them into two so each has a single, independently-testable job):

- **`mega_gen.gen_docker_init.generate_docker_init(specs)`** -- the
  "preinit pattern" (plan section 7). Every selected feature whose own
  `Dockerfile.j2` `ENTRYPOINT` is a wrapper init script (not
  `/usr/local/bin/supervisord` directly -- i.e. `FeatureSpec.
  uses_supervisord_directly is False`; database/swss/bgp/teamd/lldp/snmp/
  radv in the default 10) has that script's/template's own text taken
  **verbatim**, with:
  - every `exec .../supervisord` line commented out (there can be more
    than one -- `database`'s own init script has an early-return
    `chassisdb` branch with its own `exec`), and
  - its own literal `/etc/supervisor/conf.d/supervisord.conf` /
    `/etc/supervisor/critical_processes` references rewritten to its own
    `/opt/sonic/core-services/<feature>/` isolation dir instead, so it
    can't clobber mega's *shared* merged versions of those same two
    conventional paths (`supervisord_merge.py`'s own output, rendered
    once by `docker-mega-init.sh` itself).

  swss is the one default feature whose init script (`docker-init.j2`) is
  itself a Jinja2 **template** (rendered at build time from one boolean
  knob, `ENABLE_ASAN`) rather than a literal `.sh` -- this module preserves
  that Jinja text verbatim rather than trying to render it itself (exactly
  how `supervisord_merge.py` treats supervisord `.j2` sources); it's
  `gen_dockerfile.py`'s job to re-run the equivalent build-time
  `sonic-cfggen` render before mega's container ever starts.

  The result also includes the single `docker-mega-init.sh` orchestrator:
  create every selected feature's isolation dir, run each stripped preinit
  **in its own subshell process** (in the same canonical feature order
  `supervisord_merge.py` uses -- isolates shell function/variable names
  across features, since the *source* scripts were never written with
  concatenation in mind), render mega's own merged `supervisord.conf.j2`
  exactly once via `sonic-cfggen -d`, then `exec supervisord` -- mirroring
  the tail of every individual feature's own (now-stripped) init script,
  just done once for the whole container. Features with
  `uses_supervisord_directly` (gnmi/eventd/sysmgr in the default 10) need
  no preinit at all -- their static `supervisord.conf`'s `[program:X]`
  stanzas are already folded into mega's own merged conf by
  `supervisord_merge.py`.

  **Documented deviation from the plan's own section-6/7 illustrative
  snippet** (the plan file itself isn't edited, per the assignment, so the
  reasoning lives here instead): that snippet's `install .../etc/supervisor/
  conf.d/supervisord.conf /opt/sonic/core-services/<svc>/supervisord.conf`
  assumes a pre-rendered file that, verified against every dynamic-init
  feature's own init script in this tree, does **not** exist pre-runtime
  (it's rendered by `sonic-cfggen -d ...` at container *startup*, reading
  live CONFIG_DB -- not baked into the built image at all). This module's
  runtime retargeting (above) is the correctness-preserving realization of
  the same intent ("collect generated supervisord configs into
  `/opt/sonic/core-services/<svc>/`") without trying to `install` a file
  that was never actually there.

- **`mega_gen.gen_dockerfile.generate_dockerfile(specs, sonic_root)`** --
  the Dockerfile itself, two-stage (`FROM $BASE AS base` does all the work;
  `FROM $BASE` + the shared `rsync_from_builder_stage()` macro copies the
  result into a clean final stage -- the same shape every feature's own
  `Dockerfile.j2` already uses, not invented for mega), hybrid per feature
  (plan section 6):
  - **`Dockerfile.common.j2` include** for features that have one
    (database/swss/bgp/lldp in the default 10, verified against this tree
    -- `docker-sonic-vs`, at `platform/vs/docker-sonic-vs/Dockerfile.j2`,
    is a live precedent for this exact pattern): `{% set copy_from =
    "<build-context-name>" %} {% include "<stem>/Dockerfile.common.j2" %}`,
    sourced from that feature's own **source directory** via the
    `--build-context`/`-I` machinery `_INCLUDE_DOCKER` already wires up
    (`rules/functions`) -- no built image needed. Supplemented,
    best-effort, with any top-level `*.j2` file that fragment didn't
    already cover (verified needed for `swss`'s own runtime templates,
    e.g. `switch.json.j2`) -- flagged as non-exhaustive, not a general
    Dockerfile.j2 parser.
  - **rsync from its own already-built image** for features with no
    `Dockerfile.common.j2` (teamd/snmp/gnmi/radv/eventd/sysmgr in the
    default 10): a `FROM docker-<stem>-{{DOCKER_USERNAME}}:{{DOCKER_USERTAG}}
    AS <feat>-layer` stage (the local tag `slave.mk`'s `docker-image-load`
    macro produces once `gen_mega_mk.py`'s `_LOAD_DOCKERS` addition for
    that feature has been loaded) + `RUN --mount=type=bind,from=<feat>-layer,
    target=/svc rsync -axAX ... /svc/ /`, excluding `/sys /proc /dev
    resolv.conf` + (deliberately) `/etc/supervisor/conf.d/` +
    `/usr/bin/start.sh`. Captures each feature's *entire* actually-built
    filesystem (apt-installed binaries like `radv`'s `radvd`, Python
    packages `snmp`'s `sonic_ax_impl install` touches, etc.) without
    needing to re-parse or re-execute that feature's own Dockerfile RUN
    commands.
  - **`start.sh` relocation**, uniformly for **every** selected feature
    that has one (regardless of bucket -- `supervisord_merge.py`'s own
    shared `[program:start]` requires every "starter" feature's
    `/opt/sonic/core-services/<feat>/start.sh` to exist, so this isn't
    optional): a plain `COPY --from=<ctx> ["start.sh", "/opt/sonic/
    core-services/<feat>/start.sh"]` from that feature's own build
    context -- always a static source file, never build-time generated,
    so this sidesteps rsync/`_LOAD_DOCKERS` entirely, even for the rsync
    bucket.
  - **preinit placement**, from `gen_docker_init.py`'s own output: a plain
    `COPY` + `chmod` for a literal `.sh`; `COPY` + a build-time
    `sonic-cfggen -a "{\"ENABLE_ASAN\":\"{{ENABLE_ASAN}}\"}" -t ...` render
    + `chmod` for swss's `.j2` (mirrors that feature's own Dockerfile.j2
    build-time render of `docker-init.j2` verbatim).
  - **`bgp`-only structural fix** (plan section 12, "FRR user/group
    creation"): `groupadd`/`useradd`/`chown /etc/frr` -- `bgp`'s own
    Dockerfile.j2 does this itself, but *outside* its own
    `Dockerfile.common.j2`, so it doesn't come along for free via the
    include above and is replicated explicitly here when `bgp` is
    selected.
  - Mega's own generated files (`supervisord_merge.py`'s merged
    `supervisord.conf.j2`/`critical_processes` + this module's own
    `docker-mega-init.sh`) are `COPY`'d in **last**, so they always win any
    basename collision with a per-feature glob copy above. Final
    `ENTRYPOINT` is `docker-mega-init.sh`, never any single feature's own
    entry point.

  Base layer (`ARG BASE=...`) selection mirrors `gen_mega_mk.py`'s own
  (`*_SWSS_LAYER*` if `swss` selected, else `*_CONFIG_ENGINE*`) but is
  **re-derived independently** from the same `FeatureSpec`s rather than
  importing `MegaMkResult` -- the plan's own section-5 pipeline diagram
  draws `gen_mega_mk.py` and `gen_dockerfile.py` as parallel, independent
  consumers of `discovery.py`, not of each other.

```bash
# Print what generate_docker_init() would produce, and optionally write the
# preinit-<feature>.sh(.j2) files + docker-mega-init.sh to disk:
python3 -m mega_gen.gen_docker_init --sonic-root /path/to/sonic-buildimage \
    --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr \
    --out-dir /tmp/mega-init

# Print what generate_dockerfile() would produce (internally calls
# generate_docker_init() too), and optionally write everything to disk:
python3 -m mega_gen.gen_dockerfile --sonic-root /path/to/sonic-buildimage \
    --features database,swss,bgp,teamd,lldp,snmp,gnmi,radv,eventd,sysmgr \
    --out-dir /tmp/mega-dockerfile

# Validation harness (8 scenarios: default 10, no-swss, swss-layer subset,
# single-feature edge cases, bgp-without-swss, common-bucket-only,
# rsync-bucket-only -- checks bucketing, Jinja2 syntax validity, base-layer
# agreement with gen_mega_mk.py, exactly-once COPY/FROM/mount emission,
# the FRR fix, start.sh/preinit placement + ordering, and exec-supervisord/
# shared-path-retargeting sanity):
python3 test_gen_dockerfile.py --sonic-root /path/to/sonic-buildimage
```

## Known limitations

These are real gaps identified by static review and, where feasible,
partially or fully fixed; the ones below are deliberately left as
documented limitations rather than implemented, because a correct fix
requires either changes outside this tree or a much larger structural
change than the surrounding fix pass, and there is no execution
environment available here to validate the riskier option end-to-end:

- **Redis-dependent preinit scripts still run before redis is up**
  (bgp/swss/teamd/snmp/radv's own init scripts call `sonic-cfggen -d`,
  which reads CONFIG_DB from redis). `docker-mega-init.sh` tolerates the
  resulting failures (`|| true`) rather than blocking the whole
  ENTRYPOINT, and `database`'s own preinit is special-cased so the merged
  `supervisord.conf.j2` still gets fed `database_config.json` +
  `additional_data_json` (so redis-server programs render correctly --
  this part *is* fixed). The remaining features' `-d`-dependent config
  (bgpd/zebra.conf, teamd/snmp/radv templates) is only regenerated the
  next time something re-invokes `sonic-cfggen -d` at runtime (e.g. a
  config reload), not automatically once redis comes up. A full fix
  would convert each of these into a `[program:preinit-<feat>]`
  supervisord one-shot gated on `redis-server:running` via
  `dependent_startup_wait_for`, with that feature's real entry program(s)
  in turn waiting on the one-shot's `:exited` state -- this touches the
  core chaining logic in `supervisord_merge.py` broadly enough (every
  affected feature's terminal-program wait-edges) that it was judged too
  risky to land without a real container boot to validate against.
- **Multi-ASIC / chassis database extras** (`chassisdb`, `link_namespace`
  midplane veth wiring) are not reproduced for mega; only the
  single-ASIC `redis`/`redis_bmp` dump-restore and
  `waitForAllInstanceDatabaseConfigJsonFilesReady` wait were ported (see
  `_mega_pre_start_block`/`_mega_post_start_block` in
  `patch_templates.py`). Chassis/multi-ASIC topologies are out of scope
  for the current acceptance criteria (single-ASIC default-feature
  builds) and would need a dedicated follow-up.
- **External per-feature container-name references in `sonic-utilities`**
  (e.g. `docker exec teamd ...`, `config feature state`, the `FEATURE`
  table's per-feature service names used by `container_checker` and
  `featured`) are not repointed. `sonic-utilities` is a separate
  submodule (`src/sonic-utilities`) -- this generator only patches files
  inside `sonic-buildimage` itself, per repo convention ("do NOT modify
  files in `src/` directly"). The systemd-unit-level portion of the same
  problem (services in `files/build_templates/*.service(.j2)` that
  reference a folded feature's `.service` name) *is* handled, by
  `patch_templates.py`'s `_fan_in_scan`/`_repoint_service_refs`.
- **`ctl_parser.py`** (~1000 lines) remains unused. It could in principle
  replace some of `patch_templates.py`'s hand-written anchor-based edits
  (e.g. the database pre/postStart block copying used for the
  multi-ASIC fix above) with a structured per-container-name block
  parser, but rewiring `patch_templates.py` onto it is an orthogonal
  refactor with its own regression risk, not required by any of the
  findings' acceptance criteria, so it is left in place rather than
  wired in or removed.
