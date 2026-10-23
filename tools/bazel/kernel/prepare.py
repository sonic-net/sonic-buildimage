#!/usr/bin/env python3
"""Forward Make configuration to Bazel and verify its kernel Debian packages."""

import argparse
from contextlib import contextmanager
import ast
import base64
import hashlib
import json
import fcntl
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import platform
import tempfile
import urllib.parse
import urllib.request

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import resolution

MANIFEST = "kernel-packages.json"
PROVENANCE = "kernel-provenance.json"
TARGET = "@sonic_linux_kernel//:kernel_packages"
DEBS_DIRECTORY = Path("target/debs/trixie")
KERNEL_PATH = Path("src/sonic-linux-kernel")
REGISTRY_PREFIX = "https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/"
DEFAULT_REGISTRY = REGISTRY_PREFIX + "main"
WATCHED = [
    ".gitmodules", "Makefile", "Makefile.work", "rules/linux-kernel.mk", "rules/linux-kernel.dep",
    "tools/bazel/ci/resolution.py", "tools/bazel/ci/kernel_make_smoke.py", "rules/config",
    "tools/bazel/kernel", "build_debian.sh", "slave.mk", "Makefile.cache",
    "tools/bazel/ci/kernel_install_smoke.sh", "scripts/build_sbom.py",
    "scripts/sbom_kernel_deb.py", str(KERNEL_PATH),
]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def regular(path):
    path = Path(path)
    require(path.is_file() and not path.is_symlink() and path.stat().st_size > 0,
            "expected nonempty regular kernel output: " + str(path))
    return path


def git(workspace, *args):
    return subprocess.check_output(
        ["git", "-c", "safe.directory=" + str(workspace), "-C", str(workspace), *args],
        text=True, timeout=60).strip()


def endpoint(value):
    parsed = urllib.parse.urlsplit(value)
    require(parsed.scheme in {"http", "https", "grpc", "grpcs"} and parsed.hostname
            and parsed.username is None and parsed.password is None and not parsed.query
            and not parsed.fragment and not re.search(r"[\s\x00-\x1f]", value),
            "kernel cache needs an HTTP(S) or gRPC(S) endpoint without URL credentials or query parameters")
    return value


def module_declarations(path):
    """Read literal module declarations without executing Starlark."""
    modules = {}
    root = None
    for node in ast.parse(path.read_text()).body:
        if not isinstance(node, ast.Expr) or not isinstance(node.value, ast.Call):
            continue
        call = node.value
        if not isinstance(call.func, ast.Name):
            continue
        name = call.func.id
        require(not name.endswith("_override"), "kernel consumer template must not contain module overrides")
        if name not in {"module", "bazel_dep"}:
            continue
        values = {item.arg: ast.literal_eval(item.value) for item in call.keywords}
        if name == "module":
            require(root is None, "duplicate module declaration")
            root = values
        else:
            require(values.get("name") not in modules, "duplicate kernel dependency declaration")
            modules[values["name"]] = values
    return root, modules


def consumer_modules(workspace, source, kernel_gitlink):
    root, modules = module_declarations(workspace / "tools/bazel/kernel/MODULE.bazel")
    source_root, source_modules = module_declarations(source / "MODULE.bazel")
    require(root == {"name": "sonic-kernel-cache-consumer"}, "unexpected kernel consumer module")
    require(set(modules) == {"sonic-linux-kernel", "sonic-build-infra"},
            "kernel consumer must declare only the kernel and shared tools modules")
    require(source_root.get("name") == "sonic-linux-kernel"
            and modules["sonic-linux-kernel"] == {
                "name": "sonic-linux-kernel", "version": source_root.get("version") + "-" + kernel_gitlink,
                "repo_name": "sonic_linux_kernel"},
            "registered kernel version differs from the checked-out gitlink")
    infra = modules["sonic-build-infra"]
    require(infra == source_modules.get("sonic-build-infra")
            and infra.get("repo_name") == "sonic_build_infra"
            and re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+-[0-9a-f]{40}", infra.get("version", "")),
            "kernel consumer tools version differs from its source module")
    return {name: value["version"] for name, value in modules.items()}


def native_architecture():
    machine = platform.machine()
    require(platform.system() == "Linux" and machine in {"x86_64", "aarch64"},
            "kernel packages require a native Linux AMD64 or ARM64 host")
    return {"x86_64": "amd64", "aarch64": "arm64"}[machine]


def launcher_contract(workspace, source, architecture="amd64"):
    names = {"BAZEL_URL", "BAZEL_SHA256", "ARCHITECTURES"}
    values = {}
    def literal(node):
        if isinstance(node, ast.Name) and node.id in values:
            return values[node.id]
        if isinstance(node, ast.Dict):
            return {literal(k): literal(v) for k, v in zip(node.keys, node.values)}
        return ast.literal_eval(node)
    for node in ast.parse((source / "tools/bazel/build.py").read_text()).body:
        if (isinstance(node, ast.Assign) and len(node.targets) == 1
                and isinstance(node.targets[0], ast.Name) and node.targets[0].id in names):
            values[node.targets[0].id] = literal(node.value)
    version = (workspace / "tools/bazel/kernel/.bazelversion").read_text().strip()
    require(version == (source / ".bazelversion").read_text().strip()
            and re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", version),
            "kernel consumer Bazel version differs from its source launcher")
    selected = values.get("ARCHITECTURES", {}).get(architecture, {})
    suffix = {"amd64": "x86_64", "arm64": "arm64"}.get(architecture)
    require(set(values) == names and suffix
            and selected.get("bazel_url") == "https://releases.bazel.build/" + version + "/release/bazel-" + version + "-linux-" + suffix
            and re.fullmatch(r"[0-9a-f]{64}", selected.get("bazel_sha256", "")),
            "kernel launcher must pin its native Bazel executable")
    return {"execution_strategy": "linux-sandbox", "architecture": architecture,
            "bazel_version": version, "bazel_sha256": selected["bazel_sha256"]}


