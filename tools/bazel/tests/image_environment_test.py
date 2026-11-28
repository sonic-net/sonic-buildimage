"""Verify Make's version reaches the declared OCI input without shell evaluation."""

import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("image_environment", ROOT / "tools/bazel/prepare_image_environment.py")
ENVIRONMENT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ENVIRONMENT)


class ImageEnvironmentTest(unittest.TestCase):
    def test_version_change_noop_and_failed_preparation(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "image.env"
            ENVIRONMENT.publish(output, "master.1-test")
            original = output.stat().st_mtime_ns
            ENVIRONMENT.publish(output, "master.1-test")
            self.assertEqual(output.stat().st_mtime_ns, original)
            self.assertEqual(output.read_text(), "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=master.1-test\n")
            ENVIRONMENT.publish(output, "master.2-test")
            self.assertEqual(output.read_text(), "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=master.2-test\n")
            for bad in ("", "master\nEXTRA=value", "master\rvalue", "master\0value", "build-$PATH", "build-${HOME}"):
                with self.subTest(value=repr(bad)), self.assertRaises(ValueError):
                    ENVIRONMENT.publish(output, bad)
                self.assertEqual(output.read_text(), "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=master.2-test\n")
            self.assertEqual(output.stat().st_mode & 0o777, 0o644)
            self.assertEqual(list(output.parent.glob(".version-*")), [])

    def test_no_opt_in_does_not_export_into_legacy_recipes(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "tools").symlink_to(ROOT / "tools", target_is_directory=True)
            makefile = root / "test.mk"
            makefile.write_text("""TARGET_PATH := target
SONIC_IMAGE_VERSION := master.legacy
include tools/bazel/manifests.mk
.PHONY: all
all:
\t@python3 -c 'import os; assert "SONIC_IMAGE_VERSION" not in os.environ'
""")
            env = dict(os.environ)
            for key in ("MAKEFLAGS", "MFLAGS", "SONIC_IMAGE_VERSION"):
                env.pop(key, None)
            result = subprocess.run(["make", "-s", "--no-print-directory", "-f", str(makefile), "all"], cwd=root, env=env, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertFalse((root / "target").exists())

    def test_make_opt_in_is_independent_of_package_manifests(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "tools").symlink_to(ROOT / "tools", target_is_directory=True)
            makefile = root / "test.mk"
            makefile.write_text("""TARGET_PATH := target
SONIC_BAZEL_IMAGE_ENVIRONMENTS := docker-sonic-gnmi docker-example
include tools/bazel/manifests.mk
.PHONY: all
all: target/bazel-manifests/docker-sonic-gnmi/image.env target/bazel-manifests/docker-example/image.env
""")
            def run(version):
                env = dict(os.environ, SONIC_IMAGE_VERSION=version)
                for key in ("MAKEFLAGS", "MFLAGS"):
                    env.pop(key, None)
                return subprocess.run(["make", "-s", "--no-print-directory", "-j2", "-f", str(makefile), "all"], cwd=root, env=env, text=True, capture_output=True)
            # Quotes, backticks and shell punctuation must remain ordinary bytes.
            version = "master.42-test'`touch sentinel`;echo preserved"
            result = run(version)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            outputs = [root / "target/bazel-manifests" / name / "image.env" for name in ("docker-sonic-gnmi", "docker-example")]
            times = [p.stat().st_mtime_ns for p in outputs]
            for p in outputs:
                self.assertEqual(p.read_text(), "DEBIAN_FRONTEND=noninteractive\nIMAGE_VERSION=" + version + "\n")
            self.assertFalse((root / "sentinel").exists())
            self.assertFalse(list(root.glob("target/bazel-manifests/*/manifest.json")))
            self.assertEqual(run(version).returncode, 0)
            self.assertEqual([p.stat().st_mtime_ns for p in outputs], times)
            self.assertEqual(run("master.43-test").returncode, 0)
            before = [p.read_bytes() for p in outputs]
            self.assertNotEqual(run("").returncode, 0)
            self.assertEqual([p.read_bytes() for p in outputs], before)
            self.assertFalse((root / "target/bazel-manifests/docker-other/image.env").exists())


if __name__ == "__main__":
    unittest.main()
