import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from urllib.parse import parse_qs, urlsplit

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "ci"))
import vs_host_base_snapshot as snapshot


REPO = "securely1g/sonic-buildimage"
PR_NAME = snapshot.PREFIX + "pr-271"
MASTER = snapshot.MASTER


class HostBaseSnapshotTest(unittest.TestCase):
    def setUp(self):
        self.env = dict(GITHUB_EVENT_NAME="pull_request", GITHUB_REF="refs/pull/271/merge",
                        GITHUB_REPOSITORY=REPO, GITHUB_RUN_ID="100", GITHUB_RUN_ATTEMPT="1")
        self.pr = {"number": 271, "head": {"ref": "host-base", "repo": {"full_name": REPO}},
                   "base": {"repo": {"full_name": REPO}}}
        self.event = {"pull_request": self.pr}
        self.artifacts = []
        self.runs = {100: self.run_data()}

    def run_data(self):
        return {"workflow_id": 42, "repository": {"full_name": REPO, "id": 20},
                "head_repository": {"full_name": REPO}, "head_branch": "host-base",
                "event": "pull_request", "pull_requests": [{"number": 271,
                    "head": {"repo": {"id": 20}}, "base": {"repo": {"id": 20}}}]}

    def add(self, artifact_id, run_id, name=PR_NAME, **changes):
        self.artifacts.append({"id": artifact_id, "name": name, "workflow_run": {"id": run_id},
                               "expired": False, "expires_at": "2100-01-01T00:00:00Z",
                               "created_at": "2026-11-21T09:12:13Z", **changes})
        if run_id not in self.runs:
            self.runs[run_id] = self.run_data()
            if name == MASTER:
                self.runs[run_id].update(event="push", head_branch="master", pull_requests=[])
        return self.runs[run_id]

    def api(self, endpoint):
        if endpoint.startswith("artifacts?"):
            query = parse_qs(urlsplit(endpoint).query)
            matches = [a for a in self.artifacts if a["name"] == query["name"][0]]
            offset = 100 * (int(query["page"][0]) - 1)
            return {"artifacts": matches[offset:offset + 100], "total_count": len(matches)}
        run_id = int(endpoint.removeprefix("runs/"))
        if run_id not in self.runs:
            raise FileNotFoundError("run expired")
        return self.runs[run_id]

    def selected(self):
        names, _ = snapshot.scopes(self.env, self.event)
        return snapshot.select(self.api, names, self.env, self.event)

    def test_only_a_same_repository_pr_and_master_push_publish(self):
        self.assertEqual(snapshot.scopes(self.env, self.event), ([PR_NAME, MASTER], PR_NAME))
        self.pr["head"]["repo"]["full_name"] = "fork/buildimage"
        self.assertEqual(snapshot.scopes(self.env, self.event), ([], None))
        self.pr["head"]["repo"]["full_name"] = REPO
        self.env["GITHUB_REF"] = "refs/pull/500/merge"
        self.assertEqual(snapshot.scopes(self.env, self.event), ([], None))
        for kind, ref, expected in [
            ("push", "refs/heads/master", ([MASTER], MASTER)),
            ("push", "refs/heads/other", ([], None)),
            ("workflow_dispatch", "refs/heads/other", ([MASTER], None)),
            ("pull_request_target", "refs/heads/master", ([], None)),
        ]:
            self.assertEqual(snapshot.scopes(dict(self.env, GITHUB_EVENT_NAME=kind, GITHUB_REF=ref), {}), expected)

    def test_latest_valid_pr_wins_before_master(self):
        self.add(90, 9, MASTER)
        self.add(8, 8)
        self.add(2, 2, created_at="2026-11-21T12:13:14Z")
        self.add(3, 3, expired=True, created_at="2026-11-23T00:01:00Z")
        self.add(4, 4, expires_at="2000-01-01T00:00:00Z")
        self.assertEqual(self.selected(), {"read_artifact_id": 2, "read_run_id": 2})

    def test_misnamed_or_unqualified_producer_falls_back_to_master(self):
        self.add(1, 1, MASTER)
        self.add(2, 2)["workflow_id"] = 43
        self.add(3, 3)["event"] = "workflow_dispatch"
        self.add(4, 4)["head_repository"] = {"full_name": "fork/buildimage"}
        self.add(5, 5)["head_branch"] = "different-pr"
        self.add(6, 6)["pull_requests"][0]["number"] = 123
        self.add(7, 7)["pull_requests"][0]["head"]["repo"]["id"] = 999
        self.add(8, 8)["pull_requests"][0]["base"]["repo"]["id"] = 999
        self.add(9, 9)["pull_requests"] = []
        self.add(10, 10, MASTER)["event"] = "pull_request"
        self.add(11, 11, MASTER)["head_branch"] = "other-branch"
        self.assertEqual(self.selected(), {"read_artifact_id": 1, "read_run_id": 1})

    def test_reruns_can_restore_their_previous_attempt(self):
        self.add(8, 100)
        self.assertEqual(self.selected(), {})
        self.env["GITHUB_RUN_ATTEMPT"] = "2"
        self.assertEqual(self.selected(), {"read_artifact_id": 8, "read_run_id": 100})
        self.runs[100]["pull_requests"] = []
        self.assertEqual(self.selected(), {})

    def test_search_continues_past_a_page_of_expired_snapshots(self):
        for number in range(100):
            self.add(number, 50, expired=True)
        self.add(150, 75)
        self.assertEqual(self.selected(), {"read_artifact_id": 150, "read_run_id": 75})

    def test_artifacts_without_a_surviving_producer_are_ignored(self):
        self.add(20, 20, MASTER)
        self.add(21, 21)
        del self.runs[21]
        self.add(22, 22, workflow_run=None)
        self.assertEqual(self.selected(), {"read_artifact_id": 20, "read_run_id": 20})

    def test_workflow_outputs(self):
        self.add(65, 62)
        with tempfile.TemporaryDirectory() as work:
            event = Path(work) / "event.json"
            event.write_text(json.dumps(self.event))
            output = Path(work) / "output"
            with patch.dict(os.environ, dict(self.env, GITHUB_OUTPUT=str(output), GITHUB_EVENT_PATH=str(event))), \
                    patch.object(snapshot, "github_api", self.api):
                snapshot.main()
            self.assertEqual(output.read_text().splitlines(),
                             ["write_name=" + PR_NAME, "read_artifact_id=65", "read_run_id=62"])

    def test_configured_host_reads_only_master_and_prs_cannot_publish(self):
        name = snapshot.CONFIGURED_MASTER
        self.add(61, 70, name)
        self.runs[70].update(head_branch="master", event="push")
        self.add(62, 71, name, created_at="2026-11-22T09:12:13Z")
        with tempfile.TemporaryDirectory() as work:
            event = Path(work) / "event.json"
            event.write_text(json.dumps(self.event))
            output = Path(work) / "output"
            with patch.dict(os.environ, dict(self.env, GITHUB_OUTPUT=str(output), GITHUB_EVENT_PATH=str(event))), \
                    patch.object(snapshot, "github_api", self.api):
                snapshot.main(configured_host=True)
                self.assertEqual(output.read_text().splitlines(), ["read_artifact_id=61", "read_run_id=70"])
                output.unlink()
                with patch.dict(os.environ, GITHUB_EVENT_NAME="push", GITHUB_REF="refs/heads/master"):
                    snapshot.main(configured_host=True)
                self.assertIn("write_name=" + name, output.read_text().splitlines())

    def test_api_never_forwards_authorization_on_redirect(self):
        with patch.dict(os.environ, GITHUB_REPOSITORY=REPO, GITHUB_TOKEN="fixture-secret",
                        GITHUB_API_URL="https://api.github.com"), \
                patch.object(snapshot.http.client, "HTTPSConnection") as connect:
            conn = connect.return_value
            conn.getresponse.return_value.status = 302
            conn.getresponse.return_value.headers = {"location": "https://another.example/download"}
            with self.assertRaisesRegex(RuntimeError, "^GitHub Actions API returned HTTP 302$"):
                snapshot.github_api("runs/33")
            connect.assert_called_once_with("api.github.com", timeout=20)
            conn.request.assert_called_once()
            self.assertEqual(conn.request.call_args.kwargs["headers"]["Authorization"], "Bearer fixture-secret")
            conn.close.assert_called_once()


if __name__ == "__main__":
    unittest.main()