CONFIG_KEYS = (
    "CONFIGURED_ARCH", "CONFIGURED_PLATFORM", "BLDENV", "CROSS_BUILD_ENVIRON",
    "MULTIARCH_QEMU_ENVIRON", "KERNEL_VERSION", "KERNEL_ABISUFFIX", "KERNEL_SUBVERSION",
    "KERNEL_FEATURESET", "KVERSION", "ADDITIONAL_BUILD_PROFILES", "SECURE_UPGRADE_MODE",
    "SECURE_UPGRADE_KERNEL_CAFILE", "INCLUDE_EXTERNAL_PATCHES",
    "EXTERNAL_KERNEL_PATCH_URL", "EXTERNAL_KERNEL_PATCH_LOC",
)


def parse_kernel_config(values):
    """Parse Make assignments without interpreting shell or Make expressions."""
    result = {}
    for assignment in values:
        name, separator, value = assignment.partition("=")
        require(separator and name in CONFIG_KEYS, "unknown kernel configuration assignment: " + name)
        require(name not in result, "duplicate kernel configuration assignment: " + name)
        result[name] = value
    return result


def effective_config(workspace, architecture=None, kernel_config=None):
    """Fill standalone defaults; capability validation belongs to the producer."""
    architecture = architecture or native_architecture()
    values = {
        "CONFIGURED_ARCH": architecture, "CONFIGURED_PLATFORM": "vs", "BLDENV": "trixie",
        "CROSS_BUILD_ENVIRON": "n", "MULTIARCH_QEMU_ENVIRON": "n",
        "ADDITIONAL_BUILD_PROFILES": "", "SECURE_UPGRADE_MODE": "no_sign",
        "SECURE_UPGRADE_KERNEL_CAFILE": "", "INCLUDE_EXTERNAL_PATCHES": "n",
        "EXTERNAL_KERNEL_PATCH_URL": "", "EXTERNAL_KERNEL_PATCH_LOC": "",
    }
    contents = (workspace / "rules/linux-kernel.mk").read_text()
    for name in ("KERNEL_VERSION", "KERNEL_ABISUFFIX", "KERNEL_SUBVERSION", "KERNEL_FEATURESET"):
        matches = re.findall(r"^" + name + r"\s*=\s*([A-Za-z0-9.+~-]+)\s*$", contents, re.MULTILINE)
        require(len(matches) == 1, "kernel version default must be literal: " + name)
        values[name] = matches[0]
    supplied = {} if kernel_config is None else kernel_config
    require(isinstance(supplied, dict) and set(supplied) <= set(CONFIG_KEYS)
            and all(isinstance(value, str) for value in supplied.values()),
            "kernel configuration must use known string-valued Make assignments")
    values.update(supplied)
    for name in ("CROSS_BUILD_ENVIRON", "MULTIARCH_QEMU_ENVIRON", "INCLUDE_EXTERNAL_PATCHES"):
        if values[name] == "":
            values[name] = "n"
    values.setdefault("KVERSION", values["KERNEL_VERSION"] + values["KERNEL_ABISUFFIX"]
                      + "-" + values["KERNEL_FEATURESET"] + "-"
                      + ("armmp" if values["CONFIGURED_ARCH"] == "armhf" else values["CONFIGURED_ARCH"]))
    mode = values["SECURE_UPGRADE_MODE"]
    if len(mode) >= 2 and mode.startswith('"') and mode.endswith('"'):
        values["SECURE_UPGRADE_MODE"] = mode[1:-1]
    values["SECURE_UPGRADE_MODE"] = values["SECURE_UPGRADE_MODE"] or "no_sign"
    return values


def contract(workspace, architecture=None, kernel_config=None):
    """Derive output names from the effective Make configuration."""
    values = effective_config(workspace, architecture, kernel_config)
    architecture = values["CONFIGURED_ARCH"]
    version = values["KERNEL_VERSION"]
    release = version + "-" + values["KERNEL_SUBVERSION"]
    abi = version + values["KERNEL_ABISUFFIX"]
    feature = values["KERNEL_FEATURESET"]
    kernel = values["KVERSION"]
    expected = {"schema": 3, "architecture": architecture, "platform": values["CONFIGURED_PLATFORM"],
                "kernel_version": version, "kernel_abi": kernel,
                "package_version": release,
                # Make signs installed boot files during image assembly, after
                # consuming the kernel DEB. Trust configuration is separate.
                "signing": "unsigned",
                "kernel_config": values}
    packages = {}
    for name, arch, kind in (
        ("linux-image-" + kernel + ("" if architecture == "armhf" else "-unsigned"), architecture, "runtime"),
        ("linux-image-" + kernel + "-dbg", architecture, "debug"),
        ("linux-headers-" + kernel, architecture, "headers"),
        ("linux-headers-" + abi + "-common-" + feature, "all", "headers_common"),
        ("linux-kbuild-" + abi, architecture, "kbuild"),
    ):
        packages[name + "_" + release + "_" + arch + ".deb"] = {
            "kind": kind, "package": name, "version": release, "architecture": arch}
    return expected, packages


