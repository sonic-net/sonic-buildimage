"use strict";

// Public CI metadata only. Text is rendered through DOM textContent, never HTML.
const IMAGE_KINDS = new Set(["make-vs", "bazel-vs"]);
const KIND_LABELS = {"make-vs": "Make VS", "bazel-vs": "Bazel VS installer", container: "Container", source: "Source", checks: "Checks", other: "Other"};
const PHASES = ["setup", "build", "validation", "upload", "other"];
// Keep this vocabulary in sync with breakdown.py. Never display labels or log text from a snapshot.
const SECTION_LABELS = {
  "vs-image": "VS image, dependencies and cleanup", "orchagent-release": "Orchagent release",
  "orchagent-debug": "Orchagent debug", "syncd-release": "Syncd release", "syncd-debug": "Syncd debug",
  "p4rt-runtime": "P4RT runtime package", "p4rt-debug": "P4RT debug package",
};
const TASK_LABELS = {
  "final-image": "Final VS image recipe", rootfs: "Root filesystem squashfs", base: "Base container",
  "config-engine": "Config-engine container", "orchagent-release": "Orchagent release container",
  "orchagent-debug": "Orchagent debug container", "syncd-release": "Syncd release container",
  "syncd-debug": "Syncd debug container", "p4rt-runtime": "P4RT runtime package", "p4rt-debug": "P4RT debug package",
  "bmp-watchdog": "BMP watchdog container", "gnmi-watchdog": "gNMI watchdog container",
  "sonic-otel": "OpenTelemetry container", "router-advertiser": "Router advertiser container",
  "dash-engine": "DASH engine container", "sonic-mgmt-framework": "Management framework container",
  database: "Database container", "gnmi-sidecar": "gNMI sidecar container", "sonic-gnmi": "gNMI container",
  mux: "MUX container", "restapi-sidecar": "REST API sidecar container", sysmgr: "System manager container",
  lldp: "LLDP container", "sonic-p4rt": "P4RT container", "gbsyncd-vs": "Gearbox syncd container",
  "platform-monitor": "Platform monitor container", snmp: "SNMP container", "dhcp-relay": "DHCP relay container",
  eventd: "Event daemon container", "sonic-bmp": "BMP container", "swss-layer": "SWSS layer container",
  "dash-ha": "DASH HA container", "fpm-frr": "FRR container", nat: "NAT container", sflow: "sFlow container",
  teamd: "Team daemon container", macsec: "MACsec container",
};
const BREAKDOWN_REASONS = {
  "pending-collection": "Detailed timings have not been collected for this step yet.",
  "step-not-successful": "A complete breakdown is available only for a successful build step.",
  "logs-expired": "The job log is no longer available from GitHub.",
  "logs-too-large": "The job log exceeds the timing collector's size limit.",
  "logs-temporarily-unavailable": "The job log could not be collected. A later snapshot may include these timings.",
  "unsupported-log-format": "This job log does not contain a supported timing breakdown.",
  "incomplete-log": "The job log does not contain all boundaries needed for a complete breakdown.",
  "invalid-timing": "The log timing boundaries could not be verified.",
};
const IMAGE_PHASE_LABELS = {assembly: "Assembly", "boot-checks": "Boot checks", compression: "Compression"};
const IMAGE_CONTEXT_LABELS = {recipe: "Recipe", filesystem: "Filesystem and payload", installer: "ONIE installer", bios: "BIOS", uefi: "UEFI", "4-asic": "4 ASIC", "6-asic": "6 ASIC"};
const IMAGE_PHASE_REASONS = {
  "not-recorded": "Inner phases were not measured in this run. The recipe duration above does not provide separate assembly, boot checks or compression timings.",
  "invalid-timing": "The inner phase measurements could not be verified. The outer recipe timing is still available above.",
  "incomplete-timing": "The inner phase measurements are incomplete. The outer recipe timing is still available above.",
};
const state = {data: null, selectedRun: null, selectedJob: null, loading: false};

