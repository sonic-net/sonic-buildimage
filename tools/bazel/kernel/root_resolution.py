"""Revalidate the audited root-module kernel closure without starting Bazel.

The request key must first bind the maintained root declarations, local modules,
configuration and worker. This validator additionally freezes the complete
registry resolution observed by a fresh kernel-only analysis, including negative
lookups. A changed transitive source cannot authorize a new extension merely by
seeding a new cache entry. New graphs take the normal Bazel path until reviewed.
"""

import hashlib
import json
import os
from pathlib import Path
import sys
import time

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from tools.bazel.kernel import cache_facts, cache_identity as identity, root_request


RootResolutionUnverifiable = identity.CacheUnverifiable
REGISTRY = identity.SONIC_REGISTRY + "main"
REGISTRY_MEMBERSHIP = Path(__file__).with_name("root_resolution_registry.json")
REGISTRY_WORKERS = 32
# Fresh Bazel 8.5.1 AMD64 analysis of the image root with kernel 0.0.3 and
# sonic-build-infra 0.0.42. Membership uses m=MODULE.bazel and
# s=source.json. Its canonical projected registry hash also binds every value.
AUDITED_REGISTRY_SHA256 = "2289b7fa5d6fae85fbb7c31f9f7b9c3e651528955a4160f16fff642b7792c3ce"
AUDITED_EXTENSIONS = {
    "@@aspect_rules_py+//py:extensions.bzl%py_tools": "6f73de4c9958470ef2c2ba4a7c4f979026f10ac7333cb127d8cb7804351f1ab9",
    "@@aspect_tools_telemetry+//:extension.bzl%telemetry": "ef51cb53d67c2a9057ab718a8604bfee2c583ad0be7e6d862fba6ba4ce75fcc7",
    "@@rules_kotlin+//src/main/starlark/core/repositories:bzlmod_setup.bzl%rules_kotlin_extensions": "9a50add992dbd3f4a283a11e5175b5a4ea4da651f820c1842b55cdc30eb100d2",
    "@@rules_oci+//oci:extensions.bzl%oci": "628ab08b4720c7432fb12b058399d50a58f2d2d5a2750d14cfd14b167afc6595",
    "@@rules_python+//python/uv:uv.bzl%uv": "cc3d4bb5ccfcc4d4b468a41ae817e35321ff9ee9a856c3c06d8ee19e7c8b1f57",
    "@@sonic-build-infra+//toolchains/gcc:extensions.bzl%gcc": "551059deec3d23419894eb1d075b83f4fd3505cb7c240fa9c542f63db304f450",
    "@@sonic-build-infra+//toolchains/sysroot:extensions.bzl%sysroots": "1ac9c37ddcdbe6d319c0f0336b1c1c7d3489f0a56f584ffb51179e232bd19bca",
    "@@sonic-linux-kernel+//tools/bazel:sources.bzl%kernel_sources": "56ddebdbd8bdc6f59c751d39d2534489023bbe0e20dce831fa6b87165cfdd206",
    "@@yq.bzl+//yq:extensions.bzl%yq": "341ae1a897ee9ae38e0b766b7551eb8704e40bdcbc0273d3e942dd7a59baf547",
}


def _registry_urls():
    membership = identity._json(REGISTRY_MEMBERSHIP.read_bytes())
    identity.require(isinstance(membership, dict) and set(membership) == {"sonic", "bcr"},
                     "invalid audited root registry membership")
    urls = set()
    for name, origin in (("sonic", REGISTRY), ("bcr", identity.BCR)):
        entry = membership[name]
        identity.require(isinstance(entry, dict) and set(entry) == {"registry", "modules"}
                         and isinstance(entry["registry"], bool) and isinstance(entry["modules"], dict),
                         "invalid audited registry membership")
        if entry["registry"]:
            urls.add(origin + "/bazel_registry.json")
        for module, versions in entry["modules"].items():
            identity.require(isinstance(versions, dict), "invalid audited module membership")
            for version, files in versions.items():
                identity.require(files in {"m", "s", "ms"}, "invalid audited registry file membership")
                for flag in files:
                    path = "modules/" + module + "/" + version + "/" + {"m": "MODULE.bazel", "s": "source.json"}[flag]
                    url = origin + "/" + path
                    identity._registry_path(url, REGISTRY)
                    urls.add(url)
    return urls


