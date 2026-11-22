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
            "${{ inputs.vs_artifact_id && github.sha || github.event.pull_request.head.sha || github.sha }}",
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

    def test_staged_yang_tools_run_on_the_architecture_assigned_to_the_job(self):
        """The image root defaults to AMD64, so both the test and its copy tool need the native platform."""
        job = yaml.safe_load(WORKFLOW.read_text())["jobs"]["package"]
        step = next(step for step in job["steps"] if step.get("id") == "yang_build")
        self.assertEqual(step["env"]["NATIVE_PLATFORM"], "@sonic_build_infra//platforms:${{ matrix.machine }}_trixie")
        machines = {row["architecture"]: row["machine"] for row in job["strategy"]["matrix"]["include"]}
        self.assertEqual(machines, {"amd64": "x86_64", "arm64": "aarch64"})
        self.assertIn('libteam_draft_bazel.sh test --platforms="$NATIVE_PLATFORM" --host_platform="$NATIVE_PLATFORM"',
                      step["run"])

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
            env = dict(os.environ, HOME=str(home), PATH=str(binary) + os.pathsep + os.environ["PATH"], RECORD=str(record),
                       BASH_ENV="/dev/null", NATIVE_PLATFORM="@sonic_build_infra//platforms:aarch64_trixie")
            subprocess.run(["bash", "-euc", setup], cwd=work, env=env, check=True)
            user = (work / ".bazelrc.user").read_text()
            self.assertEqual(user.strip().splitlines(), [
                "build --disk_cache=/retained/disk",
                "common --repository_cache=/retained/repository",
                "common --remote_cache=grpc://172.16.0.4:9092",
                "common --experimental_remote_downloader=grpc://172.16.0.4:9092",
                "common --experimental_remote_downloader_local_fallback",
                "build --remote_local_fallback",
                "build --incompatible_remote_local_fallback_for_remote_cache",
                "build --experimental_build_event_upload_strategy=local",
            ])
            subprocess.run(["bash", "-euc", "bash " + command], cwd=work, env=env, check=True)
            observed = json.loads(record.read_text())
            self.assertIn("--nohome_rc", observed["argv"])
            self.assertIn("cquery", observed["argv"])
            self.assertIn("--platforms=" + env["NATIVE_PLATFORM"], observed["argv"])
            self.assertIn("--host_platform=" + env["NATIVE_PLATFORM"], observed["argv"])
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

    def test_native_workflows_prepare_retained_submodules_consistently(self):
        steps = yaml.safe_load(WORKFLOW.read_text())["jobs"]["package"]["steps"]
        prepare = next(step["run"] for step in steps
                       if step.get("name") == "Prepare pinned source submodules")
        self.assertEqual(prepare, "bash tools/bazel/ci/prepare_image_submodules.sh")
        selected = set()
        for path in WORKFLOW.parent.glob("*.yml"):
            for name, job in yaml.safe_load(path.read_text())["jobs"].items():
                for index, step in enumerate(job.get("steps", [])):
                    command = step.get("run", "")
                    if command != prepare:
                        continue
                    with self.subTest(workflow=path.name, job=name):
                        self.assertTrue(any(
                            earlier.get("uses", "").startswith("actions/checkout@")
                            for earlier in job["steps"][:index]
                        ))
                        selection = next((i for i, earlier in enumerate(job["steps"])
                                          if earlier.get("name") == "Select the reviewed source and registry"), None)
                        if selection is not None:
                            self.assertLess(selection, index)
                        selected.add((path.name, name))
        self.assertEqual(selected, {
            ("bazel-oci.yml", name) for name in
            ("archive", "package_amd64", "checks", "dhcp_relay_source_image")
        } | {
            ("bazel-eventd.yml", "native"),
            ("bazel-supervisor-rust.yml", "native"), ("bazel-yang-models.yml", "package"),
            ("bazel-libteam.yml", "native"), ("bazel-sysmgr.yml", "native"),
        })

    def test_preparation_restores_pins_without_cloning_unrelated_modules_or_losing_edits(self):
        _, build = self.steps()
        command = build.strip().splitlines()[-1]
        self.assertEqual(command, "git diff --exit-code HEAD")
        steps = yaml.safe_load(WORKFLOW.read_text())["jobs"]["package"]["steps"]
        prepare = next(step["run"] for step in steps
                       if step.get("name") == "Prepare pinned source submodules")

        def git(directory, *arguments):
            return subprocess.run(
                ["git", "-C", str(directory), "-c", "user.name=YANG Test",
                 "-c", "user.email=yang-test@example.invalid", *arguments],
                check=True, capture_output=True, text=True,
            ).stdout.strip()

        with tempfile.TemporaryDirectory(prefix="yang source check ") as temporary:
            root = Path(temporary)
            nested_source = root / "nested source"
            nested_source.mkdir()
            git(nested_source, "init", "--quiet")
            (nested_source / "nested.txt").write_text("previous nested source\n")
            git(nested_source, "add", ".")
            git(nested_source, "commit", "--quiet", "-m", "Previous nested pin")
            nested_previous = git(nested_source, "rev-parse", "HEAD")
            (nested_source / "nested.txt").write_text("current nested source\n")
            git(nested_source, "commit", "--quiet", "-am", "Current nested pin")
            nested_expected = git(nested_source, "rev-parse", "HEAD")
            source = root / "pins source"
            source.mkdir()
            git(source, "init", "--quiet")
            (source / "source.txt").write_text("previous job\n")
            git(source, "-c", "protocol.file.allow=always", "submodule", "add",
                str(nested_source), "nested")
            git(source / "nested", "checkout", "--detach", nested_previous)
            git(source, "add", ".")
            git(source, "commit", "--quiet", "-m", "Previous pin")
            previous = git(source, "rev-parse", "HEAD")
            (source / "source.txt").write_text("current pin\n")
            git(source / "nested", "checkout", "--detach", nested_expected)
            git(source, "commit", "--quiet", "-am", "Current pin")

            work = root / "checkout"
            component = work / "src/sonic-yang-models"
            component.mkdir(parents=True)
            helper = Path("tools/bazel/ci/prepare_image_submodules.sh")
            (work / helper).parent.mkdir(parents=True)
            shutil.copyfile(ROOT / helper, work / helper)
            tracked = component / "setup.py"
            tracked.write_text("original source\n")
            git(work, "init", "--quiet")
            git(work, "-c", "protocol.file.allow=always", "submodule", "add",
                str(source), "src/sonic-p4rt/sonic-pins")
            for name in ("src/sonic-sysmgr/gnoi", "src/unused"):
                git(work, "-c", "protocol.file.allow=always", "submodule", "add",
                    str(source), name)
            git(work, "add", ".")
            git(work, "commit", "--quiet", "-m", "Distribution and pinned source")
            for name in ("src/sonic-sysmgr/gnoi", "src/unused"):
                git(work, "submodule", "deinit", "--", name)
            git(work, "config", "submodule.src/unused.url", str(source))
            git(work / "src/sonic-p4rt/sonic-pins", "checkout", "--detach", previous)
            git(work / "src/sonic-p4rt/sonic-pins", "-c", "protocol.file.allow=always",
                "submodule", "update", "--init", "--", "nested")

            (source / "source.txt").write_text("newly pinned source\n")
            git(source, "commit", "--quiet", "-am", "New pin absent from retained clone")
            expected = git(source, "rev-parse", "HEAD")
            git(work, "update-index", "--cacheinfo",
                "160000," + expected + ",src/sonic-p4rt/sonic-pins")
            git(work, "commit", "--quiet", "-m", "Update the recorded pin")
            baseline = subprocess.run(
                ["git", "diff", "--exit-code"], cwd=component,
                capture_output=True, text=True, check=False,
            )
            self.assertNotEqual(baseline.returncode, 0)
            self.assertIn("Subproject commit", baseline.stdout)

            environment = dict(os.environ, GITHUB_WORKSPACE=str(work),
                               GIT_ALLOW_PROTOCOL="file", BASH_ENV="/dev/null")
            for attempt in range(2):
                prepared = subprocess.run(
                    ["bash", "-euc", prepare], cwd=work, env=environment,
                    capture_output=True, text=True, check=False,
                )
                self.assertEqual(prepared.returncode, 0,
                                 f"Attempt {attempt}: {prepared.stdout}{prepared.stderr}")
                self.assertEqual(git(work / "src/sonic-p4rt/sonic-pins",
                                     "rev-parse", "HEAD"), expected)
                self.assertEqual(git(work / "src/sonic-p4rt/sonic-pins/nested",
                                     "rev-parse", "HEAD"), nested_expected)
                self.assertTrue((work / "src/sonic-sysmgr/gnoi/.git").is_file())
                self.assertFalse((work / "src/unused/.git").exists())

            def check():
                return subprocess.run(
                    ["bash", "-euc", command], cwd=component,
                    env=dict(os.environ, BASH_ENV="/dev/null"),
                    capture_output=True, text=True, check=False,
                )

            clean = check()
            self.assertEqual(clean.returncode, 0, clean.stdout + clean.stderr)
            tracked.write_text("changed source\n")
            for staged in (False, True):
                with self.subTest(staged=staged):
                    if staged:
                        git(work, "add", "src/sonic-yang-models/setup.py")
                    changed = check()
                    self.assertNotEqual(changed.returncode, 0)
                    self.assertIn("changed source", changed.stdout)

            pins = work / "src/sonic-p4rt/sonic-pins"
            git(pins, "checkout", "--detach", previous)
            dirty = pins / "source.txt"
            dirty.write_text("uncommitted source to preserve\n")
            rejected = subprocess.run(
                ["bash", "-euc", prepare], cwd=work, env=environment,
                capture_output=True, text=True, check=False,
            )
            self.assertNotEqual(rejected.returncode, 0)
            self.assertIn("would be overwritten", rejected.stderr)
            self.assertEqual(dirty.read_text(), "uncommitted source to preserve\n")

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
