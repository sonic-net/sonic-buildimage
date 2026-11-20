# SONiC build-time dashboard

The dashboard publishes existing GitHub Actions measurements at
<https://securely1g.github.io/sonic-buildimage/>. It collects Actions metadata
and selected timing markers from public build logs. It does not launch a build,
generate a package, modify a runner, or clear a cache.

## Measurements

- **Image build status** opens on **Master** with **All master commits**
  selected. The latest master image result appears above the chart.
  The source selector also filters the chart and run history. Select
  **Master + pull requests** to include each PR's latest image result,
  **Pull requests** for PR builds, or
  **Other branches** for runs on branches with no recorded PR association.
  Under **Master**, choose a **Master commit** to see that revision's latest
  image result, successful timings, run history, and details. Each commit
  appears once, newest run first, including revisions with only failed or
  pending runs. The picker shows the short SHA, commit title, and latest run
  date. For example, look for **9bbd301c1212 · Wait for the Framework REST
  binary before checking its certificate** to find that commit's VS installer
  build. Older records without a title show the SHA and date.
  If the selected commit's image job is still queued or running, the empty
  duration chart links to that job so you can check its progress.
  **All master commits** restores the branch history. Architecture and date
  filters apply independently of the status summary.
- Keep every collected run attempt and job outcome. Skipped or incomplete work
  is not a zero-second build; failed builds are not faster successful builds.
- Show the explicit image-build step separately from job setup, checks, uploads,
  and the complete job duration. Parallel job times must not be added together
  to claim elapsed workflow time.
- Keep the Make VS and Bazel installer paths separate. The active
  `VS installer in SONiC slave (AMD64)` job builds `sonic-vs.bin` and the
  Orchagent/Syncd runtime archives by default. A manual run with
  `enable_debug_layer` also builds their debug archives. Compare timings for
  the same archive selection. The image-build duration excludes post-build
  validation, VM disk generation and BIOS/UEFI guest boot checks.
- Record the run's source commit separately from the current PR head. Historical
  evidence does not establish that the latest PR revision passed.
- Resolve each PR's selected base branch independently of historical PR metadata.
  Display that target separately from the run's recorded event base. Neither
  proves which base a job checked out; the dashboard labels attempt bases as
  unverified and does not infer image artifact publication from successful timings.

Trend lines also separate exact measured step names. **Build the VS installer
(.bin)** has its own series. The earlier **Build the VS image** step included
Make's VM generation and boot checks, while historical combined VS/OCI steps
also included follow-up work and archive validation. Changing the image target
or moving validation to its own step is not a same-scope build speedup.
Point tooltips, job details and the latest measurement identify the timing
boundary. Unknown boundaries remain
isolated points, and historical durations are preserved.

The chart starts with one point per measured image execution. Master builds
are green and PR builds are yellow. Point labels and the legend name the source.
For example, a push to `master` after merging PR #253 is a master build. A
`pull_request` workflow remains a PR build even if GitHub omits its PR number.
A workflow dispatched on a feature branch with a recorded PR association also
appears in the PR view. Other branch builds appear in gray in **Other branches**.

Select one **Measurement series** to see trend lines. Each line follows one
build source, path, architecture and measured step. The **Host Make .bin + archives** series
includes both runtime-only and paired installer builds. Recognized steps use
their measurement label. Unrecognized steps appear as isolated **Boundary
unknown** points. Date ranges end at snapshot time and
use verified image execution starts. Carried retries count once at the original
execution time. The chart shows the newest 10 matching measurements by default.
Choose **Newest 60**, **Newest 120**, or **Show all** to see more. Hover or
focus a point to see its measurement and revision. Select it to open the
full-width **Run details**.

The PR result comes from recognized image jobs (`make-vs` or `bazel-vs`), not
the surrounding workflow conclusion. A successful workflow whose image job was
skipped contributes only to **Skipped**. The latest associated run containing
an image job supplies the result; a later run without an image job does not
replace that evidence. Same-time reruns prefer the higher attempt. A newer
skipped or cancelled image result moves that PR into the corresponding
status count. All attempts remain available in run history.

