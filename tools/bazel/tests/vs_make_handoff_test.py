"""Execute the real Make command expansion and nested VS validation shells."""

import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]


def assignment_block(contents, name):
    """Keep the production assignment, including every continuation line."""
    lines = contents.splitlines(keepends=True)
    start = next(index for index, line in enumerate(lines) if line.startswith(name + " :="))
    end = start
    while lines[end].rstrip().endswith("\\"):
        end += 1
    return "".join(lines[start:end + 1])


class VsMakeHandoffTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="vs-make-handoff-")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        (self.directory / "scripts").mkdir()
        (self.directory / "scripts/run_with_retry").symlink_to(ROOT / "scripts/run_with_retry")
        scripts = self.directory / "tools/bazel/ci"
        scripts.mkdir(parents=True)
        shutil.copyfile(ROOT / "tools/bazel/ci/validate_vs.sh", scripts / "validate_vs.sh")
        artifacts = self.directory / "artifacts/vs"
        artifacts.mkdir(parents=True)
        (artifacts / "build-revision.txt").write_text("a" * 40 + "\n")

        source = (ROOT / "Makefile.work").read_text()
        instruction = assignment_block(source, "SONIC_BUILD_INSTRUCTION")
        cache = re.search(
            r"(?m)^SONIC_VERSION_CACHE :=[^\n]*\n"
            r"(?:SONIC_OVERRIDE_BUILD_VARS \+=[^\n]*\n)+"
            r"export SONIC_VERSION_CACHE SONIC_VERSION_CACHE_SOURCE\n", source)
        recipe = re.search(r"(?m)^sonic-slave-run\s*:\s*sonic-slave-build\n(?:\t[^\n]*\n)+", source)
        self.assertIsNotNone(cache, "Expected the production version-cache append block")
        self.assertIsNotNone(recipe, "Expected the production sonic-slave-run recipe")
        # Only expensive prerequisites and container execution are controlled.
        # GNU Make evaluates the exact production command and run recipe below.
        (self.directory / "Makefile.work").write_text("""SHELL := /bin/bash
.SHELLFLAGS := -e -c
Q := @
PLATFORM := vs
PLATFORM_ARCH := amd64
USER := fixture-builder
SLAVE_BASE_IMAGE := fixture-slave
SLAVE_BASE_TAG := fixture
DOCKER_RUN := docker run
DOCKER_USER_COMMANDS := :
SONIC_VERSION_CACHE_METHOD := readwrite
SONIC_VERSION_CACHE_SOURCE := /fixture-version-cache
.PHONY: bookworm docker-cleanup sonic-slave-build
bookworm docker-cleanup sonic-slave-build:
	@printf '%s\\n' '$@' >> events
""" + cache[0] + instruction + recipe[0])

        commands = self.directory / "commands"
        commands.mkdir()
        shim = "#!" + sys.executable + "\n" + r'''
import os, pathlib, subprocess, sys
name = pathlib.Path(sys.argv[0]).name
args = sys.argv[1:]
if name == "git":
    assert args == ["rev-parse", "HEAD"]
    print("a" * 40)
    raise SystemExit(0)
with pathlib.Path("events").open("a") as output:
    output.write(name + "\n")
if name == "docker":
    assert args[:2] == ["run", "fixture-slave:fixture"]
    assert args[2:4] == ["bash", "-c"]
    command = args[2:]
    # Docker does not inherit arbitrary host Make exports. Only preserve the
    # executable search path and this fixture's failure injection control.
    environment = {key: os.environ[key] for key in ("PATH", "VERIFIER_STATUS") if key in os.environ}
else:
    assert name == "su" and args[:2] == ["fixture-builder", "-c"]
    # Match su's shell invocation; it does not introduce bash's -e option.
    command = ["bash", "-c", args[2]]
    environment = None
raise SystemExit(subprocess.run(command, env=environment).returncode)
'''
        for name in ("docker", "su", "git"):
            path = commands / name
            path.write_text(shim)
            path.chmod(0o755)
        (scripts / "vs_archive_reuse.py").write_text(
            "import pathlib, sys\n"
            "with pathlib.Path('events').open('a') as output:\n"
            "    output.write(sys.argv[1] + '\\n')\n")
        (scripts / "verify_vs_oci.sh").write_text(
            "#!/usr/bin/env bash\n"
            "python3 tools/bazel/ci/record-command.py \"$@\"\n")
        (scripts / "record-command.py").write_text(r'''
import json, os, pathlib, sys
with pathlib.Path("events").open("a") as output:
    output.write("verifier\n")
pathlib.Path("verifier-arguments.json").write_text(json.dumps({
    "arguments": sys.argv[1:],
    "environment": {name: os.environ.get(name) for name in
                    ("INCLUDE_VS_DASH_SAI", "SONIC_OVERRIDE_BUILD_VARS")},
}))
raise SystemExit(int(os.environ.get("VERIFIER_STATUS", "0")))
''')
        # Avoid passing host proxy, signing, or other build configuration into
        # the production assignment block or the recorded test arguments.
        self.environment = {
            "PATH": os.pathsep.join((str(commands), str(Path(sys.executable).parent), os.defpath)),
            "SONIC_CI_CONTAINER_LABEL": "fixture-label", "BAZEL": "fixture-bazel",
            "SONIC_BUILD_RETRY_COUNT": "0",
            "SONIC_OVERRIDE_BUILD_VARS": "OVERRIDE_MARKER=first SHARED_MARKER=old",
            "SONIC_BUILD_VARS": "BUILD_MARKER=second SHARED_MARKER=new",
        }

    def validate(self, **environment):
        return subprocess.run(["bash", "tools/bazel/ci/validate_vs.sh"], cwd=self.directory,
                              env=dict(self.environment, **environment), text=True, capture_output=True)

    def events(self):
        return (self.directory / "events").read_text().splitlines()

    def test_real_make_expands_command_and_preserves_caller_and_cache_overrides(self):
        result = self.validate()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        recorded = json.loads((self.directory / "verifier-arguments.json").read_text())
        arguments = recorded["arguments"]
        self.assertEqual(Path(arguments[0]).name, "make")
        self.assertEqual(arguments[1:3], ["-f", "slave.mk"])
        values = dict(argument.split("=", 1) for argument in arguments[3:])
        for key, value in {
            "PLATFORM": "vs", "PLATFORM_ARCH": "amd64", "INCLUDE_FIPS": "y",
            "ENABLE_ASAN": "n", "SONIC_ENABLE_SYNCD_RPC": "n", "BUILD_WITH_BAZEL_WHEN_AVAILABLE": "y",
            "OVERRIDE_MARKER": "first", "BUILD_MARKER": "second", "SHARED_MARKER": "new",
            "SONIC_VERSION_CACHE": "readwrite", "SONIC_VERSION_CACHE_SOURCE": "/fixture-version-cache",
        }.items():
            self.assertEqual(values[key], value, key)
        self.assertLess(arguments.index("SHARED_MARKER=old"), arguments.index("SHARED_MARKER=new"))
        self.assertIsNone(recorded["environment"]["INCLUDE_VS_DASH_SAI"])
        self.assertIsNone(recorded["environment"]["SONIC_OVERRIDE_BUILD_VARS"])
        self.assertFalse(any("$(" in argument for argument in arguments))
        self.assertEqual(self.events(), ["capture-pairs", "bookworm", "sonic-slave-build",
                                        "docker", "su", "verifier", "docker-cleanup", "verify-pairs"])

    def test_inner_failure_crosses_both_shells_and_stops_success_validation(self):
        result = self.validate(VERIFIER_STATUS="19")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.events(), ["capture-pairs", "bookworm", "sonic-slave-build",
                                        "docker", "su", "verifier"])
        self.assertFalse((self.directory / "artifacts/vs/runtime-debug-pairs-timing.json").exists())


if __name__ == "__main__":
    unittest.main()
