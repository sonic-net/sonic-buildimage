"""Test collection of VS image recipe receipts from their producing Actions step."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from test_collect import collector, FakeAPI, REPO, START, END, empty, job, run


def payload():
    return {"version": 1, "images": [
        {"owner": owner, "bazel_seconds": 100, "lock_seconds": 13.5, "compile_seconds": 37,
         "foreign_seconds": 0, "fetch_seconds": 10, "export_seconds": 2,
         "bazel_phases": 3, "profiles": 3, "exports": 2, "failed": False}
        for owner in sorted(collector.breakdown.RECIPE_OWNERS)]}


def marker(value):
    return "SONIC_VS_IMAGE_TIMINGS_V1 " + json.dumps(value, separators=(",", ":"))


def log(lines):
    # The Actions API records whole seconds. Neighbouring steps, the runner's
    # ANSI-coloured command echo and a genuine receipt can all share that second.
    events = [
        ("001", "##[group]Run previous step"), ("002", "##[endgroup]"),
        ("003", marker(payload())), ("010", "##[group]Run set -euo pipefail"),
        ("011", '\x1b[36;1m  python3 -B tools/bazel/ci/summarize_vs_image_timings.py "$timings"\x1b[0m'),
        ("012", marker(payload())), ("013", "##[endgroup]"),
        *(("020", line) for line in lines),
        ("030", "##[group]Run actions/upload-artifact"), ("031", "##[endgroup]"),
        ("032", marker(payload())),
    ]
    return "".join(f"2026-10-10T00:10:00.{ms}0000Z {line}\n" for ms, line in events).encode()


def host_job(build_conclusion="success", summary_step="Summarize VS image build timings"):
    raw = dict(job(), name="VS installer in SONiC slave (AMD64)")
    build = dict(raw["steps"][0], name=collector.HOST_VS_BUILD, conclusion=build_conclusion)
    summary = dict(build, name=summary_step, number=2, started_at=END, conclusion="success")
    return dict(raw, steps=[build, summary], conclusion=build_conclusion)


def api_with(raw, log_bytes):
    api = FakeAPI(latest=[run(path=".github/workflows/bazel-oci.yml")])
    api.jobs[f"repos/{REPO}/actions/runs/10/attempts/1/jobs"] = [raw]
    api.logs[99] = log_bytes
    return api


class RecipeTimingsTest(unittest.TestCase):
    def test_cached_host_build_timing_is_recovered_even_when_later_validation_failed(self):
        for build_outcome, expected in (("success", 600), ("failure", None)):
            with self.subTest(build_outcome=build_outcome):
                raw = dict(host_job(build_conclusion=build_outcome), conclusion="failure")
                cached = collector.normalize_job(raw, REPO, 10)
                cached.update(image_build_seconds=None, image_build_elapsed_seconds=None, image_build_step_numbers=[])
                history_run = {"run_id": 10, "attempt": 1, "workflow_path": ".github/workflows/bazel-oci.yml",
                               "jobs": [cached]}
                collector.enrich_breakdowns(None, REPO, [history_run], limit=0)
                self.assertEqual(cached["image_build_seconds"], expected)
                self.assertEqual(cached["image_build_elapsed_seconds"], 600)
                self.assertEqual(cached["image_build_step_numbers"], [1])
                self.assertEqual(cached["conclusion"], "failure")

    def test_fences_require_command_header_and_standalone_receipt(self):
        step = host_job()["steps"][1]
        parse = collector.breakdown.parse_recipe_log
        receipt = payload()
        # A measured zero is preserved. Missing observations remain null.
        row = receipt["images"][0]
        row.update(exports=0, export_seconds=None, failed=True)
        self.assertEqual(parse(log([marker(receipt)]), step), dict(receipt, status="available"))
        self.assertEqual(parse(log([]), step)["reason"], "not-recorded")
        self.assertEqual(parse(log([marker(receipt)] * 2), step)["reason"], "invalid-timing")
        self.assertEqual(parse(log([marker(receipt)]).replace(b"summarize_vs_image_timings.py",
                                                                b"some_other_script.py"), step)["reason"],
                         "invalid-timing")
        wrong_step = dict(step, started_at=START)
        self.assertEqual(parse(log([marker(receipt)]), wrong_step)["status"], "unavailable")

    def test_invalid_receipts_cannot_publish_unknown_or_unbounded_data(self):
        step = host_job()["steps"][1]
        invalid = [dict(failed=1), dict(owner="private-runner"), dict(owner=[]), dict(profiles=True),
                   dict(exports=3), dict(exports=0), dict(fetch_seconds=None), dict(compile_seconds=-1),
                   dict(lock_seconds=float("inf")), dict(secret="never publish this")]
        for edits in invalid:
            receipt = payload()
            receipt["images"][0].update(edits)
            with self.subTest(edits=edits):
                self.assertEqual(collector.breakdown.parse_recipe_log(log([marker(receipt)]), step),
                                 collector.breakdown.recipe_unavailable("invalid-timing"))
        receipt = payload()
        receipt["images"][0] = receipt["images"][1]
        for text in (marker(receipt), marker(payload()).replace('"owner"', '"owner":"hidden","owner"', 1),
                     marker(payload()) + " " * 4096):
            self.assertEqual(collector.breakdown.parse_recipe_log(log([text]), step)["reason"], "invalid-timing")

    def test_collection_shares_log_budget_and_retries_transient_failures(self):
        raw = host_job()
        api = api_with(raw, collector.LogReadError("logs-temporarily-unavailable"))
        old = collector.collect(api, REPO, empty(), log_jobs=0)
        self.assertEqual(api.log_calls, [])
        self.assertEqual(old["runs"][0]["jobs"][0]["image_recipe_timings"]["reason"], "pending-collection")
        old = collector.collect(api, REPO, old)
        self.assertEqual(api.log_calls, [(REPO, 99)])
        self.assertEqual(old["runs"][0]["jobs"][0]["image_recipe_timings"]["reason"], "logs-temporarily-unavailable")
        retry = api_with(raw, log([marker(payload())]))
        new = collector.collect(retry, REPO, old)
        self.assertEqual(retry.log_calls, [(REPO, 99)])
        result = new["runs"][0]["jobs"][0]
        self.assertEqual(result["image_recipe_timings"]["status"], "available")
        self.assertEqual(result["image_build_seconds"], 600)
        self.assertEqual(result["image_build_step_numbers"], [1])
        # Re-reading identical metadata preserves the receipt without another download.
        saved = collector.collect(retry, REPO, new, backfill=True)
        self.assertEqual(retry.log_calls, [(REPO, 99)])
        self.assertEqual(saved["runs"][0]["jobs"], new["runs"][0]["jobs"])

    def test_failed_build_can_publish_partial_timings_and_old_jobs_stay_readable(self):
        receipt = payload()
        receipt["images"][0].update(profiles=0, lock_seconds=None, compile_seconds=None,
                                    foreign_seconds=None, fetch_seconds=None, failed=True)
        api = api_with(host_job("failure"), log([marker(receipt)]))
        detail = collector.collect(api, REPO, empty())["runs"][0]["jobs"][0]
        self.assertIsNone(detail["image_build_seconds"])
        self.assertEqual(detail["image_recipe_timings"], dict(receipt, status="available"))
        raw = host_job()
        raw["steps"].pop()
        detail = collector.collect(api_with(raw, b""), REPO, empty(), log_jobs=0)["runs"][0]["jobs"][0]
        self.assertNotIn("image_recipe_timings", detail)

    def test_history_rejects_unbound_receipts_and_does_not_reuse_changed_jobs(self):
        raw = host_job()
        saved = collector.collect(api_with(raw, log([marker(payload())])), REPO, empty())
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "history.json"
            collector.write_atomic(path, saved)
            self.assertEqual(collector.load_history(path, REPO), saved)
            for change in ("step", "value", "workflow"):
                bad = copy.deepcopy(saved)
                item = bad["runs"][0]["jobs"][0]
                if change == "step":
                    item["steps"].pop()
                elif change == "value":
                    item["image_recipe_timings"]["images"][0]["profiles"] = 0
                else:
                    bad["runs"][0]["workflow_path"] = ".github/workflows/bazel.yml"
                collector.write_atomic(path, bad)
                with self.subTest(change=change), self.assertRaises(collector.CollectionError):
                    collector.load_history(path, REPO)
        raw["steps"][1]["number"] = 3
        api = api_with(raw, collector.LogReadError("logs-expired"))
        updated = collector.collect(api, REPO, saved, backfill=True)["runs"][0]["jobs"][0]
        self.assertEqual(api.log_calls, [(REPO, 99)])
        self.assertEqual(updated["image_recipe_timings"]["reason"], "logs-expired")


class BazelRecipeTimingsTest(unittest.TestCase):
    summary_step = "Summarize VS Bazel build timings"

    def row(self, owner):
        return dict(payload()["images"][0], owner=owner, bazel_expected=3, profiles_expected=3, exports_expected=2)

    def line(self, obj):
        return "SONIC_VS_BAZEL_TIMINGS_V2 " + json.dumps(obj, separators=(",", ":"))

    def parse(self, lines):
        return collector.breakdown.parse_recipe_log(log(lines), host_job(summary_step=self.summary_step)["steps"][1])

    def test_dynamic_targets_and_variable_command_counts_survive_collection_and_history(self):
        # The kernel launcher has one elapsed measurement but three independent profiles.
        rows = [dict(self.row("linux-kernel"), bazel_phases=1, bazel_expected=1, exports=0,
                     exports_expected=0, export_seconds=None)]
        rows += [self.row(owner) for owner in ("docker-database.gz", "docker-sysmgr.gz", "docker-gnmi.gz",
                                               "docker-snmp.gz", "docker-base-trixie.gz")]
        lines = [self.line({"recipe": row}) for row in rows] + [self.line({"count": len(rows)})]
        raw = host_job("failure", self.summary_step)
        api = api_with(raw, log(lines))
        saved = collector.collect(api, REPO, empty())
        self.assertEqual(api.log_calls, [(REPO, 99)])
        job_data = saved["runs"][0]["jobs"][0]
        self.assertEqual(job_data["image_recipe_timings"], {"version": 2, "status": "available", "recipes": rows})
        self.assertIsNone(job_data["image_build_seconds"])
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "history.json"
            collector.write_atomic(path, saved)
            self.assertEqual(collector.load_history(path, REPO), saved)
            job_data["steps"][1]["name"] = "Summarize VS image build timings"
            collector.write_atomic(path, saved)
            with self.assertRaises(collector.CollectionError):
                collector.load_history(path, REPO)

    def test_multiple_lines_exceeding_one_marker_size_and_empty_completed_report(self):
        rows = [self.row(f"docker-owner-{index}.gz") for index in range(48)]
        lines = [self.line({"recipe": row}) for row in rows]
        self.assertGreater(sum(map(len, lines)), 4096)
        self.assertEqual(self.parse(lines + [self.line({"count": len(rows)})])["recipes"], rows)
        self.assertEqual(self.parse([self.line({"count": 0})]), {"version": 2, "status": "available", "recipes": []})
        self.assertEqual(self.parse([]), {"version": 2, "status": "unavailable", "reason": "not-recorded", "recipes": []})

    def test_optional_numeric_build_evidence_survives_history_and_rejects_untrusted_details(self):
        build = {"expected": 1, "timings": 1, "profiles": 1, "seconds": 30,
                 **{key: 0 for key in collector.breakdown.BUILD_PROFILE_METRICS}}
        build.update(lock_seconds=5, repository_seconds=12, repository_read_seconds=10)
        row = dict(self.row("docker-example.gz"), build=build)
        lines = [self.line({"recipe": row}), self.line({"count": 1})]
        saved = collector.collect(api_with(host_job(summary_step=self.summary_step), log(lines)), REPO, empty())
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "history.json"
            collector.write_atomic(path, saved)
            self.assertEqual(collector.load_history(path, REPO), saved)
        self.assertEqual(self.parse(lines)["recipes"][0]["build"], build)
        for edits in ({"seconds": "30"}, {"repository_seconds": float("inf")}, {"expected": True},
                      {"timings": 4}, {"profiles": 4}, {"lock_seconds": 31},
                      {"repository_seconds": -1}, {"repository": "never publish"},
                      {"profiles": 0}, {"expected": 0}):
            with self.subTest(edits=edits):
                candidate = dict(row, build=dict(build, **edits))
                result = self.parse([self.line({"recipe": candidate}), self.line({"count": 1})])
                self.assertEqual(result["reason"], "invalid-timing")
        partial = dict(build, profiles=0, **{key: None for key in collector.breakdown.BUILD_PROFILE_METRICS})
        row["build"] = partial
        self.assertEqual(self.parse([self.line({"recipe": row}), self.line({"count": 1})])["recipes"][0]["build"], partial)

    def batch(self):
        group = dict(self.row("vs-container-images"), exports_expected=0, exports=0, export_seconds=None)
        image = dict(self.row("docker-syncd-vs.gz"), bazel_phases=0, bazel_expected=0, profiles=0, profiles_expected=0,
                     bazel_seconds=None, compile_seconds=None, lock_seconds=None, foreign_seconds=None, fetch_seconds=None)
        main = [self.line({"recipe": row}) for row in [group, image]] + [self.line({"count": 2})]
        member = {"owner": image["owner"], "action_seconds": 50.25, "actions": 4, "fetch_seconds": 1.25,
                  "archive_seconds": 38., "compress_seconds": 3.}
        totals = {"other_action_seconds": 12., "parallel_seconds": 0., "no_action_seconds": 3.}
        return main, member, totals

    def test_optional_container_markers_are_bounded_and_cannot_invalidate_the_original_receipt(self):
        main, member, totals = self.batch()
        def encoded(values):
            return ["SONIC_VS_CONTAINER_TIMINGS_V1 " + json.dumps(value) for value in values]
        trailer = {"count": 1, **totals}
        lines = encoded([{"row": member}, trailer])
        for order in (main + lines, lines + main):
            result = self.parse(order)
            self.assertEqual(result["container_breakdown"], {"rows": [member], **totals})
            self.assertTrue(collector.breakdown.valid_recipes(result))
        existing = self.parse(main)
        self.assertEqual(existing["container_breakdown"], {"status": "unavailable", "reason": "not-recorded"})
        for invalid in ([{"row": member}], [trailer], [{"row": member}, dict(trailer, count=True)],
                        [{"row": dict(member, owner="unknown.gz")}, trailer],
                        [{"row": dict(member, actions=True)}, trailer],
                        [{"row": dict(member, compress_seconds=-1)}, trailer],
                        [{"row": dict(member, argv="private")}, trailer],
                        [{"row": member}, trailer, trailer],
                        [{"row": member}]*2 + [dict(trailer, count=2)]):
            with self.subTest(invalid=invalid):
                parsed = self.parse(main + encoded(invalid))
                self.assertEqual(parsed["recipes"], existing["recipes"])
                self.assertEqual(parsed["container_breakdown"], {"status": "unavailable", "reason": "invalid-timing"})
        for malformed in ["{} trailing", '{"row":' + "x"*5000, '{"row":false,"row":true}']:
            parsed = self.parse(main + ["SONIC_VS_CONTAINER_TIMINGS_V1 " + malformed])
            self.assertEqual(parsed["recipes"], existing["recipes"])
            self.assertEqual(parsed["container_breakdown"]["reason"], "invalid-timing")

    def test_previously_retained_batch_is_enriched_once_without_erasing_old_exports(self):
        main, member, totals = self.batch()
        values = [{"row": member}, dict(totals, count=1)]
        lines = main + ["SONIC_VS_CONTAINER_TIMINGS_V1 " + json.dumps(value) for value in values]
        raw = host_job(summary_step=self.summary_step)
        for get_log, expected in [(log(lines), {"rows": [member], **totals}),
                                  (log(main), {"status": "unavailable", "reason": "not-recorded"}),
                                  (collector.LogReadError("logs-expired"), {"status": "unavailable", "reason": "logs-expired"})]:
            with self.subTest(expected=expected):
                previous = collector.collect(api_with(raw, log(main)), REPO, empty())
                old = previous["runs"][0]["jobs"][0]["image_recipe_timings"]
                del old["container_breakdown"]
                api = api_with(raw, get_log)
                updated = collector.collect(api, REPO, previous)
                saved = updated["runs"][0]["jobs"][0]["image_recipe_timings"]
                self.assertEqual(api.log_calls, [(REPO, 99)])
                self.assertEqual(saved, {**old, "container_breakdown": expected})
                api.log_calls.clear()
                collector.collect(api, REPO, updated)
                self.assertEqual(api.log_calls, [])

    def test_container_compile_and_total_times_are_optional_in_new_logs_and_retained_history(self):
        main, member, totals = self.batch()
        raw = host_job(summary_step=self.summary_step)
        for metrics in ({}, {"compile_seconds": 0}, {"total_seconds": 60},
                        {"compile_seconds": 7.25, "total_seconds": member["action_seconds"]}):
            with self.subTest(metrics=metrics), tempfile.TemporaryDirectory() as tmp:
                row = {**member, **metrics}
                lines = main + ["SONIC_VS_CONTAINER_TIMINGS_V1 " + json.dumps(value)
                                for value in ({"row": row}, {"count": 1, **totals})]
                api = api_with(raw, log(lines))
                saved = collector.collect(api, REPO, empty())
                self.assertEqual(saved["runs"][0]["jobs"][0]["image_recipe_timings"]["container_breakdown"]["rows"],
                                 [row])
                path = Path(tmp) / "history.json"
                collector.write_atomic(path, saved)
                retained = collector.load_history(path, REPO)
                self.assertEqual(retained, saved)
                api.log_calls.clear()
                collector.collect(api, REPO, retained)
                self.assertEqual(api.log_calls, [])

    def test_invalid_container_times_preserve_the_recipe_measurements(self):
        main, member, totals = self.batch()
        recipes = self.parse(main)["recipes"]
        invalid_numbers = (None, True, "7.25", -1, float("nan"), float("inf"), 2**53)
        for field, out_of_bounds in (("compile_seconds", member["action_seconds"] + 0.1),
                                     ("total_seconds", member["action_seconds"] - 0.1)):
            for invalid in (*invalid_numbers, out_of_bounds):
                with self.subTest(field=field, invalid=invalid):
                    row = {**member, field: invalid}
                    lines = main + ["SONIC_VS_CONTAINER_TIMINGS_V1 " + json.dumps(value)
                                    for value in ({"row": row}, {"count": 1, **totals})]
                    parsed = self.parse(lines)
                    self.assertEqual(parsed["recipes"], recipes)
                    self.assertEqual(parsed["container_breakdown"], {"status": "unavailable", "reason": "invalid-timing"})

    def test_rejects_partial_duplicate_and_mixed_transports_and_untrusted_rows(self):
        row = self.row("docker-eventd.gz")
        item = self.line({"recipe": row})
        end = self.line({"count": 1})
        for lines in ([item], [end], [item, item, self.line({"count": 2})], [item, end, end],
                      [item, end, item], [item, self.line({"count": True})], [marker(payload()), end]):
            with self.subTest(lines=lines):
                self.assertEqual(self.parse(lines)["reason"], "invalid-timing")
        for edits in ({"owner": "/home/build"}, {"owner": "https://elsewhere.invalid"},
                      {"owner": "foo --remote_header=bar"}, {"owner": "é"}, {"owner": "a" * 193},
                      {"exports_expected": 1}, {"profiles_expected": -1}, {"bazel_expected": 65536},
                      {"bazel_expected": True}, {"profiles": 0}, {"lock_seconds": None},
                      {"bazel_expected": 1}, {"argv": ["--unpublished-argument"]}):
            with self.subTest(edits=edits):
                self.assertEqual(self.parse([self.line({"recipe": dict(row, **edits)}), end])["reason"], "invalid-timing")
        over_limit = [{"recipe": self.row(f"recipe-{n}")} for n in range(collector.breakdown.MAX_RECIPES + 1)]
        self.assertEqual(self.parse([self.line(obj) for obj in [*over_limit, {"count": len(over_limit)}]])["reason"],
                         "invalid-timing")

    def test_named_evidence_requires_complete_groups_and_survives_history(self):
        row = dict(self.row("linux-kernel"), bazel_phases=1, bazel_expected=1, exports=0,
                   exports_expected=0, export_seconds=None, build=None, details={
                       "scope": "kernel-build", "expected": 1, "profiles": 1, "phase_count": 1,
                       "repositories": [{"name": "//:apt+example", "seconds": 12, "read_seconds": 10, "read_count": 2}],
                       "critical_path": [{"kind": "OCIImage", "target": "//example:image", "seconds": 8}],
                       "phases": [{"phase": "launcher", "seconds": 100, "lock_seconds": None,
                                   "start_epoch_seconds": 1794944781.5, "exit_code": 0}],
                   })

        def lines(candidate):
            header = copy.deepcopy(candidate)
            header["details"].update(repositories=[], critical_path=[], phases=[])
            return [self.line({"recipe": header}), *[
                self.line({"evidence": {"owner": candidate["owner"], "group": group,
                                       "values": candidate["details"][group]}})
                for group in ("repositories", "critical_path", "phases")],
                self.line({"count": 1, "evidence_count": 3})]

        records = lines(row)
        self.assertEqual(self.parse(records)["recipes"], [row])
        saved = collector.collect(
            api_with(host_job(summary_step=self.summary_step), log(records)), REPO, empty())
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "history.json"
            collector.write_atomic(path, saved)
            self.assertEqual(collector.load_history(path, REPO), saved)
        for records in (
                lines(row)[:-1], lines(row)[:2] + lines(row)[3:],
                lines(row)[:2] + lines(row)[1:], list(reversed(lines(row))),
                lines(row)[:-1] + [self.line({"count": 1})],
                lines(row)[:-1] + [self.line({"count": 1, "evidence_count": True})],
                lines(row)[:1] + [self.line({"evidence": {
                    "owner": "other-owner", "group": "repositories", "values": []}})] + lines(row)[2:]):
            with self.subTest(records=records):
                self.assertEqual(self.parse(records)["reason"], "invalid-timing")
        for group, edits in (
                ("repositories", {"name": "/home/private"}),
                ("repositories", {"name": "//:example\n"}),
                ("repositories", {"read_seconds": 13}),
                ("repositories", {"read_count": True}),
                ("repositories", {"command": "--token=never-publish"}),
                ("critical_path", {"kind": "Unknown --token=never-publish"}),
                ("critical_path", {"target": "/home/private"}),
                ("critical_path", {"target": "platform/../private"}),
                ("critical_path", {"target": "//example:../private"}),
                ("critical_path", {"target": "//..:image"}),
                ("critical_path", {"target": "//example:image --token=x"}),
                ("critical_path", {"target": "//example:image\n"}),
                ("phases", {"phase": "build --token=x"}),
                ("phases", {"start_epoch_seconds": 253402300800}),
                ("phases", {"exit_code": True})):
            candidate = copy.deepcopy(row)
            candidate["details"][group][0].update(edits)
            with self.subTest(group=group, edits=edits):
                self.assertEqual(self.parse(lines(candidate))["reason"], "invalid-timing")
        candidate = copy.deepcopy(row)
        candidate["details"]["repositories"] *= 9
        self.assertEqual(self.parse(lines(candidate))["reason"], "invalid-timing")
        candidate = copy.deepcopy(row)
        candidate["details"]["profiles"] = 0
        self.assertEqual(self.parse(lines(candidate))["reason"], "invalid-timing")

        longest = copy.deepcopy(row)
        longest.update(exports=7, exports_expected=7, export_seconds=2)
        longest["details"].update(
            phase_count=8,
            repositories=[{"name": "//:" + "r" * 188, "seconds": 9007199254740991,
                           "read_seconds": 9007199254740991, "read_count": 1000000}] * 8,
            critical_path=[{"kind": "GoToolchainBinaryBuild", "target": "//example:".ljust(240, "p"),
                            "seconds": 9007199254740991}] * 8,
            phases=[{"phase": "p" * 192, "seconds": 9007199254740991,
                     "lock_seconds": 9007199254740991, "start_epoch_seconds": 253402300799,
                     "exit_code": 255}] * 8)
        self.assertEqual(self.parse(lines(longest))["recipes"], [longest])
        self.assertTrue(all(len(line.encode()) + 192 <= 4096 for line in lines(longest)))
