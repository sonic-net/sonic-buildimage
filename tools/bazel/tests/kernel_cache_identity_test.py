#!/usr/bin/env python3
"""Exercise kernel handoff invalidation independently of Bazel and cache I/O."""

import base64
import copy
from contextlib import ExitStack
import hashlib
import json
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.kernel import cache_identity as identity


def release_fixture():
    """Small real-shaped release indexes, checked by production fact validation."""
    facts_api = identity.cache_facts
    python_url = facts_api.PYTHON_RELEASES + "/20241002/SHA256SUMS"
    python_data = ("a" * 64 + "  cpython-fixture.tar.gz\n").encode()
    python = {"release_index_20241002_" + facts_api.PYTHON_RELEASES: {
        "3.13/x86_64-unknown-linux-gnu/install_only": {
            "filename": "cpython-fixture.tar.gz", "full_version": "3.13.0", "sha256": "a" * 64}}}
    go_file = {"os": "linux", "arch": "amd64", "filename": "go1.26.7.linux-amd64.tar.gz",
               "sha256": "b" * 64, "kind": "archive"}
    facts = {facts_api.PYTHON_EXTENSION: python, facts_api.GO_EXTENSION: {
        facts_api.GO_VERSION: {"linux_amd64": [go_file["filename"], go_file["sha256"]]}}}
    files = {python_url: python_data, facts_api.GO_RELEASES: json.dumps([
        {"version": "go" + facts_api.GO_VERSION, "files": [go_file]}]).encode()}
    return facts, files, {python_url: hashlib.sha256(python_data).hexdigest()}


def resolution_fixture(registry, source):
    """Return original lock bytes and URL->bytes/None for integration tests.

    patchelf is an actual transitive kernel dependency. Its metadata must be
    revalidated even when both direct source pins and the request key match.
    """
    files = {}
    integrity = "sha256-" + base64.b64encode(b"a" * 32).decode()
    for name, version in source["modules"].items():
        commit = source["kernel_gitlink"] if name == "sonic-linux-kernel" else version.rsplit("-", 1)[1]
        prefix = registry + "/modules/" + name + "/" + version + "/"
        files[prefix + "MODULE.bazel"] = f'module(name="{name}", version="{version}")\n'.encode()
        files[prefix + "source.json"] = json.dumps({
            "url": f"https://github.com/securely1g/{name}/archive/{commit}.tar.gz",
            "strip_prefix": name + "-" + commit, "integrity": integrity,
        }).encode()
    prefix = identity.BCR + "/modules/patchelf/0.18.0.bcr.1/"
    files[prefix + "MODULE.bazel"] = b'module(name="patchelf", version="0.18.0.bcr.1")\n'
    files[prefix + "source.json"] = json.dumps({"url": "https://github.com/NixOS/patchelf/archive/refs/tags/0.18.0.tar.gz",
                                               "integrity": integrity}).encode()
    files[registry + "/modules/patchelf/0.18.0.bcr.1/MODULE.bazel"] = None
    lock = {
        "lockFileVersion": 24,
        "registryFileHashes": {url: hashlib.sha256(data).hexdigest() if data is not None else "not found"
                               for url, data in files.items()},
        "selectedYankedVersions": {}, "facts": release_fixture()[0],
        "moduleExtensions": {
            "@@sonic-build-infra+//toolchains/sysroot:extensions.bzl%sysroots": {
                "general": {"bzlTransitiveDigest": base64.b64encode(b"b" * 32).decode(),
                            "usagesDigest": base64.b64encode(b"c" * 32).decode(),
                            "recordedFileInputs": {"@@sonic-build-infra+//apt/apt.lock.json": "d" * 64},
                            "recordedDirentsInputs": {}, "envVariables": {}, "generatedRepoSpecs": {}}}},
    }
    # metadata.json is queried independently: yanking can change after a build.
    for name in source["modules"]:
        files[registry + "/modules/" + name + "/metadata.json"] = b'{"yanked_versions":{}}'
    files[identity.BCR + "/modules/patchelf/metadata.json"] = b'{"yanked_versions":{}}'
    files.update(release_fixture()[1])
    return json.dumps(lock).encode(), files


