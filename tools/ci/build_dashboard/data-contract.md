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
  `head_branch`, `updated_at`, `merged_at`. These SHAs describe the **current PR
  metadata**, never the base/head that an old workflow tested.
- `runs`: one entry per `(run_id, attempt)`, retained after Actions history expires:
  `run_id`, `attempt`, `workflow_name`, `workflow_path`, `source_sha`, `branch`,
  `event`, `status`, `conclusion`, `pr_numbers`, `pr_association`, `pr_association_note`, `started_at`,
  `completed_at`, `updated_at`, `url`, `jobs`.
  `source_sha` is GitHub's run `head_sha`; it is not replaced by the current PR
  head. `pr_association` is `github` (run's explicit PR links), `commit` (exact
  current same-repository PR head), `branch` (unique same-repository feature
  branch, additionally verified through GitHub's commit-to-PR endpoint), or
  `none`. Multiple/ambiguous inferred matches are left unassociated.
  `pr_association_note` is normally `null`; it discloses HTTP 404/422 when an old
  commit cannot be resolved for optional PR inference. The run stays visible.
  Other optional commit-lookup failures are disclosed as temporarily unavailable
  and retried next refresh without refetching cached jobs. Main repository, PR,
  workflow, run-attempt, and job API failures still abort collection.
- Each job: `job_id`, `name`, `kind`, `architecture`, `status`, `conclusion`,
  `started_at`, `completed_at`, `duration_seconds`, `image_build_seconds`,
  `image_build_elapsed_seconds`, `phases`, `steps`, `url`.
  `kind`: `make-vs`, `bazel-vs`, `container`, `source`, `checks`, `other`.
  `architecture`: `amd64`, `arm64`, or `null`; the classifier only uses explicit
  job-name annotations, never the physical runner's identity.
  `phases`: keys `setup`, `build`, `validation`, `upload`, `other`, with summed
  completed-step seconds or `null` where there is no measured step. These are
  not guaranteed to partition job wall time (runner overhead is separate).
  `image_build_seconds` is populated only for successful, explicitly recognized
  image-build steps; `image_build_elapsed_seconds` also records a failed image
  build's elapsed time. A failed job is never a successful benchmark.
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

Only `bazel-swss-oci.yml`, `bazel-oci.yml`, and `bazel.yml` are collected. All
pages and run attempts are covered. Completed attempts are reused from history;
new, nonterminal, and changed latest attempts fetch current jobs. `--backfill`
forces a fresh read of every available attempt, including completed history. API failures
fail collection before replacing the output. Historical entries absent from
GitHub's response remain in history. Every URL is constructed on `github.com`
from the validated repository and integer identifiers.
