"""Validate the inputs of a trusted kernel handoff before starting Bazel.

The request key locates a candidate; it is not a Bazel action digest. A candidate
is usable only after its original consumer lock has been authenticated by the
caller and validate_resolution has checked its registry and extension inputs.
Unverifiable requests fall back to the existing Bazel producer.
"""

from concurrent.futures import FIRST_EXCEPTION, ThreadPoolExecutor, wait
import ast
from functools import lru_cache
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shlex
import ssl
import subprocess
import time
import urllib.error
import urllib.parse
import urllib.request

from tools.bazel.kernel import cache_facts


SONIC_REGISTRY = "https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/"
BCR = "https://bcr.bazel.build"
MAX_LOCK_BYTES = 128 * 1024 * 1024
MAX_REGISTRY_FILE_BYTES = 4 * 1024 * 1024
MAX_REGISTRY_FILES = 1500
REGISTRY_TIMEOUT_SECONDS = 30
REGISTRY_WORKERS = 12
HEX = re.compile(r"[0-9a-f]{64}\Z")
MODULE_FILE = re.compile(r"modules/([a-z][a-z0-9._-]*)/([^/]+)/(MODULE\.bazel|source\.json)\Z")
MODULE_INPUT = re.compile(r"@@([a-z][a-z0-9._-]*)\+//(.+)\Z")
# Audited producer contract from CI run 44833295432. The extension fingerprints
# cover implementation/transitive .bzl digests, usages, recorded inputs and every
# generated repository spec. A new producer or changed extension takes the
# ordinary Bazel path until its input contract is reviewed; names alone do not
# authorize new custom repository behavior.
AUDITED_PRODUCERS = frozenset({("96c621843a2bc9dbbe17eb890c8c6e1cea5e834b", "0baa4acc9eac9ccc793b81104b1580d530271cbe")})
AUDITED_EXTENSIONS = {
    "@@aspect_rules_py+//py:extensions.bzl%py_tools": "6f73de4c9958470ef2c2ba4a7c4f979026f10ac7333cb127d8cb7804351f1ab9",
    "@@aspect_tools_telemetry+//:extension.bzl%telemetry": "ef51cb53d67c2a9057ab718a8604bfee2c583ad0be7e6d862fba6ba4ce75fcc7",
    "@@rules_distroless+//apt:extensions.bzl%apt": "bb736c1288c86ac6153aed0a6248aee07ba92471be5ca528a024f207987821ef",
    "@@rules_kotlin+//src/main/starlark/core/repositories:bzlmod_setup.bzl%rules_kotlin_extensions": "9a50add992dbd3f4a283a11e5175b5a4ea4da651f820c1842b55cdc30eb100d2",
    "@@rules_python+//python/uv:uv.bzl%uv": "cc3d4bb5ccfcc4d4b468a41ae817e35321ff9ee9a856c3c06d8ee19e7c8b1f57",
    "@@sonic-build-infra+//toolchains/gcc:extensions.bzl%gcc": "4e0dc416cde4f5adacc8c953f51158345e2fb52f25d3b9347437ed4a4aa1f2c5",
    "@@sonic-build-infra+//toolchains/sysroot:extensions.bzl%sysroots": "e05da0de974a3895242c98ed2e45249b7da1215eb1d83c74dea417be09141f4d",
    "@@sonic-linux-kernel+//tools/bazel:sources.bzl%kernel_sources": "e68820284835921fbc5fec97155edd0968f4d6174753df20a104ee08a7932fb7",
    "@@yq.bzl+//yq:extensions.bzl%yq": "6922f30406ae116bb8a501bd18ddf85a7287e1d778ae49a073c6114930ad2e8e",
}


class CacheUnverifiable(ValueError):
    """The normal Bazel path must decide whether these inputs are usable."""


def require(condition, message):
    if not condition:
        raise CacheUnverifiable(message)


def _json(data):
    def pairs(values):
        result = {}
        for key, value in values:
            require(key not in result, "duplicate JSON key in kernel cache metadata")
            result[key] = value
        return result
    try:
        return json.loads(data, object_pairs_hook=pairs)
    except (UnicodeError, ValueError, TypeError, RecursionError) as error:
        raise CacheUnverifiable("invalid kernel cache JSON") from error


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":"),
                                     allow_nan=False).encode()).hexdigest()


