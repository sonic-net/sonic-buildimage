"""Execute the timing producer and real shell boundaries with build tools stubbed."""
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import signal
import subprocess
import tempfile
import textwrap
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[3]
HELPER = ROOT / "scripts/image_timing.py"
KVM_MARKER = "[ image phases ] [ target/sonic-vs.img.gz ] "
spec = importlib.util.spec_from_file_location("image_timing", HELPER)
timing = importlib.util.module_from_spec(spec)
spec.loader.exec_module(timing)


class ReceiptTestCase(unittest.TestCase):
    def setUp(self):
        self.paths = []

    def tearDown(self):
        for path in self.paths:
            Path(path).unlink(missing_ok=True)

    def begin(self):
        path = timing.begin()
        self.paths.append(path)
        return path

    def complete(self, path, pattern=timing.SINGLE, offset=1):
        for phase, context in pattern[offset:]:
            timing.switch(path, phase, context)

    def cli(self, path, *args, **kwargs):
        env = dict(os.environ)
        env.pop(timing.ENV, None)
        if path:
            env[timing.ENV] = path
        return subprocess.run(["python3", str(HELPER), *args], env=env,
                              capture_output=True, text=True, check=True, **kwargs)


class ProducerTests(ReceiptTestCase):
    def test_microsecond_endpoints_partition_each_installer_recipe(self):
        for target, pattern in ((timing.KVM_TARGET, timing.SINGLE), (timing.KVM_TARGET, timing.MULTI),
                                (timing.ONIE_TARGET, timing.ONIE)):
            marker = f"[ image phases ] [ {target} ] "
            with self.subTest(pattern=len(pattern)):
                with mock.patch.object(timing, "now_us", side_effect=[
                        100_000_000 + i * 1_234_567 for i in range(len(pattern) + 1)]):
                    path = self.begin()
                    self.complete(path, pattern)
                    line = timing.end(path, target)
                self.assertFalse(Path(path).exists())
                self.assertTrue(line.startswith(marker))
                result = json.loads(line[len(marker):])
                self.assertEqual(result["version"], 1)
                self.assertEqual([(s["phase"], s["context"]) for s in result["segments"]], pattern)
                self.assertEqual(result["segments"][0]["offset_seconds"], 0)
                elapsed = 0
                for segment in result["segments"]:
                    self.assertAlmostEqual(segment["offset_seconds"], elapsed, places=6)
                    elapsed += segment["duration_seconds"]
                self.assertAlmostEqual(elapsed, result["duration_seconds"], places=6)
                self.assertLess(len(line), timing.MAX_BYTES)

    def test_incomplete_or_missing_transition_cannot_publish(self):
        for skipped in (None, 2, 8, 10, 13):
            with self.subTest(skipped=skipped):
                path = self.begin()
                if skipped is not None:
                    for index, event in enumerate(timing.SINGLE[1:], 1):
                        if index != skipped:
                            self.cli(path, "switch", *event)
                self.assertEqual(self.cli(path, "end").stdout, "")
                self.assertFalse(Path(path).exists())

    def test_wrong_target_or_missing_onie_docker_phase_cannot_publish(self):
        for pattern, target in ((timing.SINGLE, timing.ONIE_TARGET), (timing.ONIE, timing.KVM_TARGET),
                                ([event for event in timing.ONIE if event[1] != "docker-load"], timing.ONIE_TARGET)):
            path = self.begin()
            for event in pattern[1:]:
                self.cli(path, "switch", *event)
            self.assertEqual(self.cli(path, "end", target).stdout, "")
            self.assertFalse(Path(path).exists())

    def test_invalid_labels_duplicates_and_oversized_receipts_are_quiet(self):
        for content in ("invalid\n", "x" * (timing.MAX_BYTES + 1),
                        '[true,"assembly","recipe"]\n', '[1,"private-host","secret"]\n'):
            path = self.begin()
            Path(path).write_text(content)
            result = self.cli(path, "end")
            self.assertEqual((result.stdout, result.stderr), ("", ""))
        path = self.begin()
        self.cli(path, "switch", "assembly", "recipe")
        self.complete_quietly(path)
        self.assertEqual(self.cli(path, "end").stdout, "")

    def complete_quietly(self, path):
        for event in timing.SINGLE[1:]:
            self.cli(path, "switch", *event)

    def test_missing_environment_is_noop(self):
        for args in (("switch", "compression", "bios"), ("end",), ("discard",)):
            result = self.cli(None, *args)
            self.assertEqual((result.stdout, result.stderr), ("", ""))

    def test_readonly_symlink_hardlink_and_wrong_owner_are_untouched(self):
        path = self.begin()
        original = Path(path).read_bytes()
        os.chmod(path, 0o400)
        for command in (("switch", "assembly", "filesystem"), ("end",), ("discard",)):
            self.assertEqual(self.cli(path, *command).stdout, "")
        self.assertEqual(Path(path).read_bytes(), original)
        os.chmod(path, 0o600)
        alias = self.begin()
        os.unlink(alias)
        os.symlink(path, alias)
        for command in (("switch", "assembly", "filesystem"), ("end",), ("discard",)):
            self.cli(alias, *command)
        self.assertTrue(Path(alias).is_symlink())
        self.assertEqual(Path(path).read_bytes(), original)
        os.unlink(alias)
        os.link(path, alias)
        self.cli(alias, "discard")
        self.assertEqual(Path(path).read_bytes(), original)
        self.assertTrue(Path(alias).exists())
        os.unlink(alias)
        with mock.patch.object(timing.os, "geteuid", return_value=os.stat(path).st_uid + 1):
            with self.assertRaises(ValueError):
                timing.switch(path, "assembly", "filesystem")
        self.assertEqual(Path(path).read_bytes(), original)

    def test_nonreceipt_path_not_opened(self):
        with tempfile.NamedTemporaryFile() as handle:
            handle.write(b"preserve me")
            handle.flush()
            self.cli(handle.name, "discard")
            self.cli(handle.name, "switch", "assembly", "filesystem")
            self.assertEqual(Path(handle.name).read_bytes(), b"preserve me")