function seconds(value) { return typeof value === "number" && Number.isFinite(value) && value >= 0 ? value : null; }
function duration(value) {
  const number = seconds(value);
  if (number === null) return "—";
  if (number < 60) return `${Math.round(number)}s`;
  const rounded = Math.round(number);
  return rounded >= 3600 ? `${Math.floor(rounded / 3600)}h ${Math.floor(rounded % 3600 / 60)}m` : `${Math.floor(rounded / 60)}m ${rounded % 60}s`;
}
function timestamp(value) { const parsed = Date.parse(value || ""); return Number.isFinite(parsed) ? parsed : null; }
function dateTime(value, short = false) {
  if (timestamp(value) === null) return "Not recorded";
  return new Date(value).toLocaleString(undefined, short ? {month: "short", day: "numeric"} : {month: "short", day: "numeric", year: "numeric", hour: "numeric", minute: "2-digit", timeZoneName: "short"});
}
function result(item) { return item.conclusion || item.status || "unknown"; }
function resultLabel(value) { return String(value || "unknown").replaceAll("_", " "); }
function runKey(run) { return `${run.run_id}:${run.attempt}`; }
function imageJob(job) { return IMAGE_KINDS.has(job.kind); }
function measured(job) { return imageJob(job) && job.conclusion === "success" && seconds(job.image_build_seconds) !== null; }
function imageResult(jobs) {
  const outcomes = jobs.filter(imageJob).map(result);
  if (!outcomes.length) return "unmeasured";
  if (outcomes.some(value => ["failure", "timed_out", "startup_failure"].includes(value))) return "failure";
  if (outcomes.some(value => ["in_progress", "queued", "requested", "waiting", "pending"].includes(value))) return "pending";
  // An unknown or incomplete job result must not turn into a successful or skipped PR.
  if (outcomes.some(value => !["success", "cancelled", "skipped"].includes(value))) return "unmeasured";
  if (outcomes.includes("cancelled")) return "cancelled";
  if (outcomes.includes("success")) return "success";
  return "skipped";
}
function prCoverage(snapshot) {
  const imageRuns = snapshot.runs.filter(run => run.jobs.some(imageJob)).sort((a, b) => runTime(b) - runTime(a) || b.attempt - a.attempt || b.run_id - a.run_id);
  return [...snapshot.pull_requests].sort((a, b) => b.number - a.number).map(pr => {
    const latestImageRun = imageRuns.find(run => prNumbers(run).includes(pr.number)) || null;
    const jobs = latestImageRun ? latestImageRun.jobs.filter(imageJob) : [];
    return {pr, classification: imageResult(jobs), latestImageRun, jobs};
  });
}
function summarizePrCoverage(coverage) {
  const counts = {total: coverage.length, success: 0, failure: 0, cancelled: 0, skipped: 0, pending: 0, unmeasured: 0};
  for (const row of coverage) counts[row.classification]++;
  return counts;
}
function kindLabel(job) { return KIND_LABELS[job.kind] || "Other"; }
function architecture(job) { return job.architecture ? String(job.architecture).toUpperCase() : "Unknown architecture"; }
function runTime(run) { return timestamp(run.started_at) || timestamp(run.completed_at) || 0; }
function safeGithubUrl(value, repository) {
  try {
    const url = new URL(value);
    return url.protocol === "https:" && url.hostname === "github.com" && !url.port && !url.username && !url.password && url.pathname.startsWith(`/${repository}/`) ? url.href : null;
  } catch { return null; }
}
function validateSnapshot(data) {
  if (!data || data.schema_version !== 1 || !/^[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+$/.test(data.repository || "") || !Array.isArray(data.runs) || !Array.isArray(data.pull_requests)) throw new Error("The published data has an unsupported format.");
  for (const run of data.runs) {
    if (!run || !Number.isSafeInteger(run.run_id) || !Number.isSafeInteger(run.attempt) || !Array.isArray(run.jobs) || !Array.isArray(run.pr_numbers) || run.jobs.some(job => !job || !Array.isArray(job.steps))) throw new Error("The published run data is incomplete.");
  }
  for (const pr of data.pull_requests) {
    if (!pr || !Number.isSafeInteger(pr.number) || typeof pr.title !== "string") throw new Error("The published pull request data is incomplete.");
  }
  return data;
}
function timingIdentity(id, labels) {
  const match = typeof id === "string" && /^([a-z][a-z0-9-]*)-([1-9][0-9]{0,5})$/.exec(id);
  if (!match || !Object.hasOwn(labels, match[1])) return null;
  return {kind: match[1], label: labels[match[1]] + (Number(match[2]) > 1 ? ` · invocation ${Number(match[2])}` : "")};
}
function timingSpan(item, total) {
  return item && seconds(item.offset_seconds) !== null && seconds(item.duration_seconds) !== null &&
    item.offset_seconds <= Number.MAX_SAFE_INTEGER && item.duration_seconds <= Number.MAX_SAFE_INTEGER &&
    item.offset_seconds + item.duration_seconds <= total + .01;
}
function normalizePhaseBreakdown(value, parentSeconds, outcome) {
  const unavailable = reason => ({status: "unavailable", reason, message: IMAGE_PHASE_REASONS[reason]});
  if (value === undefined) return unavailable("not-recorded");
  if (!value || value.version !== 1 || value.source !== "build-phase-markers") return unavailable("invalid-timing");
  if (value.status === "unavailable") return unavailable(typeof value.reason === "string" && Object.hasOwn(IMAGE_PHASE_REASONS, value.reason) ? value.reason : "invalid-timing");
  if (value.status !== "available" || value.reason !== null || outcome !== "finished" ||
      seconds(parentSeconds) === null || parentSeconds > Number.MAX_SAFE_INTEGER || seconds(value.duration_seconds) === null ||
      value.duration_seconds > Number.MAX_SAFE_INTEGER || Math.abs(value.duration_seconds - parentSeconds) > 2 ||
      !Array.isArray(value.segments) || value.segments.length > 64) return unavailable("invalid-timing");
  if (!value.segments.length) return unavailable("incomplete-timing");
  const segments = [], identities = new Set(), totals = {assembly: null, "boot-checks": null, compression: null};
  let end = 0, sum = 0;
  for (const segment of value.segments) {
    if (!segment || typeof segment.phase !== "string" || !Object.hasOwn(IMAGE_PHASE_LABELS, segment.phase) ||
        typeof segment.context !== "string" || !Object.hasOwn(IMAGE_CONTEXT_LABELS, segment.context) ||
        seconds(segment.offset_seconds) === null || seconds(segment.duration_seconds) === null ||
        segment.offset_seconds > Number.MAX_SAFE_INTEGER || segment.duration_seconds > Number.MAX_SAFE_INTEGER) return unavailable("invalid-timing");
    const identity = JSON.stringify([segment.phase, segment.context, segment.offset_seconds, segment.duration_seconds]);
    if (identities.has(identity)) return unavailable("invalid-timing");
    identities.add(identity);
    if (Math.abs(segment.offset_seconds - end) > .00001) return unavailable("incomplete-timing");
    end = segment.offset_seconds + segment.duration_seconds;
    if (end > value.duration_seconds + .00001) return unavailable("incomplete-timing");
    sum += segment.duration_seconds;
    totals[segment.phase] = (totals[segment.phase] ?? 0) + segment.duration_seconds;
    segments.push({phase: segment.phase, context: segment.context, offset_seconds: segment.offset_seconds, duration_seconds: segment.duration_seconds});
  }
  if (Math.abs(end - value.duration_seconds) > .00001 || Math.abs(sum - value.duration_seconds) > .00001) return unavailable("incomplete-timing");
  return {status: "available", duration_seconds: value.duration_seconds, totals, segments};
}
function normalizeBreakdown(value) {
  // A missing optional field preserves historical snapshots. Malformed optional data never breaks the dashboard.
  if (value === undefined) return null;
  const unavailable = reason => ({status: "unavailable", reason});
  const invalid = () => unavailable("The published breakdown is incomplete or unsupported.");
  if (!value || value.version !== 1 || value.source !== "github-job-log") return invalid();
  if (value.status === "unavailable") return unavailable(typeof value.reason === "string" && Object.hasOwn(BREAKDOWN_REASONS, value.reason) ? BREAKDOWN_REASONS[value.reason] : "Detailed timings are unavailable for this step.");
  if (value.status !== "available" || seconds(value.duration_seconds) === null || value.duration_seconds <= 0 ||
      value.duration_seconds > Number.MAX_SAFE_INTEGER || !Array.isArray(value.sections) || !value.sections.length || value.sections.length > 32) return invalid();
  let end = 0, taskCount = 0;
  const sections = [], sectionIds = new Set();
  for (const section of value.sections) {
    const identity = timingIdentity(section?.id, SECTION_LABELS);
    if (!identity || sectionIds.has(section.id) || !timingSpan(section, value.duration_seconds) ||
        Math.abs(section.offset_seconds - end) > .01 || !Array.isArray(section.tasks)) return invalid();
    sectionIds.add(section.id);
    end = section.offset_seconds + section.duration_seconds;
    taskCount += section.tasks.length;
    if (taskCount > 512) return invalid();
    const tasks = [], taskIds = new Set();
    for (const task of section.tasks) {
      const taskIdentity = timingIdentity(task?.id, TASK_LABELS);
      if (!taskIdentity || taskIds.has(task.id) || !timingSpan(task, section.duration_seconds) || !["finished", "cached"].includes(task.outcome)) return invalid();
      taskIds.add(task.id);
      const normalized = {...taskIdentity, offset_seconds: task.offset_seconds, duration_seconds: task.duration_seconds, outcome: task.outcome};
      if (taskIdentity.kind === "final-image") normalized.phase_breakdown = normalizePhaseBreakdown(task.phase_breakdown, task.duration_seconds, task.outcome);
      tasks.push(normalized);
    }
    tasks.sort((a, b) => a.offset_seconds - b.offset_seconds);
    sections.push({...identity, offset_seconds: section.offset_seconds, duration_seconds: section.duration_seconds, tasks});
  }
  if (Math.abs(end - value.duration_seconds) > .01) return invalid();
  return {status: "available", duration_seconds: value.duration_seconds, sections};
}
function el(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined) node.textContent = String(text);
  return node;
}
function byId(id) { return document.getElementById(id); }
function badge(value, label) {
  const allowed = new Set(["success", "failure", "timed_out", "action_required", "startup_failure", "in_progress", "queued", "requested", "waiting", "pending", "current"]);
  return el("span", `badge${allowed.has(value) ? ` ${value}` : ""}`, label || resultLabel(value));
}
function link(text, url) {
  const safe = safeGithubUrl(url, state.data.repository);
  if (!safe) return el("span", "", text);
  const anchor = el("a", "", text);
  anchor.href = safe;
  return anchor;
}
function prNumbers(run) { return run.pr_numbers.filter(number => Number.isSafeInteger(number)); }
function matchesRun(run) {
  const selectedPr = byId("pr-filter").value;
  return selectedPr === "all" || prNumbers(run).includes(Number(selectedPr));
}
function matchesJob(job) {
  const path = byId("path-filter").value;
  const arch = byId("arch-filter").value;
  return (path === "all" || job.kind === path) && (arch === "all" || (job.architecture || "unknown") === arch);
}
function records() { return state.data.runs.flatMap(run => run.jobs.map(job => ({run, job}))); }
function successfulImages() { return records().filter(item => measured(item.job)).sort((a, b) => runTime(b.run) - runTime(a.run)); }
function setSelected(run, jobId) {
  state.selectedRun = runKey(run);
  state.selectedJob = jobId || null;
  renderCoverage();
  renderRuns();
  renderDetails();
}

