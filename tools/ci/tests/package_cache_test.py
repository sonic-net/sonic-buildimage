#!/usr/bin/env python3
"""Check native Make cache reuse across isolated source checkouts, without DEBs."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]


class PackageCacheTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.cache = self.root / "shared-cache"
        self.cache.mkdir()
        commands = self.root / "commands"
        commands.mkdir()
        # Make's cache changes permissions through sudo. All fixture files are
        # already ours, so exercise chmod without requiring privileged tests.
        sudo = commands / "sudo"
        sudo.write_text('#!/bin/sh\n[ "$1" = chmod ] || exit 99\nexec "$@"\n')
        sudo.chmod(0o755)
        self.environment = {**os.environ, "PATH": str(commands) + os.pathsep + os.environ["PATH"]}

    def workspace(self, name, content=b"first source contents\n"):
        directory = self.root / name
        directory.mkdir()
        (directory / "target").mkdir()
        (directory / "rules").mkdir()
        (directory / "rules" / "fixture.dep").write_text("# One declared fixture below.\n")
        (directory / "source.txt").write_bytes(content)
        # Include the complete production framework: it generates the flags
        # and dependency hashes and executes its real LOAD_CACHE/SAVE_CACHE.
        # The only producer is a file copy; no package builder is involved.
        makefile = """SHELL := /bin/bash
.SHELLFLAGS := -ec
.ONESHELL:
.SECONDEXPANSION:
.DEFAULT_GOAL := target/artifact.bin
RULES_PATH := rules
TARGET_PATH := target
FILES_PATH := target
SONIC_MAKE_FILES := artifact.bin
SONIC_DPKG_CACHE_METHOD := rwcache
SONIC_BUILD_QUIETER := 1
CONFIGURED_PLATFORM := vs
CONFIGURED_ARCH := amd64
BLDENV := trixie
SLAVE_BASE_TAG := test-trixie-v1
artifact.bin_CACHE_MODE := GIT_CONTENT_SHA
artifact.bin_DEP_FILES := source.txt
artifact.bin_DEP_FLAGS = $(SONIC_COMMON_FLAGS_LIST)
include {cache_makefile}

target/artifact.bin: target/artifact.bin.dep
\t$(call LOAD_CACHE,artifact.bin,$@)
\tif [ -z '$(artifact.bin_CACHE_LOADED)' ]; then
\t  cp source.txt $@
\t  echo produced >> producer.log
\t  $(call SAVE_CACHE,artifact.bin,$@)
\tfi
""".format(cache_makefile=ROOT / "Makefile.cache")
        (directory / "Makefile").write_text(makefile)
        for command in (
            ["git", "init", "--quiet"],
            ["git", "add", "Makefile", "rules", "source.txt"],
            ["git", "-c", "user.name=Cache Test", "-c", "user.email=cache-test@example.invalid",
             "-c", "commit.gpgsign=false", "commit", "--quiet", "-m", "Cache fixture"],
        ):
            subprocess.run(command, cwd=directory, env=self.environment,
                           check=True, capture_output=True, text=True)
        return directory

    def build(self, workspace, *settings):
        result = subprocess.run(
            ["make", "--no-print-directory", "SONIC_DPKG_CACHE_DIR=" + str(self.cache), *settings],
            cwd=workspace, env=self.environment, capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual((workspace / "target/artifact.bin").read_bytes(),
                         (workspace / "source.txt").read_bytes())
        return (workspace / "target/artifact.bin.log").read_text()

    def test_fresh_run_reuses_cache_and_changed_inputs_rebuild(self):
        first = self.workspace("run-one")
        self.assertIn("[ CACHE::SAVED ]", self.build(first))
        self.assertEqual((first / "producer.log").read_text(), "produced\n")
        original_entries = set(self.cache.glob("*.tgz"))
        self.assertEqual(len(original_entries), 1)

        second = self.workspace("run-two")
        self.assertIn("[ CACHE::LOADED ]", self.build(second))
        self.assertFalse((second / "producer.log").exists(), "cache hit reran the producer")
        self.assertEqual(set(self.cache.glob("*.tgz")), original_entries)

        changed = self.workspace("changed-source", b"new source contents\n")
        log = self.build(changed)
        self.assertIn("[ CACHE::SAVED ]", log)
        self.assertNotIn("[ CACHE::LOADED ]", log)
        self.assertEqual((changed / "producer.log").read_text(), "produced\n")
        source_entries = set(self.cache.glob("*.tgz"))
        self.assertEqual(len(source_entries - original_entries), 1)

        changed_flags = self.workspace("changed-architecture")
        log = self.build(changed_flags, "CONFIGURED_ARCH=arm64")
        self.assertIn("[ CACHE::SAVED ]", log)
        self.assertNotIn("[ CACHE::LOADED ]", log)
        self.assertEqual((changed_flags / "producer.log").read_text(), "produced\n")
        self.assertEqual(len(set(self.cache.glob("*.tgz")) - source_entries), 1)

        changed_slave = self.workspace("changed-slave")
        log = self.build(changed_slave, "SLAVE_BASE_TAG=test-trixie-v2")
        self.assertIn("[ CACHE::SAVED ]", log)
        self.assertNotIn("[ CACHE::LOADED ]", log)
        self.assertEqual((changed_slave / "producer.log").read_text(), "produced\n")


if __name__ == "__main__":
    unittest.main()
