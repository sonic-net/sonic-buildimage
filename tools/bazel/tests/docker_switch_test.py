#!/usr/bin/env python3
"""Exercise independent Make/Bazel container switches and real cache restores."""

import os
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.tests.docker_test import ArchiveRecipeFixture


class DockerSwitchTest(ArchiveRecipeFixture, unittest.TestCase):
    archives = ("telemetry.gz", "telemetry-dbg.gz", "routing.gz", "routing-dbg.gz")
    labels = {name: "//services:" + name for name in archives}

    def setUp(self):
        super().setUp()
        self.prerequisites = {name: [] for name in self.archives}
        self.makefile += """
SONIC_DPKG_CACHE_METHOD = none
BUILD_WITH_BAZEL_WHEN_AVAILABLE ?= n
SONIC_BAZEL_SWITCHABLE_IMAGES = telemetry.gz telemetry-dbg.gz routing.gz routing-dbg.gz telemetry.gz
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),y)
ifeq ($(TELEMETRY_BAZEL_AVAILABLE),y)
SONIC_BAZEL_DOCKER_IMAGES += telemetry.gz
SONIC_BAZEL_DBG_DOCKER_IMAGES += telemetry-dbg.gz
endif
ifeq ($(ROUTING_BAZEL_AVAILABLE),y)
SONIC_BAZEL_DOCKER_IMAGES += routing.gz
SONIC_BAZEL_DBG_DOCKER_IMAGES += routing-dbg.gz
endif
endif
"""
        for name in self.archives:
            self.makefile += f"{name}_BAZEL_TARGET = {self.labels[name]}\n"
            self.makefile += f"{name}_PATH = services/{name.split('.')[0]}\n"
        self.makefile += """include tools/bazel/docker.mk
LEGACY_IMAGES = $(filter-out $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES),$(sort $(SONIC_BAZEL_SWITCHABLE_IMAGES)))
$(addprefix $(TARGET_PATH)/,$(LEGACY_IMAGES)): $(TARGET_PATH)/%.gz:
	@printf 'make %s' '$(@F)' > "$@"
	@printf '%s\\n' '$(@F)' >> legacy-calls
"""

    def build(self, telemetry="n", routing="n", bazel="y"):
        self.calls.unlink(missing_ok=True)
        result = self.run_make(*self.archives, parallel=True, environment={
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE": bazel,
            "TELEMETRY_BAZEL_AVAILABLE": telemetry, "ROUTING_BAZEL_AVAILABLE": routing,
        })
        self.assert_success(result)
        self.assertNotIn("given more than once", result.stderr)
        self.assertNotIn("overriding recipe", result.stderr)
        snapshot = {}
        for name in self.archives:
            active = bazel == "y" and (telemetry if name.startswith("telemetry") else routing) == "y"
            destination = self.target / name
            stamp = self.target / ".container-build-method" / name
            self.assertEqual(destination.read_bytes(), (("new " if active else "make ") + name).encode())
            self.assertEqual(stamp.read_text().strip(), "bazel" if active else "make")
            snapshot[name] = (destination.stat().st_mtime_ns, stamp.stat().st_mtime_ns)
        expected_labels = {self.labels[name] for name in self.archives
                           if bazel == "y" and (telemetry if name.startswith("telemetry") else routing) == "y"}
        self.assertEqual({call["arguments"][-1] for call in self.recorded_calls()}, expected_labels)
        self.assertEqual(len(self.recorded_calls()), 2 * len(expected_labels))
        return snapshot

    def test_independent_runtime_debug_switches_rebuild_only_changed_family(self):
        """Track each owner's available targets without rebuilding unrelated Make images."""
        previous = None
        for telemetry, routing, changed in (
            ("n", "n", set(self.archives)),
            ("n", "n", set()),
            ("y", "n", set(self.archives[:2])),
            ("y", "n", set()),
            ("y", "y", set(self.archives[2:])),
            ("n", "y", set(self.archives[:2])),
            ("n", "y", set()),
            ("n", "n", set(self.archives[2:])),
            ("n", "n", set()),
        ):
            with self.subTest(telemetry=telemetry, routing=routing, changed=changed):
                current = self.build(telemetry, routing)
                if previous:
                    for name in self.archives:
                        if name in changed:
                            self.assertNotEqual(current[name][0], previous[name][0])
                            self.assertNotEqual(current[name][1], previous[name][1])
                        else:
                            self.assertEqual(current[name], previous[name])
                previous = current

    def test_global_flag_uses_bazel_only_for_available_families(self):
        """Use the same global selector for all owners and invalidate only changed builders."""
        previous = None
        for bazel, telemetry, routing, changed in (
            ("n", "y", "n", set(self.archives)),
            ("y", "y", "n", set(self.archives[:2])),
            ("y", "y", "n", set()),
            ("n", "y", "n", set(self.archives[:2])),
            ("n", "y", "y", set()),
            ("y", "y", "y", set(self.archives)),
            ("n", "y", "y", set(self.archives)),
            ("n", "y", "y", set()),
        ):
            with self.subTest(bazel=bazel, telemetry=telemetry, routing=routing):
                current = self.build(telemetry, routing, bazel)
                if previous:
                    for name in self.archives:
                        if name in changed:
                            self.assertNotEqual(current[name][0], previous[name][0])
                            self.assertNotEqual(current[name][1], previous[name][1])
                        else:
                            self.assertEqual(current[name], previous[name])
                previous = current

    def test_first_make_request_invalidates_preexisting_bazel_outputs_and_stale_flags(self):
        """Do not trust stale legacy stamps to prove that preexisting archives came from the newly
        requested Make builder.
        """
        for name in self.archives:
            (self.target / name).write_bytes(("new " + name).encode())
            flags = self.target / (name + ".flags")
            flags.write_text("make\n")
            os.utime(flags, ns=(0, 0))
        (self.target / ".swss-build-method").write_text("y\n")
        previous = {name: (self.target / name).stat().st_mtime_ns for name in self.archives}
        current = self.build()
        for name in self.archives:
            self.assertNotEqual(current[name][0], previous[name])
        self.assertCountEqual((self.root / "legacy-calls").read_text().splitlines(), self.archives)
        self.assertEqual(self.build(), current)

    def test_local_packages_remain_allowed_for_archives_using_make(self):
        """Apply the Bazel local-package restriction only to selected archives, preserving the
        normal Make package path.
        """
        self.makefile = "SONIC_PACKAGES_LOCAL = telemetry.gz telemetry-dbg.gz\n" + self.makefile
        self.build(telemetry="n", routing="y")