function renderMetrics() {
  const successes = successfulImages();
  const latest = successes[0];
  const measuredPrs = new Set(successes.flatMap(item => prNumbers(item.run)));
  const metrics = [
    ["Latest successful image step", latest ? duration(latest.job.image_build_seconds) : "—", latest ? `${kindLabel(latest.job)} · ${architecture(latest.job)}` : "Waiting for an image measurement"],
    ["Successful image measurements", successes.length, "Build steps with a successful result"],
    ["PRs with image measurements", `${state.data.pull_requests.filter(pr => measuredPrs.has(pr.number)).length} / ${state.data.pull_requests.length}`, "Includes historical PR revisions"],
    ["Workflow runs retained", state.data.runs.length, "Each retry attempt is recorded separately"],
  ];
  byId("metrics").replaceChildren(...metrics.map(([label, value, note]) => {
    const card = el("div", "metric");
    card.append(el("p", "metric-label", label), el("p", "metric-value", value), el("p", "metric-note", note));
    return card;
  }));
}
function renderFilters() {
  const select = byId("pr-filter");
  const old = select.value;
  const all = el("option", "", "All pull requests & branches");
  all.value = "all";
  select.replaceChildren(all);
  for (const pr of [...state.data.pull_requests].sort((a, b) => b.number - a.number)) {
    const option = el("option", "", `#${pr.number} · ${pr.title}`);
    option.value = String(pr.number);
    select.append(option);
  }
  select.value = [...select.options].some(option => option.value === old) ? old : "all";
}
function svgEl(tag, attrs, text) {
  const node = document.createElementNS("http://www.w3.org/2000/svg", tag);
  for (const [key, value] of Object.entries(attrs || {})) node.setAttribute(key, String(value));
  if (text !== undefined) node.textContent = String(text);
  return node;
}
function renderTrend() {
  const all = successfulImages().filter(item => matchesRun(item.run) && matchesJob(item.job));
  const points = all.slice(0, 60).reverse();
  const container = byId("trend");
  container.replaceChildren();
  byId("trend-note").textContent = `Showing ${points.length} of ${all.length} matching successful image measurements. Select a point to inspect its job. Lines join only the same build path and architecture; they do not imply a controlled comparison.`;
  if (!points.length) { container.append(el("p", "empty", "No successful image measurements match these filters.\nFailed, skipped, and pending runs remain visible below.")); return; }
  const padding = getComputedStyle(container);
  const width = Math.max(240, Math.round(container.clientWidth - parseFloat(padding.paddingLeft) - parseFloat(padding.paddingRight)));
  const height = width < 520 ? 250 : 265;
  const left = 55, right = 40, top = 19, bottom = 41;
  const maxSeconds = Math.max(60, ...points.map(item => item.job.image_build_seconds)) * 1.14;
  const firstTime = Math.min(...points.map(item => runTime(item.run)));
  const lastTime = Math.max(...points.map(item => runTime(item.run)));
  const x = item => lastTime === firstTime ? (width + left - right) / 2 : left + (runTime(item.run) - firstTime) / (lastTime - firstTime) * (width - left - right);
  const y = item => top + (1 - item.job.image_build_seconds / maxSeconds) * (height - top - bottom);
  const svg = svgEl("svg", {viewBox: `0 0 ${width} ${height}`, role: "group", "aria-label": "Successful image build times, in minutes, ordered by run start time. Each point opens run details."});
  for (let step = 0; step <= 4; step++) {
    const value = maxSeconds * step / 4;
    const position = top + (1 - step / 4) * (height - top - bottom);
    svg.append(svgEl("line", {x1: left, y1: position, x2: width - right, y2: position, stroke: "#e6edef", "stroke-dasharray": step ? "3 4" : "0"}), svgEl("text", {x: left - 11, y: position + 4, "text-anchor": "end"}, `${Math.round(value / 60)}m`));
  }
  const dateCount = lastTime === firstTime ? 1 : width < 400 ? 2 : width < 700 ? 3 : 5;
  for (let index = 0; index < dateCount; index++) {
    const fraction = dateCount === 1 ? .5 : index / (dateCount - 1);
    const tick = firstTime + (lastTime - firstTime) * fraction;
    svg.append(svgEl("text", {x: left + fraction * (width - left - right), y: height - 13, "text-anchor": "middle"}, dateTime(new Date(tick).toISOString(), true)));
  }
  const groups = new Map();
  for (const item of points) {
    const group = `${item.job.kind}:${item.job.architecture || "unknown"}`;
    if (!groups.has(group)) groups.set(group, []);
    groups.get(group).push(item);
  }
  for (const items of groups.values()) {
    const color = items[0].job.kind === "make-vs" ? "#007f78" : "#23548a";
    const line = svgEl("polyline", {points: items.map(item => `${x(item)},${y(item)}`).join(" "), fill: "none", stroke: color, "stroke-opacity": ".45", "stroke-width": "2", "stroke-dasharray": items[0].job.architecture === "arm64" ? "5 4" : "0"});
    svg.append(line);
  }
  for (const item of points) {
    const prs = prNumbers(item.run).map(number => `PR #${number}`).join(", ") || item.run.branch || "Branch build";
    const label = `${prs} · ${kindLabel(item.job)} · ${architecture(item.job)} · ${duration(item.job.image_build_seconds)} · ${dateTime(item.run.started_at)}`;
    const point = svgEl("g", {class: "chart-point", tabindex: "0", role: "button", "aria-label": label});
    point.append(svgEl("title", {}, label));
    point.append(svgEl("circle", {cx: x(item), cy: y(item), r: 17, fill: "transparent", "pointer-events": "all"}));
    point.append(svgEl("circle", {class: "chart-marker", cx: x(item), cy: y(item), r: 4.5, fill: item.job.kind === "make-vs" ? "#007f78" : "#23548a", stroke: "white", "stroke-width": 2, "pointer-events": "none"}));
    point.addEventListener("click", () => setSelected(item.run, item.job.job_id));
    point.addEventListener("keydown", event => { if (["Enter", " "].includes(event.key)) { event.preventDefault(); setSelected(item.run, item.job.job_id); } });
    svg.append(point);
  }
  container.append(svg);
}

