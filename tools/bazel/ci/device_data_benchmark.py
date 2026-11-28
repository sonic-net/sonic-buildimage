"""Measure one VS device-data build and installation on a fresh remote-cache runner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
import time

from tools.bazel.ci.cache_reuse import json_records
from tools.bazel.image.vs.build_device_data import WORKSPACE


TARGET = "//src/sonic-device-data:vs_runtime_tar"


def owner_actions(log, restoring):
    actions = []
    for record in json_records(log):
        label = (record.get("targetLabel") or "").lstrip("@")
        if label not in (TARGET, "//src/sonic-device-data:vs_install_files"):
            continue
        hit = record.get("cacheHit", False)
        runner = record.get("runner", "")
        if hit != restoring or (runner == "remote cache hit") != restoring:
            raise ValueError(f"Unexpected cache result for {label}: {runner}")
        actions.append({"target": label, "mnemonic": record["mnemonic"], "remote_hit": hit})
    if not any(action["target"] == TARGET for action in actions):
        raise ValueError("Missing TAR action in the execution log")
    return actions


def measure(mode):
    restoring = mode == "restore"
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    if revision != os.environ["GITHUB_SHA"]:
        raise ValueError("Benchmark checkout differs from the dispatched revision")
    with TemporaryDirectory(dir=os.environ["RUNNER_TEMP"]) as directory:
        work = Path(directory)
        log = work / "execution.json"
        # Time Make's production build and export from a fresh Bazel output base.
        wrapper = work / "bazel-benchmark"
        wrapper.write_text("""#!/bin/bash
set -eu
command=$1
shift
flags=()
if [[ "$command" == build ]]; then flags+=("--execution_log_json_file=$BENCH_DIR/execution.json"); fi
exec bazel "--output_base=$BENCH_DIR/base" "--host_jvm_args=-Djavax.net.ssl.trustStore=$SONIC_BAZEL_JAVA_TRUST_STORE" "$command" "${flags[@]}" "$@"
""")
        wrapper.chmod(0o755)
        flags = ["--remote_cache_async=false",
                 "--remote_accept_cached" if restoring else "--noremote_accept_cached",
                 "--noremote_upload_local_results" if restoring else "--remote_upload_local_results"]
        env = dict(os.environ, BENCH_DIR=directory, BAZEL=str(wrapper), BAZEL_CONTAINER_ARGS=" ".join(flags))
        # The pool's verified home RC supplies all cache settings for this run.
        for name in ("BAZEL_CONTAINER_CACHE_DIR", "BAZEL_SWSS_CACHE_DIR", "BAZEL_REMOTE_AUTH", "SONIC_BAZEL_REMOTE_CACHE"):
            env.pop(name, None)
        tar = work / "sonic-device-data-vs.tar"
        start = time.monotonic()
        try:
            subprocess.run(["python3", "-B", "tools/bazel/image/vs/build_device_data.py", str(tar)], env=env, check=True)
            build_seconds = time.monotonic() - start
        finally:
            subprocess.run([wrapper, "shutdown"], cwd=WORKSPACE, env=env, check=False)
        actions = owner_actions(log, restoring)
        root = work / "root"
        database = root / "var/lib/dpkg"
        (database / "info").mkdir(parents=True)
        (database / "updates").mkdir()
        (database / "status").touch()
        install = ["python3", "-B", "scripts/install_device_data.py", "--root", str(root), "--tar", str(tar)]
        if os.geteuid() != 0:
            install = ["fakeroot", "--", *install]
        start = time.monotonic()
        subprocess.run(install, check=True)
        install_seconds = time.monotonic() - start
        package = subprocess.check_output(["dpkg-query", f"--admindir={database}", "-W",
                                           "-f", "${Status} ${Version}", "sonic-device-data"], text=True)
        if not package.startswith("install ok installed "):
            raise ValueError("Installed package is missing from the package database")
        verify = subprocess.check_output(["dpkg", f"--root={root}", "--verify", "sonic-device-data"], text=True)
        if verify:
            raise ValueError("dpkg reported changes to the installed TAR payload")
        paths = subprocess.check_output(["dpkg-query", f"--admindir={database}", "-L", "sonic-device-data"], text=True)
        with tar.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        return {"revision": revision, "mode": mode, "runner": os.environ["RUNNER_NAME"],
                "build_and_export_seconds": round(build_seconds, 3), "install_seconds": round(install_seconds, 3),
                "tar_sha256": digest, "tar_bytes": tar.stat().st_size,
                "installed_package": package, "installed_paths": len(paths.splitlines()), "actions": actions}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=["populate", "restore"])
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    result = measure(args.mode)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result))
