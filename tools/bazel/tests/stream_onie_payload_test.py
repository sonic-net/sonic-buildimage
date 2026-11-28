"""Validate streamed ONIE payloads against the existing tar and header readers."""

import hashlib
import io
import os
from pathlib import Path
import re
import runpy
import shutil
import subprocess
import tarfile
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[3]
write_payload = runpy.run_path(str(ROOT / "scripts/stream_onie_payload.py"))["write_payload"]


def read_payload(path):
    data = path.read_bytes()
    boundary = data.index(b"\nexit_marker\n") + len(b"\nexit_marker\n")
    header = data[:boundary]
    size = int(re.search(rb"^payload_image_size=(\d+)$", header, re.M)[1])
    checksum = re.search(rb"^payload_sha1=([0-9a-f]{40})$", header, re.M)[1].decode()
    payload = data[boundary:boundary + size]
    if len(payload) != size or hashlib.sha1(payload).hexdigest() != checksum:
        raise ValueError("Invalid ONIE checksum or length")
    return header, payload


class StreamOniePayloadTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.work = Path(temporary.name)
        self.installer = self.work / "installer"
        self.installer.mkdir()
        self.template = ROOT / "installer/sharch_body.sh"

    def test_tar_bytes_metadata_and_real_shell_reader_are_preserved(self):
        payload = self.installer / "fs.zip"
        payload.write_bytes(b"payload" * 10000)
        payload.chmod(0o640)
        (self.installer / "link").symlink_to("fs.zip")
        os.link(payload, self.installer / "hardlink")
        original = subprocess.check_output(["tar", "-C", str(self.work), "-cf", "-", "installer"])
        expected_header = self.work / "copied-header"
        subprocess.run(["cp", str(self.template), str(expected_header)], check=True)
        output = self.work / "image.bin"
        write_payload(self.work, self.template, output)
        header, actual = read_payload(output)
        self.assertEqual(actual, original)
        self.assertEqual(len(header), 4096)
        subprocess.run(["sh", "-n"], input=header, check=True, capture_output=True)
        reader = subprocess.check_output(
            ["sed", "-e", "1,/^exit_marker$/d", str(output)])
        self.assertEqual(reader, original)
        with tarfile.open(fileobj=io.BytesIO(actual)) as archive:
            member = archive.getmember("installer/fs.zip")
            self.assertEqual(member.mode, 0o640)
            self.assertEqual((member.uid, member.gid), (os.getuid(), os.getgid()))
        self.assertEqual(output.stat().st_mode & 0o777, expected_header.stat().st_mode & 0o777)

    def test_failures_leave_no_partial_output_and_preserve_previous_image(self):
        output = self.work / "image.bin"
        output.write_bytes(b"previous image")
        with self.assertRaises(RuntimeError):
            write_payload(self.work / "missing", self.template, output)
        self.assertEqual(output.read_bytes(), b"previous image")
        for body in (b"bad template\n", self.template.read_bytes() + b"trailing",
                     b"#" * 4096 + self.template.read_bytes()):
            template = self.work / "template"
            template.write_bytes(body)
            with self.assertRaises(ValueError):
                write_payload(self.work, template, output)
            self.assertEqual(output.read_bytes(), b"previous image")
        self.assertFalse(list(self.work.glob(".onie-payload-*")))
        with self.assertRaisesRegex(ValueError, "outside the payload tree"):
            write_payload(self.work, self.template, self.installer / "image.bin")
        self.assertFalse((self.installer / "image.bin").exists())

    def test_output_permissions_respect_umask_and_existing_output_mode(self):
        template = self.work / "executable-header"
        shutil.copyfile(self.template, template)
        template.chmod(0o755)
        for mask in (0o022, 0o077):
            output = self.work / ("image-" + str(mask))
            original = os.umask(mask)
            try:
                write_payload(self.work, template, output)
            finally:
                os.umask(original)
            self.assertEqual(output.stat().st_mode & 0o777, 0o755 & ~mask)
        output.chmod(0o750)
        write_payload(self.work, self.template, output)
        self.assertEqual(output.stat().st_mode & 0o777, 0o750)

    def test_signed_cross_non_vs_and_legacy_paths_do_not_select_streaming(self):
        source = (ROOT / "onie-mk-demo.sh").read_text()
        gate = source[source.index("tmp_dir=\n"):source.index("\nclean_up()")]
        env = dict(os.environ, arch="amd64", machine="vs", demo_type="OS",
                   DOCKER_LOAD_ON_BOOT="y", SECURE_UPGRADE_MODE="none",
                   SONIC_VS_STREAM_ONIE="1", TARGET_MACHINE="vs",
                   CROSS_BUILD_ENVIRON="n", MULTIARCH_QEMU_ENVIRON="n")
        for overrides in ({}, {"SECURE_UPGRADE_MODE": "dev"}, {"SECURE_UPGRADE_MODE": "prod"},
                          {"SECURE_UPGRADE_MODE": "custom"},
                          {"CROSS_BUILD_ENVIRON": "y"}, {"MULTIARCH_QEMU_ENVIRON": "y"},
                          {"machine": "mellanox"}, {"arch": "arm64"}, {"demo_type": "DIAG"},
                          {"DOCKER_LOAD_ON_BOOT": "n"}, {"SONIC_VS_STREAM_ONIE": "0"}):
            result = subprocess.check_output(
                ["sh", "-c", gate + '\nprintf "%s" "$stream_payload"'], env={**env, **overrides})
            self.assertEqual(result, b"y" if not overrides and os.uname().machine == "x86_64" else b"n")
        invalid = subprocess.run(["sh", "-c", gate], env={**env, "SONIC_VS_STREAM_ONIE": "invalid"},
                                 capture_output=True)
        self.assertNotEqual(invalid.returncode, 0)

    @unittest.skipUnless(os.uname().machine == "x86_64", "Native VS wrapper requires AMD64")
    def test_actual_wrapper_handles_embedded_sidecar_and_late_signing(self):
        scripts = self.work / "scripts"
        scripts.mkdir()
        shutil.copyfile(ROOT / "scripts/stream_onie_payload.py", scripts / "stream_onie_payload.py")
        wrapper = self.work / "onie-mk-demo.sh"
        shutil.copyfile(ROOT / "onie-mk-demo.sh", wrapper)
        shutil.copyfile(self.template, self.installer / "sharch_body.sh")
        (self.installer / "install.sh").write_text(
            "#!/bin/sh\n# %%DEMO_TYPE%% %%ARCH%% %%IMAGE_VERSION%%\n"
            "# %%ONIE_IMAGE_PART_SIZE%% %%EXTRA_CMDLINE_LINUX%% %%OUTPUT_RAW_IMAGE%%\n")
        (self.work / "onie-image-amd64.conf").write_text("# architecture settings\n")
        platform = self.work / "platform.conf"
        platform.write_text("platform=test\n")
        temporary = self.work / "temporary"
        temporary.mkdir()
        env = dict(os.environ, SONIC_VS_STREAM_ONIE="1", DOCKER_LOAD_ON_BOOT="y",
                   TARGET_MACHINE="vs", SECURE_UPGRADE_MODE="none", TMPDIR=str(temporary),
                   CROSS_BUILD_ENVIRON="n", MULTIARCH_QEMU_ENVIRON="n", ENABLE_FIPS="n",
                   EXTRA_CMDLINE_LINUX="")
        for name, embedded, late_mode, late_load in (
                ("embedded", True, None, None), ("sidecar", False, None, None),
                ("signed", True, "dev", None), ("custom", True, "custom", None),
                ("legacy", True, None, "n")):
            payload = self.work / "fs.zip"
            with zipfile.ZipFile(payload, "w") as archive:
                archive.writestr("fs.squashfs", b"filesystem")
                if embedded:
                    archive.writestr("sonic-images.tar.gz", b"bundle")
            (self.work / "sonic-images.tar.gz").write_bytes(b"sidecar bundle")
            config = ("OUTPUT_RAW_IMAGE=target/sonic-$TARGET_MACHINE.raw\n"
                      "FILESYSTEM_DOCKERFS=sonic-images.tar.gz\n")
            if late_mode:
                config += f"SECURE_UPGRADE_MODE={late_mode}\n"
            if late_load:
                config += f"DOCKER_LOAD_ON_BOOT={late_load}\n"
            if late_mode == "dev":
                signer = scripts / "sign_image_dev.sh"
                signer.write_text('#!/bin/sh\nprintf signature > "$4"\n')
                signer.chmod(0o755)
                sudo = scripts / "sudo"
                sudo.write_text('#!/bin/sh\nexec "$@"\n')
                sudo.chmod(0o755)
            (self.work / "onie-image.conf").write_text(config)
            output = self.work / (name + ".bin")
            result = subprocess.run(
                ["sh", str(wrapper), "amd64", "vs", "x86_64-vs", str(self.installer),
                 str(platform), str(output), "OS", "test-version", "32768",
                 str(payload), "none", "none"], cwd=self.work,
                env={**env, "PATH": str(scripts) + os.pathsep + env["PATH"]},
                capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            header, data = read_payload(output)
            self.assertEqual(len(header) == 4096, late_mode is None and late_load is None)
            if late_mode == "dev":
                self.assertTrue(output.read_bytes().endswith(b"signature"))
            with tarfile.open(fileobj=io.BytesIO(data)) as archive:
                self.assertEqual(archive.extractfile("installer/fs.zip").read(), payload.read_bytes())
                self.assertEqual("installer/sonic-images.tar.gz" in archive.getnames(), not embedded)
                self.assertIn(b"target/sonic-vs.raw", archive.extractfile("installer/install.sh").read())
            self.assertFalse(list(temporary.iterdir()))
            self.assertFalse(list(self.work.glob(".onie-payload-*")))


if __name__ == "__main__":
    unittest.main()