def _registry(registry):
    require(isinstance(registry, str) and registry.startswith(SONIC_REGISTRY),
            "kernel cache requires the selected SONiC registry")
    ref = registry[len(SONIC_REGISTRY):]
    require(bool(re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_./-]*", ref))
            and all(part not in {"", ".", ".."} for part in ref.split("/")),
            "invalid kernel cache registry ref")
    return registry


def _relative(path):
    parts = PurePosixPath(path).parts
    return bool(parts) and not path.startswith("/") and all(part not in {".", ".."} for part in parts) \
        and "\\" not in path and not any(ord(char) < 32 for char in path)


def _producer(source):
    try:
        pair = (source["kernel_gitlink"], source["modules"]["sonic-build-infra"].rsplit("-", 1)[1])
    except (KeyError, IndexError, TypeError, AttributeError) as error:
        raise CacheUnverifiable("invalid kernel producer identity") from error
    require(pair in AUDITED_PRODUCERS, "kernel producer pair has not been audited for direct cache reuse")


def _template(workspace, source):
    template = Path(workspace) / "tools/bazel/kernel"
    # The generated consumer is a copy of this directory. An ignored lock or
    # untracked RC must not silently become a resolution input.
    try:
        untracked = subprocess.check_output(
            ["git", "-C", str(workspace), "ls-files", "--others", "--", "tools/bazel/kernel"],
            text=True, timeout=10).splitlines()
        require(not any("__pycache__" not in PurePosixPath(name).parts for name in untracked),
                "kernel consumer template has untracked or ignored inputs")
        require(not any(path.is_symlink() for path in template.rglob("*")),
                "kernel consumer template contains redirected inputs")
        lines = [shlex.split(line, comments=True) for line in (template / ".bazelrc").read_text().splitlines()]
        declarations = ast.parse((template / "MODULE.bazel").read_text()).body
        require(len(declarations) == 3, "kernel consumer has additional module declarations")
        expected = [
            ("module", {"name": "sonic-kernel-cache-consumer"}),
            ("bazel_dep", {"name": "sonic-linux-kernel", "version": source["modules"]["sonic-linux-kernel"],
                           "repo_name": "sonic_linux_kernel"}),
            ("bazel_dep", {"name": "sonic-build-infra", "version": source["modules"]["sonic-build-infra"],
                           "repo_name": "sonic_build_infra"}),
        ]
        for declaration, (name, keywords) in zip(declarations, expected):
            require(isinstance(declaration, ast.Expr) and isinstance(declaration.value, ast.Call)
                    and isinstance(declaration.value.func, ast.Name), "unknown kernel module expression")
            call = declaration.value
            require(call.func.id == name and not call.args and len(call.keywords) == len(keywords)
                    and {item.arg: ast.literal_eval(item.value) for item in call.keywords} == keywords,
                    "kernel consumer has an override or unbound module input")
    except (OSError, subprocess.SubprocessError, ValueError, SyntaxError, KeyError, TypeError) as error:
        raise CacheUnverifiable("cannot verify the kernel consumer template") from error
    allowed = {
        ("common", "--registry=" + SONIC_REGISTRY + "main"),
        ("common", "--registry=" + BCR),
        ("common", "--lockfile_mode=update"),
        ("common", "--downloader_config=debian_mirrors.cfg"),
        ("build", "--host_features=fully_static_link"),
        ("build", "--remote_download_regex=(^|.*/)tools/build_tools/kernel_runtime_(amd64|arm64)(/.*)?$"),
    }
    actual = [tuple(line) for line in lines if line]
    require(set(actual) == allowed and len(actual) == len(allowed),
            "kernel consumer has an unbound Bazel option or import")


