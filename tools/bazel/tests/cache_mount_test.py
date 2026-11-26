#!/usr/bin/env python3
"""Check host cache configuration without starting Docker or a builder."""

import json
import os
from pathlib import Path
import pwd
import subprocess
import sys
import tempfile
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[3]


class CacheMountTest(unittest.TestCase):
    def test_local_repo_contents_cache_allows_builder_and_user_overrides(self):
        rc = (ROOT / ".bazelrc").read_text().splitlines()
        host = "common --repo_contents_cache=~/.cache/sonic/bazel-repo-contents"
        builder = "try-import /etc/sonic-buildimage/repo-contents.bazelrc"
        common = "import %workspace%/tools/bazel/common.bazelrc"
        self.assertLess(rc.index(host), rc.index(builder))
        self.assertLess(rc.index(builder), rc.index(common))
        self.assertIn("try-import %workspace%/.bazelrc.user",
                      (ROOT / "tools/bazel/common.bazelrc").read_text().splitlines())

    @unittest.skipUnless(os.geteuid() == 0, "requires root to exercise actual root/non-root cache ownership")
    def test_vs_cache_rejects_unwritable_entries_and_reuses_the_runner_cache(self):
        """A VS runner can reuse its own writes and cannot replace another owner's cached digest."""
        nobody = pwd.getpwnam("nobody")
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        steps = workflow["jobs"]["vs_container"]["steps"]
        prepare, = (step["run"] for step in steps if step.get("name") == "Configure persistent caches")
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
            cache.chmod(0o700)
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
            def environment(source, env_file):
                return dict(os.environ, SONIC_BAZEL_CACHE_SOURCE=str(source),
                            SONIC_DPKG_CACHE_SOURCE=str(package_cache), GITHUB_ENV=str(workspace / env_file))

            blocked = execute(["bash", "-e", "-c", prepare], nobody.pw_uid, environment(cache, "blocked-env"))
            self.assertNotEqual(blocked.returncode, 0)
            self.assertIn("Unwritable cache directory", blocked.stderr)
            self.assertFalse((workspace / "blocked-env").exists())
            for uid in (0, nobody.pw_uid):
                effective = cache / f"runner-{uid}" / "vs-cache/bazel"
                for attempt in (1, 2):
                    env = environment(effective, f"env-{uid}-{attempt}")
                    prepared = execute(["bash", "-e", "-c", prepare], uid, env)
                    self.assertEqual(prepared.returncode, 0, prepared.stderr)
                    exported = dict(line.split("=", 1) for line in Path(env["GITHUB_ENV"]).read_text().splitlines())
                    self.assertEqual(exported["SONIC_BAZEL_CACHE_SOURCE"], str(effective))
                    self.assertEqual(exported["SONIC_DPKG_CACHE_SOURCE"], str(package_cache))
                    target = effective / "repository_cache/content_addressable/sha256/new-digest"
                    published = execute([sys.executable, "-c", writer, str(target), str(attempt)], uid)
                    self.assertEqual(published.returncode, 0, published.stderr)
                    self.assertEqual((target / "file").read_text(), str(attempt))
                    self.assertFalse((target / "file.partial").exists())
                    receipt = json.loads((workspace / "artifacts/vs/source-download-receipt.json").read_text())
                    self.assertEqual(Path(receipt["output"]), effective / "distdir")
                    self.assertEqual((effective / "distdir/verified-source").read_text(), "source archive")
            self.assertEqual(cache_owner_mode, (cache.stat().st_uid, cache.stat().st_mode))
            self.assertEqual(before, (legacy.stat().st_uid, legacy.stat().st_mode,
                                     sentinel.stat().st_uid, sentinel.stat().st_mode,
                                     sentinel.stat().st_mtime_ns, sentinel.read_bytes()))

    def make(self, makefile, mode="n", source=None, package_cache=None, run_as=None, cwd=ROOT,
             repo_contents=None, developer_home=None):
        config = (ROOT / "rules/config").read_text().splitlines()
        defaults = "\n".join(line for line in config if line.startswith(
            ("BUILD_WITH_BAZEL_WHEN_AVAILABLE ?=", "SONIC_BAZEL_CACHE_SOURCE ?=",
             "SONIC_DPKG_CACHE_SOURCE ?=", "sonic_bazel_dev_repo_contents =",
             "SONIC_BAZEL_REPO_CONTENTS_SOURCE ?=")))
        environment = dict(os.environ, USER="test-builder")
        for name in ("BUILD_WITH_BAZEL_WHEN_AVAILABLE", "SONIC_BAZEL_CACHE_SOURCE",
                     "SONIC_DPKG_CACHE_SOURCE", "SONIC_BAZEL_REPO_CONTENTS_SOURCE", "MAKEFLAGS", "MFLAGS"):
            environment.pop(name, None)
        credentials = ({} if run_as is None else
                       {"user": run_as.pw_uid, "group": run_as.pw_gid, "extra_groups": []})
        return subprocess.run(
            ["make", "--no-print-directory", "-f", "-", "check", f"BUILD_WITH_BAZEL_WHEN_AVAILABLE={mode}"]
            + ([] if source is None else [f"SONIC_BAZEL_CACHE_SOURCE={source}"])
            + ([] if package_cache is None else [f"SONIC_DPKG_CACHE_SOURCE={package_cache}"])
            + ([] if repo_contents is None else [f"SONIC_BAZEL_REPO_CONTENTS_SOURCE={repo_contents}"])
            + ([] if developer_home is None else [f"HOME={developer_home}"]),
            input=defaults + "\n" + makefile, cwd=cwd, env=environment,
            text=True, capture_output=True, check=False, **credentials,
        )

    def cache_mount(self, directory, mode="n", source=None, package_cache=None, run_as=None,
                    repo_contents=None):
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
if '/bazel_repo_contents_cache' in mounts:
    prepared = pathlib.Path(mounts['/bazel_repo_contents_cache'][0])
    marker = prepared / 'prior-builder-write'
    record['reused_prepared'] = marker.is_file()
    marker.touch()
