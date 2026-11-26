"""Identify the native root-module kernel request before starting Bazel."""

import ast
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import stat
import subprocess

from tools.bazel.kernel.cache_identity import require


# This registered toolchain supplies sonic-utilities data generation. Kernel
# targets do not select its toolchain type, but Bazel loads its declaration.
UTILITIES_TOOLCHAIN = "//tools/bazel/image/vs/utilities_data:toolchain"
UTILITIES_TOOLCHAIN_BUILD = "tools/bazel/image/vs/utilities_data/BUILD.bazel"

# These module declarations contain comprehensions and repo rules as well as
# literal file labels. A new declaration can introduce another external input.
# Re-audit its file reads before enabling reuse for a different graph.
MODULE_FILES = {
    UTILITIES_TOOLCHAIN_BUILD: "c565d060ef41510587cace2256fff1845b98210ed3571c2fc4c2fe653331380d",
    "MODULE.bazel": "69ebf4321d6cd15ee991f770d7db7f1529dea9bd1c5db5308a2db0e0bbde72f0",
    # The device-data repo rule reads host data only when that repository is used.
    # Its implementation was audited against the kernel target dependency graph.
    "src/sonic-device-data/input_metadata.bzl": "42105a32d10b65657d1009b268499f8f2c2c895a3aaa577de86022f7593d9ace",
    "src/sonic-frr/deps.MODULE.bazel": "ac806146b62547f8c1324da9c577e915dff103432ed329e996e7d173fbf06dbf",
    "src/sonic-supervisord-utilities-rs/deps.MODULE.bazel": "0778d4747d59dc2d27147a38f536e22fdd4bc0a2a24c2e538f050bfae75b68ad",
    "tools/bazel/equivalence_checker/equivalence.MODULE.bazel": "e80a42304c8c9780ed6438fffb47af9fb60f8b94b27a2d6e34242a3e2ebe853a",
}
# Includes the import graph, ordered registries, strict action environment,
# downloader policy path, platforms, toolchain switches, and remote-cache flags.
RC_FILES = {
    ".bazelrc": "e615f468bd5325f385113c06c9f1d80ec16f9ec43a499fea5dcfffdc70d9b371",
    "tools/bazel/common.bazelrc": "851e54956e6acb9276f5df438fa06022f7555515ac56897a9804a6165559531a",
    "tools/bazel/slave.bazelrc": "f9cd2a0ee500a23c0a6e9efbb09d1a19151a0d2f1a6b2cc274cbcdfde63fe9cf",
}
# Workspace Python modules imported by the kernel consumer outside its package.
CONSUMER_HELPERS = {
    "tools/bazel/build_helpers.py", "tools/bazel/cache/credentials.py", "tools/bazel/image/file_copy.py",
    "tools/bazel/ci/kernel_artifacts.py", "tools/bazel/ci/public_artifacts.py", "tools/bazel/ci/resolution.py",
    "tools/bazel/ci/resources.py", "tools/bazel/ci/time_vs_image.py",
}
LOCAL_MODULES = {
    "bash-tacplus": "src/tacacs/bash_tacplus",
    "sonic-sysmgr": "src/sonic-sysmgr",
    "sonic-yang-mgmt": "src/sonic-yang-mgmt",
    "sonic-yang-models": "src/sonic-yang-models",
}
KERNEL = "0.0.3-96c621843a2bc9dbbe17eb890c8c6e1cea5e834b"
INFRA = "0.0.42-da5f4c8d370f501572f435ade330ec209c9c0203"
BAZEL_SHA256 = "61d89402f0368e64b6c827be5de79d8e65382e8124c3cbb97325611a1851392e"
PLATFORM = "@sonic_build_infra//platforms:x86_64_trixie"
VS_REMOTE_CACHE = "grpc://172.16.0.4:9092"
CACHE_FALLBACK_FLAGS = {"--remote_local_fallback", "--incompatible_remote_local_fallback_for_remote_cache"}
# resources.py --configure emits these two lines into the VS workspace before
# the slave starts. They only budget the JVM heap and local action scheduler.
VS_RESOURCE_RC = re.compile(rb"startup --host_jvm_args=-Xmx2048m\nbuild --local_ram_resources=[1-9][0-9]*\n")
SLAVE_RC = Path("/etc/bazel.bazelrc")
BAZELISK = Path("/usr/local/bin/bazel")
BAZELISK_HOME = Path("/bazel_cache/bazelisk")
WORKER_PATHS = {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin", "/bin",
                "/usr/.cargo/bin", "/usr/games", "/usr/local/games"}


