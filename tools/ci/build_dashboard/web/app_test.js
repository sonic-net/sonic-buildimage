"use strict";

const assert = require("node:assert/strict");
const {readFileSync} = require("node:fs");
const test = require("node:test");
const vm = require("node:vm");
const {duration, measured, completedImageSeconds, imageMeasurement, imageTrendGroups, imageSeries, selectTrendPoints, chartScale, imageBreakdownLabel, attemptTimeLabel, imageExecution, executionTimeLabel, executionOriginLabel, jobExecutionLabel, imageMeasurements, imageResult, prCoverage, summarizePrCoverage, safeGithubUrl, validateSnapshot, normalizeBreakdown, normalizePhaseBreakdown, renderStep, renderRecipeTimings, recipeRows, renderBuildExplanations, baseSummary} = require("./app.js");
const pr72Fixture = require("../testdata/pr72-carried-image-execution.json");

test("current selected base and recorded event base never imply tested-base validation", () => {
  const pr = {number: 13, base_branch: "stack/base", base_target_sha: "a".repeat(40), base_sha: "b".repeat(40)};
  const summary = baseSummary(pr, {event_bases: [{number: 13, ref: "old/base", sha: "c".repeat(40)}]});
  assert.equal(summary.selected, "Selected base: stack/base@aaaaaaaaaaaa");
  assert.match(summary.attempt, /old\/base@cccccccccccc/);
  assert.match(summary.attempt, /checkout base is unverified/);
  assert.match(baseSummary({...pr, base_target_sha: null}, {}).selected, /unavailable/);
  assert.match(baseSummary(pr, {}).attempt, /no recorded event base/);
  assert.equal(baseSummary({number: 13, base_sha: "b".repeat(40)}, {}).selected, "Selected base: unavailable");
});

test("PR status uses the latest associated image attempt and counts PRs without one", () => {
  const image = {kind: "make-vs", conclusion: "failure", image_build_seconds: null};
  const run = (number, started_at, jobs = [image], completed_at = null) => ({pr_numbers: [number], started_at, completed_at, jobs});
  const data = {
    pull_requests: [1, 2, 3, 4, 5, 6, 7].map(number => ({number})),
    runs: [
      run(1, "2026-10-21T02:00:00Z"),
      run(2, "2026-10-21T01:00:00Z"),
      run(2, "2026-10-21T03:00:00Z", [{kind: "source"}]),
      run(3, "2026-10-21T02:00:00Z"),
      run(4, null, [image], "2026-10-21T04:00:00Z"),
      run(5, "2026-10-21T05:00:00Z", [{kind: "checks"}]),
      run(6, null),
    ],
  };
  const rows = prCoverage(data);
  assert.equal(summarizePrCoverage(rows).unmeasured, 2);
  assert.equal(rows.find(row => row.pr.number === 2).latestImageRun.started_at, "2026-10-21T01:00:00Z");
  assert.equal(rows.find(row => row.pr.number === 5).latestImageRun, null);
  assert.deepEqual(data.pull_requests.map(pr => pr.number), [1, 2, 3, 4, 5, 6, 7]);
});

test("unmeasured and invalid durations never look like fast builds", () => {
  for (const value of [null, undefined, "12", -1, NaN, Infinity]) assert.equal(duration(value), "—");
  assert.equal(duration(0), "0s");
  assert.equal(duration(125), "2m 5s");
  assert.equal(duration(3660), "1h 1m");
});

test("a later validation failure preserves the completed build time in the PR details", () => {
  const job = {kind: "make-vs", conclusion: "failure", image_build_seconds: 10259};
  assert.equal(completedImageSeconds(job), 10259);
  assert.equal(measured(job), false);
  assert.equal(completedImageSeconds({...job, image_build_seconds: null}), null);
  assert.equal(completedImageSeconds({...job, kind: "source"}), null);
});

test("only successful recognized image jobs enter the performance chart", () => {
  const job = {kind: "make-vs", conclusion: "success", image_build_seconds: 120};
  assert.equal(measured(job), true);
  assert.equal(measured({...job, kind: "bazel-vs"}), true);
  for (const conclusion of ["failure", "skipped", "cancelled", null]) assert.equal(measured({...job, conclusion}), false);
  for (const kind of ["container", "source", "checks", "other"]) assert.equal(measured({...job, kind}), false);
  assert.equal(measured({...job, image_build_seconds: null}), false);
});

function timingPoint(name, extra = {}) {
  return {job: {kind: "make-vs", architecture: "amd64", conclusion: "success",
    image_build_seconds: 120, steps: [{name}], ...extra}};
}

test("moving validation out of the build step starts a separate trend series", () => {
  const old = timingPoint("Build the VS image and both OCI container variants");
  const current = timingPoint("Build the VS image");
  const next = timingPoint("Build the VS image", {image_build_seconds: 110});
  const points = [old, current, next], before = structuredClone(points);
  assert.deepEqual(imageTrendGroups(points), [[old], [current, next]]);
  assert.notEqual(imageMeasurement(old.job).signature, imageMeasurement(current.job).signature);
  assert.match(imageMeasurement(old.job).label, /follow-up and archive validation/);
  assert.match(imageMeasurement(current.job).label, /embedded boot checks; post-build validation excluded/);
  assert.deepEqual(points, before, "grouping must preserve historical durations and metadata");
});

test("ONIE installer timings never join VM-image or combined validation timings", () => {
  const combined = timingPoint("Build the VS image and both OCI container variants");
  const vm = timingPoint("Build the VS image");
  const installer = timingPoint("Build the VS installer (.bin)");
  const next = timingPoint("Build the VS installer (.bin)", {image_build_seconds: 100});
  const points = [combined, vm, installer, next], before = structuredClone(points);
  assert.deepEqual(imageTrendGroups(points), [[combined], [vm], [installer, next]]);
  assert.match(imageMeasurement(installer.job).label, /installer \(\.bin\); VM generation and boot checks excluded/);
  assert.match(imageMeasurement(vm.job).label, /embedded boot checks/);
  assert.deepEqual(points, before, "target changes must preserve historical measurements");
});

test("trend grouping retains architecture and build-path boundaries", () => {
  const amd64 = timingPoint("Build the VS image");
  const arm64 = timingPoint("Build the VS image", {architecture: "arm64"});
  const otherPath = timingPoint("Build the VS image", {kind: "bazel-vs"});
  const repeated = timingPoint("Build the VS image");
  assert.deepEqual(imageTrendGroups([amd64, arm64, otherPath, repeated]),
    [[amd64, repeated], [arm64], [otherPath]]);
});

test("missing and unknown measurement identities remain isolated", () => {
  const points = [timingPoint("Renamed image build"), timingPoint("Renamed image build"),
    timingPoint("<script>"), timingPoint("__proto__"), timingPoint(null, {steps: []}),
    timingPoint(null, {steps: undefined})];
  assert.deepEqual(imageTrendGroups(points), points.map(point => [point]));
  for (const {job} of points) {
    assert.deepEqual(imageMeasurement(job), {signature: null, label: "Image step boundary not recorded"});
  }
});

test("measurement signatures include every counted image step but exclude separate checks", () => {
  const point = timingPoint("Build the VS image");
  const validated = timingPoint("Build the VS image", {steps: [
    {name: "Build the VS image"}, {name: "Validate VS container pairs and P4 debug symbols"},
  ]});
  assert.deepEqual(imageMeasurement(point.job), imageMeasurement(validated.job));
  const combined = timingPoint(null, {steps: [
    {name: "Build the VS image"}, {name: "Build the VS image and both OCI container variants"},
  ]});
  assert.notEqual(imageMeasurement(point.job).signature, imageMeasurement(combined.job).signature);
  assert.equal(imageMeasurement(combined.job).signature,
    imageMeasurement({...combined.job, steps: [...combined.job.steps].reverse()}).signature);
  assert.deepEqual(imageTrendGroups([point, combined, validated]), [[point, validated], [combined]]);
});

function pr72CarriedSnapshot() {
  const fixture = structuredClone(pr72Fixture);
  const makeRun = value => {
    const {job_id, observed_run_started_at, created_at, ...attempt} = value;
    assert.equal(observed_run_started_at, attempt.started_at);
    const job = {...structuredClone(fixture.job), job_id, image_build_step_numbers: fixture.image_build_step_numbers,
      url: `https://github.com/${fixture.repository}/actions/runs/${fixture.run_id}/job/${job_id}`};
    return {...attempt, started_at_source: "run_started_at", run_id: fixture.run_id, workflow_path: fixture.workflow_path, source_sha: fixture.source_sha,
      pr_numbers: [72], jobs: [job], url: `https://github.com/${fixture.repository}/actions/runs/${fixture.run_id}/attempts/${attempt.attempt}`};
  };
  return {schema_version: 1, repository: fixture.repository, pull_requests: [fixture.pr],
    runs: [makeRun(fixture.attempt1), makeRun(fixture.attempt2)]};
}

test("PR72 retry keeps the original image execution and counts it once", () => {
  const snapshot = pr72CarriedSnapshot(), before = structuredClone(snapshot);
  const retry = snapshot.runs[1], job = retry.jobs[0], execution = imageExecution(snapshot, retry, job);
  assert.equal(execution.originStatus, "carried");
  assert.equal(execution.originAttempt, 1);
  assert.equal(execution.originJobId, 123981002106);
  assert.equal(execution.originUrl, "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/job/123981002106");
  assert.equal(execution.timeKind, "image_step");
  assert.equal(execution.startedAt, "2026-10-30T11:55:36.000Z");
  assert.equal(execution.completedAt, "2026-10-30T12:34:25.000Z");
  assert.equal(execution.attemptStartedAt, "2026-10-30T13:36:46Z");
  assert.equal(execution.attemptStartVerified, true);
  assert.match(attemptTimeLabel(retry), /^Attempt 2 started /);
  assert.match(executionTimeLabel(execution), /^Image step executed /);
  assert.equal(executionOriginLabel(execution), "Carried from attempt 1");
  assert.match(jobExecutionLabel(job), /^Job executed /);
  const measurements = imageMeasurements(snapshot);
  assert.equal(measurements.records.length, 1);
  assert.equal(measurements.unverified.length, 0);
  assert.equal(measurements.records[0].execution.startedAt, "2026-10-30T11:55:36.000Z");
  assert.equal(measurements.records[0].run.attempt, 2);
  assert.equal(prCoverage(snapshot)[0].classification, "success", "the retained image job succeeded");
  assert.equal(prCoverage(snapshot)[0].latestImageRun.status, "in_progress", "the retry itself remains pending");
  assert.deepEqual(snapshot, before, "display analysis must preserve raw attempts and jobs");
});

test("legacy PR72 records without image step identities stay readable", () => {
  const snapshot = pr72CarriedSnapshot();
  for (const run of snapshot.runs) delete run.jobs[0].image_build_step_numbers;
  assert.equal(validateSnapshot(snapshot), snapshot);
  const execution = imageExecution(snapshot, snapshot.runs[1], snapshot.runs[1].jobs[0]);
  assert.equal(execution.originStatus, "carried");
  assert.equal(execution.timeKind, "image_job");
  assert.equal(execution.startedAt, "2026-10-30T11:50:35.000Z");
  assert.equal(execution.completedAt, "2026-10-30T13:27:57.000Z");
  assert.match(executionTimeLabel(execution), /^Image job executed /);
  assert.equal(imageMeasurements(snapshot).records.length, 1);
});

test("PR72 created-at fallback and legacy retry times never prove a new execution", () => {
  for (const source of [undefined, "created_at"]) {
    const snapshot = pr72CarriedSnapshot(), retry = snapshot.runs[1];
    if (source === undefined) { delete retry.started_at_source; delete snapshot.runs[0].started_at_source; }
    else { retry.started_at_source = source; retry.started_at = pr72Fixture.attempt2.created_at; }
    assert.equal(validateSnapshot(snapshot), snapshot);
    const execution = imageExecution(snapshot, retry, retry.jobs[0]);
    assert.equal(execution.originStatus, "unverified");
    assert.equal(execution.attemptStartedAt, null);
    assert.equal(execution.attemptStartVerified, false);
    assert.equal(imageMeasurements(snapshot).records.length, 1);
    assert.equal(imageMeasurements(snapshot).unverified.length, 1);
    assert.match(attemptTimeLabel(retry), source === undefined ? /attempt 2 start unverified$/ : /attempt 2 start not recorded$/);
    assert.equal(snapshot.runs.length, 2);
  }
});

test("missing or ambiguous carried origins never become new image executions", () => {
  const missing = pr72CarriedSnapshot();
  missing.runs.shift();
  let execution = imageExecution(missing, missing.runs[0], missing.runs[0].jobs[0]);
  assert.equal(execution.originStatus, "unverified");
  assert.equal(execution.originAttempt, null);
  assert.equal(executionOriginLabel(execution), "Execution origin unverified");
  assert.equal(imageMeasurements(missing).records.length, 0);
  assert.equal(imageMeasurements(missing).unverified.length, 1);

  const changed = pr72CarriedSnapshot();
  changed.runs[1].jobs[0].steps[0].completed_at = "2026-10-30T11:51:01Z";
  execution = imageExecution(changed, changed.runs[1], changed.runs[1].jobs[0]);
  assert.equal(execution.originStatus, "unverified");
  assert.equal(imageMeasurements(changed).records.length, 1);
  assert.equal(imageMeasurements(changed).unverified.length, 1);

  const ambiguous = pr72CarriedSnapshot();
  const candidate = structuredClone(ambiguous.runs[0]);
  candidate.attempt = 2; candidate.started_at = "2026-10-30T09:05:43Z"; candidate.jobs[0].job_id = 999999;
  candidate.url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/attempts/2";
  candidate.jobs[0].url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/job/999999";
  ambiguous.runs[1].attempt = 3;
  ambiguous.runs[1].url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/attempts/3";
  ambiguous.runs.splice(1, 0, candidate);
  execution = imageExecution(ambiguous, ambiguous.runs[2], ambiguous.runs[2].jobs[0]);
  assert.equal(execution.originStatus, "unverified");
  assert.equal(execution.originJobId, null);

  const incomplete = pr72CarriedSnapshot();
  incomplete.runs[1].jobs[0].started_at = null;
  execution = imageExecution(incomplete, incomplete.runs[1], incomplete.runs[1].jobs[0]);
  assert.equal(execution.originStatus, "unverified");
  assert.equal(imageMeasurements(incomplete).unverified.length, 1);

  const missingStep = pr72CarriedSnapshot();
  missingStep.runs[1].jobs[0].steps[0].started_at = null;
  execution = imageExecution(missingStep, missingStep.runs[1], missingStep.runs[1].jobs[0]);
  assert.equal(execution.originStatus, "unverified");
  assert.equal(imageMeasurements(missingStep).unverified.length, 1);
});

