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


class SourceWorkflowTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="source-workflow-")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.commands = self.directory / "commands"
        self.commands.mkdir()
        (self.directory / "artifacts/vs").mkdir(parents=True)
        for name in (".bazelversion", "tools/bazel/build_helpers.py",
                     "platform/vs/docker-syncd-vs/bazel/apt.lock.json",
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

    def test_hosted_regressions_gate_vs_without_source_compilation(self):
        workflow_jobs = jobs()
        self.assertNotIn("source", workflow_jobs)
        self.assertNotIn("syncd-contracts", workflow_jobs)
        checks = workflow_jobs["checks"]
        self.assertIn("name: OCI regression checks (AMD64)", checks)
        self.assertIn("runs-on: ubuntu-24.04", checks)
        self.assertRegex(workflow_jobs["vs"], r"(?m)^    needs: (?:checks|\[checks\])$")
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

    def test_make_source_validation_reuses_outputs_and_propagates_failures(self):
        workflow_steps = steps(jobs()["vs"])
        names = list(workflow_steps)
        families = (
            ("SWSS", "//dockers/docker-orchagent", "rules/docker-orchagent.mk", "artifacts/swss", False),
            ("Syncd", "//platform/vs/docker-syncd-vs", "platform/vs/docker-syncd-vs.mk", "artifacts/syncd-vs/bazel", True),
        )
        lock = self.directory / "MODULE.bazel.lock"
        lock.write_text("already-resolved Make build\n")
        for family, owner, source, artifacts, dash in families:
            name = "Validate " + family + " source packages from the Make build"
            self.assertLess(names.index("Build the VS image and both OCI container variants"), names.index(name))
            for source_exit, make_exit in ((0, 0), (19, 0), (0, 23)):
                with self.subTest(family=family, source_exit=source_exit, make_exit=make_exit):
                    self.calls.unlink(missing_ok=True)
                    result = self.execute(shell(workflow_steps[name]),
                                          SOURCE_EXIT=str(source_exit), MAKE_EXIT=str(make_exit))
                    self.assertEqual(result.returncode, source_exit or make_exit,
                                     result.stdout + result.stderr)
                    self.assertEqual(lock.read_text(), "already-resolved Make build\n")
                    calls = self.recorded()
                    make = [call for call in calls if call["name"] == "make"]
                    self.assertEqual(len(make), 1)
                    self.assertEqual(make[0]["args"][:2], ["-f", "Makefile.work"])
                    self.assertIn("BLDENV=trixie", make[0]["args"])
                    self.assertIn("sonic-slave-run", make[0]["args"])
                    values = dict(arg.split("=", 1) for arg in make[0]["args"] if "=" in arg)
                    self.assertRegex(values["SONIC_RUN_CMDS"], r"\bbash artifacts/vs/[\w-]+\.sh$")
                    runner = [call for call in calls if "tools/bazel/ci/container.py" in call["args"]]
                    self.assertEqual(len(runner), 0 if make_exit else 1)
                    if runner:
                        args = runner[0]["args"]
                        self.assertIn("--reuse-build", args)
                        for flag, value in (("--container", owner), ("--source-file", source),
                                            ("--artifacts", artifacts), ("--bazel", self.environment["BAZEL"])):
                            self.assertEqual(args[args.index(flag) + 1], value)
                        self.assertEqual("--bazel-arg=--config=syncd-dash" in args, dash)
                        self.assertEqual("--bazel-arg=--//dockers/docker-orchagent:source_config_engine=true" in args,
                                         not dash)
                        for option in ("--distdir=/bazel_cache/source downloads", "--remote_download_all",
                                       "--platforms=@sonic_build_infra//platforms:x86_64_trixie",
                                       "--host_platform=@sonic_build_infra//platforms:x86_64_trixie",
                                       "--repository_cache=" + self.environment["BAZEL_CONTAINER_CACHE_DIR"] + "/repository_cache",
                                       "--disk_cache=" + self.environment["BAZEL_CONTAINER_CACHE_DIR"] + "/disk_cache"):
                            self.assertIn("--bazel-arg=" + option, args)
                        self.assertEqual(runner[0]["environment"],
                                         {key: self.environment[key] for key in ("BAZEL", "BAZEL_CONTAINER_ARGS")})
                    analyses = [call for call in calls if "tools/bazel/ci/bazel_commands.py" in call["args"]]
                    self.assertEqual(len(analyses), int(dash and not source_exit and not make_exit))
                    self.assertFalse(any(call["name"] == "rm" or "clean" in call["args"] for call in calls))


if __name__ == "__main__":
    unittest.main()
