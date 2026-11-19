# SONiC build-time dashboard

The dashboard publishes existing GitHub Actions measurements at
<https://securely1g.github.io/sonic-buildimage/>. It collects Actions metadata
and selected timing markers from public build logs. It does not launch a build,
generate a package, modify a runner, or clear a cache.

## Measurements

- In **Pull requests**, list only PRs whose latest associated image result is
  successful or failed. Summarize success, failure, cancellation and skipping
  across all collected PRs, counting each PR once. Pending and unmeasured PRs
  are noted separately. These counts do not follow the chart filters.
- Keep every collected run attempt and job outcome. Skipped or incomplete work
  is not a zero-second build; failed builds are not faster successful builds.
- Show the explicit image-build step separately from job setup, checks, uploads,
  and the complete job duration. Parallel job times must not be added together
  to claim elapsed workflow time.
- Keep the Make VS and Bazel installer paths separate. The current Make VS step
  builds the `sonic-vs.bin` ONIE installer and the Orchagent/Syncd runtime and
  debug archives. It excludes VM disk generation and BIOS/UEFI guest boot checks.
  `VS installer in SONiC slave (AMD64)` builds the same installer directly with
  `slave.mk` and retains the matching runtime/debug archive pairs.
  Post-build archive and P4 validation have their own CI step and are excluded
  from the image-build duration.
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

The chart starts as a scatter overview with a legend for the visible measurement
scopes. Select one **Measurement series** to show its trend without connecting
different build paths, architectures or measured steps. Date ranges end at the
published snapshot time and use verified image execution starts, not retry
attempt starts. Carried retry records count once and retain their original
execution date. The default view shows the newest 60 matching points;
choose **Show all** to remove that display limit. These chart-only controls do
not filter PR coverage or run history. Hover or focus a point to inspect its
measured scope, source revision and breakdown availability.

Host-Make installer and paired-archive builds have their own **Host Make .bin +
archives** series and color. Recognized measurement boundaries without an
explicit style use their measurement label and a neutral color rather than
breaking chart rendering. Unrecognized boundaries remain **Boundary unknown**;
the fallback does not infer a measurement scope or change recorded timings.

The PR result comes from recognized image jobs (`make-vs` or `bazel-vs`), not
the surrounding workflow conclusion. A successful workflow whose image job was
skipped contributes only to **Skipped**. The latest associated run containing
an image job supplies the result; a later run without an image job does not
replace that evidence. Same-time reruns prefer the higher attempt. A newer
skipped or cancelled image result removes an older successful result from the
PR table. All attempts remain available in run history.

If a run has multiple image jobs, any failure (including a timeout or startup
failure) makes the PR failed. Otherwise, pending work takes precedence, followed
by an unknown result, cancellation, success, and skipping. Success requires at
least one successful image job and all other image jobs successful or skipped;
**Skipped** requires every image job to be skipped. Missing or unknown image
evidence is **Unmeasured**, never an inferred success or skip. Historical results
retain their source revision and current-head annotation.

The collector covers `bazel-swss-oci.yml`, `bazel-oci.yml`,
`bazel-vs-unprivileged.yml`, and `bazel.yml`. The unprivileged workflow's final
`sonic-vs.bin` assembly job uses the Bazel installer path; its explicit build
step includes installer verification. Native preparation remains a separate
job and does not supply an image result. If all image-build steps pass and a
later check fails or the job is cancelled, the PR row and job details still show
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
Current `.bin` builds expose the measured final recipe and root-filesystem
tasks, but do not emit inner-phase receipts. Their inner phases remain
**not recorded**. An old `.img.gz` receipt cannot populate `.bin` timings.
The host Make step uses `TERM=dumb` to emit the existing build/finish markers
instead of terminal redraws. The Make wrapper already marks its single outer
installer request. Historical direct-slave terminal redraws do not provide task
end times and cannot be reconstructed into a complete breakdown.
Likewise, a Make `cached` marker does not establish a Bazel cache-hit rate.
Repeated target invocations remain separate observations. Log timestamps have
finer precision than Actions step timestamps, so their totals can differ slightly.

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

History is serialized as compact, deterministically ordered JSON; all collected
records and the schema remain unchanged. Existing indented history is read
normally and compacted on the next successful collection. The collector reports
the output byte size and rejects files above 95 MiB before replacing the output,
leaving headroom below GitHub's 100 MiB per-file limit. It does not silently prune
older records to meet this budget. If compact history eventually reaches that
budget, split storage and update the collector and dashboard readers together;
retrying the same oversized output or enabling Git LFS alone does not fix the
static JSON publication path.

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
