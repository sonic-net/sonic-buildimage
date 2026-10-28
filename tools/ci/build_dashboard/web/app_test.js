"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");
const {duration, measured, imageResult, prCoverage, summarizePrCoverage, safeGithubUrl, validateSnapshot, normalizeBreakdown, normalizePhaseBreakdown, renderStep} = require("./app.js");

test("unmeasured and invalid durations never look like fast builds", () => {
  for (const value of [null, undefined, "12", -1, NaN, Infinity]) assert.equal(duration(value), "—");
  assert.equal(duration(0), "0s");
  assert.equal(duration(125), "2m 5s");
  assert.equal(duration(3660), "1h 1m");
});

test("only successful recognized image jobs enter the performance chart", () => {
  const job = {kind: "make-vs", conclusion: "success", image_build_seconds: 120};
  assert.equal(measured(job), true);
  assert.equal(measured({...job, kind: "bazel-vs"}), true);
  for (const conclusion of ["failure", "skipped", "cancelled", null]) assert.equal(measured({...job, conclusion}), false);
  for (const kind of ["container", "source", "checks", "other"]) assert.equal(measured({...job, kind}), false);
  assert.equal(measured({...job, image_build_seconds: null}), false);
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
  constructor(tag) { this.tagName = tag; this.children = []; this.style = {}; this.attributes = {}; this.text = ""; }
  append(...children) { this.children.push(...children); }
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
function stepWith(breakdown) { return {name: "Make VS image", status: "completed", conclusion: "success", duration_seconds: 101, ...(breakdown === undefined ? {} : {breakdown})}; }

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
