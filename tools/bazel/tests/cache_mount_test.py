#!/usr/bin/env python3
"""Check host cache configuration without starting Docker or a builder."""

import json
import os
from pathlib import Path
import pwd
import subprocess
import sys
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).resolve().parents[3]


class CacheMountTest(unittest.TestCase):
    def workflow_step(self, name):
        """Run the workflow's actual shell, without a second implementation of its policy."""
        workflow = (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        marker = f"      - name: {name}\n"
        self.assertEqual(workflow.count(marker), 1)
        step = workflow.split(marker, 1)[1].split("\n      - name:", 1)[0]
        return textwrap.dedent(step.split("        run: |\n", 1)[1])

    @unittest.skipUnless(os.geteuid() == 0, "requires root to exercise actual root/non-root cache ownership")
    def test_ci_cache_namespace_preserves_legacy_entries_and_reuses_writer_cache(self):
        """Execute CI prep with both users: old root-owned CAS entries stay untouched,
        each UID can publish/replace cache files, and the next distdir step uses its export.
        """
        nobody = pwd.getpwnam("nobody")
        workflow = (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        self.assertLess(workflow.index("- name: Prepare persistent agent caches"),
                        workflow.index("- name: Prepare the shared Bazel source-download directory"))
        writer = """from pathlib import Path
import sys
directory = Path(sys.argv[1])
directory.mkdir(parents=True, exist_ok=True)
partial = directory / 'file.partial'
partial.write_text(sys.argv[2])
partial.replace(directory / 'file')
"""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            directory.chmod(0o755)
            cache = directory / "persistent cache"
            package_cache = directory / "packages"
            workspace = directory / "workspace"
            for path in (cache, package_cache, workspace):
                path.mkdir()
                os.chown(path, nobody.pw_uid, nobody.pw_gid)
            cache_owner_mode = (cache.stat().st_uid, cache.stat().st_mode)
            legacy = cache / "repository_cache/content_addressable/sha256/old-digest"
            legacy.mkdir(parents=True)
            sentinel = legacy / "file"
            sentinel.write_bytes(b"existing root-owned digest")
            legacy.chmod(0o755)
            sentinel.chmod(0o644)
            before = (legacy.stat().st_uid, legacy.stat().st_mode,
                      sentinel.stat().st_uid, sentinel.stat().st_mode,
                      sentinel.stat().st_mtime_ns, sentinel.read_bytes())

            def execute(command, uid, environment=None):
                return subprocess.run(
                    command, cwd=workspace, env=environment, text=True, capture_output=True,
                    check=False, user=uid, group=nobody.pw_gid if uid else 0, extra_groups=[],
                )

            denied = execute([sys.executable, "-c", writer, str(legacy), "must not replace"], nobody.pw_uid)
            self.assertNotEqual(denied.returncode, 0)
            self.assertIn("PermissionError", denied.stderr)

            # Record the following workflow step's actual arguments and write to its output.
            # Source integrity checking itself is covered by source_archive_test.py.
            recorder = workspace / "tools/bazel/gzip/source_archive.py"
            recorder.parent.mkdir(parents=True)
            recorder.write_text("""import argparse, json
from pathlib import Path
parser = argparse.ArgumentParser()
parser.add_argument('--archives-dir', required=True)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
assert Path(args.archives_dir).is_dir()
args.output.mkdir(parents=True, exist_ok=True)
(args.output / 'verified-source').write_text('source archive')
print(json.dumps({'output': str(args.output)}))
""")
            (workspace / "artifacts/vs/repository-inputs").mkdir(parents=True)
            (workspace / "artifacts/vs/source-download-receipt.json").touch()
            for path in workspace.rglob("*"):
                os.chown(path, nobody.pw_uid, nobody.pw_gid)
            selected = {}
            for uid in (0, nobody.pw_uid):
                for attempt in (1, 2):
                    environment = dict(os.environ, SONIC_BAZEL_CACHE_SOURCE=str(cache),
                                       SONIC_DPKG_CACHE_SOURCE=str(package_cache),
                                       SONIC_DPKG_CACHE_METHOD="rwcache",
                                       GITHUB_ENV=str(workspace / f"env-{uid}-{attempt}"))
                    prepared = execute(["bash", "-e", "-c", self.workflow_step(
                        "Prepare persistent agent caches")], uid, environment)
                    self.assertEqual(prepared.returncode, 0, prepared.stderr)
                    exported = dict(line.split("=", 1) for line in Path(environment["GITHUB_ENV"]).read_text().splitlines())
                    effective = Path(exported["SONIC_BAZEL_CACHE_SOURCE"])
                    self.assertEqual(effective.parent, cache)
                    self.assertNotEqual(effective, cache)
                    self.assertEqual(effective.stat().st_uid, uid)
                    self.assertEqual(selected.setdefault(uid, effective), effective)
                    target = effective / "repository_cache/content_addressable/sha256/new-digest"
                    published = execute([sys.executable, "-c", writer, str(target), str(attempt)], uid)
                    self.assertEqual(published.returncode, 0, published.stderr)
                    self.assertEqual((target / "file").read_text(), str(attempt))
                    self.assertFalse((target / "file.partial").exists())
                    environment.update(exported)  # GitHub applies GITHUB_ENV to subsequent steps.
                    consumed = execute(["bash", "-e", "-c", self.workflow_step(
                        "Prepare the shared Bazel source-download directory")], uid, environment)
                    self.assertEqual(consumed.returncode, 0, consumed.stderr)
                    receipt = json.loads((workspace / "artifacts/vs/source-download-receipt.json").read_text())
                    self.assertEqual(receipt["output"], str(effective / "distdir"))
                    self.assertTrue((effective / "distdir/verified-source").is_file())
            self.assertNotEqual(selected[0], selected[nobody.pw_uid])
            self.assertFalse((cache / "distdir").exists())
            self.assertEqual(cache_owner_mode, (cache.stat().st_uid, cache.stat().st_mode))
            self.assertEqual(before, (legacy.stat().st_uid, legacy.stat().st_mode,
                                     sentinel.stat().st_uid, sentinel.stat().st_mode,
                                     sentinel.stat().st_mtime_ns, sentinel.read_bytes()))

    def make(self, makefile, mode="n", source=None, package_cache=None, run_as=None, cwd=ROOT):
        config = (ROOT / "rules/config").read_text().splitlines()
        defaults = "\n".join(line for line in config if line.startswith(
            ("BUILD_WITH_BAZEL_WHEN_AVAILABLE ?=", "SONIC_BAZEL_CACHE_SOURCE ?=",
             "SONIC_DPKG_CACHE_SOURCE ?=")))
        environment = dict(os.environ)
        for name in ("BUILD_WITH_BAZEL_WHEN_AVAILABLE", "SONIC_BAZEL_CACHE_SOURCE",
                     "SONIC_DPKG_CACHE_SOURCE", "MAKEFLAGS", "MFLAGS"):
            environment.pop(name, None)
        credentials = ({} if run_as is None else
                       {"user": run_as.pw_uid, "group": run_as.pw_gid, "extra_groups": []})
        return subprocess.run(
            ["make", "--no-print-directory", "-f", "-", "check", f"BUILD_WITH_BAZEL_WHEN_AVAILABLE={mode}"]
            + ([] if source is None else [f"SONIC_BAZEL_CACHE_SOURCE={source}"])
            + ([] if package_cache is None else [f"SONIC_DPKG_CACHE_SOURCE={package_cache}"]),
            input=defaults + "\n" + makefile, cwd=cwd, env=environment,
            text=True, capture_output=True, check=False, **credentials,
        )

    def cache_mount(self, directory, mode="n", source=None, package_cache=None, run_as=None):
        work = (ROOT / "Makefile.work").read_text()
        start = work.index("# Reuse downloads and action outputs")
        end = work.index('# User name and tag for "docker-*" images', start)
        docker = directory / "docker.py"
        docker.write_text("""import json, os, pathlib, sys
pathlib.Path(__file__).with_name('docker-started').touch()
args = sys.argv[1:]
record = {'args': args}
mounts = {args[index + 1].rsplit(':', 2)[1]: args[index + 1].rsplit(':', 2)[::2]
          for index, arg in enumerate(args) if arg == '-v'}
record['mounts'] = mounts
if '/bazel_cache' in mounts:
    source = pathlib.Path(mounts['/bazel_cache'][0])
    record['source_exists'] = source.is_dir()
    record['source_owner'] = source.stat().st_uid
    (source / 'container-cache-write').write_text('writable')
if '/etc/bazel.bazelrc' in mounts:
    record['system_rc'] = pathlib.Path(mounts['/etc/bazel.bazelrc'][0]).read_text()
print(json.dumps(record))
""")
        makefile = f"""DOCKER_RUN = python3 {docker}
{work[start:end]}
.PHONY: check
check:
	@$(DOCKER_RUN) builder-image
"""
        return self.make(makefile, mode, source, package_cache, run_as=run_as,
                         cwd=directory if run_as else ROOT)

    def test_default_uses_package_cache_location_independently_of_selector(self):
        """Retain the shared cache location for all Bazel consumers and allow an explicit opt-out."""
        # Only expand the default: never create or alter the user's real cache.
        makefile = ".PHONY: check\ncheck:\n\t@printf '%s\\n' '$(SONIC_BAZEL_CACHE_SOURCE)'\n"
        for mode, source, package_cache, expected in (
            ("n", None, None, "/var/cache/sonic/artifacts/bazel"),
            ("y", None, None, "/var/cache/sonic/artifacts/bazel"),
            ("n", None, "/example/packages", "/example/packages/bazel"),
            ("y", None, "/example/packages", "/example/packages/bazel"),
            ("y", "", "/example/packages", ""),
            ("n", "/example/cache", "/example/packages", "/example/cache"),
        ):
            with self.subTest(mode=mode, source=source, package_cache=package_cache):
                result = self.make(makefile, mode, source, package_cache)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.strip(), expected)

    def test_explicit_cache_works_with_bazel_disabled_and_preserves_quoted_paths(self):
        """Make the cache writable by the builder and pass quoted host paths intact regardless of
        the global Bazel selector.
        """
        for mode in ("n", "y"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                source = directory / "builder's `printf SHOULD_NOT_EXPAND` cache"
                result = self.cache_mount(directory, mode, source)
                self.assertEqual(result.returncode, 0, result.stderr)
                record = json.loads(result.stdout)
                self.assertTrue(record["source_exists"])
                self.assertEqual(record["source_owner"], os.getuid())
                self.assertEqual(record["mounts"]["/bazel_cache"], [str(source), "rw"])
                self.assertIn("BAZELISK_HOME=/bazel_cache/bazelisk", record["args"])
                self.assertFalse(any(arg.startswith("BAZEL_CONTAINER_CACHE_DIR=") for arg in record["args"]))
                self.assert_system_cache_settings(record)
                self.assertTrue((source / "container-cache-write").is_file())

    def assert_system_cache_settings(self, record):
        self.assertEqual(record["mounts"]["/etc/bazel.bazelrc"],
                         [str(ROOT / "tools/bazel/slave.bazelrc"), "ro"])
        options = [line.split() for line in record["system_rc"].splitlines()
                   if line.strip() and not line.lstrip().startswith("#")]
        self.assertIn(["common", "--repository_cache=/bazel_cache/repository_cache"], options)
        self.assertIn(["common", "--disk_cache=/bazel_cache/disk_cache"], options)
        # Sharing output/server state would couple unrelated workspaces. Only
        # completed action outputs and downloaded files belong in these mounts.
        self.assertFalse(any(option.startswith(("--output_base", "--output_user_root"))
                             for line in options for option in line))

    def test_default_cache_is_created_before_docker_and_mounts_system_rc(self):
        """Supply shared cache settings to every Bazel invocation through the mounted system RC."""
        for mode in ("n", "y"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                package_cache = directory / "packages"
                source = package_cache / "bazel"
                result = self.cache_mount(directory, mode, package_cache=package_cache)
                self.assertEqual(result.returncode, 0, result.stderr)
                record = json.loads(result.stdout)
                self.assertEqual(record["mounts"]["/bazel_cache"], [str(source), "rw"])
                self.assertEqual(record["source_owner"], os.getuid())
                self.assertTrue((source / "container-cache-write").is_file())
                self.assert_system_cache_settings(record)

    def test_explicit_empty_cache_adds_no_cache_mount_or_system_rc(self):
        """Disabling the cache must also avoid an RC that refers to an unmounted shared path."""
        for mode in ("n", "y"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as temporary:
                result = self.cache_mount(Path(temporary), mode, "")
                self.assertEqual(result.returncode, 0, result.stderr)
                record = json.loads(result.stdout)
                self.assertNotIn("/bazel_cache", record["mounts"])
                self.assertNotIn("/etc/bazel.bazelrc", record["mounts"])
                self.assertNotIn("system_rc", record)
                self.assertFalse(any(arg.startswith("BAZELISK_HOME=") for arg in record["args"]))
                self.assertEqual(record["args"], ["builder-image"])
                self.assertEqual({path.name for path in Path(temporary).iterdir()},
                                 {"docker.py", "docker-started"})

    def test_relative_cache_path_is_mounted_as_an_absolute_host_directory(self):
        """Docker must receive a bind source, not interpret a relative path as a volume name."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            source = directory / "relative cache"
            result = self.cache_mount(directory, source=os.path.relpath(source, ROOT))
            self.assertEqual(result.returncode, 0, result.stderr)
            record = json.loads(result.stdout)
            self.assertEqual(record["mounts"]["/bazel_cache"], [str(source), "rw"])
            self.assertTrue((source / "container-cache-write").is_file())

    def test_unusable_cache_stops_before_docker_even_without_swss(self):
        """Reject an invalid host cache path before Docker starts, without overwriting that path
        or requiring SWSS.
        """
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            source = directory / "cache"
            source.write_bytes(b"existing file")
            result = self.cache_mount(directory, source=source)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, "")
            self.assertFalse((directory / "docker-started").exists())
            self.assertEqual(source.read_bytes(), b"existing file")

    def test_unwritable_cache_stops_before_docker(self):
        """An existing directory must be writable, even when mkdir itself succeeds."""
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            # CI runs these tests as root in a Debian container. Drop privileges
            # for this Make invocation so directory permissions remain meaningful.
            run_as = pwd.getpwnam("nobody") if os.geteuid() == 0 else None
            if run_as:
                os.chown(directory, run_as.pw_uid, run_as.pw_gid)
            source = directory / "read-only cache"
            source.mkdir()
            source.chmod(0o555)
            try:
                result = self.cache_mount(directory, source=source, run_as=run_as)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "")
                self.assertFalse((directory / "docker-started").exists())
                self.assertFalse((source / "container-cache-write").exists())
            finally:
                source.chmod(0o755)


if __name__ == "__main__":
    unittest.main()
