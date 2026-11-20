"use strict";

// Public CI metadata only. Text is rendered through DOM textContent, never HTML.
const IMAGE_KINDS = new Set(["make-vs", "bazel-vs"]);
const KIND_LABELS = {"make-vs": "Make VS", "bazel-vs": "Bazel VS installer", container: "Container", source: "Source", checks: "Checks", other: "Other"};
const PHASES = ["setup", "build", "validation", "upload", "other"];
const HOST_VS_BUILD_STEP = "Build sonic-vs.bin through the host Make wrapper";
// Match collect.py's IMAGE_STEPS. Exact names preserve historical timing boundaries.
const IMAGE_STEP_LABELS = {
  [HOST_VS_BUILD_STEP]: "VS installer (.bin) and paired image archives through host Make",
  "Build the VS installer (.bin)": "VS installer (.bin); VM generation and boot checks excluded",
  "Build sonic-vs.bin directly with slave.mk": "VS installer (.bin), direct slave.mk; VM generation and boot checks excluded",
  "Build the VS image": "VS build with embedded boot checks; post-build validation excluded",
  "Build the VS image with Bazel SWSS archives": "Build the VS image with Bazel SWSS archives",
  "Build the VS image and both OCI container variants": "VS build, OCI follow-up and archive validation",
  "Build the Bazel VS installer": "Build the Bazel VS installer",
  "Build Bazel VS installer": "Build Bazel VS installer",
  "Build and verify sonic-vs.bin with Bazel": "Build and verify sonic-vs.bin with Bazel",
};
const IMAGE_SERIES_STYLES = {
  [HOST_VS_BUILD_STEP]: ["Host Make .bin + archives", "#b02c45"],
  "Build the VS installer (.bin)": ["Outer Make .bin", "#007f78"],
  "Build sonic-vs.bin directly with slave.mk": ["Direct slave .bin", "#a34d12"],
  "Build the VS image": ["Legacy VM image", "#7458a6"],
  "Build the VS image with Bazel SWSS archives": ["Legacy VM + SWSS archives", "#8d4773"],
  "Build the VS image and both OCI container variants": ["Legacy image + OCI validation", "#536775"],
  "Build the Bazel VS installer": ["Installer build", "#23548a"],
  "Build Bazel VS installer": ["Installer build (earlier step)", "#356aa0"],
  "Build and verify sonic-vs.bin with Bazel": ["Installer + verification", "#5165a0"],
};
// Keep this vocabulary in sync with breakdown.py. Never display labels or log text from a snapshot.
const SECTION_LABELS = {
  "oci-session": "OCI archives and verification",
  "oci-verification": "P4 packages and OCI verification",
  "vs-image": "VS image, dependencies and cleanup", "orchagent-release": "Orchagent release",
  "orchagent-debug": "Orchagent debug", "syncd-release": "Syncd release", "syncd-debug": "Syncd debug",
  "p4rt-runtime": "P4RT runtime package", "p4rt-debug": "P4RT debug package",
};
const TASK_LABELS = {
  "vs-container-images": "Bazel container images",
  "final-image": "Final VS image recipe", rootfs: "Root filesystem squashfs", base: "Base container",
  "config-engine": "Config-engine container", "orchagent-release": "Orchagent release container",
  "orchagent-debug": "Orchagent debug container", "syncd-release": "Syncd release container",
  "syncd-debug": "Syncd debug container", "p4rt-runtime": "P4RT runtime package", "p4rt-debug": "P4RT debug package",
  "orchagent-pair": "Orchagent runtime and debug containers", "syncd-pair": "Syncd runtime and debug containers",
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
const LEGACY_RECIPE_OWNERS = new Set(["docker-teamd.gz", "docker-syncd-vs.gz", "docker-fpm-frr.gz", "docker-mux.gz"]);
const RECIPE_REASONS = {
  "pending-collection": "Bazel build timings are pending collection.",
  "step-not-successful": "The timing summary did not finish successfully.",
  "logs-expired": "The GitHub log for these Bazel timings has expired.",
  "logs-too-large": "The timing log exceeds the collector’s size limit.",
  "logs-temporarily-unavailable": "GitHub’s timing log is temporarily unavailable.",
  "not-recorded": "No Bazel timing breakdown was recorded for this build.",
  "invalid-timing": "The Bazel timing measurements could not be verified.",
};
const RECIPE_METRICS = [
  ["bazel_seconds", "Bazel / launcher", "bazel_phases", "bazel_expected"],
  ["lock_seconds", "Bazel lock wait", "profiles", "profiles_expected"],
  ["compile_seconds", "Compiler subprocesses", "profiles", "profiles_expected"],
  ["foreign_seconds", "Nested build subprocesses", "profiles", "profiles_expected"],
  ["fetch_seconds", "Cache fetch", "profiles", "profiles_expected"],
  ["export_seconds", "Export", "exports", "exports_expected"],
];
const BUILD_EVIDENCE = {
  repository_seconds: "Repository preparation",
  repository_read_seconds: "Repository input reads/waits",
  compile_seconds: "Compiler subprocesses",
  foreign_seconds: "Nested build subprocesses",
  packaging_seconds: "Image/tar/gzip critical path",
  package_selection_seconds: "APT selection critical path",
  validation_seconds: "Identity/source validation critical path",
  fetch_seconds: "Cache input/output fetch",
  cache_write_seconds: "Cache write spans (not proof of remote upload)",
  arm64_repository_seconds: "ARM64 package repository preparation",
  armhf_repository_seconds: "ARMHF package repository preparation",
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
function prStateLabel(pr) {
  if (pr.merged_at) return "Merged";
  if (pr.state === "closed") return "Closed";
  if (pr.state === "open" && pr.draft) return "Draft";
  return resultLabel(pr.state);
}
function runKey(run) { return `${run.run_id}:${run.attempt}`; }
function verifiedAttemptStart(run) { return run.started_at_source === "run_started_at" && timestamp(run.started_at) !== null; }
function attemptTimeLabel(run) {
  if (timestamp(run.started_at) === null) return `Attempt ${run.attempt} start not recorded`;
  if (verifiedAttemptStart(run)) return `Attempt ${run.attempt} started ${dateTime(run.started_at)}`;
  if (run.started_at_source === "created_at") return `Run created ${dateTime(run.started_at)} · attempt ${run.attempt} start not recorded`;
  return `Recorded run time ${dateTime(run.started_at)} · attempt ${run.attempt} start unverified`;
}
function attemptTimeSourceLabel(run) {
  if (verifiedAttemptStart(run)) return "Verified attempt start";
  if (run.started_at_source === "created_at") return "Run creation; attempt start not recorded";
  return "Attempt start unverified";
}
function imageJob(job) { return IMAGE_KINDS.has(job.kind); }
function completedImageSeconds(job) { return imageJob(job) ? seconds(job.image_build_seconds) : null; }
function measured(job) { return job.conclusion === "success" && completedImageSeconds(job) !== null; }
function imageMeasurement(job) {
  const names = (Array.isArray(job.steps) ? job.steps : []).map(step => step?.name)
    .filter(name => typeof name === "string" && Object.hasOwn(IMAGE_STEP_LABELS, name)).sort();
  return names.length ? {signature: JSON.stringify(names), label: names.map(name => IMAGE_STEP_LABELS[name]).join(" + ")} :
    {signature: null, label: "Image step boundary not recorded"};
}
function imageTrendGroups(points) {
  const groups = new Map();
  for (const item of points) {
    const signature = imageMeasurement(item.job).signature;
    // Unknown boundaries are isolated points, even when path and architecture match.
    const key = signature === null ? item : JSON.stringify([item.job.kind, item.job.architecture || "unknown", signature]);
    if (!groups.has(key)) groups.set(key, []);
    groups.get(key).push(item);
  }
  return [...groups.values()];
}
function jobExecutionTimes(job) {
  const start = timestamp(job.started_at), end = timestamp(job.completed_at);
  if (job.status !== "completed" || typeof job.conclusion !== "string" || start === null || end === null || end < start || !Array.isArray(job.steps)) return null;
  const executed = job.steps.some(step => {
    const stepStart = timestamp(step.started_at), stepEnd = timestamp(step.completed_at);
    return step.status === "completed" && typeof step.conclusion === "string" && step.conclusion !== "skipped" &&
      stepStart !== null && stepEnd !== null && stepStart >= start && stepEnd >= stepStart && stepEnd <= end;
  });
  return executed ? {start, end} : null;
}
function executionSignature(job) {
  const times = jobExecutionTimes(job);
  if (times === null || typeof job.name !== "string" || !job.steps.length) return null;
  const steps = [], numbers = new Set();
  for (const step of job.steps) {
    if (!Number.isSafeInteger(step.number) || step.number < 1 || numbers.has(step.number) || typeof step.name !== "string" || !step.name ||
        step.status !== "completed" || typeof step.conclusion !== "string" ||
        !Object.hasOwn(step, "started_at") || !Object.hasOwn(step, "completed_at")) return null;
    const stepStart = timestamp(step.started_at), stepEnd = timestamp(step.completed_at);
    if (step.conclusion !== "skipped" && (stepStart === null || stepEnd === null)) return null;
    if ((step.started_at !== null && stepStart === null) || (step.completed_at !== null && stepEnd === null) ||
        (stepStart !== null && stepStart < times.start) || (stepEnd !== null && stepEnd > times.end) ||
        (stepStart !== null && stepEnd !== null && stepEnd < stepStart)) return null;
    numbers.add(step.number);
    steps.push([step.number, step.name, step.status, step.conclusion, stepStart, stepEnd]);
  }
  return JSON.stringify([job.name, job.kind, job.architecture || null, job.status, job.conclusion, times.start, times.end, steps]);
}
function imageExecution(snapshot, run, job) {
  const execution = {originStatus: "unverified", originAttempt: null, originJobId: null, originUrl: null,
    timeKind: "unknown", startedAt: null, completedAt: null, attemptStartedAt: verifiedAttemptStart(run) ? run.started_at : null,
    recordedRunTime: run.started_at || null, attemptStartVerified: verifiedAttemptStart(run)};
  const jobTimes = jobExecutionTimes(job), jobStart = jobTimes?.start ?? null, jobEnd = jobTimes?.end ?? null, attemptStart = timestamp(run.started_at);
  const numbers = job.image_build_step_numbers;
  if (jobTimes !== null && Array.isArray(numbers) && numbers.length && numbers.every(number => Number.isSafeInteger(number) && number > 0) && new Set(numbers).size === numbers.length) {
    const selected = numbers.map(number => job.steps.filter(step => step.number === number));
    if (selected.every(group => group.length === 1)) {
      const steps = selected.map(group => group[0]);
      const starts = steps.map(step => timestamp(step.started_at)), ends = steps.map(step => timestamp(step.completed_at));
      if (steps.every(step => step.status === "completed") && starts.every(value => value !== null && value >= jobStart) &&
          ends.every((value, index) => value !== null && value >= starts[index] && value <= jobEnd)) {
        execution.timeKind = steps.length === 1 ? "image_step" : "image_steps";
        execution.startedAt = new Date(Math.min(...starts)).toISOString();
        execution.completedAt = new Date(Math.max(...ends)).toISOString();
      }
    }
  }
  const completed = jobTimes !== null;
  if (execution.startedAt === null && completed) {
    execution.timeKind = "image_job";
    execution.startedAt = new Date(jobStart).toISOString();
    execution.completedAt = new Date(jobEnd).toISOString();
  }
  if (!completed || attemptStart === null || !Number.isSafeInteger(run.run_id) || run.run_id < 1 || !Number.isSafeInteger(run.attempt) || run.attempt < 1 ||
      !Number.isSafeInteger(job.job_id) || job.job_id < 1 || typeof run.source_sha !== "string" || !run.source_sha ||
      typeof run.workflow_path !== "string" || !run.workflow_path) return execution;
  const expectedUrl = `https://github.com/${snapshot.repository}/actions/runs/${run.run_id}/job/${job.job_id}`;
  const signature = executionSignature(job);
  if (safeGithubUrl(job.url, snapshot.repository) !== expectedUrl || signature === null) return execution;
  if (!Array.isArray(snapshot.runs) || snapshot.runs.filter(item => item.run_id === run.run_id && item.attempt === run.attempt).length !== 1 ||
      !Array.isArray(run.jobs) || run.jobs.filter(item => item.job_id === job.job_id).length !== 1) return execution;
  if (run.attempt > 1 && !verifiedAttemptStart(run)) return execution;
  if (jobStart >= attemptStart) {
    return {...execution, originStatus: "current_attempt", originAttempt: run.attempt, originJobId: job.job_id, originUrl: job.url};
  }
  if (job.conclusion !== "success" || jobEnd >= attemptStart) return execution;
  const candidates = [];
  for (const prior of snapshot.runs) {
    if (prior.run_id !== run.run_id || !Number.isSafeInteger(prior.attempt) || prior.attempt < 1 || prior.attempt >= run.attempt || prior.source_sha !== run.source_sha ||
        prior.workflow_path !== run.workflow_path || prior.status !== "completed" || typeof prior.conclusion !== "string") continue;
    if ((prior.attempt > 1 && !verifiedAttemptStart(prior)) || snapshot.runs.filter(item => item.run_id === prior.run_id && item.attempt === prior.attempt).length !== 1) continue;
    const priorStart = timestamp(prior.started_at), priorEnd = timestamp(prior.completed_at);
    if (priorStart === null || priorEnd === null) continue;
    if (!Array.isArray(prior.jobs)) continue;
    for (const earlier of prior.jobs) {
      const times = jobExecutionTimes(earlier);
      const expected = `https://github.com/${snapshot.repository}/actions/runs/${run.run_id}/job/${earlier.job_id}`;
      if (Number.isSafeInteger(earlier.job_id) && earlier.job_id > 0 && times !== null && times.start >= priorStart && times.end <= priorEnd &&
          prior.jobs.filter(item => item.job_id === earlier.job_id).length === 1 &&
          safeGithubUrl(earlier.url, snapshot.repository) === expected &&
          executionSignature(earlier) === signature) candidates.push({attempt: prior.attempt, jobId: earlier.job_id, url: earlier.url});
    }
  }
  if (candidates.length !== 1) return execution;
  return {...execution, originStatus: "carried", originAttempt: candidates[0].attempt,
    originJobId: candidates[0].jobId, originUrl: candidates[0].url};
}
function executionTimeLabel(execution) {
  if (execution.startedAt === null || execution.completedAt === null) return "Image execution time not recorded";
  if (execution.timeKind === "image_steps") return `Image step window ${dateTime(execution.startedAt)} – ${dateTime(execution.completedAt)}`;
  const label = execution.timeKind === "image_step" ? "Image step" : "Image job";
  return `${label} executed ${dateTime(execution.startedAt)} – ${dateTime(execution.completedAt)}`;
}
function executionOriginLabel(execution) {
  if (execution.originStatus === "carried") return `Carried from attempt ${execution.originAttempt}`;
  if (execution.originStatus === "current_attempt") return `Executed in attempt ${execution.originAttempt}`;
  return "Execution origin unverified";
}
function jobExecutionLabel(job) {
  if (jobExecutionTimes(job) === null) return "Job execution time is incomplete or unverified";
  return `Job executed ${dateTime(job.started_at)} – ${dateTime(job.completed_at)}`;
}
function imageMeasurements(snapshot) {
  const unique = new Map(), ambiguous = new Set(), unverified = [];
  for (const run of snapshot.runs) for (const job of run.jobs) {
    if (!measured(job)) continue;
    const execution = imageExecution(snapshot, run, job), item = {run, job, execution};
    if (execution.originStatus === "unverified" || timestamp(execution.startedAt) === null) { unverified.push(item); continue; }
    const key = `${run.run_id}:${execution.originAttempt}:${execution.originJobId}`;
    if (ambiguous.has(key)) { unverified.push(item); continue; }
    if (unique.has(key)) {
      const previous = unique.get(key), before = executionSignature(previous.job), after = executionSignature(job);
      if (before === null || after === null || before !== after) {
        unique.delete(key); ambiguous.add(key); unverified.push(previous, item); continue;
      }
    }
    if (!unique.has(key) || unique.get(key).run.attempt < run.attempt) unique.set(key, item);
  }
  const records = [...unique.values()].sort((a, b) => timestamp(b.execution.startedAt) - timestamp(a.execution.startedAt) || b.run.attempt - a.run.attempt || b.run.run_id - a.run.run_id);
  return {records, unverified};
}
function imageSeries(job) {
  const measurement = imageMeasurement(job);
  const names = measurement.signature === null ? [] : JSON.parse(measurement.signature);
  const styles = names.map(name => Object.hasOwn(IMAGE_SERIES_STYLES, name) ?
    IMAGE_SERIES_STYLES[name] : [IMAGE_STEP_LABELS[name] ?? "Unknown image step", "#536775"]);
  const label = styles.length ? styles.map(style => style[0]).join(" + ") : "Boundary unknown";
  return {
    key: JSON.stringify([job.kind, job.architecture || "unknown", measurement.signature]),
    label: `${kindLabel(job)} · ${label} · ${architecture(job)}`,
    color: styles.length === 1 ? styles[0][1] : "#536775",
  };
}
function selectTrendPoints(points, {series = "all", days = "all", limit = "60", reference = Date.now()} = {}) {
  const cutoff = days === "all" ? null : reference - Number(days) * 86400000;
  const matching = points.filter(item => (series === "all" || imageSeries(item.job).key === series) &&
    (cutoff === null || (timestamp(item.execution?.startedAt) >= cutoff && timestamp(item.execution?.startedAt) <= reference)))
    .sort((a, b) => timestamp(b.execution?.startedAt) - timestamp(a.execution?.startedAt) || b.run.attempt - a.run.attempt || b.run.run_id - a.run.run_id);
  return {matching, points: (limit === "all" ? [...matching] : matching.slice(0, Number(limit))).reverse()};
}
function chartScale(maximum) {
  const target = Math.max(1, maximum * 1.08) / 5;
  const step = [1, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200, 10800, 21600, 43200, 86400]
    .find(value => value >= target) || Math.ceil(target / 86400) * 86400;
  const end = Math.ceil(Math.max(1, maximum * 1.08) / step) * step;
  return {maximum: end, ticks: Array.from({length: end / step + 1}, (_, index) => index * step)};
}
function imageBreakdownLabel(job) {
  const details = (Array.isArray(job.steps) ? job.steps : []).filter(step => Object.hasOwn(IMAGE_STEP_LABELS, step?.name))
    .map(step => normalizeBreakdown(step.breakdown));
  const available = details.filter(detail => detail?.status === "available").length;
  return available ? available === details.length ? "Measured breakdown" : "Partial breakdown" :
    details.some(Boolean) ? "Breakdown unavailable" : "Breakdown not recorded";
}
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
function baseSummary(pr, run) {
  const selected = pr.base_branch ? `${pr.base_branch}@${String(pr.base_target_sha || "unavailable").slice(0, 12)}` : "unavailable";
  const event = (run?.event_bases || []).find(base => base.number === pr.number);
  return {selected: `Selected base: ${selected}`, attempt: event?.sha
    ? `Recorded event base: ${event.ref || "unknown ref"}@${event.sha.slice(0, 12)}. Attempt checkout base is unverified.`
    : "Attempt checkout base is unverified; no recorded event base is available."};
}
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
function link(text, url, repository = state.data.repository) {
  const safe = safeGithubUrl(url, repository);
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
function successfulImages() { return imageMeasurements(state.data).records; }
function setSelected(run, jobId) {
  state.selectedRun = runKey(run);
  state.selectedJob = jobId || null;
  renderCoverage();
  renderRuns();
  renderDetails();
}

function renderMetrics() {
  const measurements = imageMeasurements(state.data), successes = measurements.records;
  const latest = successes[0];
  const measuredPrs = new Set(successes.flatMap(item => prNumbers(item.run)));
  const metrics = [
    ["Latest successful image step", latest ? duration(latest.job.image_build_seconds) : "—", latest ? `${kindLabel(latest.job)} · ${architecture(latest.job)} · ${imageMeasurement(latest.job).label} · ${executionTimeLabel(latest.execution)}` : "Waiting for a verified image execution"],
    ["Successful image executions", successes.length, `Carried retry records counted once · ${measurements.unverified.length} unverified records omitted`],
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
  const measurements = imageMeasurements(state.data);
  const all = measurements.records.filter(item => matchesRun(item.run) && matchesJob(item.job));
  const unverified = measurements.unverified.filter(item => matchesRun(item.run) && matchesJob(item.job)).length;
  const select = byId("series-filter"), old = select.value;
  const series = [...new Map(all.map(item => { const value = imageSeries(item.job); return [value.key, value]; })).values()]
    .sort((a, b) => a.label.localeCompare(b.label));
  const option = el("option", "", "All measurement series (scatter)"); option.value = "all";
  select.replaceChildren(option);
  for (const value of series) {
    const option = el("option", "", value.label); option.value = value.key; select.append(option);
  }
  select.value = series.some(value => value.key === old) ? old : "all";
  const {matching, points} = selectTrendPoints(all, {
    series: select.value, days: byId("date-filter").value, limit: byId("limit-filter").value,
    reference: timestamp(state.data.generated_at) ?? Date.now(),
  });
  const container = byId("trend");
  container.replaceChildren();
  const legend = byId("history-legend"); legend.replaceChildren();
  for (const value of series.filter(value => points.some(item => imageSeries(item.job).key === value.key))) {
    const item = el("span"), dot = el("i"); dot.style.backgroundColor = value.color; dot.setAttribute("aria-hidden", "true");
    item.append(dot, document.createTextNode(value.label)); legend.append(item);
  }
  legend.hidden = !points.length;
  const preview = byId("trend-preview");
  preview.replaceChildren(el("p", "", "Hover or focus a point to inspect its measurement, source revision and breakdown availability. Select it to open the full run details."));
  byId("trend-note").textContent = `${points.length < matching.length ? `Showing the newest ${points.length} of ${matching.length}` : `Showing all ${points.length}`} matching verified image executions; carried retries count once and ${unverified} unverified records are omitted. ${select.value === "all" ? "Scatter overview: select one measurement series to show its trend." : "Lines join only one build path, architecture and exact measured step."} Dates are image step or job execution starts, and date ranges end at the published snapshot. Cache state and runner configuration are not controlled; this is not a performance comparison.`;
  if (!points.length) { container.append(el("p", "empty", "No successful image measurements with verified execution times match these filters.\nFailed, skipped, pending, and unverified records remain visible below.")); return; }
  const padding = getComputedStyle(container);
  const width = Math.max(240, Math.round(container.clientWidth - parseFloat(padding.paddingLeft) - parseFloat(padding.paddingRight)));
  const height = width < 520 ? 250 : 265;
  const left = 55, right = 40, top = 19, bottom = 41;
  const scale = chartScale(points.reduce((maximum, item) => Math.max(maximum, item.job.image_build_seconds), 0));
  const maxSeconds = scale.maximum;
  const firstTime = Math.min(...points.map(item => timestamp(item.execution.startedAt)));
  const lastTime = Math.max(...points.map(item => timestamp(item.execution.startedAt)));
  const x = item => lastTime === firstTime ? (width + left - right) / 2 : left + (timestamp(item.execution.startedAt) - firstTime) / (lastTime - firstTime) * (width - left - right);
  const y = item => top + (1 - item.job.image_build_seconds / maxSeconds) * (height - top - bottom);
  const svg = svgEl("svg", {viewBox: `0 0 ${width} ${height}`, role: "group", "aria-label": "Successful image build durations, ordered by image execution start time. Each point opens run details."});
  for (const value of scale.ticks) {
    const position = top + (1 - value / maxSeconds) * (height - top - bottom);
    svg.append(svgEl("line", {x1: left, y1: position, x2: width - right, y2: position, stroke: "#e6edef", "stroke-dasharray": value ? "3 4" : "0"}), svgEl("text", {x: left - 11, y: position + 4, "text-anchor": "end"}, value ? duration(value).replace(/ 0[ms]$/, "") : "0"));
  }
  const dateCount = lastTime === firstTime ? 1 : width < 400 ? 2 : width < 700 ? 3 : 5;
  for (let index = 0; index < dateCount; index++) {
    const fraction = dateCount === 1 ? .5 : index / (dateCount - 1);
    const tick = firstTime + (lastTime - firstTime) * fraction;
    svg.append(svgEl("text", {x: left + fraction * (width - left - right), y: height - 13, "text-anchor": "middle"}, dateTime(new Date(tick).toISOString(), true)));
  }
  for (const items of select.value === "all" ? [] : imageTrendGroups(points).filter(items => items.length > 1)) {
    const color = imageSeries(items[0].job).color;
    const line = svgEl("polyline", {points: items.map(item => `${x(item)},${y(item)}`).join(" "), fill: "none", stroke: color, "stroke-opacity": ".45", "stroke-width": "2", "stroke-dasharray": items[0].job.architecture === "arm64" ? "5 4" : "0"});
    svg.append(line);
  }
  for (const item of points) {
    const prs = prNumbers(item.run).map(number => `PR #${number}`).join(", ") || item.run.branch || "Branch build";
    const source = String(item.run.source_sha || "unknown").slice(0, 12), breakdown = imageBreakdownLabel(item.job);
    const label = `${prs} · ${imageSeries(item.job).label} · ${duration(item.job.image_build_seconds)} · ${imageMeasurement(item.job).label} · Source ${source} · ${breakdown} · ${executionTimeLabel(item.execution)} · ${executionOriginLabel(item.execution)}`;
    const point = svgEl("g", {class: "chart-point", tabindex: "0", role: "button", "aria-label": label});
    point.append(svgEl("title", {}, label));
    point.append(svgEl("circle", {cx: x(item), cy: y(item), r: 17, fill: "transparent", "pointer-events": "all"}));
    point.append(svgEl("circle", {class: "chart-marker", cx: x(item), cy: y(item), r: 4.5, fill: imageSeries(item.job).color, stroke: "white", "stroke-width": 2, "pointer-events": "none"}));
    const inspect = () => {
      const heading = el("div", "chart-preview-heading");
      heading.append(el("strong", "", duration(item.job.image_build_seconds)), el("span", "", imageSeries(item.job).label));
      preview.replaceChildren(heading,
        el("p", "", `${item.run.workflow_name || "Workflow"} · ${prs} · ${source} · ${executionTimeLabel(item.execution)} · ${executionOriginLabel(item.execution)}`),
        el("p", "", `${imageMeasurement(item.job).label}. ${breakdown}.`));
    };
    point.addEventListener("mouseenter", inspect);
    point.addEventListener("focus", inspect);
    point.addEventListener("click", () => setSelected(item.run, item.job.job_id));
    point.addEventListener("keydown", event => { if (["Enter", " "].includes(event.key)) { event.preventDefault(); setSelected(item.run, item.job.job_id); } });
    svg.append(point);
  }
  container.append(svg);
}

function coverageRows(data) {
  return prCoverage(data).sort((a, b) => Number(!!b.latestImageRun) - Number(!!a.latestImageRun)
    || (b.latestImageRun ? runTime(b.latestImageRun) : 0) - (a.latestImageRun ? runTime(a.latestImageRun) : 0)
    || b.pr.number - a.pr.number);
}
function renderCoverage() {
  const coverage = coverageRows(state.data);
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
    title.append(button, el("span", "subtext", prStateLabel(pr)));
    title.append(el("span", "subtext sha", baseSummary(pr, latestImageRun).selected));
    const status = el("td"), timings = el("td");
    const stack = el("div", "status-stack");
    for (const job of jobs) {
      const execution = imageExecution(state.data, latestImageRun, job);
      const item = el("div");
      item.append(badge(result(job)), el("span", "subtext", `${kindLabel(job)} · ${architecture(job)}`));
      stack.append(item);
      const buildSeconds = completedImageSeconds(job);
      const time = el("div", "", duration(buildSeconds));
      time.append(el("span", "subtext", buildSeconds !== null ? "build step passed" : "no completed build timing"));
      time.append(el("span", "subtext", executionTimeLabel(execution)));
      const origin = el("span", "subtext");
      origin.append(execution.originStatus === "carried" ? link(`${executionOriginLabel(execution)} ↗`, execution.originUrl) : document.createTextNode(executionOriginLabel(execution)));
      time.append(origin);
      timings.append(time);
    }
    status.append(stack);
    const current = !!pr.head_sha && latestImageRun.source_sha === pr.head_sha;
    title.append(el("span", "subtext sha", String(latestImageRun.source_sha || "unknown").slice(0, 10)), badge(current ? "current" : "historical", current ? "Current PR head" : "Different revision"));
    title.append(el("span", "subtext", "Attempt base unverified"));
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
    const time = el("td", "", dateTime(run.started_at));
    time.append(el("span", "subtext", attemptTimeSourceLabel(run)));
    row.append(workflow, prs, status, time, el("td", "", run.attempt));
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

function recipeRows(value) {
  if (value?.status !== "available") return null;
  const legacy = value.version === 1;
  const rows = legacy ? value.images : value.recipes;
  if ((!legacy && value.version !== 2) || !Array.isArray(rows) || rows.length > 1024 ||
      (legacy && rows.length !== LEGACY_RECIPE_OWNERS.size)) return null;
  const owners = new Set(), result = [];
  for (const raw of rows) {
    if (!raw || typeof raw.owner !== "string" || typeof raw.failed !== "boolean" || owners.has(raw.owner) ||
        !/^[A-Za-z0-9][A-Za-z0-9_.+-]{0,191}$/.test(raw.owner) ||
        (legacy && !LEGACY_RECIPE_OWNERS.has(raw.owner))) return null;
    const row = legacy ? {...raw, bazel_expected: 3, profiles_expected: 3, exports_expected: 2} : raw;
    for (const [metric, , count, expected] of RECIPE_METRICS) {
      if (!Number.isInteger(row[expected]) || row[expected] < 0 || row[expected] > 65535 ||
          !Number.isInteger(row[count]) || row[count] < 0 || row[count] > row[expected]) return null;
      if (row[count] === 0 ? row[metric] !== null :
          seconds(row[metric]) === null || row[metric] > Number.MAX_SAFE_INTEGER) return null;
    }
    if (Object.hasOwn(raw, "build") && (legacy || !validBuildEvidence(raw.build, row))) return null;
    if (Object.hasOwn(raw, "details") && (legacy || !validNamedEvidence(raw.details, row))) return null;
    owners.add(row.owner);
    // Only Bazel commands and the separate exports contribute to this sum.
    const recorded = row.bazel_phases || row.exports ? (row.bazel_seconds ?? 0) + (row.export_seconds ?? 0) : null;
    result.push({...row, recorded});
  }
  return result.sort((a, b) => (b.recorded ?? -1) - (a.recorded ?? -1) || a.owner.localeCompare(b.owner));
}

function validBuildEvidence(build, row) {
  if (build === null) return true;
  const metrics = ["lock_seconds", ...Object.keys(BUILD_EVIDENCE)];
  const fields = ["expected", "timings", "profiles", "seconds", ...metrics];
  if (!build || typeof build !== "object" || Object.keys(build).length !== fields.length ||
      fields.some(key => !Object.hasOwn(build, key)) ||
      !Number.isInteger(build.expected) || build.expected < 1 || build.expected > row.bazel_expected ||
      !Number.isInteger(build.timings) || build.timings < 0 || build.timings > Math.min(build.expected, row.bazel_phases) ||
      !Number.isInteger(build.profiles) || build.profiles < 0 || build.profiles > Math.min(build.expected, row.profiles)) return false;
  for (const [count, keys] of [["timings", ["seconds"]], ["profiles", metrics]]) {
    if (keys.some(key => build[count] ? seconds(build[key]) === null || build[key] > Number.MAX_SAFE_INTEGER : build[key] !== null)) return false;
  }
  return build.timings !== build.expected || build.profiles !== build.expected || build.lock_seconds <= build.seconds;
}

function buildNonlock(row) {
  const b = row.build;
  return b && b.expected === b.timings && b.expected === b.profiles ? b.seconds - b.lock_seconds : null;
}

const CRITICAL_KINDS = {
  OCIImage: "OCI image", OCITarball: "OCI tarball", OCITarballManifest: "OCI tar manifest",
  Gzip: "Gzip compression", APTDebug: "Select debug APT additions", APTRuntime: "Select runtime APT additions",
  ValidatePackages: "Validate installed package identities", ValidateSource: "Validate source/runtime identities",
  GoCompilePkg: "Go package compilation", GoLink: "Go linking", GoStdlib: "Go standard library",
  GoToolchainBinaryBuild: "Go toolchain builder", PackageControls: "Derive Debian package controls",
  Unpack: "Unpack package", KernelBuild: "Kernel package action", KernelTools: "Prepare kernel build tools", Genrule: "Genrule",
};
function evidenceFields(value, fields) {
  return value && typeof value === "object" && !Array.isArray(value) &&
    Object.keys(value).length === fields.length && fields.every(key => Object.hasOwn(value, key));
}
function publicTarget(value) {
  return value === null || typeof value === "string" && value.length <= 240 &&
    /^(?:(?:@{1,2}[A-Za-z0-9_.+~-]+)?\/\/[A-Za-z0-9_+./~-]*:[A-Za-z0-9_+./~-]+|(?:platform|dockers|tools|src|external)\/[A-Za-z0-9_+./~-]+)$/.test(value) &&
    !/\s/.test(value) && !value.split(/[/:]/).some(part => part === "." || part === "..");
}
function validNamedEvidence(value, row) {
  const groups = ["repositories", "critical_path", "phases"], numeric = value => seconds(value) !== null && value <= Number.MAX_SAFE_INTEGER;
  if (!evidenceFields(value, ["scope", "expected", "profiles", "phase_count", ...groups]) ||
      !["build", "kernel-build", "unavailable"].includes(value.scope) ||
      ["expected", "profiles", "phase_count"].some(key => !Number.isInteger(value[key]) || value[key] < 0 || value[key] > (key === "phase_count" ? 131070 : 65535)) ||
      value.profiles > Math.min(value.expected, row.profiles) || value.phase_count !== row.bazel_phases + row.exports ||
      groups.some(key => !Array.isArray(value[key]) || value[key].length > 8) ||
      value.phases.length !== Math.min(8, value.phase_count)) return false;
  if (value.scope === "build" && (!row.build || value.expected !== row.build.expected || value.profiles !== row.build.profiles) ||
      value.scope === "kernel-build" && (row.owner !== "linux-kernel" || row.build != null) ||
      value.scope === "unavailable" && (value.expected || value.profiles) ||
      !value.profiles && (value.repositories.length || value.critical_path.length)) return false;
  return value.repositories.every(span => evidenceFields(span, ["name", "seconds", "read_count", "read_seconds"]) &&
    typeof span.name === "string" && /^\/\/:[A-Za-z0-9_.+~-]{1,188}$/.test(span.name) && !/\s/.test(span.name) &&
    numeric(span.seconds) && numeric(span.read_seconds) && span.read_seconds <= span.seconds + 1e-6 &&
    Number.isInteger(span.read_count) && span.read_count >= 0 && span.read_count <= 1000000) &&
    value.critical_path.every(span => evidenceFields(span, ["kind", "target", "seconds"]) &&
      typeof span.kind === "string" && Object.hasOwn(CRITICAL_KINDS, span.kind) && publicTarget(span.target) && numeric(span.seconds)) &&
    value.phases.every(phase => evidenceFields(phase, ["phase", "seconds", "lock_seconds", "start_epoch_seconds", "exit_code"]) &&
      typeof phase.phase === "string" && /^[A-Za-z0-9][A-Za-z0-9_.+-]{0,191}$/.test(phase.phase) && !/\s/.test(phase.phase) &&
      numeric(phase.seconds) && (phase.lock_seconds === null || numeric(phase.lock_seconds)) &&
      (phase.start_epoch_seconds === null || numeric(phase.start_epoch_seconds) && phase.start_epoch_seconds <= 253402300799) &&
      Number.isInteger(phase.exit_code) && phase.exit_code >= 0 && phase.exit_code <= 255);
}
function readable(elapsed) {
  if (elapsed === null || elapsed === undefined) return "Unavailable";
  if (elapsed > 0 && elapsed < 0.01) return "<0.01s";
  if (elapsed > 0 && elapsed < 1) return elapsed.toFixed(2) + "s";
  const rounded = Math.round(elapsed), hours = Math.floor(rounded / 3600), minutes = Math.floor(rounded % 3600 / 60), secs = rounded % 60;
  return hours ? `${hours}h${String(minutes).padStart(2, "0")}m${String(secs).padStart(2, "0")}s` :
    minutes ? `${minutes}m${String(secs).padStart(2, "0")}s` : `${secs}s`;
}
function measuredDuration(elapsed) {
  return readable(elapsed) + (elapsed >= 1 ? ` (${elapsed.toFixed(2)}s)` : "");
}
function cardTime(row) {
  return buildNonlock(row) ?? (row.owner === "linux-kernel" && row.bazel_phases === row.bazel_expected ? row.bazel_seconds : null);
}
function buildDiagnosis(row) {
  const b = row.build;
  if (!b) return row.owner === "linux-kernel" ? "Isolated kernel launcher" : "Detailed profiling unavailable";
  if (!b.profiles) return "Build profile unavailable";
  const critical = row.details?.critical_path ?? [], repositories = row.details?.repositories ?? [];
  const debug = critical.some(span => ["Gzip", "OCIImage", "OCITarball"].includes(span.kind) && span.target?.includes("-dbg"));
  const primary = Object.keys(BUILD_EVIDENCE).sort((a, c) => (b[c] ?? 0) - (b[a] ?? 0))[0];
  if (!b[primary]) return "Analysis/setup not fully attributed";
  if (["repository_seconds", "repository_read_seconds"].includes(primary)) {
    if (repositories.some(span => span.name.includes("++gcc+") || span.name.includes("++sysroots+"))) return "Toolchain and repository preparation";
    return b.packaging_seconds ? `Repository preparation + ${debug ? "debug" : "image"} packaging` : "Repository preparation";
  }
  if (primary === "compile_seconds" && critical.some(span => span.kind === "GoCompilePkg") && critical.some(span => span.kind === "GoLink")) return "Go compilation and linking";
  return {
    packaging_seconds: b.validation_seconds ? `${debug ? "Debug" : "Image"} packaging and validation` : "Image/archive processing",
    compile_seconds: "Source compilation", foreign_seconds: "Nested build subprocesses",
    package_selection_seconds: "APT selection and package assembly", validation_seconds: "Source and package identity validation",
    cache_write_seconds: "Cache I/O and output processing", fetch_seconds: "Cache input/output fetch",
  }[primary] || BUILD_EVIDENCE[primary];
}

function buildExplanation(row) {
  const b = row.build;
  if (!b) return row.owner === "linux-kernel" ?
    "Separate kernel launcher: bootstrap, build, output query and module resolution; not a directly comparable image build." :
    "Detailed build-only evidence was not recorded.";
  if (!b.profiles) return "Build profile unavailable; no cause inferred from elapsed time.";
  const evidence = Object.entries(BUILD_EVIDENCE).filter(([key]) => b[key] > 0).sort((a, c) => b[c[0]] - b[a[0]]);
  let text = evidence.length ? `Largest recorded work category: ${evidence[0][1]} (${readable(b[evidence[0][0]])}${b.profiles < b.expected ? ", partial" : ""}).` :
    "No matching work spans recorded; remaining analysis/setup work is not attributed.";
  if (buildNonlock(row) !== null) {
    if (b.lock_seconds > b.seconds / 2) text = "Most build-command elapsed time was server lock waiting. " + text;
    if (row.profiles === row.profiles_expected && row.lock_seconds > b.lock_seconds) {
      text += ` Other-command lock wait: ${recipeSeconds(row.lock_seconds - b.lock_seconds)}.`;
    }
  }
  return text;
}

function evidenceTable(title, columns, rows, empty) {
  const section = el("section", "target-evidence");
  section.append(el("h4", "", title));
  if (!rows.length) { section.append(el("p", "stage-note", empty)); return section; }
  const scroll = el("div", "table-scroll"), table = el("table", `target-evidence-table${columns.length === 5 ? " command-receipts" : ""}`), head = el("thead"), tr = el("tr"), body = el("tbody");
  scroll.tabIndex = 0; scroll.setAttribute("role", "region"); scroll.setAttribute("aria-label", title);
  for (const column of columns) {
    const th = el("th", "", column); th.setAttribute("scope", "col"); tr.append(th);
  }
  head.append(tr);
  for (const values of rows) {
    const tr = el("tr");
    for (const value of values) tr.append(el("td", "", value));
    body.append(tr);
  }
  table.append(head, body); scroll.append(table); section.append(scroll);
  return section;
}

function renderBuildExplanations(rows, context = "") {
  const box = el("section", "recipe-explanations");
  const heading = el("div", "target-analysis-heading"), title = el("div"), actions = el("div", "target-actions"), cards = [];
  title.append(el("h2", "", "Why each target took its time"),
    el("p", "stage-note", `${context ? context + " · " : ""}${rows.length} targets · ordered by build elapsed excluding lock. Kernel launcher elapsed has a different scope.`));
  for (const [label, open] of [["Expand visible", true], ["Collapse visible", false]]) {
    const button = el("button", "", label); button.type = "button";
    button.addEventListener("click", () => { for (const card of cards) card.open = open; }); actions.append(button);
  }
  heading.append(title, actions); box.append(heading);
  for (const row of [...rows].sort((a, b) => (cardTime(b) ?? -1) - (cardTime(a) ?? -1) || a.owner.localeCompare(b.owner))) {
    const card = el("details", "target-card"), summary = el("summary", "target-card-summary"), name = el("span", "target-name");
    const build = row.build, details = row.details, scope = row.owner === "linux-kernel" && !build ? "Launcher elapsed" : "Build excluding lock";
    name.append(el("strong", "", row.owner + (row.owner === "linux-kernel" ? " *" : "")), el("span", "diagnosis-badge", buildDiagnosis(row)));
    if (row.failed) name.append(el("small", "recipe-failed", "Command failed"));
    const time = el("span", "target-duration"); time.append(el("strong", "", readable(cardTime(row))), el("span", "", scope));
    summary.append(name, time);
    const body = el("div", "target-card-body"), metrics = el("dl", "target-metrics");
    body.append(el("p", "target-interpretation", buildExplanation(row)));
    const otherLock = build && build.profiles === build.expected && row.profiles === row.profiles_expected ?
      Math.max(0, row.lock_seconds - build.lock_seconds) : null;
    for (const [label, value] of [
      [scope, cardTime(row)], ["Build elapsed", build?.seconds ?? null], ["Build lock", build?.lock_seconds ?? null],
      ["Other-command lock", otherLock], ["All command elapsed", row.bazel_seconds], ["Exports", row.export_seconds],
    ]) {
      const metric = el("div"); metric.append(el("dt", "", label), el("dd", "", measuredDuration(value))); metrics.append(metric);
    }
    body.append(metrics);
    if (build) {
      body.append(el("p", "stage-note", `Build coverage: ${build.timings}/${build.expected} receipts; ${build.profiles}/${build.expected} profiles. All-command/export metrics retain recorded partial coverage shown in Run details.`),
        evidenceTable("Recorded work categories", ["Category", "Observed span coverage"],
          Object.entries(BUILD_EVIDENCE).filter(([key]) => build[key] > 0).map(([key, label]) => [label, measuredDuration(build[key])]),
          "No matching work-category spans recorded."));
    }
    if (details) {
      body.append(el("p", "stage-note", `Named evidence: ${details.profiles}/${details.expected} build profiles. Up to eight longest recognized repository/action spans are retained, not an exhaustive partition.${details.scope === "kernel-build" ? " Kernel spans describe its build profile only, not the whole launcher." : ""}`),
        evidenceTable("Longest repository spans", ["Repository", "Span", "Nested reads/waits"],
          details.repositories.map(span => [span.name, measuredDuration(span.seconds), `${span.read_count} reads; ${measuredDuration(span.read_seconds)}`]),
          details.profiles ? "No recognized repository spans recorded." : "Build profile unavailable."),
        evidenceTable("Longest critical-path actions", ["Action", "Target / output", "Span"],
          details.critical_path.map(span => [CRITICAL_KINDS[span.kind], span.target ?? "—", measuredDuration(span.seconds)]),
          details.profiles ? "No recognized critical-path actions recorded." : "Build profile unavailable."),
        evidenceTable(`Command receipts (${details.phases.length}/${details.phase_count} shown)`, ["Phase", "Requested (UTC)", "Elapsed", "Lock", "Exit"],
          details.phases.map(phase => [phase.phase, phase.start_epoch_seconds === null ? "Unavailable" :
            new Date(phase.start_epoch_seconds * 1000).toISOString().replace("T", " "), measuredDuration(phase.seconds),
          measuredDuration(phase.lock_seconds), String(phase.exit_code)]), "No command receipts recorded."),
        el("p", "stage-note", "Receipts include server lock waiting; queries and exports are not included in the build-only ranking. Missing lock profiles are not zero. Receipt tables scroll horizontally on narrow screens."));
    } else body.append(el("p", "stage-note", "Named spans were not recorded for this historical run."));
    card.append(summary, body); cards.push(card); box.append(card);
  }
  box.append(el("p", "stage-note", "Evidence spans overlap and must not be added. Repository reads can wait for other repositories; packaging and critical-path actions can include cached-output materialization rather than execution. Cache writes do not establish remote publication. ARM repository spans do not prove unnecessary work or achievable savings. Unattributed analysis/setup work remains. * Kernel launcher elapsed is not directly comparable to image build-only time."));
  return box;
}

const recipeNumber = new Intl.NumberFormat(undefined, {maximumFractionDigits: 3});
function recipeSeconds(elapsed) {
  if (elapsed === null) return "—";
  return elapsed > 0 && elapsed < 0.001 ? "<0.001s" : recipeNumber.format(elapsed) + "s";
}

function containerMembers(rows, detail) {
  const images = rows.filter(row => row.owner.endsWith(".gz") && row.exports_expected > 0);
  const byOwner = new Map(images.map(row => [row.owner, row]));
  const fallback = () => ({profile: null, members: images
    .filter(row => row.owner.startsWith("docker-") && row.bazel_expected === 0 && row.profiles_expected === 0)
    .map(recipe => ({recipe}))});
  const validTime = value => seconds(value) !== null && value <= Number.MAX_SAFE_INTEGER;
  if (!Array.isArray(detail?.rows) || !detail.rows.length || detail.rows.length > 1024 ||
      ![detail.other_action_seconds, detail.no_action_seconds, detail.parallel_seconds].every(validTime)) return fallback();
  const seen = new Set(), members = [];
  for (const member of detail.rows) {
    const recipe = byOwner.get(member?.owner);
    if (!recipe || seen.has(member.owner) || !Number.isSafeInteger(member.actions) || member.actions < 0 ||
        ![member.action_seconds, member.archive_seconds, member.compress_seconds, member.fetch_seconds].every(validTime)) return fallback();
    seen.add(member.owner);
    members.push({...member, recipe});
  }
  members.sort((a, b) => b.action_seconds - a.action_seconds || a.recipe.owner.localeCompare(b.recipe.owner));
  return {profile: detail, members};
}

function renderContainerMembers({profile, members}, wallSeconds) {
  const box = el("section", "container-members");
  box.append(el("h4", "", `Containers (${members.length})`));
  if (!members.length) {
    box.append(el("p", "stage-note", "No per-container measurements were recorded for this batch."));
    return box;
  }
  if (profile) {
    box.append(el("p", "stage-note", "Active time counts intervals with at least one observed action owned by the image. Images run in parallel. Cache fetch, OCI archive and gzip can overlap within active time. Make exports the archives after Bazel returns."));
    const totals = el("dl", "recipe-metrics");
    if (wallSeconds !== null && wallSeconds >= profile.other_action_seconds + profile.no_action_seconds) {
      const imageWall = wallSeconds - profile.other_action_seconds - profile.no_action_seconds;
      totals.append(el("dt", "", "One or more image owners active"), el("dd", "", recipeSeconds(imageWall)));
    }
    for (const [key, label] of [["other_action_seconds", "Only shared or unattributed actions active"],
      ["no_action_seconds", "No action recorded"]]) {
      totals.append(el("dt", "", label), el("dd", "", recipeSeconds(profile[key])));
    }
    box.append(totals, el("p", "stage-note", `Two or more image owners were active together for ${recipeSeconds(profile.parallel_seconds)} of the measured image time.`));
  } else {
    box.append(el("p", "stage-note", "This run retained per-container export times. Per-container Bazel action times are unavailable."));
  }
  const scroll = el("div", "table-scroll"), table = el("table"), head = el("thead"), header = el("tr"), body = el("tbody");
  scroll.setAttribute("tabindex", "0");
  scroll.setAttribute("role", "region");
  scroll.setAttribute("aria-label", "Container timings");
  const columns = profile ? ["Image", "Actions", "Active", "Cache fetch", "OCI archive", "Gzip", "Export"] : ["Image", "Export"];
  for (const label of columns) {
    const th = el("th", "", label); th.setAttribute("scope", "col"); header.append(th);
  }
  head.append(header);
  for (const member of members) {
    const recipe = member.recipe, tr = el("tr"), name = el("th", "", recipe.owner);
    name.setAttribute("scope", "row");
    if (recipe.failed) name.append(el("small", "recipe-failed", "Command failed"));
    tr.append(name);
    if (profile) {
      tr.append(el("td", "", recipeNumber.format(member.actions)));
      for (const metric of ["action_seconds", "fetch_seconds", "archive_seconds", "compress_seconds"]) {
        tr.append(el("td", "", recipeSeconds(member[metric])));
      }
    }
    const exported = el("td", "", recipeSeconds(recipe.export_seconds));
    if (recipe.exports < recipe.exports_expected) {
      exported.append(el("small", "recipe-sample", ` (${recipe.exports}/${recipe.exports_expected})`));
    }
    if (recipe.bazel_expected || recipe.profiles_expected) exported.append(el("small", "recipe-sample", " (all invocations)"));
    tr.append(exported); body.append(tr);
  }
  table.append(head, body); scroll.append(table); box.append(scroll);
  return box;
}

function renderRecipeRows(rows, legacy, detail) {
  const box = el("section", "recipe-list");
  box.append(el("h3", "", legacy ? "Bazel timings for four image recipes" : "Bazel build breakdown"));
  if (!rows.length) {
    box.append(el("p", "stage-note", "No Bazel activity was recorded during this build."));
    return box;
  }
  box.append(el("p", "stage-note", "Recipes are ordered by recorded Bazel and export command time. Expand a recipe for its breakdown."));
  const list = el("ol"), group = rows.find(row => row.owner === "vs-container-images");
  const batch = group && containerMembers(rows, detail);
  const exportsInBatch = new Set(batch?.members.filter(({recipe}) =>
    recipe.bazel_expected === 0 && recipe.profiles_expected === 0).map(({recipe}) => recipe.owner));
  const topRows = rows.filter(row => !exportsInBatch.has(row.owner));
  for (const row of topRows) {
    const item = el("li"), details = el("details");
    details.open = row === topRows[0];
    const summary = el("summary", "timing-row");
    const name = el("span", "timing-name", row.owner);
    if (row.failed) name.append(el("small", "recipe-failed", "Command failed"));
    const sum = el("span", "timing-duration", recipeSeconds(row.recorded));
    if (row.recorded !== null && (row.bazel_phases < row.bazel_expected || row.exports < row.exports_expected)) {
      sum.append(el("small", "recipe-sample", "Partial"));
    }
    summary.append(name, sum);
    const metrics = el("dl", "recipe-metrics");
    for (const [metric, title, count, expected] of RECIPE_METRICS) {
      const cell = el("dd", "", row[expected] === 0 ? "N/A" : recipeSeconds(row[metric]));
      if (row[count] < row[expected]) cell.append(el("small", "recipe-sample", ` (${row[count]}/${row[expected]})`));
      metrics.append(el("dt", "", title), cell);
    }
    details.append(summary, metrics);
    if (row === group) details.append(renderContainerMembers(batch, row.bazel_seconds));
    item.append(details); list.append(item);
  }
  const notes = ["Lock waits, compiler subprocesses, nested builds and cache reads can overlap. The Bazel total also includes work such as analysis and linking. Cache fetch includes disk and remote reads."];
  if (rows.some(row => row.owner === "linux-kernel")) {
    notes.push("Kernel time includes launcher setup and its nested build covers kernel Make as a whole.");
  }
  notes.push("Fractions show recorded coverage. A dash means missing data, N/A means no expected operation, and zero is a measured value.");
  box.append(list, el("p", "stage-note", notes.join(" ")));
  return box;
}

function renderRecipeTimings(job, repository) {
  if (job.kind !== "make-vs" || !job.steps.some(step => step.name === HOST_VS_BUILD_STEP)) return null;
  const value = job.image_recipe_timings, rows = recipeRows(value);
  const box = el("div", "recipe-timings");
  if (rows) {
    box.append(renderRecipeRows(rows, value.version === 1, value.container_breakdown));
  } else {
    let reason = value === undefined ? "not-recorded" : "invalid-timing";
    if ((value?.version === 1 || value?.version === 2) && value.status === "unavailable" &&
        typeof value.reason === "string" && Object.hasOwn(RECIPE_REASONS, value.reason)) reason = value.reason;
    box.append(el("p", "stage-note", RECIPE_REASONS[reason]));
  }
  const upload = job.steps.find(step => ["Preserve VS Bazel profiles, timings and Make logs",
    "Preserve VS image build profiles, timings and Make logs"].includes(step.name) &&
    step.conclusion === "success" && Number.isSafeInteger(step.number) && step.number > 0);
  const jobUrl = safeGithubUrl(job.url, repository);
  if (upload && jobUrl) {
    const url = new URL(jobUrl);
    url.hash = `step:${upload.number}:1`;
    box.append(link("Profiles and Make logs · upload log ↗", url.href, repository));
  }
  return box;
}

function renderDetails() {
  const box = byId("details"); box.replaceChildren();
  const analysis = byId("target-analysis");
  if (analysis) { analysis.replaceChildren(); analysis.hidden = true; }
  const run = state.data.runs.find(item => runKey(item) === state.selectedRun);
  if (!run) { box.append(el("p", "empty", "Select a chart point, pull request, or workflow run to see its timing breakdown.")); return; }
  const heading = el("p", "detail-meta");
  heading.append(link(`${run.workflow_name || "Workflow"} ↗`, run.url), badge(result(run)), el("span", "subtext", attemptTimeLabel(run)), el("span", "subtext sha", `Source ${String(run.source_sha || "unknown").slice(0, 12)} · ${run.event || "unknown event"}`));
  box.append(heading);
  if (run.pr_association_note) box.append(el("p", "source-warning", run.pr_association_note));
  const selectedPr = Number(byId("pr-filter").value);
  const pr = state.data.pull_requests.find(item => item.number === selectedPr) || state.data.pull_requests.find(item => prNumbers(run).includes(item.number));
  if (pr) {
    const note = el("p", "detail-meta");
    note.append(link(`PR #${pr.number}`, pr.url), document.createTextNode(` · association: ${run.pr_association || "unknown"}`));
    box.append(note);
    const base = baseSummary(pr, run);
    box.append(el("p", "detail-meta", `${base.selected} · observed ${dateTime(pr.base_target_observed_at)}`), el("p", "source-warning", base.attempt));
    box.append(el("p", "stage-note", "Successful step timings do not establish that an image artifact was published or that this attempt tested the selected base."));
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
  if (imageJob(job)) {
    const execution = imageExecution(state.data, run, job);
    box.append(el("p", "detail-meta", execution.timeKind === "image_job" ? "Image step time not recorded" : executionTimeLabel(execution)), el("p", "detail-meta", jobExecutionLabel(job)));
    const origin = el("p", "detail-meta");
    origin.append(execution.originStatus === "carried" ? link(`${executionOriginLabel(execution)} · original job ${execution.originJobId} ↗`, execution.originUrl) : document.createTextNode(executionOriginLabel(execution)));
    box.append(origin);
  }
  const values = el("div", "detail-values");
  for (const [name, value] of [["Successful image build step", completedImageSeconds(job)], ["Total job elapsed", job.duration_seconds]]) {
    const valueBox = el("div", "detail-value"); valueBox.append(el("span", "", name), el("strong", "", duration(value))); values.append(valueBox);
  }
  box.append(values);
  if (completedImageSeconds(job) !== null) box.append(el("p", "stage-note", `Measured scope: ${imageMeasurement(job).label}.`));
  const phases = job.phases || {};
  const total = PHASES.reduce((sum, phase) => sum + (seconds(phases[phase]) || 0), 0);
  const bar = el("div", "stage-bar"); bar.setAttribute("aria-label", "Recorded step time by phase");
  const legend = el("div", "stage-legend");
  for (const phase of PHASES) {
    const value = seconds(phases[phase]);
    if (total > 0 && value !== null) { const segment = el("span", `stage-${phase}`); segment.style.width = `${value / total * 100}%`; segment.title = `${phase}: ${duration(value)}`; bar.append(segment); }
    const item = el("span"); item.append(el("i", `stage-${phase}`), document.createTextNode(phase[0].toUpperCase() + phase.slice(1)), el("b", "", duration(value))); legend.append(item);
  }
  box.append(bar, legend, el("p", "stage-note", "Stage totals sum recorded step durations. They may not equal job elapsed time, and the build phase can include more work than the image build step. Step times exclude queue time. Total elapsed can include waiting for a runner when a job never starts. Cache state and runner comparability are unknown."));
  const recipes = renderRecipeTimings(job, state.data.repository);
  if (recipes) box.append(recipes);
  const targetRows = recipes && recipeRows(job.image_recipe_timings);
  if (analysis && targetRows?.length) {
    analysis.append(renderBuildExplanations(targetRows, `Run ${run.run_id} · ${job.name}`));
    analysis.hidden = false;
  }
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
  for (const id of ["series-filter", "date-filter", "limit-filter"]) byId(id).addEventListener("change", () => { if (state.data) renderTrend(); });
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
if (typeof module !== "undefined") module.exports = {seconds, duration, timestamp, safeGithubUrl, validateSnapshot, measured, completedImageSeconds, imageMeasurement, imageTrendGroups, imageSeries, selectTrendPoints, chartScale, imageBreakdownLabel, attemptTimeLabel, imageExecution, executionTimeLabel, executionOriginLabel, jobExecutionLabel, imageMeasurements, imageResult, prCoverage, summarizePrCoverage, normalizeBreakdown, normalizePhaseBreakdown, renderStep, renderRecipeTimings, recipeRows, renderBuildExplanations, coverageRows, baseSummary, prStateLabel};
