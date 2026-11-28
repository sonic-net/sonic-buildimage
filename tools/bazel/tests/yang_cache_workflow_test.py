#!/usr/bin/env python3
"""Exercise YANG's real cache setup and root-query wrapper without Bazel."""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = ROOT / ".github/workflows/bazel-yang-models.yml"


class YangCacheWorkflowTest(unittest.TestCase):
    def test_workspace_repair_is_scoped_and_preserves_cache_links(self):
        steps = yaml.safe_load(WORKFLOW.read_text())["jobs"]["package"]["steps"]
        repair, checkout = steps[:2]
        self.assertEqual(repair["name"], "Restore workspace access before checkout")
        self.assertEqual(repair["working-directory"], "${{ runner.temp }}")
        self.assertEqual(repair["env"]["RUNNER_PIPELINE_WORKSPACE"], "${{ runner.workspace }}")
        self.assertTrue(checkout["uses"].startswith("actions/checkout@"))
        self.assertEqual(
            checkout["with"]["ref"],
            "${{ inputs.vs_artifact_id && github.sha || github.event.pull_request.head.sha || github.sha }}",
        )
        script = repair["run"]
        for scenario in ("repair", "separate-temp", "aliased-work-root",
                         "absent", "outside", "symlink", "sudo-failure"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                work_root = root / "work"
                scratch = (root / "separate-disk/_temp" if scenario == "separate-temp"
                           else work_root / "_temp")
                scratch.mkdir(parents=True)
                workspace = work_root / "sonic-buildimage/sonic-buildimage"
                workspace.mkdir(parents=True)
                outside = root / "cache"
                outside.mkdir()
                payload = outside / "object"
                payload.write_text("shared cache object")
                payload.chmod(0o444)
                linked = workspace / "hardlink"
                os.link(payload, linked)
                (workspace / "symlink").symlink_to(outside, target_is_directory=True)
                regular = workspace / "tracked-file"
                regular.write_text("tracked")
                regular.chmod(0o444)
                nested = workspace / ".git"
                nested.mkdir()
                nested.chmod(0o555)
                outside.chmod(0o555)
                commands = root / "commands"
                commands.mkdir()
                calls = root / "sudo-calls.jsonl"
                changed = root / "chown-calls.jsonl"
                sudo = commands / "sudo"
                sudo.write_text(
                    "#!" + sys.executable + "\n"
                    "import json, os, sys\n"
                    "with open(os.environ['SUDO_CALLS'], 'a') as f:\n"
                    "    f.write(json.dumps(sys.argv[1:]) + '\\n')\n"
                    "if int(os.environ['SUDO_EXIT']):\n"
                    "    sys.exit(int(os.environ['SUDO_EXIT']))\n"
                    "assert sys.argv[1] == '-n'\n"
                    "os.execvp(sys.argv[2], sys.argv[2:])\n")
                sudo.chmod(0o755)
                chown = commands / "chown"
                chown.write_text(
                    "#!" + sys.executable + "\n"
                    "import json, os, sys\n"
                    "with open(os.environ['CHOWN_CALLS'], 'a') as f:\n"
                    "    f.write(json.dumps(sys.argv[1:]) + '\\n')\n")
                chown.chmod(0o755)
                selected = workspace
                pipeline = workspace.parent
                if scenario == "absent":
                    selected = work_root / "missing/missing"
                    pipeline = selected.parent
                elif scenario == "outside":
                    selected = outside
                elif scenario == "symlink":
                    shutil.rmtree(workspace)
                    workspace.symlink_to(outside, target_is_directory=True)
                elif scenario == "aliased-work-root":
                    alias = root / "work-link"
                    alias.symlink_to(work_root, target_is_directory=True)
                    selected = alias / "sonic-buildimage/sonic-buildimage"
                    pipeline = selected.parent
                repository = "missing" if scenario == "absent" else "sonic-buildimage"
                result = subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", script],
                    cwd=scratch, capture_output=True, text=True,
                    env=dict(os.environ, PATH=str(commands) + os.pathsep + os.environ["PATH"],
                             RUNNER_TEMP=str(scratch), GITHUB_WORKSPACE=str(selected),
                             RUNNER_PIPELINE_WORKSPACE=str(pipeline),
                             GITHUB_REPOSITORY="owner/" + repository,
                             SUDO_CALLS=str(calls), CHOWN_CALLS=str(changed),
                             SUDO_EXIT="23" if scenario == "sudo-failure" else "0"))
                expected_exit = {"outside": 1, "symlink": 1, "sudo-failure": 23}.get(scenario, 0)
                self.assertEqual(result.returncode, expected_exit, result.stdout + result.stderr)
                if scenario in ("repair", "separate-temp", "aliased-work-root"):
                    self.assertEqual(nested.stat().st_mode & 0o777, 0o755)
                    self.assertEqual(regular.stat().st_mode & 0o777, 0o444)
                    ownership_calls = [json.loads(line) for line in changed.read_text().splitlines()]
                    paths = [path for call in ownership_calls for path in call[3:]]
                    self.assertIn(str(regular), paths)
                    self.assertNotIn(str(linked), paths)
                    self.assertNotIn(str(workspace / "symlink"), paths)
                    self.assertTrue(all(path.startswith(str(workspace)) for path in paths))
                    self.assertEqual(len(calls.read_text().splitlines()), 3)
                elif scenario != "sudo-failure":
                    self.assertFalse(calls.exists())
                self.assertEqual(payload.read_text(), "shared cache object")
                self.assertEqual(payload.stat().st_mode & 0o777, 0o444)
                self.assertEqual(outside.stat().st_mode & 0o777, 0o555)
                outside.chmod(0o755)
                if nested.exists():
                    nested.chmod(0o755)

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

    def test_root_bazel_jobs_initialize_the_gitlinks(self):
        # Archive consumers and the small device-data workspace can operate
        # without initializing every module in the buildimage root.
        prepare_script = "bash tools/bazel/ci/prepare_image_submodules.sh"
        extra = {("bazel-oci.yml", "dhcp_relay_source_image"), ("bazel-oci.yml", "vs_container"),
                 ("bazel-kernel-bundle.yml", "kernel-bundle"), ("bazel-make-cache.yml", "native")}
        for path in WORKFLOW.parent.glob("bazel-*.yml"):
            for name, job in yaml.safe_load(path.read_text())["jobs"].items():
                steps = job.get("steps", [])
                setup = [i for i, step in enumerate(steps) if
                         step.get("uses", "").startswith("bazel-contrib/setup-bazel@")]
                prepare = [i for i, step in enumerate(steps) if
                           step.get("run", "").strip() in (prepare_script, prepare_script + " --bazel-only")
                           or step.get("with", {}).get("submodules") == "recursive"]
                root_job = bool(setup) or (path.name, name) in extra
                with self.subTest(workflow=path.name, job=name):
                    self.assertEqual(len(prepare), int(root_job))
                    if root_job:
                        self.assertTrue(any(s.get("uses", "").startswith("actions/checkout@")
                                            for s in steps[:prepare[0] + 1]))
                        if setup:
                            self.assertLess(prepare[0], setup[0])

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
            ("bazel-image-rules.yml", "native-vs-installer"),
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
            (work / "tools/bazel/submodule_paths").write_text("src/sonic-swss-common\n")
            tracked = component / "setup.py"
            tracked.write_text("original source\n")
            git(work, "init", "--quiet")
            git(work, "-c", "protocol.file.allow=always", "submodule", "add",
                str(source), "src/sonic-p4rt/sonic-pins")
            for name in ("src/sonic-sysmgr/gnoi", "src/sonic-swss-common", "src/unused"):
                git(work, "-c", "protocol.file.allow=always", "submodule", "add",
                    str(source), name)
            git(work, "add", ".")
            git(work, "commit", "--quiet", "-m", "Distribution and pinned source")
            for name in ("src/sonic-sysmgr/gnoi", "src/sonic-swss-common", "src/unused"):
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
                self.assertEqual(git(work / "src/sonic-swss-common", "rev-parse", "HEAD"),
                                 git(work, "rev-parse", "HEAD:src/sonic-swss-common"))
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
