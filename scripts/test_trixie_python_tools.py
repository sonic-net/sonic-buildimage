"""Regression coverage for the Debian setuptools vendor refresh."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[1]


class PythonToolsBackportTest(unittest.TestCase):
    def refresh_vendors(self, context_is_package):
        helper = (ROOT / "sonic-slave-trixie/backport-python-tools.sh").read_text(
            encoding="utf-8"
        )
        transform = helper.split("python3 - <<'PY'\n", 1)[1].split("\nPY\n", 1)[0]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            vendor = root / "source/setuptools/_vendor"
            (vendor / "jaraco").mkdir(parents=True)
            context = vendor / "jaraco/context.py"
            if context_is_package:
                context = vendor / "jaraco/context/obsolete.py"
                context.parent.mkdir()
            context.write_text("obsolete context\n", encoding="utf-8")
            (vendor / "jaraco/text.py").write_text("keep\n", encoding="utf-8")
            (vendor / "wheel").mkdir()
            (vendor / "wheel/obsolete.py").write_text("obsolete wheel\n", encoding="utf-8")
            for name in ("jaraco.context-5.3.0", "wheel-0.45.1"):
                metadata = vendor / f"{name}.dist-info/METADATA"
                metadata.parent.mkdir()
                metadata.write_text(f"Name: {name.rsplit('-', 1)[0]}\n", encoding="utf-8")
            wheels = root / "wheels"
            wheels.mkdir()
            for name, version, module in (
                ("jaraco_context", "6.1.2", "jaraco/context"),
                ("wheel", "0.48.0", "wheel"),
            ):
                with zipfile.ZipFile(wheels / f"{name}-{version}-py3-none-any.whl", "w") as archive:
                    archive.writestr(f"{module}/__init__.py", "updated\n")
                    archive.writestr(
                        f"{name}-{version}.dist-info/METADATA",
                        f"Name: {name.replace('_', '.')}\nVersion: {version}\n",
                    )
            result = subprocess.run(
                [sys.executable, "-c", transform], cwd=root,
                text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertFalse((vendor / "jaraco/context.py").exists())
            self.assertFalse((vendor / "jaraco/context/obsolete.py").exists())
            self.assertFalse((vendor / "wheel/obsolete.py").exists())
            self.assertEqual((vendor / "jaraco/context/__init__.py").read_text(), "updated\n")
            self.assertEqual((vendor / "wheel/__init__.py").read_text(), "updated\n")
            self.assertEqual((vendor / "jaraco/text.py").read_text(), "keep\n")
            self.assertEqual(
                {path.name for path in vendor.glob("*.dist-info")},
                {"jaraco_context-6.1.2.dist-info", "wheel-0.48.0.dist-info"},
            )

    def test_replaces_legacy_context_module_and_metadata(self):
        self.refresh_vendors(context_is_package=False)

    def test_replaces_context_package_without_obsolete_files(self):
        self.refresh_vendors(context_is_package=True)


if __name__ == "__main__":
    unittest.main()
