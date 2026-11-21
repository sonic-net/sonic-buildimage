"""Run the VS installer's outer Make and builder shell boundaries with a disposable fixture."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]


class VsBuildTailTimingTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.work = Path(tmp.name)
        for directory in ("scripts", "tools/bazel"):
            (self.work / directory).mkdir(parents=True)
        for path in ("scripts/run_with_retry", "tools/bazel/direct_image_goals.mk"):
            (self.work / path).symlink_to(ROOT / path)
        # Execute the production host recipe, including the two nested shells in
        # the Docker/su command. The tiny stand-ins omit the actual image build.
        source = (ROOT / "Makefile.work").read_text()
        auto_remove = source[source.index("builder_auto_remove :="):source.index("DOCKER_RUN :=")]
        run_prefix = source.split("DOCKER_RUN := ", 1)[1].split(" $(DOCKER_PRIVMODE)", 1)[0]
        image_selection = source[source.index("vs_build_slave_image ="):source.index("# Reuse downloads")]
        timing = source[source.index("vs_installer_tail_timing ="):source.index("ifdef SOURCE_FOLDER")]
        rule = source[source.index("%:: | sonic-build-hooks"):source.index("\ndocker-cleanup:")]

        (self.work / "Makefile.work").write_text(
            "SHELL = /bin/bash\n.SHELLFLAGS = -e -c\n.ONESHELL:\nQ = @\n"
            + auto_remove + f"DOCKER_RUN := {run_prefix}\n"
            "DOCKER_USER_COMMANDS = :\nUSER = builder\n"
            "SLAVE_BASE_IMAGE = fixture-slave\nSLAVE_BASE_TAG = test\n"
            "SONIC_BUILD_INSTRUCTION = ./build\ndocker-image-cleanup = echo HOST_CLEANUP\n"
            ".PHONY: sonic-build-hooks\n" + image_selection + timing + rule +
            "\n.PHONY: docker-cleanup\ndocker-cleanup:\n\t@echo RECURSIVE_CLEANUP\n")
        self.script("docker", 'echo "BUILDER ${1} ${2}" >> builder-flags\nshift 3\n"$@"\nstatus=$?\necho "DOCKER_RETURN $status"\nexit "$status"')
        self.script("su", 'exec bash -c "$3"')
        self.script("build", 'echo BUILT\nexit "${BUILD_STATUS:-0}"')
        # The production collector sources buildinfo from the installed slave.
        # Substitute that dependency with an early return of the supplied status.
        collector = (ROOT / "scripts/collect_build_version_files.sh").read_text()
        self.script("scripts/collect_build_version_files.sh", collector.removeprefix("#!/bin/bash\n").replace(
            ". /usr/local/share/buildinfo/scripts/buildinfo_base.sh",
            'echo "COLLECT $RET"; exit "$RET"'))

    def script(self, name, body):
        path = self.work / name
        path.write_text("#!/bin/bash\n" + body + "\n")
        path.chmod(0o755)

    def run_make(self, goal="target/sonic-vs.bin", timings="1", status=0, *, bookworm=False, defer=False, variables=()):
        result = subprocess.run(
            ["make", "--no-print-directory", "-f", str(ROOT / "Makefile"), goal,
             f"NOBOOKWORM={int(not bookworm)}", "SONIC_VS_IMAGE_TIMINGS=" + timings,
             *(["SONIC_VS_DEFER_BUILDER_CLEANUP=y"] if defer else []), *variables],
            cwd=self.work, capture_output=True, text=True,
            env={"PATH": str(self.work) + ":" + os.defpath, "BUILD_STATUS": str(status)})
        events = [line for line in result.stdout.splitlines() if
                  line.startswith(("[ vs timing ]", "BUILT", "COLLECT ", "DOCKER_RETURN ",
                                   "HOST_CLEANUP", "RECURSIVE_CLEANUP"))]
        return result, events

    def test_cached_build_runs_the_inspected_slave_image(self):
        self.script("docker", """set -eu
if [[ "$1" == image ]]; then
    [[ "$*" == "image inspect --format {{.Id}} fixture-slave:test" ]]
    echo sha256:fixture
    exit
fi
[[ "$1" == run && "$2" == --rm=true ]]
shift 2
[[ "$1" == --mount && "$2" == "type=bind,src=$PWD/cache,dst=/vs_host_base_cache" ]]
[[ "$3" == -e && "$4" == SONIC_HOSTBASE_CACHE_DIR=/vs_host_base_cache ]]
[[ "$5" == -e && "$6" == SONIC_HOSTBASE_BUILDER_ID ]]
[[ "$7" == sha256:fixture && "$7" == "$SONIC_HOSTBASE_BUILDER_ID" ]]
shift 7
exec "$@"
""")
        result, events = self.run_make(variables=(
            "SONIC_HOSTBASE_CACHE_SOURCE=cache", "CONFIGURED_ARCH=amd64",
            "CONFIGURED_PLATFORM=vs", "BUILD_WITH_BAZEL_WHEN_AVAILABLE=y"))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("BUILT", events)

    def test_markers_bound_version_collection_container_return_and_cleanups(self):
        result, events = self.run_make()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(events, [
            "BUILT", "[ vs timing ] [ build-version-collection ] [ start ]", "COLLECT 0",
            "[ vs timing ] [ build-version-collection ] [ end ]", "[ vs timing ] [ builder-exit ] [ start ]",
            "DOCKER_RETURN 0", "[ vs timing ] [ builder-exit ] [ end ]",
            "[ vs timing ] [ trixie-image-cleanup ] [ start ]", "HOST_CLEANUP",
            "[ vs timing ] [ trixie-image-cleanup ] [ end ]",
            "[ vs timing ] [ bookworm-cleanup ] [ start ]", "RECURSIVE_CLEANUP",
            "[ vs timing ] [ bookworm-cleanup ] [ end ]"])

    def test_untimed_installer_and_other_timed_targets_keep_the_original_build(self):
        for goal, timings in (("target/sonic-vs.bin", ""), ("target/sonic-vs.img.gz", "1")):
            with self.subTest(goal=goal):
                result, events = self.run_make(goal, timings)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(events, ["BUILT", "COLLECT 0", "DOCKER_RETURN 0",
                                          "HOST_CLEANUP", "RECURSIVE_CLEANUP"])

    def test_deferred_cleanup_retains_only_the_final_trixie_installer_builder(self):
        for goal, defer, expected in (
            ("target/sonic-vs.bin", False, "true"),
            ("target/sonic-vs.bin", True, "false"),
            ("target/sonic-vs.img.gz", True, "true"),
        ):
            with self.subTest(goal=goal, defer=defer):
                flags = self.work / "builder-flags"
                flags.write_text("")
                result, _ = self.run_make(goal, bookworm=True, defer=defer)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(flags.read_text().splitlines(),
                                 ["BUILDER run --rm=true", f"BUILDER run --rm={expected}"])

    def test_inner_build_status_crosses_collector_and_docker_shells(self):
        result, events = self.run_make(status=19)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(events, ["BUILT", "COLLECT 19", "DOCKER_RETURN 19"])