test("a genuinely new image execution remains a separate measurement", () => {
  const snapshot = pr72CarriedSnapshot(), retry = snapshot.runs[1], job = retry.jobs[0];
  const shift = value => value === null ? null : new Date(Date.parse(value) + 7765000).toISOString();
  job.started_at = shift(job.started_at); job.completed_at = shift(job.completed_at);
  for (const step of job.steps) { step.started_at = shift(step.started_at); step.completed_at = shift(step.completed_at); }
  retry.status = "completed"; retry.conclusion = "success"; retry.completed_at = job.completed_at;
  const execution = imageExecution(snapshot, retry, job);
  assert.equal(execution.originStatus, "current_attempt");
  assert.equal(execution.originAttempt, 2);
  assert.equal(imageMeasurements(snapshot).records.length, 2);
  const carried = structuredClone(retry);
  carried.attempt = 3; carried.started_at = "2026-10-31T00:05:00Z"; carried.status = "in_progress"; carried.conclusion = null; carried.completed_at = null;
  carried.url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/attempts/3";
  carried.jobs[0].job_id = 999998; carried.jobs[0].url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/job/999998";
  snapshot.runs.push(carried);
  const carriedExecution = imageExecution(snapshot, carried, carried.jobs[0]);
  assert.equal(carriedExecution.originStatus, "carried");
  assert.equal(carriedExecution.originAttempt, 2);
  assert.equal(imageMeasurements(snapshot).records.length, 2);
});

test("three-attempt PR72 carry chain stays on the original execution day", () => {
  const snapshot = pr72CarriedSnapshot();
  snapshot.runs[1].status = "completed"; snapshot.runs[1].conclusion = "success"; snapshot.runs[1].completed_at = "2026-10-30T13:55:10Z";
  const third = structuredClone(snapshot.runs[1]);
  third.attempt = 3; third.started_at = "2026-10-31T00:05:00Z"; third.jobs[0].job_id = 999997;
  third.status = "in_progress"; third.conclusion = null; third.completed_at = null;
  third.url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/attempts/3";
  third.jobs[0].url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/job/999997";
  snapshot.runs.push(third);
  const execution = imageExecution(snapshot, third, third.jobs[0]);
  assert.equal(execution.originStatus, "carried");
  assert.equal(execution.originAttempt, 1);
  assert.equal(execution.originJobId, 123981002106);
  const measurements = imageMeasurements(snapshot);
  assert.equal(measurements.records.length, 1);
  assert.equal(measurements.records[0].run.attempt, 3);
  assert.equal(measurements.records[0].execution.startedAt, "2026-10-30T11:55:36.000Z");
  assert.equal(new Date(measurements.records[0].execution.startedAt).getUTCDate(), 30);
  assert.equal(new Date(third.started_at).getUTCDate(), 31);
  assert.equal(snapshot.runs.length, 3);
});

test("a legacy prior retry cannot become a false original when attempt 1 is missing", () => {
  const snapshot = pr72CarriedSnapshot(); snapshot.runs.shift();
  const prior = snapshot.runs[0]; prior.started_at = pr72Fixture.attempt2.created_at; prior.started_at_source = "created_at";
  prior.status = "completed"; prior.conclusion = "success"; prior.completed_at = prior.jobs[0].completed_at;
  const third = structuredClone(prior); third.attempt = 3; third.started_at = "2026-10-31T00:05:00Z"; third.started_at_source = "run_started_at";
  third.status = "in_progress"; third.conclusion = null; third.completed_at = null; third.jobs[0].job_id = 999996;
  third.url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/attempts/3";
  third.jobs[0].url = "https://github.com/securely1g/sonic-buildimage/actions/runs/41030906724/job/999996"; snapshot.runs.push(third);
  const execution = imageExecution(snapshot, third, third.jobs[0]);
  assert.equal(execution.originStatus, "unverified");
  assert.equal(execution.originAttempt, null);
  assert.equal(imageMeasurements(snapshot).records.length, 0);
  assert.equal(imageMeasurements(snapshot).unverified.length, 2);
});

test("duplicate legacy job identities keep different executions unverified", () => {
  const snapshot = pr72CarriedSnapshot(); snapshot.runs.pop();
  const run = snapshot.runs[0], duplicate = structuredClone(run.jobs[0]);
  const shift = value => value === null ? null : new Date(Date.parse(value) + 60000).toISOString();
  duplicate.started_at = shift(duplicate.started_at); duplicate.completed_at = shift(duplicate.completed_at);
  for (const step of duplicate.steps) { step.started_at = shift(step.started_at); step.completed_at = shift(step.completed_at); }
  run.completed_at = duplicate.completed_at; run.jobs.push(duplicate);
  assert.equal(validateSnapshot(snapshot), snapshot);
  assert.equal(imageExecution(snapshot, run, run.jobs[0]).originStatus, "unverified");
  assert.equal(imageExecution(snapshot, run, run.jobs[1]).originStatus, "unverified");
  assert.equal(imageMeasurements(snapshot).records.length, 0);
  assert.equal(imageMeasurements(snapshot).unverified.length, 2);
  assert.equal(run.jobs.length, 2);
});

test("direct-slave, outer-Make and legacy image scopes have distinct stable series", () => {
  const points = ["Build sonic-vs.bin directly with slave.mk", "Build the VS installer (.bin)",
    "Build the VS image", "Build the VS image and both OCI container variants"].map(name => timingPoint(name));
  const before = structuredClone(points);
  assert.equal(new Set(points.map(point => imageSeries(point.job).key)).size, 4);
  assert.match(imageMeasurement(points[0].job).label, /direct slave.mk; VM generation and boot checks excluded/);
  assert.deepEqual(imageTrendGroups([points[0], timingPoint("Build sonic-vs.bin directly with slave.mk"), points[1]]),
    [[points[0], timingPoint("Build sonic-vs.bin directly with slave.mk")], [points[1]]]);
  assert.notEqual(imageSeries(points[0].job).key, imageSeries({...points[0].job, architecture: "arm64"}).key);
  assert.notEqual(imageSeries(points[0].job).key, imageSeries({...points[0].job, kind: "bazel-vs"}).key);
  assert.deepEqual(points, before, "series metadata must not change recorded timings");
});

test("every recognized measurement boundary has a distinct series key and label", () => {
  const context = vm.createContext({module: {exports: {}}, URL});
  vm.runInContext(readFileSync(require.resolve("./app.js"), "utf8"), context);
  const names = Array.from(vm.runInContext("Object.keys(IMAGE_STEP_LABELS)", context));
  const series = names.map(name => imageSeries(timingPoint(name).job));
  assert.ok(names.includes("Build sonic-vs.bin through the host Make wrapper"));
  assert.equal(new Set(series.map(value => value.key)).size, names.length);
  assert.equal(new Set(series.map(value => value.label)).size, names.length);
});

test("host-Make and unknown boundaries remain selectable without changing measurements", () => {
  const points = [datedPoint(1, "2026-11-10T00:00:00Z", "Build sonic-vs.bin through the host Make wrapper"),
    datedPoint(2, "2026-11-11T00:00:00Z", "Future unrecognized image step")];
  const before = structuredClone(points);
  const host = imageSeries(points[0].job), unknown = imageSeries(points[1].job);
  assert.match(host.label, /Host Make \.bin \+ archives/);
  assert.match(unknown.label, /Boundary unknown/);
  assert.deepEqual(selectTrendPoints(points, {series: host.key}).points, [points[0]]);
  assert.deepEqual(selectTrendPoints(points, {series: unknown.key}).points, [points[1]]);
  assert.deepEqual(points, before);
});

test("recognized future boundaries without a short label use a safe named fallback", () => {
  const context = vm.createContext({module: {exports: {}}, URL});
  vm.runInContext(readFileSync(require.resolve("./app.js"), "utf8"), context);
  vm.runInContext(`
    IMAGE_STEP_LABELS["Future installer boundary"] = "Future installer measurement";
    IMAGE_STEP_LABELS["constructor"] = "Future prototype-named boundary";
  `, context);
  const points = [datedPoint(1, "2026-11-10T00:00:00Z", "Future installer boundary"),
    datedPoint(2, "2026-11-11T00:00:00Z", "constructor")];
  const before = structuredClone(points);
  const {imageSeries: seriesFor, selectTrendPoints: select} = context.module.exports;
  for (const [index, label] of ["Future installer measurement", "Future prototype-named boundary"].entries()) {
    const series = seriesFor(points[index].job);
    assert.ok(series.label.includes(label));
    const selection = select(points, {series: series.key});
    assert.deepEqual(Array.from(selection.points, point => point.run.run_id), [index + 1]);
  }
  assert.notEqual(seriesFor(points[0].job).key, seriesFor(points[1].job).key);
  assert.deepEqual(points, before);
});

function datedPoint(run_id, started_at, name = "Build the VS installer (.bin)") {
  const point = timingPoint(name);
  const completed_at = started_at === null ? null : new Date(Date.parse(started_at) + 120000).toISOString();
  return {
    job: {...point.job, name: "Make VS image", job_id: run_id, status: "completed", started_at, completed_at,
      image_build_step_numbers: [1], url: `https://github.com/securely1g/sonic-buildimage/actions/runs/${run_id}/job/${run_id}`,
      steps: point.job.steps.map((step, index) => ({...step, number: index + 1, status: "completed", conclusion: "success", started_at, completed_at}))},
    run: {run_id, attempt: 1, status: "completed", conclusion: "success", started_at, completed_at,
      started_at_source: "run_started_at", pr_numbers: [], source_sha: "abcdef0123456789",
      workflow_name: "Bazel OCI", workflow_path: ".github/workflows/bazel-oci.yml", branch: "master"},
    execution: {startedAt: started_at},
  };
}

test("chart date windows use the original execution time and count carried retries once", () => {
  const snapshot = pr72CarriedSnapshot(), measurements = imageMeasurements(snapshot);
  const reference = Date.parse("2026-10-31T12:00:00Z");
  assert.equal(measurements.records.length, 1);
  assert.equal(selectTrendPoints(measurements.records, {days: "1", reference}).points.length, 0);
  const points = selectTrendPoints(measurements.records, {days: "2", reference}).points;
  assert.equal(points.length, 1);
  assert.equal(points[0].execution.startedAt, "2026-10-30T11:55:36.000Z");
  assert.equal(points[0].run.attempt, 2);
});

test("chart limits expose the newest measurements and show-all preserves the entire selection", () => {
  const reference = Date.parse("2026-11-12T00:00:00Z");
  const points = Array.from({length: 84}, (_, index) => datedPoint(index + 1,
    new Date(reference - index * 3600000).toISOString(),
    index % 2 ? "Build sonic-vs.bin directly with slave.mk" : "Build the VS installer (.bin)"));
  const before = structuredClone(points);
  const selection = selectTrendPoints(points);
  assert.equal(selection.matching.length, 84);
  assert.equal(selection.points.length, 10);
  assert.deepEqual([selection.points[0].run.run_id, selection.points.at(-1).run.run_id], [10, 1]);
  assert.equal(selectTrendPoints(points, {limit: "60"}).points.length, 60);
  assert.equal(selectTrendPoints(points, {limit: "all"}).points.length, 84);
  const direct = selectTrendPoints(points, {series: imageSeries(points[1].job).key, limit: "all"});
  assert.equal(direct.points.length, 42);
  assert.ok(direct.points.every(point => point.job.steps[0].name.includes("directly")));
  const ten = selectTrendPoints(points, {series: imageSeries(points[1].job).key, limit: "10"});
  assert.deepEqual(ten.points, direct.points.slice(-10));
  assert.deepEqual(points, before);
});

test("date windows are inclusive and anchored to the snapshot rather than the viewer clock", () => {
  const reference = Date.parse("2026-11-12T00:00:00Z"), cutoff = reference - 7 * 86400000;
  const points = [datedPoint(1, new Date(cutoff - 1).toISOString()), datedPoint(2, new Date(cutoff).toISOString()),
    datedPoint(3, new Date(reference).toISOString()), datedPoint(4, new Date(reference + 1).toISOString()),
    datedPoint(5, null)];
  assert.deepEqual(selectTrendPoints(points, {days: "7", reference}).points.map(point => point.run.run_id), [2, 3]);
  assert.equal(selectTrendPoints(points, {days: "all", reference}).points.length, 5);
  assert.equal(selectTrendPoints(points.slice(0, 1), {days: "7", reference}).points.length, 0);
});

test("duration scales use readable intervals without clipping long or zero-length builds", () => {
  assert.deepEqual(chartScale(269 * 60), {maximum: 5 * 3600, ticks: [0, 3600, 7200, 10800, 14400, 18000]});
  for (const maximum of [0, 0.1, 25, 120, 5399, 3 * 86400]) {
    const scale = chartScale(maximum);
    assert.ok(scale.maximum > maximum);
    assert.equal(scale.ticks[0], 0);
    assert.equal(scale.ticks.at(-1), scale.maximum);
    assert.ok(scale.ticks.length >= 2 && scale.ticks.length <= 6);
  }
});

test("point previews distinguish available, partial, missing and malformed breakdowns", () => {
  const job = timingPoint("Build the VS installer (.bin)").job;
  assert.equal(imageBreakdownLabel(job), "Breakdown not recorded");
  assert.equal(imageBreakdownLabel({...job, steps: [{...job.steps[0], breakdown: {version: 99}}]}), "Breakdown unavailable");
  const step = {...job.steps[0], breakdown: exampleBreakdown()};
  assert.equal(imageBreakdownLabel({...job, steps: [step]}), "Measured breakdown");
  assert.equal(imageBreakdownLabel({...job, steps: [step, {name: "Build the VS image"}]}), "Partial breakdown");
  assert.equal(imageBreakdownLabel({...job, steps: [step, {name: "Validate archives"}]}), "Measured breakdown");
});

function imageOutcome(conclusion, extra = {}) {
  return {kind: "make-vs", status: "completed", conclusion, ...extra};
}

test("PR outcomes come from image jobs, including failed images without timings", () => {
  for (const outcome of ["success", "failure", "cancelled", "skipped"]) {
    assert.equal(imageResult([imageOutcome(outcome, {image_build_seconds: null})]), outcome);
  }
  for (const conclusion of ["timed_out", "startup_failure"]) {
    assert.equal(imageResult([imageOutcome(conclusion, {kind: "bazel-vs"})]), "failure");
  }
  assert.equal(imageResult([]), "unmeasured");
  assert.equal(imageResult([imageOutcome("success", {kind: "source"})]), "unmeasured");
  assert.equal(imageResult([imageOutcome("failure", {kind: "checks"}), imageOutcome("skipped")]), "skipped");
});

