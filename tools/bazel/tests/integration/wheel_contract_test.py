"""Reject wheel metadata, payload and command drift from an owner's contract."""

import importlib.metadata
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock
import zipfile

from tools.bazel.tests.integration.wheel_contract import (
    assert_wheel_contract,
    expected_sources,
    normalized_requirement,
    setup_contract,
)


class WheelContractTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.setup = self.root / "source/setup.py"
        self.setup.parent.mkdir()
        self.contract = {
            "name": "fixture-component", "version": "1.0",
            "packages": ["fixture_component"], "py_modules": ["fixture_module"],
            "install_requires": ["dependency[feature]>=1"],
            "extras_require": {"test": ['optional; python_version >= "3.11"']},
            "data_files": [("share/fixture", ["relative.txt"]), ("/etc/fixture", ["absolute.txt"])],
            "scripts": ["scripts/fixture-script"],
            "entry_points": {"console_scripts": ["fixture-command = fixture_module:main"]},
            "license_files": ["LICENSE"],
        }
        self.source_files = {
            "fixture_component/__init__.py": b"ORIGIN = 'wheel'\n",
            "fixture_module.py": b"def main():\n    return 0\n",
            "relative.txt": b"relative payload\n", "absolute.txt": b"absolute payload\n",
            "scripts/fixture-script": b"#!/usr/bin/env python3\nprint('command')\n",
            "LICENSE": b"fixture license\n",
        }
        for name, content in self.source_files.items():
            path = self.setup.parent / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
        self.setup.write_text("from setuptools import setup\nsetup(**" + repr(self.contract) + ")\n")
        self.installed = SimpleNamespace(
            site=self.root / "environment/site", environment=self.root / "environment",
            scripts=self.root / "environment/bin")
        self.metadata_file = "fixture_component-1.0.dist-info/METADATA"
        # These are fixed archive fixtures for checking the contract, not a
        # package build: installation itself is covered by the fixture tests.
        self.records = {
            "fixture_component/__init__.py": self.source_files["fixture_component/__init__.py"],
            "fixture_module.py": self.source_files["fixture_module.py"],
            self.metadata_file: (
                b"Metadata-Version: 2.1\nName: fixture_component\nVersion: 1.0\n"
                b"Requires-Dist: dependency[feature]>=1\n"
                b'Requires-Dist: optional; python_version >= "3.11" and extra == "test"\n'),
            "fixture_component-1.0.data/data/share/fixture/relative.txt": self.source_files["relative.txt"],
            "etc/fixture/absolute.txt": self.source_files["absolute.txt"],
            "fixture_component-1.0.data/scripts/fixture-script": b"#!python\nprint('command')\n",
            "fixture_component-1.0.dist-info/entry_points.txt": b"[console_scripts]\nfixture-command = fixture_module:main\n",
            "fixture_component-1.0.dist-info/licenses/LICENSE": self.source_files["LICENSE"],
        }
        installed_files = {
            self.installed.site / "fixture_component/__init__.py": self.source_files["fixture_component/__init__.py"],
            self.installed.site / "fixture_module.py": self.source_files["fixture_module.py"],
            self.installed.environment / "share/fixture/relative.txt": self.source_files["relative.txt"],
            self.installed.site / "etc/fixture/absolute.txt": self.source_files["absolute.txt"],
            self.installed.scripts / "fixture-script": b"#!/private/venv/bin/python\nprint('command')\n",
            self.installed.scripts / "fixture-command": b"#!/private/venv/bin/python\n",
        }
        for path, content in installed_files.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
        for path in self.installed.scripts.iterdir():
            path.chmod(0o755)

    def check_contract(self):
        wheel = self.root / "fixture_component-1.0-py3-none-any.whl"
        with zipfile.ZipFile(wheel, "w") as archive:
            for name, content in self.records.items():
                archive.writestr(name, content)
        assert_wheel_contract(wheel, self.setup, self.contract, self.installed)

    def test_matching_contract_accepts_installer_shebang_changes(self):
        self.check_contract()

    def test_incorrect_name_version_dependencies_and_extras_fail(self):
        original = self.records[self.metadata_file]
        for before, after in (
            (b"Name: fixture_component", b"Name: other"),
            (b"Version: 1.0", b"Version: 2.0"),
            (b"dependency[feature]>=1", b"dependency>=1"),
            (b"dependency[feature]>=1", b"dependency[feature]>=2"),
            (b'extra == "test"', b'extra == "different"'),
            (b'python_version >= "3.11"', b'python_version >= "3.12"'),
        ):
            with self.subTest(change=after), self.assertRaises(AssertionError):
                self.records[self.metadata_file] = original.replace(before, after)
                self.check_contract()
        self.records[self.metadata_file] = original + b"Requires-Dist: unexpected\n"
        with self.assertRaisesRegex(AssertionError, "Requires-Dist"):
            self.check_contract()
        self.records[self.metadata_file] = original.replace(b"Requires-Dist: dependency[feature]>=1\n", b"")
        with self.assertRaisesRegex(AssertionError, "Requires-Dist"):
            self.check_contract()

    def test_python_payload_must_be_complete_and_exact(self):
        original = self.records.copy()
        for change in ("missing", "extra", "modified"):
            self.records = original.copy()
            if change == "missing":
                del self.records["fixture_module.py"]
            elif change == "extra":
                self.records["unexpected.py"] = b"unexpected = True\n"
            else:
                self.records["fixture_module.py"] = b"modified = True\n"
            with self.subTest(change=change), self.assertRaises(AssertionError):
                self.check_contract()

    def test_installed_source_bytes_must_match(self):
        (self.installed.site / "fixture_module.py").write_bytes(b"modified = True\n")
        with self.assertRaisesRegex(AssertionError, "Installed source differs"):
            self.check_contract()

    def test_both_data_schemes_compare_archive_bytes(self):
        for member in ("fixture_component-1.0.data/data/share/fixture/relative.txt", "etc/fixture/absolute.txt"):
            original = self.records[member]
            with self.subTest(member=member), self.assertRaisesRegex(AssertionError, "Wheel data differs"):
                self.records[member] = b"wrong data\n"
                self.check_contract()
            self.records[member] = original

    def test_both_data_schemes_compare_installed_bytes(self):
        for path in (self.installed.environment / "share/fixture/relative.txt", self.installed.site / "etc/fixture/absolute.txt"):
            original = path.read_bytes()
            with self.subTest(path=path), self.assertRaisesRegex(AssertionError, "Installed data differs"):
                path.write_bytes(b"wrong data\n")
                self.check_contract()
            path.write_bytes(original)

    def test_archive_script_body_preserves_line_endings(self):
        self.records["fixture_component-1.0.data/scripts/fixture-script"] = b"#!python\nprint('command')\r\n"
        with self.assertRaisesRegex(AssertionError, "Wheel script differs"):
            self.check_contract()

    def test_installed_script_body_preserves_line_endings(self):
        (self.installed.scripts / "fixture-script").write_bytes(b"#!python\nprint('command')\r\n")
        with self.assertRaisesRegex(AssertionError, "Installed script differs"):
            self.check_contract()

    def test_installed_commands_must_be_executable(self):
        for name in ("fixture-script", "fixture-command"):
            path = self.installed.scripts / name
            with self.subTest(name=name), self.assertRaisesRegex(AssertionError, "not executable"):
                path.chmod(0o644)
                self.check_contract()
            path.chmod(0o755)

    def test_console_entrypoint_target_must_match(self):
        self.records["fixture_component-1.0.dist-info/entry_points.txt"] = b"[console_scripts]\nfixture-command = other:main\n"
        with self.assertRaisesRegex(AssertionError, "Console scripts"):
            self.check_contract()

    def test_declared_license_must_match(self):
        member = "fixture_component-1.0.dist-info/licenses/LICENSE"
        self.records[member] = b"wrong license\n"
        with self.assertRaisesRegex(AssertionError, "Wheel license differs"):
            self.check_contract()
        del self.records[member]
        with self.assertRaisesRegex(AssertionError, "License files"):
            self.check_contract()

    def test_setup_contract_uses_owner_directory_and_restores_process_state(self):
        original_cwd, original_path = Path.cwd(), sys.path[:]
        self.setup.write_text("""
from pathlib import Path
import pkg_resources
from setuptools import setup
assert Path('LICENSE').read_text() == 'fixture license\\n'
setup(name='fixture-component', version=pkg_resources.get_distribution('sonic-py-common').version)
""")
        with mock.patch("importlib.metadata.version", side_effect=importlib.metadata.PackageNotFoundError):
            self.assertEqual(setup_contract(self.setup), {"name": "fixture-component", "version": "1.0"})
        self.assertEqual(Path.cwd(), original_cwd)
        self.assertEqual(sys.path, original_path)

    def test_setup_failure_restores_process_state(self):
        original_cwd, original_path = Path.cwd(), sys.path[:]
        self.setup.write_text("import pkg_resources\npkg_resources.get_distribution('undeclared-dependency')\n")
        with mock.patch("importlib.metadata.version", side_effect=importlib.metadata.PackageNotFoundError), \
                self.assertRaises(importlib.metadata.PackageNotFoundError):
            setup_contract(self.setup)
        self.assertEqual(Path.cwd(), original_cwd)
        self.assertEqual(sys.path, original_path)

    def test_empty_package_or_uncaptured_setup_is_an_error(self):
        with self.assertRaisesRegex(AssertionError, "no declared Python files"):
            expected_sources({"packages": ["missing_package"]}, self.setup.parent)
        self.setup.write_text("pass\n")
        with self.assertRaisesRegex(AssertionError, "No setup metadata"):
            setup_contract(self.setup)

    def test_requirement_normalization_keeps_semantic_constraints(self):
        self.assertEqual(normalized_requirement('Some_Dep[Some_Extra]>=1; python_version >= "3"'),
                         normalized_requirement("some-dep[some-extra]>=1; python_version>='3'"))
        for first, second in (("dependency[a]", "dependency[b]"),
                              ("dependency @ https://example.org/one.whl", "dependency @ https://example.org/two.whl")):
            with self.subTest(first=first):
                self.assertNotEqual(normalized_requirement(first), normalized_requirement(second))


if __name__ == "__main__":
    unittest.main()
