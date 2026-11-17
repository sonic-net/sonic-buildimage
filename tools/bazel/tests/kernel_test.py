#!/usr/bin/env python3
"""Check kernel bundle integrity and the Debian package handoff without Bazel."""

import base64
import copy
import concurrent.futures
from contextlib import redirect_stderr, redirect_stdout
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import textwrap
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.kernel import prepare as kernel
from tools.bazel.ci import kernel_scratch as kernel_cleanup


class KernelBundleTest(unittest.TestCase):
    def setUp(self):
        registry = patch.dict(os.environ, {"SONIC_REVIEW_REGISTRY": kernel.DEFAULT_REGISTRY,
                                           "SONIC_BAZEL_REMOTE_ONLY_RC": ""})
        registry.start()
        self.addCleanup(registry.stop)
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.source = self.root / kernel.KERNEL_PATH
        self.source.mkdir(parents=True)
        subprocess.run(["git", "init", "-q", str(self.source)], check=True)
        (self.source / "Makefile").write_text(".DEFAULT_GOAL := fail\n.PHONY: fail\nfail:\n\t@touch COMPILED\n\t@false\n")
        (self.source / "manage-config").write_text("fixture configuration\n")
        for name in ("config.local", "patches-debian", "patches-sonic"):
            (self.source / name).mkdir()
        for name in ("tools/bazel/stage_kernel.py",):
            (self.source / name).parent.mkdir(parents=True, exist_ok=True)
            (self.source / name).write_text("fixture preparation input\n")
        (self.source / "tools/bazel/kernel_arch.py").write_text("MARKER = 'fixture'\n")
        self.files = [{"name": name, "mode": 0o644, "sha256": kernel.sha256(self.source / name)}
                      for name in sorted(("Makefile", "manage-config", "tools/bazel/kernel_arch.py", "tools/bazel/stage_kernel.py"))]
        identity = hashlib.sha256(json.dumps(self.files, sort_keys=True).encode()).hexdigest()
        source_tools = self.source / "tools/bazel"
        source_tools.mkdir(parents=True, exist_ok=True)
        (source_tools / "kernel_action.py").write_text("from kernel_arch import MARKER\nprint('" + identity + "')\n")
        (source_tools / "build.py").write_text(
            'BAZEL_URL = "https://releases.bazel.build/8.5.1/release/bazel-8.5.1-linux-x86_64"\n'
            'BAZEL_SHA256 = "' + "b" * 64 + '"\n'
            "ARCHITECTURES = {'amd64': {'bazel_url': BAZEL_URL, 'bazel_sha256': BAZEL_SHA256}, "
            "'arm64': {'bazel_url': 'https://releases.bazel.build/8.5.1/release/bazel-8.5.1-linux-arm64', 'bazel_sha256': '" + "c" * 64 + "'}}\n"
            "raise SystemExit('fixture does not run Bazel')\n")
        (self.source / ".bazelversion").write_text("8.5.1\n")
        (source_tools / "sources.bzl").write_text('''KERNEL_SOURCES = {
    "kernel_dsc": struct(name = "linux_6.12.41-1.dsc", sha256 = "''' + "d" * 64 + '''"),
    "kernel_orig": struct(name = "linux_6.12.41.orig.tar.xz", sha256 = "''' + "e" * 64 + '''"),
    "kernel_debian": struct(name = "linux_6.12.41-1.debian.tar.xz", sha256 = "''' + "f" * 64 + '''"),
}
''')
        module = (ROOT / "tools/bazel/kernel/MODULE.bazel").read_text()
        infra_line = next(line for line in module.splitlines() if 'name = "sonic-build-infra"' in line)
        (self.source / "MODULE.bazel").write_text('module(name = "sonic-linux-kernel", version = "0.0.1")\n' + infra_line + "\n")
        subprocess.run(["git", "-C", str(self.source), "add", "."], check=True)
        self.commit(self.source, "Fixture kernel source")
        self.gitlink = kernel.git(self.source, "rev-parse", "HEAD")
        (self.root / "rules").mkdir()
        for name in ("linux-kernel.mk", "linux-kernel.dep"):
            shutil.copyfile(ROOT / "rules" / name, self.root / "rules" / name)
        shutil.copyfile(ROOT / "Makefile.work", self.root / "Makefile.work")
        shutil.copyfile(ROOT / ".gitmodules", self.root / ".gitmodules")
        shutil.copytree(ROOT / "tools/bazel/kernel", self.root / "tools/bazel/kernel")
        shutil.copyfile(ROOT / kernel.DOWNLOADER_CONFIG, self.root / kernel.DOWNLOADER_CONFIG)
        consumer_module = self.root / "tools/bazel/kernel/MODULE.bazel"
        consumer_module.write_text(module.replace(next(line for line in module.splitlines() if 'name = "sonic-linux-kernel"' in line),
            'bazel_dep(name = "sonic-linux-kernel", version = "0.0.1-' + self.gitlink + '", repo_name = "sonic_linux_kernel")'))
        ci = self.root / "tools/bazel/ci"
        ci.mkdir(parents=True)
        for name in ("resolution.py", "time_vs_image.py"):
            shutil.copyfile(ROOT / "tools/bazel/ci" / name, ci / name)
        subprocess.run(["git", "init", "-q", str(self.root)], check=True)
        subprocess.run(["git", "-C", str(self.root), "add", ".gitmodules", "Makefile.work", "rules", "tools"], check=True)
        subprocess.run(["git", "-C", str(self.root), "update-index", "--add", "--cacheinfo",
                        "160000," + self.gitlink + "," + str(kernel.KERNEL_PATH)], check=True)
        self.commit(self.root, "Fixture image source")
        self.architecture = kernel.native_architecture()
        self.state = kernel.source_state(self.root)
        self.bundle = self.root / kernel.DEBS_DIRECTORY
        self.bundle.mkdir(parents=True)
        self.config, self.packages = kernel.contract(self.root)
        tool_inputs = ["a" * 64, "b" * 64]
        tool_identity = hashlib.sha256(json.dumps({"architecture": self.architecture, "inputs": tool_inputs},
                                                  sort_keys=True).encode()).hexdigest()
        self.manifest = dict(self.config, source_date_epoch=1754969284, source_files=self.files,
                             source_tree_sha256=identity, source_archives=self.state["source_archives"],
                             build_tools={"schema_version": 1, "kind": "debian-build-tools",
                                          "architecture": self.architecture, "identity_sha256": tool_identity,
                                          "input_sha256": tool_inputs, "packages": {"make": "4.4.1-2"}}, packages=[], debug={"kernel_build_id": "a" * 40, "module_count": 1, "module_build_ids_sha256": "b" * 64})
        abi = self.config["kernel_abi"]
        self.payloads = {
            "runtime": {"boot/vmlinuz-" + abi: b"kernel fixture",
                        "usr/lib/modules/" + abi + "/kernel/test.ko": b"module fixture",
                        "usr/share/doc/linux-image-" + abi + "-unsigned/copyright": b"Debian Linux copyright fixture\n"},
        }
        for name, metadata in self.packages.items():
            self.write_deb(name, metadata)
        self.manifest["created_packages"] = [dict(item, exported=True) for item in self.manifest["packages"]]
        self.selected = {"kernel_source_kind": "registry", "kernel_source_commit": self.gitlink,
                         "modules": self.state["modules"], "registry": kernel.DEFAULT_REGISTRY,
                         "infra_source_commit": self.state["modules"]["sonic-build-infra"].rsplit("-", 1)[1],
                         "graph_sha256": "a" * 64, "lock_sha256": "b" * 64,
                         "infra_source_json_sha256": "c" * 64, "kernel_source_json_sha256": "d" * 64,
                         "graph_inspection": {"module_graph_complete": True}}
        self.provenance = {"schema": 2, "source_commit": self.state["source_commit"], "kernel_gitlink": self.gitlink,
                           "modules": self.state["modules"], "launcher": self.state["launcher"],
                           "kernel_config": self.state["kernel_config"],
                           "target": kernel.TARGET, "resolution": self.selected,
                           "consumer_inputs_sha256": self.state["consumer_inputs_sha256"]}
        self.write_manifest()

    def write_deb(self, name, metadata):
        package_root = self.root / "package-root"
        if package_root.exists():
            shutil.rmtree(package_root)
        control = package_root / "DEBIAN/control"
        control.parent.mkdir(parents=True)
        control.write_text("".join(key.title() + ": " + metadata[key] + "\n"
                                   for key in ("package", "version", "architecture"))
                           + "Maintainer: Fixture <fixture@example.test>\nDescription: Test kernel package\n")
        for relative, data in self.payloads.get(metadata["kind"], {}).items():
            path = package_root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        path = self.bundle / name
        subprocess.run(["dpkg-deb", "--build", str(package_root), str(path)],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        item = dict(metadata, name=name, sha256=kernel.sha256(path), size=path.stat().st_size)
        self.manifest["packages"] = [record for record in self.manifest["packages"] if record["name"] != name] + [item]

    def commit(self, workspace, message):
        subprocess.run(["git", "-C", str(workspace), "-c", "user.name=Fixture",
                        "-c", "user.email=fixture@example.test", "commit", "-qm", message], check=True)

    def write_manifest(self):
        (self.bundle / kernel.MANIFEST).write_text(json.dumps(self.manifest))
        self.provenance["manifest_sha256"] = kernel.sha256(self.bundle / kernel.MANIFEST)
        (self.bundle / kernel.PROVENANCE).write_text(json.dumps(self.provenance))

    def resolution_files(self, workdir, consumer):
        workdir.mkdir(parents=True, exist_ok=True)
        version = self.state["modules"]["sonic-build-infra"]
        graph = {"key": "<root>", "root": True, "dependencies": [
            {"key": "sonic-linux-kernel@" + self.state["modules"]["sonic-linux-kernel"]}, {"key": "sonic-build-infra@" + version}]}
        (workdir / "module-graph.json").write_text(json.dumps(graph))
        (workdir / "module-graph.stderr").write_text("")
        (workdir / "module-graph.exit-code").write_text("0\n")
        commit = version.rsplit("-", 1)[1]
        metadata = json.dumps({"url": "https://github.com/securely1g/sonic-build-infra/archive/" + commit + ".tar.gz",
                               "strip_prefix": "sonic-build-infra-" + commit,
                               "integrity": "sha256-" + base64.b64encode(b"x" * 32).decode()}).encode()
        url = kernel.DEFAULT_REGISTRY + "/modules/sonic-build-infra/" + version + "/source.json"
        kernel_url = kernel.DEFAULT_REGISTRY + "/modules/sonic-linux-kernel/" + self.state["modules"]["sonic-linux-kernel"] + "/source.json"
        kernel_metadata = json.dumps({"url": "https://github.com/securely1g/sonic-linux-kernel/archive/" + self.gitlink + ".tar.gz",
                                     "strip_prefix": "sonic-linux-kernel-" + self.gitlink,
                                     "integrity": "sha256-" + base64.b64encode(b"y" * 32).decode()}).encode()
        records = {url: metadata, kernel_url: kernel_metadata}
        (consumer / "MODULE.bazel.lock").write_text(json.dumps({"registryFileHashes": {
            name: hashlib.sha256(value).hexdigest() for name, value in records.items()}}))
        return records

    def declared_outputs(self, workdir, source=None):
        source = source or self.bundle
        outputs = workdir / "outputs"
        outputs.mkdir(parents=True)
        for name in [kernel.MANIFEST, *self.packages]:
            shutil.copyfile(source / name, outputs / name)
        (workdir / "output-paths.txt").write_text("".join(
            str(outputs / name) + "\n" for name in [kernel.MANIFEST, *self.packages]))
        return outputs

    def test_publication_uses_normal_deb_directory_and_preserves_unrelated_files(self):
        workdir = self.root / "build"
        outputs = self.declared_outputs(workdir)
        shutil.rmtree(self.bundle)
        self.bundle.mkdir()
        unrelated = self.bundle / "unrelated.deb"
        unrelated.write_bytes(b"another package")
        untouched = unrelated.stat().st_mtime_ns
        result = kernel.stage_outputs(workdir, self.bundle, self.root, self.state, self.selected)
        self.assertEqual(result["manifest"], self.manifest)
        self.assertEqual(kernel.verify(self.bundle, self.root), result)
        for name in [kernel.MANIFEST, *self.packages]:
            self.assertEqual((outputs / name).read_bytes(), (self.bundle / name).read_bytes())
            self.assertNotEqual((outputs / name).stat().st_ino, (self.bundle / name).stat().st_ino)
        before = {p.name: p.stat().st_mtime_ns for p in self.bundle.iterdir()}
        kernel.stage_outputs(workdir, self.bundle, self.root, self.state, self.selected)
        self.assertEqual(before, {p.name: p.stat().st_mtime_ns for p in self.bundle.iterdir()})
        self.assertEqual(unrelated.read_bytes(), b"another package")
        self.assertEqual(unrelated.stat().st_mtime_ns, untouched)
        self.assertFalse((self.root / "target/bazel-kernel-inputs").exists())
        self.assertFalse(list(self.bundle.glob(".kernel-*")))

    def test_partial_publication_cannot_be_reused_as_a_complete_package_set(self):
        workdir = self.root / "build"
        self.declared_outputs(workdir)
        shutil.rmtree(self.bundle)
        self.bundle.mkdir()
        unrelated = self.bundle / "unrelated.deb"
        unrelated.write_bytes(b"another package")
        replace = Path.replace
        published = []
        def fail_second(path, destination):
            if len(published) == 1:
                raise OSError("interrupted publication")
            published.append(destination)
            return replace(path, destination)
        with patch.object(Path, "replace", fail_second):
            with self.assertRaisesRegex(OSError, "interrupted publication"):
                kernel.stage_outputs(workdir, self.bundle, self.root, self.state, self.selected)
        self.assertEqual(len(published), 1)
        self.assertFalse((self.bundle / kernel.PROVENANCE).exists())
        with self.assertRaisesRegex(ValueError, "publication is incomplete"):
            kernel.ensure(self.root)
        self.assertEqual(unrelated.read_bytes(), b"another package")
        self.assertFalse(list(self.bundle.glob(".kernel-*")))
        kernel.stage_outputs(workdir, self.bundle, self.root, self.state, self.selected)
        self.assertTrue(kernel.ensure(self.root)["reused"])

    def test_corrupt_missing_and_symlink_outputs_are_rejected(self):
        path = self.bundle / next(iter(self.packages))
        original = path.read_bytes()
        path.write_bytes(original + b"corrupt")
        with self.assertRaisesRegex(ValueError, "SHA256"):
            kernel.verify(self.bundle, self.root)
        path.unlink()
        with self.assertRaisesRegex(ValueError, "missing files"):
            kernel.verify(self.bundle, self.root)
        replacement = self.root / "other.deb"
        replacement.write_bytes(original)
        path.symlink_to(replacement)
        with self.assertRaisesRegex(ValueError, "regular kernel output"):
            kernel.verify(self.bundle, self.root)
        path.unlink()
        path.write_bytes(original)
        (self.bundle / "unrelated.deb").write_bytes(b"another package")
        self.assertEqual(kernel.verify(self.bundle, self.root)["manifest"], self.manifest)

    def test_changed_contract_source_archives_tools_and_metadata_are_rejected(self):
        original = copy.deepcopy(self.manifest)
        mutations = [
            lambda value: value.update(kernel_abi="other-abi"),
            lambda value: value.update(signing="signed"),
            lambda value: value.update(source_tree_sha256="0" * 64),
            lambda value: value["source_files"][0].update(sha256="0" * 64),
            lambda value: value["source_archives"][0].update(sha256="0" * 64),
            lambda value: value["build_tools"]["input_sha256"].__setitem__(0, "0" * 64),
            lambda value: value["packages"].pop(),
            lambda value: value["packages"][0].update(kind="development"),
        ]
        for mutation in mutations:
            self.manifest = copy.deepcopy(original)
            mutation(self.manifest)
            self.write_manifest()
            with self.subTest(manifest=self.manifest), self.assertRaises(ValueError):
                kernel.verify(self.bundle, self.root)

    def test_deb_control_is_checked_even_when_file_hash_matches(self):
        name, metadata = next(iter(self.packages.items()))
        self.write_deb(name, dict(metadata, package="wrong-kernel"))
        self.manifest["packages"][-1]["package"] = metadata["package"]
        self.manifest["created_packages"] = [dict(item, exported=True) for item in self.manifest["packages"]]
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, "DEB control differs"):
            kernel.verify(self.bundle, self.root)

    def test_wrong_architecture_and_created_package_inventory_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "missing files"):
            kernel.verify(self.bundle, self.root, architecture="arm64" if self.architecture == "amd64" else "amd64")
        self.manifest["created_packages"][0]["exported"] = False
        self.write_manifest()
        with self.assertRaises(ValueError):
            kernel.verify(self.bundle, self.root)

    def test_provenance_requires_the_explicit_review_registry(self):
        reviewed = kernel.REGISTRY_PREFIX + "codex/kernel-sandbox-registry"
        with patch.dict(os.environ, {"SONIC_REVIEW_REGISTRY": reviewed}):
            with self.assertRaisesRegex(ValueError, "resolution provenance"):
                kernel.verify(self.bundle, self.root)
            self.provenance["resolution"]["registry"] = reviewed
            self.write_manifest()
            self.assertEqual(kernel.verify(self.bundle, self.root)["manifest"], self.manifest)
        with self.assertRaisesRegex(ValueError, "resolution provenance"):
            kernel.verify(self.bundle, self.root)

    def test_stale_source_and_provenance_are_rejected(self):
        self.provenance["kernel_gitlink"] = "0" * 40
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, "provenance differs"):
            kernel.verify(self.bundle, self.root)
        self.provenance["kernel_gitlink"] = self.gitlink
        self.write_manifest()
        (self.source / "Makefile").write_text("changed source\n")
        with self.assertRaisesRegex(ValueError, "inputs must match|checkout must have"):
            kernel.verify(self.bundle, self.root)

    def test_resolution_binds_registered_kernel_and_exact_tools_source(self):
        workdir, consumer = self.root / "resolution", self.root / "consumer"
        consumer.mkdir()
        metadata = self.resolution_files(workdir, consumer)
        selected = kernel.inspect_resolution(workdir, consumer, self.state, kernel.DEFAULT_REGISTRY,
                                             fetch=lambda url: metadata[url])
        self.assertEqual(selected["kernel_source_kind"], "registry")
        self.assertEqual(selected["kernel_source_commit"], self.gitlink)
        graph = json.loads((workdir / "module-graph.json").read_text())
        graph["dependencies"][1]["key"] = "sonic-build-infra@0.0.1-other"
        (workdir / "module-graph.json").write_text(json.dumps(graph))
        with self.assertRaisesRegex(ValueError, "different kernel tools module"):
            kernel.inspect_resolution(workdir, consumer, self.state, kernel.DEFAULT_REGISTRY,
                                      fetch=lambda url: metadata[url])

    def test_stored_graph_failure_reports_primary_command_without_fetching(self):
        """Keep a post-build graph failure visible through the kernel's direct inspection path."""
        workdir, consumer = self.root / "resolution", self.root / "consumer"
        consumer.mkdir()
        self.resolution_files(workdir, consumer)
        primary = "ERROR: downloading https://private.invalid/package?token=fixture: HTTP 503\n"
        (workdir / "module-graph.stderr").write_text(primary)
        (workdir / "module-graph.exit-code").write_text("2\n")
        before = {path: path.read_bytes() for path in [
            workdir / "module-graph.json", workdir / "module-graph.stderr",
            workdir / "module-graph.exit-code", consumer / "MODULE.bazel.lock",
        ]}
        console = io.StringIO()

        def unexpected_fetch(url):
            self.fail("A rejected module graph must not fetch package metadata")

        with redirect_stderr(console), \
                self.assertRaisesRegex(ValueError, "Unexpected module graph inspection failure"):
            kernel.inspect_resolution(workdir, consumer, self.state, kernel.DEFAULT_REGISTRY,
                                      fetch=unexpected_fetch)
        diagnostic = json.loads(console.getvalue())
        self.assertEqual(diagnostic["returncode"], 2)
        self.assertEqual(diagnostic["http_statuses"], [503])
        self.assertNotIn("private.invalid", console.getvalue())
        self.assertEqual(before, {path: path.read_bytes() for path in before})

    def test_remote_cache_policy_is_forwarded_to_isolated_launcher(self):
        for index, (remote, upload) in enumerate((
                (None, False), ("grpc://cache.example:9092", False),
                ("grpc://cache.example:9092", True))):
            with self.subTest(remote=remote, upload=upload):
                planned = kernel.plan(self.root, self.root / f"remote-plan-{index}", remote, upload)
                self.assertEqual(planned["remote_cache"], remote)
                self.assertEqual(planned["upload_local_results"], upload)
                argv = planned["argv"]
                if remote:
                    self.assertEqual(argv[argv.index("--remote-cache") + 1], remote)
                else:
                    self.assertNotIn("--remote-cache", argv)
                self.assertEqual("--remote-cache-read-only" in argv, not upload)

    def test_cli_managed_trust_reaches_plan_with_explicit_override(self):
        managed = self.root / "managed-cacerts"
        managed.write_bytes(b"managed trust fixture")
        override = self.root / "override-cacerts"
        override.write_bytes(b"explicit trust fixture")
        key = "SONIC_BAZEL_JAVA_TRUST_STORE"
        cases = [(None, None), ("", None), (str(managed), None),
                 (str(self.root / "missing-cacerts"), override)]
        for index, (setting, explicit) in enumerate(cases):
            argv = ["prepare.py", "plan", "--workspace", str(self.root),
                    "--state-dir", str(self.root / ("trust-plan-" + str(index)))]
            if explicit:
                argv += ["--java-trust-store", str(explicit)]
            with self.subTest(setting=setting, explicit=explicit), \
                    patch.dict(os.environ), patch.object(sys, "argv", argv), \
                    redirect_stdout(io.StringIO()) as output:
                if setting is None:
                    os.environ.pop(key, None)
                else:
                    os.environ[key] = setting
                kernel.main()
                planned = json.loads(output.getvalue())
            selected = explicit or (managed if setting else None)
            if selected:
                self.assertEqual(planned["execution_trust"], {
                    "java_trust_store_sha256": kernel.sha256(selected)})
                option = planned["argv"].index("--java-trust-store")
                self.assertEqual(planned["argv"][option + 1], str(selected))
            else:
                self.assertEqual(planned["execution_trust"], {})
                self.assertNotIn("--java-trust-store", planned["argv"])

    def test_cli_rejects_missing_managed_trust_file(self):
        with patch.dict(os.environ, {"SONIC_BAZEL_JAVA_TRUST_STORE": str(self.root / "missing-cacerts")}), \
                patch.object(sys, "argv", ["prepare.py", "plan", "--workspace", str(self.root),
                                          "--state-dir", str(self.root / "trust-plan")]), \
                self.assertRaises(FileNotFoundError):
            kernel.main()

    def test_cli_verify_ignores_managed_trust(self):
        with patch.dict(os.environ, {"SONIC_BAZEL_JAVA_TRUST_STORE": str(self.root / "missing-cacerts")}), \
                patch.object(sys, "argv", ["prepare.py", "verify", "--workspace", str(self.root)]), \
                redirect_stdout(io.StringIO()) as output:
            kernel.main()
        self.assertEqual(json.loads(output.getvalue())["manifest"], self.manifest)

    @patch.dict(os.environ, {"SONIC_VS_IMAGE_TIMINGS": "1"})
    def test_producer_uses_registered_source_and_retains_evidence(self):
        prebuilt = self.bundle
        with self.assertRaisesRegex(ValueError, "must not overlap"):
            kernel.plan(self.root, self.bundle / "state", None, False)
        calls = []
        metadata = None

        def execute(command, workspace):
            nonlocal metadata
            calls.append(command)
            self.assertEqual(workspace, self.root)
            consumer = Path(command[command.index("--workspace") + 1])
            workdir = Path(command[command.index("--work-dir") + 1])
            self.assertIn(kernel.DEFAULT_REGISTRY, (consumer / ".bazelrc").read_text())
            self.assertIn("common --downloader_config=debian_mirrors.cfg",
                          (consumer / ".bazelrc").read_text().splitlines())
            self.assertEqual((consumer / kernel.DOWNLOADER_CONFIG.name).read_bytes(),
                             (self.root / kernel.DOWNLOADER_CONFIG).read_bytes())
            self.assertIn(kernel.DEFAULT_REGISTRY, (self.root / "tools/bazel/kernel/.bazelrc").read_text())
            outputs = workdir / "outputs"
            outputs.mkdir(parents=True)
            (workdir / "build.log").write_text("kernel source build\n")
            (workdir / "profile.json.gz").write_bytes(b"build profile")
            for name in [kernel.MANIFEST, *self.packages]:
                shutil.copyfile(prebuilt / name, outputs / name)
            (workdir / "output-paths.txt").write_text("".join(
                str(outputs / name) + "\n" for name in [kernel.MANIFEST, *self.packages]))
            metadata = self.resolution_files(workdir, consumer)
            (workdir / "invocation.json").write_text(json.dumps({
                "schema": 2, "target": kernel.TARGET, "uid": 1000, "host_machine": {"amd64": "x86_64", "arm64": "aarch64"}[self.architecture],
                "kernel_config": self.state["kernel_config"], **self.state["launcher"],
                "remote_cache": "http://127.0.0.1:8080", "remote_cache_read_only": True, "disk_cache": None}))

        receipt = kernel.build(self.root, self.root / "state", "http://127.0.0.1:8080", False,
                               execute=execute, fetch=lambda url: metadata[url])
        command = calls[0]
        self.assertIn("--remote-cache-read-only", command)
        self.assertNotIn("--disk-cache", command)
        self.assertEqual(command[command.index("--target") + 1], kernel.TARGET)
        self.assertNotIn("--module-override", command)
        self.assertEqual(command[command.index("--architecture") + 1], self.architecture)
        self.assertEqual(json.loads(command[command.index("--kernel-config") + 1]), self.state["kernel_config"])
        self.assertEqual(receipt["result"]["manifest"], self.manifest)
        self.assertEqual(receipt["plan"]["debs_directory"], str(self.bundle))
        self.assertEqual(kernel.verify(self.bundle, self.root)["manifest"], self.manifest)
        self.assertTrue((self.bundle / "kernel-provenance.json").is_file())
        self.assertFalse((self.root / "state/bundle").exists())
        self.assertTrue((self.root / "state/plan.json").is_file())
        self.assertTrue((self.root / "state/kernel-receipt.json").is_file())
        self.assertEqual((self.root / "state").stat().st_mode & 0o077, 0)
        self.assertFalse((self.root / "tools/bazel/kernel/MODULE.bazel.lock").exists())
        directory, = (self.root / "artifacts/vs/image-timings/linux-kernel").iterdir()
        recipe = json.loads((directory / "recipe.json").read_text())
        self.assertEqual(recipe, {"bazel": ["launcher"], "profiles": ["build", "cquery", "mod"],
                                  "exports": ["stage-debs"]})
        for phase in ("launcher", "stage-debs"):
            self.assertEqual(json.loads((directory / f"{phase}.timing.json").read_text())["exit_code"], 0)
        self.assertEqual((directory / "build.profile.json.gz").read_bytes(), b"build profile")
        self.assertEqual((directory / "build.log").read_text(), "kernel source build\n")
        rc = (self.root / "state/consumer/.bazelrc").read_text().splitlines()
        self.assertIn("common --noslim_profile", rc)
        for phase in ("cquery", "mod"):
            self.assertIn(f"{phase} --profile={directory}/{phase}.profile.json.gz", rc)
        self.assertTrue(kernel.ensure(self.root)["reused"])
        self.assertEqual(list(directory.parent.iterdir()), [directory])

    @patch.dict(os.environ, {"SONIC_VS_IMAGE_TIMINGS": "1"})
    def test_failed_kernel_launcher_retains_its_profile_and_status(self):
        def execute(command, workspace):
            workdir = Path(command[command.index("--work-dir") + 1])
            workdir.mkdir(parents=True)
            (workdir / "profile.json.gz").write_bytes(b"partial profile")
            raise subprocess.CalledProcessError(37, command)

        with self.assertRaises(subprocess.CalledProcessError) as failure:
            kernel.build(self.root, self.root / "state", None, False, execute=execute)
        self.assertEqual(failure.exception.returncode, 37)
        directory, = (self.root / "artifacts/vs/image-timings/linux-kernel").iterdir()
        self.assertEqual(json.loads((directory / "launcher.timing.json").read_text())["exit_code"], 37)
        self.assertEqual((directory / "build.profile.json.gz").read_bytes(), b"partial profile")
        self.assertFalse((directory / "stage-debs.timing.json").exists())

    def test_managed_kernel_cache_reaches_only_the_isolated_consumer(self):
        cache_rc = self.root / "worker-cache.bazelrc"
        cache_rc.write_text("common --remote_cache=https://cache.example.test\n")
        state = self.root / "managed-state"
        template = self.root / "tools/bazel/kernel/.bazelrc"
        original = template.read_bytes()
        with patch.dict(os.environ, {"SONIC_BAZEL_REMOTE_ONLY_RC": str(cache_rc)}):
            with self.assertRaisesRegex(ValueError, "explicit kernel cache"):
                kernel.plan(self.root, state, None, False, cache_directory=self.root / "disk-cache")
            with self.assertRaisesRegex(RuntimeError, "stop before compilation"):
                kernel.build(self.root, state, None, False,
                             execute=Mock(side_effect=RuntimeError("stop before compilation")))
        rc = (state / "consumer/.bazelrc").read_text()
        self.assertIn("import " + str(cache_rc), rc.splitlines())
        self.assertIn("try-import /run/sonic-native-apt/bazel-source.bazelrc", rc.splitlines())
        self.assertIn('flag_name: "repository_cache" set_value { flag_value: ""', rc)
        self.assertEqual((state / "consumer" / kernel.DOWNLOADER_CONFIG.name).read_bytes(),
                         (self.root / kernel.DOWNLOADER_CONFIG).read_bytes())
        self.assertEqual(template.read_bytes(), original)

    def test_build_rejects_privileged_or_weakened_launcher_invocations(self):
        for index, override in enumerate((
            {"uid": 0}, {"uid": True}, {"execution_strategy": "local"},
            {"schema": 1}, {"bazel_sha256": "0" * 64},
            {"host_machine": "aarch64" if self.architecture == "amd64" else "x86_64"},
            {"kernel_config": {}},
        )):
            with self.subTest(override=override):
                def execute(command, workspace):
                    workdir = Path(command[command.index("--work-dir") + 1])
                    workdir.mkdir(parents=True)
                    invocation = {
                        "schema": 2, "target": kernel.TARGET, "uid": 1000, "host_machine": {"amd64": "x86_64", "arm64": "aarch64"}[self.architecture],
                "kernel_config": self.state["kernel_config"],
                        **self.state["launcher"], "remote_cache": None,
                        "remote_cache_read_only": True, "disk_cache": None,
                        **override,
                    }
                    (workdir / "invocation.json").write_text(json.dumps(invocation))
                with self.assertRaisesRegex(ValueError, "launcher invocation differs"):
                    kernel.build(self.root, self.root / ("invalid-state-" + str(index)),
                                 None, False, execute=execute)

    def test_output_staging_rejects_paths_outside_the_local_build_directory(self):
        workdir = self.root / "local-build"
        workdir.mkdir()
        outside = self.root / "outside-output"
        outside.write_bytes(b"not a declared build output")
        link = workdir / "escaped-output"
        link.symlink_to(outside)
        for path in (outside, link, Path("relative-output")):
            with self.subTest(path=path):
                (workdir / "output-paths.txt").write_text(str(path) + "\n")
                with self.assertRaisesRegex(ValueError, "absolute path|escapes"):
                    kernel.stage_outputs(workdir, self.root / "rejected-bundle", self.root,
                                         self.state, self.selected)

    def make_fixture(self):
        output = self.root / "target/debs/trixie"
        output.mkdir(parents=True, exist_ok=True)
        (self.root / ".platform").touch()
        slave = (ROOT / "slave.mk").read_text()
        copy_recipe = slave[slave.index("# Copy debian packages from local directory"):slave.index("# Copy regular files from local directory")]
        derived_recipe = slave[slave.index("# Rules for derived debian packages"):slave.index("# Rules for extra debian packages")]
        functions = (ROOT / "rules/functions").read_text()
        start = functions.index("define add_derived_package\n")
        derived_macro = functions[start:functions.index("endef", start) + len("endef")]
        makefile = self.root / "test.mk"
        makefile.write_text(
            ".DEFAULT_GOAL := all\n.SHELLFLAGS := -ec\n.ONESHELL:\n.SECONDEXPANSION:\n"
            "SRC_PATH := src\nTARGET_PATH := target\nDEBS_PATH := target/debs/trixie\n"
            "CONFIGURED_ARCH := " + self.architecture + "\nCONFIGURED_PLATFORM := vs\nBLDENV := trixie\n"
            "SECURE_UPGRADE_MODE := no_sign\nBUILD_WITH_BAZEL_WHEN_AVAILABLE := y\n"
            + derived_macro + "\ninclude rules/linux-kernel.mk\ninclude rules/linux-kernel.dep\n"
            "all: $(DEBS_PATH)/$(LINUX_HEADERS)\n"
            "\t@test -z '$(SONIC_MAKE_DEBS)'\n"
            "\t@test '$($(LINUX_HEADERS_COMMON)_CACHE_MODE)' = none\n"
            + copy_recipe + derived_recipe)
        return makefile

    def test_real_make_rules_use_normal_debs_and_recheck_stale_source(self):
        makefile = self.make_fixture()
        unrelated = self.bundle / "unrelated.deb"
        unrelated.write_bytes(b"unrelated native output")
        before = {p.name: p.stat().st_mtime_ns for p in self.bundle.iterdir()}
        command = ["make", "--no-print-directory", "--no-builtin-rules", "-f", str(makefile)]
        for _ in range(2):
            result = subprocess.run(command, cwd=self.root, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertNotIn("cp ", result.stdout)
            self.assertFalse((self.source / "COMPILED").exists())
            self.assertEqual(before, {p.name: p.stat().st_mtime_ns for p in self.bundle.iterdir()})
        self.assertEqual(unrelated.read_bytes(), b"unrelated native output")
        (self.source / "Makefile").write_text("changed source\n")
        result = subprocess.run(command, cwd=self.root, text=True, capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse((self.source / "COMPILED").exists())

    def test_ci_make_smoke_imports_twice_without_touching_derived_packages(self):
        for relative in ("slave.mk", "rules/functions", "tools/bazel/ci/kernel_make_smoke.py"):
            shutil.copyfile(ROOT / relative, self.root / relative)
        subprocess.run(["git", "-C", str(self.root), "add", "slave.mk", "rules/functions",
                        "tools/bazel/ci/kernel_make_smoke.py"], check=True)
        self.commit(self.root, "Fixture Make smoke inputs")
        current = kernel.source_state(self.root)
        self.provenance.update(source_commit=current["source_commit"],
                               consumer_inputs_sha256=current["consumer_inputs_sha256"])
        self.write_manifest()
        result = subprocess.run([sys.executable, "-B", "tools/bazel/ci/kernel_make_smoke.py",
                                 "--architecture", self.architecture], cwd=self.root, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("Make uses normal DEBs; repeated and concurrent requests preserve their timestamps and unrelated packages.", result.stdout)
        self.assertFalse((self.root / ".platform").exists())
        self.assertFalse((self.source / "COMPILED").exists())

    def test_arm64_package_manifest_and_launcher(self):
        self.config, self.packages = kernel.contract(self.root, "arm64")
        for path in self.bundle.iterdir():
            path.unlink()
        self.state = kernel.source_state(self.root, "arm64")
        self.provenance["launcher"] = self.state["launcher"]
        self.provenance["kernel_config"] = self.state["kernel_config"]
        self.manifest.update(self.config, packages=[])
        tools = self.manifest["build_tools"]
        tools["architecture"] = "arm64"
        tools["identity_sha256"] = hashlib.sha256(json.dumps({"architecture": "arm64", "inputs": tools["input_sha256"]}, sort_keys=True).encode()).hexdigest()
        for name, metadata in self.packages.items():
            self.write_deb(name, metadata)
        self.manifest["created_packages"] = [dict(item, exported=True) for item in self.manifest["packages"]]
        self.write_manifest()
        self.assertEqual(kernel.verify(self.bundle, self.root, architecture="arm64")["manifest"], self.manifest)
        plan = kernel.plan(self.root, self.root / "arm-state", None, False, architecture="arm64")
        self.assertEqual(plan["argv"][plan["argv"].index("--architecture") + 1], "arm64")
        self.assertFalse((self.source / "COMPILED").exists())

    def test_make_native_mode_keeps_legacy_deb_contract(self):
        makefile = self.make_fixture()
        with makefile.open("a") as stream:
            stream.write("native:\n\t@test -n '$(SONIC_MAKE_DEBS)'\n\t@test '$(words $(SONIC_DERIVED_DEBS))' = 3\n"
                         "\t@test -z '$(SONIC_COPY_DEBS)'\n"
                         "\t@test '$($(LINUX_HEADERS_COMMON)_CACHE_MODE)' = GIT_CONTENT_SHA\n"
                         "\t@test -z '$($(LINUX_HEADERS_COMMON)_CACHE_FORCE_RESTORE)'\n")
        result = subprocess.run(["make", "--no-print-directory", "-f", str(makefile), "native", "BUILD_WITH_BAZEL_WHEN_AVAILABLE=n"], cwd=self.root, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_order_only_verification_preserves_unchanged_image_output(self):
        slave = (ROOT / "slave.mk").read_text()
        start = slave.index("# Revalidate retained source packages")
        block = slave[start:slave.index("\n$(addprefix $(TARGET_PATH)/, $(SONIC_RFS_TARGETS))", start)]
        makefile = self.root / "order.mk"
        makefile.write_text("TARGET_PATH := target\nSONIC_RFS_TARGETS := rootfs.squashfs\nSONIC_INSTALLERS := sonic.bin\n"
            "BUILD_WITH_BAZEL_WHEN_AVAILABLE := y\n" + block +
            "\n.PHONY: bazel-kernel-ensure bazel-kernel-development\n"
            "bazel-kernel-ensure:\n\t@echo checked >> verified.txt\n"
            "bazel-kernel-development: bazel-kernel-ensure\n\t@true\n"
            "target/rootfs.squashfs: target/debs/trixie/" + next(name for name, value in self.packages.items() if value["kind"] == "runtime") + "\n\t@echo built >> $@\n")
        command = ["make", "--no-print-directory", "-f", str(makefile), "target/rootfs.squashfs"]
        subprocess.run(command, cwd=self.root, check=True)
        output = self.root / "target/rootfs.squashfs"
        before = output.stat().st_mtime_ns
        subprocess.run(command, cwd=self.root, check=True)
        self.assertEqual(before, output.stat().st_mtime_ns)
        self.assertEqual((self.root / "verified.txt").read_text().splitlines(), ["checked", "checked"])
        self.assertEqual(output.read_text(), "built\n")

    def test_bazel_kernel_dependencies_are_scoped_to_selected_consumers(self):
        slave = (ROOT / "slave.mk").read_text()
        start = slave.index("# Track Bazel kernel inputs for downstream Make consumers.")
        block = slave[start:slave.index("\nendif", start) + len("\nendif")]
        makefile = self.root / "cache.mk"
        consumers = ("rootfs.squashfs", "genl.deb", "ixgbe.ko")
        makefile.write_text(".DEFAULT_GOAL := all\n"
            "SONIC_BAZEL_KERNEL_INPUTS := $(addprefix $(DEBS_PATH)/," + " ".join(self.packages) + " kernel-packages.json kernel-provenance.json)\n"
            "SONIC_RFS_TARGETS := rootfs.squashfs\nGENL_PACKET_MODULE := genl.deb\nIXGBE_DRIVER := ixgbe.ko\n"
            + "".join(name + "_DEP_FILES := original-input\n" for name in consumers)
            + "unrelated.deb_DEP_FILES := unrelated-input\n" + block + "\nall:\n"
            + "".join("\t$(file >" + name + ".inputs,$(" + name + "_DEP_FILES))\n" for name in (*consumers, "unrelated.deb"))
            + "\t@true\n")
        def values(enabled, directory):
            subprocess.run(["make", "--no-print-directory", "-f", str(makefile), "BUILD_WITH_BAZEL_WHEN_AVAILABLE=" + enabled,
                            "DEBS_PATH=" + directory], cwd=self.root, check=True, stdout=subprocess.DEVNULL)
            return {name: (self.root / (name + ".inputs")).read_text().split() for name in (*consumers, "unrelated.deb")}
        native = values("", str(kernel.DEBS_DIRECTORY))
        for name in consumers:
            self.assertEqual(native[name], ["original-input"])
        for directory in (str(kernel.DEBS_DIRECTORY), "target/other-debs"):
            selected = values("y", directory)
            expected = {"original-input", "tools/bazel/kernel/prepare.py"} | {
                directory + "/" + name for name in (*self.packages, kernel.MANIFEST, kernel.PROVENANCE)}
            for name in consumers:
                self.assertEqual(set(selected[name]), expected)
            self.assertEqual(selected["unrelated.deb"], ["unrelated-input"])

    def test_make_selects_bazel_for_every_configuration_when_enabled(self):
        makefile = self.make_fixture()
        with makefile.open("a") as stream:
            stream.write("selection:\n\t@test -z '$(SONIC_MAKE_DEBS)'\n\t@test -n '$(SONIC_BAZEL_KERNEL_DEBS)'\n")
        cases = ["CONFIGURED_PLATFORM=mellanox", "CONFIGURED_ARCH=armhf", "BLDENV=bookworm",
                 "CROSS_BUILD_ENVIRON=y", "MULTIARCH_QEMU_ENVIRON=y", "KERNEL_VERSION=6.12.42",
                 "KVERSION=other-abi", "ADDITIONAL_BUILD_PROFILES=other", "SECURE_UPGRADE_MODE=sign",
                 "INCLUDE_EXTERNAL_PATCHES=y"]
        for setting in cases:
            result = subprocess.run(["make", "--no-print-directory", "-f", str(makefile), "selection", setting],
                                    cwd=self.root, text=True, capture_output=True)
            with self.subTest(setting=setting):
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_make_forwards_only_the_shared_switch(self):
        contents = (ROOT / "Makefile.work").read_text()
        definition = "SONIC_BUILD_INSTRUCTION :=" + contents.split("SONIC_BUILD_INSTRUCTION :=", 1)[1].split("\n\n", 1)[0]
        makefile = self.root / "forward.mk"
        makefile.write_text("BUILD_WITH_BAZEL_WHEN_AVAILABLE := y\n" + definition
                            + "\n.PHONY: check\ncheck:\n\t$(file >forwarded.txt,$(SONIC_BUILD_INSTRUCTION))\n\t@true\n")
        subprocess.run(["make", "--no-print-directory", "-f", str(makefile), "check"], cwd=self.root, check=True)
        forwarded = (self.root / "forwarded.txt").read_text()
        self.assertIn("BUILD_WITH_BAZEL_WHEN_AVAILABLE=y", forwarded.split())
        self.assertNotIn("_SONIC_BAZEL_KERNEL=", forwarded)

    def test_every_make_configuration_reaches_the_producer_and_failure_propagates(self):
        before = {p.name: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.bundle.iterdir()}
        changes = {
            "CONFIGURED_ARCH": "armhf", "CONFIGURED_PLATFORM": "mellanox", "BLDENV": "bookworm",
            "CROSS_BUILD_ENVIRON": "y", "MULTIARCH_QEMU_ENVIRON": "y", "KERNEL_VERSION": "6.13.1",
            "KERNEL_ABISUFFIX": "+custom", "KERNEL_SUBVERSION": "2", "KERNEL_FEATURESET": "custom",
            "KVERSION": "custom-abi", "ADDITIONAL_BUILD_PROFILES": "nodoc nocheck", "SECURE_UPGRADE_MODE": "sign",
            "SECURE_UPGRADE_KERNEL_CAFILE": "/tmp/kernel CA's.pem", "INCLUDE_EXTERNAL_PATCHES": "y",
            "EXTERNAL_KERNEL_PATCH_URL": "https://example.test/patches.tar.gz", "EXTERNAL_KERNEL_PATCH_LOC": "/tmp/patches path",
        }
        for name, value in changes.items():
            requested = {name: value}
            expected = kernel.effective_config(self.root, self.architecture, requested)
            calls = []
            def execute(command, workspace):
                calls.append(command)
                self.assertEqual(json.loads(command[command.index("--kernel-config") + 1]), expected)
                self.assertEqual(command[command.index("--architecture") + 1], self.architecture)
                raise subprocess.CalledProcessError(2, command, stderr="unsupported producer configuration")
            def builder(workspace, state, remote, upload, **options):
                return kernel._build(workspace, state, remote, upload, execute=execute, **options)
            with self.subTest(name=name), self.assertRaises(subprocess.CalledProcessError):
                kernel.ensure(self.root, kernel_config=requested, builder=builder)
            self.assertEqual(len(calls), 1)
        self.assertEqual(before, {p.name: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.bundle.iterdir()})
        self.assertFalse((self.source / "COMPILED").exists())

    def test_configuration_changes_invalidate_retained_outputs_and_bind_metadata(self):
        requested = {"ADDITIONAL_BUILD_PROFILES": "nodoc"}
        expected = kernel.effective_config(self.root, self.architecture, requested)
        before = {name: (self.bundle / name).stat().st_mtime_ns for name in self.packages}
        calls = []
        def builder(workspace, state, remote, upload, **options):
            calls.append(options)
            outputs = self.declared_outputs(state / "build")
            manifest = json.loads((outputs / kernel.MANIFEST).read_text())
            manifest["kernel_config"] = options["kernel_config"]
            (outputs / kernel.MANIFEST).write_text(json.dumps(manifest))
            source = kernel.source_state(workspace, options["architecture"], options["kernel_config"])
            kernel.stage_outputs(state / "build", options["debs_directory"], workspace, source, self.selected)
            return {"plan": {}}
        self.assertFalse(kernel.ensure(self.root, kernel_config=requested, builder=builder)["reused"])
        self.assertEqual(calls[0]["kernel_config"], expected)
        verified = kernel.verify(self.bundle, self.root, kernel_config=requested)
        self.assertEqual(verified["manifest"]["kernel_config"], expected)
        self.assertEqual(verified["provenance"]["kernel_config"], expected)
        self.assertTrue(kernel.ensure(self.root, kernel_config=requested, builder=builder)["reused"])
        self.assertEqual(len(calls), 1)
        self.assertEqual(before, {name: (self.bundle / name).stat().st_mtime_ns for name in self.packages})
        with self.assertRaisesRegex(ValueError, "configuration"):
            kernel.verify(self.bundle, self.root)

    def test_ensure_reuses_after_unrelated_commit_without_rewriting_inputs(self):
        before = {p.name: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.bundle.iterdir()}
        (self.root / "unrelated-swss-change").write_text("one line\n")
        subprocess.run(["git", "-C", str(self.root), "add", "unrelated-swss-change"], check=True)
        self.commit(self.root, "Unrelated source change")
        self.assertNotEqual(kernel.source_state(self.root)["source_commit"], self.provenance["source_commit"])
        with patch.object(kernel, "build", side_effect=AssertionError("must not compile")):
            self.assertTrue(kernel.ensure(self.root)["reused"])
            self.assertTrue(kernel.ensure(self.root)["reused"])
        self.assertEqual(before, {p.name: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.bundle.iterdir()})

    def test_ensure_builds_once_for_parallel_requests_and_preserves_shared_cache(self):
        saved = self.root / "saved-bundle"
        self.bundle.rename(saved)
        calls = []
        cache = self.root / "shared-cache"
        cache.mkdir()
        cached_file = cache / "python"
        cached_file.write_bytes(b"shared immutable toolchain file")
        cached_file.chmod(0o444)
        external = self.root / "shared-python-lib"
        external.mkdir()
        (external / "keep.py").write_text("external toolchain data\n")
        external.chmod(0o555)
        self.addCleanup(external.chmod, 0o755)
        records = {"consumer/MODULE.bazel.lock": "complete lock", "build/module-graph.json": "complete graph",
                   "build/invocation.json": "invocation", "kernel-receipt.json": "receipt", "plan.json": "plan"}
        def builder(workspace, state, remote, upload, **options):
            calls.append((remote, upload, options))
            self.declared_outputs(state / "build", saved)
            kernel.stage_outputs(state / "build", options["debs_directory"], workspace, self.state, self.selected)
            for relative, data in records.items():
                path = state / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(data)
            toolchain = state / "build/output/external/rules_python++python+python_3_11_x86_64-unknown-linux-gnu"
            toolchain.mkdir(parents=True)
            (toolchain / "lib").symlink_to(external, target_is_directory=True)
            (toolchain / "python").hardlink_to(cached_file)
            nested = toolchain / "include/python3.11"
            nested.mkdir(parents=True)
            (nested / "Python.h").write_text("disposable toolchain header\n")
            for directory in (nested, nested.parent, toolchain):
                directory.chmod(0o555)
            return {"plan": {"disk_cache": str(cache / "kernel-linux-sandbox/disk_cache")}}
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            results = list(pool.map(lambda _: kernel.ensure(self.root, cache_directory=cache,
                remote_cache="http://127.0.0.1:8080", builder=builder), range(4)))
        self.assertEqual(len(calls), 1)
        self.assertEqual(sum(not r["reused"] for r in results), 1)
        self.assertEqual(calls[0][0:2], ("http://127.0.0.1:8080", False))
        self.assertEqual(calls[0][2]["cache_directory"], cache)
        retained, = (self.root / "target/bazel-kernel-state").iterdir()
        self.assertFalse((retained / "build/output").exists())
        self.assertFalse((retained / "bundle").exists())
        for relative, data in records.items():
            self.assertEqual((retained / relative).read_text(), data)
        self.assertEqual(cached_file.read_bytes(), b"shared immutable toolchain file")
        self.assertEqual(stat.S_IMODE(cached_file.stat().st_mode), 0o444)
        self.assertEqual((external / "keep.py").read_text(), "external toolchain data\n")
        self.assertEqual(stat.S_IMODE(external.stat().st_mode), 0o555)
        prepared = kernel.plan(self.root, self.root / "plan-state", None, False, cache_directory=cache)
        argv = prepared["argv"]
        self.assertEqual(argv[argv.index("--disk-cache") + 1], str(cache / "kernel-linux-sandbox/disk_cache"))
        self.assertEqual(argv[argv.index("--repository-cache") + 1], str(cache / "kernel-linux-sandbox/repository_cache"))

    def test_ensure_restores_missing_debs_without_touching_other_packages(self):
        saved = self.root / "saved-outputs"
        shutil.copytree(self.bundle, saved)
        missing = next(iter(self.packages))
        (self.bundle / missing).unlink()
        unrelated = self.bundle / "unrelated.deb"
        unrelated.write_bytes(b"unrelated package")
        before = {p.name: (p.read_bytes(), p.stat().st_mtime_ns)
                  for p in self.bundle.iterdir() if p.name != kernel.PROVENANCE}
        calls = []
        def builder(workspace, state, *args, **options):
            calls.append(options)
            self.declared_outputs(state / "build", saved)
            kernel.stage_outputs(state / "build", options["debs_directory"], workspace, self.state, self.selected)
            return {"plan": {}}
        self.assertFalse(kernel.ensure(self.root, builder=builder)["reused"])
        self.assertEqual(len(calls), 1)
        self.assertEqual((self.bundle / missing).read_bytes(), (saved / missing).read_bytes())
        for name, original in before.items():
            path = self.bundle / name
            self.assertEqual((path.read_bytes(), path.stat().st_mtime_ns), original)
        self.assertTrue(kernel.ensure(self.root, builder=builder)["reused"])
        self.assertEqual(len(calls), 1)

    def test_missing_deb_does_not_hide_corrupted_retained_package(self):
        missing, corrupt = list(self.packages)[:2]
        (self.bundle / missing).unlink()
        (self.bundle / corrupt).write_bytes(b"corrupted package")
        with self.assertRaisesRegex(ValueError, "SHA256"):
            kernel.ensure(self.root, builder=lambda *a, **kw: self.fail("must not hide corruption"))

    def test_explicit_deb_directory_reuses_only_its_verified_packages(self):
        selected = self.root / "custom-debs"
        self.bundle.rename(selected)
        result = kernel.ensure(self.root, debs_directory=Path("custom-debs"))
        self.assertTrue(result["reused"])
        self.assertEqual(result["debs_directory"], str(selected))
        planned = kernel.plan(self.root, self.root / "state", None, False, debs_directory=selected)
        self.assertEqual(planned["debs_directory"], str(selected))
        self.assertFalse(self.bundle.exists())

    def test_ensure_corruption_fails_without_falling_back_or_recompiling(self):
        (self.bundle / next(iter(self.packages))).write_bytes(b"corrupt")
        with patch.object(kernel, "build", side_effect=AssertionError("must not hide corruption")):
            with self.assertRaisesRegex(ValueError, "SHA256"):
                kernel.ensure(self.root)

    def test_ensure_changed_handoff_inputs_builds_and_preserves_equal_package_mtimes(self):
        before = {p.name: p.stat().st_mtime_ns for p in self.bundle.iterdir() if p.name != kernel.PROVENANCE}
        helper = self.root / "tools/bazel/ci/resolution.py"
        helper.write_text(helper.read_text() + "\n# changed verifier\n")
        subprocess.run(["git", "-C", str(self.root), "add", str(helper)], check=True)
        self.commit(self.root, "Change handoff verifier")
        state = kernel.source_state(self.root)
        calls = []
        def builder(workspace, output, *args, **options):
            calls.append(options)
            self.declared_outputs(output / "build")
            kernel.stage_outputs(output / "build", options["debs_directory"], workspace, state, self.selected)
            return {"plan": {}}
        self.assertFalse(kernel.ensure(self.root, builder=builder)["reused"])
        self.assertEqual(len(calls), 1)
        self.assertEqual(before, {p.name: p.stat().st_mtime_ns for p in self.bundle.iterdir() if p.name != kernel.PROVENANCE})
        self.assertTrue(kernel.ensure(self.root, builder=builder)["reused"])
        self.assertEqual(len(calls), 1)

    def test_downloader_policy_changes_invalidate_kernel_reuse(self):
        policy = self.root / kernel.DOWNLOADER_CONFIG
        policy.write_text(policy.read_text() + "\n# changed download policy\n")
        with self.assertRaisesRegex(ValueError, "handoff inputs must match"):
            kernel.source_state(self.root)
        subprocess.run(["git", "-C", str(self.root), "add", str(policy)], check=True)
        self.commit(self.root, "Change Debian download policy")
        current = kernel.source_state(self.root)
        self.assertNotEqual(current["consumer_inputs_sha256"], self.state["consumer_inputs_sha256"])
        calls = []

        def builder(*args, **options):
            calls.append(options)
            raise RuntimeError("rebuild required for changed download policy")

        with self.assertRaisesRegex(RuntimeError, "rebuild required"):
            kernel.ensure(self.root, builder=builder)
        self.assertEqual(len(calls), 1)

    def test_runtime_installation_excludes_matching_debug_package(self):
        makefile = self.make_fixture()
        result = subprocess.run(["make", "--no-print-directory", "--no-builtin-rules", "-f", str(makefile)],
                                cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        output = self.root / "target/debs/trixie"
        script = (ROOT / "build_debian.sh").read_text()
        start = script.index("kernel_image_debs=()")
        selection = script[start:script.index("sudo cp", start)]
        selected = subprocess.check_output(["bash", "-c", selection + '\nprintf "%s\\n" "${kernel_image_debs[@]}"'],
            env={"debs_path": str(output), "LINUX_KERNEL_VERSION": "6.12.41+deb13", "CONFIGURED_ARCH": self.architecture}, text=True).splitlines()
        runtime = next(name for name, item in self.packages.items() if item["kind"] == "runtime")
        self.assertEqual(selected, [str(output / runtime)])


class KernelConfigurationTest(unittest.TestCase):
    def setUp(self):
        registry = patch.dict(os.environ, {"SONIC_REVIEW_REGISTRY": kernel.DEFAULT_REGISTRY,
                                           "SONIC_BAZEL_REMOTE_ONLY_RC": ""})
        registry.start()
        self.addCleanup(registry.stop)

    def test_cleanup_rejects_a_symlink_root_without_touching_its_target(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            retained = root / "retained"
            retained.mkdir()
            (retained / "package.deb").write_bytes(b"verified package")
            output = root / "output"
            output.symlink_to(retained, target_is_directory=True)
            before = retained.stat().st_mode
            with self.assertRaisesRegex(ValueError, "unexpected kernel build output directory"):
                kernel.remove_build_tree(output)
            self.assertTrue(output.is_symlink())
            self.assertEqual((retained / "package.deb").read_bytes(), b"verified package")
            self.assertEqual(retained.stat().st_mode, before)

    def test_registry_requires_exactly_one_maintained_endpoint(self):
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory)
            shutil.copyfile(ROOT / "tools/bazel/kernel/.bazelrc", workspace / ".bazelrc")
            original = (workspace / ".bazelrc").read_text()
            self.assertEqual(kernel.configure_registry(workspace), kernel.DEFAULT_REGISTRY)
            self.assertEqual((workspace / ".bazelrc").read_text(), original)
            for invalid in (
                original.replace("/main", "/codex/sonic-linux-kernel"),
                original + "common --registry=" + kernel.DEFAULT_REGISTRY + "\n",
                original + "common --registry=https://example.test/registry\n",
            ):
                (workspace / ".bazelrc").write_text(invalid)
                with self.assertRaisesRegex(ValueError, "one SONiC registry endpoint on main"):
                    kernel.configure_registry(workspace)
                self.assertEqual((workspace / ".bazelrc").read_text(), invalid)

    def test_review_registry_replaces_only_the_private_consumer_endpoint(self):
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory)
            template = ROOT / "tools/bazel/kernel/.bazelrc"
            original = template.read_text()
            shutil.copyfile(template, workspace / ".bazelrc")
            reviewed = kernel.REGISTRY_PREFIX + "codex/kernel-sandbox-registry"
            with patch.dict(os.environ, {"SONIC_REVIEW_REGISTRY": reviewed}):
                self.assertEqual(kernel.configure_registry(workspace), reviewed)
            self.assertEqual((workspace / ".bazelrc").read_text(), original.replace(kernel.DEFAULT_REGISTRY, reviewed))
            self.assertEqual(template.read_text(), original)

    def test_review_registry_rejects_other_hosts_and_invalid_refs(self):
        for invalid in ("", "https://example.test/registry", kernel.DEFAULT_REGISTRY + "?next=main",
                        kernel.REGISTRY_PREFIX + "../other", kernel.DEFAULT_REGISTRY + "\n--registry=other"):
            with self.subTest(registry=invalid), patch.dict(os.environ, {"SONIC_REVIEW_REGISTRY": invalid}):
                with self.assertRaisesRegex(ValueError, "one SONiC registry ref"):
                    kernel.selected_registry()

    def test_distdir_requires_exact_pinned_source_archives(self):
        with tempfile.TemporaryDirectory() as temporary:
            workspace = Path(temporary)
            directory = workspace / "archives"
            directory.mkdir()
            files = []
            for name in ("linux_6.12.41-1.dsc", "linux_6.12.41.orig.tar.xz", "linux_6.12.41-1.debian.tar.xz"):
                path = directory / name
                path.write_bytes(name.encode())
                files.append({"name": name, "sha256": kernel.sha256(path)})
            with patch.object(kernel, "source_state", return_value={"source_archives": files, "kernel_config": {}, "launcher": {"architecture": kernel.native_architecture()}}):
                prepared = kernel.plan(workspace, workspace / "state", None, False, distdir=directory)
                self.assertEqual(prepared["argv"][prepared["argv"].index("--distdir") + 1], str(directory))
                self.assertEqual({item["name"] for item in prepared["distdir"]["files"]}, {item["name"] for item in files})
                path.write_bytes(b"changed archive")
                with self.assertRaisesRegex(ValueError, "source archive SHA256 differs"):
                    kernel.plan(workspace, workspace / "state", None, False, distdir=directory)
                path.write_bytes(path.name.encode())
                (directory / "unrelated").write_bytes(b"unrelated input")
                with self.assertRaisesRegex(ValueError, "exactly the pinned archives"):
                    kernel.plan(workspace, workspace / "state", None, False, distdir=directory)

    def test_contract_tracks_make_defaults_and_explicit_configuration(self):
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory)
            (workspace / "rules").mkdir()
            contents = (ROOT / "rules/linux-kernel.mk").read_text().replace("KERNEL_VERSION = 6.12.41", "KERNEL_VERSION = 6.12.42")
            (workspace / "rules/linux-kernel.mk").write_text(contents)
            expected, packages = kernel.contract(workspace, "amd64")
            self.assertEqual(expected["kernel_version"], "6.12.42")
            self.assertIn("linux-image-6.12.42+deb13-sonic-amd64-unsigned_6.12.42-1_amd64.deb", packages)
            values = {"CONFIGURED_ARCH": "armhf", "CONFIGURED_PLATFORM": "example",
                      "KERNEL_VERSION": "6.13.1", "KERNEL_ABISUFFIX": "+custom", "KERNEL_SUBVERSION": "2"}
            expected, packages = kernel.contract(workspace, "amd64", values)
            self.assertEqual(expected["platform"], "example")
            self.assertEqual(expected["kernel_abi"], "6.13.1+custom-sonic-armmp")
            self.assertIn("linux-image-6.13.1+custom-sonic-armmp_6.13.1-2_armhf.deb", packages)
            self.assertNotIn("unsigned", next(name for name, item in packages.items() if item["kind"] == "runtime"))

    def test_secure_boot_request_does_not_claim_the_kernel_deb_is_signed(self):
        for mode in ("no_sign", "dev", "prod"):
            with self.subTest(mode=mode):
                expected, packages = kernel.contract(ROOT, "amd64", {"SECURE_UPGRADE_MODE": mode})
                self.assertEqual(expected["signing"], "unsigned")
                self.assertEqual(expected["kernel_config"]["SECURE_UPGRADE_MODE"], mode)
                runtime = next(name for name, item in packages.items() if item["kind"] == "runtime")
                self.assertIn("-unsigned_", runtime)

    def test_config_parser_preserves_values_and_normalizes_make_false_defaults(self):
        supplied = kernel.parse_kernel_config(["ADDITIONAL_BUILD_PROFILES=a b", "EXTERNAL_KERNEL_PATCH_LOC=/tmp/a=b's",
            'SECURE_UPGRADE_MODE="no_sign"', "CROSS_BUILD_ENVIRON=", "MULTIARCH_QEMU_ENVIRON=", "INCLUDE_EXTERNAL_PATCHES="])
        values = kernel.effective_config(ROOT, "amd64", supplied)
        self.assertEqual(values["ADDITIONAL_BUILD_PROFILES"], "a b")
        self.assertEqual(values["EXTERNAL_KERNEL_PATCH_LOC"], "/tmp/a=b's")
        self.assertEqual(values["SECURE_UPGRADE_MODE"], "no_sign")
        self.assertEqual(kernel.effective_config(ROOT, "amd64", {"SECURE_UPGRADE_MODE": ""})["SECURE_UPGRADE_MODE"], "no_sign")
        for name in ("CROSS_BUILD_ENVIRON", "MULTIARCH_QEMU_ENVIRON", "INCLUDE_EXTERNAL_PATCHES"):
            self.assertEqual(values[name], "n")
        for assignments in (["MISSING_EQUALS"], ["UNKNOWN=y"], ["KVERSION=a", "KVERSION=b"]):
            with self.subTest(assignments=assignments), self.assertRaises(ValueError):
                kernel.parse_kernel_config(assignments)

    def test_cache_endpoint_rejects_credentials_and_control_characters(self):
        for value in ("http://127.0.0.1:8080", "https://cache.example.test", "grpcs://cache.example.test:9092"):
            self.assertEqual(kernel.endpoint(value), value)
        for value in ("https://token@cache.example.test", "https://cache.example.test?token=private",
                      "https://cache.example.test/#secret", "file:///cache", "https://cache.example.test\n"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                kernel.endpoint(value)


class KernelWorkflowTest(unittest.TestCase):
    def setUp(self):
        self.workflow = (ROOT / ".github/workflows/bazel-kernel-bundle.yml").read_text()
        self.steps = dict(re.findall(
            r"^      - name: ([^\n]+)\n(.*?)(?=^      - |\Z)",
            self.workflow, re.MULTILINE | re.DOTALL))

    def test_self_hosted_kernel_ci_accepts_all_same_repository_prs(self):
        self.assertEqual(re.findall(r"^    if: (.+)$", self.workflow, re.MULTILINE), [
            "github.event_name != 'pull_request' || github.event.pull_request.head.repo.full_name == github.repository",
        ])

    def test_shared_cache_defaults_and_optional_cold_proof(self):
        self.assertIn("runner: [self-hosted, linux, bazel-x64]", self.workflow)
        self.assertIn("runner: [self-hosted, linux, bazel-arm64]", self.workflow)
        cold = "github.event_name == 'workflow_dispatch' && inputs.verify_cold_cache"
        self.assertIn("KERNEL_REMOTE_CACHE: ${{ " + cold
                      + " && 'http://127.0.0.1:8080' || 'grpc://172.16.0.4:9092' }}",
                      self.workflow)
        self.assertIn("UPLOAD_KERNEL_CACHE: ${{ (" + cold
                      + ") || (github.event_name == 'push' && github.ref == 'refs/heads/master') }}",
                      self.workflow)
        for name in ("Start a disposable cache for optional cold-build verification",
                     "Verify a fresh output directory reuses the kernel action"):
            self.assertIn("if: " + cold, self.steps[name])
        self.assertIn("if: always() && steps.kernel_state.outcome == 'success' && " + cold,
                      self.steps["Stop the disposable verification cache"])
        self.assertIn("'kernel-cache-hit' || 'kernel-bundle-state'",
                      self.steps["Select verified DEBs and a public validation summary"])
        self.assertNotIn("sudo rm", self.workflow)

    def test_fixed_state_is_cleaned_before_build_and_after_artifact_upload(self):
        before = self.steps["Remove stale fixed kernel state"]
        after = self.steps["Clean fixed kernel scratch state"]
        command = "run: python3 -B tools/bazel/ci/kernel_scratch.py cleanup"
        self.assertIn(command, before)
        self.assertIn(command, after)
        self.assertIn("id: kernel_state", before)
        self.assertIn(
            "if: always() && steps.kernel_state.outcome == 'success' "
            "&& (steps.kernel_cache_stop.outcome == 'success' "
            "|| steps.kernel_cache_stop.outcome == 'skipped')", after
        )
        self.assertLess(self.workflow.index("- name: Remove stale fixed kernel state"),
                        self.workflow.index("- name: Check available scratch space"))
        self.assertLess(self.workflow.index("- name: Retain verified DEBs"),
                        self.workflow.index("- name: Clean fixed kernel scratch state"))
        self.assertLess(self.workflow.index("- name: Stop the disposable verification cache"),
                        self.workflow.index("- name: Clean fixed kernel scratch state"))
        self.assertIn("KERNEL_RUN_TEMP: /mnt/build/k", self.workflow)
        self.assertNotIn("kernel-job.", self.workflow)
        self.assertIn("path: ${{ env.KERNEL_RUN_TEMP }}/kernel-bundle-artifacts", self.workflow)
        self.assertLess(self.workflow.index("- name: Prepare short kernel scratch root"),
                        self.workflow.index("- name: Remove stale fixed kernel state"))
        self.assertLess(self.workflow.index("- name: Provision short kernel scratch root"),
                        self.workflow.index("- name: Prepare short kernel scratch root"))
        for name, step in self.steps.items():
            if name not in ("Restore workspace access before checkout", "Provision short kernel scratch root"):
                self.assertNotIn("$RUNNER_TEMP", step, name)
        self.assertIn("os.environ['KERNEL_RUN_TEMP']", self.steps["Check available scratch space"])
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            workspace = root / "workspace"
            workspace.mkdir()
            for _ in range(2):
                (root / "kernel-bundle-state").mkdir()
                result = subprocess.run(
                    [sys.executable, "-B", str(ROOT / "tools/bazel/ci/kernel_scratch.py"), "cleanup"],
                    cwd=ROOT, capture_output=True, text=True,
                    env=dict(os.environ, KERNEL_RUN_TEMP=temporary, GITHUB_WORKSPACE=str(workspace)),
                )
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse((root / "kernel-bundle-state").exists())
                self.assertTrue(workspace.is_dir())

    def test_scratch_provisioning_creates_only_an_absent_root_without_elevating_builds(self):
        step = self.steps["Provision short kernel scratch root"]
        script = textwrap.dedent(step.split("        run: |\n", 1)[1])
        for scenario in ("absent", "existing", "wrong-owner", "symlink", "dangling-symlink", "file",
                         "parent-symlink", "parent-writable", "parent-owner", "other-disk",
                         "wrong-path", "root-user", "sudo-failure", "chown-failure", "mkdir-race"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                build = root / "build"
                build.mkdir()
                build.chmod(0o755)
                scratch = build / "k"
                runner_temp = root / "temp"
                runner_temp.mkdir()
                commands = root / "commands"
                commands.mkdir()
                uid, gid = "1001", "1002"
                identity = commands / "id"
                identity.write_text(
                    "#!" + sys.executable + "\n"
                    "import os, sys\n"
                    "assert sys.argv[1:] in (['-u'], ['-g'])\n"
                    "if sys.argv[1] == '-u':\n"
                    "    print('0' if os.environ['SCENARIO'] == 'root-user' else os.environ['TEST_UID'])\n"
                    "else: print(os.environ['TEST_GID'])\n")
                identity.chmod(0o755)
                calls = root / "sudo-calls.jsonl"
                retained = root / "retained"
                retained.mkdir()
                (retained / "receipt").write_text("keep")
                if scenario in ("existing", "wrong-owner"):
                    scratch.mkdir(mode=0o700 if scenario == "existing" else 0o755)
                    (scratch / "receipt").write_text("keep")
                elif scenario in ("symlink", "dangling-symlink"):
                    scratch.symlink_to(retained if scenario == "symlink" else root / "missing")
                elif scenario == "file":
                    scratch.write_text("not a directory")
                elif scenario == "parent-symlink":
                    build.rmdir()
                    build.symlink_to(retained, target_is_directory=True)
                sudo = commands / "sudo"
                sudo.write_text(
                    "#!" + sys.executable + "\n"
                    "import json, os, subprocess, sys\n"
                    "from pathlib import Path\n"
                    "with open(os.environ['CALLS'], 'a') as stream:\n"
                    "    stream.write(json.dumps(sys.argv[1:]) + '\\n')\n"
                    "assert sys.argv[1] == '-n'\n"
                    "scenario = os.environ['SCENARIO']\n"
                    "if scenario == 'sudo-failure': sys.exit(23)\n"
                    "if sys.argv[2] == 'mkdir':\n"
                    "    if scenario == 'mkdir-race': Path(sys.argv[-1]).mkdir(mode=0o755)\n"
                    "    sys.exit(subprocess.run(sys.argv[2:]).returncode)\n"
                    "assert sys.argv[2] == 'chown'\n"
                    "assert sys.argv[3:5] == ['--no-dereference', '--']\n"
                    "assert sys.argv[5] == os.environ['TEST_UID'] + ':' + os.environ['TEST_GID']\n"
                    "sys.exit(24 if scenario == 'chown-failure' else 0)\n")
                sudo.chmod(0o755)
                stat_command = commands / "stat"
                stat_command.write_text(
                    "#!" + sys.executable + "\n"
                    "import os, subprocess, sys\n"
                    "scenario = os.environ['SCENARIO']\n"
                    "field = sys.argv[2]\n"
                    "if field == '%u':\n"
                    "    print('1000' if scenario == 'parent-owner' else '0')\n"
                    "elif field == '%a' and scenario == 'parent-writable': print('777')\n"
                    "elif field == '%d' and scenario == 'other-disk' "
                    "and sys.argv[-1] == os.environ['RUNNER_TEMP']: print('-1')\n"
                    "else: sys.exit(subprocess.run(['/usr/bin/stat'] + sys.argv[1:]).returncode)\n")
                stat_command.chmod(0o755)
                selected_script = script.replace("/mnt", str(root))
                result = subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", selected_script],
                    cwd=root, capture_output=True, text=True,
                    env=dict(os.environ, PATH=str(commands) + os.pathsep + os.environ["PATH"],
                             RUNNER_TEMP=str(runner_temp), KERNEL_RUN_TEMP=str(
                                 retained if scenario == "wrong-path" else scratch),
                             CALLS=str(calls), SCENARIO=scenario, TEST_UID=uid, TEST_GID=gid))
                success = scenario in ("absent", "existing", "wrong-owner")
                expected_exit = 0 if success else {"sudo-failure": 23, "chown-failure": 24}.get(scenario, 1)
                self.assertEqual(result.returncode, expected_exit, result.stdout + result.stderr)
                if scenario == "root-user":
                    self.assertIn("unexpected user", result.stderr)
                if scenario in ("absent", "chown-failure"):
                    self.assertEqual(scratch.stat().st_mode & 0o777, 0o700)
                    self.assertEqual(
                        [json.loads(line) for line in calls.read_text().splitlines()],
                        [["-n", "mkdir", "--mode=0700", "--", str(scratch)],
                         ["-n", "chown", "--no-dereference", "--",
                          f"{uid}:{gid}", str(scratch)]])
                elif scenario in ("sudo-failure", "mkdir-race"):
                    self.assertEqual(len(calls.read_text().splitlines()), 1)
                else:
                    self.assertFalse(calls.exists())
                if scenario in ("existing", "wrong-owner"):
                    self.assertEqual((scratch / "receipt").read_text(), "keep")
                    self.assertEqual(scratch.stat().st_mode & 0o777,
                                     0o700 if scenario == "existing" else 0o755)
                self.assertEqual((retained / "receipt").read_text(), "keep")
                self.assertEqual(root.stat().st_mode & 0o777, 0o700)
                if scenario != "parent-symlink":
                    self.assertEqual(build.stat().st_mode & 0o777, 0o755)
        for name, content in self.steps.items():
            if name not in ("Restore workspace access before checkout", "Provision short kernel scratch root"):
                self.assertNotIn("sudo", content, name)

    def test_short_scratch_root_is_private_and_bound_to_one_runner(self):
        for scenario in ("reuse", "other-runner", "other-temp", "symlink", "dangling-symlink",
                         "alias-parent", "public", "file", "workspace", "temp", "relative",
                         "owner-symlink", "owner-file", "owner-directory", "wrong-uid", "other-disk"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                workspace = root / "workspace"
                workspace.mkdir()
                runner_temp = root / "temp"
                runner_temp.mkdir()
                scratch = root / "k"
                kernel_cleanup.prepare_root(scratch, runner_temp, workspace, "agent-one")
                owner = scratch / ".runner-owner"
                identity = owner.read_text()
                (scratch / "kernel-bundle-state").mkdir()
                retained = scratch / "kernel-bundle-state/receipt"
                retained.write_text("retained")
                runner_name = "agent-one"
                selected = scratch
                if scenario == "other-runner":
                    runner_name = "agent-two"
                elif scenario == "other-temp":
                    runner_temp = root / "other-temp"
                    runner_temp.mkdir()
                elif scenario in ("symlink", "dangling-symlink"):
                    selected = root / "link"
                    selected.symlink_to(scratch if scenario == "symlink" else root / "missing")
                elif scenario == "alias-parent":
                    alias = root / "alias"
                    alias.symlink_to(root, target_is_directory=True)
                    selected = alias / "k"
                elif scenario == "public":
                    scratch.chmod(0o755)
                elif scenario == "file":
                    selected = root / "file"
                    selected.write_text("not a directory")
                elif scenario == "workspace":
                    workspace = scratch / "kernel-bundle-state"
                elif scenario == "temp":
                    runner_temp = scratch
                elif scenario == "relative":
                    selected = Path("relative")
                elif scenario.startswith("owner-"):
                    owner.unlink()
                    if scenario == "owner-symlink":
                        owner.symlink_to(retained)
                    elif scenario == "owner-directory":
                        owner.mkdir()
                    else:
                        owner.write_text("another identity")
                original_stat = Path.stat

                def filesystem(path, *args, **kwargs):
                    metadata = original_stat(path, *args, **kwargs)
                    if scenario == "other-disk" and path == runner_temp:
                        values = list(metadata)
                        values[2] = metadata.st_dev + 1
                        return os.stat_result(values)
                    return metadata

                with patch.object(Path, "stat", autospec=True, side_effect=filesystem), \
                        patch.object(kernel_cleanup.os, "geteuid",
                                     return_value=-1 if scenario == "wrong-uid" else os.geteuid()):
                    if scenario == "reuse":
                        kernel_cleanup.prepare_root(selected, runner_temp, workspace, runner_name)
                        self.assertEqual(scratch.stat().st_mode & 0o777, 0o700)
                        kernel_cleanup.cleanup(scratch, workspace)
                        self.assertFalse(retained.exists())
                        self.assertEqual(owner.read_text(), identity)
                    else:
                        with self.assertRaises((ValueError, FileExistsError)):
                            kernel_cleanup.prepare_root(selected, runner_temp, workspace, runner_name)
                        self.assertEqual(retained.read_text(), "retained")

    def test_kernel_scratch_path_budget_includes_worst_case_sandbox_and_longest_header(self):
        kernel_cleanup.check_path_budget(Path("/mnt/build/k"), "6.12.41")
        kernel_cleanup.check_path_budget(Path("/mnt/build/" + "k" * 11), "6.12.41")
        for root, version in (
                (Path("/mnt/build/" + "k" * 12), "6.12.41"),
                (Path("/mnt/build/actions-runner-work/_temp"), "6.12.41"),
                (Path("/mnt/build/" + "\u00e9" * 6), "6.12.41"),
                (Path("/mnt/build/k"), "6.12.41" + "0" * 11)):
            with self.subTest(root=root, version=version):
                with self.assertRaisesRegex(ValueError, "255-byte limit"):
                    kernel_cleanup.check_path_budget(root, version)

    def test_scratch_cleanup_is_scoped_and_rejects_redirected_paths(self):
        for scenario in ("cleanup", "symlink", "dangling-symlink", "workspace", "workspace-parent",
                         "file", "mount", "nested-mount", "file-symlink", "file-directory",
                         "workspace-child", "relative", "root"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                workspace = root / "workspace"
                workspace.mkdir()
                shared = root / "shared-cache"
                shared.mkdir()
                retained = shared / "object"
                retained.write_text("retained")
                retained.chmod(0o444)
                metadata = retained.stat()
                unrelated = root / "unrelated"
                unrelated.mkdir()
                (unrelated / "receipt").write_text("keep")
                for name in kernel_cleanup.DIRECTORIES:
                    state = root / name
                    state.mkdir()
                    os.link(retained, state / "hardlink")
                    (state / "cache-link").symlink_to(shared, target_is_directory=True)
                    if scenario == "cleanup":
                        state.chmod(0o555)
                for name in kernel_cleanup.FILES:
                    (root / name).write_text("disposable")
                scratch = root / "kernel-unsupported"
                if scenario in ("symlink", "dangling-symlink", "file"):
                    shutil.rmtree(scratch)
                    if scenario in ("symlink", "dangling-symlink"):
                        scratch.symlink_to(shared if scenario == "symlink" else root / "missing",
                                           target_is_directory=True)
                    else:
                        scratch.write_text("unexpected file")
                if scenario == "workspace":
                    workspace = scratch
                elif scenario == "workspace-parent":
                    workspace = scratch / "nested-workspace"
                    workspace.mkdir()
                elif scenario == "workspace-child":
                    workspace = root
                if scenario in ("file-symlink", "file-directory"):
                    path = root / kernel_cleanup.FILES[-1]
                    path.unlink()
                    if scenario == "file-symlink":
                        path.symlink_to(retained)
                    else:
                        path.mkdir()
                mountinfo = ""
                if scenario in ("mount", "nested-mount"):
                    mount = scratch if scenario == "mount" else scratch / "nested"
                    mountinfo = f"1 0 0:0 / {mount} rw - ext4 /dev/fixture rw\n"
                selected = Path(".") if scenario == "relative" else Path("/") if scenario == "root" else root
                with patch.object(Path, "read_text", return_value=mountinfo):
                    if scenario == "cleanup":
                        kernel_cleanup.cleanup(selected, workspace)
                        kernel_cleanup.cleanup(selected, workspace)
                    else:
                        with self.assertRaises(ValueError):
                            kernel_cleanup.cleanup(selected, workspace)
                self.assertEqual(retained.read_text(), "retained")
                self.assertEqual(retained.stat().st_mode, metadata.st_mode)
                self.assertEqual(retained.stat().st_mtime_ns, metadata.st_mtime_ns)
                self.assertEqual((unrelated / "receipt").read_text(), "keep")
                if scenario == "cleanup":
                    self.assertTrue(all(not (root / name).exists()
                                        for name in kernel_cleanup.DIRECTORIES + kernel_cleanup.FILES))
                else:
                    self.assertTrue((root / "kernel-bundle-state/hardlink").exists())

    def test_scratch_cleanup_requires_a_stopped_cache(self):
        for scenario in ("live", "stopped", "invalid", "symlink", "denied"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                workspace = root / "workspace"
                workspace.mkdir()
                cache = root / "kernel-cache"
                cache.mkdir()
                pid = cache / "server.pid"
                pid.write_text("12345\n" if scenario != "invalid" else "-1\n")
                if scenario == "symlink":
                    pid.unlink()
                    pid.symlink_to(workspace / "missing")
                error = ProcessLookupError() if scenario == "stopped" else PermissionError() if scenario == "denied" else None
                with patch.object(kernel_cleanup.os, "kill", side_effect=error) as probe:
                    if scenario == "stopped":
                        kernel_cleanup.cleanup(root, workspace)
                        self.assertFalse(cache.exists())
                    else:
                        with self.assertRaises(PermissionError if scenario == "denied" else ValueError):
                            kernel_cleanup.cleanup(root, workspace)
                        self.assertTrue(cache.exists())
                    if scenario in ("invalid", "symlink"):
                        probe.assert_not_called()
                    else:
                        probe.assert_called_once_with(12345, 0)

    def test_workspace_repair_is_scoped_and_preserves_cache_links(self):
        name = "Restore workspace access before checkout"
        step = self.steps[name]
        self.assertLess(self.workflow.index("- name: " + name),
                        self.workflow.index("- uses: actions/checkout@"))
        self.assertIn("working-directory: ${{ runner.temp }}", step)
        self.assertIn("RUNNER_PIPELINE_WORKSPACE: ${{ runner.workspace }}", step)
        script = textwrap.dedent(step.split("        run: |\n", 1)[1])
        for scenario in ("repair", "separate-temp", "aliased-work-root",
                         "absent", "outside", "symlink", "sudo-failure"):
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                work_root = root / "work"
                scratch = (root / "separate-disk/_temp" if scenario == "separate-temp"
                           else work_root / "_temp")
                scratch.mkdir(parents=True)
                workspace = work_root / "sonic-buildimage/sonic-buildimage"
                workspace.mkdir(parents=True)
                outside = root / "cache"
                outside.mkdir()
                payload = outside / "object"
                payload.write_text("shared cache object")
                payload.chmod(0o444)
                linked = workspace / "hardlink"
                os.link(payload, linked)
                (workspace / "symlink").symlink_to(outside, target_is_directory=True)
                regular = workspace / "tracked-file"
                regular.write_text("tracked")
                regular.chmod(0o444)
                nested = workspace / ".git"
                nested.mkdir()
                nested.chmod(0o555)
                outside.chmod(0o555)
                commands = root / "commands"
                commands.mkdir()
                calls = root / "sudo-calls.jsonl"
                changed = root / "chown-calls.jsonl"
                sudo = commands / "sudo"
                sudo.write_text(
                    "#!" + sys.executable + "\n"
                    "import json, os, sys\n"
                    "with open(os.environ['SUDO_CALLS'], 'a') as f:\n"
                    "    f.write(json.dumps(sys.argv[1:]) + '\\n')\n"
                    "if int(os.environ['SUDO_EXIT']):\n"
                    "    sys.exit(int(os.environ['SUDO_EXIT']))\n"
                    "assert sys.argv[1] == '-n'\n"
                    "os.execvp(sys.argv[2], sys.argv[2:])\n")
                sudo.chmod(0o755)
                chown = commands / "chown"
                chown.write_text(
                    "#!" + sys.executable + "\n"
                    "import json, os, sys\n"
                    "with open(os.environ['CHOWN_CALLS'], 'a') as f:\n"
                    "    f.write(json.dumps(sys.argv[1:]) + '\\n')\n")
                chown.chmod(0o755)
                selected = workspace
                pipeline = workspace.parent
                if scenario == "absent":
                    selected = work_root / "missing/missing"
                    pipeline = selected.parent
                elif scenario == "outside":
                    selected = outside
                elif scenario == "symlink":
                    shutil.rmtree(workspace)
                    workspace.symlink_to(outside, target_is_directory=True)
                elif scenario == "aliased-work-root":
                    alias = root / "work-link"
                    alias.symlink_to(work_root, target_is_directory=True)
                    selected = alias / "sonic-buildimage/sonic-buildimage"
                    pipeline = selected.parent
                repository = "missing" if scenario == "absent" else "sonic-buildimage"
                result = subprocess.run(
                    ["bash", "-e", "-o", "pipefail", "-c", script],
                    cwd=scratch, capture_output=True, text=True,
                    env=dict(os.environ, PATH=str(commands) + os.pathsep + os.environ["PATH"],
                             RUNNER_TEMP=str(scratch), GITHUB_WORKSPACE=str(selected),
                             RUNNER_PIPELINE_WORKSPACE=str(pipeline),
                             GITHUB_REPOSITORY="owner/" + repository,
                             SUDO_CALLS=str(calls), CHOWN_CALLS=str(changed),
                             SUDO_EXIT="23" if scenario == "sudo-failure" else "0"))
                expected_exit = {"outside": 1, "symlink": 1, "sudo-failure": 23}.get(scenario, 0)
                self.assertEqual(result.returncode, expected_exit, result.stdout + result.stderr)
                if scenario in ("repair", "separate-temp", "aliased-work-root"):
                    self.assertEqual(nested.stat().st_mode & 0o777, 0o755)
                    self.assertEqual(regular.stat().st_mode & 0o777, 0o444)
                    ownership_calls = [json.loads(line) for line in changed.read_text().splitlines()]
                    paths = [path for call in ownership_calls for path in call[3:]]
                    self.assertIn(str(regular), paths)
                    self.assertNotIn(str(linked), paths)
                    self.assertNotIn(str(workspace / "symlink"), paths)
                    self.assertTrue(all(path.startswith(str(workspace)) for path in paths))
                    self.assertEqual(len(calls.read_text().splitlines()), 3)
                elif scenario != "sudo-failure":
                    self.assertFalse(calls.exists())
                self.assertEqual(payload.read_text(), "shared cache object")
                self.assertEqual(payload.stat().st_mode & 0o777, 0o444)
                self.assertEqual(outside.stat().st_mode & 0o777, 0o555)
                outside.chmod(0o755)
                if nested.exists():
                    nested.chmod(0o755)

    def test_build_cache_flags_and_failure_propagation(self):
        step = self.steps["Build and verify kernel DEBs in the normal package directory"]
        script = textwrap.dedent(step.split("        run: |\n", 1)[1])
        script = script.replace("${{ matrix.arch }}", "arm64")
        for endpoint, upload in (("grpc://172.16.0.4:9092", "true"),
                                 ("grpc://172.16.0.4:9092", "false"),
                                 ("http://127.0.0.1:8080", "true")):
            for exit_code in (0, 9):
                with self.subTest(endpoint=endpoint, upload=upload, exit_code=exit_code):
                    with tempfile.TemporaryDirectory() as temporary:
                        root = Path(temporary)
                        executable = root / "python3"
                        executable.write_text(
                            "#!" + sys.executable + "\n"
                            "import json, os, sys\n"
                            "from pathlib import Path\n"
                            "Path(os.environ['CALLS']).write_text(json.dumps(sys.argv[1:]))\n"
                            "sys.exit(int(os.environ['BUILD_EXIT']))\n")
                        executable.chmod(0o755)
                        calls = root / "calls.json"
                        result = subprocess.run(
                            ["bash", "-e", "-o", "pipefail", "-c", script],
                            cwd=root, capture_output=True, text=True,
                            env=dict(os.environ, PATH=str(root) + os.pathsep + os.environ["PATH"],
                                     KERNEL_RUN_TEMP=temporary, KERNEL_REMOTE_CACHE=endpoint,
                                     UPLOAD_KERNEL_CACHE=upload, CALLS=str(calls),
                                     BUILD_EXIT=str(exit_code)))
                        self.assertEqual(result.returncode, exit_code, result.stdout + result.stderr)
                        argv = json.loads(calls.read_text())
                        self.assertEqual(argv[argv.index("--remote-cache") + 1], endpoint)
                        self.assertEqual("--upload-local-results" in argv, upload == "true")
                        self.assertEqual(argv[argv.index("--architecture") + 1], "arm64")
                        self.assertEqual(
                            argv[argv.index("--state-dir") + 1],
                            str(root / "kernel-bundle-state"),
                        )


if __name__ == "__main__":
    unittest.main()
