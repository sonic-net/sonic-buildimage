"""Exercise the pre-Bazel request using real Git trees and a disposable worker."""

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import resources
from tools.bazel.kernel import root_request as request


def git(directory, *args):
    return subprocess.check_output(["git", "-c", "user.name=Kernel fixture", "-c", "user.email=fixture@example.com",
                                    "-c", "protocol.file.allow=always", "-C", str(directory), *args],
                                   stderr=subprocess.PIPE, text=True).strip()


class RootKernelRequestTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="root-kernel-request-")
        self.addCleanup(temporary.cleanup)
        self.tmp = Path(temporary.name)
        self.repo = self.tmp / "root"
        self.repo.mkdir()
        git(self.repo, "init", "-q")
        source = self.tmp / "kernel"
        source.mkdir()
        git(source, "init", "-q")
        (source / "source").write_text("kernel")
        git(source, "add", "source")
        git(source, "commit", "-qm", "kernel")
        commit = git(source, "rev-parse", "HEAD")
        version = "0.0.3-" + commit
        git(self.repo, "submodule", "add", "-q", str(source), "src/sonic-linux-kernel")
        self.files = {
            ".bazelversion": "8.5.1\n", ".bazelignore": "", "Makefile.work": "fixture\n",
            ".bazelrc": "import %workspace%/tools/bazel/common.bazelrc\n",
            "tools/bazel/common.bazelrc": "try-import %workspace%/.bazelrc.user\n",
            "tools/bazel/slave.bazelrc": "common --disk_cache=/bazel_cache/disk_cache\n",
            "rules/linux-kernel.mk": "fixture\n", "tools/bazel/debian_mirrors.cfg": "# fixture\n",
            **{name: "# fixture\n" for name in request.CONSUMER_HELPERS},
            "tools/bazel/kernel/packages.py": "# fixture\n", "tools/bazel/kernel/root_resolution_registry.json": "{}",
            "tools/bazel/kernel/cache_wire.proto": "// fixture\n", "src/other/lock.txt": "abc\n",
            "src/other/deps.MODULE.bazel": 'register_toolchains("@infra//:cc")\n',
            "MODULE.bazel": f'bazel_dep(name="sonic-linux-kernel",version="{version}")\n'
                            f'bazel_dep(name="sonic-build-infra",version="{request.INFRA}")\n'
                            'lock_file="//src/other:lock.txt"\ninclude("//src/other:deps.MODULE.bazel")\n',
        }
        for name in sorted(request.LOCAL_MODULES):
            self.files["MODULE.bazel"] += f'local_path_override(module_name="{name}",path="src/{name}")\n'
            self.files[f"src/{name}/MODULE.bazel"] = f'module(name="{name}")\n'
        for name, data in self.files.items():
            destination = self.repo / name
            destination.parent.mkdir(exist_ok=True, parents=True)
            destination.write_text(data)
        git(self.repo, "submodule", "add", "-q", str(source), "src/sonic-sysmgr/gnoi")
        (self.repo / "src/sonic-sysmgr/.bazelversion").symlink_to("../../.bazelversion")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-qm", "root")
        bazel = self.tmp / "bin/bazel"
        bazel.parent.mkdir()
        bazel.write_text("#!/bin/sh\nexit 99\n")
        bazel.chmod(0o755)
        rc = self.tmp / "system.bazelrc"
        rc.write_text(self.files["tools/bazel/slave.bazelrc"])
        home = self.tmp / "home"
        home.mkdir()
        cache = self.tmp / "cache"
        self.environment = {"HOME": str(home), "PATH": str(bazel.parent), "BAZEL": "bazel",
                            "BAZELISK_HOME": str(cache), "SONIC_HOSTBASE_BUILDER_ID": "sha256:" + "a" * 64}
        self.config = {"CONFIGURED_ARCH": "amd64", "CONFIGURED_PLATFORM": "vs", "BLDENV": "trixie",
                       "CROSS_BUILD_ENVIRON": "n", "MULTIARCH_QEMU_ENVIRON": "n", "SECURE_UPGRADE_MODE": "no_sign",
                       "INCLUDE_EXTERNAL_PATCHES": "n", "EXTERNAL_KERNEL_PATCH_URL": "",
                       "EXTERNAL_KERNEL_PATCH_LOC": "", "SECURE_UPGRADE_KERNEL_CAFILE": "",
                       "ADDITIONAL_BUILD_PROFILES": ""}
        self.options = ["--platforms=" + request.PLATFORM, "--host_platform=" + request.PLATFORM,
                        "--@sonic_linux_kernel//tools/bazel:kernel_config=" + json.dumps(self.config, sort_keys=True),
                        "--repository_cache=/bazel_cache/kernel-linux-sandbox/repository_cache",
                        "--disk_cache=/bazel_cache/kernel-linux-sandbox/disk_cache",
                        "--remote_cache=grpc://172.16.0.4:9092", "--remote_upload_local_results=false"]
        digests = {k: hashlib.sha256(v.encode()).hexdigest() for k, v in self.files.items()}
        for name, value in {"KERNEL": version, "MODULE_FILES": {k: digests[k] for k in ("MODULE.bazel", "src/other/deps.MODULE.bazel")},
                            "RC_FILES": {k: digests[k] for k in request.RC_FILES}, "SLAVE_RC": rc,
                            "BAZELISK_HOME": cache, "BAZELISK": bazel, "WORKER_PATHS": {str(bazel.parent)}}.items():
            self.enterContext(patch.object(request, name, value))
        self.enterContext(patch.object(request.platform, "machine", return_value="x86_64"))
        self.enterContext(patch.object(request.platform, "system", return_value="Linux"))

    def key(self, **kwargs):
        return request.request_key(self.repo, kwargs.get("architecture", "amd64"), self.config,
                                   options=kwargs.get("options", self.options),
                                   environment=kwargs.get("environment", self.environment))

    def test_clean_requests_reuse_across_unrelated_commits_and_cache_roles(self):
        before = self.key()
        (self.repo / "notes").write_text("unrelated")
        git(self.repo, "add", "notes")
        git(self.repo, "commit", "-qm", "unrelated")
        self.assertEqual(before, self.key(options=[*self.options[:-1], "--remote_upload_local_results=true"]))
        self.assertNotEqual(before, self.key(environment=dict(self.environment,
                            SONIC_HOSTBASE_BUILDER_ID="sha256:" + "b" * 64)))

    def test_local_override_and_root_label_changes_invalidate(self):
        original = self.key()
        for name in ("src/sonic-sysmgr/MODULE.bazel", "src/other/lock.txt", "tools/bazel/image/file_copy.py"):
            path = self.repo / name
            path.write_text(path.read_text() + "# changed\n")
            with self.assertRaises(ValueError):
                self.key()
            git(self.repo, "commit", "-qam", "update " + name)
            current = self.key()
            self.assertNotEqual(original, current)
            original = current
        nested = self.repo / "src/sonic-sysmgr/gnoi"
        (nested / "source").write_text("changed nested source")
        with self.assertRaises(ValueError):
            self.key()
        git(nested, "commit", "-qam", "changed nested file")
        git(self.repo, "commit", "-qam", "updated nested local override input")
        self.assertNotEqual(original, self.key())
        (nested / "extraneous").touch()
        with self.assertRaises(ValueError):
            self.key()

    def test_cache_fallback_settings_keep_the_same_root_request(self):
        """A VS worker and strict cache verifier can reuse the same authenticated kernel bundle."""
        original = self.key()
        options = ["--remote_local_fallback", "--incompatible_remote_local_fallback_for_remote_cache"]
        for flags in (options, ["--noremote_local_fallback", options[1] + "=false"]):
            self.assertEqual(original, self.key(options=[*self.options, *flags]))
        for option in ("--remote_local_fallback=invalid", "--remote_executor=grpc://other:9092"):
            with self.subTest(option=option), self.assertRaisesRegex(ValueError, "unbound root kernel Bazel option"):
                self.key(options=[*self.options, option])

    def test_dedicated_ci_resource_budgets_share_a_key_across_cache_roles(self):
        original = self.key()
        home = Path(self.environment["HOME"])
        rc = self.repo / ".bazelrc.user"
        for budget, role in ((23989, "false"), (23990, "true")):
            for path in (rc, home / ".bazelrc"):
                path.unlink(missing_ok=True)
            with patch.object(resources, "action_memory_mb", return_value=budget):
                resources.configure(self.repo, home)
            self.assertEqual(self.key(options=[*self.options[:-1], "--remote_upload_local_results=" + role]), original)
        for extra in (b"import %workspace%/extra.rc\n", b"build --action_env=CFLAGS=-g\n",
                      b"common --remote_cache=grpc://elsewhere:9092\n", b"startup --host_jvm_args=-Xmx4g\n"):
            with self.subTest(extra=extra):
                rc.write_bytes((home / ".bazelrc").read_bytes() + extra)
                with self.assertRaisesRegex(ValueError, "unexpected root kernel configuration"):
                    self.key()
        rc.unlink()
        rc.symlink_to(home / ".bazelrc")
        with self.assertRaisesRegex(ValueError, "redirected root kernel configuration"):
            self.key()
        rc.unlink()
        with self.assertRaisesRegex(ValueError, "unsupported root kernel cache endpoint"):
            self.key(options=[*self.options[:-2], "--remote_cache=grpc://elsewhere:9092", self.options[-1]])

    def test_unknown_startup_settings_and_action_flags_miss(self):
        for key, value in (("USE_BAZEL_VERSION", "9.0.0"), ("BAZELISK_BASE_URL", "https://other.invalid"),
                           ("JAVA_TOOL_OPTIONS", "-Dtest=true"), ("PYTHONPATH", "src")):
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.key(environment=dict(self.environment, **{key: value}))
        with self.assertRaises(ValueError):
            self.key(options=[*self.options, "--config=other"])
        with self.assertRaises(ValueError):
            self.key(architecture="arm64")
        for rc in (self.repo / ".bazelrc.user", Path(self.environment["HOME"]) / ".bazelrc", request.SLAVE_RC):
            rc.write_text("build --action_env=OTHER=value\n")
            with self.assertRaises(ValueError):
                self.key()
            rc.unlink()

    def test_new_module_code_and_untracked_inputs_miss(self):
        root = self.repo / "MODULE.bazel"
        root.write_text(root.read_text() + 'include("//new:inputs.MODULE.bazel")\n')
        git(self.repo, "commit", "-qam", "unreviewed input")
        with self.assertRaises(ValueError):
            self.key()

    def test_misdirected_bazelisk_download_is_rejected_without_running_bazel(self):
        metadata = request.BAZELISK_HOME / "downloads/metadata/bazelbuild/bazel-8.5.1-linux-x86_64"
        metadata.parent.mkdir(parents=True)
        metadata.write_text("0" * 64)
        with self.assertRaises(ValueError):
            self.key()


if __name__ == "__main__":
    unittest.main()
