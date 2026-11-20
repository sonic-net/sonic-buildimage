#!/usr/bin/env python3
"""Exercise the real hosted cache handoff without invoking Bazel."""

import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[3]
HELPER = "tools/bazel/ci/preserve_setup_bazel_cache.py"
OWNER = "frr" if (ROOT / "tools/bazel/ci/frr_draft_bazel.sh").is_file() else "libteam"
WRAPPER = "tools/bazel/ci/" + OWNER + "_draft_bazel.sh"
JOBS = [("bazel-base-images.yml", "native"), ("bazel-eventd.yml", "native"),
        ("bazel-" + OWNER + ".yml", "native"), ("bazel-oci.yml", "archive"),
        ("bazel-oci.yml", "checks"), ("bazel-supervisor-rust.yml", "native"),
        ("bazel-yang-models.yml", "package")]
PRIVATE_CACHE_JOBS = {
    ("bazel-oci.yml", "otel_archive"), ("bazel-oci.yml", "p4rt_native"),
    ("bazel-oci.yml", "radv_native"),
    ("bazel-oci.yml", "gearbox_archive"), ("bazel-oci.yml", "dash_engine_archive"),
    ("bazel-base-images.yml", "native"), ("bazel-eventd.yml", "native"),
    ("bazel-libteam.yml", "native"),
    ("bazel-frr.yml", "native"), ("bazel-frr-python.yml", "packages"),
    ("bazel-oci.yml", "gnmi_native"), ("bazel-oci.yml", "mgmt_framework_native"),
    ("bazel-sysmgr.yml", "native"),
    ("bazel-oci.yml", "archive"), ("bazel-supervisor-rust.yml", "native"),
    ("bazel-yang-models.yml", "package"),
}
PRIVATE_CACHE_SETTINGS = [
    "common --remote_cache=grpc://172.16.0.4:9092",
    "common --experimental_remote_downloader=grpc://172.16.0.4:9092",
    "common --experimental_remote_downloader_local_fallback",
    "build --remote_local_fallback",
    "build --incompatible_remote_local_fallback_for_remote_cache",
    "build --experimental_build_event_upload_strategy=local",
]


