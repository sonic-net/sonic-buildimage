"""Exercise Make's real manifest generator before the Bazel container bridge."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class MakeManifestsTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        for name in ("tools", "rules", "scripts"):
            (self.root / name).symlink_to(ROOT / name, target_is_directory=True)
        templates = self.root / "files/build_templates"
        (templates / "per_namespace").mkdir(parents=True)
        shutil.copyfile(ROOT / "files/build_templates/manifest.json.j2", templates / "manifest.json.j2")
        (templates / "telemetry.service.j2").touch()
        (templates / "per_namespace/routing.service.j2").touch()
        for owner in ("telemetry", "routing"):
            (self.root / "containers" / owner).mkdir(parents=True)
        self.metadata = self.root / "metadata.mk"
        self.metadata.write_text("""
VERSION ?= 2.3.0
FEATURE ?= y
SONIC_BAZEL_MANIFESTS = telemetry telemetry-dbg routing routing-dbg
telemetry_MANIFEST_IMAGE = docker-telemetry.gz
telemetry-dbg_MANIFEST_IMAGE = docker-telemetry.gz
telemetry-dbg_MANIFEST_SUFFIX = dbg
routing_MANIFEST_IMAGE = docker-routing.gz
routing-dbg_MANIFEST_IMAGE = docker-routing.gz
routing-dbg_MANIFEST_SUFFIX = dbg
docker-telemetry.gz_PATH = containers/telemetry
docker-telemetry.gz_VERSION = $(VERSION)
docker-telemetry.gz_PACKAGE_NAME = telemetry
docker-telemetry.gz_CONTAINER_NAME = telemetry
ifeq ($(FEATURE),y)
docker-telemetry.gz_SERVICE_AFTER += database
endif
docker-telemetry.gz_SERVICE_AFTER += routing
docker-telemetry.gz_WARM_SHUTDOWN_BEFORE = syncd
docker-telemetry.gz_FAST_SHUTDOWN_AFTER = database
docker-routing.gz_PATH = containers/routing
docker-routing.gz_VERSION = 4.1.0+lab
docker-routing.gz_PACKAGE_NAME = routing
docker-routing.gz_CONTAINER_NAME = routing
docker-routing.gz_SERVICE_BEFORE = telemetry
""")
        self.fragment = self.root / "containers/telemetry/manifest.part.json.j2"
        self.fragment.write_text('{"package":{"name":"custom-{{ package_name }}"},"extra":{"enabled":true}}')

    def run_make(self, *arguments, makefile="tools/bazel/prepare_manifests.mk"):
        env = dict(os.environ)
        for name in ("MAKEFLAGS", "MFLAGS"):
            env.pop(name, None)
        return subprocess.run(
            ["make", "-s", "--no-print-directory", "-j4", "-f", makefile,
             "MANIFEST_METADATA=metadata.mk", *arguments],
            cwd=self.root, env=env, capture_output=True, text=True,
        )

    def assert_success(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def path(self, owner):
        return self.root / "target/bazel-manifests" / owner / "manifest.json"

    def read(self, owner):
        return json.loads(self.path(owner).read_text())

    def test_parallel_owners_suffixes_computed_values_and_fragment_merge(self):
        """Keep each container and debug suffix separate while evaluating real Make values and overrides."""
        self.assert_success(self.run_make("VERSION=3.2.1"))
        for owner, version, host, asic in (
            ("telemetry", "3.2.1", True, False),
            ("routing", "4.1.0+lab", False, True),
        ):
            for suffix in ("", "-dbg"):
                with self.subTest(owner=owner, suffix=suffix):
                    data = self.read(owner + suffix)
                    expected = version + (("." if "+" in version else "+") + "dbg" if suffix else "")
                    self.assertEqual(data["package"]["version"], expected)
                    self.assertEqual(data["service"]["name"], owner)
                    self.assertEqual(data["service"]["host-service"], host)
                    self.assertEqual(data["service"]["asic-service"], asic)
        data = self.read("telemetry")
        self.assertEqual(data["service"]["after"], ["database", "routing"])
        self.assertEqual(data["service"]["warm-shutdown"]["before"], ["syncd"])
        self.assertEqual(data["service"]["fast-shutdown"]["after"], ["database"])
        self.assertEqual(data["package"]["name"], "custom-telemetry")
        self.assertEqual(data["extra"], {"enabled": True})
        self.assertFalse((self.root / "containers/telemetry/manifest.json").exists())

    def test_unchanged_bytes_preserve_timestamps(self):
        """Keep published input timestamps stable when metadata generates the same JSON."""
        self.assert_success(self.run_make())
        snapshot = {name: (self.path(name).read_bytes(), self.path(name).stat().st_mtime_ns)
                    for name in ("telemetry", "telemetry-dbg", "routing", "routing-dbg")}
        self.assert_success(self.run_make())
        for name, before in snapshot.items():
            self.assertEqual((self.path(name).read_bytes(), self.path(name).stat().st_mtime_ns), before)

    def test_timed_vs_manifests_pair_by_owner_across_parallel_recipes(self):
        profile = ["SONIC_VS_IMAGE_TIMINGS=1", "BLDENV=trixie", "CONFIGURED_PLATFORM=vs"]
        result = self.run_make(*profile)
        self.assert_success(result)
        markers = [line for line in result.stdout.splitlines() if line.startswith("[ vs timing ]")]
        owners = ("telemetry", "telemetry-dbg", "routing", "routing-dbg")
        self.assertEqual(len(markers), 2 * len(owners))
        for owner in owners:
            prefix = f"[ vs timing ] [ container-manifest ] [ {owner} ]"
            self.assertEqual([line for line in markers if line.startswith(prefix)],
                             [prefix + " [ start ]", prefix + " [ end ]"])

        target = "target/bazel-manifests/telemetry/manifest.json"
        for override in ("SONIC_VS_IMAGE_TIMINGS=0", "BLDENV=bookworm", "CONFIGURED_PLATFORM=broadcom"):
            with self.subTest(override=override):
                result = self.run_make(*profile, override, target)
                self.assert_success(result)
                self.assertNotIn("[ vs timing ]", result.stdout)

        self.fragment.write_text('{"package":')
        failed = self.run_make(*profile, target)
        self.assertNotEqual(failed.returncode, 0)
        self.assertIn("[ vs timing ] [ container-manifest ] [ telemetry ] [ start ]", failed.stdout)
        self.assertNotIn("[ end ]", failed.stdout)

    def test_condition_and_service_template_changes_refresh_inputs(self):
        """Refresh JSON after Make options, service files or optional overrides change."""
        self.assert_success(self.run_make())
        (self.root / "files/build_templates/telemetry.service.j2").unlink()
        self.assert_success(self.run_make("FEATURE=n", "VERSION=5.0.0"))
        self.assertEqual(self.read("telemetry")["service"]["after"], ["routing"])
        self.assertFalse(self.read("telemetry")["service"]["host-service"])
        self.assertEqual(self.read("telemetry")["package"]["version"], "5.0.0")
        self.fragment.unlink()
        self.assert_success(self.run_make())
        self.assertEqual(self.read("telemetry")["package"]["name"], "telemetry")
        self.assertNotIn("extra", self.read("telemetry"))

    def test_failed_generation_preserves_published_input_and_cleans_staging(self):
        """Leave the previous valid manifest intact when rendering or JSON merging fails."""
        self.assert_success(self.run_make())
        before = (self.path("telemetry").read_bytes(), self.path("telemetry").stat().st_mtime_ns)
        for invalid in ('{"package":', '{% invalid %}', '[]'):
            with self.subTest(fragment=invalid):
                self.fragment.write_text(invalid)
                result = self.run_make("target/bazel-manifests/telemetry/manifest.json")
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual((self.path("telemetry").read_bytes(), self.path("telemetry").stat().st_mtime_ns), before)
                self.assertEqual(list(self.path("telemetry").parent.glob(".manifest.*")), [])

    def test_optional_output_directory_matches_legacy_default(self):
        """Produce the same bytes through the legacy default and the new isolated output path."""
        self.assert_success(self.run_make())
        legacy = self.root / "legacy.mk"
        legacy.write_text("""SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
.ONESHELL:
include rules/functions
include metadata.mk
legacy:
	$(call generate_manifest,docker-telemetry)
""")
        self.assert_success(self.run_make("legacy", makefile="legacy.mk"))
        self.assertEqual((self.root / "containers/telemetry/manifest.json").read_bytes(), self.path("telemetry").read_bytes())


if __name__ == "__main__":
    unittest.main()