test("pending and unknown image outcomes never become skipped or successful", () => {
  for (const status of ["queued", "in_progress", "requested", "waiting", "pending"]) {
    assert.equal(imageResult([imageOutcome(null, {status})]), "pending");
  }
  for (const conclusion of [null, undefined, "neutral", "action_required", "stale", "unknown"]) {
    assert.equal(imageResult([imageOutcome(conclusion)]), "unmeasured");
  }
});

test("multiple image paths produce one conservative PR outcome", () => {
  const cases = [
    [["success", "skipped"], "success"],
    [["skipped", "skipped"], "skipped"],
    [["success", "cancelled"], "cancelled"],
    [["success", "neutral"], "unmeasured"],
    [["cancelled", "action_required"], "unmeasured"],
    [["success", "failure"], "failure"],
    [["cancelled", "timed_out"], "failure"],
  ];
  for (const [outcomes, expected] of cases) {
    const jobs = outcomes.map((outcome, index) => imageOutcome(outcome, {kind: index ? "bazel-vs" : "make-vs"}));
    assert.equal(imageResult(jobs), expected);
    assert.equal(imageResult([...jobs].reverse()), expected);
  }
  const pending = imageOutcome(null, {kind: "bazel-vs", status: "in_progress"});
  assert.equal(imageResult([imageOutcome("success"), pending]), "pending");
  assert.equal(imageResult([imageOutcome("neutral"), pending]), "pending");
  assert.equal(imageResult([imageOutcome("failure"), pending]), "failure");
});

function coverageRun(run_id, pr_numbers, conclusion, extra = {}) {
  return {run_id, attempt: 1, started_at: `2026-10-27T10:${String(run_id).padStart(2, "0")}:00Z`,
    pr_numbers, jobs: [imageOutcome(conclusion)], ...extra};
}

test("latest skipped or cancelled image attempts supersede historical success", () => {
  const snapshot = {pull_requests: [{number: 1}, {number: 2}], runs: [
    coverageRun(1, [1, 2], "success"), coverageRun(2, [1], "skipped"), coverageRun(3, [2], "cancelled"),
  ]};
  const before = structuredClone(snapshot), coverage = prCoverage(snapshot);
  assert.deepEqual(coverage.map(entry => [entry.pr.number, entry.classification, entry.latestImageRun.run_id]),
    [[2, "cancelled", 3], [1, "skipped", 2]]);
  assert.deepEqual(snapshot, before, "coverage must preserve collected history and ordering");
  assert.deepEqual(summarizePrCoverage(coverage), {total: 2, success: 0, failure: 0, cancelled: 1, skipped: 1, pending: 0, unmeasured: 0});
});

test("a newer non-image workflow does not replace the associated image result", () => {
  const imageRun = coverageRun(1, [1], "failure", {source_sha: "historical-revision"});
  const snapshot = {pull_requests: [{number: 1, head_sha: "current-revision"}, {number: 2}], runs: [
    imageRun, coverageRun(2, [1, 2], "success", {conclusion: "success", jobs: [imageOutcome("success", {kind: "checks"})]}),
  ]};
  const coverage = prCoverage(snapshot);
  assert.equal(coverage[0].classification, "unmeasured");
  assert.equal(coverage[0].latestImageRun, null);
  assert.deepEqual(coverage[0].jobs, []);
  assert.equal(coverage[1].classification, "failure");
  assert.equal(coverage[1].latestImageRun, imageRun, "retain source revision and run link for the historical badge");
});

test("same-time retries select the higher attempt regardless of snapshot ordering", () => {
  const earlier = coverageRun(1, [1], "failure"), retry = {...earlier, attempt: 2, jobs: [imageOutcome("success")]};
  for (const runs of [[earlier, retry], [retry, earlier]]) {
    const coverage = prCoverage({pull_requests: [{number: 1}], runs});
    assert.equal(coverage[0].latestImageRun, retry);
    assert.equal(coverage[0].classification, "success");
    assert.equal(summarizePrCoverage(coverage).total, 1);
  }
});

test("PR summary counts every PR once without treating workflow conclusions as image outcomes", () => {
  const snapshot = {pull_requests: Array.from({length: 6}, (_, index) => ({number: index + 1})), runs: [
    coverageRun(1, [1], "success"), coverageRun(2, [1], "success"),
    coverageRun(3, [2], "failure"), coverageRun(4, [3], "cancelled"),
    coverageRun(5, [4], "skipped", {conclusion: "success"}),
    coverageRun(6, [5], null, {jobs: [imageOutcome(null, {status: "queued"})]}),
    coverageRun(7, [6], "success", {conclusion: "success", jobs: []}),
  ]};
  const coverage = prCoverage(snapshot), summary = summarizePrCoverage(coverage);
  assert.deepEqual(summary, {total: 6, success: 1, failure: 1, cancelled: 1, skipped: 1, pending: 1, unmeasured: 1});
  assert.equal(Object.entries(summary).filter(([key]) => key !== "total").reduce((sum, [, count]) => sum + count, 0), 6);
  assert.deepEqual(coverage.filter(entry => ["success", "failure"].includes(entry.classification)).map(entry => entry.pr.number), [2, 1]);
});

test("empty PR coverage retains explicit zero counts", () => {
  assert.deepEqual(prCoverage({pull_requests: [], runs: []}), []);
  assert.deepEqual(summarizePrCoverage([]), {total: 0, success: 0, failure: 0, cancelled: 0, skipped: 0, pending: 0, unmeasured: 0});
});

test("metadata links stay on the configured GitHub repository", () => {
  const repo = "securely1g/sonic-buildimage";
  const expected = `https://github.com/${repo}/actions/runs/123`;
  assert.equal(safeGithubUrl(expected, repo), expected);
  for (const url of ["javascript:alert(1)", "data:text/html,hi", "https://github.com.evil.test/securely1g/sonic-buildimage/", "https://evil.test/", "https://github.com/other/repository/", "https://github.com/securely1g/sonic-buildimage-other/", "https://user:secret@github.com/securely1g/sonic-buildimage/", "https://github.com:444/securely1g/sonic-buildimage/", "https://github.com/securely1g/sonic-buildimage/../../other/repository/"]) assert.equal(safeGithubUrl(url, repo), null);
});

test("unsupported or partial snapshots fail explicitly", () => {
  const snapshot = {schema_version: 1, repository: "securely1g/sonic-buildimage", pull_requests: [], runs: []};
  assert.equal(validateSnapshot(snapshot), snapshot);
  for (const malformed of [null, {...snapshot, schema_version: 2}, {...snapshot, repository: "bad<script>"}, {...snapshot, runs: [{run_id: 1}]}, {...snapshot, pull_requests: [{number: "1", title: "PR"}]}]) assert.throws(() => validateSnapshot(malformed));
});

function exampleBreakdown() {
  return {version: 1, status: "available", reason: null, source: "github-job-log", duration_seconds: 100, sections: [
    {id: "vs-image-1", label: "Ignored source label", offset_seconds: 0, duration_seconds: 60, tasks: [
      {id: "base-1", offset_seconds: 0, duration_seconds: 40, outcome: "finished"},
      {id: "config-engine-1", offset_seconds: 10, duration_seconds: 40, outcome: "finished"},
      {id: "final-image-1", offset_seconds: 30, duration_seconds: 30, outcome: "finished"},
    ]},
    {id: "p4rt-debug-1", offset_seconds: 60, duration_seconds: 40, tasks: [
      {id: "p4rt-debug-1", offset_seconds: 10, duration_seconds: 1.25, outcome: "cached"},
    ]},
  ]};
}

test("sequential sections partition wall time while child tasks may overlap", () => {
  const source = exampleBreakdown(), before = structuredClone(source);
  source.sections[0].tasks.reverse();
  const parsed = normalizeBreakdown(source);
  assert.equal(parsed.status, "available");
  assert.equal(parsed.sections.reduce((sum, section) => sum + section.duration_seconds, 0), 100);
  assert.equal(parsed.sections[0].tasks.reduce((sum, task) => sum + task.duration_seconds, 0), 110);
  assert.deepEqual(parsed.sections[0].tasks.map(task => task.offset_seconds), [0, 10, 30]);
  source.sections[0].tasks.reverse();
  assert.deepEqual(source, before, "normalization does not alter the published snapshot");
  assert.equal(parsed.sections[0].label, "VS image, dependencies and cleanup");
  assert.equal(parsed.sections[1].tasks[0].outcome, "cached");
  assert.equal(parsed.sections[1].tasks[0].duration_seconds, 1.25);
});

test("malformed breakdowns degrade locally without displaying partial timing totals", () => {
  const mutate = change => { const value = exampleBreakdown(); change(value); return value; };
  const invalid = [
    null, {}, [], "log text", {...exampleBreakdown(), version: 2}, {...exampleBreakdown(), source: "arbitrary-url"},
    ...[null, "100", -1, 0, Infinity, NaN, Number.MAX_VALUE].map(duration_seconds => ({...exampleBreakdown(), duration_seconds})),
    {...exampleBreakdown(), sections: []}, {...exampleBreakdown(), sections: null},
    mutate(value => { value.sections[0] = null; }),
    mutate(value => { value.sections[0].id = "unrecognized-1"; }),
    mutate(value => { value.sections[0].id = "vs-image-0"; }),
    mutate(value => { value.sections[0].offset_seconds = 1; }),
    mutate(value => { value.sections[1].offset_seconds = 59; }),
    mutate(value => { value.sections[1].offset_seconds = 61; }),
    mutate(value => { value.sections[1].duration_seconds = 39; }),
    mutate(value => { value.sections[1].id = value.sections[0].id; }),
    mutate(value => { value.sections[0].tasks = null; }),
    mutate(value => { value.sections[0].tasks[0] = null; }),
    mutate(value => { value.sections[0].tasks[0].offset_seconds = -1; }),
    mutate(value => { value.sections[0].tasks[0].duration_seconds = "40"; }),
    mutate(value => { value.sections[0].tasks[0].duration_seconds = 61; }),
    mutate(value => { value.sections[0].tasks[0].offset_seconds = Infinity; }),
    mutate(value => { value.sections[0].tasks[0].outcome = "<script>"; }),
    mutate(value => { value.sections[0].tasks[0].id = "__proto__-1"; }),
    mutate(value => { value.sections[0].tasks[0].id = value.sections[0].tasks[1].id; }),
    mutate(value => { value.sections[0].tasks = Array.from({length: 513}, (_, i) => ({id: `base-${i + 1}`, offset_seconds: 0, duration_seconds: 1, outcome: "finished"})); }),
  ];
  for (const value of invalid) {
    const parsed = normalizeBreakdown(value);
    assert.equal(parsed.status, "unavailable");
    assert.equal(parsed.duration_seconds, undefined);
    assert.equal(parsed.sections, undefined);
  }
});

test("small timestamp rounding differences and empty task lists remain supported", () => {
  const source = exampleBreakdown();
  source.duration_seconds += .001;
  source.sections[1].tasks = [];
  const parsed = normalizeBreakdown(source);
  assert.equal(parsed.status, "available");
  assert.deepEqual(parsed.sections[1].tasks, []);
});

// A deliberately small DOM fixture: rendering must use nodes and text, never an HTML parser.
class TestElement {
  constructor(tag) { this.tagName = tag; this.children = []; this.style = {}; this.attributes = {}; this.text = ""; this.listeners = {}; }
  append(...children) { this.children.push(...children); }
  replaceChildren(...children) { this.text = ""; this.children = [...children]; }
  addEventListener(name, callback) { this.listeners[name] = callback; }
  get options() { return this.children; }
  set textContent(value) { this.text = value; this.children = []; }
  get textContent() { return this.text + this.children.map(child => child.textContent).join(""); }
  set innerHTML(_) { throw new Error("Untrusted data must not use innerHTML"); }
  setAttribute(name, value) { this.attributes[name] = value; }
}
function withDocument(callback) {
  const previous = global.document;
  global.document = {createElement: tag => new TestElement(tag), createTextNode: text => ({textContent: text})};
  try { return callback(); } finally { if (previous === undefined) delete global.document; else global.document = previous; }
}
function descendants(node, predicate) {
  return [node, ...(node.children || []).flatMap(child => descendants(child, predicate))].filter(predicate);
}
function chartFixture(points, width = 1100) {
  const nodes = Object.fromEntries(["trend", "trend-note", "trend-preview", "history-legend",
    "series-filter", "path-filter", "arch-filter", "source-filter", "commit-filter", "pr-filter", "date-filter", "limit-filter"]
    .map(id => [id, new TestElement(id.endsWith("filter") ? "select" : "div")]));
  for (const node of Object.values(nodes)) node.value = "all";
  nodes["source-filter"].value = "master";
  nodes["limit-filter"].value = "10";
  nodes.trend.clientWidth = width;
  const context = vm.createContext({module: {exports: {}}, URL, getComputedStyle: () => ({paddingLeft: "17px", paddingRight: "17px"})});
  vm.runInContext(readFileSync(require.resolve("./app.js"), "utf8"), context);
  context.document = {getElementById: id => nodes[id], createElement: tag => new TestElement(tag),
    createElementNS: (_, tag) => new TestElement(tag), createTextNode: text => ({textContent: text})};
  context.snapshot = {repository: "securely1g/sonic-buildimage", generated_at: "2026-11-12T00:00:00Z",
    runs: points.map(point => ({...point.run, jobs: point.jobs || [point.job]}))};
  const render = () => vm.runInContext("state.data = snapshot; renderTrend();", context);
  render();
  return {nodes, render};
}

test("chart renders host-Make and unknown-boundary snapshots and filters their series", () => {
  const points = [datedPoint(1, "2026-11-10T00:00:00Z", "Build sonic-vs.bin through the host Make wrapper"),
    datedPoint(2, "2026-11-11T00:00:00Z", "Future unrecognized image step")];
  const {nodes, render} = chartFixture(points);
  assert.equal(descendants(nodes.trend, node => node.attributes?.class === "chart-point").length, 2);
  assert.equal(nodes["history-legend"].children.length, 2);
  assert.match(nodes["history-legend"].textContent, /Host Make \.bin \+ archives/);
  assert.match(nodes["history-legend"].textContent, /Boundary unknown/);
  nodes["series-filter"].value = imageSeries(points[0].job).key;
  render();
  assert.equal(descendants(nodes.trend, node => node.attributes?.class === "chart-point").length, 1);
  assert.equal(nodes["history-legend"].children.length, 1);
  assert.match(nodes["history-legend"].textContent, /Host Make \.bin \+ archives/);
});