class DockerSwitchCacheTest(ArchiveRecipeFixture, unittest.TestCase):
    """Use the production cache macros, with file copies as the Make producer."""

    archives = DockerSwitchTest.archives
    labels = DockerSwitchTest.labels

    def setUp(self):
        super().setUp()
        self.prerequisites = {name: [] for name in self.archives}
        self.cache = self.root / "cache"
        self.cache.mkdir()
        (self.root / "rules").mkdir()
        (self.root / "rules/fixture.dep").write_text("# Fixture dependencies below.\n")
        (self.root / "source.txt").write_text("Make source\n")
        commands = self.root / "commands"
        commands.mkdir()
        sudo = commands / "sudo"
        sudo.write_text('#!/bin/sh\n[ "$1" = chmod ] || exit 99\nexec "$@"\n')
        sudo.chmod(0o755)
        self.environment = {"PATH": str(commands) + os.pathsep + os.environ["PATH"]}
        for command in (
            ["git", "init", "--quiet"],
            ["git", "add", "rules", "source.txt"],
            ["git", "-c", "user.name=Cache Test", "-c", "user.email=cache-test@example.invalid",
             "-c", "commit.gpgsign=false", "commit", "--quiet", "-m", "Container cache fixture"],
        ):
            subprocess.run(command, cwd=self.root, check=True, capture_output=True, text=True)
        self.makefile += """
RULES_PATH = rules
SONIC_DPKG_CACHE_METHOD = rwcache
SONIC_DOCKER_IMAGES = telemetry.gz routing.gz
SONIC_DOCKER_DBG_IMAGES = telemetry-dbg.gz routing-dbg.gz
SONIC_BAZEL_SWITCHABLE_IMAGES = $(SONIC_DOCKER_IMAGES) $(SONIC_DOCKER_DBG_IMAGES)
CONFIGURED_PLATFORM = vs
CONFIGURED_ARCH = amd64
BLDENV = trixie
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE),y)
ifeq ($(TELEMETRY_BAZEL_AVAILABLE),y)
SONIC_BAZEL_DOCKER_IMAGES += telemetry.gz
SONIC_BAZEL_DBG_DOCKER_IMAGES += telemetry-dbg.gz
endif
ifeq ($(ROUTING_BAZEL_AVAILABLE),y)
SONIC_BAZEL_DOCKER_IMAGES += routing.gz
SONIC_BAZEL_DBG_DOCKER_IMAGES += routing-dbg.gz
endif
endif
"""
        for name in self.archives:
            selector = "TELEMETRY_BAZEL_AVAILABLE" if name.startswith("telemetry") else "ROUTING_BAZEL_AVAILABLE"
            self.makefile += f"{name}_BAZEL_TARGET = {self.labels[name]}\n"
            self.makefile += f"{name}_PATH = services/{name.split('.')[0]}\n"
            self.makefile += f"{name}_CACHE_MODE = GIT_CONTENT_SHA\n"
            self.makefile += f"{name}_DEP_FILES = source.txt\n"
            self.makefile += f"{name}_DEP_FLAGS = $(SONIC_COMMON_FLAGS_LIST) $({selector})\n"
        self.makefile += f"include {ROOT / 'Makefile.cache'}\n"
        self.makefile += """include tools/bazel/docker.mk
LEGACY_IMAGES = $(filter-out $(SONIC_BAZEL_DOCKER_IMAGES) $(SONIC_BAZEL_DBG_DOCKER_IMAGES),$(SONIC_BAZEL_SWITCHABLE_IMAGES))
$(addprefix $(TARGET_PATH)/,$(LEGACY_IMAGES)): $(TARGET_PATH)/%.gz: $(TARGET_PATH)/%.gz.dep
	$(call LOAD_CACHE,$*.gz,$@)
	if [ -z '$($*.gz_CACHE_LOADED)' ]; then
	  printf 'make %s' '$(@F)' > "$@"
	  printf '%s\\n' '$(@F)' >> legacy-calls
	  $(call SAVE_CACHE,$*.gz,$@)
	fi
"""

    def build(self, telemetry="n", routing="n", archives=None, bazel="y"):
        return self.run_make(environment={**self.environment,
                                         "BUILD_WITH_BAZEL_WHEN_AVAILABLE": bazel,
                                         "TELEMETRY_BAZEL_AVAILABLE": telemetry, "ROUTING_BAZEL_AVAILABLE": routing},
                             goals=["SONIC_DPKG_CACHE_DIR=" + str(self.cache),
                                    *("target/" + name for name in (archives or self.archives))])

    def assert_contents(self, telemetry="n", routing="n", bazel="y"):
        for name in self.archives:
            active = bazel == "y" and (telemetry if name.startswith("telemetry") else routing) == "y"
            self.assertEqual((self.target / name).read_bytes(),
                             (("new " if active else "make ") + name).encode())

    def add_make_consumer(self, base):
        """Model a legacy Docker image that loads one selected parent archive."""
        cache_include = f"include {ROOT / 'Makefile.cache'}"
        self.makefile = self.makefile.replace(cache_include, f"""
SONIC_DOCKER_IMAGES += consumer.gz
consumer.gz_PATH = services/consumer
consumer.gz_CACHE_MODE = GIT_CONTENT_SHA
consumer.gz_DEP_FILES = source.txt
consumer.gz_DEP_FLAGS = $(SONIC_COMMON_FLAGS_LIST)
consumer.gz_LOAD_DOCKERS = {base}
""" + cache_include)
        self.makefile += f"""
target/consumer.gz: target/{base} target/consumer.gz.dep
\t$(call LOAD_CACHE,consumer.gz,$@)
\tif [ -z '$(consumer.gz_CACHE_LOADED)' ]; then
\t  cp target/{base} "$@"
\t  printf '%s\\n' 'consumer built' >> consumer-calls
\t  $(call SAVE_CACHE,consumer.gz,$@)
\tfi
"""

    def assert_bazel_base_cache_invalidation(self, base):
        self.add_make_consumer(base)
        consumer = self.target / "consumer.gz"
        cache_files = []
        for payload in ("base A", "base B"):
            self.environment["FAKE_BAZEL_PAYLOAD"] = payload
            result = self.build(telemetry="y", archives=["consumer.gz"])
            self.assert_success(result)
            self.assertNotIn("Dependency file(s) are not found", result.stderr)
            self.assertNotIn("fatal:", result.stderr)
            self.assertEqual((self.target / base).read_text(), payload)
            self.assertEqual(consumer.read_text(), payload)
            cache_files.append(set(self.cache.glob("consumer.gz-*.tgz")))
        self.assertEqual(len(cache_files[0]), 1)
        self.assertEqual(len(cache_files[1]), 2)
        self.assertLess(cache_files[0], cache_files[1])
        # With unchanged base bytes, a clean checkout can reuse the correct
        # child cache entry instead of rebuilding or restoring the old child.
        consumer.unlink()
        result = self.build(telemetry="y", archives=["consumer.gz"])
        self.assert_success(result)
        self.assertEqual(consumer.read_text(), "base B")
        self.assertIn("[ CACHE::LOADED ]", (self.target / "consumer.gz.log").read_text())
        self.assertEqual((self.root / "consumer-calls").read_text().splitlines(),
                         ["consumer built", "consumer built"])
        self.assertEqual(set(self.cache.glob("consumer.gz-*.tgz")), cache_files[1])

    def test_bazel_runtime_base_changes_invalidate_make_child_cache(self):
        """A changed Bazel base must not leave a Make child built from old bytes."""
        self.assert_bazel_base_cache_invalidation("telemetry.gz")

    def test_bazel_debug_base_ignores_stale_make_sidecars(self):
        """Old Make metadata cannot identify the current Bazel debug archive."""
        for suffix in (".flags", ".dep.sha"):
            (self.target / ("telemetry-dbg.gz" + suffix)).write_text("old Make state\n")
        self.assert_bazel_base_cache_invalidation("telemetry-dbg.gz")

    def test_warm_make_cache_replaces_bazel_archives_when_returning_to_make(self):
        """Restore Make bytes from a warm cache after a builder switch without rerunning producers
        or disturbing other images.
        """
        self.assert_success(self.build())
        self.assert_contents()
        self.assertEqual(len(list(self.cache.glob("*.tgz"))), 4)
        self.assert_success(self.build(telemetry="y", routing="y"))
        self.assert_contents(telemetry="y", routing="y")
        routing_before = {name: (self.target / name).stat().st_mtime_ns for name in self.archives[2:]}
        self.assert_success(self.build(routing="y"))
        self.assert_contents(routing="y")
        for name in self.archives[:2]:
            self.assertIn("[ CACHE::LOADED ]", (self.target / (name + ".log")).read_text())
            self.assertTrue((self.target / (name + ".cached.log")).is_file())
        for name in self.archives[2:]:
            self.assertEqual((self.target / name).stat().st_mtime_ns, routing_before[name])
        self.assert_success(self.build())
        self.assert_contents()
        # Both families came from cache: their native producers ran only once.
        self.assertCountEqual((self.root / "legacy-calls").read_text().splitlines(), self.archives)
        before = {name: (self.target / name).stat().st_mtime_ns for name in self.archives}
        self.assert_success(self.build())
        self.assertEqual({name: (self.target / name).stat().st_mtime_ns for name in self.archives}, before)

    def test_corrupt_cache_preserves_bazel_archive_and_retry_restores_make(self):
        """Keep cache restoration atomic even on late archive corruption, with cleanup and a
        retryable builder transition.
        """
        self.assert_success(self.build())
        self.assert_success(self.build(telemetry="y"))
        # CACHE_USER may be a privilege wrapper; extraction, publication and
        # cleanup must all use it so privileged temporary files cannot leak.
        cache_user = self.root / "commands/cache-user"
        cache_user.write_text('#!/bin/sh\nprintf "%s\\n" "$1" >> "$CACHE_USER_CALLS"\nexec "$@"\n')
        cache_user.chmod(0o755)
        cache_user_calls = self.root / "cache-user-calls"
        self.environment["CACHE_USER_CALLS"] = str(cache_user_calls)
        self.makefile += f"telemetry.gz_CACHE_USER = {cache_user}\n"
        archive = self.target / "telemetry.gz"
        before = (archive.read_bytes(), archive.stat().st_mtime_ns)
        cache = next(self.cache.glob("telemetry.gz-*.tgz"))
        cached_bytes = cache.read_bytes()
        # Tar can extract members before gzip detects a corrupt footer. The
        # staged restore must not publish any of those bytes on that failure.
        cache.write_bytes(cached_bytes[:-8])
        failed = self.build(archives=["telemetry.gz"])
        self.assertNotEqual(failed.returncode, 0, failed.stdout + failed.stderr)
        self.assertEqual((archive.read_bytes(), archive.stat().st_mtime_ns), before)
        stamp = self.target / ".container-build-method/telemetry.gz"
        self.assertGreater(stamp.stat().st_mtime_ns, archive.stat().st_mtime_ns)
        self.assertFalse(list(self.target.glob(".cache-restore.*")))
        self.assertEqual(cache_user_calls.read_text().splitlines(), ["tar", "rm"])
        cache_user_calls.unlink()
        cache.write_bytes(cached_bytes)
        self.assert_success(self.build(archives=["telemetry.gz"]))
        self.assertEqual(archive.read_bytes(), b"make telemetry.gz")
        self.assertCountEqual((self.root / "legacy-calls").read_text().splitlines(), self.archives)
        self.assertFalse(list(self.target.glob(".cache-restore.*")))
        self.assertEqual(cache_user_calls.read_text().splitlines(), ["tar", "mv", "mv", "rm"])


if __name__ == "__main__":
    unittest.main()
