# Build dashboard data (schema 1)

`collect.py` produces one JSON document, also used as its durable history. It
reads public GitHub Actions metadata and bounded timing markers from selected
public job logs. Raw logs, artifacts, machine names, workflow inputs, environment
variables and credentials are never retained in the public document. Collection
does not execute a build or execute any content from a log.

- `schema_version`: `1`.
- `repository`: validated `owner/repository`.
- `generated_at`: UTC collection timestamp.
- `pull_requests`: current metadata, including closed and unmeasured PRs:
  `number`, `title`, `state`, `draft`, `url`, `head_sha`, `base_sha`,
  `head_branch`, `updated_at`, `merged_at`, `base_branch`, `base_target_sha`,
  `base_target_observed_at`. `base_sha` is GitHub's PR metadata and may be
  historical even for an open PR. `base_target_sha` is independently read from
  the selected branch ref, or `null` when that ref no longer exists. Other ref
  lookup failures abort collection. These fields never establish the base/head
  that an old workflow tested.
- `runs`: one entry per `(run_id, attempt)`, retained after Actions history expires:
  `run_id`, `attempt`, `workflow_name`, `workflow_path`, `source_sha`, `branch`,
  `event`, `status`, `conclusion`, `pr_numbers`, `pr_association`, `pr_association_note`, `started_at`, `started_at_source`,
  `completed_at`, `updated_at`, `url`, `jobs`, `event_bases`.
  `source_sha` is GitHub's run `head_sha`; it is not replaced by the current PR
  head. `pr_association` is `github` (run's explicit PR links), `commit` (exact
  current same-repository PR head, or a unique exact fork repository/branch/head
  match for a `pull_request` event), `branch` (unique same-repository feature
  branch, additionally verified through GitHub's commit-to-PR endpoint), or
  `none`. Multiple/ambiguous inferred matches are left unassociated.
  `event_bases` contains only `{number, ref, sha}` from the workflow run's
  recorded PR metadata. It is never filled from current PR base targets. These
  event records do not independently verify job checkout bases; absent older
  records remain unknown. A normal refresh adds event metadata to the latest
  retained attempt without refetching completed jobs. Successful image timings
  do not establish that an image artifact was published.
  `pr_association_note` is normally `null`; it discloses HTTP 404/422 when an old
  commit cannot be resolved for optional PR inference. The run stays visible.
  Other optional commit-lookup failures are disclosed as temporarily unavailable
  and retried next refresh without refetching cached jobs. Main repository, PR,
  workflow, run-attempt, and job API failures still abort collection.
  `started_at_source` is `run_started_at` when GitHub supplied a valid attempt
  start, `created_at` when the recorded time only falls back to run creation,
  or `null` when unavailable. Older history may omit it. A cached latest attempt
  gains verified start provenance only from matching fresh run ID, attempt,
  source SHA and workflow metadata with a valid `run_started_at`; retained jobs
  are not refetched for that upgrade. Legacy historical retry times stay
  unverified. The run view labels fallback and legacy times accordingly.
- Each job: `job_id`, `name`, `kind`, `architecture`, `status`, `conclusion`,
  `started_at`, `completed_at`, `duration_seconds`, `image_build_seconds`,
  `image_build_elapsed_seconds`, `image_build_step_numbers`, `phases`, `steps`, `url`.
  `kind`: `make-vs`, `bazel-vs`, `container`, `source`, `checks`, `other`.
  `architecture`: `amd64`, `arm64`, or `null`; the classifier only uses explicit
  job-name annotations, never the physical runner's identity.
  `phases`: keys `setup`, `build`, `validation`, `upload`, `other`, with summed
  completed-step seconds or `null` where there is no measured step. These are
  not guaranteed to partition job wall time (runner overhead is separate).
  `image_build_seconds` is populated only for successful, explicitly recognized
  image-build steps; `image_build_elapsed_seconds` also records a failed image
  build's elapsed time. The PR table and job details keep a completed build's timing
  when later validation fails. The performance chart requires a successful job.
  Exact recognized step names identify the measurement scope. In particular,
  `Build the VS installer (.bin)` excludes VM generation and boot checks;
  historical `Build the VS image` measurements include them. Consumers must
  keep those trend series separate even when job kind and architecture match.

  `image_build_step_numbers` identifies the exact recognized steps contributing
  to those image durations. It is empty when no image step is recognized and
  `null` when step numbers are missing or duplicated. Older schema-1 history
  without this field remains readable; the next collection recomputes it from
  retained steps. A display without verified step identities uses an explicitly
  labeled job execution time instead of inventing an image step timestamp.
- A Make VS installer job with a profiling summary step may include
  `image_recipe_timings`. Version 2 stores a `recipes` list. Each `owner`
  is the Make output basename or package build group. New owners appear as
  the build registers them. Repeated invocations for an owner contribute to
  that owner's row.
  - `bazel_seconds`, `bazel_phases` and `bazel_expected` report the measured
    duration, recorded timing count and expected timing count for its commands.
    The kernel measures its launcher once per invocation.
  - `lock_seconds`, `compile_seconds`, `foreign_seconds` and `fetch_seconds`
    cover spans in `profiles` of `profiles_expected` Bazel traces. A nested
    build includes a whole foreign build or the kernel package action. Within
    each category concurrent spans count once per trace. A cache fetch
    includes input staging and output materialization from disk or remote storage.
  - `export_seconds`, `exports` and `exports_expected` cover archive or package
    exports. `failed` records whether a command returned a nonzero exit status.
  Durations are null when their source count is zero. Counts below the
  expectation show partial coverage. A valid empty list records zero observed
  recipes. An unavailable result has a fixed `reason` and an empty list.
  The collector reads one bounded record per recipe and verifies the final
  count from the same job's summary step before persisting the result.
  Version 1 history keeps `images` for FRR, mux, TeamD and Syncd VS, with
  three command timings, three profiles and two exports expected per pair.
  Jobs predating the profiling step omit this field.
- Each step: `number`, `name`, `status`, `conclusion`, `started_at`,
  `completed_at`, `duration_seconds`, `phase`.
  An optional `breakdown` provides nested Make build timings without changing
  the existing step duration or successful-image measurement:
  - `version`: `1`; `source`: `github-job-log`.
  - `status`: `available` or `unavailable`; `reason`: a fixed reason or `null`.
  - `duration_seconds`: observed log interval, or `null` when unavailable.
  - `sections`: sequential outer Make invocations. Each contains a known `id`,
    fixed `label`, `offset_seconds` relative to the observed build-step start,
    `duration_seconds`, and `tasks`.
  - Each task contains a known `id`, fixed `label`, `offset_seconds` relative to
    its containing section, `duration_seconds`, and `outcome` (`finished` or
    `cached`). Tasks can overlap and are not an additive partition of the
    section. Unobserved work inside a recipe is not synthesized.
  - A final-image task may additionally contain `phase_breakdown`:
    - `version`: `1`; `source`: `build-phase-markers`.
    - `status`: `available` or `unavailable`; `reason`: `null`, `not-recorded`,
      `invalid-timing`, or `incomplete-timing`.
    - `duration_seconds`: the measured producer interval, or `null` when
      unavailable.
    - `segments`: at most 64 sequential, contiguous intervals, each with
      `phase` (`assembly`, `boot-checks`, or `compression`), `context` (`recipe`,
      `filesystem`, `installer`, `bios`, `uefi`, `4-asic`, or `6-asic`),
      `offset_seconds` relative to the producer interval, and `duration_seconds`.
      Labels are derived from these fixed identifiers. Segments partition the
      producer interval; category totals aggregate repeated segments without
      changing their timeline order.
    The producer interval uses a monotonic clock. The containing task uses
    GitHub log timestamps, so their durations can differ slightly; validation
    permits an absolute difference of at most two seconds from the containing
    interval and the receipt's elapsed log interval. It never rescales
    measurements to force equality. A shorter producer interval does
    not assign the remaining time to a phase. Missing historical measurements
    and cache reuse do not supply zero-duration phase measurements.
    Phase receipts are accepted only inside the matching active final-image
    recipe, before its successful finish marker. Invalid or incomplete receipts
    make that inner detail unavailable while preserving valid outer timings.
    The `.bin` installer and `.img.gz` VM target identities are distinct when
    associating receipts. Both support outer recipe and root-filesystem task
    timing; current `.bin` builds do not emit inner-phase receipts.
    Previously retained tasks without this optional field remain supported.
  Timing values must be finite and nonnegative; child intervals must fit their
  parent. Identifiers and labels come from the parser's allowlist, never
  arbitrary log text. Unavailable detail is not a zero-duration build. The
  optional field is compatible with older schema-1 snapshots.
- `comparisons`: empty in this phase. API timing alone does not prove matched
  cache state, toolchain/configuration, hardware, or a historical tested base.
  Therefore no improvement or regression percentage is asserted.

Durations for skipped, incomplete, missing, malformed, placeholder, or reversed
start/end times are `null`, never a fabricated zero. Valid measured zero-second
steps are preserved. Concluded failed/cancelled jobs may have elapsed time;
consumers must use the outcome before plotting successful performance.

Retry attempt start is separate from image execution. GitHub can assign a new
job ID to a prior successful execution when retrying failed jobs. The display
names an earlier origin only when one retained, completed prior attempt of the
same run, source SHA and workflow has matching job name, kind, architecture,
outcome, execution timestamps and every step's number, name, outcome and
timestamps. The original job must fit that prior attempt, and the carried job
must have completed before the retry started. Missing or ambiguous evidence
leaves the origin unverified.
Every retry used as the current or original attempt requires observed
`run_started_at` provenance; a `created_at` fallback cannot establish a new
execution. A job in attempt 1 can establish its own origin from its job and step
evidence because there is no earlier attempt in that run. Duplicate run or job
identities remain in raw history and are unverified. Colliding metric keys are
deduplicated only when the execution signatures match.

PR coverage and run details label image step time, job execution time and retry
attempt start separately. Verified carried records link to the original job.
Performance counts and trends count each verified origin once and use the image
step's execution start, or a labeled job start when step identities are absent.
A successful job that completed before its retry began is never counted as a new
execution when its origin is unverified; it remains visible in raw run history
with that limitation. These timestamps do not establish artifact publication.

Only `bazel-swss-oci.yml`, `bazel-oci.yml`, `bazel-vs-unprivileged.yml`, and
`bazel.yml` are collected. All
pages and run attempts are covered. Completed attempts are reused from history;
new, nonterminal, and changed latest attempts fetch current jobs. `--backfill`
forces a fresh read of every available attempt, including completed history. API failures
fail collection before replacing the output. Historical entries absent from
GitHub's response remain in history. Every URL is constructed on `github.com`
from the validated repository and integer identifiers.
