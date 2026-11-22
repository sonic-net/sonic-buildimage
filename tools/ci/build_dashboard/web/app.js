"use strict";

// Public CI metadata only. Text is rendered through DOM textContent, never HTML.
const FULL_COMMIT_SHA = /^(?:[0-9a-f]{40}|[0-9a-f]{64})$/i;
const IMAGE_KINDS = new Set(["make-vs", "bazel-vs"]);
const PENDING_JOB_TEXT = {
  queued: "is queued for a runner", in_progress: "is running", requested: "has been requested",
  waiting: "is waiting to start", pending: "is pending",
};
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
const IMAGE_SERIES_LABELS = {
  [HOST_VS_BUILD_STEP]: "Host Make .bin + archives",
  "Build the VS installer (.bin)": "Outer Make .bin",
  "Build sonic-vs.bin directly with slave.mk": "Direct slave .bin",
  "Build the VS image": "Legacy VM image",
  "Build the VS image with Bazel SWSS archives": "Legacy VM + SWSS archives",
  "Build the VS image and both OCI container variants": "Legacy image + OCI validation",
  "Build the Bazel VS installer": "Installer build",
  "Build Bazel VS installer": "Installer build (earlier step)",
  "Build and verify sonic-vs.bin with Bazel": "Installer + verification",
};
const BUILD_SOURCES = {
  master: {label: "Master", color: "#187247"},
  pr: {label: "Pull request", color: "#facc15", outline: "#805f00"},
  other: {label: "Other branch", color: "#536775"},
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
  "device-data": "SONiC device data package",
  "config-engine-load": "Load the config-engine base image",
  "swss-layer-load": "Load the SWSS base image",
  "kernel-prep": "Kernel package preparation",
  "bookworm-make": "Bookworm Make invocation",
  "prep-deb": "Build or reuse Debian packages",
  "prep-deb-install": "Install Debian packages",
  "prep-wheel": "Build or reuse Python wheels",
  "prep-wheel-install": "Install Python wheels",
  "trixie-cache-eval": "Evaluate Trixie package cache",
  "container-manifest": "Generate container manifests",
  "p4rt-prep": "P4RT source and SDK preparation",
  "p4rt-audit": "P4RT dependency graph audit",
  "p4rt-sdk": "P4RT SDK inputs",
  "p4rt-owner": "P4RT source build",
  "p4rt-handoff": "P4RT artifact handoff",
  "rootfs-prepare": "Prepare the stage-one root filesystem",
  "rootfs-compress": "Compress the intermediate squashfs",
  "rootfs-cache-save": "Save the intermediate squashfs to cache",
  "build-version-collection": "Collect build-version metadata",
  "builder-exit": "Exit the builder and return to the host",
  "trixie-image-cleanup": "Trixie image cleanup",
  "bookworm-cleanup": "Bookworm cleanup invocation",
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
const IMAGE_CONTEXT_LABELS = {"docker-load": "Load and tag Docker images", "docker-store": "Compress the Docker store", payload: "Build the payload ZIP", recipe: "Recipe", filesystem: "Filesystem and payload", installer: "ONIE installer", bios: "BIOS", uefi: "UEFI", "4-asic": "4 ASIC", "6-asic": "6 ASIC"};
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
  ["export_seconds", "Output handoff", "exports", "exports_expected"],
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
function duration(value, includeSeconds = false) {
  const number = seconds(value);
  if (number === null) return "—";
  if (number < 60) return `${Math.round(number)}s`;
  const rounded = Math.round(number);
  return rounded >= 3600 ? `${Math.floor(rounded / 3600)}h ${Math.floor(rounded % 3600 / 60)}m${includeSeconds ? ` ${rounded % 60}s` : ""}` : `${Math.floor(rounded / 60)}m ${rounded % 60}s`;
}
function timestamp(value) { const parsed = Date.parse(value || ""); return Number.isFinite(parsed) ? parsed : null; }
function dateTime(value, short = false) {
  if (timestamp(value) === null) return "Not recorded";
  return new Date(value).toLocaleString(undefined, short ? {month: "short", day: "numeric"} : {month: "short", day: "numeric", year: "numeric", hour: "numeric", minute: "2-digit", timeZoneName: "short"});
}
function result(item) { return item.conclusion || item.status || "unknown"; }
function resultLabel(value) { return String(value || "unknown").replaceAll("_", " "); }
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
    const key = signature === null ? item : JSON.stringify([buildSource(item.run), item.job.kind, item.job.architecture || "unknown", signature]);
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
  const labels = names.map(name => Object.hasOwn(IMAGE_SERIES_LABELS, name) ? IMAGE_SERIES_LABELS[name] : IMAGE_STEP_LABELS[name]);
  const label = labels.length ? labels.join(" + ") : "Boundary unknown";
  return {
    key: JSON.stringify([job.kind, job.architecture || "unknown", measurement.signature]),
    label: `${kindLabel(job)} · ${label} · ${architecture(job)}`,
  };
}
function selectTrendPoints(points, {series = "all", days = "all", limit = "10", reference = Date.now()} = {}) {
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
  if (outcomes.some(value => Object.hasOwn(PENDING_JOB_TEXT, value))) return "pending";
  // An unknown or incomplete job result must not turn into a successful or skipped PR.
  if (outcomes.some(value => !["success", "cancelled", "skipped"].includes(value))) return "unmeasured";
  if (outcomes.includes("cancelled")) return "cancelled";
  if (outcomes.includes("success")) return "success";
  return "skipped";
}
function prCoverage(snapshot) {
  const imageRuns = snapshot.runs.filter(run => run.jobs.some(imageJob)).sort(latestRunFirst);
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
function latestRunFirst(a, b) { return runTime(b) - runTime(a) || b.attempt - a.attempt || b.run_id - a.run_id; }
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
  const allowed = new Set(["success", "failure", "timed_out", "action_required", "startup_failure", "in_progress", "queued", "requested", "waiting", "pending"]);
  return el("span", `badge${allowed.has(value) ? ` ${value}` : ""}`, label || resultLabel(value));
}
function link(text, url, repository = state.data.repository) {
  const safe = safeGithubUrl(url, repository);
  if (!safe) return el("span", "", text);
  const anchor = el("a", "", text);
  anchor.href = safe;
  return anchor;
}
function prNumbers(run) { return (run?.pr_numbers || []).filter(Number.isSafeInteger); }
function buildSource(run) {
  if (run?.event === "pull_request" || run?.event === "pull_request_target") return "pr";
  if (run?.branch === "master") return "master";
  return prNumbers(run).length ? "pr" : "other";
}
function matchesRun(run) {
  const source = buildSource(run);
  const selectedSource = byId("source-filter").value;
  if (selectedSource === "combined" ? source === "other" : source !== selectedSource) return false;
  if (selectedSource === "master") {
    const commit = byId("commit-filter").value;
    return commit === "all" || run.source_sha === commit;
  }
  const selectedPr = byId("pr-filter").value;
  return selectedPr === "all" || (source === "pr" && prNumbers(run).includes(Number(selectedPr)));
}
function matchesJob(job) {
  const path = byId("path-filter").value;
  const arch = byId("arch-filter").value;
  return (path === "all" || job.kind === path) && (arch === "all" || (job.architecture || "unknown") === arch);
}
function setSelected(run, jobId) {
  state.selectedRun = runKey(run);
  state.selectedJob = jobId || null;
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
  const all = el("option", "", "All pull requests");
  all.value = "all";
  select.replaceChildren(all);
  for (const pr of [...state.data.pull_requests].sort((a, b) => b.number - a.number)) {
    const option = el("option", "", `#${pr.number} · ${pr.title}`);
    option.value = String(pr.number);
    select.append(option);
  }
  select.value = [...select.options].some(option => option.value === old) ? old : "all";

  const commits = byId("commit-filter"), previous = commits.value;
  const allCommits = el("option", "", "All master commits");
  allCommits.value = "all";
  commits.replaceChildren(allCommits);
  const masterRuns = state.data.runs.filter(run => buildSource(run) === "master" &&
    FULL_COMMIT_SHA.test(run.source_sha)).sort(latestRunFirst);
  const revisions = new Map(), prefixCounts = new Map();
  for (const run of masterRuns) {
    const sha = run.source_sha;
    if (!revisions.has(sha)) {
      revisions.set(sha, {latest: run, title: ""});
      const prefix = sha.slice(0, 12);
      prefixCounts.set(prefix, (prefixCounts.get(prefix) || 0) + 1);
    }
    if (typeof run.source_commit_title === "string") revisions.get(sha).title ||= run.source_commit_title.trim();
  }
  for (const [sha, {latest, title}] of revisions) {
    const prefix = sha.slice(0, 12);
    const label = prefixCounts.get(prefix) > 1 ? sha : prefix;
    const date = dateTime(latest.started_at || latest.completed_at);
    const option = el("option", "", [label, title, date].filter(Boolean).join(" · "));
    option.value = sha;
    option.title = title ? `${sha} · ${title}` : sha;
    commits.append(option);
  }
  commits.value = revisions.has(previous) ? previous : "all";
}
function svgEl(tag, attrs, text) {
  const node = document.createElementNS("http://www.w3.org/2000/svg", tag);
  for (const [key, value] of Object.entries(attrs || {})) node.setAttribute(key, String(value));
  if (text !== undefined) node.textContent = String(text);
  return node;
}
function pendingCommitImage() {
  if (byId("source-filter").value !== "master" || byId("commit-filter").value === "all") return null;
  const images = job => imageJob(job) && matchesJob(job);
  const run = state.data.runs.filter(item => matchesRun(item) && item.jobs.some(images)).sort(latestRunFirst)[0];
  const job = run?.jobs.find(item => images(item) && Object.hasOwn(PENDING_JOB_TEXT, result(item)));
  return job ? {run, job} : null;
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
  const visible = new Map();
  for (const {run, job} of points) {
    const source = BUILD_SOURCES[buildSource(run)], scope = imageSeries(job);
    visible.set(`${source.label} · ${scope.label}`, source);
  }
  for (const [label, source] of [...visible].sort(([a], [b]) => a.localeCompare(b))) {
    const item = el("span"), dot = el("i"); dot.style.backgroundColor = source.color;
    if (source.outline) dot.style.border = `1px solid ${source.outline}`;
    dot.setAttribute("aria-hidden", "true");
    item.append(dot, document.createTextNode(label)); legend.append(item);
  }
  legend.hidden = !points.length;
  const preview = byId("trend-preview");
  const note = byId("trend-note");
  preview.hidden = !points.length;
  preview.replaceChildren();
  if (!points.length) {
    const pending = !all.length && !unverified && byId("date-filter").value === "all" ? pendingCommitImage() : null;
    const message = el("p", "empty");
    if (pending) {
      const job = pending.job;
      message.append(link(job.name, job.url), document.createTextNode(` ${PENDING_JOB_TEXT[result(job)]}. No build duration yet.`));
    } else {
      message.textContent = "No successful image measurements with verified execution times match these filters. Failed, skipped, pending, and unverified records remain visible in run history.";
    }
    container.append(message);
    note.hidden = !unverified;
    note.textContent = unverified ? `${unverified} unverified records are omitted.` : "";
    return pending || all[0];
  }
  preview.append(el("p", "", "Hover or focus a point to inspect its measurement, source revision and breakdown availability. Select it to open the full run details."));
  note.hidden = false;
  note.textContent = `${points.length < matching.length ? `Showing the newest ${points.length} of ${matching.length}` : `Showing all ${points.length}`} matching verified image executions. Carried retries count once. ${unverified} unverified records are omitted. ${select.value === "all" ? "Select one measurement series to see its trend." : "Each line follows one build source, path, architecture and measured step."} Date ranges use execution start times and end at the published snapshot.`;
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
    const source = BUILD_SOURCES[buildSource(items[0].run)];
    const line = svgEl("polyline", {points: items.map(item => `${x(item)},${y(item)}`).join(" "), fill: "none", stroke: source.outline || source.color, "stroke-opacity": source.outline ? ".8" : ".45", "stroke-width": "2", "stroke-dasharray": items[0].job.architecture === "arm64" ? "5 4" : "0"});
    svg.append(line);
  }
  for (const item of points) {
    const build = BUILD_SOURCES[buildSource(item.run)];
    const prs = prNumbers(item.run).map(number => `PR #${number}`).join(", ");
    const context = [build.label, prs || (item.run.branch === "master" ? "" : item.run.branch)].filter(Boolean).join(" · ");
    const sha = item.run.source_sha;
    const commit = `Commit ${String(sha || "unknown").slice(0, 12)}`;
    const subject = item.run.source_commit_title || "Commit message unavailable";
    const breakdown = imageBreakdownLabel(item.job);
    const label = `${commit} · ${subject}\n${context} · ${imageSeries(item.job).label} · ${duration(item.job.image_build_seconds)} · ${imageMeasurement(item.job).label} · ${breakdown} · ${executionTimeLabel(item.execution)} · ${executionOriginLabel(item.execution)}`;
    const point = svgEl("g", {class: "chart-point", tabindex: "0", role: "button", "aria-label": label});
    point.append(svgEl("title", {}, label));
    point.append(svgEl("circle", {cx: x(item), cy: y(item), r: 17, fill: "transparent", "pointer-events": "all"}));
    point.append(svgEl("circle", {class: "chart-marker", cx: x(item), cy: y(item), r: 4.5, fill: build.color, stroke: build.outline || "white", "stroke-width": build.outline ? 1 : 2, "pointer-events": "none"}));
    const inspect = () => {
      const heading = el("div", "chart-preview-heading");
      heading.append(el("strong", "", duration(item.job.image_build_seconds)), el("span", "", imageSeries(item.job).label));
      const revision = el("p");
      const commitId = FULL_COMMIT_SHA.test(sha) ? link(commit, `https://github.com/${state.data.repository}/commit/${sha}`) : el("span", "", commit);
      revision.append(commitId, document.createTextNode(` · ${subject}`));
      preview.replaceChildren(heading, revision,
        el("p", "", `${item.run.workflow_name || "Workflow"} · ${context} · ${executionTimeLabel(item.execution)} · ${executionOriginLabel(item.execution)}`),
        el("p", "", `${imageMeasurement(item.job).label}. ${breakdown}.`));
    };
    point.addEventListener("mouseenter", inspect);
    point.addEventListener("focus", inspect);
    point.addEventListener("click", () => setSelected(item.run, item.job.job_id));
    point.addEventListener("keydown", event => { if (["Enter", " "].includes(event.key)) { event.preventDefault(); setSelected(item.run, item.job.job_id); } });
    svg.append(point);
  }
  container.append(svg);
  return all[0];
}