function renderCoverage() {
  const coverage = prCoverage(state.data);
  const counts = summarizePrCoverage(coverage);
  byId("coverage-summary").replaceChildren(...[["success", "Success"], ["failure", "Failed"], ["cancelled", "Cancelled"], ["skipped", "Skipped"]].map(([outcome, label]) => {
    const item = el("div", `coverage-stat ${outcome}`);
    item.append(el("dt", "", label), el("dd", "", counts[outcome]));
    return item;
  }));
  const note = byId("coverage-other");
  note.textContent = `${counts.pending} pending · ${counts.unmeasured} unmeasured (no image jobs or result unavailable).`;
  note.hidden = counts.pending + counts.unmeasured === 0;
  const rows = [];
  const selected = state.data.runs.find(run => runKey(run) === state.selectedRun);
  for (const {pr, classification, latestImageRun, jobs} of coverage) {
    if (classification !== "success" && classification !== "failure") continue;
    const row = el("tr", selected && prNumbers(selected).includes(pr.number) ? "selected" : "");
    const title = el("td");
    const button = el("button", "table-button");
    button.type = "button";
    button.append(el("span", "pr-number", `#${pr.number}`), document.createTextNode(pr.title));
    button.addEventListener("click", () => {
      byId("pr-filter").value = String(pr.number);
      state.selectedRun = runKey(latestImageRun);
      state.selectedJob = jobs[0]?.job_id || null;
      renderFiltered();
    });
    title.append(button, el("span", "subtext", pr.merged_at ? "Merged" : pr.draft ? "Draft" : resultLabel(pr.state)));
    const status = el("td"), timings = el("td", "nowrap");
    const stack = el("div", "status-stack");
    for (const job of jobs) {
      const item = el("div");
      item.append(badge(result(job)), el("span", "subtext", `${kindLabel(job)} · ${architecture(job)}`));
      stack.append(item);
      const time = el("div", "", measured(job) ? duration(job.image_build_seconds) : "—");
      time.append(el("span", "subtext", measured(job) ? "successful step" : job.conclusion === "success" ? "no step timing" : "not a successful build"));
      timings.append(time);
    }
    status.append(stack);
    const current = !!pr.head_sha && latestImageRun.source_sha === pr.head_sha;
    title.append(el("span", "subtext sha", String(latestImageRun.source_sha || "unknown").slice(0, 10)), badge(current ? "current" : "historical", current ? "Current PR head" : "Different revision"));
    title.append(el("span", "subtext", dateTime(latestImageRun.started_at)));
    row.append(title, status, timings); rows.push(row);
  }
  byId("coverage-body").replaceChildren(...rows);
  byId("coverage-count").textContent = `${rows.length} of ${counts.total} PRs`;
  if (!rows.length) { const row = el("tr"), cell = el("td", "empty", counts.total ? "No pull requests have a successful or failed image result in the retained history." : "No pull requests have been collected yet."); cell.colSpan = 3; row.append(cell); byId("coverage-body").append(row); }
}
function renderRuns() {
  const all = state.data.runs.filter(run => matchesRun(run) && (byId("path-filter").value === "all" && byId("arch-filter").value === "all" || run.jobs.some(matchesJob))).sort((a, b) => runTime(b) - runTime(a));
  const rows = all.slice(0, 100).map(run => {
    const row = el("tr", state.selectedRun === runKey(run) ? "selected" : "");
    const workflow = el("td");
    const button = el("button", "table-button", run.workflow_name || run.workflow_path || "Workflow");
    button.type = "button";
    button.addEventListener("click", () => setSelected(run));
    workflow.append(button, el("span", "subtext sha", `${String(run.source_sha || "unknown").slice(0, 10)} · ${run.branch || "unknown branch"}`));
    const prs = el("td", "", prNumbers(run).map(number => `#${number}`).join(", ") || "—");
    const status = el("td"); status.append(badge(result(run)));
    row.append(workflow, prs, status, el("td", "", dateTime(run.started_at)), el("td", "", run.attempt));
    return row;
  });
  byId("runs-body").replaceChildren(...rows);
  byId("run-count").textContent = `${rows.length} of ${all.length}`;
  if (!rows.length) { const row = el("tr"), cell = el("td", "empty", "No workflow runs match these filters."); cell.colSpan = 5; row.append(cell); byId("runs-body").append(row); }
}

