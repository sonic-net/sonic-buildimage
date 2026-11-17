"""Exercise manual base-image selection against real Git branch histories."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.base_review import validate_pair, verify_branch_commit

WRAPPER = ROOT / "tools/bazel/ci/libteam_draft_bazel.sh"


class BaseReviewTest(unittest.TestCase):
    def test_pair_requires_two_lowercase_full_shas_and_amd64(self):
        validate_pair("a" * 40, "b" * 40, "amd64")
        for source, catalog, arch in [
            ("master", "b" * 40, "amd64"), ("a" * 40, "", "amd64"),
            ("", "b" * 40, "amd64"), ("A" * 40, "b" * 40, "amd64"),
            ("a" * 40, "b" * 40, "all"),
        ]:
            with self.subTest(source=source, catalog=catalog, arch=arch), self.assertRaises(ValueError):
                validate_pair(source, catalog, arch)

    def test_only_commits_reachable_from_a_published_branch_are_accepted(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            repo = root / "repository"

            def git(*args):
                return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()

            repo.mkdir()
            git("init", "-q", "-b", "main")
            git("config", "user.email", "fixture@example.org")
            git("config", "user.name", "Fixture")
            git("commit", "-q", "--allow-empty", "-m", "base")
            base = git("rev-parse", "HEAD")
            git("checkout", "-q", "-b", "review")
            git("commit", "-q", "--allow-empty", "-m", "review")
            review = git("rev-parse", "HEAD")
            git("checkout", "-q", "main")
            git("checkout", "-q", "--detach")
            git("commit", "-q", "--allow-empty", "-m", "unreferenced")
            detached = git("rev-parse", "HEAD")
            git("checkout", "-q", "main")

            verify_branch_commit(str(repo), base, root / "published-base")
            verify_branch_commit(str(repo), review, root / "published-review")
            with self.assertRaises((ValueError, subprocess.CalledProcessError)):
                verify_branch_commit(str(repo), detached, root / "unpublished")

    def test_review_replaces_the_default_registry_and_disables_uploads(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            recorder = directory / "bazel"
            recorder.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
            recorder.chmod(0o755)
            env = {**os.environ, "PATH": f"{directory}:/usr/bin:/bin",
                   "SONIC_BASE_REVIEW_REGISTRY": "a" * 40}
            result = subprocess.check_output([WRAPPER, "build", "//..."], env=env, text=True)
            registries = [arg for arg in result.splitlines() if arg.startswith("--registry=")]
            self.assertEqual(registries, [
                "--registry=https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/" + "a" * 40,
                "--registry=https://bcr.bazel.build",
            ])
            self.assertIn("--remote_upload_local_results=false", result.splitlines())
            private_base = "--output_base=" + str(directory / "owner")
            command = subprocess.check_output([WRAPPER, private_base, "shutdown"], env=env, text=True)
            self.assertEqual(command.splitlines(), [private_base, "--nosystem_rc", "--nohome_rc",
                             "--noworkspace_rc", f"--bazelrc={Path.cwd()}/tools/bazel/common.bazelrc", "shutdown"])
            for extra in (["--registry=other"], ["--registry", "other"]):
                result = subprocess.run([WRAPPER, "build", *extra], env=env, capture_output=True)
                self.assertNotEqual(result.returncode, 0)
            env.pop("SONIC_BASE_REVIEW_REGISTRY")
            ordinary = subprocess.check_output([WRAPPER, "info", "release"], env=env, text=True)
            self.assertIn("--registry=https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/main", ordinary.splitlines())
