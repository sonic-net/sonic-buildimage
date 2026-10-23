#!/usr/bin/env python3
"""Exercise direct kernel preparation, cache mounts and real Git worktrees."""

import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.kernel import prepare as kernel


def git(directory, *args):
    return subprocess.check_output(
        ["git", "-C", str(directory), *args], text=True, stderr=subprocess.PIPE,
    ).strip()


def commit(directory):
    git(directory, "add", ".")
    git(directory, "-c", "user.name=Kernel fixture", "-c", "user.email=fixture@example.com",
        "commit", "-qm", "Fixture")


def between(text, start, end):
    first = text.index(start)
    return text[first:text.index(end, first)]


class KernelMakeMappingTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="kernel-make-mapping-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.workspace = self.root / "workspace"
        self.workspace.mkdir()
        git(self.workspace, "init", "-q")
        (self.workspace / "input").write_text("workspace content\n")
        commit(self.workspace)

    def run_make(self, makefile, *targets, cwd=None, **variables):
        environment = os.environ.copy()
        for name in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL"):
            environment.pop(name, None)
        return subprocess.run(
            ["make", "--no-print-directory", "--no-builtin-rules", "-f", str(makefile),
             *targets, *(f"{key}={value}" for key, value in variables.items())],
            cwd=cwd or self.workspace, env=environment, capture_output=True, text=True,
        )

    def docker_arguments(self, workspace=None, cache=None, native=True, enabled=True):
        workspace = workspace or self.workspace
        work = (ROOT / "Makefile.work").read_text()
        git_mounts = between(work, "# Empty by default; only set when building from a git worktree", "# Consider removing the --ulimit")
        cache_mounts = between(work, "# Reuse downloads and action outputs across disposable builder containers.", "# User name and tag for")
        capture = self.root / "docker-argv.json"
        recorder = self.root / "docker-recorder.py"
        recorder.write_text("import json,sys\nfrom pathlib import Path\nPath(sys.argv[1]).write_text(json.dumps(sys.argv[2:]))\n")
        makefile = self.root / "mounts.mk"
        makefile.write_text(
            ".DEFAULT_GOAL := all\nSHELL := /bin/bash\n.SHELLFLAGS := -ec\n"
            + f"PWD := {workspace}\nBUILD_WITH_BAZEL_WHEN_AVAILABLE := {'y' if enabled else 'n'}\n"
            + f"SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD := {'y' if native else 'n'}\n"
            + f"SONIC_BAZEL_CACHE_SOURCE := {cache or ''}\n"
            + git_mounts
            + "\nDOCKER_RUN := " + shlex.join([sys.executable, str(recorder), str(capture)])
            + ' -v "$(PWD):/sonic" $(GIT_WORKTREE_MOUNTS)\n'
            + cache_mounts
            + "\nall:\n\t$(SONIC_WT_GITDIR_FIXUP)\n\t$(DOCKER_RUN) fixture-slave\n"
        )
        result = self.run_make(makefile, cwd=workspace)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        arguments = json.loads(capture.read_text())
        mounts, environment = {}, {}
        for index, arg in enumerate(arguments):
            if arg in ("-v", "--volume"):
                source, destination, *_ = arguments[index + 1].split(":")
                mounts[Path(destination)] = Path(source)
            if arg in ("-e", "--env"):
                name, separator, value = arguments[index + 1].partition("=")
                self.assertTrue(separator)
                environment[name] = value
        return mounts, environment

    def visible_path(self, path, mounts):
        """Resolve a path visible to the ordinary SONiC slave."""
        candidates = [(destination, source) for destination, source in mounts.items()
                      if path == destination or path.is_relative_to(destination)]
        self.assertTrue(candidates, f"path is invisible inside slave: {path}")
        destination, source = max(candidates, key=lambda item: len(item[0].parts))
        return source / path.relative_to(destination)

    def test_workspace_and_cache_use_only_the_normal_slave_mounts(self):
        cache = self.root / "external cache's files"
        cache.mkdir()
        for native in (True, False):
            with self.subTest(native_dockerd=native):
                mounts, environment = self.docker_arguments(cache=cache, native=native)
                self.assertEqual(mounts[Path("/sonic")], self.workspace)
                self.assertEqual(mounts[Path("/bazel_cache")], cache)
                self.assertNotIn(self.workspace, mounts)
                self.assertNotIn(cache, mounts)
                self.assertFalse(any(name.startswith("_SONIC_BAZEL_HOST_") for name in environment))

    def submodule_fixture(self):
        source = self.root / "kernel-source"
        source.mkdir()
        git(source, "init", "-q")
        (source / "Makefile").write_text("fixture source\n")
        commit(source)
        git(self.workspace, "-c", "protocol.file.allow=always", "submodule", "add", "-q",
            str(source), "src/sonic-linux-kernel")
        commit(self.workspace)
        return source

    def assert_git_paths_visible(self, workspace, mounts):
        common = Path(git(workspace, "rev-parse", "--path-format=absolute", "--git-common-dir"))
        gitdir = Path(git(workspace, "rev-parse", "--absolute-git-dir"))
        submodule = workspace / "src/sonic-linux-kernel"
        submodule_gitdir = Path(git(submodule, "rev-parse", "--absolute-git-dir"))
        for path in (common, gitdir, submodule_gitdir):
            visible = Path("/sonic") / path.relative_to(workspace) if path.is_relative_to(workspace) else path
            self.assertTrue(os.path.samefile(path, self.visible_path(visible, mounts)), str(path))
        alias = self.root / (workspace.name + "-alias")
        alias.symlink_to(workspace, target_is_directory=True)
        self.assertEqual(git(alias, "rev-parse", "HEAD"), git(workspace, "rev-parse", "HEAD"))
        self.assertEqual(git(alias / "src/sonic-linux-kernel", "rev-parse", "HEAD"), git(submodule, "rev-parse", "HEAD"))

    def test_normal_clone_submodule_metadata_is_visible_without_workspace_mirror(self):
        self.submodule_fixture()
        mounts, _ = self.docker_arguments()
        self.assert_git_paths_visible(self.workspace, mounts)

    def test_linked_worktree_and_submodule_keep_external_gitdirs_visible(self):
        self.submodule_fixture()
        linked = self.root / "linked"
        git(self.workspace, "worktree", "add", "-q", "--detach", str(linked), "HEAD")
        git(linked, "-c", "protocol.file.allow=always", "submodule", "update", "--init", "--quiet")
        mounts, _ = self.docker_arguments(workspace=linked)
        self.assertEqual(mounts[self.workspace / ".git"], self.workspace / ".git")
        self.assert_git_paths_visible(linked, mounts)
        pointer = (linked / "src/sonic-linux-kernel/.git").read_text().removeprefix("gitdir: ").strip()
        self.assertTrue(Path(pointer).is_absolute())

    def test_existing_direct_kernel_module_and_rootfs_goals_are_forwarded(self):
        outer = (ROOT / "Makefile").read_text()
        fragment = outer[outer.index("# Let the inner build check changed sources"):]
        makefile = self.root / "forwarding.mk"
        makefile.write_text(
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE := y\n"
            + "%::\n\t@echo forwarded-$@ >> forwarded.txt\n" + fragment
        )
        for architecture in ("amd64", "arm64"):
            abi = "6.12.41+deb13-sonic-" + architecture
            targets = [
                f"target/debs/trixie/linux-image-{abi}-unsigned_6.12.41-1_{architecture}.deb",
                f"target/debs/trixie/linux-image-{abi}-dbg_6.12.41-1_{architecture}.deb",
                f"target/debs/trixie/linux-headers-{abi}_6.12.41-1_{architecture}.deb",
                "target/debs/trixie/linux-headers-6.12.41+deb13-common-sonic_6.12.41-1_all.deb",
                f"target/debs/trixie/linux-kbuild-6.12.41+deb13_6.12.41-1_{architecture}.deb",
                f"target/debs/trixie/genl-packet-module_1.0-1_{architecture}.deb",
                "target/files/trixie/ixgbe.ko",
                "target/sonic-vs.bin__vs__rfs.squashfs",
            ]
            targets += [target + "-install" for target in targets if target.endswith(".deb")]
            for target in targets:
                with self.subTest(architecture=architecture, target=target):
                    path = self.workspace / target
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text("existing output\n")
                    log = self.workspace / "forwarded.txt"
                    log.unlink(missing_ok=True)
                    result = self.run_make(makefile, target)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertTrue(log.exists(), "existing output bypassed inner freshness checks: " + target)
                    self.assertEqual(log.read_text().splitlines(), ["forwarded-" + target])
        unrelated = "target/docker-fpm-frr.gz"
        (self.workspace / unrelated).write_text("existing Docker output\n")
        log = self.workspace / "forwarded.txt"
        log.unlink(missing_ok=True)
        result = self.run_make(makefile, unrelated)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse(log.exists(), "unrelated existing Docker target was forwarded by kernel hook")

    def lazy_make_fixture(self):
        makefile = self.root / "lazy.mk"
        binary = self.root / "bin"
        binary.mkdir()
        python = binary / "python3"
        python.write_text(
            "#!" + sys.executable + "\nimport json,sys\nfrom pathlib import Path\n"
            "assert sys.argv[1:4] == ['-B', 'tools/bazel/kernel/prepare.py', 'ensure']\n"
            "with Path('order.txt').open('a') as out: out.write('ensure\\n')\n"
            "Path('arguments.json').write_text(json.dumps(sys.argv[4:]))\n"
            "if Path('fail-prepare').exists(): raise SystemExit(29)\n"
        )
        python.chmod(0o755)
        functions = (ROOT / "rules/functions").read_text()
        derived = between(functions, "define add_derived_package\n", "endef") + "endef\n"
        makefile.write_text(
            ".DEFAULT_GOAL := unrelated\nSHELL := /bin/bash\n.SHELLFLAGS := -ec\n"
            "SRC_PATH := src\nDEBS_PATH := target/debs/trixie\n"
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE := y\n"
            + f"export PATH := {binary}:$(PATH)\n"
            + "CONFIGURED_ARCH := amd64\nCONFIGURED_PLATFORM := vs\nBLDENV := trixie\n"
            "SECURE_UPGRADE_MODE := no_sign\n"
            + derived + f"\ninclude {ROOT / 'rules/linux-kernel.mk'}\n"
            + f"include {ROOT / 'rules/linux-kernel.dep'}\n"
            + "\nunrelated:\n\t@echo unrelated > unrelated.txt\n"
            + "\nselection:\n\t@echo '$(SONIC_MAKE_DEBS)' > native-debs.txt\n"
            + "\t@echo '$($(LINUX_HEADERS_COMMON)_CACHE_MODE)' > native-cache.txt\n"
            + "\n$(addprefix $(DEBS_PATH)/,$(SONIC_MAKE_DEBS)):\n\t@echo built > native-built.txt\n"
            + "\ndocker-start:\n\t@echo unexpected-docker-start >> order.txt; false\n"
        )
        return makefile

    def kernel_arguments(self):
        arguments = json.loads((self.workspace / "arguments.json").read_text())
        self.assertEqual(len(arguments) % 2, 0)
        pairs = list(zip(arguments[::2], arguments[1::2]))
        config = dict(value.split("=", 1) for flag, value in pairs if flag == "--kernel-config")
        options = {flag: value for flag, value in pairs if flag != "--kernel-config"}
        return config, options

    def test_lazy_ensure_runs_directly_with_only_the_visible_cache_path(self):
        makefile = self.lazy_make_fixture()
        cache = self.root / "cache's files"
        cache.mkdir()
        result = self.run_make(makefile, "unrelated")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse((self.workspace / "order.txt").exists())
        result = self.run_make(makefile, "bazel-kernel-ensure", SONIC_BAZEL_CACHE_SOURCE=str(cache))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual((self.workspace / "order.txt").read_text().splitlines(), ["ensure"])
        config, options = self.kernel_arguments()
        self.assertEqual(options, {"--debs-directory": "target/debs/trixie", "--cache-directory": "/bazel_cache"})
        self.assertEqual(config, {
            "CONFIGURED_ARCH": "amd64", "CONFIGURED_PLATFORM": "vs", "BLDENV": "trixie",
            "CROSS_BUILD_ENVIRON": "", "MULTIARCH_QEMU_ENVIRON": "", "KERNEL_VERSION": "6.12.41",
            "KERNEL_ABISUFFIX": "+deb13", "KERNEL_SUBVERSION": "1", "KERNEL_FEATURESET": "sonic",
            "KVERSION": "6.12.41+deb13-sonic-amd64", "ADDITIONAL_BUILD_PROFILES": "",
            "SECURE_UPGRADE_MODE": "no_sign", "SECURE_UPGRADE_KERNEL_CAFILE": "",
            "INCLUDE_EXTERNAL_PATCHES": "n", "EXTERNAL_KERNEL_PATCH_URL": "", "EXTERNAL_KERNEL_PATCH_LOC": "",
        })
        result = self.run_make(makefile, "bazel-kernel-ensure", SONIC_BAZEL_CACHE_SOURCE="")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual((self.workspace / "order.txt").read_text().splitlines(), ["ensure", "ensure"])
        self.assertEqual(self.kernel_arguments()[1], {"--debs-directory": "target/debs/trixie"})

    def test_enabled_kernel_forwards_each_configuration_to_bazel(self):
        makefile = self.lazy_make_fixture()
        variations = {
            "CONFIGURED_ARCH": "armhf", "CONFIGURED_PLATFORM": "mellanox", "BLDENV": "bookworm",
            "CROSS_BUILD_ENVIRON": "y", "MULTIARCH_QEMU_ENVIRON": "y", "KERNEL_VERSION": "6.12.42",
            "KERNEL_ABISUFFIX": "+other", "KERNEL_SUBVERSION": "2", "KERNEL_FEATURESET": "custom",
            "KVERSION": "custom-abi", "ADDITIONAL_BUILD_PROFILES": "profile-a profile-b",
            "SECURE_UPGRADE_MODE": '"sign"', "SECURE_UPGRADE_KERNEL_CAFILE": "/tmp/trusted cert.pem",
            "INCLUDE_EXTERNAL_PATCHES": "y", "EXTERNAL_KERNEL_PATCH_URL": "https://example.test/patches.tar.gz",
            "EXTERNAL_KERNEL_PATCH_LOC": "/tmp/platform patches",
        }
        for key, value in variations.items():
            with self.subTest(configuration=key):
                result = self.run_make(makefile, "bazel-kernel-ensure", **{key: value})
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                forwarded = self.kernel_arguments()[0]
                self.assertEqual(forwarded[key], value.strip('"') if key == "SECURE_UPGRADE_MODE" else value)
                self.assertNotIn("--architecture", self.kernel_arguments()[1])
        result = self.run_make(makefile, "bazel-kernel-ensure", **variations)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        expected = dict(variations, SECURE_UPGRADE_MODE="sign")
        self.assertEqual(self.kernel_arguments()[0], expected)
        self.assertFalse((self.workspace / "native-built.txt").exists())

    def test_kernel_configuration_is_shell_quoted_and_make_mode_stays_native(self):
        makefile = self.lazy_make_fixture()
        special = "profile's value; `touch shell-executed` $$(touch shell-executed)"
        result = self.run_make(makefile, "bazel-kernel-ensure", ADDITIONAL_BUILD_PROFILES=special,
                               SECURE_UPGRADE_KERNEL_CAFILE="/tmp/owner's certificate.pem")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.kernel_arguments()[0]["ADDITIONAL_BUILD_PROFILES"], special.replace("$$", "$"))
        self.assertEqual(self.kernel_arguments()[0]["SECURE_UPGRADE_KERNEL_CAFILE"], "/tmp/owner's certificate.pem")
        self.assertFalse((self.workspace / "shell-executed").exists())
        (self.workspace / "order.txt").unlink()
        for disabled in ("n", ""):
            with self.subTest(shared_switch=disabled):
                result = self.run_make(makefile, "selection", BUILD_WITH_BAZEL_WHEN_AVAILABLE=disabled)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual((self.workspace / "native-debs.txt").read_text().strip(),
                    "linux-headers-6.12.41+deb13-common-sonic_6.12.41-1_all.deb")
                self.assertEqual((self.workspace / "native-cache.txt").read_text().strip(), "GIT_CONTENT_SHA")
                self.assertFalse((self.workspace / "order.txt").exists())

    def test_bazel_preparation_failure_does_not_use_native_producer(self):
        makefile = self.lazy_make_fixture()
        (self.workspace / "fail-prepare").touch()
        target = "target/debs/trixie/linux-image-6.12.41+deb13-sonic-amd64-unsigned_6.12.41-1_amd64.deb"
        result = self.run_make(makefile, target)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Error 29", result.stderr)
        self.assertEqual((self.workspace / "order.txt").read_text().splitlines(), ["ensure"])
        self.assertFalse((self.workspace / "native-built.txt").exists())
        self.assertFalse((self.workspace / target).exists())

    def test_debug_install_uses_the_selected_kernel_abi(self):
        script = (ROOT / "build_debian.sh").read_text()
        fragment = between(script, "# Install the source-built debug package", "## Update initramfs")
        packages = self.workspace / "packages"
        packages.mkdir()
        name = "linux-image-custom-armmp-dbg_6.12.42-2_armhf.deb"
        (packages / name).touch()
        environment = dict(os.environ, debs_path=str(packages), kversion="custom-armmp",
                           FILESYSTEM_ROOT="fixture-rootfs", DEBUG_IMG="y")
        for enabled in ("y", "n"):
            with self.subTest(shared_switch=enabled):
                result = subprocess.run(
                    ["bash", "-ec", 'sudo() { printf "%s\\n" "$*"; }\n' + fragment],
                    cwd=self.workspace, env=dict(environment, BUILD_WITH_BAZEL_WHEN_AVAILABLE=enabled),
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                if enabled == "y":
                    self.assertEqual(result.stdout.splitlines(), [
                        f"cp {packages / name} fixture-rootfs",
                        f"LANG=C DEBIAN_FRONTEND=noninteractive chroot fixture-rootfs apt -y install ./{name}",
                        f"rm -f fixture-rootfs/{name}",
                    ])
                else:
                    self.assertEqual(result.stdout, "")

    def test_changed_kernel_outputs_rebuild_consumers_in_the_same_make_invocation(self):
        binary = self.root / "bin"
        binary.mkdir()
        python = binary / "python3"
        python.write_text(
            "#!" + sys.executable + "\nimport os\nfrom pathlib import Path\n"
            "data = Path('revision.txt').read_bytes()\n"
            "for name in os.environ['TEST_KERNEL_OUTPUTS'].split():\n"
            "    output = Path(name)\n"
            "    output.parent.mkdir(parents=True, exist_ok=True)\n"
            "    if not output.exists() or output.read_bytes() != data: output.write_bytes(data)\n"
        )
        python.chmod(0o755)
        functions = (ROOT / "rules/functions").read_text()
        derived = between(functions, "define add_derived_package\n", "endef") + "endef\n"
        slave = (ROOT / "slave.mk").read_text()
        derived_recipe = between(slave, "# Rules for derived debian packages", "# Rules for extra debian packages")
        for architecture in ("amd64", "arm64"):
            with self.subTest(architecture=architecture):
                (self.workspace / "revision.txt").write_text("initial " + architecture)
                consumers = {
                    architecture + "-rootfs": "$(DEBS_PATH)/$(LINUX_KERNEL)",
                    architecture + "-module": "$(DEBS_PATH)/$(LINUX_HEADERS)",
                    architecture + "-metadata": "$(DEBS_PATH)/kernel-provenance.json",
                }
                makefile = self.root / (architecture + ".mk")
                makefile.write_text(
                    ".DEFAULT_GOAL := all\nSHELL := /bin/bash\n.SHELLFLAGS := -ec\n"
                    ".ONESHELL:\n.SECONDEXPANSION:\n"
                    "SRC_PATH := src\nDEBS_PATH := target/debs/trixie\n"
                    "BUILD_WITH_BAZEL_WHEN_AVAILABLE := y\n"
                    + f"export PATH := {binary}:$(PATH)\n"
                    + "CONFIGURED_ARCH := " + architecture + "\n"
                    "CONFIGURED_PLATFORM := vs\nBLDENV := trixie\nSECURE_UPGRADE_MODE := no_sign\n"
                    + derived + f"include {ROOT / 'rules/linux-kernel.mk'}\n"
                    + "export TEST_KERNEL_OUTPUTS := $(SONIC_BAZEL_KERNEL_INPUTS)\n"
                    + "all: " + " ".join(consumers) + "\n"
                    + "".join(f"{name}: {prerequisite}\n\tcat $< > $@\n\techo rebuilt >> {name}.updates\n"
                              for name, prerequisite in consumers.items())
                    + derived_recipe
                )
                for index, (revision, expected_rebuilds) in enumerate((("initial", 1), ("changed", 2), ("changed", 2))):
                    (self.workspace / "revision.txt").write_text(revision + " " + architecture)
                    before = {p.name: p.stat().st_mtime_ns for p in (self.workspace / "target/debs/trixie").glob("*.deb")}
                    result = self.run_make(makefile)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    for name in consumers:
                        self.assertEqual((self.workspace / name).read_text(), revision + " " + architecture)
                        self.assertEqual(len((self.workspace / (name + ".updates")).read_text().splitlines()), expected_rebuilds)
                    if index == 2:
                        after = {p.name: p.stat().st_mtime_ns for p in (self.workspace / "target/debs/trixie").glob("*.deb")}
                        self.assertEqual(before, after)


if __name__ == "__main__":
    unittest.main()