def _lock(data):
    identity.require(isinstance(data, bytes) and 0 < len(data) <= identity.MAX_LOCK_BYTES,
                     "invalid root kernel resolution lock size")
    lock = identity._json(data)
    identity.require(isinstance(lock, dict) and lock.get("lockFileVersion") == 24
                     and set(lock) == {"lockFileVersion", "registryFileHashes", "selectedYankedVersions",
                                      "moduleExtensions", "facts"}
                     and all(isinstance(lock[key], dict) for key in lock if key != "lockFileVersion"),
                     "unknown root kernel resolution lock format")
    identity.require(not lock["selectedYankedVersions"], "root lock selects a yanked dependency")
    return lock


def _projection(lock):
    urls = _registry_urls()
    identity.require(urls <= lock["registryFileHashes"].keys(), "root lock omitted an audited registry input")
    hashes = {url: lock["registryFileHashes"][url] for url in urls}
    identity.require(identity._digest(hashes) == AUDITED_REGISTRY_SHA256,
                     "root kernel registry closure has not been audited")
    extensions = lock["moduleExtensions"]
    identity.require(AUDITED_EXTENSIONS.keys() <= extensions.keys(), "root lock omitted an audited extension")
    for name, digest in AUDITED_EXTENSIONS.items():
        identity.require(identity._digest(extensions[name]) == digest,
                         "root kernel extension implementation, usage or repository spec changed: " + name)
    # A shared root lock also retains facts/extensions from other targets. They
    # are not inputs of this exact kernel graph: the authenticated, fixed root
    # declarations AND complete fixed registry closure establish that boundary.
    owners = (cache_facts.PYTHON_EXTENSION, cache_facts.GO_EXTENSION)
    identity.require(all(owner in lock["facts"] for owner in owners), "root lock omitted kernel release facts")
    facts = {owner: lock["facts"][owner] for owner in owners}
    return hashes, {name: extensions[name] for name in AUDITED_EXTENSIONS}, facts


def _existing_lock(workspace, expected):
    path = workspace / "MODULE.bazel.lock"
    if not path.exists() and not path.is_symlink():
        return
    identity.require(not path.is_symlink() and path.resolve(strict=True) == path
                     and path.stat().st_size <= identity.MAX_LOCK_BYTES,
                     "root workspace lock is redirected or oversized")
    current = _lock(path.read_bytes())
    for field, selected in zip(("registryFileHashes", "moduleExtensions", "facts"), expected):
        for key, value in selected.items():
            identity.require(key not in current[field] or current[field][key] == value,
                             "root workspace lock conflicts with the kernel candidate: " + key)


def _file_input(path, digest, workspace, local_modules, modules):
    identity.require(isinstance(path, str) and isinstance(digest, str) and identity.HEX.fullmatch(digest),
                     "invalid root extension file input")
    if path.startswith("@@//"):
        directory, relative = workspace, path[4:]
    else:
        match = identity.MODULE_INPUT.fullmatch(path)
        identity.require(match is not None and identity._relative(match[2]),
                         "root extension reads an unbound file")
        directory, relative = local_modules.get(match[1]), match[2]
        if directory is None:
            identity.require(len(modules.get(match[1], [])) == 1,
                             "root extension input is not bound to one source archive")
            return
    identity.require(identity._relative(relative), "unsafe root extension file input")
    file = directory / relative
    identity.require(file.is_file() and not file.is_symlink() and file.resolve(strict=True) == file
                     and hashlib.sha256(file.read_bytes()).hexdigest() == digest,
                     "root extension workspace file changed: " + path)


def _extensions(extensions, workspace, modules, environment):
    local = root_request.local_modules(workspace)
    for name, contexts in extensions.items():
        owner = identity.MODULE_INPUT.fullmatch(name.split("%", 1)[0])
        identity.require(owner is not None and len(modules.get(owner[1], [])) == 1,
                         "root extension implementation is not bound to one source archive")
        for record in contexts.values():
            identity.require(not record["recordedDirentsInputs"], "unbound root extension directory probe")
            for name, value in record["envVariables"].items():
                identity.require(environment.get(name) == value, "root extension environment changed: " + name)
            for path, digest in record["recordedFileInputs"].items():
                _file_input(path, digest, workspace, local, modules)


