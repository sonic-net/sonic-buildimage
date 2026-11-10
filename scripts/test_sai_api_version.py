"""Exercise Broadcom binary-only API version comparison with real ELF fixtures."""

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

    def build_runtime(self, vendor, compiled=11902, status=0, symbol=True, directory=None):
        directory = directory or self.root
        directory.mkdir(parents=True, exist_ok=True)
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
            ["gcc", "-shared", "-fPIC", "-Wl,-soname,libsai.so.1", str(library),
             "-o", str(directory / "libsai.so.1")],
        ]
        metadata_link = []
        if compiled is not None:
            metadata = directory / "metadata.c"
            metadata.write_text(
                f"#include <stdint.h>\nuint64_t sai_metadata_query_api_version(void) {{ return {compiled}; }}\n",
                encoding="utf-8",
            )
            commands.append([
                "gcc", "-shared", "-fPIC", "-Wl,-soname,libsaimetadata.so.0", str(metadata),
                "-o", str(directory / "libsaimetadata.so.0"),
            ])
            metadata_link = ["-l:libsaimetadata.so.0"]
        commands.append([
            "gcc", str(main), "-Wl,--no-as-needed", "-L" + str(directory),
            "-Wl,-rpath," + str(directory), "-l:libsai.so.1",
        ] + metadata_link + ["-o", str(directory / "syncd")])
        for command in commands:
            subprocess.run(command, check=True, capture_output=True)
        return directory / "syncd"

    def run_probe(self, vendor, **kwargs):
        syncd = self.build_runtime(vendor, **kwargs)
        return subprocess.run(
            ["python3", str(ROOT / "scripts/check_sai_api_version.py"), "--syncd", str(syncd)],
            capture_output=True, text=True,
        )

    def test_vendor_query_does_not_compare_metadata(self):
        result = self.run_probe(11801, compiled=11902)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "SAI_API_VERSION=11801")

    def test_query_does_not_require_metadata_library(self):
        result = self.run_probe(11801, compiled=None)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_vendor_query_error_is_rejected(self):
        result = self.run_probe(11801, status=-1)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("status=-1", result.stderr)

    def test_missing_api_is_rejected(self):
        result = self.run_probe(11801, symbol=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("sai_query_api_version", result.stderr)

    def test_missing_linked_library_is_rejected(self):
        syncd = self.build_runtime(11801)
        (self.root / "libsai.so.1").unlink()
        with self.assertRaises(gate.VersionCheckError):
            gate.query_runtime(syncd)

    def test_timeout_removes_only_its_probe_container(self):
        def run(command, **kwargs):
            if command[1] == "inspect":
                return subprocess.CompletedProcess(command, 0, "sha256:" + "a" * 64 + "\n")
            if command[1] == "run":
                raise subprocess.TimeoutExpired(command, 60)
            return subprocess.CompletedProcess(command, 0)

        with mock.patch.object(gate.subprocess, "run", side_effect=run) as execute:
            with self.assertRaises(subprocess.TimeoutExpired):
                gate.query_image("test-image")
        run_command = execute.call_args_list[1].args[0]
        cleanup = execute.call_args_list[2].args[0]
        self.assertEqual(cleanup, ["docker", "rm", "--force", "--volumes", run_command[3]])
        self.assertIn("sha256:" + "a" * 64, run_command)

    def test_invalid_or_duplicate_query_output_is_rejected(self):
        for output in ("", "SAI_API_VERSION=0\n", "SAI_API_VERSION=11801\nSAI_API_VERSION=11801\n"):
            with self.subTest(output=output):
                def run(command, **kwargs):
                    if command[1] == "inspect":
                        return subprocess.CompletedProcess(command, 0, "sha256:" + "a" * 64 + "\n")
                    return subprocess.CompletedProcess(command, 0, output)

                with mock.patch.object(gate.subprocess, "run", side_effect=run):
                    with self.assertRaises(gate.VersionCheckError):
                        gate.query_image("test-image")

    def make_gate(self, versions=(11801, 11801, 11801), images=None, compiled=11902,
                  legacy_status=0, legacy_symbol=True, header=True, empty=None, parallel=False):
        runtimes = [
            self.build_runtime(version, compiled=compiled, directory=self.root / str(index),
                               status=legacy_status if index == 1 else 0,
                               symbol=legacy_symbol if index == 1 else True)
            for index, version in enumerate(versions)
        ]
        (self.root / "target").mkdir()
        for image in BROADCOM_IMAGES if images is None else images:
            (self.root / "target" / image).write_bytes(b"" if image == empty else b"existing cached archive")
        (self.root / "target/broadcom-sai-api-version-check").write_bytes(b"previous result")
        for image in BROADCOM_IMAGES:
            (self.root / "target" / (image + "-sai-api-check")).write_bytes(b"previous result")
        if header:
            source = self.root / "src/sonic-sairedis/SAI/inc"
            source.mkdir(parents=True)
            (source / "saiversion.h").write_text(
                "#define SAI_MAJOR 1\n#define SAI_MINOR 19\n#define SAI_REVISION 2\n", encoding="utf-8",
            )
        binary = self.root / "bin"
        binary.mkdir()
        docker = binary / "docker"
        docker.write_text(
            "#!/usr/bin/env python3\n"
            "import os, subprocess, sys\n"
            "with open(os.environ['DOCKER_LOG'], 'a') as log: log.write(' '.join(sys.argv[1:]) + '\\n')\n"
            "if sys.argv[1] == 'inspect':\n"
            "    tag = sys.argv[-1]\n"
            "    key = 'b' if 'legacy-th' in tag else 'c' if 'dnx' in tag else 'a'\n"
            "    print('sha256:' + key * 64)\n"
            "if sys.argv[1] == 'run':\n"
            "    image = sys.argv[sys.argv.index('--entrypoint') + 2]\n"
            "    syncd = os.environ['FAKE_SYNCD_' + image.split(':')[1][0]]\n"
            "    sys.exit(subprocess.run(['python3', '-', '--syncd', syncd],\n"
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
        (self.root / "Makefile").write_text(
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
                   DOCKER_LOG=str(self.root / "docker.log"), TMPDIR=str(self.root))
        env.update({"FAKE_SYNCD_" + key: str(syncd) for key, syncd in zip("abc", runtimes)})
        result = subprocess.run(
            ["make", "-s", "-j3" if parallel else "-j1",
             "target/broadcom-sai-api-version-check", "BUILD_SKIP_TEST=y"],
            cwd=self.root, env=env, text=True, capture_output=True,
        )
        self.assertFalse((self.root / "UNEXPECTED_REBUILD").exists())
        self.assertEqual(list(self.root.glob("tmp*")), [], "Probe result directory was not removed")
        return result

    def test_matching_binaries_pass_despite_different_header_and_metadata(self):
        result = self.make_gate()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.count("vendor=11801"), 3)

    def test_no_source_header_or_metadata_is_required(self):
        result = self.make_gate(compiled=None, header=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_cached_and_existing_targets_check_only_three_broadcom_binaries(self):
        images = BROADCOM_IMAGES + ["docker-syncd-brcm-dbg.gz", "docker-syncd-brcm-rpc.gz", "docker-syncd-mlnx.gz"]
        result = self.make_gate(images=images, parallel=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        loads = [line for line in (self.root / "docker.log").read_text().splitlines() if line.startswith("load ")]
        self.assertCountEqual(loads, ["load -i target/" + image for image in BROADCOM_IMAGES])
        self.assertEqual(result.stdout.count("vendor=11801"), 3)

    def test_xgs_mismatch_is_rejected(self):
        result = self.make_gate(versions=(11902, 11801, 11801))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("must be equal", result.stderr)
        self.assertEqual(result.stdout.count("vendor="), 3)

    def test_legacy_revision_mismatch_is_rejected(self):
        result = self.make_gate(versions=(11801, 11800, 11801))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("must be equal", result.stderr)
        self.assertEqual(result.stdout.count("vendor="), 3)

    def test_dnx_mismatch_is_rejected(self):
        result = self.make_gate(versions=(11801, 11801, 11902))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("must be equal", result.stderr)
        self.assertEqual(result.stdout.count("vendor="), 3)

    def test_query_failure_still_checks_the_other_binaries(self):
        result = self.make_gate(legacy_status=-1)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("status=-1", result.stderr)
        self.assertEqual(result.stdout.count("vendor=11801"), 2)

    def test_missing_symbol_still_checks_the_other_binaries(self):
        result = self.make_gate(legacy_symbol=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("sai_query_api_version", result.stderr)
        self.assertEqual(result.stdout.count("vendor=11801"), 2)

    def test_missing_required_artifact_does_not_rebuild_or_pass(self):
        result = self.make_gate(images=[BROADCOM_IMAGES[0], BROADCOM_IMAGES[2]])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing required SAI artifact: docker-syncd-brcm-legacy-th.gz", result.stderr)
        self.assertEqual(result.stdout.count("vendor=11801"), 2)

    def test_empty_required_artifact_does_not_pass(self):
        result = self.make_gate(empty=BROADCOM_IMAGES[1])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing required SAI artifact: docker-syncd-brcm-legacy-th.gz", result.stderr)


if __name__ == "__main__":
    unittest.main()
