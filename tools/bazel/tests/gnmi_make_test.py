"""Exercise the actual Make selector and the original dialout configuration."""

import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SERVICE = ROOT / "dockers/docker-sonic-gnmi"


class GnmiMakeTest(unittest.TestCase):
    def selection(self, **changes):
        values = {
            "BLDENV": "trixie", "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "y",
            "CONFIGURED_PLATFORM": "vs", "CONFIGURED_ARCH": "amd64",
            "INCLUDE_FIPS": "y", "DBG_IMAGE_MARK": "dbg",
            "ENABLE_TRANSLIB_WRITE": "y", "ENABLE_NATIVE_WRITE": "y",
            "DOCKERS_PATH": str(ROOT / "dockers"), "TARGET_PATH": "target",
        }
        values.update(changes)
        source = "\n".join(key + " := " + value for key, value in values.items())
        source += "\ninclude rules/docker-gnmi.mk\n"
        source += "\n.PHONY: show\nshow:\n"
        source += "\t@printf '%s\\n' '$(SONIC_BAZEL_DOCKER_IMAGES)' '$(SONIC_BAZEL_DBG_DOCKER_IMAGES)' '$(docker-sonic-gnmi.gz_BAZEL_ARGS)' '$(docker-sonic-gnmi-dbg.gz_BAZEL_ARGS)' '$(docker-sonic-gnmi-dbg.gz_BAZEL_DEPENDS)'\n"
        return subprocess.check_output(["make", "--no-print-directory", "-f", "-", "show"], input=source, text=True, cwd=ROOT).splitlines()

    def test_native_profile_and_dialout(self):
        for flag, expected in [("", "true"), ("y", "true"), ("n", "false")]:
            runtime, debug, args, debug_args, dependencies = self.selection(ENABLE_DIALOUT=flag)
            self.assertEqual(runtime, "docker-sonic-gnmi.gz")
            self.assertEqual(debug, "docker-sonic-gnmi-dbg.gz")
            self.assertEqual(args, "--@sonic_gnmi//tools/bazel:enable_dialout=" + expected)
            self.assertEqual(debug_args, args)
            self.assertEqual(set(dependencies.split()), {
                "target/bazel-manifests/docker-base-trixie/manifest.json",
                "target/bazel-manifests/docker-config-engine-trixie/manifest.json",
                "target/bazel-manifests/docker-sonic-gnmi/manifest.json",
                "target/bazel-manifests/docker-sonic-gnmi/image.env",
                "target/bazel-manifests/docker-sonic-gnmi-dbg/manifest.json",
            })

    def test_unsupported_profiles_retain_make(self):
        cases = [
            {"BLDENV": "bookworm"}, {"BUILD_WITH_BAZEL_WHEN_AVAILABLE": "n"},
            {"CONFIGURED_PLATFORM": "broadcom"}, {"CONFIGURED_ARCH": "arm64"},
            {"INCLUDE_FIPS": "n"}, {"DBG_IMAGE_MARK": "debug"},
            {"ENABLE_TRANSLIB_WRITE": "n"}, {"ENABLE_NATIVE_WRITE": "n"},
            {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"},
            {"ENABLE_ASAN": "y"},
        ]
        for changes in cases:
            with self.subTest(changes=changes):
                self.assertEqual(self.selection(**changes)[:2], ["", ""])
        with tempfile.TemporaryDirectory() as directory:
            custom = Path(directory) / "docker-sonic-gnmi"
            custom.mkdir()
            (custom / "Dockerfile.gnmi.custom.j2").write_text("# custom image extension\n")
            self.assertEqual(self.selection(DOCKERS_PATH=directory)[:2], ["", ""])

    def test_disabled_supervisor_matches_legacy_make(self):
        spec = importlib.util.spec_from_file_location("render_supervisor", SERVICE / "config/render_supervisor.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        original = (SERVICE / "supervisord.conf").read_text()
        expression = r"/^\[program:dialout\]$/,/^\[/{/^\[program:dialout\]$/d; /^\[/!d}"
        self.assertIn(expression, (SERVICE / "Dockerfile.j2").read_text())
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "supervisord.conf"
            path.write_text(original)
            subprocess.run(["sed", "-i", expression, str(path)], check=True)
            subprocess.run(["sed", "-i", "-e", ":a", "-e", r"/^[[:space:]]*$/{$d;N;ba", "-e", "}", str(path)], check=True)
            self.assertEqual(module.without_dialout(original), path.read_text())
        following = "\n[program:unrelated]\ncommand=preserved\n"
        self.assertTrue(module.without_dialout(original + following).endswith(following.lstrip()))
        with self.assertRaises(ValueError):
            module.without_dialout("[program:start]\ncommand=preserved\n")


if __name__ == "__main__":
    unittest.main()