If a run has multiple image jobs, any failure (including a timeout or startup
failure) makes the PR failed. Otherwise, pending work takes precedence, followed
by an unknown result, cancellation, success, and skipping. Success requires at
least one successful image job and all other image jobs successful or skipped;
**Skipped** requires every image job to be skipped. Missing or unknown image
evidence is **Unmeasured**, never an inferred success or skip. Historical results
retain their source revision in Run details.

The collector covers `bazel-swss-oci.yml`, `bazel-oci.yml`,
`bazel-vs-unprivileged.yml`, and `bazel.yml`. The unprivileged workflow's final
`sonic-vs.bin` assembly job uses the Bazel installer path; its explicit build
step includes installer verification. Native preparation remains a separate
job and does not supply an image result. If all image-build steps pass and a
later check fails or the job is cancelled, the job details still show
the completed build time. The job keeps its recorded outcome. Only successful
image jobs enter the performance chart.

The dashboard can report observed durations from earlier builds, but their
cache state, hardware/configuration match, and historical tested base are not
recorded. It therefore makes **no improvement or regression percentage claim**.
Controlled baseline/head benchmarking is a separate change and must preserve
the existing full-image/package execution opt-in.

See [data-contract.md](data-contract.md) for the public JSON format. Only a
bounded set of public metadata fields is retained. Raw logs, command lines,
environment variables, machine names, and build artifacts are not published.

## Expand an image build

Select a run and the **VS installer in SONiC slave (AMD64)** job. Its Bazel
breakdown lists the Make targets that invoked Bazel during installer
construction, ordered by measured command and export time. Expand a target
to inspect lock waits, compiler subprocesses, nested builds, cache fetches
and exports. A Docker row can cover one archive or a runtime/debug pair.
For a multi-image VS build, expand `vs-container-images` to see the selected
containers in order of observed Bazel action time. For example, the Syncd VS
row groups work on its runtime and debug images. Active time counts overlapping
actions for that owner once. OCI archiving, gzip and cache reads can overlap
within that time. Two owners can also be active at the same time. The shared
command is measured once. The group lists time with only shared or unattributed
actions, time with no action recorded, and time with multiple image owners
active. The Export column measures Make's subsequent archive exports outside
the shared Bazel command. Older runs with only recipe receipts show the
recorded exports for each image.

