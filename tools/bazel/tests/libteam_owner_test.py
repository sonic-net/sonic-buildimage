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
        invocation = re.search(r'(?ms)^run_bazel test .*?tee artifacts/test.log', step["run"])[0]
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
            probe.parent.mkdir(parents=True)
            probe.write_text("# selected owner source\n")
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
    if "info" in sys.argv:
        assert sys.argv[-1] == "output_base"
        print(os.environ["OUTPUT_BASE"])
    elif "cquery" in sys.argv:
        print(os.environ["PROBE_RESULT"])
''')
                recorder.chmod(0o755)
            environment = dict(os.environ, PATH=str(commands) + ":/usr/bin:/bin",
                               CALLS=str(calls), OUTPUT_BASE=str(output_base),
                               CI_CPU=cpu, CI_DISTRIBUTION="trixie", RUNNER_TEMP=temporary)
            for result in ("external/libteam+/bazel/verify_rebuild.py", str(probe)):
                with self.subTest(path=result):
                    calls.unlink(missing_ok=True)
                    subprocess.run(["bash", "-eu", "-c", block], cwd=work,
                                   env={**environment, "PROBE_RESULT": result}, check=True,
                                   capture_output=True, text=True)
                    recorded = [json.loads(line) for line in calls.read_text().splitlines()]
                    self.assertEqual(len(recorded), 3)
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
                    self.assertEqual(len(recorded), 1)
                    self.assertIn("info", recorded[0])
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
        oci_workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        step = next(item for item in oci_workflow["jobs"]["archive"]["steps"]
                    if item.get("name") == "Validate Orchagent owner packages and original Scapy tests")
        heredoc = re.search(r"cat > \"\$owner_bazel\" <<'SH'\n(.*?)\n\s*SH", step["run"], re.S)[1]
        import textwrap
        heredoc = textwrap.dedent(heredoc)
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
            owner_output = root / "orchagent-owner-output.abc12345"
            owner_output.mkdir()
            owner_bazel = root / "orchagent-owner-bazel.abc12345"
            owner_bazel.write_text(heredoc + "\n")
            owner_bazel.chmod(0o755)
            base_env = {k: v for k, v in os.environ.items() if k != "OWNER_OUTPUT"}
            base_env.update(PATH=str(root) + ":/usr/bin:/bin",
                            CALLS=str(calls), BAZEL=str(wrapper), RUNNER_TEMP=str(root))
            missing = subprocess.run(["bash", str(owner_bazel), "query", "//..."],
                                     cwd=ROOT, env=base_env, capture_output=True, text=True)
            self.assertNotEqual(missing.returncode, 0)
            env = dict(base_env, OWNER_OUTPUT=str(owner_output))
            for args in (["--version"], ["query", "//..."], ["build", "//dockers/docker-orchagent:ci_artifacts"],
                         ["cquery", "//dockers/docker-orchagent:ci_artifacts"],
                         ["test", "//dockers/docker-orchagent:ci_tests"]):
                subprocess.run(["bash", str(owner_bazel), *args], cwd=ROOT, env=env,
                               capture_output=True, text=True, check=True)
            recorded = [json.loads(line) for line in calls.read_text().splitlines()]
            self.assertEqual(recorded[0], ["--version"])
            # query: no pre-shutdown, no build/test extra flags
            self.assertIn("query", recorded[1])
            self.assertEqual(recorded[1][:2], [f"--output_base={owner_output}", "--host_jvm_args=-Xmx6g"])
            self.assertNotIn("--noexecution_log_sort", recorded[1])
            self.assertNotIn("--experimental_build_event_upload_strategy=local", recorded[1])
            # build: pre-shutdown with matching startup flags + build with --noexecution_log_sort and local BEP upload
            self.assertEqual(recorded[2][-1], "shutdown")
            self.assertEqual(recorded[2][0], f"--output_base={owner_output}")
            self.assertIn("--host_jvm_args=-Xmx6g", recorded[2])
            self.assertIn("--nohome_rc", recorded[2])
            build_call = recorded[3]
            self.assertEqual(build_call[:2], [f"--output_base={owner_output}", "--host_jvm_args=-Xmx6g"])
            self.assertLess(build_call.index("--host_jvm_args=-Xmx6g"), build_call.index("build"))
            self.assertIn("--noexecution_log_sort", build_call)
            self.assertIn("--experimental_build_event_upload_strategy=local", build_call)
            self.assertLess(build_call.index("build"), build_call.index("--noexecution_log_sort"))
            self.assertLess(build_call.index("--noexecution_log_sort"),
                            build_call.index("--experimental_build_event_upload_strategy=local"))
            # cquery: no pre-shutdown, no build/test extra flags
            self.assertIn("cquery", recorded[4])
            self.assertNotIn("--noexecution_log_sort", recorded[4])
            self.assertNotIn("--experimental_build_event_upload_strategy=local", recorded[4])
            # test: pre-shutdown + test with local BEP upload and without --noexecution_log_sort
            self.assertEqual(recorded[5][-1], "shutdown")
            self.assertEqual(recorded[5][0], f"--output_base={owner_output}")
            test_call = recorded[6]
            self.assertIn("test", test_call)
            self.assertNotIn("--noexecution_log_sort", test_call)
            self.assertIn("--experimental_build_event_upload_strategy=local", test_call)
            self.assertLess(test_call.index("test"),
                            test_call.index("--experimental_build_event_upload_strategy=local"))


if __name__ == "__main__":
    unittest.main()
