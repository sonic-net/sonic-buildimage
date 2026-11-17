#!/usr/bin/env python3
"""Exercise YANG's real cache setup and root-query wrapper without Bazel."""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = ROOT / ".github/workflows/bazel-yang-models.yml"


class YangCacheWorkflowTest(unittest.TestCase):
    def test_checkout_uses_guarded_kernel_workspace_repair(self):
        workflow = yaml.safe_load(WORKFLOW.read_text())
        steps = workflow["jobs"]["package"]["steps"]
        kernel = yaml.safe_load(
            (ROOT / ".github/workflows/bazel-kernel-bundle.yml").read_text()
        )
        repair = next(
            step for step in kernel["jobs"]["kernel-bundle"]["steps"]
            if step.get("name") == "Restore workspace access before checkout"
        )
        self.assertEqual(steps[0], repair)
        self.assertTrue(steps[1]["uses"].startswith("actions/checkout@"))
        self.assertEqual(
            steps[1]["with"]["ref"],
            "${{ github.event.pull_request.head.sha || github.sha }}",
        )

    def copy_helper(self, work):
        relative = "tools/bazel/ci/preserve_setup_bazel_cache.py"
        target = work / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / relative, target)

    def steps(self):
        steps = yaml.safe_load(WORKFLOW.read_text())["jobs"]["package"]["steps"]
        cache = next(s for s in steps if s.get("name") == "Preserve setup-bazel caches for the image-root query")
        build = next(s for s in steps if s.get("name") == "Build and verify the installed source distribution")
        setup = next(s for s in steps if s.get("uses", "").startswith("bazel-contrib/setup-bazel@"))
        self.assertLess(steps.index(setup), steps.index(cache))
        self.assertLess(steps.index(cache), steps.index(build))
        return cache["run"], build["run"]

    def test_actual_setup_reaches_root_query_without_other_home_options(self):
        setup, build = self.steps()
        query = re.search(r"\(cd \.\./\.\. && (tools/bazel/ci/.*?cquery.*?)\) > artifacts/image-root-targets.txt", build, re.S)
        self.assertIsNotNone(query)
        command = query.group(1)
        wrapper = command.split()[0]
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            self.copy_helper(work)
            home = work / "home"
            home.mkdir()
            # No workspace cache settings exist before the real setup step.
            (home / ".bazelrc").write_text(
                "build --disk_cache=/retained/disk\n"
                "common --repository_cache=/retained/repository\n"
                "common --registry=https://unrelated.invalid\n"
                "startup --output_base=/unrelated/output\n")
            for relative in (wrapper, "tools/bazel/common.bazelrc"):
                target = work / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / relative, target)
            record = work / "record.json"
            binary = work / "bin"
            binary.mkdir()
            recorder = binary / "bazel"
            recorder.write_text("""#!/usr/bin/env python3
import json, os, sys
from pathlib import Path
args = sys.argv[1:]
rc = Path(next(arg.split('=', 1)[1] for arg in args if arg.startswith('--bazelrc=')))
texts = [rc.read_text()]
for line in texts[0].splitlines():
    if line.startswith('try-import %workspace%/'):
        path = Path.cwd() / line.removeprefix('try-import %workspace%/')
        if path.is_file():
            texts.append(path.read_text())
Path(os.environ['RECORD']).write_text(json.dumps({'argv': args, 'explicit_rc': texts}))
""")
            recorder.chmod(0o755)
            env = dict(os.environ, HOME=str(home), PATH=str(binary) + os.pathsep + os.environ["PATH"], RECORD=str(record), BASH_ENV="/dev/null")
            subprocess.run(["bash", "-euc", setup], cwd=work, env=env, check=True)
            user = (work / ".bazelrc.user").read_text()
            self.assertEqual(user.strip().splitlines(), [
                "build --disk_cache=/retained/disk",
                "common --repository_cache=/retained/repository",
                "common --remote_cache=grpc://172.16.0.4:9092",
                "common --experimental_remote_downloader=grpc://172.16.0.4:9092",
                "common --experimental_remote_downloader_local_fallback",
            ])
            subprocess.run(["bash", "-euc", "bash " + command], cwd=work, env=env, check=True)
            observed = json.loads(record.read_text())
            self.assertIn("--nohome_rc", observed["argv"])
            self.assertIn("cquery", observed["argv"])
            self.assertIn("//src/sonic-yang-models:yang_models_package_test", observed["argv"])
            self.assertIn("--lockfile_mode=update", observed["argv"])
            selected_rc = "\n".join(observed["explicit_rc"])
            self.assertIn("common --repository_cache=/retained/repository", selected_rc)
            self.assertIn("build --disk_cache=/retained/disk", selected_rc)
            self.assertIn("common --remote_cache=grpc://172.16.0.4:9092", selected_rc)
            self.assertIn("common --experimental_remote_downloader=grpc://172.16.0.4:9092", selected_rc)
            self.assertNotIn("unrelated", selected_rc)
            self.assertFalse(any("unrelated" in arg for arg in observed["argv"]))

    def test_setup_preserves_local_download_policy(self):
        setup, _ = self.steps()
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            self.copy_helper(work)
            home = work / "home"
            home.mkdir()
            (home / ".bazelrc").write_text("build --disk_cache=/disk\ncommon --repository_cache=/repository\n")
            before = "common --experimental_downloader_config=/verified/downloads.cfg\n"
            (work / ".bazelrc.user").write_text(before)
            subprocess.run(["bash", "-euc", setup], cwd=work, env=dict(os.environ, HOME=str(home), BASH_ENV="/dev/null"), check=True)
            self.assertTrue((work / ".bazelrc.user").read_text().startswith(before))

    def test_missing_or_duplicate_cache_settings_leave_local_rc_unchanged(self):
        setup, _ = self.steps()
        for settings in ("", "build --disk_cache=/disk\n", "build --disk_cache=/one\nbuild --disk_cache=/two\n"):
            with self.subTest(settings=settings), tempfile.TemporaryDirectory() as temporary:
                work = Path(temporary)
                self.copy_helper(work)
                home = work / "home"
                home.mkdir()
                (home / ".bazelrc").write_text(settings)
                before = "common --experimental_downloader_config=/verified/downloads.cfg\n"
                (work / ".bazelrc.user").write_text(before)
                result = subprocess.run(["bash", "-euc", setup], cwd=work, env=dict(os.environ, HOME=str(home), BASH_ENV="/dev/null"), capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("cache setting", result.stderr)
                self.assertEqual((work / ".bazelrc.user").read_text(), before)


if __name__ == "__main__":
    unittest.main()
