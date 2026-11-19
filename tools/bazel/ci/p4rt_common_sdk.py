#!/usr/bin/env python3
"""Validate P4RT's Bazel 6 build against Common's Bazel 8 SDK."""

import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def audit_graph(graph):
    """Publish counts, never raw action environments or command lines."""
    targets = {str(item["id"]): item["label"] for item in graph["targets"]}
    fragments = {str(item["id"]): item for item in graph["pathFragments"]}

    def path(identity):
        item = fragments[str(identity)]
        parent = item.get("parentId")
        return (path(parent) + "/" if parent else "") + item["label"]

    artifacts = {str(item["id"]): path(item["pathFragmentId"])
                 for item in graph["artifacts"]}
    actions = graph["actions"]
    counts = Counter(action["mnemonic"] for action in actions)
    for action in actions:
        label = targets[str(action["targetId"])]
        if (label.startswith("@sonic_swss_common//")
                and action["mnemonic"] in ("CppCompile", "CppLink", "CppArchive")):
            raise ValueError("Bazel 6 must import Common, not compile or link its sources")
        for identity in action.get("outputIds", []):
            if artifacts[str(identity)].endswith(".deb"):
                raise ValueError("Validation must not build Debian packages")
    headers = sorted(value for value in artifacts.values()
                     if "external/sonic_swss_common/include/swss/" in value)
    libraries = sorted(value for value in artifacts.values()
                       if "external/sonic_swss_common/lib/libswsscommon.so" in value)
    if not counts["CppCompile"] or not headers or not libraries:
        raise ValueError("P4RT must compile against the imported Common headers and library")
    return {"action_counts": dict(sorted(counts.items())),
            "common_compilation_actions": 0, "deb_outputs": 0,
            "sdk_header_count": len(headers), "sdk_libraries": libraries}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--output-base", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--arch", choices=("amd64", "arm64"), required=True)
    parser.add_argument("--jobs", type=int)
    args = parser.parse_args()
    source, sdk = args.source.resolve(), args.sdk.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    private = args.output_base.resolve().parent / "p4rt-private-evidence"
    private.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, USE_BAZEL_VERSION="6.4.0")
    bazel = ["bazel", "--output_base=" + str(args.output_base.resolve())]
    override = "--override_repository=sonic_swss_common=" + str(sdk)
    mirrors = "--experimental_downloader_config=" + str(source.parent / "source_mirrors.cfg")
    options = [override, mirrors, "-c", "opt", "--fission=opt", "--copt=-g",
               "--copt=-gdwarf-4", "--copt=-Wno-array-bounds",
               "--copt=-Wno-stringop-overflow", *(["--jobs=" + str(args.jobs)] if args.jobs is not None else [])]

    def run(arguments, **kwargs):
        return subprocess.run([*bazel, *arguments], cwd=source, env=env,
                              check=True, **kwargs)

    def capture(arguments):
        return run(arguments, stdout=subprocess.PIPE, text=True).stdout.strip()

    def record(name, value):
        (output / name).write_text(json.dumps(value, indent=2) + "\n")

    try:
        version = capture(["version", "--gnu_format"])
        if [line for line in version.splitlines() if line.startswith("bazel ")] != ["bazel 6.4.0"]:
            raise ValueError("P4RT must run with Bazel 6.4.0: " + version)
        (output / "bazel-version.txt").write_text(version + "\n")
        shutil.copyfile(sdk / "sdk-build.json", output / "sdk-build.json")
        tests = sorted(capture(["query", override, "tests(//p4rt_app/...)",
                                "--output=label"]).splitlines())
        if not tests or any(not label.startswith(("//p4rt_app/", "//p4rt_app:")) for label in tests):
            raise ValueError("Expected explicit P4RT test labels")
        (output / "test-targets.txt").write_text("\n".join(tests) + "\n")
        selection = "deps(set(//p4rt_app:p4rt " + " ".join(tests) + "))"
        graph_path = private / "actions.json"
        with graph_path.open("w") as stream:
            run(["aquery", *options, "--output=jsonproto", "--include_artifacts",
                 "--include_commandline=false", selection], stdout=stream)
        record("action-audit.json", audit_graph(json.loads(graph_path.read_text())))
        graph_path.unlink()
        run(["build", *options, "//p4rt_app:p4rt"])
        binary = Path(capture(["info", *options, "bazel-bin"])) / "p4rt_app/p4rt"
        header = subprocess.check_output(["readelf", "-h", binary], text=True)
        machine = "Advanced Micro Devices X86-64" if args.arch == "amd64" else "AArch64"
        if machine not in header:
            raise ValueError("P4RT executable has the wrong architecture")
        dynamic = subprocess.check_output(["readelf", "-d", binary], text=True)
        if not re.search(r"\(NEEDED\).*\[libswsscommon.so.0\]", dynamic):
            raise ValueError("P4RT must dynamically link libswsscommon.so.0")
        loaded = subprocess.check_output(["ldd", binary], text=True)
        if "not found" in loaded:
            raise ValueError("Unresolved P4RT runtime libraries:\n" + loaded)
        match = re.search(r"libswsscommon\.so\.0 => (\S+)", loaded)
        if not match or digest(match[1]) != digest(sdk / "lib/libswsscommon.so.0"):
            raise ValueError("P4RT must load the library produced by the Common SDK build")
        record("runtime.json", {"architecture": args.arch, "machine": machine,
                                "p4rt_sha256": digest(binary),
                                "common_sha256": digest(match[1]),
                                "resolved_common": match[1],
                                "sdk_common": str(sdk / "lib/libswsscommon.so.0")})
        subprocess.run([str(binary), "--only_check_args"], check=True, cwd=source)
        events = private / "test-events.json"
        result = subprocess.run([*bazel, "test", *options, "--nocache_test_results",
                                 "--test_output=errors", "--build_event_json_file=" + str(events),
                                 *tests], cwd=source, env=env)
        statuses = {}
        if events.exists():
            for line in events.read_text().splitlines():
                event = json.loads(line)
                if "testSummary" in event:
                    label = event["id"]["testSummary"]["label"]
                    statuses[label] = event["testSummary"]["overallStatus"]
        record("test-results.json", statuses)
        result.check_returncode()
        if set(statuses) != set(tests) or any(value != "PASSED" for value in statuses.values()):
            raise ValueError("Every selected P4RT test must execute and pass")
        print(f"PASS: P4RT on {args.arch} imported Common's SDK; {len(tests)} tests passed")
    finally:
        subprocess.run([*bazel, "shutdown"], cwd=source, env=env, check=False)


if __name__ == "__main__":
    main()