def validate(lock_bytes, workspace, *, fetch=None, environment=None,
             timeout=identity.REGISTRY_TIMEOUT_SECONDS):
    """Check a full authenticated root lock before publishing or restoring DEBs.

Every lookup in the audited root graph, including a preferred-registry 404,
must still match. Current yanking metadata and release-index facts are checked
independently. Unknown graphs, local lock conflicts and network failures miss;
the caller then uses the ordinary root Bazel build.
    """
    deadline = time.monotonic() + timeout
    workspace = Path(workspace).resolve(strict=True)
    environment = os.environ if environment is None else environment
    # Also enforce these guards when called directly by the publisher. The
    # request-key caller additionally validates all tracked files/execution.
    for filename in root_request.MODULE_FILES:
        root_request._module(workspace, filename)
    versions = root_request.declared_kernel_modules(workspace)
    projected = _projection(_lock(lock_bytes))
    _existing_lock(workspace, projected)
    fetch = fetch or identity._fetch
    hashes, extensions, facts = projected
    inputs = identity._registry_inputs(hashes, REGISTRY, fetch, deadline - time.monotonic(), workers=REGISTRY_WORKERS)
    modules = identity._module_sources(inputs, REGISTRY, {
        "modules": versions, "kernel_gitlink": versions["sonic-linux-kernel"].rsplit("-", 1)[1]})
    identity._yanked_versions(modules, REGISTRY, fetch, deadline - time.monotonic(), workers=REGISTRY_WORKERS)
    _extensions(extensions, workspace, modules, environment)
    try:
        cache_facts.validate_root(facts, lambda urls: identity._fetch_many(urls, fetch, deadline - time.monotonic(),
                                                                        workers=REGISTRY_WORKERS))
    except (OSError, ValueError, TypeError) as error:
        raise RootResolutionUnverifiable("root kernel release facts cannot be revalidated: " + str(error)) from error


def audit_registry(lock_bytes):
    """Format review data from a fresh root kernel-only analysis, not a busy lock.

    Regenerate membership with `python3 tools/bazel/kernel/root_resolution.py
    /path/to/fresh/MODULE.bazel.lock > tools/bazel/kernel/root_resolution_registry.json`.
    The registry and extension fingerprints printed on stderr still require a
    producer/input audit before updating constants. This never authorizes a graph.
    """
    lock = _lock(lock_bytes)
    origins = {"sonic": REGISTRY, "bcr": identity.BCR}
    entries = {name: {"registry": False, "modules": {}} for name in origins}
    for url in lock["registryFileHashes"]:
        origin, path = identity._registry_path(url, REGISTRY)
        entry = entries[next(name for name, base in origins.items() if base == origin)]
        if path == "bazel_registry.json":
            entry["registry"] = True
        else:
            _, name, version, filename = path.split("/")
            versions = entry["modules"].setdefault(name, {})
            versions[version] = ''.join(sorted(versions.get(version, "") + {"MODULE.bazel": "m", "source.json": "s"}[filename]))
    lines = ["{"]
    for index, (name, entry) in enumerate(sorted(entries.items())):
        lines.append('  ' + json.dumps(name) + ': {"registry": ' + json.dumps(entry["registry"]) + ', "modules": {')
        modules = sorted(entry["modules"].items())
        for number, (module, versions) in enumerate(modules):
            lines.append('    ' + json.dumps(module) + ': ' + json.dumps(versions, sort_keys=True)
                         + (',' if number < len(modules) - 1 else ''))
        lines.append('  }}' + (',' if index < len(entries) - 1 else ''))
    return '\n'.join([*lines, '}']) + '\n'


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Generate review data from a fresh root kernel-only lock")
    parser.add_argument("lock", type=Path)
    args = parser.parse_args()
    data = args.lock.read_bytes()
    print(audit_registry(data), end="")
    lock = _lock(data)
    print("AUDITED_REGISTRY_SHA256 =", repr(identity._digest(lock["registryFileHashes"])), file=sys.stderr)
    print("AUDITED_EXTENSIONS =", repr({name: identity._digest(contexts)
          for name, contexts in lock["moduleExtensions"].items()}), file=sys.stderr)
