"""Exercise shared archive and declared-filegroup export with unrelated targets."""

import contextlib
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import build


class BuildEvidenceTest(unittest.TestCase):
    """Exercise collection through a subprocess that simulates Bazel commands."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.workspace = self.root / 'checkout'
        self.workspace.mkdir()
        self.directory = self.root / 'evidence'
        self.directory.mkdir()
        self.receipt = {'commands': []}
        self.targets = {'telemetry.tar': '@telemetry//dist:runtime',
                        'routing.tar': '@routing//:package'}
        self.mode = self.workspace / 'mode'
        self.mode.write_text('')
        self.bazel = self.root / 'bazel executable'
        self.bazel.write_text('#!/usr/bin/env python3\n' + f'''
import json, pathlib, sys
args = sys.argv[1:]
mode = pathlib.Path('mode').read_text()
labels = {self.targets!r}
if args[0] == 'build':
    if mode == 'build_failure':
        raise SystemExit('fixture build failed')
    output = pathlib.Path('bazel-bin/archive outputs')
    output.mkdir(parents=True, exist_ok=True)
    for name in labels:
        (output / name).write_bytes(b'' if mode == 'empty_file' else ('payload ' + name).encode())
elif args[0] == 'cquery':
    print('INFO: query diagnostic', file=sys.stderr)
    label = args[-1]
    if mode == 'query_failure':
        print('bogus-but-existing-output.tar')
        raise SystemExit('fixture query failed')
    name = next(name for name, target in labels.items() if target == label)
    if mode != 'no_outputs':
        print('bazel-bin/archive outputs/' + ('absent.tar' if mode == 'missing_file' else name))
    if mode == 'multiple_outputs': print('bazel-bin/second.tar')
else:
    raise SystemExit('unexpected operation: ' + args[0])
''')
        self.bazel.chmod(0o755)
        self.options = ['--platforms=@platforms//:unrelated', '--jobs=2']

    def collect(self):
        with contextlib.redirect_stdout(io.StringIO()):
            return build.collect_archives(self.workspace, self.directory, self.receipt, self.targets,
                                          bazel=str(self.bazel), options=self.options)

    def test_two_consumers_keep_labels_options_bytes_and_hash_receipts(self):
        """Export unrelated targets with correct evidence and preserve unchanged mtimes."""
        paths = self.collect()
        self.assertEqual(set(paths), set(self.targets))
        for name, target in self.targets.items():
            content = ('payload ' + name).encode()
            self.assertEqual(paths[name].read_bytes(), content)
            self.assertEqual(paths[name].stat().st_mode & 0o777, 0o644)
            self.assertEqual(self.receipt['artifacts'][name], {
                'target': target, 'bytes': len(content), 'sha256': hashlib.sha256(content).hexdigest()})
            self.assertIn('INFO: query diagnostic', (self.directory / (name + '.query.log')).read_text())
        commands = [command['argv'] for command in self.receipt['commands']]
        self.assertEqual([command[1] for command in commands], ['build', 'cquery', 'cquery'])
        for command in commands:
            for option in self.options: self.assertIn(option, command)
        self.assertEqual(commands[0][-2:], list(self.targets.values()))
        before = {name: path.stat().st_mtime_ns for name, path in paths.items()}
        self.collect()
        self.assertEqual(before, {name: path.stat().st_mtime_ns for name, path in paths.items()})

    def test_query_failures_keep_prior_outputs_and_command_evidence(self):
        """Keep prior publications and diagnostics when queries fail or yield invalid files."""
        for mode in ('query_failure', 'no_outputs', 'multiple_outputs', 'missing_file', 'empty_file'):
            with self.subTest(mode=mode):
                self.mode.write_text(mode)
                self.receipt = {'commands': []}
                previous = self.directory / 'telemetry.tar'
                previous.write_bytes(b'previous publication')
                mtime = previous.stat().st_mtime_ns
                with self.assertRaises((ValueError, subprocess.CalledProcessError)):
                    self.collect()
                self.assertEqual((previous.read_bytes(), previous.stat().st_mtime_ns),
                                 (b'previous publication', mtime))
                self.assertEqual(self.receipt['artifacts'], {})
                self.assertEqual(len(self.receipt['commands']), 2)
                self.assertIn('INFO: query diagnostic', (self.directory / 'telemetry.tar.query.log').read_text())

    def test_build_failure_stops_queries_and_retains_failure_receipt(self):
        """Stop after a failed build and retain its exit status and diagnostic log."""
        self.mode.write_text('build_failure')
        with self.assertRaises(subprocess.CalledProcessError): self.collect()
        self.assertEqual(self.receipt['artifacts'], {})
        self.assertEqual(len(self.receipt['commands']), 1)
        self.assertEqual(self.receipt['commands'][0]['returncode'], 1)
        self.assertIn('fixture build failed', (self.directory / 'build.log').read_text())

    def test_failed_copy_keeps_prior_archive_and_no_completed_export_receipt(self):
        """Preserve the published archive and remove temporary output when copying fails."""
        previous = self.directory / 'telemetry.tar'
        previous.write_bytes(b'previous publication')
        with mock.patch('tools.bazel.build_helpers.shutil.copyfile', side_effect=OSError('disk full')):
            with self.assertRaisesRegex(OSError, 'disk full'): self.collect()
        self.assertEqual(previous.read_bytes(), b'previous publication')
        self.assertEqual(self.receipt['artifacts'], {})
        self.assertEqual(sorted(path.name for path in self.directory.glob('telemetry.tar.*')),
                         ['telemetry.tar.query.log'])



class DeclaredArtifactTest(unittest.TestCase):
    """Export files selected by an owner BUILD without a second Python inventory."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.workspace = self.root / "checkout"
        self.workspace.mkdir()
        self.directory = self.root / "evidence"
        self.directory.mkdir()
        self.receipt = {"commands": []}
        self.target = "//containers/example:ci_artifacts"
        self.options = ["--jobs=2"]
        self.mode = self.workspace / "mode"
        self.mode.write_text("pass")
        self.bazel = self.root / "bazel"
        self.bazel.write_text("#!" + sys.executable + "\n" + r"""
from pathlib import Path
import sys
mode = Path('mode').read_text()
if sys.argv[1] == 'build':
    if mode == 'build-failure': raise SystemExit('build failed')
    names = ['first/telemetry.tar', 'second/routing.tar', 'third/telemetry.tar']
    if mode.startswith('collision:'): names.append('collision/' + mode.split(':', 1)[1])
    for name in names:
        output = Path('bazel-bin/archive outputs') / name
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(b'' if mode == 'empty-file' else ('payload ' + name).encode())
elif sys.argv[1] == 'cquery':
    print('query diagnostic', file=sys.stderr)
    if mode == 'query-failure': raise SystemExit('query failed')
    if mode != 'empty-group':
        print('bazel-bin/archive outputs/first/telemetry.tar')
        second = 'collision/' + mode.split(':', 1)[1] if mode.startswith('collision:') else {
            'duplicate-basename': 'third/telemetry.tar',
            'missing-file': 'second/absent.tar',
        }.get(mode, 'second/routing.tar')
        print('bazel-bin/archive outputs/' + second)
else:
    raise SystemExit('unexpected command')
""")
        self.bazel.chmod(0o755)

    def collect(self):
        with contextlib.redirect_stdout(io.StringIO()):
            return build.collect_artifacts(self.workspace, self.directory, self.receipt, self.target,
                                           bazel=str(self.bazel), options=self.options)

    def test_collects_declared_files_by_basename_with_hash_receipts(self):
        """Collect all filegroup outputs from Bazel, retaining target identity and exact bytes."""
        paths = self.collect()
        self.assertEqual(set(paths), {"telemetry.tar", "routing.tar"})
        for name, original in (("telemetry.tar", "first/telemetry.tar"), ("routing.tar", "second/routing.tar")):
            content = ("payload " + original).encode()
            self.assertEqual(paths[name].read_bytes(), content)
            self.assertEqual(self.receipt["artifacts"][name], {
                "target": self.target, "output": "bazel-bin/archive outputs/" + original,
                "bytes": len(content), "sha256": hashlib.sha256(content).hexdigest()})
            self.assertEqual(paths[name].stat().st_mode & 0o777, 0o644)
        commands = [command["argv"] for command in self.receipt["commands"]]
        self.assertEqual([command[1] for command in commands], ["build", "cquery"])
        for command in commands:
            self.assertIn(self.target, command)
            self.assertIn("--jobs=2", command)

    def test_empty_group_is_a_valid_contract_only_job(self):
        """Tests may establish contracts without publishing a production image or archive."""
        self.mode.write_text("empty-group")
        self.assertEqual(self.collect(), {})
        self.assertEqual(self.receipt.get("artifacts", {}), {})

    def test_duplicate_names_fail_before_any_output_is_replaced(self):
        """Different outputs with the same basename cannot overwrite one another at publication."""
        self.mode.write_text("duplicate-basename")
        previous = self.directory / "telemetry.tar"
        previous.write_bytes(b"previous publication")
        with self.assertRaises(ValueError): self.collect()
        self.assertEqual(previous.read_bytes(), b"previous publication")
        self.assertEqual(self.receipt.get("artifacts", {}), {})

    def test_reserved_future_evidence_names_fail_before_any_export(self):
        """Artifacts cannot impersonate receipts, reports or dependency evidence written later."""
        for name in ("report.json", "receipt.json", "MODULE.bazel.lock", "module-graph.json"):
            with self.subTest(name=name):
                self.mode.write_text("collision:" + name)
                self.receipt = {"commands": []}
                with self.assertRaisesRegex(ValueError, "collides with build evidence"):
                    self.collect()
                self.assertFalse((self.directory / name).exists())
                self.assertFalse((self.directory / "telemetry.tar").exists())
                self.assertEqual(self.receipt.get("artifacts", {}), {})

    def test_existing_evidence_name_cannot_be_overwritten_by_an_artifact(self):
        """Preserve an earlier log and reject the whole filegroup before exporting any file."""
        self.mode.write_text("collision:checked.log")
        evidence = self.directory / "checked.log"
        evidence.write_text("recorded diagnostic\n")
        original_mtime = evidence.stat().st_mtime_ns
        with self.assertRaisesRegex(ValueError, "collides with build evidence"):
            self.collect()
        self.assertEqual(evidence.read_text(), "recorded diagnostic\n")
        self.assertEqual(evidence.stat().st_mtime_ns, original_mtime)
        self.assertFalse((self.directory / "telemetry.tar").exists())
        self.assertEqual(self.receipt.get("artifacts", {}), {})

    def test_invalid_declared_output_preserves_previous_publication(self):
        """Validate every declared output before publishing files from an incomplete build."""
        for mode in ("missing-file", "empty-file", "query-failure"):
            with self.subTest(mode=mode):
                self.mode.write_text(mode)
                self.receipt = {"commands": []}
                previous = self.directory / "telemetry.tar"
                previous.write_bytes(b"previous publication")
                with self.assertRaises((ValueError, subprocess.CalledProcessError)): self.collect()
                self.assertEqual(previous.read_bytes(), b"previous publication")
                self.assertEqual(self.receipt.get("artifacts", {}), {})

    def test_build_failure_stops_output_discovery(self):
        """A failed artifact build must retain failure evidence and must not query stale outputs."""
        self.mode.write_text("build-failure")
        with self.assertRaises(subprocess.CalledProcessError): self.collect()
        self.assertEqual(len(self.receipt["commands"]), 1)
        self.assertNotEqual(self.receipt["commands"][0]["returncode"], 0)
        self.assertEqual(self.receipt.get("artifacts", {}), {})

    def test_failed_copy_does_not_publish_an_incomplete_artifact(self):
        """An interrupted first publication must leave no artifact or temporary partial copy."""
        with mock.patch('tools.bazel.build_helpers.shutil.copyfile', side_effect=OSError('disk full')):
            with self.assertRaisesRegex(OSError, 'disk full'): self.collect()
        self.assertFalse(list(self.directory.glob("telemetry.tar*")))
        self.assertEqual(self.receipt.get("artifacts", {}), {})


if __name__ == '__main__':
    unittest.main()