def request_key(source, registry, *, workspace, environment=None, extra_options=()):
    """Identify the audited native VS request; unknown execution options miss.

    source is the result of prepare.source_state(), which checks tracked input
    cleanliness. The original producer source_commit is deliberately excluded:
    unrelated buildimage commits may consume the same verified handoff.
    """
    environment = os.environ if environment is None else environment
    registry = _registry(registry)
    _producer(source)
    require(not extra_options, "kernel cache cannot bind additional launcher options")
    builder = environment.get("SONIC_HOSTBASE_BUILDER_ID", "")
    require(isinstance(builder, str) and re.fullmatch(r"sha256:[0-9a-f]{64}", builder),
            "kernel direct cache requires the immutable slave image identity")
    for name in ("SONIC_BAZEL_REMOTE_ONLY_RC", "BAZEL_OPTS", "BAZEL_FLAGS", "BAZELRC",
                 "BAZEL_STARTUP_OPTIONS", "BAZEL_BUILD_OPTS", "JAVA_TOOL_OPTIONS",
                 "_JAVA_OPTIONS", "JDK_JAVA_OPTIONS", "PYTHONHOME", "PYTHONPATH"):
        require(not environment.get(name), "unbound kernel execution setting: " + name)
    _template(workspace, source)
    try:
        launcher = source["launcher"]
        config = source["kernel_config"]
        arch = launcher["architecture"]
        require(platform.system() == "Linux" and
                platform.machine() == {"amd64": "x86_64", "arm64": "aarch64"}.get(arch),
                "kernel direct cache requires the native Linux host")
        require(launcher["execution_strategy"] == "linux-sandbox" and launcher["bazel_version"] == "8.5.1",
                "kernel direct cache requires the audited launcher contract")
        require(config["CONFIGURED_ARCH"] == arch and config["CONFIGURED_PLATFORM"] == "vs"
                and config["BLDENV"] == "trixie" and config["CROSS_BUILD_ENVIRON"] == "n"
                and config["MULTIARCH_QEMU_ENVIRON"] == "n" and config["INCLUDE_EXTERNAL_PATCHES"] == "n"
                and config["SECURE_UPGRADE_MODE"] == "no_sign" and not config["ADDITIONAL_BUILD_PROFILES"]
                and all(not config[name] for name in ("SECURE_UPGRADE_KERNEL_CAFILE", "EXTERNAL_KERNEL_PATCH_URL",
                                                       "EXTERNAL_KERNEL_PATCH_LOC")),
                "kernel direct cache requires native unsigned VS inputs")
        fields = ("consumer_inputs_sha256", "kernel_gitlink", "source_tree_sha256", "source_archives",
                  "kernel_config", "modules", "launcher")
        identity = {name: source[name] for name in fields}
        require(all(HEX.fullmatch(source[name]) for name in ("consumer_inputs_sha256", "source_tree_sha256"))
                and re.fullmatch(r"[0-9a-f]{40}", source["kernel_gitlink"]), "invalid kernel input identity")
    except (KeyError, TypeError) as error:
        raise CacheUnverifiable("incomplete kernel input identity") from error
    return _digest({"contract": "sonic-kernel-handoff-v1", "source": identity, "registry": registry,
                    "execution": {"os": "linux", "architecture": arch,
                                  "builder_image_id": builder,
                                  "github_actions": environment.get("GITHUB_ACTIONS") == "true"}})


class _NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, message, headers, newurl):
        if cache_facts.allow_redirect(request.full_url, newurl):
            return super().redirect_request(request, fp, code, message, headers, newurl)
        raise CacheUnverifiable("unexpected redirect fetching registry metadata")


@lru_cache(maxsize=1)
def _tls_context():
    # Load the system trust store once. Repeating this for every registry file
    # dominates large root graphs; SSLContext supports concurrent connections.
    return ssl.create_default_context()


def _fetch(url):
    try:
        opener = urllib.request.build_opener(_NoRedirect, urllib.request.HTTPSHandler(context=_tls_context()))
        with opener.open(url, timeout=5) as response:
            data = response.read(MAX_REGISTRY_FILE_BYTES + 1)
            require(len(data) <= MAX_REGISTRY_FILE_BYTES, "registry metadata exceeds size limit")
            return data
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return None
        raise CacheUnverifiable("registry metadata is unavailable") from error
    except (OSError, ValueError) as error:
        raise CacheUnverifiable("registry metadata is unavailable") from error


def _registry_path(url, registry):
    require(isinstance(url, str), "invalid registry URL in kernel lock")
    for origin in (registry, BCR):
        if url.startswith(origin + "/"):
            relative = url[len(origin) + 1:]
            require(_relative(relative) and not any(char in relative for char in "?#%"),
                    "invalid registry path in kernel lock")
            require(relative == "bazel_registry.json" or MODULE_FILE.fullmatch(relative),
                    "unknown registry input in kernel lock")
            return origin, relative
    raise CacheUnverifiable("kernel lock refers to an unselected registry")


