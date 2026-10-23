"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");
const {duration, measured, safeGithubUrl, validateSnapshot, normalizeBreakdown, renderStep} = require("./app.js");

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
  assert.equal(disclosures.length, 3, "one step disclosure and two section disclosures");
  for (const disclosure of disclosures) assert.equal(disclosure.children[0].tagName, "summary", "native disclosure supports keyboard and expanded state");
  assert.match(row.textContent, /sections run sequentially/);
  assert.match(row.textContent, /Task spans can overlap; do not add their durations/);
  assert.match(row.textContent, /Inner phase timings are unavailable/);
  assert.match(row.textContent, /ONIE, BIOS, UEFI and compression/);
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