def source_fixture():
    arch = "arm64" if platform.machine() == "aarch64" else "amd64"
    return {"source_commit": "a" * 40, "kernel_gitlink": "b" * 40,
            "consumer_inputs_sha256": "c" * 64, "source_tree_sha256": "d" * 64,
            "source_archives": [{"name": "linux.tar.xz", "sha256": "e" * 64}],
            "modules": {"sonic-linux-kernel": "0.0.2-" + "b" * 40,
                        "sonic-build-infra": "0.0.38-" + "f" * 40},
            "launcher": {"architecture": arch, "execution_strategy": "linux-sandbox", "bazel_version": "8.5.1",
                         "bazel_sha256": "f" * 64},
            "kernel_config": {"CONFIGURED_ARCH": arch, "CONFIGURED_PLATFORM": "vs", "BLDENV": "trixie",
                              "CROSS_BUILD_ENVIRON": "n", "MULTIARCH_QEMU_ENVIRON": "n",
                              "INCLUDE_EXTERNAL_PATCHES": "n", "SECURE_UPGRADE_MODE": "no_sign",
                              "ADDITIONAL_BUILD_PROFILES": "", "SECURE_UPGRADE_KERNEL_CAFILE": "",
                              "EXTERNAL_KERNEL_PATCH_URL": "", "EXTERNAL_KERNEL_PATCH_LOC": "",
                              "KERNEL_VERSION": "6.12.41"}}


def fixture_policy(source, lock_bytes):
    """Apply test-only trusted fingerprints; all validation code still runs."""
    lock = json.loads(lock_bytes)
    facts, _, checksums = release_fixture()
    stack = ExitStack()
    stack.enter_context(patch.multiple(identity,
        AUDITED_PRODUCERS=frozenset({(source["kernel_gitlink"], source["modules"]["sonic-build-infra"].rsplit("-", 1)[1])}),
        AUDITED_EXTENSIONS={name: identity._digest(contexts) for name, contexts in lock["moduleExtensions"].items()}))
    stack.enter_context(patch.multiple(identity.cache_facts,
        PYTHON_FACTS_SHA256=identity._digest(facts[identity.cache_facts.PYTHON_EXTENSION]), PYTHON_CHECKSUMS=checksums))
    return stack


class IsolatedKernelTemplateTest(unittest.TestCase):
    def test_shipped_legacy_template_selects_the_registered_gitlink(self):
        from tools.bazel.kernel import prepare
        _, declarations = prepare.module_declarations(ROOT / "tools/bazel/kernel/MODULE.bazel")
        modules = {name: spec["version"] for name, spec in declarations.items()}
        gitlink = prepare.git(ROOT, "rev-parse", "HEAD:src/sonic-linux-kernel")
        self.assertRegex(modules["sonic-linux-kernel"], r"^[0-9]+\.[0-9]+\.[0-9]+-" + gitlink + "$")
        # This source pair has no retained, audited legacy index. Its first
        # use must go through Bazel and verify the registry source metadata.
        with self.assertRaisesRegex(identity.CacheUnverifiable, "has not been audited"):
            identity._producer({"kernel_gitlink": gitlink, "modules": modules})
        source = ROOT / prepare.KERNEL_PATH
        if (source / "MODULE.bazel").is_file():
            self.assertEqual(prepare.consumer_modules(ROOT, source, gitlink), modules)


