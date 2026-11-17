"""Exercise version metadata from the real Make exports through installation."""

import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest

from jinja2 import Environment, StrictUndefined, UndefinedError
import yaml


ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location(
    "validate_sonic_version", ROOT / "scripts/validate_sonic_version.py")
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)

METADATA = {
    "build_version": "20261101.1-fixture",
    "debian_version": "13.1",
    "kernel_version": "6.12.41+deb13-sonic-amd64",
    "asic_type": "vs",
    "asic_subtype": "vs",
    "commit_id": "274052f20",
    "branch": "HEAD",
    "release": "",
    "build_date": "Sun Nov 1 00:00:00 UTC 2026",
    "build_number": "0",
    "built_by": "builder@fixture",
    "sonic_os_version": "13",
}


class SonicVersionTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="sonic-version-test-")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.template = Environment(undefined=StrictUndefined).from_string(
            (ROOT / "files/build_templates/sonic_version.yml.j2").read_text())

    def make_environment(self, setting=None, arguments=()):
        # File assignments deliberately do not inherit Make's command-line
        # auto-export behavior. Keep the actual quoting/normalization recipe.
        source = (ROOT / "slave.mk").read_text()
        end = source.index('\n\tj2 -f env files/initramfs-tools/union-mount.j2')
        start = source.rindex('\n\texport debs_path="$(IMAGE_DISTRO_DEBS_PATH)"', 0, end)
        exports = re.findall(
            r'^\texport (?:sonic_su_mode|SECURE_UPGRADE_MODE)=.*\n', source[start:end], re.M)
        default = re.search(r'^SECURE_UPGRADE_MODE \?=.*$',
                            (ROOT / "rules/config").read_text(), re.M).group()
        recorder = self.directory / "environment.py"
        recorder.write_text(
            'import json, os\nfrom pathlib import Path\n'
            'Path("environment.json").write_text(json.dumps({name: os.environ.get(name) '
            'for name in ("SECURE_UPGRADE_MODE", "sonic_su_mode")}))\n')
        makefile = self.directory / "Makefile"
        makefile.write_text(
            "SHELL := /bin/bash\n.SHELLFLAGS := -eu -c\n.ONESHELL:\n" + default + "\n" +
            ("SECURE_UPGRADE_MODE := " + setting + "\n" if setting is not None else "") +
            "probe:\n" + "".join(exports) +
            f"\t{shlex.quote(sys.executable)} {shlex.quote(str(recorder))}\n")
        result = subprocess.run(
            ["make", "--no-print-directory", "probe", *arguments], cwd=self.directory,
            env={"PATH": os.defpath, "LC_ALL": "C"}, text=True, capture_output=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return json.loads((self.directory / "environment.json").read_text())

    def render(self, mode="no_sign", asan="n"):
        return yaml.safe_load(self.template.render(
            dict(METADATA, SECURE_UPGRADE_MODE=mode, ENABLE_ASAN=asan)))

    def test_file_assigned_modes_reach_child_environment(self):
        for setting, expected in ((None, "no_sign"), ('"dev"', "dev"),
                                  ('"prod"', "prod"), ("no_sign", "no_sign")):
            with self.subTest(setting=setting):
                self.assertEqual(self.make_environment(setting),
                                 {"SECURE_UPGRADE_MODE": expected, "sonic_su_mode": expected})
        self.assertEqual(self.make_environment('"dev"', ("SECURE_UPGRADE_MODE=prod",)),
                         {"SECURE_UPGRADE_MODE": "prod", "sonic_su_mode": "prod"})

    def test_actual_template_signing_and_asan_modes(self):
        for mode in ("no_sign", "dev", "prod"):
            exported = self.make_environment('"' + mode + '"')
            for asan in ("n", "y"):
                with self.subTest(mode=mode, asan=asan):
                    data = self.render(exported["SECURE_UPGRADE_MODE"], asan)
                    validator.validate_version(data, mode)
                    self.assertEqual(data["secure_boot_image"], "no" if mode == "no_sign" else "yes")
                    self.assertEqual(data.get("asan"), "yes" if asan == "y" else None)
                    self.assertEqual(data["build_version"], METADATA["build_version"])
                    self.assertEqual(data["build_number"], 0)
                    self.assertEqual(data["sonic_os_version"], 13)
                    self.assertEqual(data["release"], "none")

    def test_missing_required_template_environment_fails(self):
        with self.assertRaises(UndefinedError):
            self.template.render(dict(METADATA, ENABLE_ASAN="n"))

    def test_validator_rejects_empty_and_nonmapping_data(self):
        for data in (None, {}, [], ["version"], "version", 0, False):
            with self.subTest(data=data), self.assertRaises(ValueError):
                validator.validate_version(data, "no_sign")

    def test_validator_requires_critical_string_fields(self):
        missing = object()
        for field in ("build_version", "asic_type", "kernel_version", "commit_id"):
            for value in (missing, None, "", " \t", 0, False):
                with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                    data = self.render()
                    if value is missing:
                        del data[field]
                    else:
                        data[field] = value
                    validator.validate_version(data, "no_sign")

    def test_validator_rejects_wrong_or_untyped_signing_flag(self):
        for mode, wrong in (("no_sign", "yes"), ("dev", "no"), ("prod", "no")):
            for value in (wrong, True, False, None):
                with self.subTest(mode=mode, value=value), self.assertRaises(ValueError):
                    data = self.render(mode)
                    data["secure_boot_image"] = value
                    validator.validate_version(data, mode)
        for mode in ("", "unknown"):
            with self.subTest(mode=mode), self.assertRaises(ValueError):
                validator.validate_version(self.render(), mode)

    def install_fixture(self, renderer="normal", payload="", mode="no_sign"):
        directory = self.directory / "installation"
        directory.mkdir()
        for name in ("scripts", "files/build_templates", "bin", "tmp", "root fs/etc/sonic"):
            (directory / name).mkdir(parents=True, exist_ok=True)
        for relative in ("scripts/validate_sonic_version.py", "files/build_templates/sonic_version.yml.j2"):
            shutil.copyfile(ROOT / relative, directory / relative)
        destination = directory / "root fs/etc/sonic/sonic_version.yml"
        destination.write_bytes(b"existing complete metadata\n")
        original_inode = destination.stat().st_ino
        # Use the actual installed j2 CLI when present (including runner CI).
        # The local fallback renders the same template with StrictUndefined.
        commands = {
            "j2": '''import os, pathlib, sys
mode = os.environ["VERSION_TEST_RENDERER"]
if mode == "failure":
    print("partial renderer output")
    sys.exit(41)
if mode == "payload":
    sys.stdout.write(os.environ["VERSION_TEST_PAYLOAD"])
elif os.environ["VERSION_TEST_REAL_J2"]:
    os.execv(os.environ["VERSION_TEST_REAL_J2"], ["j2", *sys.argv[1:]])
else:
    from jinja2 import Environment, StrictUndefined
    print(Environment(undefined=StrictUndefined).from_string(pathlib.Path(sys.argv[-1]).read_text()).render(os.environ))
''',
            "sudo": '''import json, pathlib, subprocess, sys
args = sys.argv[1:]
pathlib.Path("install-call.json").write_text(json.dumps(args))
if args[:5] != ["install", "-o", "root", "-g", "root"]:
    sys.exit(42)
# Exercise real install while keeping the test unprivileged.
sys.exit(subprocess.call(["/usr/bin/install", *args[5:]]))
''',
        }
        for name, body in commands.items():
            executable = directory / "bin" / name
            executable.write_text("#!" + sys.executable + "\n" + body)
            executable.chmod(0o755)
        python = directory / "bin/python3"
        python.write_text("#!/bin/sh\nexec " + shlex.quote(sys.executable) + ' "$@"\n')
        python.chmod(0o755)
        source = (ROOT / "build_debian.sh").read_text()
        start = source.index("sonic_version_tmp=$(mktemp)")
        end = source.index("\nif [ -f sonic_debian_extension.sh ]; then", start)
        # Execute the unchanged production block, with its real trap helper and
        # validator. Only sudo ownership and renderer faults are substituted.
        script = ('set -e\nsource "$SOURCE_ROOT/functions.sh"\n' + source[start:end] +
                  "\nprintf complete > completed\n")
        env = dict(METADATA, PATH=str(directory / "bin") + os.pathsep + os.defpath,
                   LC_ALL="C", SOURCE_ROOT=str(ROOT), TMPDIR=str(directory / "tmp"),
                   FILESYSTEM_ROOT=str(directory / "root fs"), SECURE_UPGRADE_MODE=mode,
                   ENABLE_ASAN="n", VERSION_TEST_RENDERER=renderer, VERSION_TEST_PAYLOAD=payload,
                   VERSION_TEST_REAL_J2=shutil.which("j2") or "")
        result = subprocess.run(["bash", "-c", script], cwd=directory, env=env,
                                text=True, capture_output=True, timeout=15)
        return result, directory, destination, original_inode

    def test_production_block_installs_valid_metadata(self):
        result, directory, destination, _ = self.install_fixture()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((directory / "completed").is_file())
        self.assertTrue((directory / "install-call.json").is_file())
        self.assertEqual(stat.S_IMODE(destination.stat().st_mode), 0o644)
        self.assertEqual(yaml.safe_load(destination.read_text()), self.render())
        self.assertEqual(list((directory / "tmp").iterdir()), [])

    def test_production_block_preserves_destination_on_renderer_failure(self):
        result, directory, destination, inode = self.install_fixture(renderer="failure")
        self.assertEqual(result.returncode, 41, result.stdout + result.stderr)
        self.assertFalse((directory / "install-call.json").exists())
        self.assertFalse((directory / "completed").exists())
        self.assertEqual(destination.read_bytes(), b"existing complete metadata\n")
        self.assertEqual(destination.stat().st_ino, inode)
        self.assertEqual(list((directory / "tmp").iterdir()), [])

    def test_production_block_preserves_destination_on_invalid_metadata(self):
        for payload in ("", "field: [unterminated", "[]\n", "scalar\n",
                        "build_version: incomplete\n"):
            with self.subTest(payload=payload):
                result, directory, destination, inode = self.install_fixture("payload", payload)
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse((directory / "install-call.json").exists())
                self.assertFalse((directory / "completed").exists())
                self.assertEqual(destination.read_bytes(), b"existing complete metadata\n")
                self.assertEqual(destination.stat().st_ino, inode)
                self.assertEqual(list((directory / "tmp").iterdir()), [])
                shutil.rmtree(directory)


if __name__ == "__main__":
    unittest.main()
