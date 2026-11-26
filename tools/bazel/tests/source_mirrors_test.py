"""Shared mirror setup preserves source integrity and existing download policy."""

import hashlib
from http.client import IncompleteRead
import io
from pathlib import Path
import re
import shlex
import sys
import tempfile
import unittest
from unittest.mock import patch
from urllib.error import HTTPError, URLError

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import source_mirrors


class SourceMirrorsTest(unittest.TestCase):
    def test_transient_http_errors_retry_then_verify_policy(self):
        shared = (Path(__file__).parent / 'fixtures/source-mirrors-f9a9baae.cfg').read_bytes()
        for status in (408, 429, 500, 502, 503, 504):
            with self.subTest(status=status), \
                    patch.object(source_mirrors, 'urlopen', side_effect=[
                        HTTPError(source_mirrors.URL, status, 'temporary', None, None),
                        io.BytesIO(shared),
                    ]) as fetch, \
                    patch.object(source_mirrors.time, 'sleep') as sleep, \
                    patch.object(source_mirrors.random, 'uniform', return_value=0.5), \
                    patch.object(sys, 'stderr', new_callable=io.StringIO) as diagnostics:
                self.assertEqual(source_mirrors.download(), shared)
                self.assertEqual(fetch.call_count, 2)
                sleep.assert_called_once_with(1.5)
                self.assertIn(f'HTTP {status}', diagnostics.getvalue())
                self.assertIn('attempt 2/4', diagnostics.getvalue())

    def test_transient_connection_and_response_read_errors_retry(self):
        shared = (Path(__file__).parent / 'fixtures/source-mirrors-f9a9baae.cfg').read_bytes()
        for error in (URLError('temporary DNS failure'), TimeoutError(),
                      ConnectionResetError(), IncompleteRead(b'partial')):
            with self.subTest(error=type(error).__name__), \
                    patch.object(source_mirrors, 'urlopen') as fetch, \
                    patch.object(source_mirrors.time, 'sleep') as sleep:
                response = unittest.mock.MagicMock()
                response.__enter__.return_value.read.side_effect = error
                fetch.side_effect = [response, io.BytesIO(shared)]
                self.assertEqual(source_mirrors.download(), shared)
                self.assertEqual(fetch.call_count, 2)
                sleep.assert_called_once()
                response.__exit__.assert_called_once()

    def test_exhausted_retries_preserve_existing_configuration(self):
        error = HTTPError(source_mirrors.URL, 503, 'Backend.max_conn reached', None, None)
        with tempfile.TemporaryDirectory() as temporary, \
                patch.object(source_mirrors, 'urlopen', side_effect=error) as fetch, \
                patch.object(source_mirrors.time, 'sleep') as sleep, \
                patch.object(source_mirrors.random, 'uniform', return_value=0.5):
            output = Path(temporary) / 'mirrors.cfg'
            output.write_bytes(b'previous verified policy\n')
            with self.assertRaises(HTTPError) as raised:
                source_mirrors.prepare(output)
            self.assertIs(raised.exception, error)
            self.assertEqual(fetch.call_count, 4)
            self.assertEqual(sleep.call_args_list,
                             [unittest.mock.call(1.5), unittest.mock.call(2.5),
                              unittest.mock.call(4.5)])
            self.assertEqual(output.read_bytes(), b'previous verified policy\n')

    def test_permanent_http_errors_are_not_retried(self):
        for status in (400, 401, 403, 404):
            error = HTTPError(source_mirrors.URL, status, 'permanent', None, None)
            with self.subTest(status=status), \
                    patch.object(source_mirrors, 'urlopen', side_effect=error) as fetch, \
                    patch.object(source_mirrors.time, 'sleep') as sleep:
                with self.assertRaises(HTTPError):
                    source_mirrors.download()
                fetch.assert_called_once()
                sleep.assert_not_called()

    def test_dhcp_workflow_prefers_locked_snapshots_and_preserves_all_fallbacks(self):
        """Exercise the real workflow's CLI arguments through the shared helper."""
        root = Path(__file__).resolve().parents[3]
        workflow = yaml.safe_load((root / '.github/workflows/bazel-oci.yml').read_text())
        job = workflow['jobs']['dhcp_relay_source_image']
        step = next(step for step in job['steps']
                    if step.get('name') == 'Configure verified source download mirrors')
        shared = (Path(__file__).parent / 'fixtures/source-mirrors-f9a9baae.cfg').read_bytes()
        with tempfile.TemporaryDirectory() as temporary:
            command = shlex.split(step['run'].splitlines()[0].replace('$RUNNER_TEMP', temporary))
            self.assertEqual(command[:3], ['python3', '-B', 'tools/bazel/ci/source_mirrors.py'])
            with patch.object(sys, 'argv', command[2:]), \
                    patch.object(source_mirrors, 'urlopen', return_value=io.BytesIO(shared)):
                source_mirrors.main()
            configuration = (Path(temporary) / 'dhcp-source-mirrors.cfg').read_bytes()
            local = (root / 'tools/bazel/debian_mirrors.cfg').read_bytes()
            self.assertEqual(configuration, source_mirrors.SNAPSHOT_FIRST + local + b'\n' + shared)

    def test_changed_shared_policy_does_not_replace_existing_configuration(self):
        """An unexpected policy cannot redirect downloads, even with an old output."""
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'mirrors.cfg'
            output.write_bytes(b'previous verified policy\n')
            with patch.object(source_mirrors, 'urlopen', return_value=io.BytesIO(b'changed policy')) as fetch, \
                    patch.object(source_mirrors.time, 'sleep') as sleep, \
                    self.assertRaisesRegex(ValueError, 'checksum mismatch'):
                source_mirrors.prepare(output)
            fetch.assert_called_once()
            sleep.assert_not_called()
            self.assertEqual(output.read_bytes(), b'previous verified policy\n')

    def test_verified_shared_policy_keeps_local_debian_priority(self):
        """Hosted jobs retain local mirrors and fallbacks ahead of shared choices."""
        shared = b'# verified registry policy\nrewrite example/source https://mirror.example/source\n'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            local = root / 'debian.cfg'
            local.write_bytes(b'rewrite snapshot.example/(.*) https://sonic.example/$1\n'
                              b'rewrite snapshot.example/(.*) https://snapshot.example/$1\n')
            output = root / 'generated' / 'mirrors.cfg'
            with patch.object(source_mirrors, 'SHA256', hashlib.sha256(shared).hexdigest()), \
                    patch.object(source_mirrors, 'urlopen', return_value=io.BytesIO(shared)) as fetch:
                source_mirrors.prepare(output, local)
            fetch.assert_called_once_with(source_mirrors.URL, timeout=60)
            self.assertEqual(output.read_bytes(), local.read_bytes() + b'\n' + shared)

    def test_native_policy_adds_only_the_original_timestamped_snapshot_first(self):
        shared = (Path(__file__).parent / 'fixtures/source-mirrors-f9a9baae.cfg').read_bytes()
        with tempfile.TemporaryDirectory() as temporary, \
                patch.object(source_mirrors, 'urlopen', side_effect=lambda *_a, **_k: io.BytesIO(shared)):
            output = Path(temporary) / 'mirrors.cfg'
            source_mirrors.prepare(output)
            default = output.read_bytes()
            source_mirrors.prepare(output, prefer_debian_snapshot=True)
            rule, fallbacks = output.read_bytes().split(b'\n', 1)
            self.assertEqual(fallbacks, default)
            operation, pattern, replacement = rule.decode().split()
            self.assertEqual(operation, 'rewrite')
            for archive in ('debian', 'debian-security'):
                url = f'snapshot.debian.org/archive/{archive}/20260726T121236Z/pool/locked.deb'
                match = re.fullmatch(pattern, url)
                self.assertIsNotNone(match)
                self.assertEqual(replacement.replace('$1', match[1]), 'https://' + url)
            self.assertIsNone(re.fullmatch(pattern, 'snapshot.debian.org/file/abc'))


if __name__ == '__main__':
    unittest.main()