def source_archives(source):
    """Read the source owner's three literal archive pins without executing it."""
    tree = ast.parse((source / "tools/bazel/sources.bzl").read_text())
    declarations = [node.value for node in tree.body if isinstance(node, ast.Assign)
                    and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name)
                    and node.targets[0].id == "KERNEL_SOURCES"]
    require(len(declarations) == 1 and isinstance(declarations[0], ast.Dict),
            "kernel source must declare a literal KERNEL_SOURCES map")
    result = []
    for value in declarations[0].values:
        require(isinstance(value, ast.Call) and isinstance(value.func, ast.Name)
                and value.func.id == "struct" and not value.args,
                "kernel source archive declarations must be literal structs")
        fields = {item.arg: ast.literal_eval(item.value) for item in value.keywords}
        require(set(fields) == {"name", "sha256"} and isinstance(fields["name"], str)
                and re.fullmatch(r"[0-9a-f]{64}", fields.get("sha256", "")),
                "invalid kernel source archive declaration")
        result.append(fields)
    require(len(result) == 3 and len({item["name"] for item in result}) == 3,
            "kernel source must pin three distinct source archives")
    return sorted(result, key=lambda item: item["name"])


def source_state(workspace, architecture=None, kernel_config=None):
    architecture = architecture or native_architecture()
    configuration = effective_config(workspace, architecture, kernel_config)
    workspace = workspace.resolve(strict=True)
    source = workspace / KERNEL_PATH
    source_commit = git(workspace, "rev-parse", "HEAD")
    require(re.fullmatch(r"[0-9a-f]{40}", source_commit), "invalid buildimage source commit")
    recorded = git(workspace, "ls-tree", "HEAD", "--", str(KERNEL_PATH))
    match = re.fullmatch(r"160000 commit ([0-9a-f]{40})\tsrc/sonic-linux-kernel", recorded)
    require(match is not None, "buildimage must pin the kernel as a gitlink")
    kernel_gitlink = match.group(1)
    require(git(source, "rev-parse", "HEAD") == kernel_gitlink,
            "kernel checkout differs from the buildimage gitlink")
    require(not git(workspace, "diff", "--name-only", "HEAD", "--", *WATCHED),
            "kernel handoff inputs must match the buildimage source commit")
    require(not git(source, "status", "--porcelain=v1", "--untracked-files=all"),
            "kernel checkout must have no tracked or untracked changes")
    selected = ["Makefile", "manage-config", "config.local", "patches-debian", "patches-sonic",
                "tools/bazel/kernel_arch.py", "tools/bazel/stage_kernel.py"]
    tracked = set(git(source, "ls-files", "--", *selected).splitlines())
    actual = {"Makefile", "manage-config", "tools/bazel/kernel_arch.py", "tools/bazel/stage_kernel.py"}
    for name in selected[2:]:
        actual.update(str(path.relative_to(source)) for path in (source / name).rglob("*") if path.is_file())
    require(actual == tracked and all(not (source / name).is_symlink() for name in actual),
            "kernel source inventory differs from its tracked gitlink inputs")
    helper = regular(source / "tools/bazel/kernel_action.py")
    # The pinned helper must resolve its sibling imports independently of the
    # caller's PYTHONPATH/PYTHONSAFEPATH settings.
    identity = subprocess.check_output(
        [sys.executable, "-E", "-B", str(helper), "--source-tree-sha256", str(source)], text=True, timeout=60).strip()
    require(re.fullmatch(r"[0-9a-f]{64}", identity), "invalid kernel checkout source digest")
    # Bind consumption to the checked handoff files, not an unrelated SWSS or
    # buildimage commit. This permits verified reuse without changing package,
    # manifest or provenance timestamps merely because HEAD advanced.
    handoff = git(workspace, "ls-tree", "-r", "HEAD", "--", *WATCHED)
    return {"source_commit": source_commit, "consumer_inputs_sha256": hashlib.sha256(handoff.encode()).hexdigest(),
            "kernel_gitlink": kernel_gitlink,
            "source_tree_sha256": identity, "source_archives": source_archives(source),
            "kernel_config": configuration,
            "modules": consumer_modules(workspace, source, kernel_gitlink), "launcher": launcher_contract(workspace, source, architecture)}


def verify_tools(tools, architecture):
    inputs = tools.get("input_sha256") if isinstance(tools, dict) else None
    require(isinstance(tools, dict) and tools.get("schema_version") == 1
            and tools.get("kind") == "debian-build-tools" and tools.get("architecture") == architecture
            and isinstance(inputs, list) and inputs
            and all(isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value) for value in inputs)
            and inputs == sorted(inputs) and isinstance(tools.get("packages"), dict) and tools["packages"]
            and all(isinstance(name, str) and isinstance(version, str) and name and version
                    for name, version in tools["packages"].items()),
            "kernel build tools must have a declared native runtime identity")
    expected = hashlib.sha256(json.dumps({"architecture": architecture, "inputs": inputs},
                                         sort_keys=True).encode()).hexdigest()
    require(tools.get("identity_sha256") == expected, "kernel build-tool identity differs from its input hashes")