test("chart overview is scatter-only with a visible-series legend and keyboard point previews", () => {
  const points = [datedPoint(1, "2026-11-10T00:00:00Z"), datedPoint(2, "2026-11-11T00:00:00Z"),
    datedPoint(3, "2026-11-12T00:00:00Z", "Build sonic-vs.bin directly with slave.mk")];
  points[2].job.image_build_seconds = 269 * 60;
  const {nodes, render} = chartFixture(points);
  assert.equal(descendants(nodes.trend, node => node.tagName === "polyline").length, 0);
  assert.equal(nodes["history-legend"].children.length, 2);
  assert.doesNotMatch(nodes["history-legend"].textContent, /Bazel VS installer/);
  assert.match(nodes["trend-note"].textContent, /Showing all 3.*Select one measurement series/);
  const ticks = descendants(nodes.trend, node => node.tagName === "text").map(node => node.textContent);
  assert.ok(ticks.includes("1h") && ticks.includes("5h"), "long builds should use rounded hour ticks");
  const point = descendants(nodes.trend, node => node.attributes?.class === "chart-point")[0];
  assert.equal(point.attributes.tabindex, "0");
  point.listeners.focus();
  assert.match(nodes["trend-preview"].textContent, /2m 0s.*Outer Make .bin.*abcdef012345.*Bazel OCI.*VM generation and boot checks excluded.*Breakdown not recorded/);
  nodes["series-filter"].value = imageSeries(points[0].job).key;
  render();
  assert.equal(descendants(nodes.trend, node => node.tagName === "polyline").length, 1);
  assert.equal(nodes["history-legend"].children.length, 1);
  assert.doesNotMatch(nodes["history-legend"].textContent, /Direct slave/);
  assert.match(nodes["trend-note"].textContent, /Showing all 2.*one build source/);
});

test("the hover tooltip and keyboard preview identify the commit used for each build", () => {
  const master = datedPoint(1, "2026-11-10T00:00:00Z"), pr = datedPoint(2, "2026-11-11T00:00:00Z");
  Object.assign(master.run, {source_sha: "a".repeat(40), source_commit_title: "Build <installer> from source"});
  Object.assign(pr.run, {event: "pull_request", pr_numbers: [270], source_sha: "b".repeat(40),
    source_commit_title: "Require debug producers only for paired VS verification"});
  const {nodes, render} = chartFixture([master, pr]);
  nodes["source-filter"].value = "combined";
  render();
  const dots = descendants(nodes.trend, node => node.attributes?.class === "chart-point");
  assert.equal(dots.length, 2);
  for (const [index, {run}] of [master, pr].entries()) {
    const dot = dots[index], expected = `Commit ${run.source_sha.slice(0, 12)} · ${run.source_commit_title}`;
    assert.equal(dot.children[0].tagName, "title");
    assert.equal(dot.children[0].textContent.split("\n")[0], expected);
    assert.equal(dot.attributes["aria-label"], dot.children[0].textContent);
    for (const event of ["mouseenter", "focus"]) {
      dot.listeners[event]();
      const preview = nodes["trend-preview"];
      assert.ok(preview.textContent.includes(expected));
      assert.equal(descendants(preview, node => node.tagName === "a")[0].href,
        `https://github.com/securely1g/sonic-buildimage/commit/${run.source_sha}`);
    }
  }
});

test("unknown boundaries never gain trend lines and empty filters clear stale chart details", () => {
  const points = [datedPoint(1, "2026-11-01T00:00:00Z", "Unknown image step"),
    datedPoint(2, "2026-11-02T00:00:00Z", "Unknown image step")];
  const {nodes, render} = chartFixture(points, 320);
  assert.equal(descendants(nodes.trend, node => node.attributes?.class === "chart-point").length, 2);
  nodes["series-filter"].value = imageSeries(points[0].job).key;
  render();
  assert.equal(descendants(nodes.trend, node => node.tagName === "polyline").length, 0);
  nodes["date-filter"].value = "7";
  render();
  assert.match(nodes.trend.textContent, /No successful image measurements/);
  assert.equal(nodes["history-legend"].children.length, 0);
  assert.equal(nodes["history-legend"].hidden, true);
  assert.ok(nodes["trend-preview"].hidden || !nodes["trend-preview"].textContent.includes("Hover or focus a point"));
  assert.doesNotMatch(nodes.trend.textContent, /NaN|Infinity/);
});

test("a selected master commit with a pending installer explains the missing duration and links to that job", () => {
  const sha = "a".repeat(40), point = datedPoint(2, "2026-11-11T18:00:00Z");
  Object.assign(point.run, {source_sha: sha, event: "push", status: "in_progress", conclusion: null, completed_at: null});
  const job = {...point.job, name: "VS installer in SONiC slave (AMD64)", status: "queued", conclusion: null,
    completed_at: null, image_build_seconds: null, steps: []};
  const skipped = {...job, name: "Make VS with Bazel SWSS (AMD64)", job_id: 3, status: "completed",
    conclusion: "skipped", url: job.url.replace("/job/2", "/job/3")};
  point.jobs = [skipped, job];
  const {nodes, render} = chartFixture([point]);
  nodes["source-filter"].value = "master";
  nodes["commit-filter"].value = sha;
  for (const [status, expected] of [["queued", /queued|waiting for a runner/i], ["in_progress", /running|in progress/i]]) {
    job.status = status;
    render();
    assert.match(nodes.trend.textContent, expected);
    assert.ok(descendants(nodes.trend, el => el.tagName === "a").some(link => link.href === job.url));
    assert.equal(descendants(nodes.trend, el => el.attributes?.class === "chart-point").length, 0);
    assert.ok(nodes["trend-preview"].hidden || !nodes["trend-preview"].textContent.includes("Hover or focus a point"));
  }

  const success = datedPoint(1, "2026-11-01T00:00:00Z");
  success.run.source_sha = sha;
  const dated = chartFixture([success, point]);
  dated.nodes["source-filter"].value = "master";
  dated.nodes["commit-filter"].value = sha;
  dated.nodes["date-filter"].value = "7";
  dated.render();
  assert.match(dated.nodes.trend.textContent, /match.*filters/i);
  assert.equal(descendants(dated.nodes.trend, el => el.tagName === "a").length, 0);
  assert.equal(descendants(dated.nodes.trend, el => el.attributes?.class === "chart-point").length, 0);
  dated.nodes["date-filter"].value = "all";
  dated.render();
  assert.equal(descendants(dated.nodes.trend, el => el.attributes?.class === "chart-point").length, 1);
  assert.equal(dated.nodes["trend-preview"].hidden, false);
  assert.match(dated.nodes["trend-preview"].textContent, /Hover or focus a point/);
});

async function loadDashboard(points, pullRequests = [], fetchData) {
  const html = readFileSync(require.resolve("./index.html"), "utf8");
  const nodes = Object.fromEntries([...html.matchAll(/id="([^"]+)"/g)].map(([, id]) => [id, new TestElement("div")]));
  for (const node of Object.values(nodes)) node.value = "all";
  for (const [, id, options] of html.matchAll(/<select id="([^"]+)">([\s\S]*?)<\/select>/g)) {
    const choices = [...options.matchAll(/<option value="([^"]+)"( selected)?>/g)];
    nodes[id].value = (choices.find(choice => choice[2]) || choices[0])[1];
  }
  nodes.trend.clientWidth = 1100;
  const snapshot = () => ({schema_version: 1, repository: "securely1g/sonic-buildimage", generated_at: "2026-11-12T00:00:00Z",
    pull_requests: pullRequests, runs: points.map(point => ({...point.run, jobs: [point.job]}))});
  const context = vm.createContext({
    document: {getElementById: id => nodes[id], createElement: tag => new TestElement(tag),
      createElementNS: (_, tag) => new TestElement(tag), createTextNode: text => ({textContent: text})},
    getComputedStyle: () => ({paddingLeft: "17px", paddingRight: "17px"}),
    ResizeObserver: class { observe() {} },
    setInterval() {}, URL, AbortSignal, btoa,
    window: {location: {href: "https://example.test/"}},
    fetch: fetchData || (async () => ({ok: true, json: async () => snapshot()})),
  });
  vm.runInContext(readFileSync(require.resolve("./app.js"), "utf8"), context);
  await new Promise(resolve => setImmediate(resolve));
  assert.equal(nodes["load-status"].textContent, "", "the actual page must load without missing-element errors");
  assert.equal(nodes.dashboard.hidden, false);
  return nodes;
}