class ShellBoundaryTests(ReceiptTestCase):
    def setUp(self):
        super().setUp()
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.cwd = Path(self.temp.name)
        (self.cwd / "scripts").mkdir()
        (self.cwd / "scripts/image_timing.py").symlink_to(HELPER)
        self.env = dict(os.environ)
        self.env.pop(timing.ENV, None)
        self.env["vs_build_prepare_mem"] = "no"

    def shell(self, script, **env):
        return subprocess.run(["bash", "-e", "-c", script], cwd=self.cwd,
                              env=dict(self.env, **env), text=True, capture_output=True, timeout=20)

    def test_host_make_workflow_emits_parseable_markers_and_preserves_make_failure(self):
        from test_collect import collector, job

        workflow = (ROOT / ".github/workflows/bazel-oci.yml").read_text()
        block = workflow.split("      - name: Build sonic-vs.bin through the host Make wrapper\n", 1)[1]
        header, script = block.split("\n      - name:", 1)[0].split("        run: |\n", 1)
        environment = {"TERM": "xterm", **dict(re.findall(r"^          (TERM): (\S+)$", header, re.MULTILINE))}
        script = textwrap.dedent(script)
        (self.cwd / "artifacts/vs").mkdir(parents=True)
        (self.cwd / "target").mkdir()
        helper = self.cwd / "tools/bazel/ci/build_vs.sh"
        helper.parent.mkdir(parents=True)
        helper.write_text((ROOT / "tools/bazel/ci/build_vs.sh").read_text())
        self.assertIn('@echo "+++ --- Making $@ --- +++"', (ROOT / "Makefile").read_text())
        # Exercise the real logger without touching its host-global target list.
        logger = (ROOT / "update_screen.sh").read_text().replace(
            "target_list_file=/tmp/target_list", 'target_list_file="$TEST_WORKSPACE/target_list"')
        (self.cwd / "update_screen.sh").write_text(logger)
        stubs = """
lockfile() { :; }
export -f lockfile
git() { echo aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa; }
make() {
    echo "+++ --- Making target/sonic-vs.bin --- +++"
    for target in target/docker-base-trixie.gz target/sonic-vs.bin__vs__rfs.squashfs target/sonic-vs.bin; do
        bash ./update_screen.sh -a "$target"
        bash ./update_screen.sh -d "$target"
    done
    printf installer > target/sonic-vs.bin
    printf archives > target/sonic-vs.bin.dockers
    return "$MAKE_STATUS"
}
export -f git make
"""
        for status in (0, 37):
            with self.subTest(make_status=status):
                result = self.shell(stubs + script, **environment,
                                    RUNNER_TEMP=str(self.cwd), TEST_WORKSPACE=str(self.cwd),
                                    MAKE_STATUS=str(status), SONIC_CI_CONTAINER_LABEL="fixture=vs",
                                    BAZEL="/fixture/bazel")
                self.assertEqual(result.returncode, status, result.stderr)
                self.assertIn("[ building ] [ target/sonic-vs.bin ]", result.stdout)
                self.assertIn("[ finished ] [ target/sonic-vs.bin ]", result.stdout)
                if status:
                    continue
                lines = ["2026-10-10T00:00:00.2000000Z ##[group]Run set -euo pipefail"]
                lines.extend(f"2026-10-10T00:00:{index:02d}Z {line}"
                             for index, line in enumerate(result.stdout.splitlines(), 1))
                lines.append("2026-10-10T00:10:00.4000000Z ##[group]Run validate installer")
                step = dict(job()["steps"][0], name="Build sonic-vs.bin through the host Make wrapper")
                value = collector.breakdown.parse_log(("\n".join(lines) + "\n").encode(), step)
                self.assertTrue(collector.breakdown.valid(value))
                self.assertEqual(value["status"], "available")
                self.assertEqual([section["id"] for section in value["sections"]], ["vs-image-1"])
                self.assertEqual([task["id"] for task in value["sections"][0]["tasks"]],
                                 ["base-1", "rootfs-1", "final-image-1"])

    def test_real_make_relay_bypasses_target_log_and_preserves_failure(self):
        source = (ROOT / "slave.mk").read_text()
        begin = source.split("\t# The receipt is private;", 1)[1].split("\t# Pass initramfs", 1)[0]
        begin = begin[begin.index("\tunset SONIC_IMAGE_TIMING_FILE"):]
        # Exercise the real ONIE target with the shell's automatic variable expanded by Make.
        begin = begin.replace("$$", "$").replace('"$@"', '"target/sonic-vs.bin"')
        end = source.split("\t# Reached only on success,", 1)[1].split("\t$(FOOTER)", 1)[0]
        end = end[end.index("\tif [ -n"):].replace("$$", "$").replace('"$@"', '"target/sonic-vs.bin"')
        transitions = "\n".join("python3 ./scripts/image_timing.py switch " + " ".join(event)
                                for event in timing.ONIE[1:])
        marker = "[ image phases ] [ target/sonic-vs.bin ] "
        for enabled, fail in (("1", False), ("1", True), ("", False)):
            with self.subTest(enabled=enabled, failure=fail):
                script = begin + '\nprintf "%s" "$SONIC_IMAGE_TIMING_FILE" > receipt-path\n'
                script += "(echo original-target-log\n" + transitions + ") >> target.log 2>&1\n"
                script += "exit 37\n" if fail else end
                result = self.shell(script, SONIC_VS_IMAGE_TIMINGS=enabled)
                receipt_path = (self.cwd / "receipt-path").read_text()
                self.assertEqual(bool(receipt_path), bool(enabled))
                if receipt_path:
                    self.assertFalse(Path(receipt_path).exists())
                self.assertEqual(result.returncode, 37 if fail else 0, result.stderr)
                self.assertEqual(result.stdout.count(marker), 1 if enabled and not fail else 0)
                self.assertNotIn(marker, (self.cwd / "target.log").read_text())
                self.assertIn("original-target-log", (self.cwd / "target.log").read_text())

    def test_final_filesystem_compression_boundaries_and_failure(self):
        source = (ROOT / "build_debian.sh").read_text()
        marker = 'if [ -n "${SONIC_IMAGE_TIMING_FILE:-}" ]; then\n    python3 ./scripts/image_timing.py switch compression filesystem'
        tail = source[source.index(marker):]
        (self.cwd / "fsroot").mkdir()
        stub = '''
sudo() {
    printf '%s\\n' "$*" >> commands
    case "$1" in
        mksquashfs) return "${FAIL_SQUASHFS:-0}" ;;
        LANG=C) echo 1000 ;;
    esac
    return 0
}
stat() { echo 100; }
FILESYSTEM_ROOT=./fsroot
FILESYSTEM_SQUASHFS=filesystem.squashfs
FILESYSTEM_DOCKERFS=dockerfs.tar.gz
INSTALLER_PAYLOAD=payload.zip
PLATFORM_DIR=platform
'''
        for image_type, pattern in (("onie", timing.ONIE), ("kvm", timing.FILESYSTEM)):
            squashfs = pattern.index(("compression", "filesystem"))
            for failure in (0, 43):
                with self.subTest(image_type=image_type, failure=failure):
                    path = self.begin()
                    self.complete(path, pattern[:squashfs])
                    result = self.shell(stub + tail, **{timing.ENV: path, "IMAGE_TYPE": image_type,
                                                       "FAIL_SQUASHFS": str(failure)})
                    self.assertEqual(result.returncode, failure, result.stderr)
                    events = [json.loads(line) for line in Path(path).read_text().splitlines()]
                    stop = squashfs + 1 if failure else pattern.index(("assembly", "installer"))
                    self.assertEqual([(e[1], e[2]) for e in events], pattern[:stop])
                    self.assertIn("mksquashfs", (self.cwd / "commands").read_text())
                    if not failure:
                        self.assertIn("zip -g", (self.cwd / "commands").read_text())

    def test_onie_load_interval_encloses_the_rendered_images_and_manifest_checks(self):
        template = (ROOT / "files/build_templates/sonic_debian_extension.j2").read_text()
        before, rest = template.split("switch assembly docker-load", 1)
        loop, after = rest.split("switch assembly filesystem", 1)
        self.assertIn("docker info", before)
        for command in ("docker load -i", "docker tag", "docker inspect", "jq -e"):
            self.assertIn(command, loop)
        self.assertIn("{% for docker_installation_target in installer_images", loop)
        self.assertIn("{% endfor %}", loop)
        self.assertIn("SONIC_PACKAGE_MANAGER_FOLDER=", after)

    def test_onie_wrapper_is_timed_through_completion_only(self):
        source = (ROOT / "build_image.sh").read_text()
        branch = source.split('if [ "$IMAGE_TYPE" = "onie" ]; then', 1)[1].split('## Build a raw', 1)[0]
        stub = """
OUTPUT_ONIE_IMAGE=target/sonic-vs.bin
sudo() { :; }
generate_device_list() { :; }
generate_onie_installer_image() {
    tail -n1 "$SONIC_IMAGE_TIMING_FILE" > at-wrapper
    return "$WRAPPER_STATUS"
}
"""
        for status in (0, 42):
            path = self.begin()
            self.complete(path, timing.ONIE[:-2])
            result = self.shell(stub + branch, **{timing.ENV: path, "WRAPPER_STATUS": str(status)})
            self.assertEqual(result.returncode, status, result.stderr)
            self.assertEqual(json.loads((self.cwd / "at-wrapper").read_text())[1:], ["assembly", "installer"])
            line = self.cli(path, "end", timing.ONIE_TARGET).stdout
            self.assertEqual(bool(line), not status)

    def test_gnu_make_oneshell_relay_and_exit_trap_with_production_log_pipeline(self):
        source = (ROOT / "slave.mk").read_text()
        begin = source.split("\t# The receipt is private;", 1)[1].split("\t# Pass initramfs", 1)[0]
        begin = begin[begin.index("\tunset SONIC_IMAGE_TIMING_FILE"):]
        end = source.split("\t# Reached only on success,", 1)[1].split("\t$(FOOTER)", 1)[0]
        end = end[end.index("\tif [ -n"):]
        log_definition = next(line for line in (ROOT / "rules/functions").read_text().splitlines()
                              if line.startswith("LOG = "))
        (self.cwd / "scripts/process_log.sh").symlink_to(ROOT / "scripts/process_log.sh")
        self.executable(self.cwd / "update_screen.sh", "#!/bin/bash\nexit 0\n")
        (self.cwd / "target").mkdir()
        preamble = (".ONESHELL:\nSHELL = /bin/bash\n.SHELLFLAGS = -ec\n"
                    "PROJECT_ROOT := " + str(self.cwd) + "\n" + log_definition
                    + "\n.PHONY: target/sonic-vs.img.gz\ntarget/sonic-vs.img.gz:\n\t@\n")
        for failure in ("none", "recipe", "pipeline"):
            with self.subTest(failure=failure):
                body = "#!/bin/bash\nset -e\necho original-target-log\n"
                body += "exit 37\n" if failure == "pipeline" else ""
                body += "\n".join("python3 ./scripts/image_timing.py switch " + " ".join(event)
                                  for event in timing.SINGLE[1:]) + "\n"
                self.executable(self.cwd / "phase-work.sh", body)
                recipe = preamble + begin + '\tprintf "%s" "$$SONIC_IMAGE_TIMING_FILE" > receipt-path\n'
                recipe += "\t./phase-work.sh $(LOG)\n"
                recipe += "\texit 37\n" if failure == "recipe" else ""
                recipe += end
                (self.cwd / "Makefile").write_text(recipe)
                log = self.cwd / "target/sonic-vs.img.gz.log"
                log.unlink(missing_ok=True)
                result = subprocess.run(["make", "--no-print-directory", "target/sonic-vs.img.gz"],
                                        cwd=self.cwd, env=self.env, text=True,
                                        capture_output=True, timeout=20)
                path = (self.cwd / "receipt-path").read_text()
                self.paths.append(path)
                self.assertFalse(Path(path).exists())
                self.assertEqual(result.returncode, 0 if failure == "none" else 2, result.stderr)
                if failure != "none":
                    # The existing LOG pipeline deliberately maps a tool error to 1.
                    self.assertIn("Error " + ("37" if failure == "recipe" else "1"), result.stderr)
                self.assertEqual(result.stdout.count(KVM_MARKER), 1 if failure == "none" else 0)
                self.assertNotIn(KVM_MARKER, log.read_text())
                self.assertIn("original-target-log", log.read_text())

    def test_first_rootfs_and_other_products_disable_inherited_receipt(self):
        source = (ROOT / "build_debian.sh").read_text()
        guard = source.split("# Only the final VS recipe", 1)[1].split("ORGANIZATION_BUILD_HOOK=", 1)[0]
        guard = guard[guard.index("if [["):]
        for overrides in ({"RFS_SPLIT_FIRST_STAGE": "y"}, {"RFS_SPLIT_LAST_STAGE": "n"},
                          {"TARGET_MACHINE": "broadcom"}, {"IMAGE_TYPE": "raw"}):
            path = self.begin()
            env = {timing.ENV: path, "RFS_SPLIT_LAST_STAGE": "y", "RFS_SPLIT_FIRST_STAGE": "n",
                   "TARGET_MACHINE": "vs", "IMAGE_TYPE": "kvm", **overrides}
            result = self.shell(guard + '\nprintf "%s" "${SONIC_IMAGE_TIMING_FILE:-}"', **env)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, "")
            self.assertEqual(len(Path(path).read_text().splitlines()), 1)

    def test_real_bios_uefi_shell_flow_and_boot_or_compression_failure(self):
        # Run the production function and KVM script, replacing only external tools.
        bindir = self.cwd / "bin"
        bindir.mkdir()
        self.env["PATH"] = str(bindir) + os.pathsep + self.env["PATH"]
        for name in ("apt-get", "mount", "free", "fallocate", "mkfs.ext2", "cp", "ps"):
            self.executable(bindir / name, "#!/bin/bash\nexit 0\n")
        self.executable(bindir / "sudo", '#!/bin/bash\n[ "$1" != -E ] || shift\nexec "$@"\n')
        self.executable(bindir / "qemu-img", '#!/bin/bash\ntouch "$4"\n')
        self.executable(bindir / "netstat", '#!/bin/bash\necho :9000\n')
        self.executable(bindir / "sleep", '#!/bin/bash\n/bin/sleep 0.02\n')
        self.executable(bindir / "kvm", '#!/bin/bash\necho $$ >> kvm-pids\nexec /bin/sleep 60\n')
        self.executable(bindir / "pigz", '#!/bin/bash\n[ "${FAIL_PIGZ:-0}" = 0 ] || exit "$FAIL_PIGZ"\nmv "$1" "$1.gz"\n')
        self.executable(self.cwd / "install_sonic.py", '#!/bin/bash\nexit 0\n')
        self.executable(self.cwd / "check_install.py", '#!/bin/bash\nexit "${FAIL_BOOT:-0}"\n')
        kvm = (ROOT / "scripts/build_kvm_image.sh").read_text().replace("/usr/bin/kvm", shlex.quote(str(bindir / "kvm")))
        self.executable(self.cwd / "scripts/build_kvm_image.sh", kvm)
        function = (ROOT / "build_image.sh").read_text().split("generate_kvm_image()", 1)[1].split("generate_onie_installer_image()", 1)[0]
        script = '''
OUTPUT_KVM_IMAGE=sonic-vs.img.gz
OUTPUT_ONIE_IMAGE=installer.bin
KVM_IMAGE_DISK_SIZE=1
image_password=not-for-the-summary
''' + "generate_kvm_image()" + function + '''
generate_kvm_image 1
generate_kvm_image 1 UEFI
python3 ./scripts/image_timing.py end 2>/dev/null || true
'''
        try:
            for env, status in (({}, 0), ({"FAIL_BOOT": "37"}, 1), ({"FAIL_PIGZ": "43"}, 43),
                                ({"BROKEN_TIMING": "y"}, 0)):
                with self.subTest(env=env):
                    path = self.begin()
                    for event in timing.FILESYSTEM[1:]:
                        timing.switch(path, *event)
                    if env.get("BROKEN_TIMING"):
                        (self.cwd / "scripts/image_timing.py").unlink()
                    result = self.shell(script, **{timing.ENV: path, **env})
                    self.assertEqual(result.returncode, status, result.stderr)
                    lines = [line for line in result.stdout.splitlines() if line.startswith(KVM_MARKER)]
                    self.assertEqual(len(lines), 0 if status or env.get("BROKEN_TIMING") else 1)
                    if not status and not env.get("BROKEN_TIMING"):
                        summary = json.loads(lines[0][len(KVM_MARKER):])
                        self.assertEqual([(s["phase"], s["context"]) for s in summary["segments"]], timing.SINGLE)
                        self.assertNotIn("not-for-the-summary", lines[0])
        finally:
            if (self.cwd / "kvm-pids").exists():
                for pid in (self.cwd / "kvm-pids").read_text().splitlines():
                    try:
                        os.kill(int(pid), signal.SIGTERM)
                    except ProcessLookupError:
                        pass

    @staticmethod
    def executable(path, content):
        path.write_text(content)
        path.chmod(0o755)


if __name__ == "__main__":
    unittest.main()
