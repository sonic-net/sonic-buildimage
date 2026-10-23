"""Behavioral tests for public, bounded Make build timing summaries."""
import copy
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
