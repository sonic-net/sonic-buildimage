import base64
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import yaml


PIPELINES = Path(__file__).resolve().parents[1]


def template_script(name):
    template = yaml.safe_load((PIPELINES / name).read_text())
    return next(step["script"] for step in template["steps"] if "script" in step)


class SigningMaterialTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.save_script = template_script("azure-pipelines-download-certificate.yml")
        cls.cleanup_script = template_script("azure-pipelines-cleanup-signing.yml")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            key = root / "key.pem"
            cert = root / "cert.pem"
            pfx = root / "cert.pfx"
            subprocess.run(
                ["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                 "-keyout", str(key), "-out", str(cert), "-days", "1",
                 "-subj", "/CN=signing-lifecycle-test"],
                check=True, capture_output=True,
            )
            subprocess.run(
                ["openssl", "pkcs12", "-export", "-inkey", str(key),
                 "-in", str(cert), "-out", str(pfx), "-passout", "pass:"],
                check=True, capture_output=True,
            )
            cls.certificate = base64.b64encode(pfx.read_bytes()).decode()
            cls.public_certificate = cert.read_bytes()

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.agent_temp = self.root / "agent temp"
        self.agent_temp.mkdir()
        self.public_cert = self.root / "artifacts" / "public cert.pem"
        self.env = dict(
            os.environ,
            AGENT_TEMPDIRECTORY=str(self.agent_temp),
            CERTIFICATE=self.certificate,
            PUBLIC_CERTIFICATE_PATH=str(self.public_cert),
        )

    def run_script(self, script, **env):
        return subprocess.run(
            ["bash", "-s"], input=script, text=True,
            env=dict(self.env, **env), capture_output=True, timeout=30,
        )

    def test_save_and_cleanup_preserve_public_certificate(self):
        result = self.run_script(self.save_script)
        self.assertEqual(result.returncode, 0, result.stderr)
        variables = dict(
            line.removeprefix("##vso[task.setvariable variable=").split("]", 1)
            for line in result.stdout.splitlines()
            if line.startswith("##vso[task.setvariable variable=")
        )
        signing_dir = Path(variables["SONIC_SIGNING_DIR"])
        key = Path(variables["SIGNING_KEY"])
        self.assertEqual(signing_dir.parent, self.agent_temp)
        self.assertEqual(signing_dir.stat().st_mode & 0o777, 0o700)
        self.assertEqual(key.stat().st_mode & 0o777, 0o600)
        self.assertEqual(variables["SIGNING_CERT"], str(self.public_cert))
        self.assertEqual(self.public_cert.read_bytes(), self.public_certificate)
        self.assertEqual(self.public_cert.stat().st_mode & 0o777, 0o644)
        self.assertFalse((signing_dir / "certificate.pfx").exists())
        subprocess.run(["openssl", "pkey", "-in", str(key), "-check", "-noout"],
                       check=True, capture_output=True)
        for _ in range(2):
            result = self.run_script(
                self.cleanup_script, SIGNING_MATERIAL_DIR=str(signing_dir),
            )
            self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(signing_dir.exists())
        self.assertEqual(self.public_cert.read_bytes(), self.public_certificate)

    def test_failure_does_not_leave_private_material(self):
        for certificate, destination in (
            ("not base64", self.public_cert),
            (base64.b64encode(b"not a PFX").decode(), self.public_cert),
            (self.certificate, self.root),
        ):
            with self.subTest(destination=destination):
                result = self.run_script(
                    self.save_script, CERTIFICATE=certificate,
                    PUBLIC_CERTIFICATE_PATH=str(destination),
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(list(self.agent_temp.iterdir()), [])

    def test_cleanup_refuses_unrelated_paths_and_symlinks(self):
        outside = self.root / "sonic-signing.outside"
        outside.mkdir()
        marker = outside / "keep"
        marker.touch()
        unrelated = self.agent_temp / "unrelated"
        unrelated.mkdir()
        symlink = self.agent_temp / "sonic-signing.symlink"
        symlink.symlink_to(outside, target_is_directory=True)
        for path in (outside, unrelated, symlink):
            with self.subTest(path=path):
                result = self.run_script(
                    self.cleanup_script, SIGNING_MATERIAL_DIR=str(path),
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertTrue(path.exists())
                self.assertTrue(marker.exists())

    def test_termination_removes_private_material(self):
        bin_dir = self.root / "bin"
        bin_dir.mkdir()
        decoder = bin_dir / "base64"
        decoder.write_text('#!/bin/bash\nkill -TERM "$PPID"\nexit 143\n')
        decoder.chmod(0o700)
        result = self.run_script(
            self.save_script, PATH=f"{bin_dir}:{os.environ['PATH']}",
        )
        self.assertEqual(result.returncode, 143, result.stderr)
        self.assertEqual(list(self.agent_temp.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
