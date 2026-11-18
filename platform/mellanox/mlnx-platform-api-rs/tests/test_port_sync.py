"""The port guard must detect behavior changes without flagging comments."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[1] / 'check_python_port.py'
SPEC = importlib.util.spec_from_file_location('port_sync', SOURCE)
PORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PORT)


class PortSyncTests(unittest.TestCase):
    def test_changed_python_body_requires_a_native_review(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'thermal_updater.py'
            source.write_text('def ready():\n    return False\n')
            manifest = {'sources': {'thermal_updater.py': PORT.fingerprint(source)}}
            self.assertEqual(PORT.source_problems(root, manifest), [])
            source.write_text('def ready():\n    return True\n')
            self.assertIn('thermal_updater.py', PORT.source_problems(root, manifest)[0])

    def test_comments_do_not_change_the_python_ast(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'pcie.py'
            source.write_text('def check():\n    return True\n')
            expected = PORT.fingerprint(source)
            source.write_text('# An editorial change.\ndef check():\n    return True\n')
            self.assertEqual(PORT.fingerprint(source), expected)

    def test_conflicting_python_link_versions_fail_before_cargo(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            common, vendor = root / 'common.toml', root / 'vendor.toml'
            common.write_text('[dependencies]\npyo3 = { version = "0.30" }\n')
            vendor.write_text('[dependencies]\npyo3 = { version = "0.25" }\n')
            self.assertTrue(PORT.version_problems(common, vendor))
            vendor.write_text('[dependencies]\npyo3 = { version = "0.30" }\n')
            self.assertEqual(PORT.version_problems(common, vendor), [])


if __name__ == '__main__':
    unittest.main()