def _hash(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def _git(directory, *args):
    return subprocess.check_output(["git", "-C", str(directory), *args], stderr=subprocess.PIPE)


def _module(workspace, name):
    data = (workspace / name).read_bytes()
    require(hashlib.sha256(data).hexdigest() == MODULE_FILES[name],
            f"root kernel module declaration needs input review: {name}")
    return ast.parse(data, filename=name)


def _calls(workspace, command):
    for statement in _module(workspace, "MODULE.bazel").body:
        call = statement.value if isinstance(statement, ast.Expr) else None
        if isinstance(call, ast.Call) and isinstance(call.func, ast.Name) and call.func.id == command:
            yield {item.arg: ast.literal_eval(item.value) for item in call.keywords}


def local_modules(workspace):
    """Return the in-checkout repositories that replace Registry sources."""
    workspace = Path(workspace)
    paths = {}
    for entry in _calls(workspace, "local_path_override"):
        name = entry["module_name"]
        require(name in LOCAL_MODULES and name not in paths and entry["path"] == LOCAL_MODULES[name],
                "unexpected root kernel local module override")
        paths[name] = workspace / entry["path"]
    require(paths.keys() == LOCAL_MODULES.keys(), "root kernel local module inventory changed")
    return paths


def declared_kernel_modules(workspace):
    workspace = Path(workspace)
    expected = {"sonic-linux-kernel": KERNEL, "sonic-build-infra": INFRA}
    versions = {item["name"]: item["version"] for item in _calls(workspace, "bazel_dep")
                if item["name"] in expected}
    for override in _calls(workspace, "single_version_override"):
        if override["module_name"] in expected and override.get("version"):
            versions[override["module_name"]] = override["version"]
    require(versions == expected, "root kernel producer release has not been audited")
    return versions


def _tracked_inputs(workspace):
    paths = {".bazelversion", ".bazelignore", ".gitmodules", "Makefile.work", "rules/linux-kernel.mk",
             "tools/bazel/debian_mirrors.cfg", *CONSUMER_HELPERS,
             "tools/bazel/kernel/root_resolution_registry.json", "tools/bazel/kernel/cache_wire.proto",
             *RC_FILES, *MODULE_FILES}
    paths.update(str(path.relative_to(workspace)) for path in local_modules(workspace).values())
    required = set(paths)
    for helper in CONSUMER_HELPERS:
        paths.update((parent / "__init__.py").as_posix() for parent in Path(helper).parents
                     if parent != Path("."))
    for module in MODULE_FILES:
        declaration = _module(workspace, module)
        if module == UTILITIES_TOOLCHAIN_BUILD:
            continue
        for node in ast.walk(declaration):
            if isinstance(node, ast.Constant) and isinstance(node.value, str) and node.value.startswith("//"):
                package, filename = node.value[2:].split(":", 1)
                label = Path(package) / filename
                if node.value == UTILITIES_TOOLCHAIN:
                    label = Path(UTILITIES_TOOLCHAIN_BUILD)
                require(".." not in label.parts and not label.is_absolute(), "non-workspace root module input")
                paths.add(label.as_posix())
                required.add(label.as_posix())
                # A newly introduced BUILD file can shadow the existing package.
                for parent in label.parents:
                    paths.update((parent / name).as_posix() for name in ("BUILD", "BUILD.bazel"))
    paths.add("src/sonic-linux-kernel")
    # Include the consumer/verification code, without treating unrelated files
    # generated by Make elsewhere in the checkout as kernel inputs.
    consumer = "tools/bazel/kernel"
    entries = _git(workspace, "ls-tree", "--name-only", "-z", "HEAD:" + consumer).split(b"\0")
    paths.update(consumer + "/" + os.fsdecode(file) for file in entries if file.endswith(b".py"))
    paths.update(str(file.relative_to(workspace)) for file in (workspace / consumer).glob("*.py"))
    inventory = _git(workspace, "ls-tree", "-r", "-z", "HEAD", "--", *sorted(paths))
    tracked = {}
    for item in inventory.rstrip(b"\0").split(b"\0"):
        metadata, name = item.split(b"\t", 1)
        mode, _, oid = metadata.decode().split()
        tracked[os.fsdecode(name)] = (mode, oid)
    missing = [name for name in sorted(required)
               if name not in tracked and not any(path.startswith(name + "/") for path in tracked)]
    require(not missing, "missing tracked root module inputs: " + ", ".join(missing))
    require(not _git(workspace, "diff", "--no-ext-diff", "--name-only", "--ignore-submodules=none",
                     "HEAD", "--", *sorted(paths))
            and not _git(workspace, "ls-files", "--others", "-z", "--", *sorted(paths)),
            "root kernel inputs have local changes or extra files")
    checked = hashlib.sha256(inventory)
    for name, (mode, oid) in tracked.items():
        path = workspace / name
        if mode == "160000":
            require((path / ".git").exists() and _git(path, "rev-parse", "HEAD").decode().strip() == oid
                    and not _git(path, "ls-files", "--others", "-z"),
                    f"root kernel gitlink is unavailable or has extra files: {name}")
        elif mode == "120000":
            require(name == "src/sonic-sysmgr/.bazelversion" and path.is_symlink()
                    and path.resolve(strict=True) == workspace / ".bazelversion",
                    f"unbound symlink in kernel root inputs: {name}")
        else:
            require(stat.S_ISREG(path.lstat().st_mode) and path.resolve(strict=True) == path,
                    f"root kernel file is not in the checkout: {name}")
            require(bool(path.stat().st_mode & stat.S_IXUSR) == (mode == "100755"),
                    f"root kernel input mode differs: {name}")
            # Also bind the bytes Bazel actually reads if an index stat entry
            # was stale or a checkout filter changed the on-disk representation.
            checked.update(bytes.fromhex(_hash(path)))
    require(tracked["src/sonic-linux-kernel"] == ("160000", KERNEL.split("-", 1)[1]),
            "kernel gitlink differs from its registry declaration")
    return checked.hexdigest()


def _execution(workspace, environment, bazel):
    require(platform.system() == "Linux" and platform.machine() == "x86_64", "expected native AMD64 worker")
    builder = environment.get("SONIC_HOSTBASE_BUILDER_ID", "")
    require(re.fullmatch(r"sha256:[0-9a-f]{64}", builder), "root kernel cache requires a trusted builder image")
    for key in environment:
        if not environment[key]:
            continue
        if key.startswith(("BAZELISK_", "BAZEL_")):
            require(key in {"BAZELISK_HOME", "BAZEL_CONTAINER_ARGS"}, "unexpected Bazel setting: " + key)
        require(key not in {"USE_BAZEL_VERSION", "BAZELRC", "BAZEL_OPTS", "BAZEL_FLAGS", "BAZELISK",
                            "JAVA_TOOL_OPTIONS", "JDK_JAVA_OPTIONS", "_JAVA_OPTIONS", "LD_PRELOAD",
                            "LD_LIBRARY_PATH", "PYTHONPATH", "PYTHONHOME", "BASH_ENV", "SONIC_BAZEL_REMOTE_ONLY_RC",
                            "CC", "CXX", "CFLAGS", "CXXFLAGS", "LDFLAGS", "GIT_DIR", "GIT_COMMON_DIR",
                            "GIT_WORK_TREE", "GIT_INDEX_FILE", "GIT_CONFIG_PARAMETERS", "GIT_CONFIG_COUNT",
                            "GIT_OBJECT_DIRECTORY", "GIT_ALTERNATE_OBJECT_DIRECTORIES"},
                "unbound root kernel execution setting: " + key)
    require(set(environment.get("PATH", "").split(os.pathsep)) <= WORKER_PATHS,
            "kernel builder PATH includes a directory outside the image toolchain")
    require(bazel == environment.get("BAZEL", "bazel") == "bazel"
            and shutil.which(bazel, path=environment["PATH"]) == str(BAZELISK)
            and not BAZELISK.is_symlink(), "unexpected Bazel executable")
    require((workspace / ".bazelversion").read_text().strip() == "8.5.1"
            and environment.get("BAZELISK_HOME") == str(BAZELISK_HOME), "unexpected Bazel version/cache")
    for rc, digest in RC_FILES.items():
        require(_hash(workspace / rc) == digest, "root Bazel configuration needs review: " + rc)
    home = Path(environment.get("HOME", ""))
    require(home.is_absolute(), "root kernel worker home is unknown")
    resource_files = {home / ".bazelrc", workspace / ".bazelrc.user"}
    for file in (*resource_files, home / ".bazeliskrc", workspace / ".bazeliskrc",
                 workspace / "WORKSPACE", workspace / "WORKSPACE.bazel"):
        if file.is_symlink() or file.exists():
            require(file.is_file() and not file.is_symlink(), "redirected root kernel configuration: " + str(file))
            data = file.read_bytes()
            if file in resource_files and VS_RESOURCE_RC.fullmatch(data):
                continue
            require(all(not line.strip() or line.lstrip().startswith(b"#") for line in data.splitlines()),
                    "unexpected root kernel configuration: " + str(file))
    require(not SLAVE_RC.is_symlink() and _hash(SLAVE_RC) == RC_FILES["tools/bazel/slave.bazelrc"],
            "system Bazel configuration differs from the checked-in slave RC")
    # A fresh worker may skip Bazel entirely. If Bazelisk has a cached binary,
    # verify its routing metadata and the actual executable before trusting it.
    metadata = BAZELISK_HOME / "downloads/metadata/bazelbuild/bazel-8.5.1-linux-x86_64"
    binary = BAZELISK_HOME / "downloads/sha256" / BAZEL_SHA256 / "bin/bazel"
    if metadata.exists() or binary.exists() or metadata.is_symlink() or binary.is_symlink():
        require(metadata.resolve(strict=True) == metadata and metadata.read_text().strip() == BAZEL_SHA256
                and binary.resolve(strict=True) == binary and _hash(binary) == BAZEL_SHA256,
                "unexpected cached Bazel executable")
    return {"builder": builder, "bazelisk": _hash(BAZELISK), "bazel": BAZEL_SHA256,
            "system": platform.release(), "path": environment["PATH"]}


def is_cache_fallback_option(option):
    """These two Bazel booleans change how a cache outage is handled."""
    flag, equals, value = option.partition("=")
    if flag in CACHE_FALLBACK_FLAGS:
        return not equals or value.lower() in ("true", "1", "yes", "t", "y", "false", "0", "no", "f", "n")
    return option.startswith("--no") and "--" + option[4:] in CACHE_FALLBACK_FLAGS


def request_key(workspace, architecture, kernel_config, *, environment=None, options=(), bazel="bazel"):
    """Return a pre-launch identity or raise ValueError/OSError for ordinary Bazel fallback."""
    workspace = Path(workspace).resolve(strict=True)
    environment = os.environ if environment is None else environment
    require(architecture == "amd64" and all(kernel_config.get(key) == value for key, value in {
        "CONFIGURED_ARCH": "amd64", "CONFIGURED_PLATFORM": "vs", "BLDENV": "trixie",
        "CROSS_BUILD_ENVIRON": "n", "MULTIARCH_QEMU_ENVIRON": "n", "SECURE_UPGRADE_MODE": "no_sign",
        "INCLUDE_EXTERNAL_PATCHES": "n", "EXTERNAL_KERNEL_PATCH_URL": "", "EXTERNAL_KERNEL_PATCH_LOC": "",
        "SECURE_UPGRADE_KERNEL_CAFILE": "", "ADDITIONAL_BUILD_PROFILES": "",
    }.items()), "expected the native unsigned VS kernel configuration")
    flags = ["--platforms=" + PLATFORM, "--host_platform=" + PLATFORM,
             "--@sonic_linux_kernel//tools/bazel:kernel_config=" + json.dumps(kernel_config, sort_keys=True)]
    require(list(options[:3]) == flags, "root kernel Bazel configuration differs from its arguments")
    endpoints = set()
    for option in options[3:]:
        if is_cache_fallback_option(option):
            continue
        flag, _, value = option.partition("=")
        if flag == "--remote_cache":
            require(value == VS_REMOTE_CACHE, "unsupported root kernel cache endpoint")
            endpoints.add(value)
        elif flag == "--remote_upload_local_results":
            require(value in ("true", "false"), "unsupported Bazel upload policy")
        else:
            require(flag in ("--repository_cache", "--disk_cache")
                    and value == "/bazel_cache/kernel-linux-sandbox/" + flag[2:],
                    "unbound root kernel Bazel option: " + flag)
    require(len(endpoints) == 1, "expected one explicit kernel gRPC endpoint")
    identity = {"contract": "sonic-root-kernel-v1", "options": flags, "remote": next(iter(endpoints)),
                "modules": declared_kernel_modules(workspace), "inputs": _tracked_inputs(workspace),
                "execution": _execution(workspace, environment, bazel)}
    return hashlib.sha256(json.dumps(identity, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
