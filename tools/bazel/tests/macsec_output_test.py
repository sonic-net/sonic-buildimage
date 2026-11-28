"""Bind MACsec outputs and incremental evidence to the actual Bazel invocation."""

import contextlib
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

from tools.bazel.ci import build, macsec_image
from tools.bazel.ci.public_artifacts import safe_json


class MacsecOutputTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="MACsec output evidence ")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.workspace = self.root / "checkout"
        self.workspace.mkdir()
        self.execution_root = self.root / "private Bazel root/execroot/_main"
        self.execution_root.mkdir(parents=True)
        self.directory = self.root / "evidence"
        self.directory.mkdir()
        self.configuration = self.workspace / "dockers/docker-macsec/supervisord.conf"
        self.configuration.parent.mkdir(parents=True)
        self.configuration.write_text("original configuration\n")
        self.labels = {
            **{macsec_image.OWNER + name: "bazel-out/layouts/" + name for name in macsec_image.IMAGES},
            **{macsec_image.OWNER + name + ".gz": "bazel-out/archives/" + name + ".gz"
               for name in macsec_image.IMAGES},
            macsec_image.BASE: "bazel-out/layouts/base",
            macsec_image.TARGETS["macsec.source.tar"]: "bazel-out/owners/wpa.tar",
            macsec_image.TARGETS["swss.source.tar"]: "bazel-out/owners/swss.tar",
        }
        self.bazel = self.root / "fake bazel"
        self.bazel.write_text("#!" + sys.executable + "\n" + f"""
import hashlib, json
from pathlib import Path
import sys
root = Path({str(self.execution_root)!r})
labels = {self.labels!r}
args = sys.argv[1:]
if args[0] == 'build':
    contents = Path('dockers/docker-macsec/supervisord.conf').read_bytes()
    for label, relative in labels.items():
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        if '/layouts/' in relative:
            path.mkdir(exist_ok=True)
            path = path / 'digest'
        path.write_bytes(contents if 'docker-macsec' in relative else b'unchanged owner/base')
    events = Path(next(arg.split('=', 1)[1] for arg in args if arg.startswith('--build_event_json_file=')))
    mode = Path('mode').read_text() if Path('mode').exists() else ''
    roots = [] if mode == 'missing-root' else [str(root)]
    if mode == 'multiple-roots': roots.append(str(root.parent))
    if mode == 'relative-root': roots = ['relative/execroot']
    records = [{{'workspaceInfo': {{'localExecRoot': value}}}} for value in roots]
    records.append({{'buildMetrics': {{'actionSummary': {{
        'actionsCreated': '9', 'actionsExecuted': '1',
        'runnerCount': [{{'name': 'internal', 'count': 1}}],
        'actionCacheStatistics': {{'hits': 8, 'misses': 1}}
    }}}}}})
    events.write_text(''.join(json.dumps(record) + '\\n' for record in records))
elif args[0] == 'cquery':
    print(labels[args[-1]])
else:
    raise SystemExit('unexpected operation: ' + args[0])
""")
        self.bazel.chmod(0o755)

    def fake_build(self, events):
        subprocess.run([str(self.bazel), "build", "--build_event_json_file=" + str(events)],
                       cwd=self.workspace, check=True)

    def convenience_link(self, stale):
        if stale:
            old = self.root / "old outputs"
            old.mkdir()
            for relative in self.labels.values():
                path = old / Path(relative).relative_to("bazel-out")
                path.parent.mkdir(parents=True, exist_ok=True)
                if "/layouts/" in relative:
                    path.mkdir(exist_ok=True)
                    path = path / "digest"
                path.write_bytes(b"stale output")
            (self.workspace / "bazel-out").symlink_to(old, target_is_directory=True)
        custom = self.workspace / "custom-out"
        if not custom.exists():
            custom.symlink_to(self.execution_root / "bazel-out", target_is_directory=True)

    def test_archive_collector_ignores_stale_or_missing_convenience_links(self):
        for stale in (False, True):
            with self.subTest(stale=stale):
                self.convenience_link(stale)
                receipt = {"commands": []}
                targets = {"wpa.tar": macsec_image.TARGETS["macsec.source.tar"]}
                with contextlib.redirect_stdout(io.StringIO()):
                    result = build.collect_archives(
                        self.workspace, self.directory, receipt, targets, bazel=str(self.bazel),
                        options=["--symlink_prefix=custom-"], use_execution_root=True)
                self.assertEqual(result["wpa.tar"].read_bytes(), b"unchanged owner/base")
                self.assertEqual(receipt["execution_root"], str(self.execution_root))
                self.assertTrue(all("--symlink_prefix=custom-" in call["argv"] for call in receipt["commands"]))

    def test_bad_execution_root_evidence_preserves_previous_publication(self):
        previous = self.directory / "wpa.tar"
        previous.write_bytes(b"previous publication")
        for mode in ("missing-root", "multiple-roots", "relative-root"):
            with self.subTest(mode=mode):
                (self.workspace / "mode").write_text(mode)
                receipt = {"commands": []}
                with contextlib.redirect_stdout(io.StringIO()), self.assertRaises(ValueError):
                    build.collect_archives(self.workspace, self.directory, receipt,
                        {"wpa.tar": macsec_image.TARGETS["macsec.source.tar"]},
                        bazel=str(self.bazel), use_execution_root=True)
                self.assertEqual(previous.read_bytes(), b"previous publication")
                self.assertEqual(len(receipt["commands"]), 1)
                self.assertEqual(receipt["artifacts"], {})

    def test_declared_outputs_cannot_escape_recorded_execution_root(self):
        outside = self.root / "outside.tar"
        outside.write_bytes(b"outside")
        (self.execution_root / "escape.tar").symlink_to(outside)
        for name in (str(outside), "../outside.tar", "escape.tar"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                build.resolve_output(self.execution_root, name)

    def test_macsec_incremental_uses_recorded_outputs_and_keeps_action_evidence(self):
        self.fake_build(self.directory / "layouts-events.jsonl")
        self.convenience_link(True)
        receipt = {"status": "built", "revision": "a" * 40,
                   "layout_execution_root": str(self.execution_root),
                   "base_layout": self.labels[macsec_image.BASE],
                   "layouts": {name: self.labels[macsec_image.OWNER + name] for name in macsec_image.IMAGES},
                   "artifacts": {name + ".gz": {"sha256": hashlib.sha256(self.configuration.read_bytes()).hexdigest()}
                                 for name in macsec_image.IMAGES}}
        (self.directory / "build-receipt.json").write_text(json.dumps(receipt))

        def layout(path, platform):
            self.assertTrue(path.is_relative_to(self.execution_root))
            return SimpleNamespace(descriptor={"digest": hashlib.sha256((path / "digest").read_bytes()).hexdigest()})

        with mock.patch.object(macsec_image, "ROOT", self.workspace), \
             mock.patch.object(macsec_image, "OPTIONS", ["--symlink_prefix=custom-"]), \
             mock.patch.object(macsec_image, "clean_source"), \
             mock.patch.object(macsec_image, "source_identity", return_value={"revision": "a" * 40}), \
             mock.patch.object(macsec_image.oci_layout, "validate_layout", side_effect=layout), \
             contextlib.redirect_stdout(io.StringIO()):
            macsec_image.incremental(SimpleNamespace(bazel=str(self.bazel)), self.directory)
        result = json.loads((self.directory / "incremental.json").read_text())
        safe_json(result)
        self.assertEqual(result["baseline"], result["restored"])
        self.assertNotEqual(result["baseline"]["images"], result["changed"]["images"])
        self.assertEqual(set(result["invocations"]), {"baseline", "no-op", "configuration-edit", "restored"})
        for record in result["invocations"].values():
            self.assertEqual(record["bep_sha256"], hashlib.sha256((self.directory / record["bep"]).read_bytes()).hexdigest())
            self.assertEqual(record["action_statistics"]["action_cache"], {"hits": 8, "misses": 1})
            self.assertEqual(record["action_statistics"]["actions_executed"], 1)
        self.assertEqual(self.configuration.read_text(), "original configuration\n")

    def test_missing_action_statistics_do_not_claim_cache_hits(self):
        events = self.directory / "no-metrics.jsonl"
        events.write_text(json.dumps({"workspaceInfo": {"localExecRoot": str(self.execution_root)}}) + "\n")
        self.assertEqual(macsec_image.action_metrics(events), {"available": False})

    def test_protobuf_default_zero_counts_are_distinct_from_missing_metrics(self):
        events = self.directory / "zero-metrics.jsonl"
        record = json.dumps({"buildMetrics": {"actionSummary": {"actionCacheStatistics": {}}}}) + "\n"
        events.write_text(record)
        self.assertEqual(macsec_image.action_metrics(events), {
            "available": True, "actions_created": 0, "actions_executed": 0,
            "runners": {}, "action_cache": {"hits": 0, "misses": 0}})
        events.write_text(record + record)
        with self.assertRaisesRegex(ValueError, "ambiguous"):
            macsec_image.action_metrics(events)


if __name__ == "__main__":
    unittest.main()
