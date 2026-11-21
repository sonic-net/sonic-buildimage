"""Execute source-validation workflow shells without compiling source packages."""

import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import textwrap
import unittest
import yaml


ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = ROOT / ".github/workflows/bazel-oci.yml"


def jobs():
    """Read top-level job blocks; the shell itself is executed without rewriting."""
    contents = WORKFLOW.read_text().split("\njobs:\n", 1)[1]
    return dict(re.findall(r"^  ([\w-]+):\n(.*?)(?=^  [\w-]+:\n|\Z)",
                           contents, re.MULTILINE | re.DOTALL))


def steps(job):
    return dict(re.findall(r"^      - name: ([^\n]+)\n(.*?)(?=^      - |\Z)",
                           job, re.MULTILINE | re.DOTALL))


def shell(step):
    match = re.search(r"^        run: (.*)\n?", step, re.MULTILINE)
    if match is None:
        return None
    if match[1] == "|":
        lines = []
        for line in step[match.end():].splitlines():
            if line and not line.startswith("          "):
                break
            lines.append(line)
        return textwrap.dedent("\n".join(lines))
    return match[1]


class ArchiveCachePolicyTest(unittest.TestCase):
    def test_local_bazel_cache_is_step_scoped_and_shared_package_cache_is_preserved(self):
        workflow = yaml.safe_load(WORKFLOW.read_text())
        for name, job in workflow["jobs"].items():
            with self.subTest(job=name):
                for value in job.get("env", {}).values():
                    self.assertNotRegex(str(value), r"\$\{\{\s*runner\.")
        job = workflow["jobs"]["vs_container"]
        configure = next(step for step in job["steps"]
                         if step.get("name") == "Configure persistent caches")
        self.assertNotIn("SONIC_BAZEL_CACHE_SOURCE", job["env"])
        self.assertEqual(configure["env"], {
            "SONIC_BAZEL_CACHE_SOURCE": "${{ runner.workspace }}/vs-cache/bazel"})
        self.assertEqual(job["env"]["SONIC_DPKG_CACHE_SOURCE"], "/nfs/dpkg_cache/vs")
        with tempfile.TemporaryDirectory(prefix="vs cache paths ") as temporary:
            directory = Path(temporary)
            (directory / "artifacts/vs").mkdir(parents=True)
            shared_cache = directory / "nfs/dpkg_cache/vs"
            shared_cache.mkdir(parents=True)
            sentinel = shared_cache / "existing-package.tgz"
            sentinel.write_bytes(b"retained shared package")
            shared_cache.chmod(0o555)
            before = shared_cache.stat()
            commands = directory / "commands"
            commands.mkdir()
            python = commands / "python3"
            python.write_text("#!/bin/sh\nprintf '%s\\n' \"$@\" > source-arguments.txt\n")
            python.chmod(0o755)
            expected = {name: value.replace("${{ runner.workspace }}", temporary)
                        for name, value in configure["env"].items()}
            expected["SONIC_DPKG_CACHE_SOURCE"] = str(shared_cache)
            rc = directory / ".bazelrc.user"
            rc.write_text("common --color=no")
            environment_file = directory / "github-env"
            result = subprocess.run(
                ["bash", "-c", configure["run"]], cwd=directory,
                env=dict(os.environ, **expected, GITHUB_ENV=str(environment_file),
                         PATH=str(commands) + os.pathsep + os.environ["PATH"]),
                text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(rc.read_text(), "common --color=no")
            self.assertNotIn("--host_jvm_args", job["env"]["BAZEL_CONTAINER_ARGS"])
            self.assertNotIn("--local_startup_timeout_secs", job["env"]["BAZEL_CONTAINER_ARGS"])
            exported = dict(line.split("=", 1) for line in environment_file.read_text().splitlines())
            self.assertEqual(exported, expected)
            self.assertTrue(Path(exported["SONIC_BAZEL_CACHE_SOURCE"]).is_dir())
            after = shared_cache.stat()
            self.assertEqual((before.st_mode, before.st_uid, before.st_gid, before.st_mtime_ns),
                             (after.st_mode, after.st_uid, after.st_gid, after.st_mtime_ns))
            self.assertEqual(sentinel.read_bytes(), b"retained shared package")
            self.assertFalse((directory / "vs-cache/packages").exists())
            arguments = (directory / "source-arguments.txt").read_text().splitlines()
            self.assertEqual(arguments[-2:], ["--output", expected["SONIC_BAZEL_CACHE_SOURCE"] + "/distdir"])
        configure_index = job["steps"].index(configure)
        build_index = next(index for index, step in enumerate(job["steps"])
                           if step.get("name") == "Prepare the host-driven SONiC slave")
        self.assertLess(configure_index, build_index)

    def test_host_installer_is_isolated_and_preserves_input_and_output_contracts(self):
        job = yaml.safe_load(WORKFLOW.read_text())["jobs"]["vs_container"]
        self.assertNotIn("concurrency", job)
        self.assertNotIn("container", job)
        self.assertNotIn("EXPECTED_MANAGER_REVISION", job["env"])
        self.assertEqual(job["env"]["BAZEL_REMOTE_AUTH"], "")
        self.assertEqual(job["env"]["SONIC_BAZEL_REMOTE_CACHE"], "grpc://172.16.0.4:9092")
        self.assertEqual(
            job["env"]["SONIC_BAZEL_REMOTE_UPLOAD"],
            "${{ github.event_name == 'push' && github.ref == 'refs/heads/master' && 'y' || 'n' }}")
        self.assertEqual(job["env"]["SONIC_BUILD_JOBS"], "1")
        for flag in (
            "--remote_local_fallback", "--incompatible_remote_local_fallback_for_remote_cache",
            "--experimental_remote_downloader=grpc://172.16.0.4:9092",
            "--experimental_remote_downloader_local_fallback",
        ):
            self.assertIn(flag, job["env"]["BAZEL_CONTAINER_ARGS"])
        checkout = job["steps"][0]["with"]
        directory = "vs-${{ github.run_id }}-${{ github.run_attempt }}"
        self.assertEqual(checkout["path"], directory)
        self.assertTrue(checkout["submodules"])
        self.assertEqual(job["defaults"]["run"]["working-directory"], directory)
        selected = {step.get("name"): step for step in job["steps"]}
        self.assertIn("--host-agent", selected["Check host-agent build prerequisites"]["run"])
        self.assertIn("--build-target sonic-vs.bin",
                      selected["Check host-agent build prerequisites"]["run"])
        scripts = "\n".join(step.get("run", "") for step in job["steps"])
        self.assertNotIn("prepare_vs_slave.sh", scripts)
        self.assertNotIn("sonic-vs-slave.env", scripts)
        self.assertNotIn("configure_runner_cache.py", scripts)
        self.assertNotIn("check_native_vs_storage.py", scripts)
        self.assertNotIn("/actions/bazel", scripts)
        self.assertNotIn("SONIC_BAZEL_REMOTE_ONLY_RC", job["env"])
        self.assertIn("bash tools/bazel/ci/build_vs.sh", scripts)
        self.assertNotIn("validate_vs.sh", scripts)
        self.assertNotIn("verify-installed-images.sh", scripts)
        self.assertIn('label=$SONIC_CI_CONTAINER_LABEL', scripts)
        self.assertNotIn("docker system prune", scripts)
        self.assertIn("P4LANG_SOURCE_BASE_URL=file:/sonic/artifacts/vs/p4-source-inputs/sources",
                      job["env"]["SONIC_BUILD_VARS"])
        self.assertIn("--manifest src/p4lang/source-inputs.json",
                      selected["Verify P4 source bytes before native Make"]["run"])
        for step in job["steps"]:
            if "download-artifact@" in step.get("uses", ""):
                self.assertTrue(step["with"]["path"].startswith(directory + "/"))
            if "upload-artifact@" in step.get("uses", ""):
                for path in step["with"]["path"].splitlines():
                    self.assertTrue(path.startswith(directory + "/"), path)
        installer_paths = selected["Publish the installer and matching container archives"]["with"]["path"]
        for output in ("sonic-vs.bin", "sonic-vs.bin.dockers", "docker-orchagent.gz",
                       "docker-orchagent-dbg.gz", "docker-syncd-vs.gz", "docker-syncd-vs-dbg.gz"):
            self.assertIn("/target/" + output, installer_paths)

    def test_host_cleanup_is_scoped_and_handles_already_removed_containers(self):
        job = yaml.safe_load(WORKFLOW.read_text())["jobs"]["vs_container"]
        cleanup = next(step for step in job["steps"]
                       if step.get("name") == "Remove only this job's build containers")
        self.assertEqual(cleanup["working-directory"], "${{ github.workspace }}")
        self.assertEqual(cleanup["if"], "always()")
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            docker = directory / "docker"
            docker.write_text(f"#!{sys.executable}\n" + textwrap.dedent('''\
                import os, sys
                args = sys.argv[1:]
                if args[0] == "ps":
                    assert "label=fixture-vs" in args, args
                    if os.environ["CASE"] == "daemon-error":
                        sys.exit(17)
                    if "id=fixture-container" in args:
                        if os.environ["CASE"] == "remove-error":
                            print("fixture-container")
                    elif os.environ["CASE"] != "empty":
                        print("fixture-container")
                else:
                    assert args == ["rm", "--force", "fixture-container"], args
                    if os.environ["CASE"] in ("remove-error", "already-removed"):
                        sys.exit(1)
                '''))
            docker.chmod(0o755)
            for case, expected in (("empty", 0), ("removed", 0), ("already-removed", 0),
                                   ("remove-error", 1), ("daemon-error", 17)):
                with self.subTest(case=case):
                    result = subprocess.run(
                        ["bash", "-c", cleanup["run"]], cwd=directory,
                        env=dict(os.environ, PATH=str(directory) + os.pathsep + os.environ["PATH"],
                                 SONIC_CI_CONTAINER_LABEL="fixture-vs", CASE=case),
                        text=True, capture_output=True)
                    self.assertEqual(result.returncode, expected, result.stdout + result.stderr)

    def test_checkout_cleanup_follows_uploads_and_requires_successful_container_cleanup(self):
        job = yaml.safe_load(WORKFLOW.read_text())["jobs"]["vs_container"]
        cleanup = job["steps"][-1]
        containers = next(step for step in job["steps"]
                          if step.get("id") == "vs_container_cleanup")
        self.assertEqual(containers["id"], "vs_container_cleanup")
        self.assertLess(job["steps"].index(containers), job["steps"].index(cleanup))
        self.assertEqual(cleanup["name"], "Remove only this run's checkout")
        self.assertIn("always() && steps.vs_container_cleanup.outcome == 'success'", cleanup["if"])
        for upload in ("vs_receipts_upload", "vs_installer_upload"):
            self.assertIn(f"steps.{upload}.outcome == 'success'", cleanup["if"])
            self.assertTrue(any(step.get("id") == upload and "upload-artifact@" in step.get("uses", "")
                                for step in job["steps"][:job["steps"].index(containers)]))
        self.assertIn("steps.vs_installer_upload.outcome == 'skipped'", cleanup["if"])
        self.assertEqual(cleanup["working-directory"], "${{ github.workspace }}")
        self.assertIn("cleanup_vs_workspace.py", cleanup["run"])
        self.assertIn('--workspace "$GITHUB_WORKSPACE"', cleanup["run"])
        self.assertIn('--run-id "$GITHUB_RUN_ID" --run-attempt "$GITHUB_RUN_ATTEMPT"', cleanup["run"])
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            environment = dict(os.environ, GITHUB_WORKSPACE=str(directory),
                               GITHUB_RUN_ID="123", GITHUB_RUN_ATTEMPT="2")
            for redirected in (False, True):
                with self.subTest(redirected=redirected):
                    checkout = directory / "vs-123-2"
                    if redirected:
                        checkout.symlink_to(directory, target_is_directory=True)
                    result = subprocess.run(
                        ["bash", "-c", cleanup["run"]], cwd=directory,
                        env=environment, text=True, capture_output=True)
                    self.assertEqual(result.returncode, 1 if redirected else 0,
                                     result.stdout + result.stderr)

    def test_upload_override_preserves_checkout_cleanliness(self):
        step = steps(jobs()["archive"])["Configure remote cache uploads"]
        self.assertNotRegex(step, r"(?m)^\s+if:")
        self.assertIn("github.event_name == 'push' && github.ref == 'refs/heads/master'", step)
        self.assertIn("try-import %workspace%/.bazelrc.user",
                      (ROOT / "tools/bazel/common.bazelrc").read_text())
        with tempfile.TemporaryDirectory(prefix="archive-cache-policy-") as directory:
            root = Path(directory)
            for name in (".bazelrc", ".gitignore"):
                shutil_source = ROOT / name
                (root / name).write_bytes(shutil_source.read_bytes())
            subprocess.run(["git", "init", "-q", directory], check=True)
            subprocess.run(["git", "-C", directory, "add", ".bazelrc", ".gitignore"], check=True)
            for upload in ("true", "false"):
                with self.subTest(upload=upload):
                    result = subprocess.run(
                        ["bash", "-e", "-o", "pipefail", "-c", shell(step)],
                        cwd=root, env=dict(os.environ, UPLOAD_CACHE=upload),
                        capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertEqual((root / ".bazelrc.user").read_text().splitlines()[-1],
                                     "build --remote_upload_local_results=" + upload)
                    self.assertEqual((root / ".bazelrc").read_bytes(),
                                     (ROOT / ".bazelrc").read_bytes())
                    subprocess.run(["git", "-C", directory, "diff", "--exit-code"], check=True)
                    ignored = subprocess.check_output(
                        ["git", "-C", directory, "check-ignore", ".bazelrc.user"], text=True)
                    self.assertEqual(ignored.strip(), ".bazelrc.user")
                    untracked = subprocess.check_output(
                        ["git", "-C", directory, "ls-files", "--others", "--exclude-standard"], text=True)
                    self.assertEqual(untracked, "")


class SourceWorkflowTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="source-workflow-")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.commands = self.directory / "commands"
        self.commands.mkdir()
        (self.directory / "artifacts/vs").mkdir(parents=True)
        for name in (".bazelversion", "tools/bazel/build_helpers.py",
                     "tools/bazel/ci/prepare_image_submodules.sh",
                     "tools/bazel/cache/credentials.py",
                     "platform/vs/docker-syncd-vs/BUILD.bazel"):
            fixture = self.directory / name
            fixture.parent.mkdir(parents=True, exist_ok=True)
            fixture.write_bytes((ROOT / name).read_bytes())
        self.calls = self.directory / "calls.jsonl"
        recorder = "#!" + sys.executable + "\n" + textwrap.dedent(r'''
            import json, os, shlex, subprocess, sys
            from pathlib import Path
            name = Path(sys.argv[0]).name
            args = sys.argv[1:]
            with Path(os.environ['CALLS']).open('a') as output:
                output.write(json.dumps({'name': name, 'args': args,
                    'environment': {key: os.environ.get(key) for key in
                                    ('BAZEL', 'BAZEL_CONTAINER_ARGS')}}) + '\n')
            if name == 'rm' or 'clean' in args or 'cleanall' in args:
                raise SystemExit('Source validation must preserve existing build state')
            source_runner = name == 'python3' and 'tools/bazel/ci/container.py' in args
            if os.environ.get('HOSTED_CHECKS') == '1':
                if source_runner or (name == 'bazel' and any(arg in ('build', 'test') for arg in args)):
                    raise SystemExit('Hosted regression checks must not compile source packages')
            if name == 'make' and 'sonic-slave-run' in args:
                if int(os.environ.get('MAKE_EXIT', '0')):
                    raise SystemExit(int(os.environ['MAKE_EXIT']))
                values = dict(arg.split('=', 1) for arg in args if '=' in arg)
                # Model Docker's explicit -e forwarding instead of inheriting
                # the outer workflow's Bazel settings by accident.
                environment = dict(os.environ)
                for key in ('BAZEL', 'BAZEL_CONTAINER_ARGS'):
                    environment.pop(key, None)
                extra = shlex.split(values.get('SONIC_BUILDER_EXTRA_CMDLINE', ''))
                for index, arg in enumerate(extra):
                    if arg == '-e':
                        key, separator, value = extra[index + 1].partition('=')
                        environment[key] = value if separator else os.environ[key]
                raise SystemExit(subprocess.run(
                    ['bash', '-e', '-o', 'pipefail', '-c', values['SONIC_RUN_CMDS']],
                    env=environment).returncode)
            if source_runner:
                if (os.environ.get('SOURCE_EXIT', '0') == '0' and
                        args[args.index('--container') + 1] == '//platform/vs/docker-syncd-vs'):
                    artifacts = Path(args[args.index('--artifacts') + 1])
                    artifacts.mkdir(parents=True, exist_ok=True)
                    (artifacts / 'apt.lock.json').write_text(json.dumps({'shared_lock': True}) + chr(10))
                raise SystemExit(int(os.environ.get('SOURCE_EXIT', '0')))
            if name == 'python3' and args[:1] == ['tools/bazel/build_helpers.py']:
                raise SystemExit(subprocess.run([sys.executable, *args]).returncode)
            if name == 'bazel' and args == ['--version']:
                print('bazel ' + Path('.bazelversion').read_text().strip())
            if name == 'git':
                if args[:2] == ['rev-parse', 'HEAD']:
                    print('a' * 40)
                elif args[:1] == ['check-ignore']:
                    print('MODULE.bazel.lock')
            ''')
        for name in ("make", "python3", "bazel", "apt-get", "git", "df", "du", "rm", "cp"):
            executable = self.commands / name
            executable.write_text(recorder)
            executable.chmod(0o755)
        self.environment = dict(
            os.environ, PATH=str(self.commands) + os.pathsep + os.environ["PATH"],
            CALLS=str(self.calls), BAZEL=str(self.commands / "bazel"),
            BAZEL_CONTAINER_ARGS="--distdir='/bazel_cache/source downloads' --remote_download_all",
            BAZEL_CONTAINER_CACHE_DIR=str(self.directory / "build-cache"),
            GITHUB_WORKSPACE=str(self.directory), GITHUB_OUTPUT=str(self.directory / "github-output"),
            GITHUB_STEP_SUMMARY=str(self.directory / "github-summary"),
            SONIC_CI_CONTAINER_LABEL="test-source-validation=one-job",
        )

    def execute(self, script, **environment):
        return subprocess.run(["bash", "-e", "-o", "pipefail", "-c", script],
                              cwd=self.directory, env=dict(self.environment, **environment),
                              capture_output=True, text=True)

    def recorded(self):
        return [json.loads(line) for line in self.calls.read_text().splitlines()]

    def test_regressions_gate_vs_without_source_compilation(self):
        workflow_jobs = jobs()
        self.assertNotIn("source", workflow_jobs)
        self.assertNotIn("syncd-contracts", workflow_jobs)
        checks = workflow_jobs["checks"]
        self.assertIn("name: OCI regression checks (AMD64)", checks)
        self.assertIn("runs-on: [self-hosted, Linux, X64, ubuntu-x64]", checks)
        self.assertRegex(workflow_jobs["vs_container"], r"(?m)^    needs: (?:checks|\[checks\])$")
        for name, step in steps(checks).items():
            script = shell(step)
            if script is not None:
                with self.subTest(step=name):
                    result = self.execute(script, HOSTED_CHECKS="1")
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = self.recorded()
        self.assertTrue(any(call["name"] == "python3" and
                            "unittest" in call["args"] for call in calls))
        self.assertFalse(any("tools/bazel/ci/container.py" in call["args"] or
                             (call["name"] == "bazel" and
                              any(arg in ("build", "test") for arg in call["args"]))
                             for call in calls))


if __name__ == "__main__":
    unittest.main()