class KernelCacheIdentityTest(unittest.TestCase):
    def setUp(self):
        self.source = source_fixture()
        self.registry = identity.SONIC_REGISTRY + "main"
        self.lock, self.files = resolution_fixture(self.registry, self.source)
        policy = fixture_policy(self.source, self.lock)
        self.addCleanup(policy.close)

    def validate(self, lock=None, files=None, **kwargs):
        identity.validate_resolution(lock or self.lock, self.registry, source=self.source,
                                     fetch=(self.files if files is None else files).__getitem__, **kwargs)

    def test_accepts_pinned_external_extension_inputs(self):
        self.validate(environment={})

    def test_changed_transitive_metadata_invalidates_without_changed_direct_pins(self):
        changed = dict(self.files)
        changed[identity.BCR + "/modules/patchelf/0.18.0.bcr.1/source.json"] += b"\n"
        with self.assertRaisesRegex(identity.CacheUnverifiable, "registry input changed"):
            self.validate(files=changed)

    def test_changed_registry_precedence_invalidates(self):
        changed = dict(self.files)
        missing = next(url for url, data in changed.items() if data is None)
        changed[missing] = b"new preferred registry module"
        with self.assertRaisesRegex(identity.CacheUnverifiable, "previously missing"):
            self.validate(files=changed)

    def test_newly_yanked_transitive_dependency_falls_back(self):
        changed = dict(self.files)
        changed[identity.BCR + "/modules/patchelf/metadata.json"] = b'{"yanked_versions":{"0.18.0.bcr.1":"regression"}}'
        with self.assertRaisesRegex(identity.CacheUnverifiable, "currently yanked"):
            self.validate(files=changed)

    def test_strong_registry_integrity_algorithms(self):
        for algorithm, size in (("sha384", 48), ("sha512", 64)):
            with self.subTest(algorithm=algorithm):
                changed = dict(self.files)
                url = identity.BCR + "/modules/patchelf/0.18.0.bcr.1/source.json"
                metadata = json.loads(changed[url])
                metadata["integrity"] = algorithm + "-" + base64.b64encode(b"a" * size).decode()
                changed[url] = json.dumps(metadata).encode()
                lock = json.loads(self.lock)
                lock["registryFileHashes"][url] = hashlib.sha256(changed[url]).hexdigest()
                self.validate(json.dumps(lock).encode(), files=changed)

    def test_extension_environment_must_match_including_absent_values(self):
        lock = json.loads(self.lock)
        record = next(iter(lock["moduleExtensions"].values()))["general"]
        record["envVariables"] = {"KERNEL_TEST_SETTING": None}
        with fixture_policy(self.source, json.dumps(lock).encode()):
            self.validate(json.dumps(lock).encode(), environment={})
        with self.assertRaisesRegex(identity.CacheUnverifiable, "environment changed"):
            self.validate(json.dumps(lock).encode(), environment={"KERNEL_TEST_SETTING": ""})

    def test_local_or_generated_extension_inputs_fall_back(self):
        for path in ("/tmp/input", "@@//input", "@@generated++ext+repo//input", "@@sonic-build-infra+//../escape"):
            with self.subTest(path=path):
                lock = json.loads(self.lock)
                record = next(iter(lock["moduleExtensions"].values()))["general"]
                record["recordedFileInputs"] = {path: "a" * 64}
                with self.assertRaises(identity.CacheUnverifiable):
                    self.validate(json.dumps(lock).encode())

    def test_missing_module_source_cannot_bind_extension_input(self):
        lock = json.loads(self.lock)
        record = next(iter(lock["moduleExtensions"].values()))["general"]
        record["recordedFileInputs"] = {"@@unknown+//file": "a" * 64}
        with self.assertRaisesRegex(identity.CacheUnverifiable, "not bound"):
            self.validate(json.dumps(lock).encode())

    def test_unselected_registry_is_rejected_before_network(self):
        lock = json.loads(self.lock)
        lock["registryFileHashes"]["https://example.org/private"] = "a" * 64
        with self.assertRaisesRegex(identity.CacheUnverifiable, "unselected registry"):
            self.validate(json.dumps(lock).encode())

    def test_unknown_lock_fields_fail_closed(self):
        lock = json.loads(self.lock)
        lock["newUnboundInputs"] = {}
        with self.assertRaisesRegex(identity.CacheUnverifiable, "unknown kernel resolution"):
            self.validate(json.dumps(lock).encode())

    def test_unknown_extension_implementation_or_generated_input_falls_back(self):
        for field, value in (("bzlTransitiveDigest", "changed"), ("generatedRepoSpecs", {"new": {"url": "https://example.org/latest"}})):
            lock = json.loads(self.lock)
            record = next(iter(lock["moduleExtensions"].values()))["general"]
            record[field] = value
            with self.assertRaisesRegex(identity.CacheUnverifiable, "implementation, usage or repository spec changed"):
                self.validate(json.dumps(lock).encode())

    def test_missing_facts_and_changed_release_indexes_fall_back(self):
        lock = json.loads(self.lock)
        lock["facts"] = {}
        with self.assertRaisesRegex(identity.CacheUnverifiable, "facts cannot be revalidated"):
            self.validate(json.dumps(lock).encode())
        changed = dict(self.files)
        changed[next(iter(identity.cache_facts.PYTHON_CHECKSUMS))] = b"replacement release index"
        with self.assertRaisesRegex(identity.CacheUnverifiable, "release checksums changed"):
            self.validate(files=changed)

    def test_validation_has_overall_deadline(self):
        release = threading.Event()
        def blocked(url):
            release.wait(1)
            return self.files[url]
        start = time.monotonic()
        try:
            with self.assertRaisesRegex(identity.CacheUnverifiable, "time budget"):
                identity.validate_resolution(self.lock, self.registry, source=self.source,
                                             fetch=blocked, timeout=0.01)
            self.assertLess(time.monotonic() - start, 0.5)
        finally:
            release.set()

    def test_request_identity_preserves_unrelated_commit_reuse_and_rejects_unbound_options(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            template = root / "tools/bazel/kernel"
            template.mkdir(parents=True)
            shutil.copyfile(ROOT / "tools/bazel/kernel/.bazelrc", template / ".bazelrc")
            (template / "MODULE.bazel").write_text('module(name="sonic-kernel-cache-consumer")\n' + "\n".join(
                f'bazel_dep(name="{name}", version="{version}", repo_name="{name.replace(chr(45), chr(95))}")'
                for name, version in self.source["modules"].items()))
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            subprocess.run(["git", "-C", str(root), "add", "tools"], check=True)
            environment = {"SONIC_HOSTBASE_BUILDER_ID": "sha256:" + "a" * 64}
            key = identity.request_key(self.source, self.registry, workspace=root, environment=environment)
            changed = copy.deepcopy(self.source)
            changed["source_commit"] = "e" * 40
            self.assertEqual(key, identity.request_key(changed, self.registry, workspace=root, environment=environment))
            changed["kernel_config"]["KERNEL_VERSION"] = "6.12.42"
            self.assertNotEqual(key, identity.request_key(changed, self.registry, workspace=root, environment=environment))
            with self.assertRaisesRegex(identity.CacheUnverifiable, "immutable slave"):
                identity.request_key(self.source, self.registry, workspace=root, environment={})
            changed_builder = {"SONIC_HOSTBASE_BUILDER_ID": "sha256:" + "b" * 64}
            self.assertNotEqual(key, identity.request_key(self.source, self.registry, workspace=root, environment=changed_builder))
            for settings in ({"SONIC_BAZEL_REMOTE_ONLY_RC": "/tmp/external.rc"}, {"JAVA_TOOL_OPTIONS": "-javaagent:unknown"}):
                with self.assertRaises(identity.CacheUnverifiable):
                    identity.request_key(self.source, self.registry, workspace=root, environment={**environment, **settings})
            with (template / ".bazelrc").open("a") as out:
                out.write("\nbuild --override_repository=dep=/tmp/dep\n")
            with self.assertRaisesRegex(identity.CacheUnverifiable, "unbound Bazel option"):
                identity.request_key(self.source, self.registry, workspace=root, environment=environment)
            shutil.copyfile(ROOT / "tools/bazel/kernel/.bazelrc", template / ".bazelrc")
            with (template / "MODULE.bazel").open("a") as out:
                out.write('\nlocal_path_override(module_name="patchelf", path="/tmp/local")\n')
            with self.assertRaises(identity.CacheUnverifiable):
                identity.request_key(self.source, self.registry, workspace=root, environment=environment)


if __name__ == "__main__":
    unittest.main()
