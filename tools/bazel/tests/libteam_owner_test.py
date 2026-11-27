"""Check the image consumer's shared libteam boundary without building it."""

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
WORKFLOW = ROOT / ".github/workflows/bazel-libteam.yml"


class LibteamOwnerTest(unittest.TestCase):
    def test_native_workflow_runs_the_owner_test_with_uncached_results(self):
        workflow = yaml.safe_load(WORKFLOW.read_text())
        step = next(step for step in workflow["jobs"]["native"]["steps"]
                    if step.get("name") == "Build source outputs and execute native tests")
        setup = re.search(r'(?ms)^options=\(.*?^\}', step["run"])[0]
        invocation = re.search(r'(?ms)^ *run_bazel test .*?tee artifacts/test.log', step["run"])[0]
        with tempfile.TemporaryDirectory(prefix="owner native test ") as temporary:
            work = Path(temporary)
            (work / "artifacts").mkdir()
            commands = work / "bin"
            commands.mkdir()
            calls = work / "commands.jsonl"
            recorder = commands / "bazel"
            recorder.write_text('''#!/usr/bin/python3
import json, os, sys
with open(os.environ["CALLS"], "a") as output:
    output.write(json.dumps(sys.argv[1:]) + "\\n")
''')
            recorder.chmod(0o755)
            environment = dict(os.environ, PATH=str(commands) + ":/usr/bin:/bin",
                               CALLS=str(calls), SONIC_BAZEL_REGISTRY="https://example.invalid/review",
                               CI_CPU="x86_64", CI_DISTRIBUTION="trixie", RUNNER_TEMP=temporary)
            subprocess.run(["bash", "-euo", "pipefail", "-c", setup + "\n" + invocation],
                           cwd=work, env=environment, check=True, capture_output=True, text=True)
            recorded = [json.loads(line) for line in calls.read_text().splitlines()]
            self.assertEqual(len(recorded), 1)
            command = recorded[0]
            self.assertIn("test", command)
            self.assertIn("@libteam//:consumer_test", command)
            self.assertIn("@libteam//:lacp_port_removal_test", command)
            self.assertNotIn("//src/libteam:consumer_test", command)
            self.assertIn("--nocache_test_results", command)
            self.assertIn("--build_event_json_file=" + str(work / "artifacts/test-events.json"), command)
            self.assertIn("--host_platform=@sonic_build_infra//platforms:x86_64_trixie", command)
            self.assertIn("--platforms=@sonic_build_infra//platforms:x86_64_trixie", command)
            self.assertIn("--experimental_downloader_config=" + temporary + "/libteam-source-mirrors.cfg", command)
            self.assertIn("--nohome_rc", command)

    def test_compatibility_labels_only_forward_owner_providers(self):
        aliases = {}
        namespace = {"package": lambda **kwargs: None,
                     "alias": lambda name, actual: aliases.setdefault(name, actual)}
        exec((ROOT / "src/libteam/BUILD.bazel").read_text(), namespace)
        self.assertEqual(len(aliases), 19)
        self.assertTrue(all(actual == "@libteam//:" + name for name, actual in aliases.items()))
        required = {"libteam", "libteamdctl", "consumer_test", "runtime_packages.json", "libteam_dev"}
        for package in ("libteam", "libteamdctl", "libteam_utils"):
            required.update((package + "_runtime", package + "_runtime.debug_symbols"))
        self.assertTrue(required <= aliases.keys())
        # The old root-owned producer and its inert edit probe must not coexist.
        for filename in ("deps.MODULE.bazel", "libteam.BUILD.bazel", "config.h.in", "verify_rebuild.py"):
            self.assertFalse((ROOT / "src/libteam" / filename).exists(), filename)

    def test_probe_uses_the_same_wrapper_execution_root_and_preserves_flags(self):
        for cpu in ("x86_64", "aarch64"):
            with self.subTest(cpu=cpu):
                self._check_probe_caller(cpu)

    def _check_probe_caller(self, cpu):
        workflow = yaml.safe_load(WORKFLOW.read_text())
        step = next(step for step in workflow["jobs"]["native"]["steps"]
                    if step.get("name") == "Build source outputs and execute native tests")
        block = re.search(r'(?ms)^ *probe_bazel=.*?--output artifacts/rebuild', step["run"])[0]
        with tempfile.TemporaryDirectory(prefix="owner probe caller ") as temporary:
            work = Path(temporary) / "image checkout"
            work.mkdir()
            wrapper = work / "tools/bazel/ci/libteam_draft_bazel.sh"
            wrapper.parent.mkdir(parents=True)
            shutil.copyfile(ROOT / wrapper.relative_to(work), wrapper)
            wrapper.chmod(0o755)
            output_base = Path(temporary) / "Bazel output base"
            execution = output_base / "execroot/_main"
            probe = execution / "external/libteam+/bazel/verify_rebuild.py"
            commands = Path(temporary) / "bin"
            commands.mkdir()
            calls = Path(temporary) / "commands.jsonl"
            for command in ("bazel", "python3"):
                recorder = commands / command
                recorder.write_text('''#!/usr/bin/python3
import json, os, pathlib, sys
with open(os.environ["CALLS"], "a") as output:
    output.write(json.dumps([pathlib.Path(sys.argv[0]).name, *sys.argv[1:]]) + "\\n")
if pathlib.Path(sys.argv[0]).name == "bazel":
    if "build" in sys.argv:
        assert sys.argv[-1] == "@libteam//:bazel/verify_rebuild.py"
        probe = pathlib.Path(os.environ["EXPECTED_PROBE"])
        probe.parent.mkdir(parents=True, exist_ok=True)
        probe.touch()
    elif "info" in sys.argv:
        assert sys.argv[-1] == "output_base"
        print(os.environ["OUTPUT_BASE"])
    elif "cquery" in sys.argv:
        print(os.environ["PROBE_RESULT"])
''')
                recorder.chmod(0o755)
            environment = dict(os.environ, PATH=str(commands) + ":/usr/bin:/bin",
                               CALLS=str(calls), OUTPUT_BASE=str(output_base), EXPECTED_PROBE=str(probe),
                               CI_CPU=cpu, CI_DISTRIBUTION="trixie", RUNNER_TEMP=temporary)
            for result in ("external/libteam+/bazel/verify_rebuild.py", str(probe)):
                with self.subTest(path=result):
                    calls.unlink(missing_ok=True)
                    subprocess.run(["bash", "-eu", "-c", block], cwd=work,
                                   env={**environment, "PROBE_RESULT": result}, check=True,
                                   capture_output=True, text=True)
                    recorded = [json.loads(line) for line in calls.read_text().splitlines()]
                    self.assertEqual(len(recorded), 4)
                    materialize = recorded.pop(0)
                    self.assertIn("build", materialize)
                    self.assertEqual(materialize[-1], "@libteam//:bazel/verify_rebuild.py")
                    self.assertIn("info", recorded[0])
                    self.assertEqual(recorded[0][-1], "output_base")
                    self.assertIn("@libteam//:bazel/verify_rebuild.py", recorded[1])
                    self.assertEqual(recorded[2][1:3], ["-B", str(probe)])
                    self.assertEqual(recorded[2][3:5], ["--bazel", str(wrapper)])
                    flags = [f"--host_platform=@sonic_build_infra//platforms:{cpu}_trixie",
                             f"--platforms=@sonic_build_infra//platforms:{cpu}_trixie",
                             "--experimental_downloader_config=" + temporary + "/libteam-source-mirrors.cfg"]
                    for flag in flags:
                        self.assertTrue(all(flag in call for call in recorded[:2]))
                        self.assertIn("--bazel-arg=" + flag, recorded[2])
                    self.assertTrue(all("--nohome_rc" in call for call in recorded[:2]))
            # The lookup must fail before cquery or Python when the pinned
            # Bzlmod layout is absent, rather than falling back to another base.
            for invalid in ("", "relative/base", str(Path(temporary) / "missing"),
                            str(output_base) + "\n" + str(output_base)):
                with self.subTest(output_base=invalid):
                    calls.unlink(missing_ok=True)
                    failed = subprocess.run(["bash", "-eu", "-c", block], cwd=work,
                        env={**environment, "OUTPUT_BASE": invalid,
                             "PROBE_RESULT": "external/libteam+/bazel/verify_rebuild.py"},
                        capture_output=True, text=True)
                    self.assertNotEqual(failed.returncode, 0)
                    recorded = [json.loads(line) for line in calls.read_text().splitlines()]
                    self.assertEqual(len(recorded), 2)
                    self.assertIn("info", recorded[-1])
                    self.assertIn("Bazel", failed.stderr)
            for result in ("", "missing.py", "one.py\ntwo.py"):
                with self.subTest(invalid=result):
                    calls.unlink(missing_ok=True)
                    failed = subprocess.run(["bash", "-eu", "-c", block], cwd=work,
                                            env={**environment, "PROBE_RESULT": result},
                                            capture_output=True, text=True)
                    self.assertNotEqual(failed.returncode, 0)
                    self.assertNotIn('"python3"', calls.read_text())

    def test_draft_wrapper_forwards_startup_jvm_and_output_base_options(self):
        wrapper = ROOT / "tools/bazel/ci/libteam_draft_bazel.sh"
        with tempfile.TemporaryDirectory(prefix="draft startup flags ") as temporary:
            root = Path(temporary)
            calls = root / "calls.jsonl"
            recorder = root / "bazel"
            recorder.write_text(
                "#!/usr/bin/python3\nimport json, os, sys\n"
                "with open(os.environ['CALLS'], 'a') as out:\n"
                "    out.write(json.dumps(sys.argv[1:]) + '\\n')\n"
                "if sys.argv[1:] == ['--version']:\n"
                "    print('bazel 8.5.1')\n"
            )
            recorder.chmod(0o755)
            startup = ["--output_base=" + str(root / "output"), "--host_jvm_args=-Xmx2g"]
            environment = dict(os.environ, PATH=str(root) + ":/usr/bin:/bin", CALLS=str(calls))
            for command in ("query", "build", "cquery", "test", "shutdown"):
                subprocess.run([str(wrapper), *startup, command], cwd=ROOT, env=environment, check=True)
            recorded = [json.loads(line) for line in calls.read_text().splitlines()]
            self.assertEqual(len(recorded), 5)
            for command, arguments in zip(("query", "build", "cquery", "test", "shutdown"), recorded):
                self.assertEqual(arguments[:len(startup)], startup)
                self.assertLess(arguments.index("--nohome_rc"), arguments.index(command))


if __name__ == "__main__":
    unittest.main()
