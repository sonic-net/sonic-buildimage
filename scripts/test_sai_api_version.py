"""Exercise strict SAI version checking against real shared libraries and cached artifacts."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

import check_sai_api_version as gate


ROOT = Path(__file__).resolve().parents[1]
BROADCOM_IMAGES = ["docker-syncd-brcm.gz", "docker-syncd-brcm-legacy-th.gz", "docker-syncd-brcm-dnx.gz"]


class SaiApiVersionTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.header = self.root / "saiversion.h"
        self.header.write_text(
            "#define SAI_MAJOR 1\n#define SAI_MINOR 19\n#define SAI_REVISION 2\n", encoding="utf-8",
        )

    def build_runtime(self, compiled, vendor, status=0, symbol=True, directory=None):
        directory = directory or self.root
        directory.mkdir(parents=True, exist_ok=True)
        metadata = directory / "metadata.c"
        metadata.write_text(
            f"#include <stdint.h>\nuint64_t sai_metadata_query_api_version(void) {{ return {compiled}; }}\n",
            encoding="utf-8",
        )
        library = directory / "vendor.c"
        library.write_text(
            "#include <stdint.h>\n"
            + (f"int32_t sai_query_api_version(uint64_t *v) {{ *v = {vendor}; return {status}; }}\n"
               if symbol else "int missing_api(void) { return 0; }\n"),
            encoding="utf-8",
        )
        main = directory / "syncd.c"
        main.write_text("int main(void) { return 0; }\n", encoding="utf-8")
        commands = [
            ["gcc", "-shared", "-fPIC", "-Wl,-soname,libsaimetadata.so.0", str(metadata),
             "-o", str(directory / "libsaimetadata.so.0")],
            ["gcc", "-shared", "-fPIC", "-Wl,-soname,libsai.so.1", str(library),
             "-o", str(directory / "libsai.so.1")],
            ["gcc", str(main), "-Wl,--no-as-needed", "-L" + str(directory),
             "-Wl,-rpath," + str(directory), "-l:libsaimetadata.so.0", "-l:libsai.so.1",
             "-o", str(directory / "syncd")],
        ]
        for command in commands:
            subprocess.run(command, check=True, capture_output=True)
        return directory / "syncd"

    def run_probe(self, compiled, vendor, expected=11902, status=0, symbol=True):
        syncd = self.build_runtime(compiled, vendor, status, symbol)
        return subprocess.run(
            ["python3", str(ROOT / "scripts/check_sai_api_version.py"),
             "--expected-version", str(expected), "--syncd", str(syncd)],
            capture_output=True, text=True,
        )

    def test_matching_versions_pass_without_switch_initialization(self):
        result = self.run_probe(11902, 11902)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("metadata=11902, vendor=11902", result.stdout)

    def test_header_upgrade_with_unchanged_vendor_fails(self):
        result = self.run_probe(11902, 11801)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("vendor=11801", result.stdout)
        self.assertIn("must be equal", result.stderr)

    def test_patch_version_difference_is_rejected(self):
        self.assertNotEqual(self.run_probe(11800, 11801, expected=11800).returncode, 0)

    def test_old_matching_cached_libraries_do_not_match_new_header(self):
        self.assertNotEqual(self.run_probe(11801, 11801).returncode, 0)

    def test_minimum_version_is_preserved(self):
        result = self.run_probe(10800, 10800, expected=10800)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("at least 1.9.0", result.stderr)

    def test_vendor_query_error_is_rejected(self):
        result = self.run_probe(11902, 11902, status=-1)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("status=-1", result.stderr)

    def test_missing_api_is_rejected(self):
        self.assertNotEqual(self.run_probe(11902, 11902, symbol=False).returncode, 0)

    def test_missing_linked_library_is_rejected(self):
        syncd = self.build_runtime(11902, 11902)
        (self.root / "libsai.so.1").unlink()
        with self.assertRaises(gate.VersionCheckError):
            gate.check_runtime(11902, syncd)

    def test_invalid_header_does_not_default_to_a_version(self):
        self.header.write_text("#define SAI_MAJOR 1\n#define SAI_MINOR 19\n", encoding="utf-8")
        with self.assertRaises(gate.VersionCheckError):
            gate.header_version(self.header)

    def test_timeout_removes_only_its_probe_container(self):
        def run(command, **kwargs):
            if command[1] == "inspect":
                return subprocess.CompletedProcess(command, 0, "sha256:" + "a" * 64 + "\n")
            if command[1] == "run":
                raise subprocess.TimeoutExpired(command, 60)
            return subprocess.CompletedProcess(command, 0)

        with mock.patch.object(gate.subprocess, "run", side_effect=run) as execute:
            with self.assertRaises(subprocess.TimeoutExpired):
                gate.check_image("test-image", 11902)
        run_command = execute.call_args_list[1].args[0]
        cleanup = execute.call_args_list[2].args[0]
        self.assertEqual(cleanup, ["docker", "rm", "--force", "--volumes", run_command[3]])
        self.assertIn("sha256:" + "a" * 64, run_command)

    def make_gate(self, compiled, vendor, images, legacy_vendor=None, legacy_status=0, legacy_symbol=True):
        syncd = self.build_runtime(compiled, vendor)
        legacy = self.build_runtime(
            compiled, vendor if legacy_vendor is None else legacy_vendor,
            status=legacy_status, symbol=legacy_symbol, directory=self.root / "legacy",
        )
        (self.root / "target").mkdir()
        for image in images:
            (self.root / "target" / image).write_bytes(b"existing cached archive")
        (self.root / "target/broadcom-sai-api-version-check").write_bytes(b"previous result")
        for image in BROADCOM_IMAGES:
            (self.root / "target" / (image + "-sai-api-check")).write_bytes(b"previous result")
        source = self.root / "src/sonic-sairedis/SAI/inc"
        source.mkdir(parents=True)
        (source / "saiversion.h").write_text(self.header.read_text(), encoding="utf-8")
        binary = self.root / "bin"
        binary.mkdir()
        docker = binary / "docker"
        docker.write_text(
            "#!/usr/bin/env python3\n"
            "import os, subprocess, sys\n"
            "from pathlib import Path\n"
            "with open(os.environ['DOCKER_LOG'], 'a') as log: log.write(' '.join(sys.argv[1:]) + '\\n')\n"
            "if sys.argv[1] == 'inspect':\n"
            "    print('sha256:' + ('b' if 'legacy-th' in sys.argv[-1] else 'a') * 64)\n"
            "if sys.argv[1] == 'run':\n"
            "    args = sys.argv[sys.argv.index('--expected-version'):]\n"
            "    image = sys.argv[sys.argv.index('--entrypoint') + 2]\n"
            "    syncd = os.environ['LEGACY_SYNCD' if image == 'sha256:' + 'b' * 64 else 'FAKE_SYNCD']\n"
            "    sys.exit(subprocess.run(['python3', '-'] + args + ['--syncd', syncd],\n"
            "                            input=sys.stdin.read(), text=True).returncode)\n",
            encoding="utf-8",
        )
        docker.chmod(0o755)
        slave = (ROOT / "slave.mk").read_text(encoding="utf-8")
        cache = (ROOT / "Makefile.cache").read_text(encoding="utf-8")

        def definition(text, name):
            return "define " + name + text.split("define " + name, 1)[1].split("\nendef", 1)[0] + "\nendef\n"

        check_rules = "BROADCOM_SAI_API_CHECK_IMAGES :=" + slave.split("BROADCOM_SAI_API_CHECK_IMAGES :=", 1)[1].split(
            "\n###############################################################################", 1,
        )[0]
        makefile = self.root / "Makefile"
        makefile.write_text(
            ".ONESHELL:\nSHELL := /bin/bash\n.SHELLFLAGS := -e -c\n"
            "TARGET_PATH := target\nSRC_PATH := src\nDOCKER_USERNAME := tester\nDOCKER_USERTAG := build\n"
            f"DOCKER_LOCKDIR := {self.root}\nDOCKER_LOCKFILE_SUFFIX := access\n"
            "DOCKER_LOCKFILE_TIMEOUT := 2\ndocker-get-tag = latest\n"
            "HEADER :=\nFOOTER :=\nLOG :=\nSONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD := n\n"
            + definition(cache, "MOD_LOCK") + definition(cache, "MOD_UNLOCK")
            + definition(slave, "docker-image-load") + check_rules
            + "\n.PHONY: docker-start\ndocker-start:\n\t@true\n"
            + "\ntarget/%.gz:\n\t@touch UNEXPECTED_REBUILD\n",
            encoding="utf-8",
        )
        (self.root / "scripts").symlink_to(ROOT / "scripts", target_is_directory=True)
        env = dict(os.environ, PATH=str(binary) + ":" + os.environ["PATH"],
                   FAKE_SYNCD=str(syncd), LEGACY_SYNCD=str(legacy), DOCKER_LOG=str(self.root / "docker.log"))
        return subprocess.run(
            ["make", "-s", "target/broadcom-sai-api-version-check", "BUILD_SKIP_TEST=y"],
            cwd=self.root, env=env, text=True, capture_output=True,
        )

    def test_cached_and_existing_targets_check_only_three_broadcom_binaries(self):
        images = BROADCOM_IMAGES + ["docker-syncd-brcm-dbg.gz", "docker-syncd-brcm-rpc.gz", "docker-syncd-mlnx.gz"]
        result = self.make_gate(11902, 11902, images)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.count("metadata=11902, vendor=11902"), 3)
        loads = [line for line in (self.root / "docker.log").read_text().splitlines() if line.startswith("load ")]
        self.assertEqual(loads, ["load -i target/" + image for image in BROADCOM_IMAGES])
        self.assertFalse((self.root / "UNEXPECTED_REBUILD").exists())

    def test_cached_bad_vendor_fails_even_with_skip_tests_enabled(self):
        result = self.make_gate(11902, 11801, BROADCOM_IMAGES)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout.count("vendor=11801"), 3)
        self.assertFalse((self.root / "UNEXPECTED_REBUILD").exists())

    def test_all_three_cached_libraries_matching_each_other_but_not_header_fail(self):
        result = self.make_gate(11801, 11801, BROADCOM_IMAGES)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout.count("header=11902, metadata=11801, vendor=11801"), 3)

    def test_one_vendor_revision_mismatch_still_checks_the_other_binaries(self):
        result = self.make_gate(11902, 11902, BROADCOM_IMAGES, legacy_vendor=11901)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout.count("vendor=11902"), 2)
        self.assertEqual(result.stdout.count("vendor=11901"), 1)

    def test_one_vendor_query_failure_still_checks_the_other_binaries(self):
        result = self.make_gate(11902, 11902, BROADCOM_IMAGES, legacy_status=-1)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("status=-1", result.stderr)
        self.assertEqual(result.stdout.count("vendor=11902"), 2)

    def test_one_vendor_missing_symbol_still_checks_the_other_binaries(self):
        result = self.make_gate(11902, 11902, BROADCOM_IMAGES, legacy_symbol=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("sai_query_api_version", result.stderr)
        self.assertEqual(result.stdout.count("vendor=11902"), 2)

    def test_missing_required_variant_fails(self):
        result = self.make_gate(11902, 11902, [BROADCOM_IMAGES[0], BROADCOM_IMAGES[2]])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing required SAI artifact: docker-syncd-brcm-legacy-th.gz", result.stderr)
        self.assertEqual(result.stdout.count("vendor=11902"), 2)
        self.assertFalse((self.root / "UNEXPECTED_REBUILD").exists())


if __name__ == "__main__":
    unittest.main()