**Total elapsed** includes the wait from an image's first observed Bazel action
through completion of its last runtime or debug archive export. **Compile time**
counts the intervals when a C/C++, Go or Rust compiler ran for that image.
Simultaneous compilers count once. For example, replaying the saved profile and
exports from [master run 44638292829](https://github.com/securely1g/sonic-buildimage/actions/runs/44638292829/job/135965478341)
gives Syncd VS **36m 40s total**, **23m 03s active** and **0s compiling**.
Other images ran during that same window. An em dash means the run lacks the
measurements needed for that value.

The Linux kernel row times the entire pinned launcher and reports its build,
output query and module-graph profiles. When Make rebuilds the P4RT package,
its row covers the nested Bazel build, tests, server shutdown and Debian package
staging. If Make restores a package without invoking Bazel, it only appears in
the Make breakdown.

Partial measurements show how many expected records are present. A measured
zero means Bazel recorded no matching span. Concurrent work can contribute to
more than one category, and cache fetches include both disk and remote reads.

**Why each target took its time** ranks targets by build wall time excluding
measured build-command lock waiting, separately from output-query waits. New
profiles supply numeric evidence for repository preparation and input-read waits,
compiler/nested-build subprocesses, image/tar/gzip processing, APT selection and
identity/source validation on the critical path,
cache fetch/write spans and ARM package-repository preparation. These are
overlapping observations, not an additive attribution or proof of CPU, network,
disk bottlenecks, unnecessary cross-architecture work or achievable savings.
Cache writes do not establish remote publication. The same section is written
to the VS workflow's job summary. Partial build coverage leaves the non-lock
duration unavailable; historical runs are not assigned invented explanations.
The isolated kernel launcher remains explicitly incomparable with an image build.
This section uses full-width, initially collapsed cards with diagnosis badges,
readable durations and **Expand visible / Collapse visible** controls. Each card
includes the eight longest recognized named repository spans (with nested read
coverage), eight longest recognized critical-path actions, and the first eight
command receipts with UTC request times, elapsed/lock time and exit status.
Repeated invocations retain individual spans; these lists are bounded evidence,
not an exhaustive causal partition. Historical runs without these details say so.
The workflow summary uses GitHub-native collapsible sections and the same evidence
tables; GitHub does not support the dashboard's custom styling or JavaScript controls.

Open the upload log linked from the timings to download
`sonic-vs-bazel-timings-amd64` during its 14-day retention period. Open a
Bazel profile in Perfetto to inspect individual actions within a recipe. The
dashboard keeps the numeric breakdown and validated, bounded named evidence in build history. Earlier instrumented
runs keep their four-image reports, and pre-profiling runs keep their original
Make timings.

Open **Step timings**, then expand the Make VS image-build step marked
**Measured breakdown**. Each section covers one sequential Make target, including its
setup, prerequisite checks and cleanup. Sections follow the actual top-level
Make requests: the VS-image request includes its container dependencies before
the final image recipe; later container rows are additional explicit requests.
Expand a section to see measured tasks
inside it. Those tasks may run concurrently: their durations must not be added
to the section total. The task timeline shows that overlap.

For example, [run 39909648210, job 120280929878, step 11](https://github.com/securely1g/sonic-buildimage/actions/runs/39909648210/job/120280929878#step:11:1)
reports 44m 25s for the combined VS/OCI build. Its log divides that time into
about 30m 13s for the VS image and dependencies, 4m 19s for Orchagent release,
4m 25s for Orchagent debug, 4m 35s for Syncd debug, and 53s for the final P4RT
debug target and cleanup. These values are examples, not dashboard constants:
the collector derives each attempt's measurements from its own log.

Expand **Final VS image recipe** to see three measured categories on VM builds
that contain the phase instrumentation:

- **Image assembly:** preparation, remaining filesystem work, ONIE installer
  creation, KVM disk preparation and installation, and finalization.
- **Boot checks:** booting the installed BIOS/UEFI image and running the existing
  installation checks. Installing the image belongs to assembly.
- **Compression:** final filesystem and payload compression, and compression of
  the BIOS/UEFI disk images.

The categories can repeat: for example, BIOS assembly, boot checks and
compression precede the UEFI equivalents. Category totals sum measured spans;
the sequential phase timeline preserves their actual order. These measurements
use a producer-side monotonic clock. They do not rescale to fit the surrounding
GitHub log timestamps or assign unmeasured time to a category. Receipts that
disagree with the containing recipe by more than two seconds are unavailable;
the outer recipe duration remains visible.

The historical example above has only the boundaries of its 18m 00s final
recipe. Its internal phase times were not recorded, so the dashboard shows that
limitation instead of estimating a split. Adding the instrumentation cannot
recover those missing measurements; a subsequent image build supplies them.
On a native host-Make build with both the shared container batch and final
installer markers, **Run details → Image build steps** opens directly above the
separate **Bazel build breakdown**. Each of the five headings expands
independently. For example, the master build at `9bbd301c` took 1h 55m 30s.

| Image build step | Time in that run | What to look for when expanded |
| --- | ---: | --- |
| Prepare kernel, packages and P4RT | 33m 44s | Device data, kernel preparation and P4RT dependency audit, SDK, source build and handoff |
| Build and export Bazel container images | 46m 57s | Shared Bazel command and recorded export times by container |
| Finish Make containers and stage the root filesystem | 14m 32s | Make container builds, cached targets, base image loads and intermediate SquashFS |
| Assemble the final ONIE installer | 15m 20s | Docker image loading, filesystem and store compression, payload ZIP and ONIE wrapping |
| Finish the Make invocation | 4m 57s | Version metadata, builder exit and Trixie and Bookworm cleanup |

The headings use each run's elapsed time. Inner rows require measurements from
that run. For example, the historical log recorded the ONIE recipe's total but
lacked its internal phase markers. Recent instrumented runs can show those phases.
Overlapping tasks count once when calculating uncovered wall time, and nested
P4RT and root filesystem subtasks are included in the parent duration. The
separate Bazel breakdown still reports compiler, cache and lock-wait time and the
per-container activity in the shared command.

The host Make step uses `TERM=dumb` to emit build and finish markers. Historical
direct-slave terminal redraws lack task end times. A Make `cached` marker alone
cannot establish the Bazel cache-hit rate. Repeated target invocations remain
separate observations. Log timestamps have finer precision than Actions step
timestamps, so their totals can differ slightly.

The collector reads at most 20 eligible missing job logs per refresh, newest
first. Each download has a 32 MiB size limit and a 45-second deadline. It retains
only known task identifiers, fixed labels, outcomes and numeric timings; raw
logs, command lines and machine details never enter the published snapshot.
Completed measurements are cached with their exact run, attempt, job and step
identity. Expired or unsupported logs are shown as unavailable. Temporary read
failures can be retried without discarding the ordinary step timings. Older
unsupported or incomplete results are retried once with the current parser
revision, within the same log-read budget; successful measurements are retained.
Step boundaries match both the start and end timestamps, so a preceding short
setup step in the same timestamp second does not make the build ambiguous.
Older snapshots without breakdowns remain supported and gain details incrementally.
Use `--log-jobs 0` to collect only metadata on a particular invocation, or a
value from 1 to 20 to limit its log reads.

## Automatic publication

`.github/workflows/build-dashboard.yml` validates PR changes with read-only
permissions. Once on `master`, it collects on completion of the selected
workflows, every 15 minutes at minutes 7/22/37/52, and on manual dispatch.
GitHub schedules can be delayed; `generated_at` shows the actual refresh time.
The collector checks the complete selected workflow history, so a coalesced
completion event is recovered by the next collection.

Completed attempt records are reused from the dedicated `build-metrics` branch.
New attempts, changed latest attempts, and unfinished jobs are refreshed. Old
records remain when Actions history expires. Collection failures leave the
previous history and published site intact. A normal, non-force Git commit
persists the new JSON before the static site is deployed through the official
Pages artifact/deployment actions. Publications are serialized through the end
of deployment to prevent an older run from replacing a newer site.

The published `data.json` is a small index. It contains the PR list and the
paths and record counts of the files in `history/`. Each of those files holds
complete workflow attempts. For example, a 101 MiB snapshot can now be published
as an index and roughly thirteen files instead of crossing GitHub's per-file
limit. The collector targets 8 MiB per file and keeps a large individual attempt
together. Every published file must fit within 95 MiB.

The first refresh reads the existing single-file snapshot. Later refreshes
read the index and verify each file's SHA-256 and record count before updating
the history. There is no age or count limit on retained attempts. Older snapshots
remain accessible in the Git history of `build-metrics`.

The publishing job always uses trusted `master` code. A `workflow_run` event
never supplies executable PR code, artifacts, scripts, or cached executables.
The GitHub token stays in the workflow and is not included in the static site.

## Local collection and preview

```sh
python3 -B -m unittest discover -s tools/ci/build_dashboard -p 'test_*.py' -v
node --check tools/ci/build_dashboard/web/app.js
mkdir -p /tmp/sonic-build-dashboard
python3 tools/ci/build_dashboard/collect.py \
  --repo securely1g/sonic-buildimage \
  --github-cli /home/dev-user/.local/bin/gh-securely1g \
  --history /tmp/sonic-build-dashboard/data.json \
  --output /tmp/sonic-build-dashboard/data.json
cp tools/ci/build_dashboard/web/* /tmp/sonic-build-dashboard/
python3 -m http.server 8000 --bind 127.0.0.1 --directory /tmp/sonic-build-dashboard
```

Use `--backfill` to re-read available completed attempts. It does not discard
older retained records. A missing history file starts an initial collection;
invalid existing history must be corrected rather than silently reset.

## Pages setup

Enable GitHub Pages with **GitHub Actions** as its build source. Restrict the
`github-pages` deployment environment to `master`; leave existing branch
protection and unrelated environment settings intact. The workflow needs
`actions: read`, `pull-requests: read`, and `contents: write` for the history
commit, followed by `pages: write` and `id-token: write` for deployment.

An initial site may be published from an operator-pushed `gh-pages` branch
before the automation PR merges. After that first publication, change the Pages
build source to GitHub Actions. The existing site stays available; automatic
event/scheduled updates begin when this workflow reaches the default branch.
Pushing `gh-pages` with a workflow's `GITHUB_TOKEN` does not trigger a legacy
Pages build, so automatic publication uses the Pages deployment action directly.