function renderPhaseBreakdown(phases) {
  const body = el("div", "phase-body");
  if (phases.status !== "available") {
    body.append(el("p", "unmeasured-note", phases.message));
    return body;
  }
  body.append(el("p", "breakdown-note", `Measured phase total: ${duration(phases.duration_seconds)}. These markers use the build process clock; the recipe duration above uses streamed log timestamps. Small differences are preserved, not rescaled.`));
  const totals = el("dl", "phase-totals");
  for (const [phase, label] of Object.entries(IMAGE_PHASE_LABELS)) {
    const item = el("div", "phase-total");
    item.append(el("dt", "", label), el("dd", "", phases.totals[phase] === null ? "No segment recorded" : duration(phases.totals[phase])));
    totals.append(item);
  }
  body.append(totals, el("p", "breakdown-note", "Category totals combine repeated segments. The list below preserves their execution order, including each filesystem, payload or disk compression segment."));
  const sequence = el("ol", "phase-segments");
  for (const [index, segment] of phases.segments.entries()) {
    const item = el("li", "phase-segment"), row = el("div", "timing-row");
    const name = el("span", "timing-name", `${index + 1}. ${IMAGE_PHASE_LABELS[segment.phase]} · ${IMAGE_CONTEXT_LABELS[segment.context]}`);
    name.append(el("span", "timing-hint", `Starts at +${duration(segment.offset_seconds)} on the phase clock`));
    row.append(name, el("span", "timing-duration", duration(segment.duration_seconds)));
    item.append(row); sequence.append(item);
  }
  body.append(sequence);
  return body;
}
function renderBreakdown(breakdown) {
  const box = el("div", "breakdown-body");
  if (breakdown.status !== "available") {
    box.append(el("p", "breakdown-note", `Breakdown unavailable. ${breakdown.reason} The recorded step duration is shown above.`));
    return box;
  }
  box.append(el("p", "breakdown-note", `Measured from job log timestamps · ${duration(breakdown.duration_seconds)}. These sections run sequentially in top-level Make request order; each includes its prerequisites, setup and cleanup. Log timestamps may differ slightly from the rounded step duration.`));
  const list = el("ol", "breakdown-sections");
  for (const section of breakdown.sections) {
    const item = el("li", "breakdown-section");
    const row = el(section.tasks.length ? "summary" : "div", "timing-row");
    const name = el("span", "timing-name", section.label);
    name.append(el("span", "timing-hint", `Starts at +${duration(section.offset_seconds)}${section.tasks.length ? ` · ${section.tasks.length} measured ${section.tasks.length === 1 ? "task" : "tasks"}` : " · no measured task detail"}`));
    row.append(name, el("span", "timing-duration", duration(section.duration_seconds)));
    if (!section.tasks.length) item.append(row);
    else {
      const details = el("details", "section-breakdown");
      details.append(row, el("p", "breakdown-note", "Task spans can overlap; do not add their durations. Only recognized task spans are shown; remaining section time is not assigned to tasks. Bars share the section's time scale."));
      const tasks = el("ol", "breakdown-tasks");
      for (const task of section.tasks) {
        const hasPhases = task.kind === "final-image";
        const taskItem = el("li", "breakdown-task"), taskRow = el(hasPhases ? "summary" : "div", "timing-row");
        const taskName = el("span", "timing-name", task.label);
        taskName.append(el("span", "timing-hint", `Starts at +${duration(task.offset_seconds)} in section · log: ${task.outcome}`));
        if (hasPhases) taskName.append(el("span", "timing-hint", task.phase_breakdown.status === "available" ? "Assembly, boot checks and compression" : task.phase_breakdown.reason === "not-recorded" ? "Inner phases not measured in this run" : "Inner phase breakdown unavailable"));
        taskRow.append(taskName, el("span", "timing-duration", duration(task.duration_seconds)));
        const track = el("div", "task-track"), span = el("span", "task-span");
        track.setAttribute("aria-hidden", "true");
        span.style.marginLeft = `${section.duration_seconds ? Math.min(100, task.offset_seconds / section.duration_seconds * 100) : 0}%`;
        span.style.width = `${section.duration_seconds ? Math.min(100, task.duration_seconds / section.duration_seconds * 100) : 0}%`;
        track.append(span);
        if (hasPhases) {
          const details = el("details", "phase-breakdown");
          taskRow.append(track); details.append(taskRow, renderPhaseBreakdown(task.phase_breakdown)); taskItem.append(details);
        } else taskItem.append(taskRow, track);
        tasks.append(taskItem);
      }
      details.append(tasks); item.append(details);
    }
    list.append(item);
  }
  box.append(list);
  return box;
}
function renderStep(step) {
  const item = el("li", "step-item"), breakdown = normalizeBreakdown(step.breakdown);
  const row = el(breakdown ? "summary" : "div", "timing-row");
  const name = el("span", "timing-name", step.name || "Unnamed step"), timing = el("span", "step-duration");
  timing.append(badge(result(step)), document.createTextNode(duration(step.duration_seconds)));
  if (breakdown) name.append(el("span", "timing-hint", breakdown.status === "available" ? `Measured breakdown · ${breakdown.sections.length} sections` : "Breakdown unavailable"));
  row.append(name, timing);
  if (breakdown) {
    const details = el("details", "step-breakdown"); details.append(row, renderBreakdown(breakdown)); item.append(details);
  } else item.append(row);
  return item;
}

