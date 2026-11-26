"""Regression coverage for live verification of the root kernel dependency graph."""

from contextlib import ExitStack
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import threading
import time
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.kernel import cache_facts, cache_identity, root_request, root_resolution
from tools.bazel.tests.kernel_cache_identity_test import release_fixture, resolution_fixture as isolated_fixture


def resolution_fixture(workspace):
    """Return a miniature full lock and HTTPS URL->bytes/None fixture.

    A new workspace receives the actual producer versions and the maintained
    local overrides. Existing integration workspaces keep their declarations.
    The BCR patchelf source is a real transitive dependency, not a direct pin.
    """
    workspace = Path(workspace)
    workspace.mkdir(parents=True, exist_ok=True)
    module = workspace / "MODULE.bazel"
    if not module.exists():
        module.write_text(
            'module(name="root_kernel_fixture")\n'
            f'bazel_dep(name="sonic-linux-kernel", version="{root_request.KERNEL}")\n'
            f'bazel_dep(name="sonic-build-infra", version="{root_request.INFRA}")\n' +
            ''.join(f'local_path_override(module_name="{name}", path="{path}")\n'
                    for name, path in sorted(root_request.LOCAL_MODULES.items())))
    for path in root_request.LOCAL_MODULES.values():
        (workspace / path).mkdir(parents=True, exist_ok=True)
    return isolated_fixture(root_resolution.REGISTRY, {
        "kernel_gitlink": root_request.KERNEL.rsplit("-", 1)[1],
        "modules": {"sonic-linux-kernel": root_request.KERNEL, "sonic-build-infra": root_request.INFRA}})


def _membership(hashes):
    result = {}
    for name, origin in (("sonic", root_resolution.REGISTRY), ("bcr", cache_identity.BCR)):
        modules = {}
        for url in hashes:
            if not url.startswith(origin + "/modules/"):
                continue
            _, module, version, filename = url[len(origin) + 1:].split("/")
            versions = modules.setdefault(module, {})
            versions[version] = ''.join(sorted(versions.get(version, "") + {"MODULE.bazel": "m", "source.json": "s"}[filename]))
        result[name] = {"registry": origin + "/bazel_registry.json" in hashes, "modules": modules}
    return result


def fixture_policy(workspace, lock_bytes):
    """Authorize only miniature audit constants; all production checks run."""
    workspace = Path(workspace)
    lock = json.loads(lock_bytes)
    facts, _, checksums = release_fixture()
    stack = ExitStack()
    directory = Path(stack.enter_context(tempfile.TemporaryDirectory()))
    membership = directory / "registry.json"
    membership.write_text(json.dumps(_membership(lock["registryFileHashes"])))
    stack.enter_context(patch.multiple(root_resolution,
        REGISTRY_MEMBERSHIP=membership,
        AUDITED_REGISTRY_SHA256=cache_identity._digest(lock["registryFileHashes"]),
        AUDITED_EXTENSIONS={name: cache_identity._digest(contexts) for name, contexts in lock["moduleExtensions"].items()}))
    stack.enter_context(patch.object(root_request, "MODULE_FILES", {
        name: hashlib.sha256((workspace / name).read_bytes()).hexdigest()
        for name in root_request.MODULE_FILES if (workspace / name).is_file()}))
    stack.enter_context(patch.multiple(cache_facts,
        ROOT_GO_VERSIONS={cache_facts.GO_VERSION},
        PYTHON_FACTS_SHA256=cache_identity._digest(facts[cache_facts.PYTHON_EXTENSION]),
        PYTHON_CHECKSUMS=checksums))
    return stack


class KernelRootResolutionTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.workspace = Path(directory.name)
        self.lock, self.responses = resolution_fixture(self.workspace)
        self.policy = fixture_policy(self.workspace, self.lock)
        self.addCleanup(self.policy.close)

    def validate(self, data=None, **kwargs):
        root_resolution.validate(data or self.lock, self.workspace,
                                 fetch=self.responses.__getitem__, environment={}, **kwargs)

    def test_current_full_graph_and_release_indexes_pass(self):
        self.validate()

    def test_live_transitive_registry_change_invalidates_same_direct_pins(self):
        url = cache_identity.BCR + "/modules/patchelf/0.18.0.bcr.1/source.json"
        self.responses[url] += b"\n"
        with self.assertRaisesRegex(ValueError, "registry input changed"):
            self.validate()

    def test_updated_transitive_graph_cannot_reseed_without_audit(self):
        lock = json.loads(self.lock)
        url = cache_identity.BCR + "/modules/patchelf/0.18.0.bcr.1/source.json"
        self.responses[url] += b"\n"
        lock["registryFileHashes"][url] = hashlib.sha256(self.responses[url]).hexdigest()
        with self.assertRaisesRegex(ValueError, "closure has not been audited"):
            self.validate(json.dumps(lock).encode())

    def test_missing_registry_input_or_extension_falls_back(self):
        for field in ("registryFileHashes", "moduleExtensions"):
            lock = json.loads(self.lock)
            lock[field].pop(next(iter(lock[field])))
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "omitted"):
                self.validate(json.dumps(lock).encode())

    def test_preferred_registry_negative_and_current_yanking_are_checked(self):
        missing = next(url for url, data in self.responses.items() if data is None)
        self.responses[missing] = b"new preferred registry module"
        with self.assertRaisesRegex(ValueError, "previously missing"):
            self.validate()
        self.responses[missing] = None
        self.responses[cache_identity.BCR + "/modules/patchelf/metadata.json"] = b'{"yanked_versions":{"0.18.0.bcr.1":"broken"}}'
        with self.assertRaisesRegex(ValueError, "currently yanked"):
            self.validate()

    def test_busy_root_unrelated_records_are_not_kernel_inputs(self):
        lock = json.loads(self.lock)
        lock["facts"]["@@rules_rs+//rs:extensions.bzl%crate"] = {"unrelated": "retained fact"}
        lock["moduleExtensions"]["@@rules_rs+//rs:extensions.bzl%crate"] = {"unrelated": "retained extension"}
        lock["registryFileHashes"][cache_identity.BCR + "/modules/unrelated/1.0/MODULE.bazel"] = "a" * 64
        data = json.dumps(lock).encode()
        (self.workspace / "MODULE.bazel.lock").write_bytes(data)
        self.validate(data)

    def test_existing_lock_conflicts_are_rejected_before_network(self):
        for field in ("registryFileHashes", "moduleExtensions", "facts"):
            lock = json.loads(self.lock)
            key = next(iter(lock[field]))
            lock[field][key] = "conflicting old record"
            (self.workspace / "MODULE.bazel.lock").write_text(json.dumps(lock))
            with self.subTest(field=field), patch.object(cache_identity, "_fetch_many") as fetch:
                with self.assertRaisesRegex(ValueError, "workspace lock conflicts"):
                    self.validate()
                fetch.assert_not_called()

    def test_empty_existing_lock_records_can_be_filled_from_verified_closure(self):
        lock = json.loads(self.lock)
        for field in ("registryFileHashes", "moduleExtensions", "facts"):
            lock[field] = {}
        (self.workspace / "MODULE.bazel.lock").write_text(json.dumps(lock))
        self.validate()

    def test_changed_root_or_generated_repository_specs_fail_closed(self):
        module = self.workspace / "MODULE.bazel"
        original = module.read_text()
        module.write_text(original + 'local_path_override(module_name="surprise", path="/tmp/unbound")\n')
        with self.assertRaisesRegex(ValueError, "declaration needs input review"):
            self.validate()
        module.write_text(original)
        lock = json.loads(self.lock)
        next(iter(lock["moduleExtensions"].values()))["general"]["generatedRepoSpecs"] = {"surprise": {"url": "https://example.com/latest"}}
        with self.assertRaisesRegex(ValueError, "implementation, usage or repository spec changed"):
            self.validate(json.dumps(lock).encode())

    def test_known_local_file_inputs_are_rechecked(self):
        for label, file in (("@@//input.json", self.workspace / "input.json"),
                            ("@@sonic-sysmgr+//input.json", self.workspace / "src/sonic-sysmgr/input.json")):
            file.write_bytes(b"source-owned input")
            lock = json.loads(self.lock)
            next(iter(lock["moduleExtensions"].values()))["general"]["recordedFileInputs"] = {
                label: hashlib.sha256(file.read_bytes()).hexdigest()}
            data = json.dumps(lock).encode()
            with self.subTest(label=label), fixture_policy(self.workspace, data):
                self.validate(data)
                file.write_bytes(b"changed")
                with self.assertRaisesRegex(ValueError, "workspace file changed"):
                    self.validate(data)

    def test_missing_or_unknown_external_release_facts_fall_back(self):
        lock = json.loads(self.lock)
        lock["facts"] = {}
        with self.assertRaisesRegex(ValueError, "omitted kernel release facts"):
            self.validate(json.dumps(lock).encode())
        self.responses[cache_facts.GO_RELEASES] = b"[]"
        with self.assertRaisesRegex(ValueError, "release facts cannot be revalidated"):
            self.validate()

    def test_deadline_bounds_network_work(self):
        release = threading.Event()
        def fetch(url):
            release.wait(1)
            return self.responses[url]
        started = time.monotonic()
        try:
            with self.assertRaisesRegex(ValueError, "time budget"):
                root_resolution.validate(self.lock, self.workspace, fetch=fetch, environment={}, timeout=0.03)
        finally:
            release.set()
        self.assertLess(time.monotonic() - started, 0.5)


if __name__ == "__main__":
    unittest.main()