def _fetch_many(urls, fetch, timeout, *, workers=REGISTRY_WORKERS):
    require(timeout > 0, "kernel registry validation exceeded its time budget")
    def check(url):
        data = fetch(url)
        require(data is None or isinstance(data, bytes) and len(data) <= MAX_REGISTRY_FILE_BYTES,
                "invalid registry response")
        return data
    pool = ThreadPoolExecutor(max_workers=workers)
    futures = {url: pool.submit(check, url) for url in urls}
    try:
        done, pending = wait(futures.values(), timeout=timeout, return_when=FIRST_EXCEPTION)
        for future in done:
            if future.exception() is not None:
                future.result()
        require(not pending, "kernel registry validation exceeded its time budget")
        return {url: future.result() for url, future in futures.items()}
    except (OSError, ValueError, TypeError) as error:
        raise CacheUnverifiable(str(error)) from error
    finally:
        # Individual network calls also have short timeouts. Do not block a
        # caller's fallback while queued requests drain after the total budget.
        pool.shutdown(wait=False, cancel_futures=True)


def _registry_inputs(hashes, registry, fetch, timeout, *, workers=REGISTRY_WORKERS):
    require(isinstance(hashes, dict) and 0 < len(hashes) <= MAX_REGISTRY_FILES,
            "invalid registry input set in kernel lock")
    for url, digest in hashes.items():
        _registry_path(url, registry)
        require(isinstance(digest, str) and (HEX.fullmatch(digest) or digest == "not found"),
                "invalid registry input digest")
    inputs = _fetch_many(hashes, fetch, timeout, workers=workers)
    for url, digest in hashes.items():
        data = inputs[url]
        if digest == "not found":
            require(data is None, "previously missing registry input now exists")
        else:
            require(data is not None and hashlib.sha256(data).hexdigest() == digest,
                    "registry input changed: " + url)
    return inputs


def _integrity(value):
    import base64
    require(isinstance(value, str) and value, "unbound registry archive integrity")
    try:
        for entry in value.split():
            algorithm, separator, digest = entry.partition("-")
            require(separator and algorithm in {"sha256", "sha384", "sha512"}, "unknown archive integrity algorithm")
            require(len(base64.b64decode(digest, validate=True)) == {"sha256": 32, "sha384": 48, "sha512": 64}[algorithm],
                    "invalid archive integrity")
    except ValueError as error:
        raise CacheUnverifiable("invalid archive integrity") from error


def _module_sources(inputs, registry, source):
    modules = {}
    for url, data in inputs.items():
        _, path = _registry_path(url, registry)
        match = MODULE_FILE.fullmatch(path)
        if data is None or match is None or match[3] != "source.json":
            continue
        metadata = _json(data)
        require(isinstance(metadata, dict) and metadata.get("type", "archive") == "archive",
                "kernel lock uses an unbound source repository type")
        _integrity(metadata.get("integrity"))
        require(isinstance(metadata.get("url"), str) and metadata["url"].startswith("https://"),
                "kernel lock uses a non-HTTPS source archive")
        for field in ("patches", "overlay"):
            entries = metadata.get(field, {})
            require(isinstance(entries, dict), "invalid registry source patches or overlay")
            for name, integrity in entries.items():
                require(isinstance(name, str) and _relative(name), "unsafe registry source patch or overlay")
                _integrity(integrity)
        modules.setdefault(match[1], []).append((match[2], metadata, url))
    for name, version in source["modules"].items():
        url = registry + "/modules/" + name + "/" + version + "/source.json"
        module_url = registry + "/modules/" + name + "/" + version + "/MODULE.bazel"
        require(inputs.get(url) is not None and inputs.get(module_url) is not None,
                "kernel lock omitted a direct producer module")
        metadata = _json(inputs[url])
        commit = source["kernel_gitlink"] if name == "sonic-linux-kernel" else version.rsplit("-", 1)[1]
        require(metadata.get("url") == "https://github.com/securely1g/" + name + "/archive/" + commit + ".tar.gz"
                and metadata.get("strip_prefix") == name + "-" + commit,
                "kernel lock selected a different source producer")
    return modules


def _yanked_versions(modules, registry, fetch, timeout, *, workers=REGISTRY_WORKERS):
    requests = {}
    for name, versions in modules.items():
        for version, _, source_url in versions:
            origin, _ = _registry_path(source_url, registry)
            requests.setdefault(origin + "/modules/" + name + "/metadata.json", set()).add(version)
    for url, data in _fetch_many(requests, fetch, timeout, workers=workers).items():
        if data is None:
            continue  # Bazel also accepts a registry without yanking metadata.
        metadata = _json(data)
        require(isinstance(metadata, dict) and isinstance(metadata.get("yanked_versions", {}), dict),
                "invalid current module yanking metadata")
        require(not requests[url].intersection(metadata.get("yanked_versions", {})),
                "kernel dependency is currently yanked: " + url)