print(json.dumps(record))
""")
        makefile = f"""DOCKER_RUN = python3 {docker}
{work[start:end]}
.PHONY: check
check:
	@$(DOCKER_RUN) builder-image
"""
        return self.make(makefile, mode, source, package_cache, run_as=run_as,
                         cwd=directory if run_as else ROOT, repo_contents=repo_contents,
                         developer_home=directory / "developer's home")

    def test_default_uses_package_cache_location_independently_of_selector(self):
        """Retain the shared cache location for all Bazel consumers and allow an explicit opt-out."""
        # Only expand the default: never create or alter the user's real cache.
        makefile = ".PHONY: check\ncheck:\n\t@printf '%s\\n' '$(SONIC_BAZEL_CACHE_SOURCE)'\n"
        for mode, source, package_cache, expected in (
            ("n", None, None, "/var/cache/sonic/artifacts/bazel/test-builder"),
            ("y", None, None, "/var/cache/sonic/artifacts/bazel/test-builder"),
            ("n", None, "/example/packages", "/example/packages/bazel/test-builder"),
            ("y", None, "/example/packages", "/example/packages/bazel/test-builder"),
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
        # Keep each workspace's output base and server independent.
        self.assertFalse(any(option.startswith(("--output_base", "--output_user_root"))
                             for line in options for option in line))

    def test_default_cache_is_created_before_docker_and_mounts_system_rc(self):
        """Supply shared cache settings to every Bazel invocation through the mounted system RC."""
        for mode in ("n", "y"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                package_cache = directory / "packages"
                source = package_cache / "bazel" / "test-builder"
                result = self.cache_mount(directory, mode, package_cache=package_cache)
                self.assertEqual(result.returncode, 0, result.stderr)
                record = json.loads(result.stdout)
                prepared = directory / "developer's home/.cache/sonic/bazel-repo-contents"
                self.assertEqual(record["mounts"]["/bazel_repo_contents_cache"], [str(prepared), "rw"])
                self.assertEqual(prepared.stat().st_mode & 0o777, 0o700)
                self.assertEqual(record["mounts"]["/etc/sonic-buildimage/repo-contents.bazelrc"],
                                 [str(ROOT / "tools/bazel/slave-repo-contents.bazelrc"), "ro"])
                self.assertFalse(record["reused_prepared"])
                repeat = self.cache_mount(directory, mode, package_cache=package_cache)
                self.assertEqual(repeat.returncode, 0, repeat.stderr)
                self.assertTrue(json.loads(repeat.stdout)["reused_prepared"])
                self.assertEqual(record["mounts"]["/bazel_cache"], [str(source), "rw"])
                self.assertEqual(record["source_owner"], os.getuid())
                self.assertTrue((source / "container-cache-write").is_file())
                self.assertEqual(source.stat().st_mode & 0o777, 0o700)
                self.assert_system_cache_settings(record)

    def test_unavailable_default_uses_container_local_cache(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            blocked = directory / "packages"
            blocked.touch()
            result = self.cache_mount(directory, package_cache=blocked, repo_contents="")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("container-local caches", result.stderr)
            self.assertEqual(json.loads(result.stdout)["args"], ["builder-image"])

    def test_prepared_repository_mount_is_separate_from_action_and_download_caches(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            cache = directory / "existing caches"
            prepared = directory / "runner's prepared repositories"
            prepared.mkdir()
            result = self.cache_mount(directory, source=cache, repo_contents=prepared)
            self.assertEqual(result.returncode, 0, result.stderr)
            mounts = json.loads(result.stdout)["mounts"]
            self.assertEqual(mounts["/bazel_cache"], [str(cache), "rw"])
            self.assertEqual(mounts["/bazel_repo_contents_cache"], [str(prepared), "rw"])
            self.assertFalse((directory / "developer's home").exists())

    def test_empty_or_missing_prepared_cache_override(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            cache = directory / "cache"
            disabled = self.cache_mount(directory, source=cache, repo_contents="")
            self.assertEqual(disabled.returncode, 0, disabled.stderr)
            mounts = json.loads(disabled.stdout)["mounts"]
            self.assertIn("/bazel_cache", mounts)
            self.assertNotIn("/bazel_repo_contents_cache", mounts)
            self.assertNotIn("/etc/sonic-buildimage/repo-contents.bazelrc", mounts)
            missing = directory / "unmounted cache"
            failed = self.cache_mount(directory, source=cache, repo_contents=missing)
            self.assertNotEqual(failed.returncode, 0)
            self.assertFalse(missing.exists())

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