def verify(bundle, workspace, *, source=None, architecture=None, kernel_config=None, with_provenance=True):
    bundle = bundle.resolve(strict=True)
    source = source or source_state(workspace, architecture, kernel_config)
    require(architecture is None or architecture == source["launcher"]["architecture"],
            "kernel verification execution architecture differs from its source")
    require(kernel_config is None or effective_config(workspace, source["launcher"]["architecture"], kernel_config)
            == source["kernel_config"], "kernel verification configuration differs from its source")
    architecture = source["launcher"]["architecture"]
    expected, artifacts = contract(workspace, architecture, source["kernel_config"])
    names = set(artifacts) | {MANIFEST} | ({PROVENANCE} if with_provenance else set())
    require(names <= {path.name for path in bundle.iterdir()}, "kernel package directory has missing files")
    manifest = json.loads(regular(bundle / MANIFEST).read_text())
    require(all(manifest.get(key) == value for key, value in expected.items()),
            "Bazel kernel does not match the requested package configuration")
    require(manifest.get("source_tree_sha256") == source["source_tree_sha256"],
            "kernel action source inputs differ from the pinned kernel checkout")
    files = manifest.get("source_files")
    require(isinstance(files, list) and files and all(
        isinstance(item, dict) and set(item) == {"name", "mode", "sha256"}
        and isinstance(item["name"], str) and item["name"] and item["mode"] in {0o644, 0o755}
        and isinstance(item["sha256"], str) and re.fullmatch(r"[0-9a-f]{64}", item["sha256"])
        for item in files), "kernel manifest omitted its source inventory")
    require(files == sorted(files, key=lambda item: item["name"])
            and len({item["name"] for item in files}) == len(files)
            and hashlib.sha256(json.dumps(files, sort_keys=True).encode()).hexdigest() == source["source_tree_sha256"],
            "kernel manifest source inventory differs from its source digest")
    require(manifest.get("source_archives") == source["source_archives"],
            "kernel source archives differ from the pinned kernel checkout")
    verify_tools(manifest.get("build_tools"), architecture)
    entries = manifest.get("packages")
    require(isinstance(entries, list) and len(entries) == len(artifacts)
            and all(isinstance(item, dict) for item in entries)
            and {item.get("name") for item in entries} == set(artifacts),
            "kernel manifest must contain exactly five runtime, debug and development DEBs")
    for item in entries:
        name = item["name"]
        require(all(item.get(key) == value for key, value in artifacts[name].items()),
                "kernel artifact metadata differs: " + name)
        require(type(item.get("size")) is int and item["size"] > 0
                and isinstance(item.get("sha256"), str) and re.fullmatch(r"[0-9a-f]{64}", item["sha256"]),
                "invalid kernel artifact digest or size: " + name)
        path = regular(bundle / name)
        require(path.stat().st_size == item["size"] and sha256(path) == item["sha256"],
                "kernel artifact failed size/SHA256 verification: " + name)
        control = subprocess.check_output(
            ["dpkg-deb", "--field", str(path), "Package", "Version", "Architecture"], text=True)
        fields = dict(line.split(": ", 1) for line in control.strip().splitlines())
        require(fields == {key.title(): item[key] for key in ("package", "version", "architecture")},
                "kernel DEB control differs from its manifest: " + name)
    created = manifest.get("created_packages")
    require(isinstance(created, list) and created
            and all(isinstance(item, dict) and isinstance(item.get("name"), str) for item in created)
            and len({item["name"] for item in created}) == len(created),
            "kernel manifest omitted its complete created-package inventory")
    for item in created:
        require(type(item.get("exported")) is bool
                and item.get("architecture") in {expected["architecture"], "all"}
                and item.get("version") == expected["package_version"]
                and re.fullmatch(r"[a-z0-9][a-z0-9.+-]+", item.get("package", ""))
                and item.get("name") == item["package"] + "_" + item["version"] + "_" + item["architecture"] + ".deb"
                and type(item.get("size")) is int and item["size"] > 0
                and re.fullmatch(r"[0-9a-f]{64}", item.get("sha256", ""))
                and (item["exported"] or item.get("kind") == "other"),
                "invalid created-package inventory entry")
    require(sorted([{k: v for k, v in item.items() if k != "exported"}
                    for item in created if item["exported"]], key=lambda item: item["name"])
            == sorted(entries, key=lambda item: item["name"]),
            "exported kernel packages differ from the complete created-package inventory")
    debug = manifest.get("debug", {})
    require(re.fullmatch(r"[0-9a-f]{8,}", debug.get("kernel_build_id", ""))
            and type(debug.get("module_count")) is int and debug["module_count"] > 0
            and re.fullmatch(r"[0-9a-f]{64}", debug.get("module_build_ids_sha256", "")),
            "kernel manifest has no matching runtime/debug build identity")
    result = {"manifest": manifest}
    if with_provenance:
        provenance = json.loads(regular(bundle / PROVENANCE).read_text())
        require(provenance.get("schema") == 2
                and re.fullmatch(r"[0-9a-f]{40}", provenance.get("source_commit", ""))
                and provenance.get("consumer_inputs_sha256") == source["consumer_inputs_sha256"]
                and provenance.get("kernel_gitlink") == source["kernel_gitlink"]
                and provenance.get("modules") == source["modules"]
                and provenance.get("launcher") == source["launcher"]
                and provenance.get("kernel_config") == source["kernel_config"]
                and provenance.get("manifest_sha256") == sha256(bundle / MANIFEST)
                and provenance.get("target") == TARGET,
                "kernel bundle provenance differs from this source invocation")
        selected = provenance.get("resolution", {})
        require(selected.get("kernel_source_kind") == "registry"
                and selected.get("kernel_source_commit") == source["kernel_gitlink"]
                and selected.get("modules") == source["modules"]
                and selected.get("registry") == selected_registry()
                and selected.get("infra_source_commit") == source["modules"]["sonic-build-infra"].rsplit("-", 1)[1]
                and all(isinstance(selected.get(name), str) and re.fullmatch(r"[0-9a-f]{64}", selected[name])
                        for name in ("graph_sha256", "lock_sha256", "infra_source_json_sha256", "kernel_source_json_sha256"))
                and selected.get("graph_inspection", {}).get("module_graph_complete") is True,
                "kernel bundle has no verified registered-source resolution provenance")
        result["provenance"] = provenance
    return result