function renderDetails() {
  const box = byId("details"); box.replaceChildren();
  const run = state.data.runs.find(item => runKey(item) === state.selectedRun);
  if (!run) { box.append(el("p", "empty", "Select a chart point, pull request, or workflow run to see its timing breakdown.")); return; }
  const heading = el("p", "detail-meta");
  heading.append(link(`${run.workflow_name || "Workflow"} ↗`, run.url), badge(result(run)), el("span", "subtext", `${dateTime(run.started_at)} · Attempt ${run.attempt}`), el("span", "subtext sha", `Source ${String(run.source_sha || "unknown").slice(0, 12)} · ${run.event || "unknown event"}`));
  box.append(heading);
  if (run.pr_association_note) box.append(el("p", "source-warning", run.pr_association_note));
  const selectedPr = Number(byId("pr-filter").value);
  const pr = state.data.pull_requests.find(item => item.number === selectedPr) || state.data.pull_requests.find(item => prNumbers(run).includes(item.number));
  if (pr) {
    const note = el("p", "detail-meta");
    note.append(link(`PR #${pr.number}`, pr.url), document.createTextNode(` · association: ${run.pr_association || "unknown"}`));
    box.append(note);
    if (pr.head_sha !== run.source_sha) box.append(el("p", "source-warning", "The recorded source revision differs from the current PR head. This may be a historical revision or a merge build; it is not proof that the current PR head was measured."));
  }
  if (!run.jobs.length) { box.append(el("p", "empty", "No job details are available for this workflow yet.")); return; }
  const label = el("label", "detail-select", "Job");
  const select = el("select");
  for (const job of run.jobs) {
    const option = el("option", "", `${job.name} · ${resultLabel(result(job))}`);
    option.value = String(job.job_id); select.append(option);
  }
  const job = run.jobs.find(item => String(item.job_id) === String(state.selectedJob)) || run.jobs.find(imageJob) || run.jobs[0];
  select.value = String(job.job_id);
  select.addEventListener("change", () => { state.selectedJob = select.value; renderDetails(); });
  label.append(select); box.append(label);
  const meta = el("p", "detail-meta");
  meta.append(link(`${kindLabel(job)} · ${architecture(job)} ↗`, job.url), badge(result(job)));
  box.append(meta);
  const values = el("div", "detail-values");
  for (const [name, value] of [["Successful image build step", measured(job) ? job.image_build_seconds : null], ["Total job elapsed", job.duration_seconds]]) {
    const valueBox = el("div", "detail-value"); valueBox.append(el("span", "", name), el("strong", "", duration(value))); values.append(valueBox);
  }
  box.append(values);
  const phases = job.phases || {};
  const total = PHASES.reduce((sum, phase) => sum + (seconds(phases[phase]) || 0), 0);
  const bar = el("div", "stage-bar"); bar.setAttribute("aria-label", "Recorded step time by phase");
  const legend = el("div", "stage-legend");
  for (const phase of PHASES) {
    const value = seconds(phases[phase]);
    if (total > 0 && value !== null) { const segment = el("span", `stage-${phase}`); segment.style.width = `${value / total * 100}%`; segment.title = `${phase}: ${duration(value)}`; bar.append(segment); }
    const item = el("span"); item.append(el("i", `stage-${phase}`), document.createTextNode(phase[0].toUpperCase() + phase.slice(1)), el("b", "", duration(value))); legend.append(item);
  }
  box.append(bar, legend, el("p", "stage-note", "Stage totals sum recorded step durations. They may not equal job elapsed time, and the build phase can include more work than the image build step. Queue time is excluded. Cache state and runner comparability are unknown."));
  const steps = el("details", "steps"); steps.append(el("summary", "", `Step timings · ${job.steps.length} steps`));
  const list = el("ol");
  for (const step of job.steps) list.append(renderStep(step));
  steps.append(list); box.append(steps);
}
function renderFiltered() { renderTrend(); renderCoverage(); renderRuns(); renderDetails(); }
function render() {
  byId("dashboard").hidden = false;
  byId("snapshot-time").textContent = `Snapshot: ${dateTime(state.data.generated_at)}`;
  byId("repository-link").href = `https://github.com/${state.data.repository}`;
  renderFilters(); renderMetrics();
  if (!state.selectedRun) { const latest = successfulImages()[0]; const run = latest?.run || [...state.data.runs].sort((a, b) => runTime(b) - runTime(a))[0]; state.selectedRun = run ? runKey(run) : null; state.selectedJob = latest?.job.job_id || null; }
  renderFiltered();
}
async function refresh() {
  if (state.loading) return;
  state.loading = true; byId("refresh").disabled = true;
  const status = byId("load-status"); status.className = ""; status.textContent = state.data ? "Checking for a newer published snapshot…" : "Loading CI history…";
  try {
    const url = new URL("data.json", window.location.href); url.searchParams.set("t", String(Date.now()));
    const response = await fetch(url, {cache: "no-store", credentials: "omit", signal: AbortSignal.timeout(30000)});
    if (!response.ok) throw new Error(`Snapshot request failed (HTTP ${response.status}).`);
    state.data = validateSnapshot(await response.json());
    render(); status.textContent = "";
  } catch (error) {
    status.className = "error";
    status.textContent = `${state.data ? "Showing the last loaded snapshot. " : "The dashboard could not load its data. "}${error.message} Try Refresh data again.`;
  } finally { state.loading = false; byId("refresh").disabled = false; }
}
if (typeof document !== "undefined") {
  byId("refresh").addEventListener("click", refresh);
  for (const id of ["path-filter", "arch-filter", "pr-filter"]) byId(id).addEventListener("change", () => { if (state.data) renderFiltered(); });
  let chartWidth = 0;
  const chartResize = new ResizeObserver(entries => {
    const width = Math.round(entries[0].contentRect.width);
    if (width > 0 && width !== chartWidth) {
      chartWidth = width;
      if (state.data) requestAnimationFrame(renderTrend);
    }
  });
  chartResize.observe(byId("trend"));
  refresh();
  setInterval(() => { if (!document.hidden) refresh(); }, 300000);
}
if (typeof module !== "undefined") module.exports = {seconds, duration, timestamp, safeGithubUrl, validateSnapshot, measured, imageResult, prCoverage, summarizePrCoverage, normalizeBreakdown, normalizePhaseBreakdown, renderStep};