class SetupBazelCacheTest(unittest.TestCase):
    def test_workspace_defaults_do_not_enable_private_remote_services(self):
        for relative in (".bazelrc", "tools/bazel/common.bazelrc", "tools/bazel/kernel/.bazelrc"):
            with self.subTest(rc=relative):
                contents = "\n".join(
                    line.strip() for line in (ROOT / relative).read_text().splitlines()
                    if not line.lstrip().startswith("#")
                )
                self.assertNotIn("172.16.0.4", contents)
                self.assertNotRegex(
                    contents,
                    r"(?m)^(?:common|build|test|fetch) --(?:remote_cache|experimental_remote_downloader)(?:=|\s|$)",
                )

    def test_only_cache_reachable_jobs_opt_in_to_private_remote_services(self):
        enabled = set()
        for path in (ROOT / ".github/workflows").glob("*.yml"):
            document = yaml.safe_load(path.read_text())
            for name, job in document.get("jobs", {}).items():
                calls = [step["run"] for step in job.get("steps", [])
                         if "--private-remote-cache" in step.get("run", "")]
                if not calls:
                    continue
                with self.subTest(workflow=path.name, job=name):
                    enabled.add((path.name, name))
                    self.assertEqual(calls, ["python3 " + HELPER + " --private-remote-cache"])
                    if (path.name == "bazel-sysmgr.yml" or
                            (path.name == "bazel-oci.yml" and name in {
                                "otel_archive", "radv_native", "gearbox_archive", "dash_engine_archive",
                                "gnmi_native", "mgmt_framework_native", "p4rt_native"})):
                        labels = ["self-hosted", "linux", "bazel-x64"]
                        if name == "p4rt_native":
                            labels.append("vs")
                        self.assertEqual(job["runs-on"], labels)
                        continue
                    if path.name == "bazel-base-images.yml":
                        self.assertEqual(job["strategy"]["matrix"]["architecture"], ["amd64", "arm64"])
                        self.assertEqual(job["runs-on"], [
                            "self-hosted", "linux",
                            "${{ matrix.architecture == 'amd64' && 'bazel-x64' || 'bazel-arm64' }}",
                        ])
                        continue
                    self.assertEqual(job["runs-on"], "${{ matrix.runner }}")
                    matrix = job["strategy"]["matrix"]["include"]
                    expected = {("self-hosted", "linux", "bazel-x64")}
                    if path.name != "bazel-frr.yml":
                        expected.add(("self-hosted", "linux", "bazel-arm64"))
                    self.assertEqual(len(matrix), len(expected))
                    self.assertEqual({tuple(entry["runner"]) for entry in matrix}, expected)
        self.assertEqual(enabled, PRIVATE_CACHE_JOBS)

    def test_explicit_private_cache_preserves_local_caches_and_upload_policy(self):
        for upload in ("true", "false"):
            for repository_only in (False, True):
                with self.subTest(upload=upload, repository_only=repository_only), \
                        tempfile.TemporaryDirectory() as temporary:
                    root = Path(temporary)
                    prior = ("common --experimental_downloader_config=/verified/downloads.cfg\n"
                             "build --remote_upload_local_results=" + upload)
                    env = self.prepare(
                        root, "build --disk_cache=/disk\ncommon --repository_cache=/repo\n", prior)
                    command = ["python3", HELPER, "--private-remote-cache"]
                    if repository_only:
                        command.append("--repository-only")
                    subprocess.run(command, cwd=root, env=env, check=True)
                    expected = prior.splitlines()
                    if not repository_only:
                        expected.append("build --disk_cache=/disk")
                    expected += ["common --repository_cache=/repo", *PRIVATE_CACHE_SETTINGS]
                    self.assertEqual((root / ".bazelrc.user").read_text().splitlines(), expected)

    def test_sysmgr_passes_read_only_cache_rc_into_nested_workspace(self):
        document = yaml.safe_load((ROOT / ".github/workflows/bazel-sysmgr.yml").read_text())
        steps = document["jobs"]["native"]["steps"]
        setup = next(step for step in steps if "setup-bazel@" in step.get("uses", ""))
        handoff = next(step for step in steps if HELPER in step.get("run", ""))
        policy = next(step for step in steps
                      if step.get("name") == "Configure read-only cache and local test evidence")
        prefetch = next(step for step in steps if "github_sources.py" in step.get("run", ""))
        build = next(step for step in steps if "src/sonic-sysmgr/bazel/ci.py" in step.get("run", ""))
        self.assertEqual(setup["with"]["module-root"], "src/sonic-sysmgr")
        self.assertGreater(steps.index(handoff), steps.index(setup))
        self.assertLess(steps.index(handoff), steps.index(policy))
        self.assertLess(steps.index(policy), steps.index(prefetch))
        self.assertLess(steps.index(prefetch), steps.index(build))
        for event in ("pull_request", "push"):
            self.assertIn(HELPER, document.get("on", document.get(True))[event]["paths"])
        with tempfile.TemporaryDirectory(prefix="sysmgr cache ") as temporary:
            root = Path(temporary)
            env = self.prepare(root, "build --disk_cache=/disk\ncommon --repository_cache=/repo\n")
            env["GITHUB_WORKSPACE"] = str(root)
            for step in (handoff, policy):
                subprocess.run(["bash", "-euc", step["run"]], cwd=root, env=env, check=True)
            recorder = root / "bin/python3"
            recorder.write_text("""#!/usr/bin/python3
import json, os, sys
from pathlib import Path
Path(os.environ['RECORD']).write_text(json.dumps(sys.argv[1:]))
""")
            recorder.chmod(0o755)
            subprocess.run(["bash", "-euc", prefetch["run"]], cwd=root, env=env, check=True)
            prefetch_args = json.loads((root / "record.json").read_text())
            self.assertEqual(prefetch_args[prefetch_args.index("--distdir") + 1],
                             str(root / "sysmgr-distdir"))
            self.assertEqual((root / ".bazelrc.user").read_text().splitlines(), [
                "build --disk_cache=/disk", "common --repository_cache=/repo",
                *PRIVATE_CACHE_SETTINGS, "build --remote_upload_local_results=false",
                "build --experimental_build_event_upload_strategy=local",
                f'common --distdir="{root / "sysmgr-distdir"}"',
            ])
            subprocess.run(["bash", "-e", "-o", "pipefail", "-c", build["run"]],
                           cwd=root, env=env, check=True)
            args = json.loads((root / "record.json").read_text())
            self.assertEqual(args[:2], ["-B", "src/sonic-sysmgr/bazel/ci.py"])
            self.assertEqual([arg for arg in args if arg.startswith("--startup-arg=")],
                             ["--startup-arg=--bazelrc=" + str(root / ".bazelrc.user")])

    def test_container_jobs_repair_default_startup_state_and_keep_cache_handoff(self):
        jobs = [
            ("bazel-base-images.yml", "native", "architecture"),
            ("bazel-eventd.yml", "native", "architecture"),
            ("bazel-supervisor-rust.yml", "native", "arch"),
            ("bazel-oci.yml", "archive", "architecture"),
        ]
        repair_helper = "tools/bazel/ci/repair_bazel_startup.py"
        for filename, name, architecture in jobs:
            with self.subTest(workflow=filename), tempfile.TemporaryDirectory() as temporary:
                document = yaml.safe_load((ROOT / ".github/workflows" / filename).read_text())
                job = document["jobs"][name]
                self.assertIn("container", job)
                matrix = job["strategy"]["matrix"]
                self.assertEqual(len(matrix["architecture"] if filename == "bazel-base-images.yml"
                                     else matrix["include"]), 2)
                self.assertNotIn("BAZEL_OUTPUT_USER_ROOT", str(job))
                self.assertNotIn("--output_user_root=", str(job))
                handoff = next(step for step in job["steps"] if HELPER in step.get("run", ""))
                repairs = [step for step in job["steps"] if repair_helper in step.get("run", "")]
                self.assertEqual(len(repairs), 1)
                self.assertEqual(repairs[0]["run"], "python3 " + repair_helper)
                self.assertEqual(handoff["run"], "python3 " + HELPER + " --private-remote-cache")
                self.assertEqual(job["steps"].index(repairs[0]), job["steps"].index(handoff) + 1)
                if filename not in ("bazel-base-images.yml", "bazel-oci.yml"):
                    events = document.get("on", document.get(True))
                    for event in ("pull_request", "push"):
                        self.assertIn(repair_helper, events[event]["paths"])
                root = Path(temporary)
                env = self.prepare(root, "build --disk_cache=/disk\ncommon --repository_cache=/repo\n")
                subprocess.run(["bash", "-euc", handoff["run"]], cwd=root, env=env, check=True)
                subprocess.run(["bash", WRAPPER, "info", "output_base"], cwd=root, env=env, check=True)
                record = json.loads((root / "record.json").read_text())
                self.assertFalse(any(arg.startswith("--output_user_root=") for arg in record["argv"]))
                self.assertEqual(record["user"].splitlines(),
                                 ["build --disk_cache=/disk", "common --repository_cache=/repo",
                                  *PRIVATE_CACHE_SETTINGS])

    def test_startup_root_preserves_defaults_and_explicit_overrides(self):
        for explicit in ([], ["--output_user_root=/isolated proof", "--batch", "--output_base=/proof"]):
            with self.subTest(explicit=explicit), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                env = self.prepare(root, user="")
                command = ["bash", WRAPPER, *explicit, "info", "output_base"]
                subprocess.run(command, cwd=root, env=env, check=True)
                args = json.loads((root / "record.json").read_text())["argv"]
                self.assertEqual(args[:len(explicit)], explicit)
                roots = [arg for arg in args if arg.startswith("--output_user_root=")]
                self.assertEqual(roots, explicit[:1])

    def prepare(self, root, home_text=None, user=None):
        home = root / "home"
        home.mkdir()
        if home_text is not None:
            (home / ".bazelrc").write_text(home_text)
        if user is not None:
            (root / ".bazelrc.user").write_text(user)
        for name in (HELPER, WRAPPER, "tools/bazel/common.bazelrc"):
            destination = root / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, destination)
        binary = root / "bin"
        binary.mkdir()
        recorder = binary / "bazel"
        recorder.write_text("""#!/usr/bin/python3
import json, os, sys
from pathlib import Path
args = sys.argv[1:]
assert '--nohome_rc' in args
rc = Path(next(x.split('=', 1)[1] for x in args if x.startswith('--bazelrc=')))
assert 'try-import %workspace%/.bazelrc.user' in rc.read_text()
Path(os.environ['RECORD']).write_text(json.dumps({'argv': args, 'user': Path('.bazelrc.user').read_text()}))
""")
        recorder.chmod(0o755)
        return dict(os.environ, HOME=str(home), PATH=str(binary) + ":/usr/bin:/bin",
                    RECORD=str(root / "record.json"), BASH_ENV="/dev/null",
                    RUNNER_TEMP=str(root), CI_CPU="x86_64", CI_DISTRIBUTION="trixie")

    def test_all_seven_workflow_calls_reach_isolated_wrapper_and_direct_native(self):
        for filename, job_name in JOBS:
            with self.subTest(workflow=filename, job=job_name), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                document = yaml.safe_load((ROOT / ".github/workflows" / filename).read_text())
                job = document["jobs"][job_name]
                steps = job["steps"]
                setup = next(s for s in steps if "setup-bazel@" in s.get("uses", ""))
                handoff = [s for s in steps if HELPER in s.get("run", "")]
                self.assertEqual(len(handoff), 1)
                handoff = handoff[0]
                self.assertGreater(steps.index(handoff), steps.index(setup))
                repository_only = setup["with"].get("disk-cache") is False
                private_cache = (filename, job_name) in PRIVATE_CACHE_JOBS
                self.assertEqual(handoff["run"].strip(), "python3 " + HELPER +
                                 (" --repository-only" if repository_only else "") +
                                 (" --private-remote-cache" if private_cache else ""))
                self.assertTrue(setup["with"]["repository-cache"])
                if filename not in ("bazel-base-images.yml", "bazel-oci.yml"):
                    events = document.get("on", document.get(True))
                    for event in ("pull_request", "push"):
                        if event in events:
                            self.assertIn(HELPER, events[event]["paths"])
                home = ("build --disk_cache=/retained/disk\n"
                        "common --repository_cache=/retained/repository\n"
                        "common --remote_cache=grpc://unrelated.invalid\n"
                        "common --experimental_remote_downloader=grpc://unrelated.invalid\n"
                        "common --registry=https://unrelated.invalid\n"
                        "common --experimental_downloader_config=/unrelated/home.cfg\n"
                        "startup --output_base=/unrelated/output\n")
                prior = "common --experimental_downloader_config=/verified/transport.cfg"
                env = self.prepare(root, home, prior)
                subprocess.run(["bash", "-eu", "-c", handoff["run"]], cwd=root, env=env, check=True)
                if filename == "bazel-" + OWNER + ".yml":
                    build = next(s for s in steps if "run_bazel()" in s.get("run", ""))
                    script = build["run"]
                    options = re.search(r"(?ms)^options=\(\n.*?^\)", script).group()
                    function = re.search(r"(?ms)^run_bazel\(\) \{\n.*?^\}", script).group()
                    env["SONIC_BAZEL_REGISTRY"] = build["env"]["SONIC_BAZEL_REGISTRY"]
                    command = options + "\n" + function + "\nrun_bazel info repository_cache"
                else:
                    self.assertIn(WRAPPER, str(job))
                    command = "bash " + WRAPPER + " info repository_cache"
                subprocess.run(["bash", "-eu", "-c", command], cwd=root, env=env, check=True)
                observed = json.loads((root / "record.json").read_text())
                user = observed["user"]
                self.assertTrue(user.startswith(prior + "\n"))
                self.assertIn("common --repository_cache=/retained/repository\n", user)
                self.assertEqual("build --disk_cache=/retained/disk\n" in user, not repository_only)
                self.assertNotIn("unrelated", user)
                for setting in PRIVATE_CACHE_SETTINGS:
                    self.assertEqual(setting in user.splitlines(), private_cache)
                # Existing fresh-root verifiers can still override these defaults.
                override = ["bash", WRAPPER, "build", "--repository_cache=/private/repository",
                            "--disk_cache=/private/disk", "--remote_cache=",
                            "--experimental_remote_downloader=",
                            "//tools/bazel/tests:archive-fixture.gz"]
                subprocess.run(override, cwd=root, env=env, check=True)
                self.assertEqual(json.loads((root / "record.json").read_text())["argv"][-5:], override[-5:])

    def test_missing_duplicate_and_invalid_settings_leave_user_file_unchanged(self):
        cases = [None, "", "build --disk_cache=/disk\n",
                 "common --repository_cache=/repo\n",
                 "common --repository_cache=/one\ncommon --repository_cache=/two\nbuild --disk_cache=/disk\n",
                 "common --repository_cache=/repo\nbuild --disk_cache=/one\nbuild --disk_cache=/two\n",
                 "common --repository_cache=\nbuild --disk_cache=/disk\n",
                 "common --repository_cache=/repo --registry=https://unrelated.invalid\nbuild --disk_cache=/disk\n"]
        for settings in cases:
            with self.subTest(settings=settings), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                prior = "common --experimental_downloader_config=/verified/config\n"
                env = self.prepare(root, settings, prior)
                result = subprocess.run(["python3", HELPER, "--private-remote-cache"],
                                        cwd=root, env=env, capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual((root / ".bazelrc.user").read_text(), prior)
                self.assertFalse((root / "record.json").exists())

    def test_repository_only_and_newline_boundaries_preserve_policy(self):
        for prior in (None, "", "common --distdir=/retained/downloads", "common --distdir=/retained/downloads\n"):
            with self.subTest(prior=prior), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                env = self.prepare(root, "common --repository_cache=/repo\nbuild --disk_cache=/ignored\n", prior)
                subprocess.run(["python3", HELPER, "--repository-only"], cwd=root, env=env, check=True)
                prefix = (prior or "") + ("\n" if prior and not prior.endswith("\n") else "")
                self.assertEqual((root / ".bazelrc.user").read_text(), prefix + "common --repository_cache=/repo\n")

    def test_normal_ci_discovery_runs_this_regression(self):
        """The normal workflow discovers cache tests alongside other helper tests."""
        document = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        steps = document["jobs"]["checks"]["steps"]
        commands = [(index, step, line) for index, step in enumerate(steps)
                    for line in step.get("run", "").splitlines()
                    if "-m unittest discover -s tools/bazel/tests" in line]
        self.assertEqual(len(commands), 1)
        index, step, command = commands[0]
        self.assertNotIn("if", step)
        self.assertTrue(any("python3-yaml" in s.get("run", "") for s in steps[:index]))
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            tests = root / "tools/bazel/tests"
            tests.mkdir(parents=True)
            names = ("setup_bazel_cache_test", "kernel_test")
            for name in names:
                (tests / (name + ".py")).write_text(
                    "import unittest\nfrom pathlib import Path\n"
                    "class DiscoveryProbe(unittest.TestCase):\n"
                    "    def test_selected(self):\n"
                    "        Path(" + repr(name + ".selected") + ").touch()\n")
            subprocess.run(shlex.split(command), cwd=root, check=True, capture_output=True, text=True)
            self.assertEqual(sorted(p.stem for p in root.glob("*.selected")), sorted(names))


if __name__ == "__main__":
    unittest.main()