def selected_registry():
    # Draft CI may select one reviewed registry branch containing its unmerged
    # dependencies. Normal builds keep the maintained endpoint.
    registry = os.environ.get("SONIC_REVIEW_REGISTRY", DEFAULT_REGISTRY)
    require(re.fullmatch(re.escape(REGISTRY_PREFIX) + r"[A-Za-z0-9][A-Za-z0-9_.-]*(?:/[A-Za-z0-9][A-Za-z0-9_.-]*)*", registry),
            "SONIC_REVIEW_REGISTRY must select one SONiC registry ref")
    return registry


def configure_registry(workspace):
    rc = workspace / ".bazelrc"
    contents = rc.read_text()
    matches = list(re.finditer(r"^common --registry=(" + re.escape(REGISTRY_PREFIX) + r"\S+)$",
                              contents, re.MULTILINE))
    registries = re.findall(r"(?<!\S)--registry=(\S+)", contents)
    require(len(matches) == 1 and matches[0][1] == DEFAULT_REGISTRY
            and registries == [DEFAULT_REGISTRY, "https://bcr.bazel.build"],
            "kernel workspace must select one SONiC registry endpoint on main plus BCR")
    registry = selected_registry()
    if registry != DEFAULT_REGISTRY:
        rc.write_text(contents[:matches[0].start(1)] + registry + contents[matches[0].end(1):])
    return registry


def inspect_resolution(workdir, consumer, source, registry, *, fetch=None):
    graph_path = regular(workdir / "module-graph.json")
    stderr = (workdir / "module-graph.stderr").read_text()
    status = int(regular(workdir / "module-graph.exit-code").read_text())
    graph_text = graph_path.read_text()
    inspection = resolution.inspect_graph(graph_text, stderr, status)
    keys = set()

    def visit(node):
        keys.add(node["key"])
        for field in ("dependencies", "indirectDependencies", "cycles"):
            for child in node.get(field, []):
                visit(child)

    visit(json.loads(graph_text))
    kernels = {key for key in keys if key.startswith("sonic-linux-kernel@")}
    require(kernels == {"sonic-linux-kernel@" + source["modules"]["sonic-linux-kernel"]},
            "resolved graph does not contain exactly the registered kernel source module")
    infra_version = source["modules"]["sonic-build-infra"]
    require({key for key in keys if key.startswith("sonic-build-infra@")} == {"sonic-build-infra@" + infra_version},
            "resolved graph selected a different kernel tools module")
    lock = regular(consumer / "MODULE.bazel.lock")
    hashes = json.loads(lock.read_text()).get("registryFileHashes", {})
    kernel_version = source["modules"]["sonic-linux-kernel"]
    kernel_url = registry + "/modules/sonic-linux-kernel/" + kernel_version + "/source.json"
    require(isinstance(hashes.get(kernel_url), str), "generated lock omitted the kernel source metadata")
    if fetch is None:
        with urllib.request.urlopen(kernel_url, timeout=30) as response:
            kernel_data = response.read(1024 * 1024 + 1)
    else:
        kernel_data = fetch(kernel_url)
    require(len(kernel_data) <= 1024 * 1024 and hashlib.sha256(kernel_data).hexdigest() == hashes[kernel_url],
            "kernel source metadata differs from the tested resolution lock")
    kernel_metadata = json.loads(kernel_data)
    commit = source["kernel_gitlink"]
    require(kernel_metadata.get("url") == "https://github.com/securely1g/sonic-linux-kernel/archive/" + commit + ".tar.gz"
            and kernel_metadata.get("strip_prefix") == "sonic-linux-kernel-" + commit,
            "registered kernel source differs from the checked-out gitlink")
    integrity = kernel_metadata.get("integrity", "")
    require(isinstance(integrity, str) and integrity.startswith("sha256-")
            and len(base64.b64decode(integrity[7:], validate=True)) == 32,
            "kernel registry source has no SHA256 integrity")
    (workdir / "kernel-source.json").write_bytes(kernel_data)
    url = registry + "/modules/sonic-build-infra/" + infra_version + "/source.json"
    require(isinstance(hashes.get(url), str) and re.fullmatch(r"[0-9a-f]{64}", hashes[url]),
            "generated lock omitted the selected kernel tools source metadata")
    if fetch is None:
        with urllib.request.urlopen(url, timeout=30) as response:
            data = response.read(1024 * 1024 + 1)
    else:
        data = fetch(url)
    require(len(data) <= 1024 * 1024 and hashlib.sha256(data).hexdigest() == hashes[url],
            "kernel tools source metadata differs from the tested resolution lock")
    metadata = json.loads(data)
    infra_commit = infra_version.rsplit("-", 1)[1]
    require(metadata.get("url") == "https://github.com/securely1g/sonic-build-infra/archive/" + infra_commit + ".tar.gz"
            and metadata.get("strip_prefix") == "sonic-build-infra-" + infra_commit,
            "kernel tools registry source differs from its module version")
    integrity = metadata.get("integrity", "")
    require(isinstance(integrity, str) and integrity.startswith("sha256-")
            and len(base64.b64decode(integrity[7:], validate=True)) == 32,
            "kernel tools registry source has no SHA256 integrity")
    (workdir / "infra-source.json").write_bytes(data)
    return {"kernel_source_kind": "registry", "kernel_source_commit": source["kernel_gitlink"],
            "modules": source["modules"], "registry": registry, "infra_source_commit": infra_commit,
            "graph_sha256": sha256(graph_path), "lock_sha256": sha256(lock),
            "infra_source_json_sha256": hashlib.sha256(data).hexdigest(),
            "kernel_source_json_sha256": hashlib.sha256(kernel_data).hexdigest(), "graph_inspection": inspection}