test("a split snapshot loads all master and PR timings and keeps them if a refresh fails", async () => {
  const runs = ["push", "pull_request"].map((event, index) => {
    const point = datedPoint(index + 1, `2026-11-0${index + 1}T12:00:00Z`);
    return {...point.run, event, pr_numbers: index ? [270] : [], jobs: [point.job]};
  });
  const parts = runs.map((run, index) => ({
    path: `history/${String(index + 1).repeat(64)}.json`, count: 1,
  }));
  const index = {schema_version: 2, repository: "securely1g/sonic-buildimage",
    generated_at: "2026-11-12T00:00:00Z", pull_requests: [{number: 270, title: "Build the VS installer"}],
    comparisons: [], run_shards: parts};
  let contents = index, fail = false;
  const requests = [];
  const nodes = await loadDashboard([], [], async (url, options) => {
    requests.push(url.pathname);
    if (url.pathname === "/data.json") {
      assert.equal(options.cache, "no-store");
      return {ok: true, json: async () => contents};
    }
    const position = parts.findIndex(part => "/" + part.path === url.pathname);
    assert.ok(position >= 0);
    assert.equal(url.origin, "https://example.test");
    assert.equal(options.cache, "force-cache");
    assert.equal(options.integrity, "sha256-" + Buffer.from(parts[position].path.slice(8, -5), "hex").toString("base64"));
    if (fail && position === 1) return {ok: false, status: 404};
    return {ok: true, json: async () => [runs[position]]};
  });
  assert.deepEqual(requests, ["/data.json", ...parts.map(part => "/" + part.path)]);
  assert.equal(nodes["run-count"].textContent, "1 of 1");
  nodes["source-filter"].value = "pr";
  nodes["source-filter"].listeners.change();
  nodes["pr-filter"].value = "270";
  nodes["pr-filter"].listeners.change();
  assert.equal(descendants(nodes.trend, el => el.attributes?.class === "chart-point").length, 1);
  const detail = nodes.details.textContent;
  assert.match(detail, /PR #270/);

  fail = true;
  await nodes.refresh.listeners.click();
  assert.match(nodes["load-status"].textContent, /last loaded snapshot.*404/);
  assert.equal(nodes.details.textContent, detail);
  fail = false;
  contents = {...index, run_shards: [{...parts[0], count: 2}]};
  await nodes.refresh.listeners.click();
  assert.match(nodes["load-status"].textContent, /last loaded snapshot.*incomplete/);
  for (const bad of ["https://other.test/history/file.json", "../file.json", parts[0].path + "?next=1"]) {
    requests.length = 0;
    contents = {...index, run_shards: [parts[0], {...parts[1], path: bad}]};
    await nodes.refresh.listeners.click();
    assert.deepEqual(requests, ["/data.json"]);
    assert.match(nodes["load-status"].textContent, /invalid file/);
  }
  contents = index;
  await nodes.refresh.listeners.click();
  assert.equal(nodes["load-status"].textContent, "");
  assert.equal(nodes["pr-filter"].value, "270");
});

test("page startup wires chart-only filters without hiding retained workflow history", async () => {
  const points = [datedPoint(1, "2026-11-01T00:00:00Z"),
    datedPoint(2, "2026-11-12T00:00:00Z", "Build sonic-vs.bin directly with slave.mk")];
  const nodes = await loadDashboard(points);
  assert.equal(nodes["run-count"].textContent, "2 of 2");
  nodes["date-filter"].value = "7";
  nodes["date-filter"].listeners.change();
  assert.match(nodes["trend-note"].textContent, /Showing all 1/);
  assert.equal(nodes["run-count"].textContent, "2 of 2", "chart date controls must not change retained history");
  nodes["series-filter"].value = imageSeries(points[1].job).key;
  nodes["series-filter"].listeners.change();
  const point = descendants(nodes.trend, node => node.attributes?.class === "chart-point")[0];
  assert.match(point.attributes["aria-label"], /Commit abcdef012345 · Commit message unavailable.*Breakdown not recorded/s);
  point.listeners.mouseenter();
  assert.match(nodes["trend-preview"].textContent, /Direct slave .bin/);
  let activated = false;
  point.listeners.keydown({key: "Enter", preventDefault() { activated = true; }});
  assert.equal(activated, true);
  assert.match(nodes.details.textContent, /Build sonic-vs.bin directly with slave.mk/);
});

test("build source filters the chart and run history, including PR events without a PR number", async () => {
  const points = [
    {event: "push", branch: "master", pr_numbers: [20]},
    {event: "pull_request", branch: "master"},
    {event: "schedule", branch: "master"},
    {event: "workflow_dispatch", branch: "topic", pr_numbers: [20]},
    {event: "push", branch: "release"},
    {event: "pull_request_target", branch: "topic", pr_numbers: [20], conclusion: "failure"},
  ].map((run, index) => {
    const point = datedPoint(index + 1, `2026-11-0${index + 1}T00:00:00Z`);
    Object.assign(point.run, run, {workflow_name: `Workflow ${index + 1}`,
      url: `https://github.com/securely1g/sonic-buildimage/actions/runs/${index + 1}`});
    point.job.conclusion = point.run.conclusion;
    return point;
  });
  const nodes = await loadDashboard(points, [{number: 20, title: "Image test"}]);
  const select = (id, value) => { nodes[id].value = value; nodes[id].listeners.change(); };
  const markers = () => descendants(nodes.trend, node => node.attributes?.class === "chart-marker");
  const runs = () => nodes["runs-body"].children.map(row => descendants(row, node => node.tagName === "button")[0]?.textContent);
  const green = "#187247", yellow = "#facc15", ochre = "#805f00";
  assert.equal(nodes["source-filter"].value, "master");
  assert.equal(nodes["commit-filter"].value, "all");
  assert.equal(nodes["limit-filter"].value, "10");
  assert.deepEqual(runs(), ["Workflow 3", "Workflow 1"]);
  assert.deepEqual(markers().map(marker => marker.attributes.fill), [green, green]);
  select("source-filter", "combined");
  assert.deepEqual(runs(), ["Workflow 6", "Workflow 4", "Workflow 3", "Workflow 2", "Workflow 1"]);
  assert.deepEqual(markers().map(marker => marker.attributes.fill), [green, yellow, green, yellow]);
  assert.match(nodes["history-legend"].textContent, /Master/i);
  assert.match(nodes["history-legend"].textContent, /Pull request/i);
  assert.ok(markers().filter(marker => marker.attributes.fill === yellow).every(marker => marker.attributes.stroke === ochre));
  const prDot = nodes["history-legend"].children.find(item => item.textContent.startsWith("Pull request")).children[0];
  assert.equal(prDot.style.backgroundColor, yellow);
  assert.equal(prDot.style.border, `1px solid ${ochre}`);
  assert.match(nodes["coverage-summary"].textContent, /Failed1/);
  assert.equal(nodes["coverage-body"], undefined);
  assert.match(nodes["latest-branch-status"].textContent, /master/i);
  assert.match(nodes["latest-branch-status"].textContent, /success/i);
  assert.equal(descendants(nodes["latest-branch-status"], node => node.tagName === "a")[0].href, points[2].run.url);

  select("series-filter", imageSeries(points[0].job).key);
  const lines = descendants(nodes.trend, node => node.tagName === "polyline");
  assert.equal(lines.length, 2);
  for (const line of lines) {
    const sameSource = markers().filter(marker => (marker.attributes.fill === yellow ? ochre : green) === line.attributes.stroke);
    assert.equal(sameSource.length, 2);
    assert.equal(line.attributes.points, sameSource.map(({attributes}) => `${attributes.cx},${attributes.cy}`).join(" "));
  }

  select("source-filter", "pr");
  assert.equal(nodes["latest-branch-status"].hidden, true);
  assert.equal(nodes["pr-build-status"].hidden, false);
  assert.deepEqual(runs(), ["Workflow 6", "Workflow 4", "Workflow 2"]);
  assert.deepEqual(markers().map(marker => marker.attributes.fill), [yellow, yellow]);
  select("pr-filter", "20");
  assert.deepEqual(runs(), ["Workflow 6", "Workflow 4"]);
  select("source-filter", "master");
  assert.equal(nodes["latest-branch-status"].hidden, false);
  assert.equal(nodes["pr-build-status"].hidden, true);
  assert.equal(nodes["pr-filter"].value, "all");
  assert.equal(nodes["pr-filter"].disabled, true);
  assert.deepEqual(runs(), ["Workflow 3", "Workflow 1"]);
  assert.deepEqual(markers().map(marker => marker.attributes.fill), [green, green]);
  select("source-filter", "other");
  assert.equal(descendants(nodes["latest-branch-status"], node => node.tagName === "a")[0].href, points[4].run.url);
  assert.deepEqual(runs(), ["Workflow 5"]);
  assert.equal(markers().length, 1);
  select("source-filter", "combined");
  assert.equal(nodes["pr-filter"].disabled, false);
  assert.equal(markers().length, 4);
});

test("choosing a master commit scopes status, chart, history and details to the exact revision", async () => {
  const succeeded = "0123456789ab" + "1".repeat(28), failed = "0123456789ab" + "2".repeat(28);
  const checksOnly = "a".repeat(40), prOnly = "b".repeat(40);
  const points = [succeeded, failed, succeeded, checksOnly, prOnly, null].map((sha, index) => {
    const id = index + 1;
    const point = datedPoint(id, `2026-11-0${id}T18:00:00Z`);
    Object.assign(point.run, {source_sha: sha, event: "push", workflow_name: `Workflow ${id}`,
      url: `https://github.com/securely1g/sonic-buildimage/actions/runs/${id}`});
    return point;
  });
  points[1].job.conclusion = points[1].run.conclusion = "failure";
  points[3].job.kind = "checks";
  points[4].run.event = "pull_request"; // A PR can also come from a branch named master.
  const nodes = await loadDashboard(points);
  const select = (id, value) => { nodes[id].value = value; nodes[id].listeners.change(); };
  const commits = nodes["commit-filter"];
  const chartPoints = () => descendants(nodes.trend, node => node.attributes?.class === "chart-point");
  const history = () => descendants(nodes["runs-body"], node => node.tagName === "button").map(node => node.textContent);
  assert.equal(nodes["commit-filter-label"].hidden, false);
  assert.ok(nodes["pr-filter-label"].hidden);
  assert.deepEqual(commits.options.map(option => option.value), ["all", checksOnly, succeeded, failed]);
  const optionLabel = sha => commits.options.find(option => option.value === sha).textContent;
  for (const sha of [succeeded, failed]) assert.ok(optionLabel(sha).includes(sha.slice(0, 13)), "colliding SHA prefixes must be distinguishable");

  select("commit-filter", succeeded);
  assert.deepEqual(history(), ["Workflow 3", "Workflow 1"]);
  assert.equal(chartPoints().length, 2);
  assert.match(nodes["latest-branch-status"].textContent, /success/i);
  assert.ok(descendants(nodes["latest-branch-status"], node => node.href === points[2].run.url).length);
  assert.match(nodes.details.textContent, /Workflow 3/);
  select("commit-filter", failed);
  assert.deepEqual(history(), ["Workflow 2"]);
  assert.equal(chartPoints().length, 0);
  assert.match(nodes["latest-branch-status"].textContent, /fail/i);
  assert.match(nodes.details.textContent, /Workflow 2/);
  select("commit-filter", checksOnly);
  assert.deepEqual(history(), ["Workflow 4"]);
  assert.match(nodes["latest-branch-status"].textContent, /no image build/i);
  assert.match(nodes.details.textContent, /Workflow 4/);

  select("source-filter", "pr");
  assert.ok(nodes["commit-filter-label"].hidden);
  assert.equal(commits.value, "all");
  assert.equal(nodes["pr-filter-label"].hidden, false);
  assert.deepEqual(history(), ["Workflow 5"]);
  select("source-filter", "master");
  assert.deepEqual(history(), ["Workflow 6", "Workflow 4", "Workflow 3", "Workflow 2", "Workflow 1"]);
});

test("master commit labels pair each revision with its own title and latest run date", async () => {
  const installer = "1".repeat(40), kernel = "2".repeat(40), untitled = "3".repeat(40);
  const points = [installer, kernel, untitled, installer, untitled, untitled].map((sha, index) => {
    const id = index + 1;
    const point = datedPoint(id, `2026-11-0${id}T18:00:00Z`);
    Object.assign(point.run, {source_sha: sha, workflow_name: "VS workflow", event: "push"});
    return point;
  });
  points[0].run.source_commit_title = "Build <installer> from the shared archive";
  points[1].run.source_commit_title = "Update the kernel configuration";
  Object.assign(points[4].run, {event: "pull_request", source_commit_title: "PR title"});
  Object.assign(points[5].run, {branch: "topic", source_commit_title: "Feature branch title"});
  const nodes = await loadDashboard(points);
  nodes["source-filter"].value = "master";
  nodes["source-filter"].listeners.change();
  const commits = nodes["commit-filter"].options;
  assert.deepEqual(commits.map(option => option.value), ["all", installer, untitled, kernel]);
  const label = sha => commits.find(option => option.value === sha).textContent;
  const runDate = point => new Date(point.run.started_at).toLocaleString(undefined,
    {month: "short", day: "numeric", year: "numeric", hour: "numeric", minute: "2-digit", timeZoneName: "short"});
  assert.equal(label(installer), `111111111111 · Build <installer> from the shared archive · ${runDate(points[3])}`);
  assert.equal(label(kernel), `222222222222 · Update the kernel configuration · ${runDate(points[1])}`);
  assert.equal(label(untitled), `333333333333 · ${runDate(points[2])}`);
});

test("refresh preserves a selected master commit until it leaves the retained history", async () => {
  const kept = datedPoint(1, "2026-11-01T18:00:00Z"), removed = datedPoint(2, "2026-11-02T18:00:00Z");
  kept.run.source_sha = "1".repeat(40);
  removed.run.source_sha = "2".repeat(40);
  const points = [kept, removed];
  const nodes = await loadDashboard(points);
  nodes["source-filter"].value = "master";
  nodes["source-filter"].listeners.change();
  const commits = nodes["commit-filter"];
  commits.value = removed.run.source_sha;
  commits.listeners.change();
  removed.run.source_commit_title = "Refresh the base image";
  await nodes.refresh.listeners.click();
  assert.equal(commits.value, removed.run.source_sha);
  assert.match(commits.options.find(option => option.value === commits.value).textContent, /Refresh the base image/);
  assert.equal(nodes["runs-body"].children.length, 1);
  points.pop();
  await nodes.refresh.listeners.click();
  assert.equal(commits.value, "all");
  assert.deepEqual(commits.options.map(option => option.value), ["all", kept.run.source_sha]);
  assert.match(nodes.details.textContent, /Source 111111111111/);
});

test("a master build associated with a merge keeps the PR's own image result", async () => {
  const pr = datedPoint(1, "2026-11-01T00:00:00Z"), merge = datedPoint(2, "2026-11-02T00:00:00Z");
  Object.assign(pr.run, {event: "pull_request", branch: "topic", pr_numbers: [19]});
  pr.job.conclusion = "failure";
  Object.assign(merge.run, {event: "push", branch: "master", pr_numbers: [19]});
  const nodes = await loadDashboard([merge, pr], [{number: 19, title: "Image test"}]);
  nodes["source-filter"].value = "combined";
  nodes["source-filter"].listeners.change();
  assert.match(nodes["latest-branch-status"].textContent, /success/);
  assert.match(nodes["coverage-summary"].textContent, /Success0Failed1/);
});

test("master status is empty when only a PR used a branch named master", async () => {
  const point = datedPoint(1, "2026-11-01T00:00:00Z");
  point.run.event = "pull_request";
  const nodes = await loadDashboard([point]);
  assert.match(nodes["latest-branch-status"].textContent, /no image build recorded/i);
  assert.equal(descendants(nodes["latest-branch-status"], node => node.tagName === "a").length, 0);
});

function stepWith(breakdown) { return {name: "Make VS image", status: "completed", conclusion: "success", duration_seconds: 101, ...(breakdown === undefined ? {} : {breakdown})}; }

const hostStepName = "Build sonic-vs.bin through the host Make wrapper";

function recipeJob() {
  const images = ["docker-fpm-frr.gz", "docker-mux.gz", "docker-teamd.gz", "docker-syncd-vs.gz"]
    .map(owner => ({owner, bazel_seconds: 12.5, lock_seconds: 2.25, compile_seconds: 4, foreign_seconds: 0,
      fetch_seconds: 0.125, export_seconds: 1.5, bazel_phases: 3, profiles: 3, exports: 2, failed: false}));
  return {kind: "make-vs", steps: [{name: hostStepName}], image_recipe_timings: {version: 1, status: "available", images}};
}

test("the host Make installer has a consistent and separate chart boundary", () => {
  const old = timingPoint("Build the VS installer (.bin)"), first = timingPoint(hostStepName), next = timingPoint(hostStepName);
  assert.deepEqual(imageTrendGroups([old, first, next]), [[old], [first, next]]);
  assert.match(imageMeasurement(first.job).label, /through host Make/);
});

test("previous four-image receipts retain measured and partial coverage", () => withDocument(() => {
  const job = recipeJob(), [frr, mux, teamd] = job.image_recipe_timings.images;
  Object.assign(frr, {failed: true, bazel_phases: 1, profiles: 1, exports: 0, export_seconds: null});
  Object.assign(mux, {profiles: 0, lock_seconds: null, compile_seconds: null, foreign_seconds: null, fetch_seconds: null});
  Object.assign(teamd, {lock_seconds: 0.0001});
  const view = renderRecipeTimings(job);
  const rows = descendants(view, item => item.tagName === "details");
  assert.equal(rows.length, 4);
  assert.match(view.textContent, /four image recipes/);
  assert.match(rows.at(-1).textContent, /docker-fpm-frr.gzCommand failed12.5sPartial/);
  assert.match(rows.at(-1).textContent, /Bazel lock wait2.25s \(1\/3\)/);
  assert.match(rows.at(-1).textContent, /Output handoff— \(0\/2\)/);
  assert.match(view.textContent, /Bazel lock wait<0.001s/);
  assert.match(view.textContent, /can overlap/);
  assert.match(view.textContent, /disk and remote reads/);
}));

test("all build recipes are ordered by recorded commands and exports with separate profile coverage", () => withDocument(() => {
  const job = recipeJob();
  const image = {...job.image_recipe_timings.images[0], owner: "docker-orchagent.gz",
    bazel_expected: 3, profiles_expected: 3, exports_expected: 2};
  const kernel = {...image, owner: "linux-headers-amd64", bazel_seconds: 120, bazel_expected: 1, bazel_phases: 1,
    profiles_expected: 3, profiles: 2, exports_expected: 0, exports: 0, export_seconds: null};
  const unknown = {...kernel, owner: "sonic-swss", bazel_seconds: null, bazel_phases: 0,
    profiles: 0, fetch_seconds: null, compile_seconds: null, lock_seconds: null, foreign_seconds: null};
  const packages = Array.from({length: 36}, (_, n) => ({...image, owner: `package-${n}.deb`, bazel_seconds: n}));
  job.image_recipe_timings = {version: 2, status: "available", recipes: [unknown, image, ...packages, kernel]};
  const view = renderRecipeTimings(job), rows = descendants(view, el => el.tagName === "details");
  assert.equal(rows.length, 39);
  assert.match(rows[0].textContent, /linux-headers-amd64120sBazel \/ launcher120sBazel lock wait2.25s \(2\/3\)/);
  assert.match(rows[0].textContent, /Output handoffN\/A/);
  assert.equal(rows[0].open, true);
  assert.equal(rows[1].open, false);
  assert.match(rows[1].textContent, /package-35.deb/);
  assert.match(rows.at(-1).textContent, /sonic-swss—Bazel \/ launcher— \(0\/1\)/);
  assert.equal(job.image_recipe_timings.recipes[0], unknown);
  job.image_recipe_timings.recipes = [];
  assert.match(renderRecipeTimings(job).textContent, /No Bazel activity was recorded/);
  for (const change of [{owner: "bad/../path"}, {owner: "<script>"}, {owner: "a".repeat(193)},
                        {bazel_expected: 65536}, {profiles_expected: 1}, {exports_expected: 1}, {compile_seconds: NaN}]) {
    job.image_recipe_timings.recipes = [{...image, ...change}];
    const invalid = renderRecipeTimings(job);
    assert.match(invalid.textContent, /could not be verified/);
    assert.doesNotMatch(invalid.textContent, /\.\.\/path|<script>/);
  }
}));

function sharedContainerJob() {
  const job = recipeJob();
  const group = {...job.image_recipe_timings.images[0], owner: "vs-container-images", bazel_seconds: 200,
    bazel_phases: 1, bazel_expected: 1, profiles: 1, profiles_expected: 1,
    exports: 0, exports_expected: 0, export_seconds: null};
  const image = {...group, owner: "docker-sflow.gz", bazel_phases: 0, bazel_expected: 0,
    profiles: 0, profiles_expected: 0, bazel_seconds: null, compile_seconds: null, foreign_seconds: null,
    lock_seconds: null, fetch_seconds: null, exports: 2, exports_expected: 2, export_seconds: 4};
  job.image_recipe_timings = {version: 2, status: "available", recipes: [group, image]};
  return job;
}

test("expanded shared build lists the original per-image exports on older runs", () => withDocument(() => {
  const job = sharedContainerJob();
  const view = renderRecipeTimings(job), rows = descendants(view, node => node.tagName === "details");
  assert.match(rows[0].textContent, /vs-container-images200sBazel \/ launcher200s/);
  assert.equal(rows.length, 1);
  assert.match(rows[0].textContent, /Containers \(1\)/);
  assert.match(rows[0].textContent, /Per-container Bazel action times are unavailable/);
  assert.match(rows[0].textContent, /ImageTotal elapsedCompile timeOutput handoffdocker-sflow.gz——4s/);
}));

test("batch action times are nested, ordered by activity and distinct from subsequent exports", () => withDocument(() => {
  const job = recipeJob(), group = {...job.image_recipe_timings.images[0], owner: "vs-container-images", bazel_seconds: 126,
    bazel_expected: 3, profiles_expected: 3, exports_expected: 0, exports: 0, export_seconds: null};
  const base = {...group, bazel_expected: 0, profiles_expected: 0, bazel_phases: 0, profiles: 0, exports_expected: 2,
    bazel_seconds: null, compile_seconds: null, foreign_seconds: null, lock_seconds: null, fetch_seconds: null};
  const images = [{...base, owner: "docker-teamd.gz", exports: 2, export_seconds: 4},
    {...base, owner: "docker-syncd-vs.gz", exports: 1, export_seconds: 0}];
  const shared = {rows: images.map((row, i) => ({owner: row.owner, actions: 12, action_seconds: i ? 81 : 23,
    total_seconds: i ? 119.5 : 85, compile_seconds: i ? 8.25 : 0,
    archive_seconds: 10, compress_seconds: 7, fetch_seconds: 1.5})),
    other_action_seconds: 25, no_action_seconds: 2, parallel_seconds: 5};
  job.image_recipe_timings = {version: 2, status: "available", recipes: [group, ...images], container_breakdown: shared};
  const view = renderRecipeTimings(job), rows = descendants(view, node => node.tagName === "tr");
  assert.equal(descendants(view, node => node.tagName === "details").length, 1);
  assert.deepEqual(rows.map(tr => tr.children.map(cell => cell.textContent)), [
    ["Image", "Actions", "Total elapsed", "Active", "Compile time", "Cache fetch", "OCI archive", "Gzip", "Output handoff"],
    ["docker-syncd-vs.gz", "12", "119.5s", "81s", "8.25s", "1.5s", "10s", "7s", "0s (1/2)"],
    ["docker-teamd.gz", "12", "85s", "23s", "0s", "1.5s", "10s", "7s", "4s"]]);
  assert.match(view.textContent, /Only shared or unattributed actions active25s/);
  assert.match(view.textContent, /No action recorded2s/);
  assert.match(view.textContent, /One or more image owners active99s/);
  assert.match(view.textContent, /active together for 5s/);
  delete shared.rows[0].total_seconds;
  const containerRow = () => descendants(renderRecipeTimings(job), node => node.tagName === "tr")[2];
  assert.deepEqual(containerRow().children.slice(2, 5).map(cell => cell.textContent), ["—", "23s", "0s"]);
  delete shared.rows[0].compile_seconds;
  const legacyRow = containerRow();
  assert.deepEqual(legacyRow.children.map(cell => cell.textContent),
    ["docker-teamd.gz", "12", "—", "23s", "—", "1.5s", "10s", "7s", "4s"]);
  assert.equal(legacyRow.children[2].attributes["aria-label"], "Total elapsed unavailable");
  assert.equal(legacyRow.children[4].attributes["aria-label"], "Compile time unavailable");
  for (const invalid of [null, "85", 22.99, Infinity]) {
    shared.rows[0].total_seconds = invalid;
    assert.match(renderRecipeTimings(job).textContent, /Per-container Bazel action times are unavailable/);
  }
  delete shared.rows[0].total_seconds;
  for (const invalid of [null, "2", -1, 23.01, Infinity]) {
    shared.rows[0].compile_seconds = invalid;
    assert.match(renderRecipeTimings(job).textContent, /Per-container Bazel action times are unavailable/);
  }
  delete shared.rows[0].compile_seconds;
  shared.rows[1].action_seconds = "81";
  const fallback = renderRecipeTimings(job);
  assert.match(fallback.textContent, /Per-container Bazel action times are unavailable/);
  assert.doesNotMatch(fallback.textContent, /81s/);
}));

test("a measured zero total remains visible without compiler timing", () => withDocument(() => {
  const job = sharedContainerJob(), receipt = job.image_recipe_timings;
  const image = receipt.recipes.at(-1);
  image.export_seconds = 0;
  receipt.container_breakdown = {rows: [{owner: image.owner, actions: 1, action_seconds: 0, total_seconds: 0,
    fetch_seconds: 0, archive_seconds: 0, compress_seconds: 0}], other_action_seconds: 0, no_action_seconds: 200, parallel_seconds: 0};
  const [, imageRow] = descendants(renderRecipeTimings(job), node => node.tagName === "tr");
  assert.deepEqual(imageRow.children.slice(2, 5).map(cell => cell.textContent), ["0s", "0s", "—"]);
}));

test("unavailable or unverifiable profiles do not appear as measured recipes", () => withDocument(() => {
  const job = recipeJob();
  assert.equal(renderRecipeTimings({...job, kind: "container"}), null);
  assert.equal(renderRecipeTimings({...job, steps: [{name: "Build the VS image"}]}), null);
  assert.match(renderRecipeTimings({...job, image_recipe_timings: undefined}).textContent, /No Bazel timing breakdown/);
  const unavailable = {version: 1, status: "unavailable", images: [], reason: "pending-collection"};
  assert.match(renderRecipeTimings({...job, image_recipe_timings: unavailable}).textContent, /pending collection/);
  for (const change of [{profiles: 0}, {bazel_seconds: "0"}, {compile_seconds: -1}, {exports: 3}, {failed: 0},
                        {owner: "docker-mux.gz"}, {owner: "<img>"}]) {
    const candidate = structuredClone(job);
    Object.assign(candidate.image_recipe_timings.images[0], change);
    const view = renderRecipeTimings(candidate);
    assert.match(view.textContent, /could not be verified/);
    assert.equal(descendants(view, el => el.tagName === "details").length, 0);
    assert.doesNotMatch(view.textContent, /<img>/);
  }
}));

test("per-target explanations rank build-only time and keep query locks and historical gaps separate", () => withDocument(() => {
  const job = recipeJob();
  const build = {expected: 1, timings: 1, profiles: 1, seconds: 30, lock_seconds: 5,
    compile_seconds: 0, foreign_seconds: 0, fetch_seconds: 2, repository_seconds: 12,
    repository_read_seconds: 10, packaging_seconds: 8, cache_write_seconds: 4,
    package_selection_seconds: 3, validation_seconds: 1,
    arm64_repository_seconds: 3, armhf_repository_seconds: 0};
  const base = {...job.image_recipe_timings.images[0], bazel_expected: 3, profiles_expected: 3, exports_expected: 2};
  const repo = {...base, owner: "docker-repo.gz", build, lock_seconds: 105};
  const fast = {...base, owner: "docker-fast.gz", build: {...build, seconds: 7}};
  const kernel = {...base, owner: "linux-kernel", build: null, bazel_seconds: 10, bazel_phases: 1, bazel_expected: 1};
  job.image_recipe_timings = {version: 2, status: "available", recipes: [fast, kernel, repo]};
  const section = renderBuildExplanations(recipeRows(job.image_recipe_timings));
  const rows = descendants(section, node => node.className === "target-card");
  assert.match(section.textContent, /Why each target took its time/);
  assert.match(rows[0].textContent, /docker-repo.gzRepository preparation \+ image packaging25sBuild excluding lock/);
  assert.match(rows[0].textContent, /Largest recorded work category: Repository preparation/);
  assert.match(rows[0].textContent, /Repository preparation12s \(12.00s\)/);
  assert.match(rows[0].textContent, /Repository input reads\/waits10s/);
  assert.match(rows[0].textContent, /ARM64 package repository preparation3s/);
  assert.match(rows[0].textContent, /APT selection critical path3s/);
  assert.match(rows[0].textContent, /Other-command lock wait: 100s/);
  assert.match(rows[1].textContent, /linux-kernel \*Isolated kernel launcher10sLauncher elapsed/);
  assert.match(rows[1].textContent, /Separate kernel launcher/);
  assert.match(rows[2].textContent, /docker-fast.gzRepository preparation \+ image packaging2sBuild excluding lock/);
  assert.match(rows[2].textContent, /Most build-command elapsed time was server lock waiting/);
  const buttons = descendants(section, node => node.tagName === "button");
  buttons[0].listeners.click();
  assert.ok(rows.every(row => row.open));
  buttons[1].listeners.click();
  assert.ok(rows.every(row => !row.open));
  assert.match(section.textContent, /overlap and must not be added/);
  assert.match(renderBuildExplanations(recipeRows(recipeJob().image_recipe_timings)).textContent, /Detailed build-only evidence was not recorded/);
  repo.build = {...build, profiles: 0, ...Object.fromEntries(Object.keys(build).filter(key => key.endsWith("_seconds")).map(key => [key, null]))};
  const partial = renderBuildExplanations(recipeRows(job.image_recipe_timings)).textContent;
  assert.match(partial, /Build profile unavailable/);
  for (const change of [{repository: "<script>"}, {expected: true}, {repository_seconds: Infinity},
                        {lock_seconds: 31}, {repository_seconds: "12"}]) {
    repo.build = {...build, ...change};
    assert.match(renderRecipeTimings(job).textContent, /could not be verified/);
  }
}));

test("the recipe evidence link opens this job's successful timing artifact upload", () => withDocument(() => {
  const repository = "securely1g/sonic-buildimage", job = recipeJob();
  job.url = `https://github.com/${repository}/actions/runs/123/job/456`;
  const upload = {name: "Preserve VS image build profiles, timings and Make logs", conclusion: "success", number: 16};
  job.steps.push(upload);
  const links = () => descendants(renderRecipeTimings(job, repository), node => node.tagName === "a");
  assert.equal(links()[0].href, job.url + "#step:16:1");
  assert.match(links()[0].textContent, /Profiles and Make logs/);
  job.steps[1] = {...upload, name: "Preserve VS Bazel profiles, timings and Make logs"};
  assert.equal(links()[0].href, job.url + "#step:16:1");
  for (const change of [{number: 0}, {number: "16"}, {name: "A different artifact"}, {conclusion: "failure"}]) {
    job.steps[1] = {...upload, ...change};
    assert.equal(links().length, 0);
  }
  job.steps[1] = upload;
  job.url = "https://github.com/unrelated/repo/actions/runs/123/job/456";
  assert.equal(links().length, 0);
}));

test("profile cards show named spans and receipts without accepting raw private action text", () => withDocument(() => {
  const job = recipeJob(), base = job.image_recipe_timings.images[0];
  const row = {...base, owner: "linux-kernel", build: null, bazel_expected: 1, bazel_phases: 1,
    bazel_seconds: 183.843, profiles_expected: 3, exports: 0, exports_expected: 0, export_seconds: null,
    details: {scope: "kernel-build", expected: 1, profiles: 1, phase_count: 1,
      repositories: [{name: "//:apt+example", seconds: 12, read_seconds: 10, read_count: 2}],
      critical_path: [{kind: "OCIImage", target: "//example:image", seconds: 8}],
      phases: [{phase: "launcher", seconds: 183.843, lock_seconds: null, start_epoch_seconds: 1794944781.5, exit_code: 0}]}};
  const value = {version: 2, status: "available", recipes: [row]};
  job.image_recipe_timings = value;
  const section = renderBuildExplanations(recipeRows(value)), card = descendants(section, node => node.className === "target-card")[0];
  assert.match(card.textContent, /3m04sLauncher elapsed/);
  assert.match(card.textContent, /Longest repository spansRepositorySpanNested reads\/waits\/\/:apt\+example12s \(12.00s\)2 reads; 10s/);
  assert.match(card.textContent, /OCI image\/\/example:image8s/);
  assert.match(card.textContent, /2026-11-17 19:46:21.500Z/);
  assert.match(card.textContent, /Kernel spans describe its build profile only, not the whole launcher/);
  assert.match(card.textContent, /Command receipts \(1\/1 shown\)/);
  assert.equal(card.open, undefined, "Cards are collapsed initially");
  const original = structuredClone(row.details);
  for (const [group, edits] of [
    ["repositories", {name: "/home/private"}], ["repositories", {name: "//:example\n"}],
    ["repositories", {read_seconds: 13}], ["repositories", {command: "--token=never-publish"}],
    ["critical_path", {kind: "Unknown command"}], ["critical_path", {target: "/home/private"}],
    ["critical_path", {target: "platform/../private"}], ["critical_path", {target: "//example:../private"}],
    ["critical_path", {target: "//..:image"}], ["critical_path", {target: "//example:image\n"}],
    ["phases", {phase: "build --token=x"}], ["phases", {start_epoch_seconds: 253402300800}],
  ]) {
    row.details = structuredClone(original);
    Object.assign(row.details[group][0], edits);
    assert.equal(recipeRows(value), null);
    assert.match(renderRecipeTimings(job).textContent, /could not be verified/);
  }
  row.details = {...original, repositories: Array(9).fill(original.repositories[0])};
  assert.equal(recipeRows(value), null);
  row.details = {...original, profiles: 0};
  assert.equal(recipeRows(value), null);
}));

// Measured boundaries of the master VS installer build in run 44638292829.
function nativeInstallerStep() {
  const tasks = [
    {id: "vs-container-images-1", offset_seconds: 2023.544782, duration_seconds: 2817.313521},
    {id: "rootfs-1", offset_seconds: 5441.178008, duration_seconds: 243.355476},
    {id: "final-image-1", offset_seconds: 5712.876499, duration_seconds: 920.387027},
  ].map(task => ({...task, outcome: "finished"}));
  const section = {id: "vs-image-1", duration_seconds: 6929.801178, offset_seconds: 0, tasks};
  return {...stepWith({version: 1, status: "available", source: "github-job-log", duration_seconds: section.duration_seconds, sections: [section]}),
    name: "Build sonic-vs.bin through the host Make wrapper", duration_seconds: 6930};
}

const installerStageNames = [
  "Prepare kernel, packages and P4RT",
  "Build and hand off Bazel container images",
  "Finish Make containers and stage the root filesystem",
  "Assemble the final ONIE installer",
  "Finish the Make invocation",
];

function installerStages(view) {
  return descendants(view, node => node.tagName === "details" && node.children[0]?.tagName === "summary" &&
    installerStageNames.some(label => node.children[0].textContent.includes(label)));
}

function installerPoint(step) {
  const point = datedPoint(1, "2026-11-01T00:00:00Z", step.name);
  const completed = new Date(Date.parse(point.run.started_at) + step.duration_seconds * 1000).toISOString();
  Object.assign(point.job.steps[0], step, {completed_at: completed});
  Object.assign(point.job, {conclusion: step.conclusion, completed_at: completed, duration_seconds: step.duration_seconds,
    image_build_seconds: step.conclusion === "success" ? step.duration_seconds : null});
  Object.assign(point.run, {conclusion: step.conclusion, completed_at: completed});
  return point;
}

test("historical native installer boundaries account for all five intervals without inventing child timings", () => withDocument(() => {
  const step = nativeInstallerStep(), view = renderStep(step), stages = installerStages(view);
  const [batch, , installer] = step.breakdown.sections[0].tasks;
  const boundaries = [0, batch.offset_seconds, batch.offset_seconds + batch.duration_seconds,
    installer.offset_seconds, installer.offset_seconds + installer.duration_seconds, step.breakdown.duration_seconds];
  assert.equal(stages.length, installerStageNames.length);
  stages.forEach((stage, index) => {
    const [summary, ...body] = stage.children;
    assert.ok(summary.textContent.includes(installerStageNames[index]));
    assert.ok(summary.textContent.includes(duration(boundaries[index + 1] - boundaries[index])));
    assert.ok(body.some(child => child.textContent.trim()), "every stage should explain its measurements when opened");
  });
  for (const index of [0, 4]) assert.match(stages[index].textContent, /no .*timings recorded/i);
  assert.match(stages[2].textContent, /Root filesystem squashfs/);
  assert.match(stages[3].textContent, /not measured|not recorded|unavailable/i);
  assert.doesNotMatch(view.textContent, /Collect build-version metadata|P4RT source build|Compress the intermediate squashfs/);
}));

test("the container stage owns its recipe and table while independent recipes stay at job level", async () => {
  const point = installerPoint(nativeInstallerStep());
  const receipt = sharedContainerJob().image_recipe_timings;
  point.job.image_recipe_timings = receipt;
  receipt.recipes.push({...receipt.recipes[0], owner: "linux-kernel"});
  const {details} = await loadDashboard([point]);
  const overview = details.children.find(section => section.children.some(node =>
    /^h[1-6]$/.test(node.tagName) && node.textContent === "Image build steps"));
  assert.ok(overview, "the stage list should open directly under Run details");
  const stages = installerStages(overview);
  assert.equal(stages.length, 5);
  assert.equal(installerStages(details).length, 5, "the raw CI steps should not repeat the same five stages");
  assert.match(stages[1].textContent, /Shared Bazel command/);
  assert.match(stages[1].textContent, /Output handoffs by container \(1\)docker-sflow\.gz/);
  const tables = view => descendants(view, node => node.attributes?.["aria-label"] === "Container timings");
  assert.equal(tables(details).length, 1);
  assert.equal(tables(stages[1]).length, 1);
  assert.match(stages[1].textContent, /vs-container-images200s.*Containers \(1\)/);
  assert.doesNotMatch(stages[1].textContent, /linux-kernel/);
  const bazel = details.children.find(section => section.className === "recipe-timings");
  assert.match(bazel.textContent, /Bazel build breakdown.*linux-kernel/);
  assert.doesNotMatch(bazel.textContent, /vs-container-images|docker-sflow/);
});

test("kernel preparation shows measured wall time and partial Bazel receipts in separate columns", async () => {
  const step = nativeInstallerStep();
  step.breakdown.sections[0].tasks.push({id: "kernel-prep-1", offset_seconds: 20, duration_seconds: 153.118, outcome: "finished"});
  const point = installerPoint(step), receipt = sharedContainerJob().image_recipe_timings;
  receipt.recipes.push({...receipt.recipes[0], owner: "linux-kernel", bazel_seconds: 135.521, exports: 1,
    exports_expected: 1, export_seconds: 4.836, profiles: 1, profiles_expected: 3, compile_seconds: 0,
    lock_seconds: 0, foreign_seconds: 0, fetch_seconds: 30.704});
  point.job.image_recipe_timings = receipt;
  const {details} = await loadDashboard([point]);
  const [preparation, containers] = installerStages(details);
  const matches = node => node.attributes?.["aria-label"] === "Kernel Bazel breakdown";
  assert.equal(descendants(details, matches).length, 1);
  const [table] = descendants(preparation, matches);
  assert.equal(table.tabIndex, 0);
  const [header, data] = descendants(table, node => node.tagName === "tr");
  assert.deepEqual(header.children.map(c => c.textContent), ["Target", "Total elapsed", "Recorded command time",
    "Bazel launcher", "Lock wait", "Compile time", "Nested build", "Cache fetch", "Stage DEBs"]);
  assert.deepEqual(data.children.map(c => c.textContent), ["linux-kernel", "153.118s", "140.357s",
    "135.521s", "0s (1/3)", "0s (1/3)", "0s (1/3)", "30.704s (1/3)", "4.836s"]);
  assert.doesNotMatch(containers.textContent, /linux-kernel/);
  assert.doesNotMatch(details.textContent, /Bazel build breakdown/);
});

test("ambiguous kernel preparation or repeated commands keep their timings at job level", async () => {
  const task = {id: "kernel-prep-1", offset_seconds: 1, duration_seconds: 120, outcome: "finished"};
  for (const [tasks, launches] of [
    [[], 1], [[task, {...task, id: "kernel-prep-2"}], 1],
    [[{...task, offset_seconds: 2020}], 1], [[{...task, outcome: "cached"}], 1], [[task], 2],
  ]) {
    const step = nativeInstallerStep();
    step.breakdown.sections[0].tasks.push(...tasks);
    const point = installerPoint(step), receipt = sharedContainerJob().image_recipe_timings;
    receipt.recipes.push({...receipt.recipes[0], owner: "linux-kernel", bazel_expected: launches});
    point.job.image_recipe_timings = receipt;
    const {details} = await loadDashboard([point]);
    assert.doesNotMatch(installerStages(details)[0].textContent, /Kernel Bazel breakdown/);
    assert.match(details.textContent, /Bazel build breakdown.*linux-kernel/);
  }
});

test("a batch that cannot be matched to one stage keeps its job-level container table", async () => {
  const step = nativeInstallerStep(), point = installerPoint(step);
  point.job.image_recipe_timings = sharedContainerJob().image_recipe_timings;
  point.job.image_recipe_timings.recipes[0].bazel_expected = 2;
  const {details} = await loadDashboard([point]);
  assert.doesNotMatch(installerStages(details)[1].textContent, /Containers \(1\)/);
  assert.match(details.textContent, /Bazel build breakdown.*Containers \(1\)/);
});

test("Run details keeps raw CI evidence for an untimed legacy build and a failed installer step", async () => {
  for (const outcome of ["success", "failure"]) {
    const step = nativeInstallerStep();
    if (outcome === "success") delete step.breakdown;
    step.conclusion = outcome;
    const {details} = await loadDashboard([installerPoint(step)]);
    assert.equal(installerStages(details).length, 0);
    const raw = descendants(details, node => node.tagName === "details" && node.children[0]?.textContent.startsWith("Step timings"))[0];
    assert.ok(raw.textContent.includes(step.name));
    assert.ok(raw.textContent.includes(duration(step.duration_seconds)));
    if (outcome === "failure") assert.match(raw.textContent, /Root filesystem squashfs/);
  }
});

test("incomplete, repeated or overlapping native boundary pairs preserve the underlying spans", () => withDocument(() => {
  const cases = [
    step => { step.name = "Build the VS image"; }, step => { step.conclusion = "failure"; },
    (_, tasks) => { tasks.shift(); }, (_, tasks) => { tasks.pop(); },
    (_, tasks) => { tasks.push({...tasks[0], id: "vs-container-images-2"}); },
    (_, tasks) => { tasks.push({...tasks[2], id: "final-image-2"}); },
    (_, tasks) => { tasks[0].offset_seconds = tasks[2].offset_seconds - tasks[0].duration_seconds + 1; },
    (_, tasks) => { tasks[2].outcome = "cached"; },
  ];
  for (const change of cases) {
    const step = nativeInstallerStep();
    change(step, step.breakdown.sections[0].tasks);
    const view = renderStep(step);
    assert.equal(installerStages(view).length, 0);
    assert.match(view.textContent, /Root filesystem squashfs/);
  }
}));

test("native preparation substeps, ONIE Docker phases and unmarked cleanup time retain their scope", () => withDocument(() => {
  const step = nativeInstallerStep(), tasks = step.breakdown.sections[0].tasks;
  const installer = tasks.find(task => task.id === "final-image-1");
  const tailStart = installer.offset_seconds + installer.duration_seconds;
  const task = (kind, offset_seconds, duration_seconds) => ({id: kind + "-1", offset_seconds, duration_seconds, outcome: "finished"});
  tasks.push(task("kernel-prep", 60, 155), task("p4rt-prep", 440, 1300), task("p4rt-owner", 480, 1000),
    task("rootfs-prepare", 5442, 180), task("rootfs-compress", 5622, 25), task("rootfs-cache-save", 5649, 30),
    task("build-version-collection", tailStart + 3, 2), task("builder-exit", tailStart + 5, 280),
    task("trixie-image-cleanup", tailStart + 285, 1), task("bookworm-cleanup", tailStart + 286, 3));
  installer.phase_breakdown = {version: 1, source: "build-phase-markers", status: "available", reason: null,
    duration_seconds: installer.duration_seconds, segments: [
      {phase: "assembly", context: "docker-load", offset_seconds: 0, duration_seconds: 200},
      {phase: "compression", context: "docker-store", offset_seconds: 200, duration_seconds: 110},
      {phase: "compression", context: "payload", offset_seconds: 310, duration_seconds: installer.duration_seconds - 310},
    ]};
  const view = renderStep(step);
  for (const label of ["Kernel package preparation", "P4RT source and SDK preparation", "P4RT source build",
    "Collect build-version metadata", "Exit the builder and return to the host",
    "Trixie image cleanup", "Bookworm cleanup invocation",
    "Assembly · Load and tag Docker images", "Compression · Compress the Docker store", "Compression · Build the payload ZIP"]) {
    assert.ok(view.textContent.includes(label), label);
  }
  const intervals = installerStages(view);
  const nestedTasks = (stage, parent) => descendants(stage, node => node.tagName === "li" &&
    node.children[0]?.children?.[0]?.textContent === parent)[0].children.find(child => child.tagName === "ul");
  assert.match(nestedTasks(intervals[0], "P4RT source and SDK preparation").textContent, /P4RT source build/);
  const rootfs = nestedTasks(intervals[2], "Root filesystem squashfs");
  for (const label of ["Prepare the stage-one root filesystem", "Compress the intermediate squashfs", "Save the intermediate squashfs to cache"]) {
    assert.ok(rootfs.textContent.includes(label), label);
  }
  assert.match(intervals[3].textContent, /Assembly · Load and tag Docker images/);
  assert.match(intervals[4].textContent, /Trixie image cleanup/);
  const cleanup = tasks.filter(task => task.offset_seconds >= tailStart);
  const total = step.breakdown.duration_seconds - tailStart;
  const gap = total - cleanup.reduce((sum, task) => sum + task.duration_seconds, 0);
  assert.ok(intervals[4].textContent.includes(`Unattributed wall time: ${duration(gap)}`));
  tasks.find(task => task.id === "builder-exit-1").offset_seconds -= 1;
  const overlappingTail = installerStages(renderStep(step))[4];
  assert.ok(overlappingTail.textContent.includes(`Unattributed wall time: ${duration(gap + 1)}`));
}));

test("preparation groups show active wall time for each category, including parallel and subsecond work", () => withDocument(() => {
  const step = nativeInstallerStep(), tasks = step.breakdown.sections[0].tasks;
  const batchStart = tasks[0].offset_seconds;
  for (const [id, offset_seconds, duration_seconds] of [
    ["bookworm-make-1", 0, 4], ["trixie-cache-eval-1", 5, 4],
    ["prep-deb-1", 12, 4], ["prep-deb-2", 14, 5], ["prep-deb-3", 24, 1],
    ["prep-deb-install-1", 18, 2], ["prep-wheel-1", 27, 0.31], ["prep-wheel-install-1", 30, 3],
    ["container-manifest-1", batchStart - 6, 9], ["container-manifest-2", batchStart - 3, 2],
  ]) tasks.push({id, offset_seconds, duration_seconds, outcome: "finished"});
  const [prep, batch] = installerStages(renderStep(step));
  const rows = descendants(prep, node => node.className === "interval-task").map(row => row.textContent);
  assert.deepEqual(rows, [
    "Bookworm Make invocation4s", "Evaluate Trixie package cache4s",
    "Build or reuse Debian packages8s3 measured invocations", "Install Debian packages2s1 measured invocation",
    "Build or reuse Python wheels0.31s1 measured invocation", "Install Python wheels3s1 measured invocation",
    "Generate container manifests6s2 measured invocations",
  ]);
  assert.ok(prep.textContent.includes(`Unattributed wall time: ${duration(batchStart - 26.31)}`));
  assert.match(batch.textContent, /Generate container manifests3s1 measured invocation/);
}));

test("a task crossing the start of the container batch contributes only its portion to each stage", () => withDocument(() => {
  const step = nativeInstallerStep(), tasks = step.breakdown.sections[0].tasks;
  const start = tasks.find(task => task.id === "vs-container-images-1").offset_seconds;
  tasks.push({id: "kernel-prep-1", offset_seconds: start - 3, duration_seconds: 8, outcome: "finished"});
  const stages = installerStages(renderStep(step));
  assert.match(stages[0].textContent, /Kernel package preparation3s.*8s task/);
  assert.match(stages[1].textContent, /Kernel package preparation5s.*8s task/);
  assert.ok(stages[0].textContent.includes(`Unattributed wall time: ${duration(start - 3)}`));
}));

test("legacy snapshots keep their step duration without requiring a breakdown", () => withDocument(() => {
  const step = stepWith(undefined);
  const snapshot = {schema_version: 1, repository: "securely1g/sonic-buildimage", pull_requests: [], runs: [{run_id: 1, attempt: 1, pr_numbers: [], jobs: [{steps: [step]}]}]};
  assert.equal(validateSnapshot(snapshot), snapshot);
  assert.equal(normalizeBreakdown(undefined), null);
  const row = renderStep(step);
  assert.equal(descendants(row, node => node.tagName === "details").length, 0);
  assert.match(row.textContent, /Make VS image.*success.*1m 41s/);
}));

test("unavailable or malformed details do not turn unknown timing into zero", () => withDocument(() => {
  const unavailable = {version: 1, source: "github-job-log", status: "unavailable", duration_seconds: null, sections: []};
  for (const reason of ["logs-expired", "pending-collection", "logs-temporarily-unavailable", "incomplete-log", "<img src=x>", {toString: null}]) {
    const row = renderStep(stepWith({...unavailable, reason}));
    assert.match(row.textContent, /Breakdown unavailable/);
    assert.match(row.textContent, /1m 41s/);
    assert.doesNotMatch(row.textContent, /0s|<img/);
  }
  const expired = renderStep(stepWith({...unavailable, reason: "logs-expired"}));
  assert.match(expired.textContent, /no longer available from GitHub/);
  const invalid = renderStep(stepWith({version: 9000}));
  assert.match(invalid.textContent, /incomplete or unsupported/);
  assert.match(invalid.textContent, /1m 41s/);
}));

test("nested disclosures use safe labels and mark overlapping and unmeasured work", () => withDocument(() => {
  const source = exampleBreakdown();
  source.sections[0].label = "<img src=x onerror=alert(1)>";
  source.sections[0].tasks[0].label = "raw untrusted log";
  const row = renderStep(stepWith(source));
  const disclosures = descendants(row, node => node.tagName === "details");
  assert.equal(disclosures.length, 4, "one step, two section, and one final-image disclosure");
  for (const disclosure of disclosures) assert.equal(disclosure.children[0].tagName, "summary", "native disclosure supports keyboard and expanded state");
  assert.match(row.textContent, /sections run sequentially/);
  assert.match(row.textContent, /Task spans can overlap; do not add their durations/);
  assert.match(row.textContent, /Inner phases were not measured in this run/);
  assert.match(row.textContent, /does not provide separate assembly, boot checks or compression timings/);
  assert.doesNotMatch(row.textContent, /<img|raw untrusted log|cache.hit ratio/i);
  const spans = descendants(row, node => node.className === "task-span");
  assert.equal(spans[0].style.marginLeft, "0%");
  assert.equal(spans[1].style.marginLeft, `${10 / 60 * 100}%`);
  assert.equal(spans[1].style.width, `${40 / 60 * 100}%`);
  for (const span of spans) {
    assert.ok(Number.parseFloat(span.style.marginLeft) >= 0);
    assert.ok(Number.parseFloat(span.style.width) <= 100);
  }
}));

test("a section without measured child tasks is a readable row, not an empty disclosure", () => withDocument(() => {
  const source = exampleBreakdown();
  source.sections[0].tasks = [];
  source.sections[1].tasks = [];
  const row = renderStep(stepWith(source));
  assert.equal(descendants(row, node => node.tagName === "details").length, 1);
  assert.match(row.textContent, /no measured task detail/);
}));

function examplePhases() {
  let offset = 0;
  const segments = [
    ["assembly", "recipe", 5], ["compression", "filesystem", 2],
    ["assembly", "installer", 3], ["compression", "installer", 1],
    ["boot-checks", "bios", 7], ["compression", "bios", 2],
    ["boot-checks", "uefi", 7], ["compression", "uefi", 2], ["assembly", "recipe", 1],
  ].map(([phase, context, duration_seconds]) => {
    const segment = {phase, context, offset_seconds: offset, duration_seconds};
    offset += duration_seconds;
    return segment;
  });
  return {version: 1, status: "available", reason: null, source: "build-phase-markers", duration_seconds: offset, segments};
}
function breakdownWithPhases(phases, outcome = "finished") {
  const outer = exampleBreakdown();
  const final = outer.sections[0].tasks.find(task => task.id === "final-image-1");
  final.phase_breakdown = phases;
  final.outcome = outcome;
  return outer;
}

test("phase totals combine disjoint occurrences without merging their execution order", () => {
  const source = examplePhases(), before = structuredClone(source);
  const parsed = normalizePhaseBreakdown(source, 30, "finished");
  assert.equal(parsed.status, "available");
  assert.deepEqual(parsed.totals, {assembly: 9, "boot-checks": 14, compression: 7});
  assert.deepEqual(parsed.segments, source.segments);
  assert.equal(parsed.segments.length, 9);
  assert.deepEqual(source, before);
  const outer = normalizeBreakdown(breakdownWithPhases(source));
  assert.equal(outer.status, "available");
  assert.equal(outer.duration_seconds, 100);
  assert.equal(outer.sections[0].tasks.length, 3);
  assert.equal(outer.sections[0].tasks[2].duration_seconds, 30);
  assert.equal(outer.sections[0].tasks[2].phase_breakdown.duration_seconds, 30);
});

test("phase clocks tolerate up to two seconds without changing parent or segment measurements", () => {
  for (const parent of [28, 30, 32]) {
    const parsed = normalizePhaseBreakdown(examplePhases(), parent, "finished");
    assert.equal(parsed.status, "available");
    assert.equal(parsed.duration_seconds, 30);
    assert.equal(parsed.segments[8].offset_seconds, 29);
    assert.equal(parsed.segments[8].duration_seconds, 1);
  }
  for (const parent of [27.999, 32.001, null, "30", Infinity, NaN, Number.MAX_VALUE]) assert.equal(normalizePhaseBreakdown(examplePhases(), parent, "finished").status, "unavailable");
  assert.equal(normalizePhaseBreakdown(examplePhases(), 300, "finished").reason, "invalid-timing", "an internally complete but too-short summary cannot represent the whole recipe");
});

test("missing phase categories are not manufactured zeroes, but measured zero is retained", () => {
  const value = examplePhases();
  value.duration_seconds = 0;
  value.segments = [{phase: "assembly", context: "recipe", offset_seconds: 0, duration_seconds: 0}];
  const parsed = normalizePhaseBreakdown(value, 0, "finished");
  assert.equal(parsed.status, "available");
  assert.deepEqual(parsed.totals, {assembly: 0, "boot-checks": null, compression: null});
  value.segments.push({...value.segments[0]});
  assert.equal(normalizePhaseBreakdown(value, 0, "finished").status, "unavailable", "identical duplicate zero-length records are rejected");
});

test("invalid or partial phase data cannot discard the measured outer recipe", () => {
  const mutate = change => { const value = examplePhases(); change(value); return value; };
  const malformed = [
    null, {}, [], "raw log", {...examplePhases(), version: 2}, {...examplePhases(), source: "untrusted"},
    {...examplePhases(), reason: "not-recorded"}, {...examplePhases(), segments: null}, {...examplePhases(), segments: []},
    ...[null, "30", -1, Infinity, NaN, Number.MAX_VALUE].map(duration_seconds => ({...examplePhases(), duration_seconds})),
    mutate(value => { value.segments[0] = null; }),
    mutate(value => { value.segments[0].phase = "<img src=x>"; }),
    mutate(value => { value.segments[0].context = {toString: null}; }),
    mutate(value => { value.segments[0].phase = "__proto__"; }),
    mutate(value => { value.segments[0].offset_seconds = 1; }),
    mutate(value => { value.segments[1].offset_seconds = 4; }),
    mutate(value => { value.segments[1].offset_seconds = 6; }),
    mutate(value => { value.segments[1].offset_seconds = Infinity; }),
    mutate(value => { value.segments[1].duration_seconds = NaN; }),
    mutate(value => { value.segments[1].duration_seconds = -1; }),
    mutate(value => { value.segments[1].duration_seconds = "2"; }),
    mutate(value => { value.segments[8].duration_seconds = 2; }),
    mutate(value => { value.segments.pop(); }),
    mutate(value => { value.segments[1] = {...value.segments[0]}; }),
    mutate(value => { value.duration_seconds = 65; value.segments = Array.from({length: 65}, (_, i) => ({phase: "assembly", context: "recipe", offset_seconds: i, duration_seconds: 1})); }),
  ];
  for (const value of malformed) {
    const outer = normalizeBreakdown(breakdownWithPhases(value));
    assert.equal(outer.status, "available");
    const final = outer.sections[0].tasks[2];
    assert.equal(final.duration_seconds, 30);
    assert.equal(final.phase_breakdown.status, "unavailable");
    assert.equal(final.phase_breakdown.totals, undefined);
    assert.equal(final.phase_breakdown.segments, undefined);
  }
});

test("phase segment bounds and totals are checked at microsecond precision", () => {
  const accepted = examplePhases();
  accepted.duration_seconds += .000001;
  assert.equal(normalizePhaseBreakdown(accepted, 30, "finished").status, "available");
  const gap = examplePhases(); gap.segments[1].offset_seconds += .00002;
  assert.equal(normalizePhaseBreakdown(gap, 30, "finished").status, "unavailable");
  const drift = examplePhases();
  drift.segments = Array.from({length: 10}, (_, i) => ({phase: "assembly", context: "recipe", offset_seconds: i * .000009, duration_seconds: 0}));
  drift.duration_seconds = .000081;
  assert.equal(normalizePhaseBreakdown(drift, 0, "finished").status, "unavailable", "many tiny gaps cannot invent measured duration");
  const contexts = ["recipe", "filesystem", "installer", "bios", "uefi", "4-asic", "6-asic"];
  const maximum = {...examplePhases(), duration_seconds: 64, segments: Array.from({length: 64}, (_, i) => ({phase: "assembly", context: contexts[i % contexts.length], offset_seconds: i, duration_seconds: 1}))};
  assert.equal(normalizePhaseBreakdown(maximum, 64, "finished").segments.length, 64);
  maximum.segments.push({phase: "assembly", context: "recipe", offset_seconds: 64, duration_seconds: 0});
  assert.equal(normalizePhaseBreakdown(maximum, 64, "finished").status, "unavailable");
});

test("historical and cached recipes explicitly keep unknown inner timings", () => withDocument(() => {
  for (const outcome of ["finished", "cached"]) {
    const source = breakdownWithPhases(undefined, outcome);
    const row = renderStep(stepWith(source));
    const phaseDetails = descendants(row, node => node.className === "phase-breakdown")[0];
    assert.equal(phaseDetails.children[0].tagName, "summary");
    assert.match(phaseDetails.textContent, /Inner phases were not measured in this run/);
    assert.equal(descendants(phaseDetails, node => node.className === "phase-totals").length, 0);
    assert.match(phaseDetails.textContent, /30s/);
  }
  const cached = normalizeBreakdown(breakdownWithPhases(examplePhases(), "cached"));
  assert.equal(cached.status, "available");
  assert.equal(cached.sections[0].tasks[2].phase_breakdown.status, "unavailable");
}));

test("phase rendering exposes all compression contexts and preserves native disclosures", () => withDocument(() => {
  const source = examplePhases();
  source.label = "<script>";
  for (const segment of source.segments) segment.label = "raw untrusted label";
  const row = renderStep(stepWith(breakdownWithPhases(source)));
  const phaseDetails = descendants(row, node => node.className === "phase-breakdown")[0];
  assert.equal(phaseDetails.children[0].tagName, "summary");
  const totals = descendants(phaseDetails, node => node.className === "phase-total");
  assert.deepEqual(totals.map(node => node.textContent), ["Assembly9s", "Boot checks14s", "Compression7s"]);
  const segments = descendants(phaseDetails, node => node.className === "phase-segment");
  assert.equal(segments.length, 9);
  for (const label of ["Compression · Filesystem and payload", "Compression · ONIE installer", "Compression · BIOS", "Compression · UEFI"]) assert.ok(phaseDetails.textContent.includes(label));
  assert.match(phaseDetails.textContent, /build process clock/);
  assert.match(phaseDetails.textContent, /differences are preserved, not rescaled/);
  assert.doesNotMatch(phaseDetails.textContent, /<script>|raw untrusted label|not measured/);
  assert.equal(descendants(phaseDetails, node => node.className === "task-track").length, 1, "only the outer task span has a bar; repeated category segments are not merged into one bar");
}));

test("phase-unavailable messages come only from fixed reasons", () => withDocument(() => {
  for (const reason of ["not-recorded", "incomplete-timing", "invalid-timing", "<img src=x>", {toString: null}]) {
    const value = {version: 1, status: "unavailable", reason, source: "build-phase-markers", duration_seconds: null, segments: []};
    const row = renderStep(stepWith(breakdownWithPhases(value)));
    assert.doesNotMatch(row.textContent, /<img/);
    assert.equal(descendants(row, node => node.className === "phase-total").length, 0);
    assert.match(row.textContent, /30s/);
  }
}));

test("batch receipts show measured commands from the selected host step and preserve zero and partial export counts", () => {
  const {containerBatchMeasurements} = require("./app.js");
  const job = sharedContainerJob(), [step] = job.steps;
  step.conclusion = "success";
  const [batch, archive] = job.image_recipe_timings.recipes;
  const paired = {...archive, owner: "docker-syncd-vs.gz", exports: 1, exports_expected: 2, export_seconds: 0};
  const independent = {...archive, owner: "docker-independent.gz", bazel_expected: 1};
  job.image_recipe_timings.recipes.push(paired, independent);
  assert.deepEqual(containerBatchMeasurements(job, step), {bazelSeconds: 200, exports: [
    ["docker-sflow.gz (2 commands)", 4],
    ["docker-syncd-vs.gz (1 of 2 recorded commands)", 0],
  ]});
  assert.deepEqual(containerBatchMeasurements(job, {...step}), null);
  job.steps.push({...step});
  assert.deepEqual(containerBatchMeasurements(job, step), null);
  job.steps.pop();
  batch.bazel_expected = 2;
  assert.deepEqual(containerBatchMeasurements(job, step), null);
  batch.bazel_expected = 1;
  archive.exports = 0;
  assert.deepEqual(containerBatchMeasurements(job, step), null, "reject inconsistent receipt rather than imputing a duration");
  assert.deepEqual(containerBatchMeasurements({kind: "make-vs", steps: [step]}, step), null);
});
