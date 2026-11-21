"""Exercise the public/private boundary for actual P4RT evidence shapes."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import p4rt_public


class PublicEvidenceTest(unittest.TestCase):
    def test_resolved_graph_must_contain_each_accepted_version_without_a_conflicting_version(self):
        """The shared CI graph must preserve the dependency pins of the standalone P4RT build."""
        script = Path(__file__).with_name('p4rt_acceptance.py')
        pins = json.loads(script.with_suffix('.json').read_bytes())['selected_modules']
        self.assertEqual(set(pins), {'sonic-build-infra', 'rules_distroless', 'sonic-swss-common', 'sonic-fips'})
        nodes = [{'name': name, 'version': version} for name, version in pins.items()]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'module-graph.json'

            def check_graph(dependencies):
                path.write_text(json.dumps({'key': '<root>', 'dependencies': dependencies}))
                return subprocess.run([sys.executable, str(script), '--module-graph', str(path)],
                                      capture_output=True, text=True)

            accepted = check_graph(nodes)
            self.assertEqual(accepted.returncode, 0, accepted.stderr)
            for node in nodes:
                for dependencies in ([n for n in nodes if n != node],
                                     [*nodes, {'dependencies': [dict(node, version='different')]}]):
                    with self.subTest(module=node['name'], missing=node not in dependencies):
                        rejected = check_graph(dependencies)
                        self.assertNotEqual(rejected.returncode, 0)
                        self.assertIn(node['name'] + '@' + node['version'], rejected.stderr)

    def test_selects_only_hashed_artifact_and_safe_receipt_fields(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            artifact = root / 'p4rt.source.tar'
            artifact.write_bytes(b'producer-owned archive bytes')
            (root / 'build-receipt.json').write_text(json.dumps({
                'status': 'built', 'revision': 'a' * 40, 'owner_revision': 'b' * 40,
                'commands': [{'argv': ['bazel', '--output_base=/home/builder/.cache/private']}],
                'artifacts': {artifact.name: {'target': '//dockers/docker-sonic-p4rt:source_runtime',
                                            'sha256': check.sha(artifact), 'bytes': artifact.stat().st_size}},
            }))
            (root / 'prepared-owner-runtime.json').write_text('{"library_path":["/home/builder/.cache/private"]}')
            public = p4rt_public.prepare(root)
            self.assertEqual({path.name for path in public.iterdir()},
                             {'p4rt.source.tar', 'public-receipt.json', 'build-summary.json'})
            self.assertEqual(check.sha(public / artifact.name), check.sha(artifact))
            self.assertNotIn('commands', json.loads((public / 'public-receipt.json').read_text()))

    def test_rejects_changed_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'p4rt.source.tar').write_bytes(b'changed bytes')
            (root / 'build-receipt.json').write_text(json.dumps({
                'status': 'built', 'artifacts': {'p4rt.source.tar': {'sha256': '0' * 64}},
            }))
            with self.assertRaisesRegex(ValueError, 'artifact changed'):
                p4rt_public.prepare(root)

    def test_rejects_private_paths_in_public_validation(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'validation.json').write_text('{"path":"/home/builder/.cache/private"}')
            with self.assertRaisesRegex(ValueError, 'local-path'):
                p4rt_public.prepare(root)

    def test_does_not_publish_stale_previous_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'public').mkdir()
            with self.assertRaises(FileExistsError):
                p4rt_public.prepare(root)


if __name__ == '__main__':
    unittest.main()
