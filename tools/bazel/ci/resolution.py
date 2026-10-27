"""Retain generated Bazel resolution evidence without changing source inputs."""

import hashlib
import json
from pathlib import PurePosixPath
import re
import shutil
import subprocess


def inspect_graph(stdout, stderr, returncode):
    """Validate the full graph, allowing complete known unused extension failures."""
    graph = json.loads(stdout)
    if graph.get("key") != "<root>" or not graph.get("root") or not graph.get("dependencies"):
        raise ValueError("Missing resolved module graph")
    expanded, referenced = set(), set()

    def visit(node):
        key = node["key"]
        referenced.add(key)
        if not node.get("unexpanded", False):
            expanded.add(key)
        for field in ("dependencies", "indirectDependencies", "cycles"):
            for dependency in node.get(field, []):
                visit(dependency)

    visit(graph)
    if referenced != expanded:
        raise ValueError("Incomplete resolved module graph")
    result = {"returncode": returncode, "module_count": len(expanded),
              "module_graph_complete": True, "extension_inspection_complete": returncode == 0}
    if returncode == 0:
        return result

    # Bazel 8.5.1's mod inspector evaluates every extension, including these
    # unused extensions after the required Linux build/tests already succeeded.
    # Keep their diagnostics; reject every other failure and incomplete graph.
    errors = [line for line in stderr.splitlines() if line.startswith(("ERROR:", "Error:"))]
    groups = [
        ("abseil-cpp@20240722.0", "cc_configure_extension", [
            r"ERROR: @@bazel_tools//tools/cpp:cc_configure\.bzl does not export a module extension called cc_configure_extension, yet its use is requested at https://bcr\.bazel\.build/modules/abseil-cpp/20240722\.0/MODULE\.bazel:23:29",
        ]),
        ("rules_apple@3.5.1", "apple", [
            r"ERROR: Traceback \(most recent call last\):",
            r"Error: 'struct' value has no field or method 'AppleDynamicFramework'",
            r"ERROR: Error loading '@@rules_apple\+//apple:apple\.bzl' for module extensions, requested by https://bcr\.bazel\.build/modules/rules_apple/3\.5\.1/MODULE\.bazel:27:48: at [^\n]+/rules_apple\+/apple/apple\.bzl:27:5: initialization of module 'apple/internal/apple_xcframework_import\.bzl' failed: at [^\n]+/rules_apple\+/apple/apple\.bzl:27:5: initialization of module 'apple/internal/apple_xcframework_import\.bzl' failed",
        ]),
    ]
    failures, matched = [], set()
    for module, extension, patterns in groups:
        matches = [[index for index, error in enumerate(errors) if re.fullmatch(pattern, error)]
                   for pattern in patterns]
        if any(matches):
            if module not in expanded or any(len(indices) != 1 for indices in matches):
                raise ValueError("Incomplete or duplicate known extension failure")
            failures.append(module + ":" + extension)
            matched.update(indices[0] for indices in matches)
    count = len(failures)
    summary = f"ERROR: Results may be incomplete as {count} extension{'s' if count != 1 else ''} failed."
    remaining = [error for index, error in enumerate(errors) if index not in matched]
    if returncode != 2 or not failures or remaining != [summary]:
        raise ValueError("Unexpected module graph inspection failure")
    result["unused_extension_failures"] = failures
    return result


def collect(workspace, directory, *, bazel=("bazel",), options=(), source_files=None):
    """Retain module provenance without changing the tested extension lock."""
    lock = workspace / "MODULE.bazel.lock"
    tested_lock = lock.read_bytes()
    command = [*bazel, "mod", "graph", "--output=json", "--extension_info=hidden",
               "--lockfile_mode=off", "--color=no", "--curses=no", *options]
    result = subprocess.run(command, cwd=workspace, text=True, capture_output=True)
    (directory / "module-graph.json").write_text(result.stdout)
    (directory / "module-graph.stderr").write_text(result.stderr)
    (directory / "module-graph-inspection.json").write_text(
        json.dumps({"argv": command, "returncode": result.returncode}, indent=2) + "\n")
    inspection = inspect_graph(result.stdout, result.stderr, result.returncode)
    inspection["argv"] = command
    (directory / "module-graph-inspection.json").write_text(json.dumps(inspection, indent=2) + "\n")
    if lock.read_bytes() != tested_lock:
        raise ValueError("Graph inspection changed the tested extension lock")
    retained = retain(workspace, directory, result.stdout, graph_filename="module-graph.json",
                      source_files=source_files)
    retained["inspection"] = inspection
    return retained


def retain(workspace, directory, graph, graph_filename="module-graph.txt", *, source_files=None):
    """Archive the tested workspace's lock and graph; reject tracked changes."""
    def git(*args):
        return subprocess.check_output(["git", "-c", "safe.directory=" + str(workspace),
                                        "-C", str(workspace), *args], text=True)

    if git("ls-files", "--", "MODULE.bazel.lock").strip():
        raise ValueError("MODULE.bazel.lock must be generated, not tracked")
    git("check-ignore", "--quiet", "MODULE.bazel.lock")
    checked_paths = None
    if source_files is not None:
        if isinstance(source_files, (str, bytes)) or not source_files:
            raise ValueError("source_files must be a nonempty collection of relative paths")
        checked_paths = set()
        root = workspace.resolve()
        for name in source_files:
            if (not isinstance(name, str) or not re.fullmatch(r"[A-Za-z0-9_./+-]+", name) or
                    PurePosixPath(name).is_absolute() or
                    any(part in ("", ".", "..") for part in name.split("/"))):
                raise ValueError("Invalid source input path: " + repr(name))
            resolved = (root / name).resolve()
            if not resolved.is_relative_to(root):
                raise ValueError("Source input escapes workspace: " + name)
            checked_paths.update((name, resolved.relative_to(root).as_posix()))
        if not checked_paths:
            raise ValueError("source_files must not be empty")
        # Git does not inspect paths inside a submodule from its superproject.
        # Include each relevant gitlink so changing that producer's checkout
        # cannot hide behind a scoped file check. Unrelated Make builds may
        # legitimately leave their own patched submodule commits in place.
        for entry in git("ls-files", "--stage", "-z").split("\0"):
            if not entry:
                continue
            metadata, name = entry.split("\t", 1)
            if metadata.split()[0] == "160000" and any(
                    path == name or path.startswith(name + "/") for path in checked_paths):
                checked_paths.add(name)
    git("diff", "--exit-code", *(["--ignore-submodules=none"] if checked_paths is not None else []),
        "HEAD", "--", *sorted(checked_paths or ()))
    lock = workspace / "MODULE.bazel.lock"
    if not lock.is_file() or not lock.stat().st_size:
        raise ValueError("Bazel did not generate MODULE.bazel.lock")
    # A reused lock can retain old lookup records; these are evidence, not
    # necessarily the current effective registry options.
    registry_urls = sorted({url.split("/modules/", 1)[0]
                            for url in json.loads(lock.read_text())["registryFileHashes"]
                            if "/modules/" in url})
    shutil.copyfile(lock, directory / "MODULE.bazel.lock")
    (directory / graph_filename).write_text(graph)
    files = {}
    for name in ("MODULE.bazel.lock", graph_filename):
        path = directory / name
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
        files[name] = {"sha256": digest.hexdigest(), "bytes": path.stat().st_size}
    return {"lockfile_mode": "update", "tracked_files_unchanged": True,
            **({"checked_source_files": sorted(checked_paths)} if checked_paths is not None else {}),
            "observed_registry_urls": registry_urls,
            "generated_evidence": files}
