"""Keep installed-image resource limits bounded without weakening isolation."""
import sys
import unittest
import tempfile
from unittest import mock
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.validate_runtime import container_arguments
from tools.bazel.oci import runtime_smoke


class InstalledImageResourcesTest(unittest.TestCase):
    def test_native_debug_budget_preserves_runtime_budget_and_isolation(self):
        for variant, memory in [('runtime', '4g'), ('debug', '8g')]:
            with self.subTest(variant=variant):
                args = container_arguments('image', 'container', Path(__file__), [], variant=variant)
                self.assertEqual([arg for arg in args if arg.startswith('--memory=')],
                                 ['--memory=' + memory])
                for isolation in ['--network=none', '--cap-drop=ALL',
                                  '--security-opt=no-new-privileges', '--read-only',
                                  '--pids-limit=128', '--cpus=2']:
                    self.assertIn(isolation, args)

    def test_gdb_budget_still_fails_when_its_deadline_expires(self):
        expired = {'returncode': None, 'stdout': '', 'stderr': 'timeout'}
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(
                runtime_smoke, 'command', return_value=expired) as command:
            with self.assertRaisesRegex(ValueError, 'GDB did not return one successful lookup'):
                runtime_smoke.gdb_lookup(Path('/usr/bin/example'), Path(directory), '/usr/lib/debug')
        self.assertEqual(command.call_args.kwargs['timeout'], 300)

    def test_unknown_variant_cannot_select_a_resource_budget(self):
        with self.assertRaisesRegex(ValueError, 'Unknown installed-image variant'):
            container_arguments('image', 'container', Path(__file__), [], variant='unknown')


if __name__ == '__main__':
    unittest.main()
