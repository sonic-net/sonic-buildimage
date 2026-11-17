"""Exercise MACsec artifact isolation and publication in the shared OCI workflow."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[3]


class MacsecWorkflowTest(unittest.TestCase):
    def setUp(self):
        self.jobs = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())["jobs"]
        self.steps = self.jobs["archive"]["steps"]

    def step(self, name):
        return next(step for step in self.steps if step.get("name") == name)

    def publish(self, directory):
        step = self.step("Review public MACsec evidence")
        script = step["run"].split("python3 - <<'PYMACSEC'\n", 1)[1].split("\nPYMACSEC", 1)[0]
        previous = Path.cwd()
        sys.path.insert(0, str(ROOT))
        try:
            os.chdir(directory)
            exec(compile(script, "<macsec-public-evidence>", "exec"), {})
        finally:
            os.chdir(previous)
            sys.path.pop(0)

    def test_preservation_keeps_prior_evidence_and_shared_download_configuration(self):
        upload = self.step("Retain Orchagent archives and source evidence")
        preserve = self.step("Preserve completed Orchagent evidence before MACsec acceptance")
        build = self.step("Build MACsec runtime and debug images from source owners")
        self.assertLess(self.steps.index(upload), self.steps.index(preserve))
        self.assertLess(self.steps.index(preserve), self.steps.index(build))
        self.assertEqual(preserve["if"], "matrix.architecture == 'amd64'")
        with tempfile.TemporaryDirectory(prefix="macsec shared evidence ") as temporary:
            root = Path(temporary)
            scratch = root / "scratch"
            scratch.mkdir()
            mirror = scratch / "sonic-source-mirrors.cfg"
            mirror.write_text("shared mirror configuration\n")
            rc = root / ".bazelrc.user"
            rc.write_text("common --downloader_config=" + str(mirror) + "\nbuild --disk_cache=/shared/cache\n")
            original_rc = rc.read_bytes()

            def run(directory=scratch):
                return subprocess.run(["bash", "-e", "-o", "pipefail", "-c", preserve["run"]],
                                      cwd=root, capture_output=True, text=True,
                                      env=dict(os.environ, RUNNER_TEMP=str(directory)))

            for value in ("first", "second"):
                previous = root / "artifacts/orchagent"
                previous.mkdir(parents=True)
                (previous / "receipt.json").write_text(value)
                result = run()
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse((root / "artifacts").exists())
                self.assertEqual(rc.read_bytes(), original_rc)
                self.assertEqual(mirror.read_text(), "shared mirror configuration\n")
            receipts = list(scratch.glob("pre-macsec-artifacts.*/artifacts/orchagent/receipt.json"))
            self.assertCountEqual([path.read_text() for path in receipts], ["first", "second"])
            for path in receipts:
                self.assertEqual(path.parents[2].stat().st_mode & 0o777, 0o700)
            self.assertEqual(run().returncode, 0)
            previous.mkdir(parents=True)
            (previous / "receipt.json").write_text("keep on failure")
            self.assertNotEqual(run(root / "missing").returncode, 0)
            self.assertEqual((previous / "receipt.json").read_text(), "keep on failure")

    def test_amd64_runtime_uses_the_same_run_artifacts_without_building(self):
        self.assertFalse((ROOT / ".github/workflows/bazel-macsec.yml").exists())
        for name in ("Build MACsec runtime and debug images from source owners",
                     "Validate MACsec installed payloads and matching symbols",
                     "Verify MACsec no-op, configuration edit and restored archives"):
            step = self.step(name)
            self.assertEqual(step["if"], "matrix.architecture == 'amd64'")
            self.assertEqual(step["env"]["CI_HEAD_SHA"], "${{ steps.checkout_identity.outputs.head_sha }}")
            self.assertEqual(step["env"]["CI_BASE_SHA"], "${{ steps.checkout_identity.outputs.base_sha }}")
            if " validate " not in step["run"]:
                self.assertIn('--bazel "$BAZEL"', step["run"])
        runtime = self.jobs["macsec_runtime"]
        self.assertEqual(runtime["needs"], "archive")
        upload = self.step("Retain MACsec archives and reviewed evidence")["with"]
        download = next(step["with"] for step in runtime["steps"]
                        if "download-artifact@" in step.get("uses", ""))
        self.assertEqual(upload["name"], download["name"])
        self.assertIn("${{ github.sha }}", download["name"])
        self.assertEqual(download["path"], "artifacts/macsec")
        self.assertNotIn("run-id", download)
        for name in ("docker-macsec.gz", "docker-macsec-dbg.gz", "test-redis.tar",
                     "config-engine.source.tar", "validation.json", "docker-macsec.validation.json",
                     "docker-macsec-dbg.validation.json"):
            self.assertTrue(any((Path("artifacts/macsec") / name).match(pattern)
                                for pattern in upload["path"].splitlines()), name)
        scripts = "\n".join(step.get("run", "") for step in runtime["steps"])
        self.assertIn("checkout_identity.py", scripts)
        self.assertIn("macsec_loaded.py --artifacts artifacts/macsec", scripts)
        self.assertNotIn("macsec_image.py", scripts)
        self.assertFalse(any("setup-bazel@" in step.get("uses", "") for step in runtime["steps"]))

    def test_publication_handles_early_failure_and_omits_private_commands(self):
        with tempfile.TemporaryDirectory(prefix="macsec public failure ") as temporary:
            directory = Path(temporary)
            self.publish(directory)
            root = directory / "artifacts/macsec"
            self.assertEqual(json.loads((root / "build-summary.json").read_text()), {})
            receipt = {"status": "failed", "architecture": "amd64", "revision": "a" * 40,
                       "artifacts": {}, "commands": [["private diagnostics"]]}
            (root / "build-receipt.json").write_text(json.dumps(receipt))
            self.publish(directory)
            self.assertEqual(json.loads((root / "public-receipt.json").read_text()),
                             {key: value for key, value in receipt.items() if key != "commands"})

    def test_publication_rejects_unsafe_or_malformed_validation(self):
        for contents in ("{malformed", '{"environment": {"PRIVATE": "value"}}'):
            with self.subTest(contents=contents), tempfile.TemporaryDirectory(prefix="macsec unsafe ") as temporary:
                directory = Path(temporary)
                root = directory / "artifacts/macsec"
                root.mkdir(parents=True)
                (root / "validation.json").write_text(contents)
                with self.assertRaises(ValueError):
                    self.publish(directory)
                self.assertFalse((root / "build-summary.json").exists())


if __name__ == "__main__":
    unittest.main()