function renderCoverage() {
  const source = byId("source-filter").value;
  const latest = byId("latest-branch-status");
  latest.hidden = source === "pr";
  if (!latest.hidden) {
    const branchSource = source === "other" ? "other" : "master";
    const commit = source === "master" ? byId("commit-filter").value : "all";
    const run = state.data.runs.filter(item => buildSource(item) === branchSource &&
      (commit === "all" || item.source_sha === commit) && item.jobs.some(imageJob)).sort(latestRunFirst)[0];
    latest.replaceChildren(el("strong", "", branchSource === "master" ? "Latest master image build" : "Latest image build on other branches"));
    if (commit !== "all") latest.append(link(commit.slice(0, 12), `https://github.com/${state.data.repository}/commit/${commit}`));
    if (run) {
      latest.append(badge(imageResult(run.jobs)), link(run.workflow_name || "Workflow run", run.url));
      if (branchSource === "other") latest.append(el("span", "", run.branch || "Unknown branch"));
      latest.append(el("span", "", attemptTimeLabel(run)));
    } else {
      latest.append(el("span", "", "No image build recorded."));
    }
  }
  const summary = byId("pr-build-status");
  summary.hidden = source === "master" || source === "other";
  if (summary.hidden) return;
  const prRuns = state.data.runs.filter(run => buildSource(run) === "pr");
  const counts = summarizePrCoverage(prCoverage({pull_requests: state.data.pull_requests, runs: prRuns}));
  byId("coverage-count").textContent = `${counts.total} PR${counts.total === 1 ? "" : "s"}`;
  byId("coverage-summary").replaceChildren(...[["success", "Success"], ["failure", "Failed"], ["cancelled", "Cancelled"], ["skipped", "Skipped"]].map(([outcome, label]) => {
    const item = el("div", `coverage-stat ${outcome}`);
    item.append(el("dt", "", label), el("dd", "", counts[outcome]));
    return item;
  }));
  const note = byId("coverage-other");
  note.textContent = `${counts.pending} pending · ${counts.unmeasured} unmeasured (no image jobs or result unavailable).`;
  note.hidden = counts.pending + counts.unmeasured === 0;
}
function visibleRuns() {
  const allJobs = byId("path-filter").value === "all" && byId("arch-filter").value === "all";
  return state.data.runs.filter(run => matchesRun(run) && (allJobs || run.jobs.some(matchesJob))).sort((a, b) => runTime(b) - runTime(a));
}
function renderRuns(all = visibleRuns()) {
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
const P4RT_SUBTASKS = new Set(["p4rt-audit", "p4rt-sdk", "p4rt-owner", "p4rt-handoff"]);
const GROUPED_PREPARATION_TASKS = new Set(["prep-deb", "prep-deb-install", "prep-wheel", "prep-wheel-install", "container-manifest"]);

// The caller clips spans to the stage and orders them by start time.
function coveredSeconds(spans) {
  let covered = 0, previousEnd = 0;
  for (const {from, to} of spans) {
    covered += Math.max(0, to - Math.max(from, previousEnd));
    previousEnd = Math.max(previousEnd, to);
  }
  return covered;
}

function intervalTaskRow({task, from, to}) {
  const row = el("div", "interval-task");
  const clipped = from > task.offset_seconds || to < task.offset_seconds + task.duration_seconds;
  row.append(el("span", "", task.label), el("span", "timing-duration", duration(to - from)));
  if (clipped || task.outcome === "cached") {
    row.append(el("small", "timing-hint", [clipped && `Portion of a ${duration(task.duration_seconds)} task`,
      task.outcome === "cached" && "Cached"].filter(Boolean).join(" · ")));
  }
  return row;
}

function intervalTaskList(spans, start, end, reportUncovered) {
  const body = el("div", "interval-tasks");
  const list = el("ul", "interval-measurements");
  const parents = spans.filter(({task}) => task.kind === "p4rt-prep" || task.kind === "rootfs");
  const children = new Map();
  const groups = new Map();
  for (const span of spans) {
    const kind = span.task.kind;
    if (GROUPED_PREPARATION_TASKS.has(kind)) {
      if (!groups.has(kind)) groups.set(kind, []);
      groups.get(kind).push(span);
      continue;
    }
    const parentKind = P4RT_SUBTASKS.has(kind) ? "p4rt-prep" : kind.startsWith("rootfs-") ? "rootfs" : null;
    const parent = parents.find(p => p.task.kind === parentKind &&
      p.task.offset_seconds <= span.task.offset_seconds &&
      p.task.offset_seconds + p.task.duration_seconds >= span.task.offset_seconds + span.task.duration_seconds);
    if (parent) children.set(span, parent);
  }
  for (const span of spans) {
    if (children.has(span)) continue;
    const item = el("li");
    const group = groups.get(span.task.kind);
    if (group) {
      if (span !== group[0]) continue;
      const row = el("div", "interval-task");
      row.append(el("span", "", TASK_LABELS[span.task.kind]),
        el("span", "timing-duration", readable(coveredSeconds(group))),
        el("small", "timing-hint", `${group.length} measured ${group.length === 1 ? "invocation" : "invocations"}`));
      item.append(row);
      list.append(item);
      continue;
    }
    item.append(intervalTaskRow(span));
    const nested = [...children].filter(([, parent]) => parent === span);
    if (nested.length) {
      const sublist = el("ul", "interval-measurements");
      for (const [child] of nested) {
        const entry = el("li");
        entry.append(intervalTaskRow(child)); sublist.append(entry);
      }
      item.append(el("p", "timing-hint", "Included in the parent task duration"), sublist);
    }
    list.append(item);
  }
  if (spans.length) body.append(list);
  else body.append(el("p", "unmeasured-note", "No task timings recorded for this interval."));

  if (!reportUncovered) return body;
  const uncovered = Math.max(0, end - start - coveredSeconds(spans));
  body.append(el("p", "breakdown-note", `Unattributed wall time: ${duration(uncovered)}. Overlapping task spans count once.`));
  return body;
}

function onlyHostBuild(job, step) {
  return job?.kind === "make-vs" && step?.name === HOST_VS_BUILD_STEP && step.conclusion === "success" &&
    Array.isArray(job.steps) && job.steps.includes(step) && job.steps.filter(item => item.name === HOST_VS_BUILD_STEP).length === 1;
}

function renderInstallerIntervals(breakdown, job, step) {
  const [section] = breakdown.sections;
  if (breakdown.sections.length !== 1 || section.kind !== "vs-image" || section.offset_seconds !== 0 ||
      section.duration_seconds !== breakdown.duration_seconds) return null;
  const batches = section.tasks.filter(task => task.kind === "vs-container-images");
  const installers = section.tasks.filter(task => task.kind === "final-image");
  if (batches.length !== 1 || installers.length !== 1) return null;
  const [batch] = batches, [installer] = installers;
  if (batch.outcome !== "finished" || installer.outcome !== "finished") return null;
  const batchEnd = batch.offset_seconds + batch.duration_seconds;
  const installerEnd = installer.offset_seconds + installer.duration_seconds;
  if (batchEnd > installer.offset_seconds || installerEnd > section.duration_seconds) return null;

  const overview = el("section", "installer-intervals");
  overview.append(el("h3", "", "Image build steps"), el("p", "breakdown-note",
    `${duration(section.duration_seconds, true)} from the start of the host Make build through its return. Select a step for its measured work.`));
  const intervals = [
    ["Prepare kernel, packages and P4RT", 0, batch.offset_seconds],
    ["Build and hand off Bazel container images", batch.offset_seconds, batchEnd, batch],
    ["Finish Make containers and stage the root filesystem", batchEnd, installer.offset_seconds],
    ["Assemble the final ONIE installer", installer.offset_seconds, installerEnd, installer],
    ["Finish the Make invocation", installerEnd, section.duration_seconds],
  ];
  const kernelTasks = section.tasks.filter(task => task.kind === "kernel-prep");
  let kernelStage, containerStage;
  for (const [index, [label, start, end, marker]] of intervals.entries()) {
    const disclosure = el("details", "installer-interval"), summary = el("summary", "timing-row");
    const name = el("span", "timing-name", label);
    name.append(el("span", "timing-hint", `+${duration(start, true)} to +${duration(end, true)} from build start`));
    summary.append(name, el("span", "timing-duration", duration(end - start, true))); disclosure.append(summary);
    if (index === 0 && kernelTasks.length === 1 && onlyHostBuild(job, step)) {
      const [kernel] = kernelTasks;
      if (kernel.outcome === "finished" && kernel.offset_seconds + kernel.duration_seconds <= end) {
        kernelStage = {section: disclosure, elapsed: kernel.duration_seconds};
      }
    }
    if (marker === installer) {
      disclosure.append(renderPhaseBreakdown(installer.phase_breakdown));
    } else if (marker === batch) {
      const batch = containerBatchMeasurements(job, step);
      if (batch) {
        containerStage = disclosure;
        const list = el("dl", "interval-receipts");
        const add = (target, label, elapsed) => {
          const pair = el("div", "interval-task");
          pair.append(el("dt", "", label), el("dd", "timing-duration", duration(elapsed, true))); target.append(pair);
        };
        if (batch.bazelSeconds !== null) add(list, "Shared Bazel command", batch.bazelSeconds);
        if (batch.exports.length) {
          add(list, "Recorded output handoff time", batch.exports.reduce((total, entry) => total + entry[1], 0));
          const detail = el("details", "batch-exports"), rows = el("dl", "interval-receipts");
          for (const [label, elapsed] of batch.exports) add(rows, label, elapsed);
          detail.append(el("summary", "", `Output handoffs by container (${batch.exports.length})`), rows);
          disclosure.append(list, detail);
        } else disclosure.append(list);
        disclosure.append(el("p", "breakdown-note", "Output handoff time adds the commands that publish OCI layouts or export archives. The section heading shows elapsed time for the whole Make task."));
      } else disclosure.append(el("p", "unmeasured-note", "Separate Bazel command and output handoff timings were not recorded for this batch."));
    }
    const spans = section.tasks.filter(task => task !== batch && task !== installer).map(task => ({
      task, from: Math.max(start, task.offset_seconds), to: Math.min(end, task.offset_seconds + task.duration_seconds),
    })).filter(({task, from, to}) => from < to || task.duration_seconds === 0 && from === to && (from < end || end === section.duration_seconds));
    if (!marker || spans.length) disclosure.append(intervalTaskList(spans, start, end, !marker));
    overview.append(disclosure);
  }
  return {overview, containerStage, kernelStage};
}

function hostInstallerStep(step) {
  return step.name === HOST_VS_BUILD_STEP && step.status === "completed" && step.conclusion === "success";
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
        if (task.kind === "p4rt-prep") {
          taskName.append(el("span", "timing-hint", "The P4RT preparation total includes the measured P4RT substeps."));
        }
        if (hasPhases) taskName.append(el("span", "timing-hint", task.phase_breakdown.status === "available" ? "Measured installer phases" : task.phase_breakdown.reason === "not-recorded" ? "Inner phases not measured in this run" : "Inner phase breakdown unavailable"));
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
function renderStep(step, timelineShown = false) {
  const item = el("li", "step-item"), breakdown = timelineShown ? null : normalizeBreakdown(step.breakdown);
  const row = el(breakdown ? "summary" : "div", "timing-row");
  const name = el("span", "timing-name", step.name || "Unnamed step"), timing = el("span", "step-duration");
  timing.append(badge(result(step)), document.createTextNode(duration(step.duration_seconds)));
  if (timelineShown) name.append(el("span", "timing-hint", "See Image build steps above."));
  if (breakdown) name.append(el("span", "timing-hint", breakdown.status === "available" ? "Measured breakdown" : "Breakdown unavailable"));
  row.append(name, timing);
  if (breakdown) {
    const timeline = hostInstallerStep(step) && breakdown.status === "available" && renderInstallerIntervals(breakdown);
    const details = el("details", "step-breakdown"); details.append(row, timeline?.overview || renderBreakdown(breakdown)); item.append(details);
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
      ["Other-command lock", otherLock], ["All command elapsed", row.bazel_seconds], ["Output handoff", row.export_seconds],
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
    if (Object.hasOwn(member, "compile_seconds") &&
        (!validTime(member.compile_seconds) || member.compile_seconds > member.action_seconds + 1e-6)) return fallback();
    if (Object.hasOwn(member, "total_seconds") &&
        (!validTime(member.total_seconds) || member.total_seconds + 1e-6 < member.action_seconds)) return fallback();
    seen.add(member.owner);
    members.push({...member, recipe});
  }
  members.sort((a, b) => b.action_seconds - a.action_seconds || a.recipe.owner.localeCompare(b.recipe.owner));
  return {profile: detail, members};
}

function containerBatchMeasurements(job, step) {
  if (!onlyHostBuild(job, step)) return null;
  const receipts = job.image_recipe_timings;
  if (receipts?.version !== 2) return null;
  const rows = recipeRows(receipts);
  const batch = rows?.find(row => row.owner === "vs-container-images");
  // A recipe row combines every invocation of its owner in the job.
  if (!batch || batch.failed || batch.bazel_expected !== 1) return null;
  const bazelSeconds = batch.bazel_phases === 1 ? batch.bazel_seconds : null;
  const exports = [];
  const {members} = containerMembers(rows, receipts.container_breakdown);
  for (const {recipe} of members) {
    // Owners that also built independently can include exports from outside the batch.
    if (recipe.failed || recipe.bazel_expected || recipe.profiles_expected || !recipe.exports) continue;
    const count = recipe.exports < recipe.exports_expected ? `${recipe.exports} of ${recipe.exports_expected} recorded` : recipe.exports;
    const label = recipe.exports_expected === 1 ? "" : ` (${count} commands)`;
    exports.push([`${recipe.owner}${label}`, recipe.export_seconds]);
  }
  return bazelSeconds !== null || exports.length ? {bazelSeconds, exports} : null;
}

function renderContainerMembers({profile, members}, wallSeconds) {
  const box = el("section", "container-members");
  box.append(el("h4", "", `Containers (${members.length})`));
  if (!members.length) {
    box.append(el("p", "stage-note", "No per-container measurements were recorded for this batch."));
    return box;
  }
  if (profile) {
    box.append(el("p", "stage-note", "Total elapsed runs from the first observed action owned by an image through its last output handoff, including time spent waiting. Images build in parallel, so their totals overlap."));
    box.append(el("p", "stage-note", "Active time counts intervals with at least one observed action owned by the image. Compile time counts elapsed time with a C, C++, Rust or Go compiler running for that image. Simultaneous compiler processes count once per image. Compilation, cache fetch, OCI archive and gzip can overlap. After Bazel returns, Make publishes the OCI layout or exports the requested archive."));
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
    box.append(el("p", "stage-note", "This run retained per-container output handoff times. Per-container Bazel action times are unavailable."));
  }
  const scroll = el("div", "table-scroll"), table = el("table"), head = el("thead"), header = el("tr"), body = el("tbody");
  scroll.setAttribute("tabindex", "0");
  scroll.setAttribute("role", "region");
  scroll.setAttribute("aria-label", "Container timings");
  const metrics = [["actions", "Actions"], ["total_seconds", "Total elapsed"], ["action_seconds", "Active"],
    ["compile_seconds", "Compile time"], ["fetch_seconds", "Cache fetch"], ["archive_seconds", "OCI archive"],
    ["compress_seconds", "Gzip"]].filter(([key]) => profile || key === "total_seconds" || key === "compile_seconds");
  for (const label of ["Image", ...metrics.map(([, label]) => label), "Output handoff"]) {
    const th = el("th", "", label); th.setAttribute("scope", "col"); header.append(th);
  }
  head.append(header);
  for (const member of members) {
    const recipe = member.recipe, tr = el("tr"), name = el("th", "", recipe.owner);
    name.setAttribute("scope", "row");
    if (recipe.failed) name.append(el("small", "recipe-failed", "Command failed"));
    tr.append(name);
    for (const [key, label] of metrics) {
      const value = member[key];
      const cell = el("td", "", key === "actions" ? recipeNumber.format(value) : recipeSeconds(value ?? null));
      if (value === undefined) {
        cell.title = `${label} unavailable`;
        cell.setAttribute("aria-label", cell.title);
      }
      tr.append(cell);
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

function recipeMetricText(row, metric, count, expected) {
  if (!row[expected]) return "N/A";
  const samples = row[count] < row[expected] ? ` (${row[count]}/${row[expected]})` : "";
  return recipeSeconds(row[metric]) + samples;
}

function renderKernelTimings(row, elapsed) {
  const columns = ["Target", "Total elapsed", "Recorded command time", "Bazel launcher", "Lock wait",
    "Compile time", "Nested build", "Cache fetch", "Stage DEBs"];
  const partial = row.bazel_phases < row.bazel_expected || row.exports < row.exports_expected;
  const recorded = recipeSeconds(row.recorded) + (partial && row.recorded !== null ? " (partial)" : "");
  const values = [row.owner, recipeSeconds(elapsed), recorded,
    ...RECIPE_METRICS.map(([metric, , count, expected]) => recipeMetricText(row, metric, count, expected))];
  const section = evidenceTable("Kernel Bazel breakdown", columns, [values]);
  section.className += " kernel-timings";
  section.append(el("p", "stage-note", "Total elapsed covers the kernel preparation task. Recorded command time is the sum of the launcher and Debian package staging times. Lock wait, compilation, nested Make and cache fetch can overlap within the launcher."));
  return section;
}

function renderRecipeRows(rows, title, group, batch) {
  const box = el("section", "recipe-list");
  box.append(el("h3", "", title));
  if (!rows.length) {
    box.append(el("p", "stage-note", "No Bazel activity was recorded during this build."));
    return box;
  }
  if (rows.length > 1) box.append(el("p", "stage-note", "Recipes are ordered by recorded Bazel and output handoff time. Expand a recipe for its breakdown."));
  const list = el("ol");
  for (const row of rows) {
    const item = el("li"), details = el("details");
    details.open = row === rows[0];
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

function renderRecipeTimings(job, repository, timeline) {
  if (job.kind !== "make-vs" || !job.steps.some(step => step.name === HOST_VS_BUILD_STEP)) return null;
  const value = job.image_recipe_timings, rows = recipeRows(value);
  const box = el("div", "recipe-timings");
  if (rows) {
    const group = rows.find(row => row.owner === "vs-container-images");
    const batch = group && containerMembers(rows, value.container_breakdown);
    const exportsInBatch = new Set(batch?.members.filter(({recipe}) =>
      recipe.bazel_expected === 0 && recipe.profiles_expected === 0).map(({recipe}) => recipe.owner));
    const nestedGroup = timeline?.containerStage && group;
    if (nestedGroup) timeline.containerStage.append(renderRecipeRows([group], "Bazel container breakdown", group, batch));
    const kernel = timeline?.kernelStage && rows.find(row => row.owner === "linux-kernel" && !row.failed &&
      row.bazel_expected === 1 && row.exports_expected <= 1);
    if (kernel) timeline.kernelStage.section.append(renderKernelTimings(kernel, timeline.kernelStage.elapsed));
    const standalone = rows.filter(row => !exportsInBatch.has(row.owner) && row !== nestedGroup && row !== kernel);
    if (standalone.length || !rows.length) {
      const title = value.version === 1 ? "Bazel timings for four image recipes" : "Bazel build breakdown";
      box.append(renderRecipeRows(standalone, title, group, batch));
    }
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
  if (!run) { box.append(el("p", "empty", "Select a chart point or workflow run to see its timing breakdown.")); return; }
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
  const hostStep = job.steps.find(hostInstallerStep);
  const hostBreakdown = hostStep && normalizeBreakdown(hostStep.breakdown);
  const timeline = hostBreakdown?.status === "available" && renderInstallerIntervals(hostBreakdown, job, hostStep);
  if (timeline) box.append(timeline.overview);
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
  const recipes = renderRecipeTimings(job, state.data.repository, timeline);
  if (recipes?.children.length) box.append(recipes);
  const targetRows = recipes && recipeRows(job.image_recipe_timings);
  if (analysis && targetRows?.length) {
    analysis.append(renderBuildExplanations(targetRows, `Run ${run.run_id} · ${job.name}`));
    analysis.hidden = false;
  }
  const steps = el("details", "steps"); steps.append(el("summary", "", `Step timings · ${job.steps.length} steps`));
  const list = el("ol");
  for (const step of job.steps) list.append(renderStep(step, !!timeline && step === hostStep));
  steps.append(list); box.append(steps);
}
function renderFiltered() {
  const latest = renderTrend(), runs = visibleRuns();
  if (!runs.some(run => runKey(run) === state.selectedRun)) {
    let run = latest?.run;
    if (!run && byId("commit-filter").value !== "all") {
      run = runs.find(item => item.jobs.some(job => imageJob(job) && matchesJob(job)));
    }
    run ??= runs[0];
    state.selectedRun = run ? runKey(run) : null;
    state.selectedJob = latest?.job.job_id || null;
  }
  renderRuns(runs);
  renderDetails();
}
function applyBuildSource() {
  const pr = byId("pr-filter"), source = byId("source-filter").value;
  pr.disabled = source === "master" || source === "other";
  if (pr.disabled) pr.value = "all";
  byId("pr-filter-label").hidden = pr.disabled;
  byId("commit-filter-label").hidden = source !== "master";
  if (source !== "master") byId("commit-filter").value = "all";
  renderCoverage();
  renderFiltered();
}
function render() {
  byId("dashboard").hidden = false;
  byId("snapshot-time").textContent = `Snapshot: ${dateTime(state.data.generated_at)}`;
  byId("repository-link").href = `https://github.com/${state.data.repository}`;
  renderFilters(); renderMetrics();
  applyBuildSource();
}
async function fetchSnapshotJson(url, options) {
  const response = await fetch(url, {credentials: "omit", signal: AbortSignal.timeout(30000), ...options});
  if (!response.ok) throw new Error(`Snapshot request failed (HTTP ${response.status}).`);
  return response.json();
}
async function loadSnapshot(url) {
  const index = await fetchSnapshotJson(url, {cache: "no-store"});
  if (index?.schema_version !== 2) return validateSnapshot(index);
  const {run_shards: shards, ...metadata} = index;
  const snapshot = validateSnapshot({...metadata, schema_version: 1, runs: []});
  if (!Array.isArray(shards)) throw new Error("The published run history index is incomplete.");
  const paths = new Set();
  const files = shards.map(shard => {
    const match = typeof shard?.path === "string" && /^history\/([0-9a-f]{64})\.json$/.exec(shard.path);
    if (!match || paths.has(shard.path) || !Number.isSafeInteger(shard.count) || shard.count < 1) {
      throw new Error("The published run history index contains an invalid file.");
    }
    paths.add(shard.path);
    // Fetch verifies the response bytes against the SHA-256 in the immutable filename.
    const digest = btoa(String.fromCharCode(...match[1].match(/../g).map(byte => parseInt(byte, 16))));
    return {url: new URL(shard.path, url), integrity: `sha256-${digest}`, count: shard.count};
  });
  for (let start = 0; start < files.length; start += 4) {
    const parts = await Promise.all(files.slice(start, start + 4).map(async file => {
      const runs = await fetchSnapshotJson(file.url, {cache: "force-cache", redirect: "error", integrity: file.integrity});
      if (!Array.isArray(runs) || runs.length !== file.count) throw new Error("A published run history file is incomplete.");
      return runs;
    }));
    for (const part of parts) for (const run of part) snapshot.runs.push(run);
  }
  return validateSnapshot(snapshot);
}
async function refresh() {
  if (state.loading) return;
  state.loading = true; byId("refresh").disabled = true;
  const status = byId("load-status"); status.className = ""; status.textContent = state.data ? "Checking for a newer published snapshot…" : "Loading CI history…";
  try {
    const url = new URL("data.json", window.location.href); url.searchParams.set("t", String(Date.now()));
    state.data = await loadSnapshot(url);
    render(); status.textContent = "";
  } catch (error) {
    status.className = "error";
    status.textContent = `${state.data ? "Showing the last loaded snapshot. " : "The dashboard could not load its data. "}${error.message} Try Refresh data again.`;
  } finally { state.loading = false; byId("refresh").disabled = false; }
}
if (typeof document !== "undefined") {
  byId("refresh").addEventListener("click", refresh);
  byId("source-filter").addEventListener("change", () => { if (state.data) applyBuildSource(); });
  byId("commit-filter").addEventListener("change", () => { if (state.data) applyBuildSource(); });
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
if (typeof module !== "undefined") module.exports = {seconds, duration, timestamp, safeGithubUrl, validateSnapshot, measured, completedImageSeconds, imageMeasurement, imageTrendGroups, imageSeries, selectTrendPoints, chartScale, imageBreakdownLabel, attemptTimeLabel, imageExecution, executionTimeLabel, executionOriginLabel, jobExecutionLabel, imageMeasurements, imageResult, prCoverage, summarizePrCoverage, normalizeBreakdown, normalizePhaseBreakdown, renderStep, renderRecipeTimings, recipeRows, containerBatchMeasurements, renderBuildExplanations, baseSummary};