def _extensions(extensions, modules, environment):
    require(isinstance(extensions, dict), "invalid kernel extension records")
    require(set(extensions) == set(AUDITED_EXTENSIONS), "kernel extension set has not been audited")
    allowed = {"bzlTransitiveDigest", "usagesDigest", "recordedFileInputs", "recordedDirentsInputs",
               "envVariables", "generatedRepoSpecs", "moduleExtensionMetadata", "recordedRepoMappingEntries"}
    for name, contexts in extensions.items():
        require(isinstance(name, str) and isinstance(contexts, dict), "invalid kernel extension identity")
        owner = MODULE_INPUT.fullmatch(name.split("%", 1)[0])
        require(owner is not None and len(modules.get(owner[1], [])) == 1,
                "kernel extension implementation is not bound to one source archive")
        for context, record in contexts.items():
            require(isinstance(context, str) and isinstance(record, dict) and set(record) <= allowed,
                    "unknown kernel extension inputs")
            require({"bzlTransitiveDigest", "usagesDigest", "recordedFileInputs", "recordedDirentsInputs",
                     "envVariables", "generatedRepoSpecs"} <= set(record), "incomplete kernel extension record")
            require(not record.get("recordedDirentsInputs"), "kernel extension probes an unbound directory")
            variables = record.get("envVariables", {})
            require(isinstance(variables, dict), "invalid kernel extension environment")
            for variable, value in variables.items():
                require(isinstance(variable, str) and (value is None or isinstance(value, str))
                        and environment.get(variable) == value, "kernel extension environment changed: " + str(variable))
            files = record.get("recordedFileInputs", {})
            require(isinstance(files, dict), "invalid kernel extension file inputs")
            for path, digest in files.items():
                match = MODULE_INPUT.fullmatch(path) if isinstance(path, str) else None
                require(match is not None and _relative(match[2]) and isinstance(digest, str) and HEX.fullmatch(digest),
                        "kernel extension reads an unbound local or generated file")
                require(len(modules.get(match[1], [])) == 1,
                        "kernel extension input is not bound to one verified source archive")
        require(_digest(contexts) == AUDITED_EXTENSIONS[name],
                "kernel extension implementation, usage or repository spec changed")


def validate_resolution(lock_bytes, registry, *, source, environment=None, fetch=None,
                        timeout=REGISTRY_TIMEOUT_SECONDS):
    """Revalidate an authenticated producer lock without invoking Bazel.

    Every recorded positive and negative registry lookup is checked, as is
    current module yanking metadata and the audited upstream release facts. External
    extension file digests are bound to their unchanged, integrity-pinned source
    archives/patches; local and generated filesystem probes are not accepted.
    The caller must authenticate the complete original lock bytes, not merely a
    subset of registryFileHashes, before calling this function.
    """
    environment = os.environ if environment is None else environment
    deadline = time.monotonic() + timeout
    registry = _registry(registry)
    _producer(source)
    require(isinstance(lock_bytes, bytes) and 0 < len(lock_bytes) <= MAX_LOCK_BYTES,
            "invalid kernel resolution lock size")
    lock = _json(lock_bytes)
    require(isinstance(lock, dict) and lock.get("lockFileVersion") == 24
            and set(lock) <= {"lockFileVersion", "registryFileHashes", "selectedYankedVersions", "moduleExtensions", "facts"},
            "unknown kernel resolution lock format")
    require(not lock.get("selectedYankedVersions"), "kernel lock selects a yanked dependency")
    require(isinstance(lock.get("facts", {}), dict), "invalid kernel extension facts")
    fetch = fetch or _fetch
    inputs = _registry_inputs(lock.get("registryFileHashes"), registry, fetch, deadline - time.monotonic())
    modules = _module_sources(inputs, registry, source)
    _yanked_versions(modules, registry, fetch, deadline - time.monotonic())
    _extensions(lock.get("moduleExtensions", {}), modules, environment)
    try:
        cache_facts.validate(lock.get("facts", {}), lambda urls: _fetch_many(urls, fetch, deadline - time.monotonic()))
    except (OSError, ValueError, TypeError) as error:
        raise CacheUnverifiable("kernel extension facts cannot be revalidated: " + str(error)) from error
