import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

import yaml


PIPELINE = Path(__file__).with_name("azure-pipelines-build.yml")


class SecureBootVSBuildTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / "rules").mkdir()
        (self.root / "rules/config.user").write_text("EXISTING_OPTION := y\n")
        (self.root / "target").mkdir()
        (self.root / "bin").mkdir()
        (self.root / "scripts").mkdir()
        parameters = yaml.load(
            PIPELINE.read_text(), Loader=yaml.BaseLoader
        )["jobs"][0]["parameters"]
        self.vars_template = parameters["jobVariables"]["secure_boot_vars_file"]
        steps = parameters["buildSteps"] + parameters["postSteps"]
        self.steps = {
            step["displayName"]: step for step in steps
            if isinstance(step, dict) and "bash" in step
        }
        self.variables = {
            "Build.SourcesDirectory": str(self.root),
            "Build.BuildId": "42",
            "System.JobAttempt": "1",
            "Build.Reason": "PullRequest",
            "GROUP_NAME": "vs",
            "dbg_image": "no",
            "asan_image": "no",
            "K8S_MASTER_CHANGED": "NO",
            "secure_boot_image": "yes",
        }
        self.variables["secure_boot_vars_file"] = self.expand(self.vars_template)
        self.workdir = Path(self.variables["secure_boot_vars_file"]).parent
        self.env = {
            **os.environ,
            "PATH": f"{self.root / 'bin'}:{os.environ['PATH']}",
            "BUILD_OPTIONS": "USERNAME=admin",
        }
        self.write_command("bin/sudo", "exit 0\n")
        self.write_command(
            "bin/python3",
            'mkdir -p "$3/bin"\nprintf "#!/bin/sh\\nexit 0\\n" > "$3/bin/python"\n'
            'chmod +x "$3/bin/python"\n',
        )
        self.write_command(
            "bin/make",
            """printf '%s\\n' "$*" >> make.log
case " $* " in
    *" target/sonic-vs.img.gz "*)
        printf BIOS > target/sonic-vs.img.gz
        if [ "${OMIT_UEFI_IMAGE:-no}" != yes ]; then
            printf UEFI > target/sonic-vs.img-uefi.img.gz
        fi
        if [ "${OMIT_UEFI_VARS:-no}" != yes ]; then
            printf SONiC-OS > target/sonic-vs.img-uefi_vars.fd
        fi
        case " $* " in
            *" BUILD_UEFI_VS=y "*)
                mv target/sonic-vs.img-uefi.img.gz target/sonic-vs.img.gz ;;
        esac
        ;;
esac
""",
        )
        self.write_command(
            "scripts/generate_secureboot_vs_artifacts.sh",
            """reuse_keys=no
while [ "$#" -gt 0 ]; do
    case "$1" in
        --workdir) workdir="$2"; shift 2 ;;
        --output-vars) output="$2"; shift 2 ;;
        --ovmf-vars) input="$2"; shift 2 ;;
        --build-id) shift 2 ;;
        --reuse-keys) reuse_keys=yes; shift ;;
        *) exit 2 ;;
    esac
done
if [ "$reuse_keys" = yes ]; then
    cat "$input" "$workdir/DB.crt" > "$output"
else
    mkdir -p "$workdir"
    printf private > "$workdir/DB.key"
    printf public > "$workdir/DB.crt"
    printf auth > "$workdir/DB.auth"
    printf initial-vars > "$output"
fi
""",
        )

    def write_command(self, name, script):
        path = self.root / name
        path.write_text("#!/bin/bash\nset -eu\n" + script)
        path.chmod(0o755)

    def expand(self, script):
        return re.sub(
            r"\$\(([A-Za-z0-9_.]+)\)",
            lambda match: self.variables.get(match[1], match[0]),
            script,
        )

    def run_step(self, name):
        return subprocess.run(
            ["bash", "-c", self.expand(self.steps[name]["bash"])],
            cwd=self.root,
            env=self.env,
            capture_output=True,
            text=True,
        )

    def prepare(self):
        result = self.run_step("Generate ephemeral Secure Boot hierarchy")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_signed_build_keeps_both_images_and_matching_boot_variables(self):
        self.prepare()
        result = self.run_step("Build sonic image")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.root / "target/sonic-vs.img.gz").read_text(), "BIOS")
        self.assertEqual(
            (self.root / "target/sonic-vs.img-uefi.img.gz").read_text(), "UEFI"
        )
        self.assertEqual(
            (self.root / "target/sonic-vs_vars.fd").read_text(),
            "SONiC-OSpublic",
        )
        make_calls = (self.root / "make.log").read_text().splitlines()
        self.assertEqual(
            sum("target/sonic-vs.img.gz" in call for call in make_calls), 1
        )
        config = (self.root / "rules/config.user").read_text()
        self.assertTrue(config.startswith("EXISTING_OPTION := y\n"))
        self.assertIn("SECURE_UPGRADE_MODE := dev\n", config)
        self.assertIn(
            f"SECURE_UPGRADE_KERNEL_CAFILE := {self.workdir}/DB.crt\n",
            config,
        )

    def test_retry_uses_a_different_kernel_trust_input(self):
        self.prepare()
        first_workdir = self.workdir
        self.variables["System.JobAttempt"] = "2"
        self.variables["secure_boot_vars_file"] = self.expand(self.vars_template)
        self.workdir = Path(self.variables["secure_boot_vars_file"]).parent
        self.prepare()
        self.assertNotEqual(first_workdir, self.workdir)
        self.assertIn(
            f"SECURE_UPGRADE_KERNEL_CAFILE := {self.workdir}/DB.crt\n",
            (self.root / "rules/config.user").read_text(),
        )

    def test_unsigned_build_does_not_require_signing_inputs(self):
        self.variables["secure_boot_image"] = "no"
        result = self.run_step("Build sonic image")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.root / "target/sonic-vs_vars.fd").exists())
        self.assertFalse(self.workdir.exists())

    def test_signed_build_rejects_missing_uefi_artifacts(self):
        self.prepare()
        for missing in ("OMIT_UEFI_IMAGE", "OMIT_UEFI_VARS"):
            with self.subTest(missing=missing):
                for path in (self.root / "target").iterdir():
                    path.unlink()
                self.env[missing] = "yes"
                result = self.run_step("Build sonic image")
                self.env.pop(missing)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(
                    (self.root / "target/sonic-vs_vars.fd").exists()
                )

    def test_failed_build_cleans_private_keys_without_removing_artifacts(self):
        self.prepare()
        self.env["OMIT_UEFI_IMAGE"] = "yes"
        self.assertNotEqual(self.run_step("Build sonic image").returncode, 0)
        step = self.steps["Remove ephemeral Secure Boot private keys"]
        self.assertIn("always()", step["condition"])
        result = self.run_step("Remove ephemeral Secure Boot private keys")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(self.workdir.exists())
        self.assertTrue((self.root / "target/sonic-vs.img.gz").exists())


if __name__ == "__main__":
    unittest.main()