def debs_path(workspace, directory=None):
    directory = workspace / (directory if directory is not None else DEBS_DIRECTORY)
    require(not directory.is_symlink(), "kernel DEB directory must not be a symlink")
    return directory.resolve()


@contextmanager
def package_lock(workspace):
    target = workspace / "target"
    target.mkdir(exist_ok=True)
    require(not target.is_symlink(), "kernel target directory must not be a symlink")
    descriptor = os.open(target / ".bazel-kernel.lock", os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    with os.fdopen(descriptor, "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield


def stage_outputs(workdir, destination, workspace, source, selected):
    """Publish verified Bazel outputs once into Make's shared DEB directory."""
    architecture = source["launcher"]["architecture"]
    require(source_state(workspace, architecture, source["kernel_config"]) == source, "kernel handoff source changed during the build")
    workdir = workdir.resolve(strict=True)
    names = {}
    for line in regular(workdir / "output-paths.txt").read_text().splitlines():
        path = Path(line)
        require(path.is_absolute() and path.is_relative_to(workdir) and ".." not in path.parts,
                "kernel output must be an absolute path within the build directory")
        candidate = path.resolve(strict=True)
        require(candidate.is_relative_to(workdir), "kernel output escapes its build output root")
        regular(candidate)
        require(path.name not in names, "duplicate kernel output: " + path.name)
        names[path.name] = candidate
    expected = set(contract(workspace, architecture, source["kernel_config"])[1]) | {MANIFEST}
    require(set(names) == expected, "kernel target returned an unexpected output set")
    destination = debs_path(workspace, destination)
    destination.mkdir(parents=True, exist_ok=True)
    # Copy each output once. Renaming these temporary files publishes the set
    # without another persistent package collection or modifying other DEBs.
    with tempfile.TemporaryDirectory(prefix=".kernel-", dir=destination) as temporary:
        incoming = Path(temporary)
        for name, path in names.items():
            with path.open("rb") as src, (incoming / name).open("xb") as dst:
                shutil.copyfileobj(src, dst, length=1024 * 1024)
        verify(incoming, workspace, source=source, architecture=architecture, with_provenance=False)
        provenance = {"schema": 2, "source_commit": source["source_commit"],
                      "consumer_inputs_sha256": source["consumer_inputs_sha256"], "kernel_gitlink": source["kernel_gitlink"],
                      "modules": source["modules"], "launcher": source["launcher"],
                      "kernel_config": source["kernel_config"],
                      "manifest_sha256": sha256(incoming / MANIFEST),
                      "target": TARGET, "resolution": selected}
        (incoming / PROVENANCE).write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n")
        result = verify(incoming, workspace, source=source, architecture=architecture)
        require(source_state(workspace, architecture, source["kernel_config"]) == source, "kernel handoff source changed during publication")
        changed = []
        for name in sorted(expected) + [PROVENANCE]:
            old = destination / name
            require(not old.is_symlink() and (not old.exists() or old.is_file()),
                    "kernel destination must be a regular file: " + str(old))
            if not old.exists() or sha256(old) != sha256(incoming / name):
                changed.append(name)
        if changed:
            # This receipt is the completion marker. A failed/partial publication
            # cannot masquerade as the previous successfully verified package set.
            (destination / PROVENANCE).unlink(missing_ok=True)
            for name in changed:
                if name != PROVENANCE:
                    (incoming / name).replace(destination / name)
            (incoming / PROVENANCE).replace(destination / PROVENANCE)
        return result


def plan(workspace, state, remote_cache, upload, *, architecture=None, kernel_config=None, ca_bundle=None, java_trust_store=None, distdir=None, cache_directory=None, debs_directory=None):
    workspace = workspace.resolve(strict=True)
    state = state.resolve()
    require(state != workspace and state not in workspace.parents and not state.exists(),
            "kernel state must be a new dedicated directory that does not contain the workspace")
    make_inputs = debs_path(workspace, debs_directory)
    require(state != make_inputs and state not in make_inputs.parents and make_inputs not in state.parents,
            "kernel state and Make input directory must not overlap")
    source = source_state(workspace, architecture, kernel_config)
    architecture = source["launcher"]["architecture"]
    source_directory = None
    if distdir:
        directory = Path(distdir).resolve(strict=True)
        require(directory.is_dir(), "kernel source archive directory must exist")
        require(state != directory and state not in directory.parents and directory not in state.parents,
                "kernel state and source archive directory must not overlap")
        expected_archives = {item["name"]: item["sha256"] for item in source["source_archives"]}
        require({path.name for path in directory.iterdir()} == set(expected_archives),
                "kernel source archive directory must contain exactly the pinned archives")
        files = []
        for name, expected_sha256 in sorted(expected_archives.items()):
            path = regular(directory / name)
            require(sha256(path) == expected_sha256, "kernel source archive SHA256 differs: " + name)
            files.append({"name": name, "sha256": expected_sha256, "size": path.stat().st_size})
        source_directory = {"path": str(directory), "files": files}
    registry = selected_registry()
    if remote_cache:
        remote_cache = endpoint(remote_cache)
    require(not upload or remote_cache, "remote upload requires an explicit cache endpoint")
    repository_cache = state / "repository-cache"
    disk_cache = None
    if cache_directory:
        cache_directory = Path(cache_directory).resolve()
        require(cache_directory not in {workspace, Path("/"), Path.home()}
                and cache_directory not in workspace.parents,
                "kernel cache must be a dedicated directory")
        # Use one writable namespace for unprivileged builds. Older worker
        # caches may be root-owned; do not change their ownership or contents.
        repository_cache = cache_directory / "kernel-linux-sandbox/repository_cache"
        disk_cache = cache_directory / "kernel-linux-sandbox/disk_cache"
    command = [sys.executable, "-B", str(workspace / KERNEL_PATH / "tools/bazel/build.py"),
               "--workspace", str(state / "consumer"), "--work-dir", str(state / "build"),
               "--repository-cache", str(repository_cache),
               "--target", TARGET, "--architecture", architecture,
               "--kernel-config", json.dumps(source["kernel_config"], sort_keys=True, separators=(",", ":"))]
    if remote_cache:
        command += ["--remote-cache", remote_cache]
    if disk_cache:
        command += ["--disk-cache", str(disk_cache)]
    if not upload:
        command.append("--remote-cache-read-only")
    if source_directory:
        command += ["--distdir", source_directory["path"]]
    trust = {}
    for option, path in (("ca-bundle", ca_bundle), ("java-trust-store", java_trust_store)):
        if path:
            path = regular(Path(path).resolve(strict=True))
            if option == "ca-bundle":
                require("PRIVATE KEY" not in path.read_text(), "CA bundle must not contain a private key")
            command += ["--" + option, str(path)]
            trust[option.replace("-", "_") + "_sha256"] = sha256(path)
    return {"schema": 1, "source": source, "registry": registry, "target": TARGET,
            "source_kind": "registry", "state": str(state), "debs_directory": str(make_inputs),
            "remote_cache": remote_cache, "upload_local_results": upload,
            "disk_cache": str(disk_cache) if disk_cache else None,
            "execution_trust": trust, "distdir": source_directory,
            "argv": command}


def build(workspace, state, remote_cache, upload, **options):
    with package_lock(workspace.resolve(strict=True)):
        return _build(workspace, state, remote_cache, upload, **options)


def _build(workspace, state, remote_cache, upload, *, architecture=None, kernel_config=None, ca_bundle=None, java_trust_store=None, distdir=None, cache_directory=None, debs_directory=None, execute=None, fetch=None):
    prepared = plan(workspace, state, remote_cache, upload,
                    architecture=architecture, kernel_config=kernel_config, ca_bundle=ca_bundle, java_trust_store=java_trust_store,
                    distdir=distdir, cache_directory=cache_directory, debs_directory=debs_directory)
    workspace = workspace.resolve(strict=True)
    architecture = prepared["source"]["launcher"]["architecture"]
    state = Path(prepared["state"])
    state.mkdir(parents=True, mode=0o700, exist_ok=False)
    consumer = state / "consumer"
    shutil.copytree(workspace / "tools/bazel/kernel", consumer)
    require(configure_registry(consumer) == prepared["registry"], "kernel registry selection changed")
    (state / "plan.json").write_text(json.dumps(prepared, indent=2, sort_keys=True) + "\n")
    if execute is None:
        subprocess.run(prepared["argv"], cwd=workspace, check=True)
    else:
        execute(prepared["argv"], workspace)
    invocation = json.loads(regular(state / "build/invocation.json").read_text())
    require(invocation.get("schema") == 2 and invocation.get("target") == TARGET
            and invocation.get("execution_strategy") == "linux-sandbox"
            and type(invocation.get("uid")) is int and invocation["uid"] > 0
            and invocation.get("bazel_version") == prepared["source"]["launcher"]["bazel_version"]
            and invocation.get("bazel_sha256") == prepared["source"]["launcher"]["bazel_sha256"]
            and invocation.get("kernel_config") == prepared["source"]["kernel_config"]
            and invocation.get("architecture") == architecture
            and invocation.get("host_machine") == {"amd64": "x86_64", "arm64": "aarch64"}[architecture]
            and invocation.get("remote_cache") == prepared["remote_cache"]
            and invocation.get("remote_cache_read_only") is (not prepared["upload_local_results"])
            and invocation.get("disk_cache") == prepared["disk_cache"],
            "kernel launcher invocation differs from the prepared source build")
    selected = inspect_resolution(state / "build", consumer, prepared["source"], prepared["registry"], fetch=fetch)
    result = stage_outputs(state / "build", Path(prepared["debs_directory"]), workspace, prepared["source"], selected)
    receipt = {"schema": 1, "plan": prepared, "result": result}
    (state / "kernel-receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    return receipt


def ensure(workspace, *, architecture=None, kernel_config=None, state=None, cache_directory=None,
           remote_cache=None, upload=False, ca_bundle=None,
           java_trust_store=None, distdir=None, debs_directory=None, builder=None):
    """Verify/reuse or build the kernel packages in Make's normal DEB directory."""
    workspace = workspace.resolve(strict=True)
    with package_lock(workspace):
        source = source_state(workspace, architecture, kernel_config)
        architecture = source["launcher"]["architecture"]
        destination = debs_path(workspace, debs_directory)
        provenance = destination / PROVENANCE
        if provenance.exists() or provenance.is_symlink():
            recorded = json.loads(regular(provenance).read_text())
            # Matching retained outputs must pass byte/control/provenance checks.
            # Corruption must not silently trigger a replacement compilation.
            identity = ("kernel_gitlink", "modules", "launcher", "consumer_inputs_sha256", "kernel_config")
            if all(recorded.get(name) == source[name] for name in identity):
                expected = contract(workspace, architecture, source["kernel_config"])[1]
                missing = [name for name in expected if not (destination / name).exists()
                           and not (destination / name).is_symlink()]
                if not missing:
                    result = verify(destination, workspace, source=source, architecture=architecture)
                    require(source_state(workspace, architecture, source["kernel_config"]) == source,
                            "kernel source changed during retained-package verification")
                    return {"schema": 1, "reused": True, "debs_directory": str(destination), "result": result}
                # Make clean may remove one or all DEBs. Rematerialize them using
                # Bazel's cache, while still refusing corrupted retained bytes.
                manifest = json.loads(regular(destination / MANIFEST).read_text())
                require(sha256(destination / MANIFEST) == recorded.get("manifest_sha256"),
                        "retained kernel manifest differs from its provenance")
                for item in manifest["packages"]:
                    name = item["name"]
                    require(name in expected, "unexpected retained kernel package")
                    if name not in missing:
                        path = regular(destination / name)
                        require(sha256(path) == item["sha256"] and path.stat().st_size == item["size"],
                                "kernel artifact failed size/SHA256 verification: " + name)
        else:
            owned = set(contract(workspace, architecture, source["kernel_config"])[1]) | {MANIFEST}
            require(not any((destination / name).exists() or (destination / name).is_symlink() for name in owned),
                    "kernel package publication is incomplete: missing provenance; rerun prepare.py build "
                    "with a new --state-dir and the same --debs-directory")
        temporary_state = state is None
        if temporary_state:
            states = workspace / "target/bazel-kernel-state"
            require(not states.is_symlink(), "kernel state directory must not be a symlink")
            states.mkdir(exist_ok=True)
            state = Path(tempfile.mkdtemp(prefix=architecture + "-", dir=states))
            state.rmdir()  # build() deliberately requires a fresh state path.
        else:
            state = Path(state).resolve()
        build_function = builder or _build
        receipt = build_function(workspace, state, remote_cache, upload,
                                 architecture=architecture, kernel_config=source["kernel_config"], cache_directory=cache_directory,
                                 ca_bundle=ca_bundle, java_trust_store=java_trust_store, distdir=distdir,
                                 debs_directory=destination)
        require(source_state(workspace, architecture, source["kernel_config"]) == source, "kernel source changed during automatic build")
        result = verify(destination, workspace, source=source, architecture=architecture)
        if temporary_state:
            # Bazel's persistent caches own reusable compilation results. Keep
            # lightweight resolution/build evidence, not another full kernel tree.
            for path in (state / "build/output", state / "repository-cache"):
                if path.exists():
                    require(path.is_dir() and not path.is_symlink(), "unexpected kernel build output directory")
                    shutil.rmtree(path)
        return {"schema": 1, "reused": False, "debs_directory": str(destination), "result": result,
                "plan": receipt["plan"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("plan", "build", "ensure", "verify"))
    parser.add_argument("--workspace", type=Path, default=Path.cwd())
    parser.add_argument("--state-dir", type=Path)
    parser.add_argument("--debs-directory", type=Path,
                        help="normal Make DEB directory (default: target/debs/trixie)")
    parser.add_argument("--architecture", choices=("amd64", "arm64"), default=native_architecture(),
                        help="native execution architecture; CONFIGURED_ARCH selects the kernel target")
    parser.add_argument("--kernel-config", action="append", default=[], metavar="NAME=VALUE",
                        help="effective Make kernel configuration (repeat for each variable)")
    parser.add_argument("--remote-cache")
    parser.add_argument("--upload-local-results", action="store_true")
    parser.add_argument("--ca-bundle", type=Path)
    parser.add_argument("--java-trust-store", type=Path)
    parser.add_argument("--distdir", type=Path)
    parser.add_argument("--cache-directory", type=Path,
                        help="persistent Bazel download/action cache visible to this process")
    args = parser.parse_args()
    workspace = args.workspace.resolve(strict=True)
    configuration = parse_kernel_config(args.kernel_config)
    if args.command == "ensure":
        result = ensure(workspace, architecture=args.architecture, kernel_config=configuration, state=args.state_dir,
                        cache_directory=args.cache_directory, remote_cache=args.remote_cache,
                        upload=args.upload_local_results,
                        ca_bundle=args.ca_bundle, java_trust_store=args.java_trust_store, distdir=args.distdir,
                        debs_directory=args.debs_directory)
    elif args.command in {"plan", "build"}:
        if args.state_dir is None:
            parser.error("plan/build require --state-dir")
        function = plan if args.command == "plan" else build
        result = function(workspace, args.state_dir, args.remote_cache, args.upload_local_results,
                          architecture=args.architecture, kernel_config=configuration, ca_bundle=args.ca_bundle,
                          java_trust_store=args.java_trust_store, distdir=args.distdir,
                          cache_directory=args.cache_directory, debs_directory=args.debs_directory)
    else:
        if args.state_dir or args.remote_cache or args.upload_local_results or args.ca_bundle or args.java_trust_store or args.distdir or args.cache_directory:
            parser.error("verify does not accept build options")
        result = verify(debs_path(workspace, args.debs_directory), workspace, architecture=args.architecture, kernel_config=configuration)
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
