"""Behavioral tests for public, bounded Make build timing summaries."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from test_collect import collector, FakeAPI, REPO, START, END, empty, job, run

breakdown = collector.breakdown


def synthetic_log():
    """Overlapping child targets and sequential outer work with setup/cleanup."""
    return b"""2026-10-10T00:00:00.2000000Z ##[group]Run make target/sonic-vs.img.gz
2026-10-10T00:00:00.3000000Z private-host secret environment /private/runner/path
2026-10-10T00:00:01Z +++ --- Making target/sonic-vs.img.gz --- +++
2026-10-10T00:00:10Z [ building ] [ target/docker-base-trixie.gz ]
2026-10-10T00:00:15Z [ building ] [ target/docker-config-engine-trixie.gz ]
2026-10-10T00:01:05Z [ finished ] [ target/docker-config-engine-trixie.gz ]
2026-10-10T00:01:10Z [ finished ] [ target/docker-base-trixie.gz ]
2026-10-10T00:01:11Z [ building ] [ target/private-host-secret.gz ]
2026-10-10T00:01:12Z [ finished ] [ target/private-host-secret.gz ]
2026-10-10T00:02:00Z [ building ] [ target/sonic-vs.img.gz ]
2026-10-10T00:08:00Z [ finished ] [ target/sonic-vs.img.gz ]
2026-10-10T00:08:10Z +++ --- Making target/docker-orchagent.gz --- +++
2026-10-10T00:08:20Z [ building ] [ target/docker-orchagent.gz ]
2026-10-10T00:08:30Z [ cached ] [ target/docker-orchagent.gz ]
2026-10-10T00:09:59Z final cleanup private-host
2026-10-10T00:10:00.4000000Z ##[group]Run validation after build
2026-10-10T00:10:01Z [ building ] [ target/sonic-vs.img.gz ]
2026-10-10T00:10:20Z [ finished ] [ target/sonic-vs.img.gz ]
"""


def parsed():
    return breakdown.parse_log(synthetic_log(), job()["steps"][0])


def phase_payload():
    segments, offset = [], 0
    for phase, context, duration in [
        ("assembly", "recipe", 30), ("assembly", "filesystem", 30), ("assembly", "installer", 30),
        ("boot-checks", "bios", 90), ("assembly", "recipe", 10), ("boot-checks", "uefi", 90),
        ("compression", "recipe", 80),
    ]:
        segments.append({"phase": phase, "context": context, "offset_seconds": offset, "duration_seconds": duration})
        offset += duration
    return {"version": 1, "duration_seconds": offset, "segments": segments}


def phase_marker(payload=None, timestamp="00:07:59.9000000", target="target/sonic-vs.img.gz"):
    value = phase_payload() if payload is None else payload
    text = value if isinstance(value, str) else json.dumps(value)
    return f"2026-10-10T{timestamp}Z [ image phases ] [ {target} ] {text}\n".encode()


FINAL_START = b"2026-10-10T00:02:00Z [ building ] [ target/sonic-vs.img.gz ]\n"
FINAL_FINISH = b"2026-10-10T00:08:00Z [ finished ] [ target/sonic-vs.img.gz ]\n"


def log_with_phases(marker=None):
    return synthetic_log().replace(FINAL_FINISH, (phase_marker() if marker is None else marker) + FINAL_FINISH)


def final_tasks(value):
    return [task for section in value["sections"] for task in section["tasks"] if task["id"].startswith("final-image-")]


class ImagePhaseTests(unittest.TestCase):
    def parse(self, log):
        value = breakdown.parse_log(log, job()["steps"][0])
        # Inner failures must preserve the reliable outer measurement.
        self.assertEqual(value["status"], "available")
        self.assertEqual(value["duration_seconds"], 600.2)
        self.assertTrue(breakdown.valid(value))
        return final_tasks(value)[0]["phase_breakdown"]

    def test_complete_bios_uefi_partition_retains_exact_measured_segments(self):
        value = self.parse(log_with_phases())
        self.assertEqual(value, dict(phase_payload(), status="available", reason=None, source="build-phase-markers"))
        self.assertEqual([segment["context"] for segment in value["segments"]],
                         ["recipe", "filesystem", "installer", "bios", "recipe", "uefi", "recipe"])

    def test_actual_producer_receipt_survives_parser_collection_and_history_validation(self):
        helper = Path(__file__).resolve().parents[3] / "scripts/image_timing.py"
        spec = importlib.util.spec_from_file_location("integration_image_timing", helper)
        timing = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(timing)
        durations_us = {"assembly": 5_000_000, "boot-checks": 100_000_000, "compression": 26_250_000}
        endpoints = [1_000_000_000]
        for phase, _ in timing.SINGLE:
            endpoints.append(endpoints[-1] + durations_us[phase])
        with mock.patch.object(timing, "now_us", side_effect=endpoints):
            receipt = timing.begin()
            self.addCleanup(Path(receipt).unlink, missing_ok=True)
            for phase, context in timing.SINGLE[1:]:
                timing.switch(receipt, phase, context)
            marker = timing.end(receipt)
        self.assertIsInstance(marker, str)
        self.assertFalse(Path(receipt).exists())
        # This is the producer's emitted line, not reconstructed JSON.
        log = log_with_phases(b"2026-10-10T00:07:59.9000000Z " + marker.encode() + b"\n")
        phases = self.parse(log)
        self.assertEqual(phases["status"], "available")
        self.assertEqual(phases["duration_seconds"], 360)
        self.assertEqual([(s["phase"], s["context"]) for s in phases["segments"]], timing.SINGLE)
        totals = {phase: sum(s["duration_seconds"] for s in phases["segments"] if s["phase"] == phase)
                  for phase in durations_us}
        self.assertEqual(totals, {"assembly": 55, "boot-checks": 200, "compression": 105})
        cursor = 0
        for segment in phases["segments"]:
            self.assertEqual(segment["offset_seconds"], cursor)
            cursor += segment["duration_seconds"]
        self.assertEqual(cursor, phases["duration_seconds"])
        api = FakeAPI()
        api.logs[99] = log
        history = collector.collect(api, REPO, empty())
        summary = history["runs"][0]["jobs"][0]["steps"][0]["breakdown"]
        self.assertTrue(breakdown.valid(summary))
        self.assertEqual(final_tasks(summary)[0]["phase_breakdown"], phases)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "history.json"
            collector.write_atomic(path, history)
            self.assertEqual(collector.load_history(path, REPO), history)
        self.assertNotIn(receipt, json.dumps(history))

    def test_missing_legacy_measurement_is_explicit_without_reconstructing_timings(self):
        self.assertEqual(self.parse(synthetic_log()), breakdown.phase_unavailable("not-recorded"))
        legacy = parsed()
        del final_tasks(legacy)[0]["phase_breakdown"]
        self.assertTrue(breakdown.valid(legacy))

    def test_malformed_private_extra_keys_duplicate_json_and_oversize_are_isolated(self):
        values = [
            "private-host not json",
            json.dumps(phase_payload()).replace('"version": 1', '"version": 1, "version": 1', 1),
            json.dumps(phase_payload()).replace('"context": "recipe"', '"context": "recipe", "context": "recipe"', 1),
            '{"version":1,"duration_seconds":NaN,"segments":[]}', "[" * 2000 + "]" * 2000,
            json.dumps(phase_payload()) + " " * breakdown.MAX_PHASE_JSON_BYTES,
            dict(phase_payload(), label="private-host /private/path"),
            dict(phase_payload(), version=True), dict(phase_payload(), version=2),
            dict(phase_payload(), duration_seconds=float("inf")),
        ]
        for mutate in (
            lambda value: value["segments"][0].update(label="private-host"),
            lambda value: value["segments"][0].update(phase="private-host"),
            lambda value: value["segments"][0].update(context="/private/path"),
            lambda value: value["segments"][0].update(phase=[]),
            lambda value: value["segments"][0].update(offset_seconds=True),
            lambda value: value["segments"][0].update(duration_seconds=-1),
        ):
            payload = phase_payload()
            mutate(payload)
            values.append(payload)
        for payload in values:
            with self.subTest(payload_type=type(payload).__name__):
                self.assertEqual(self.parse(log_with_phases(phase_marker(payload))), breakdown.phase_unavailable("invalid-timing"))

    def test_empty_partial_out_of_order_gapped_and_overlapping_partitions_are_rejected(self):
        values = [dict(phase_payload(), segments=[])]
        for mutation in (
            lambda value: value["segments"].pop(),
            lambda value: value["segments"].reverse(),
            lambda value: value["segments"][1].update(offset_seconds=31),
            lambda value: value["segments"][1].update(offset_seconds=29),
            lambda value: value.update(duration_seconds=361),
        ):
            payload = phase_payload()
            mutation(payload)
            values.append(payload)
        for payload in values:
            self.assertEqual(self.parse(log_with_phases(phase_marker(payload))), breakdown.phase_unavailable("incomplete-timing"))

    def test_segment_count_duplicate_segments_and_duplicate_markers_are_rejected(self):
        payload = {"version": 1, "duration_seconds": 360,
                   "segments": [{"phase": "assembly", "context": "recipe", "offset_seconds": i * 360 / 65,
                                 "duration_seconds": 360 / 65} for i in range(65)]}
        self.assertEqual(self.parse(log_with_phases(phase_marker(payload)))["reason"], "invalid-timing")
        zero = {"phase": "assembly", "context": "recipe", "offset_seconds": 0, "duration_seconds": 0}
        payload = dict(phase_payload(), segments=[zero, zero] + phase_payload()["segments"])
        self.assertEqual(self.parse(log_with_phases(phase_marker(payload)))["reason"], "invalid-timing")
        self.assertEqual(self.parse(log_with_phases(phase_marker() * 2))["reason"], "invalid-timing")

    def test_small_boundary_gaps_cannot_accumulate_into_an_incomplete_partition(self):
        segments = [{"phase": "assembly", "context": "recipe", "offset_seconds": i * 5.625009,
                     "duration_seconds": 5.625} for i in range(64)]
        payload = {"version": 1, "duration_seconds": segments[-1]["offset_seconds"] + 5.625,
                   "segments": segments}
        # Each nine-microsecond gap passes the individual boundary tolerance,
        # but their sum is larger than the whole-partition tolerance.
        self.assertEqual(self.parse(log_with_phases(phase_marker(payload))),
                         breakdown.phase_unavailable("incomplete-timing"))

    def test_clock_discrepancy_bound_is_not_rescaled(self):
        for total, expected in [(358, "available"), (362, "available"), (357.999, "unavailable"), (362.001, "unavailable")]:
            payload = {"version": 1, "duration_seconds": total,
                       "segments": [{"phase": "assembly", "context": "recipe", "offset_seconds": 0, "duration_seconds": total}]}
            value = self.parse(log_with_phases(phase_marker(payload, timestamp="00:08:00")))
            self.assertEqual(value["status"], expected)
            if expected == "available":
                self.assertEqual(value["duration_seconds"], total)
                self.assertEqual(value["segments"][0]["duration_seconds"], total)

    def test_early_future_and_out_of_order_phase_timestamps_only_invalidate_inner_measurement(self):
        for timestamp in ("00:01:59", "00:02:00", "00:08:00.1000000", "00:11:00"):
            self.assertEqual(self.parse(log_with_phases(phase_marker(timestamp=timestamp)))["reason"], "invalid-timing")

    def test_wrong_target_and_stale_markers_never_bleed_into_final_image_task(self):
        self.assertEqual(self.parse(log_with_phases(phase_marker(target="target/docker-base-trixie.gz")))["reason"], "not-recorded")
        before = synthetic_log().replace(FINAL_START, phase_marker(timestamp="00:01:59") + FINAL_START)
        after = synthetic_log().replace(FINAL_FINISH, FINAL_FINISH + phase_marker(timestamp="00:08:00.1000000"))
        different_section = synthetic_log().replace(b"2026-10-10T00:08:20Z", phase_marker(timestamp="00:08:19") + b"2026-10-10T00:08:20Z")
        for log in (before, after, different_section):
            self.assertEqual(self.parse(log)["reason"], "not-recorded")

    def test_cached_final_image_never_uses_saved_phase_output(self):
        log = log_with_phases().replace(FINAL_FINISH, FINAL_FINISH.replace(b"finished", b"cached"))
        self.assertEqual(self.parse(log), breakdown.phase_unavailable("not-recorded"))

    def test_repeated_task_has_independent_phase_state(self):
        second = (b"2026-10-10T00:08:01Z [ building ] [ target/sonic-vs.img.gz ]\n"
                  b"2026-10-10T00:08:02Z [ finished ] [ target/sonic-vs.img.gz ]\n")
        value = breakdown.parse_log(log_with_phases().replace(FINAL_FINISH, FINAL_FINISH + second), job()["steps"][0])
        tasks = final_tasks(value)
        self.assertEqual([task["id"] for task in tasks], ["final-image-1", "final-image-2"])
        self.assertEqual(tasks[0]["phase_breakdown"]["status"], "available")
        self.assertEqual(tasks[1]["phase_breakdown"]["reason"], "not-recorded")

    def test_retained_phase_data_validates_strictly_and_only_on_final_image(self):
        value = breakdown.parse_log(log_with_phases(), job()["steps"][0])
        self.assertTrue(breakdown.valid(value))
        for mutation in (
            lambda v: final_tasks(v)[0]["phase_breakdown"].update(label="private-host"),
            lambda v: final_tasks(v)[0]["phase_breakdown"].update(source="github-job-log"),
            lambda v: final_tasks(v)[0].update(outcome="cached"),
            lambda v: v["sections"][0]["tasks"][0].update(phase_breakdown=final_tasks(v)[0]["phase_breakdown"]),
            lambda v: final_tasks(v)[0].update(duration_seconds=363),
        ):
            bad = copy.deepcopy(value)
            mutation(bad)
            self.assertFalse(breakdown.valid(bad))

    def test_maximum_segments_and_zero_measurements_are_valid(self):
        payload = {"version": 1, "duration_seconds": 360,
                   "segments": [{"phase": "boot-checks", "context": "4-asic" if i % 2 else "6-asic",
                                 "offset_seconds": i * 5.625, "duration_seconds": 5.625} for i in range(64)]}
        self.assertEqual(self.parse(log_with_phases(phase_marker(payload)))["status"], "available")
        zero = {"version": 1, "duration_seconds": 0,
                "segments": [{"phase": "assembly", "context": "recipe", "offset_seconds": 0, "duration_seconds": 0}]}
        value = dict(zero, status="available", reason=None, source="build-phase-markers")
        self.assertTrue(breakdown.valid_phase_breakdown(value, 0, "finished"))


class ParserTests(unittest.TestCase):
    def test_sequential_sections_include_setup_cleanup_and_exclude_later_step(self):
        value = parsed()
        self.assertTrue(breakdown.valid(value))
        self.assertEqual(value["duration_seconds"], 600.2)
        sections = value["sections"]
        self.assertEqual([s["id"] for s in sections], ["vs-image-1", "orchagent-release-1"])
        self.assertEqual([s["duration_seconds"] for s in sections], [489.8, 110.4])
        self.assertAlmostEqual(sum(s["duration_seconds"] for s in sections), 600.2)
        final = sections[0]["tasks"][-1]
        self.assertEqual(final["id"], "final-image-1")
        self.assertEqual(final["duration_seconds"], 360)
        self.assertEqual(sections[1]["tasks"][0]["outcome"], "cached")

    def test_overlapping_tasks_sorted_by_start_preserve_wallclock(self):
        tasks = parsed()["sections"][0]["tasks"]
        self.assertEqual([task["id"] for task in tasks[:2]], ["base-1", "config-engine-1"])
        self.assertEqual([task["duration_seconds"] for task in tasks[:2]], [60, 50])
        self.assertGreater(tasks[0]["offset_seconds"] + tasks[0]["duration_seconds"], tasks[1]["offset_seconds"])

    def test_private_text_and_unknown_targets_never_survive(self):
        encoded = json.dumps(parsed())
        for secret in ("private-host", "secret", "environment", "target/", "/private/runner/path", "validation after build"):
            self.assertNotIn(secret, encoded)
        malicious = synthetic_log().replace(b"target/docker-base-trixie.gz", b"target/../../private-host.gz")
        self.assertNotIn("private-host", json.dumps(breakdown.parse_log(malicious, job()["steps"][0])))

    def test_absent_truncated_failed_and_invalid_logs_are_explicit(self):
        step = job()["steps"][0]
        cases = [
            (b"", step, "unsupported-log-format"),
            (synthetic_log().split(b"2026-10-10T00:10:00")[0], step, "incomplete-log"),
            (synthetic_log().replace(b"[ finished ] [ target/sonic-vs.img.gz ]", b"missing completion"), step, "incomplete-log"),
            (synthetic_log(), dict(step, conclusion="failure"), "step-not-successful"),
            (synthetic_log(), dict(step, status="in_progress"), "step-not-successful"),
            (synthetic_log(), dict(step, started_at=END, completed_at=START), "invalid-timing"),
            (b"\xff", step, "unsupported-log-format"),
            (synthetic_log().replace(b"00:00:15Z", b"00:00:05Z"), step, "invalid-timing"),
            (synthetic_log().replace(b"Making target/docker-orchagent.gz", b"Making target/private-host.gz"), step, "unsupported-log-format"),
        ]
        for log, metadata, reason in cases:
            with self.subTest(reason=reason):
                self.assertEqual(breakdown.parse_log(log, metadata), breakdown.unavailable(reason))
        with mock.patch.object(breakdown, "MAX_LOG_BYTES", 5):
            self.assertEqual(breakdown.parse_log(synthetic_log(), step)["reason"], "logs-too-large")

    def test_duplicate_or_unmatched_known_markers_reject_partial_measurement(self):
        original = b"2026-10-10T00:00:10Z [ building ] [ target/docker-base-trixie.gz ]\n"
        for log in (synthetic_log().replace(original, original + original), synthetic_log().replace(original, b"")):
            self.assertEqual(breakdown.parse_log(log, job()["steps"][0])["reason"], "incomplete-log")

    def test_ansi_and_versioned_p4rt_only_map_to_static_labels(self):
        value = synthetic_log().replace(b"target/docker-orchagent.gz", b"target/files/trixie/sonic-p4rt-dbgsym_0.0.2_amd64.tar.bz2")
        value = value.replace(b"[ cached ]", b"\x1b[32m[ cached ]\x1b[0m")
        result = breakdown.parse_log(value, job()["steps"][0])
        self.assertEqual(result["sections"][1]["id"], "p4rt-debug-1")
        self.assertEqual(result["sections"][1]["tasks"][0]["label"], "P4RT debug package")
        self.assertNotIn("0.0.2", json.dumps(result))

    def test_strict_schema_rejects_unknown_nested_fields_labels_and_bad_numbers(self):
        mutations = [
            lambda v: v.update(private_field="private-host"),
            lambda v: v.update(version=True),
            lambda v: v.update(duration_seconds=float("nan")),
            lambda v: v.update(duration_seconds=10 ** 400),
            lambda v: v["sections"][0].update(label="private-host"),
            lambda v: v["sections"][0].update(offset_seconds=1),
            lambda v: v["sections"][0]["tasks"][0].update(duration_seconds=float("inf")),
            lambda v: v["sections"][0]["tasks"][0].update(offset_seconds=-1),
            lambda v: v["sections"][0]["tasks"][0].update(duration_seconds=9999),
            lambda v: v["sections"][0]["tasks"][0].update(outcome="private-host"),
            lambda v: v["sections"][0]["tasks"][0].update(id="private-host-1"),
            lambda v: v["sections"][0]["tasks"][0].update(log="secret"),
        ]
        for mutate in mutations:
            value = parsed()
            mutate(value)
            self.assertFalse(breakdown.valid(value))
        self.assertFalse(breakdown.valid(dict(breakdown.unavailable("logs-expired"), reason="private-host")))


class EnrichmentTests(unittest.TestCase):
    def test_optional_phase_field_preserves_cached_v1_without_log_sweep(self):
        api = FakeAPI()
        api.logs[99] = log_with_phases()
        measured = collector.collect(api, REPO, empty())
        legacy = copy.deepcopy(measured)
        del final_tasks(legacy["runs"][0]["jobs"][0]["steps"][0]["breakdown"])[0]["phase_breakdown"]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "history.json"
            for history in (legacy, measured):
                collector.write_atomic(path, history)
                self.assertEqual(collector.load_history(path, REPO), history)
                refreshed = FakeAPI()
                result = collector.collect(refreshed, REPO, history, backfill=True)
                self.assertEqual(result["runs"], history["runs"])
                self.assertEqual(refreshed.log_calls, [])

    def test_phase_measurements_do_not_transfer_across_run_attempts(self):
        api = FakeAPI()
        api.logs[99] = log_with_phases()
        history = collector.collect(api, REPO, empty())
        newer = FakeAPI(latest=[run(attempt=2)])
        newer.logs[99] = synthetic_log()
        result = collector.collect(newer, REPO, history)
        phases = [final_tasks(item["jobs"][0]["steps"][0]["breakdown"])[0]["phase_breakdown"] for item in result["runs"]]
        self.assertEqual([phase["status"] for phase in phases], ["unavailable", "available"])
        self.assertEqual(phases[0]["reason"], "not-recorded")

    def test_corrupt_saved_phase_data_rejects_history_without_discarding_file(self):
        api = FakeAPI()
        api.logs[99] = log_with_phases()
        history = collector.collect(api, REPO, empty())
        final_tasks(history["runs"][0]["jobs"][0]["steps"][0]["breakdown"])[0]["phase_breakdown"]["segments"][0]["hostname"] = "private-host"
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "history.json"
            collector.write_atomic(path, history)
            content = path.read_bytes()
            with self.assertRaises(collector.CollectionError):
                collector.load_history(path, REPO)
            self.assertEqual(path.read_bytes(), content)

    def test_completed_cached_history_enriches_without_job_refetch_or_backfill(self):
        old = collector.collect(FakeAPI(), REPO, empty(), log_jobs=0)
        api = FakeAPI()
        api.logs[99] = synthetic_log()
        result = collector.collect(api, REPO, old)
        detail = result["runs"][0]["jobs"][0]["steps"][0]["breakdown"]
        self.assertEqual(detail["status"], "available")
        self.assertEqual(api.log_calls, [(REPO, 99)])
        self.assertFalse(any(call.endswith("/jobs") for call in api.calls))
        self.assertEqual(old["runs"][0]["jobs"][0]["steps"][0]["breakdown"]["reason"], "pending-collection")

    def test_permanent_details_retained_across_refresh_and_metadata_backfill(self):
        for result_log in (synthetic_log(), collector.LogReadError("logs-expired")):
            api = FakeAPI()
            api.logs[99] = result_log
            old = collector.collect(api, REPO, empty())
            repeat = FakeAPI()
            result = collector.collect(repeat, REPO, old, backfill=True)
            self.assertEqual(result["runs"], old["runs"])
            self.assertEqual(repeat.log_calls, [])

    def test_transient_log_failures_retry_without_breaking_metadata(self):
        api = FakeAPI()
        api.logs[99] = collector.LogReadError("logs-temporarily-unavailable")
        old = collector.collect(api, REPO, empty())
        self.assertEqual(old["runs"][0]["jobs"][0]["image_build_seconds"], 600)
        retry = FakeAPI()
        retry.logs[99] = synthetic_log()
        result = collector.collect(retry, REPO, old)
        self.assertEqual(result["runs"][0]["jobs"][0]["steps"][0]["breakdown"]["status"], "available")
        self.assertEqual(retry.log_calls, [(REPO, 99)])

    def test_budget_reads_newest_jobs_and_preserves_all_history(self):
        api = FakeAPI(latest=[run(run_id=i) for i in range(1, 26)])
        for i in range(1, 26):
            api.jobs[f"repos/{REPO}/actions/runs/{i}/attempts/1/jobs"] = [dict(job(), id=100 + i)]
        old = collector.collect(api, REPO, empty())
        self.assertEqual(len(old["runs"]), 25)
        self.assertEqual([identity for _, identity in api.log_calls], list(range(125, 105, -1)))
        retry = FakeAPI(latest=[])
        result = collector.collect(retry, REPO, old)
        self.assertEqual(len(result["runs"]), 25)
        self.assertEqual([identity for _, identity in retry.log_calls], list(range(105, 100, -1)))

    def test_attempt_job_and_step_identity_changes_never_reuse_measurement(self):
        api = FakeAPI()
        api.logs[99] = synthetic_log()
        old = collector.collect(api, REPO, empty())
        for change in ("attempt", "job", "step"):
            newer = FakeAPI(latest=[run(attempt=2)] if change == "attempt" else [run(updated_at="2026-10-10T00:11:00Z")])
            if change == "job":
                newer.jobs[f"repos/{REPO}/actions/runs/10/attempts/1/jobs"] = [dict(job(), id=100)]
            if change == "step":
                changed = job()
                changed["steps"][0]["number"] = 2
                newer.jobs[f"repos/{REPO}/actions/runs/10/attempts/1/jobs"] = [changed]
            result = collector.collect(newer, REPO, old)
            self.assertEqual(len(newer.log_calls), 1)
            self.assertEqual(result["runs"][0]["jobs"][0]["steps"][0]["breakdown"]["reason"], "logs-expired")
            if change == "attempt":
                self.assertEqual(result["runs"][1], old["runs"][0])

    def test_failed_steps_do_not_fetch_logs(self):
        api = FakeAPI()
        api.jobs[f"repos/{REPO}/actions/runs/10/attempts/1/jobs"] = [job("failure")]
        result = collector.collect(api, REPO, empty())
        self.assertEqual(api.log_calls, [])
        self.assertEqual(result["runs"][0]["jobs"][0]["steps"][0]["breakdown"]["reason"], "step-not-successful")

    def test_existing_combined_step_gets_derived_totals_on_cached_history(self):
        old = collector.collect(FakeAPI(), REPO, empty(), log_jobs=0)
        old_job = old["runs"][0]["jobs"][0]
        old_job["steps"][0]["name"] = "Build the VS image and both OCI container variants"
        old_job["image_build_seconds"] = None
        result = collector.collect(FakeAPI(), REPO, old, log_jobs=0)
        self.assertEqual(result["runs"][0]["jobs"][0]["image_build_seconds"], 600)

    def test_retained_history_schema_validates_nested_breakdowns(self):
        api = FakeAPI()
        api.logs[99] = synthetic_log()
        old = collector.collect(api, REPO, empty())
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "history.json"
            collector.write_atomic(path, old)
            self.assertEqual(collector.load_history(path, REPO), old)
            old["runs"][0]["jobs"][0]["steps"][0]["breakdown"]["sections"][0]["tasks"][0]["label"] = "private-host"
            collector.write_atomic(path, old)
            with self.assertRaises(collector.CollectionError):
                collector.load_history(path, REPO)


class DownloadTests(unittest.TestCase):
    def invoke_script(self, body):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "fake-gh"
            executable.write_text("#!/usr/bin/env python3\n" + body)
            executable.chmod(0o700)
            return collector.GitHub(str(executable)).job_log(REPO, 99)

    def test_raw_bytes_are_returned_and_cli_args_do_not_execute_input(self):
        self.assertEqual(self.invoke_script("import sys\nassert sys.argv[1:] == ['api', '--allow-escape-sequences', '--method', 'GET', 'repos/securely1g/sonic-buildimage/actions/jobs/99/logs']\nsys.stdout.buffer.write(b'log bytes')\n"), b"log bytes")

    def test_download_is_bounded_even_when_process_keeps_writing(self):
        with mock.patch.object(breakdown, "MAX_LOG_BYTES", 1024):
            with self.assertRaises(collector.LogReadError) as raised:
                self.invoke_script("import os\nwhile True: os.write(1,b'x'*65536)\n")
        self.assertEqual(raised.exception.reason, "logs-too-large")

    def test_timeout_terminates_silent_download(self):
        with mock.patch.object(collector, "LOG_TIMEOUT_SECONDS", 0.05):
            with self.assertRaises(collector.LogReadError) as raised:
                self.invoke_script("import time\ntime.sleep(60)\n")
        self.assertEqual(raised.exception.reason, "logs-temporarily-unavailable")

    def test_http_errors_are_fixed_safe_codes(self):
        for status, reason in [(404, "logs-expired"), (410, "logs-expired"), (403, "logs-temporarily-unavailable"), (503, "logs-temporarily-unavailable")]:
            with self.assertRaises(collector.LogReadError) as raised:
                self.invoke_script(f"import sys\nsys.stderr.write('secret private-host HTTP {status}')\nsys.exit(1)\n")
            self.assertEqual(str(raised.exception), reason)


if __name__ == "__main__":
    unittest.main()
