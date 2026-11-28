"""Exercise the actual package recipes without network access or compilation."""

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
PACKAGES = {"monit": "5.34.3-1", "rasdaemon": "0.6.8-1"}


class DebianSourceSelectionTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="debian-source-selection-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.inputs = self.root / "inputs"
        self.inputs.mkdir()
        self.calls = self.root / "calls.jsonl"
        script = "#!" + sys.executable + "\n" + r'''
import json, os, pathlib, shutil, sys
name = pathlib.Path(sys.argv[0]).name
args = sys.argv[1:]
with open(os.environ["SOURCE_TEST_CALLS"], "a") as stream:
    stream.write(json.dumps([name, *args]) + "\n")
if name == os.environ.get("SOURCE_TEST_FAIL"):
    sys.exit(73)
if name == "git" and args[0] == "clone":
    pathlib.Path(args[1].rsplit("/", 1)[1].removesuffix(".git")).mkdir()
elif name == "wget":
    if len(args) != 3 or args[0] != "-O":
        sys.exit(74)
    shutil.copyfile(pathlib.Path(os.environ["SOURCE_TEST_INPUTS"]) / args[1], args[1])
elif name == "dpkg-source":
    destination = pathlib.Path(args[-1])
    destination.mkdir()
    (destination / "source.txt").write_text("fixture\n")
elif name == "dpkg-buildpackage":
    for output in os.environ["SOURCE_TEST_OUTPUTS"].split():
        (pathlib.Path.cwd().parent / output).write_text("fixture package\n")
'''
        for name in ("git", "wget", "dpkg-source", "dpkg-buildpackage", "stg", "patch"):
            path = self.bin / name
            path.write_text(script)
            path.chmod(0o755)
        self.env = dict(os.environ, PATH=str(self.bin) + os.pathsep + os.environ["PATH"],
                        SOURCE_TEST_CALLS=str(self.calls), SOURCE_TEST_INPUTS=str(self.inputs))
        for key in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL", "PREFER_DEBIAN_REPO"):
            self.env.pop(key, None)

    def prepare(self, package):
        source = self.root / package
        shutil.rmtree(source, ignore_errors=True)
        shutil.copytree(ROOT / "src" / package, source)
        (source / "output").mkdir()
        self.calls.unlink(missing_ok=True)
        version = PACKAGES[package]
        names = [f"{package}_{version}.dsc", f"{package}_{version.split('-')[0]}.orig.tar.gz",
                 f"{package}_{version}.debian.tar.xz"]
        self.hash_args = []
        for suffix, name in zip(("DSC", "ORIG", "DEBIAN"), names):
            data = (name + "\n").encode()
            (self.inputs / name).write_bytes(data)
            self.hash_args.append(f"{package.upper()}_{suffix}_SHA256={hashlib.sha256(data).hexdigest()}")
        return source, names

    def run_recipe(self, package, source, mode=None, fail=None):
        version = PACKAGES[package]
        outputs = [f"{package}_{version}_amd64.deb", f"{package}-dbgsym_{version}_amd64.deb"]
        command = ["make", "--no-print-directory", "output/" + outputs[0], "DEST=output",
                   "CONFIGURED_ARCH=amd64", "CROSS_BUILD_ENVIRON=n", "SONIC_CONFIG_MAKE_JOBS=4",
                   "SONIC_DPKG_ADMINDIR=/var/lib/dpkg", f"{package.upper()}_VERSION={version}",
                   *self.hash_args]
        if mode is not None:
            command.append("PREFER_DEBIAN_REPO=" + mode)
        return subprocess.run(command, cwd=source,
                              env=dict(self.env, SOURCE_TEST_FAIL=fail or "",
                                       SOURCE_TEST_OUTPUTS=" ".join(outputs)),
                              text=True, capture_output=True, timeout=30)

    def recorded(self):
        return [json.loads(line) for line in self.calls.read_text().splitlines()]

    def test_default_and_explicit_n_preserve_original_git_selection(self):
        for package in PACKAGES:
            for mode in (None, "n"):
                with self.subTest(package=package, mode=mode):
                    source, _ = self.prepare(package)
                    result = self.run_recipe(package, source, mode)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    calls = self.recorded()
                    expected = [["git", "clone", "https://salsa.debian.org/debian/monit.git"],
                                ["git", "reset", "--hard", "debian/1%5.34.3-1"]] if package == "monit" else [
                        ["git", "clone", "https://salsa.debian.org/tai271828/rasdaemon.git"],
                        ["git", "checkout", "51a7f485f8b2e2ae43e613f19c5a387595174132"],
                        ["git", "apply", "../0001-Check-CPUs-online-not-configured.patch"]]
                    self.assertEqual([call for call in calls if call[0] == "git"], expected)
                    self.assertFalse(any(call[0] in ("wget", "dpkg-source") for call in calls))

    def test_debian_path_verifies_then_extracts_and_preserves_patch_build_order(self):
        for package in PACKAGES:
            with self.subTest(package=package):
                source, names = self.prepare(package)
                result = self.run_recipe(package, source, "y")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                calls = self.recorded()
                base = "https://deb.debian.org/debian/pool/main/m/monit" if package == "monit" else (
                    "https://snapshot.debian.org/archive/debian/20230206T092119Z/pool/main/r/rasdaemon")
                self.assertEqual(calls[:3], [["wget", "-O", name, base + "/" + name] for name in names])
                self.assertEqual(calls[3], ["dpkg-source", "--skip-patches", "-x", names[0], package])
                patches = [["git", "init"], ["git", "add", "-f", "source.txt"],
                           ["git", "commit", "-m", "unmodified Debian monit source"],
                           ["stg", "init"], ["stg", "import", "-s", "../patch/series"]] if package == "monit" else [
                    ["patch", "--batch", "--forward", "-p1"]]
                self.assertEqual(calls[4:-1], patches)
                self.assertEqual(calls[-1], ["dpkg-buildpackage", "-rfakeroot", "-b", "-us", "-uc",
                                            "-j4", "--admindir", "/var/lib/dpkg"])
                self.assertEqual(len(list((source / "output").glob("*.deb"))), 2)

    def test_any_corrupt_source_fails_before_extraction_or_build(self):
        for package in PACKAGES:
            for index in range(3):
                with self.subTest(package=package, archive=index):
                    source, names = self.prepare(package)
                    (self.inputs / names[index]).write_bytes(b"corrupt")
                    result = self.run_recipe(package, source, "y")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual({call[0] for call in self.recorded()}, {"wget"})

    def test_source_or_build_failures_propagate_without_switching_source(self):
        for package in PACKAGES:
            for mode, failure in (("n", "git"), ("y", "wget"), ("y", "dpkg-source"),
                                  ("n", "dpkg-buildpackage"), ("y", "dpkg-buildpackage")):
                with self.subTest(package=package, mode=mode, failure=failure):
                    source, _ = self.prepare(package)
                    result = self.run_recipe(package, source, mode, failure)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(self.recorded()[-1][0], failure)
                    self.assertEqual(list((source / "output").iterdir()), [])

    def test_config_exports_selection_and_changes_real_cache_flags(self):
        makefile = self.root / "cache.mk"
        cache_source = (ROOT / "Makefile.cache").read_text()
        macro = "define FLAGS_DEP_RULES\n" + cache_source.split("define FLAGS_DEP_RULES\n", 1)[1].split("\nendef", 1)[0] + "\nendef\n"
        makefile.write_text(f"include {ROOT}/rules/config\n"
                            "SRC_PATH := src\nCONFIGURED_ARCH := amd64\n"
                            f"include {ROOT}/rules/monit.mk\ninclude {ROOT}/rules/rasdaemon.mk\n"
                            f"include {ROOT}/rules/monit.dep\ninclude {ROOT}/rules/rasdaemon.dep\n"
                            + macro + "$(eval $(call FLAGS_DEP_RULES,$(MONIT) $(RASDAEMON),.,flags,))\n"
                            ".PHONY: probe\nprobe: $(MONIT).flags $(RASDAEMON).flags\n"
                            "\t@$(MAKE) --no-print-directory -f child.mk child\n")
        (self.root / "child.mk").write_text("child:\n\t@printf '%s' \"$$PREFER_DEBIAN_REPO\" > selection.txt\n")
        previous = None
        for mode in (None, "y", "n"):
            args = ["make", "--no-print-directory", "-f", str(makefile), "probe"]
            if mode:
                args.append("PREFER_DEBIAN_REPO=" + mode)
            result = subprocess.run(args, cwd=self.root, env=self.env, text=True, capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual((self.root / "selection.txt").read_text(), mode or "n")
            flags = {p.name: p.read_text() for p in self.root.glob("*.flags")}
            self.assertEqual(len(flags), 2)
            for value in flags.values():
                self.assertIn("PREFER_DEBIAN_REPO=" + (mode or "n"), value)
            if previous:
                self.assertNotEqual(flags, previous)
            previous = flags

    def test_outer_slave_instruction_forwards_selection_without_host_environment(self):
        work = (ROOT / "Makefile.work").read_text()
        start = work.index("SONIC_BUILD_INSTRUCTION :=")
        instruction = work[start:work.index("\n.PHONY: sonic-slave-build", start)]
        recorder = self.root / "record.py"
        recorder.write_text("import json,sys\nfrom pathlib import Path\n"
                            "Path('slave-argv.json').write_text(json.dumps(sys.argv[1:]))\n")
        makefile = self.root / "outer.mk"
        makefile.write_text(f"include {ROOT}/rules/config\n"
                            f"MAKE := {sys.executable} {recorder}\n" + instruction +
                            "\nprobe:\n\t@env -i PATH=/usr/bin:/bin $(SONIC_BUILD_INSTRUCTION) target/sonic-vs.bin\n")
        for mode in (None, "y"):
            args = ["make", "--no-print-directory", "-f", str(makefile), "probe"]
            if mode:
                args.append("PREFER_DEBIAN_REPO=" + mode)
            result = subprocess.run(args, cwd=self.root, env=self.env, text=True, capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            argv = json.loads((self.root / "slave-argv.json").read_text())
            self.assertEqual(argv[:2], ["-f", "slave.mk"])
            self.assertEqual([arg for arg in argv if arg.startswith("PREFER_DEBIAN_REPO=")],
                             ["PREFER_DEBIAN_REPO=" + (mode or "n")])


if __name__ == "__main__":
    unittest.main()
