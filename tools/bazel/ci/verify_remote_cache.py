#!/usr/bin/env python3
"""Verify remote action-cache reuse with a small source-only OCI archive."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.verify_agent_cache import (
    TARGET,
    action_evidence,
    archive_output,
    remove_temporary_work,
)


def run_build(bazel, workspace, work, phase, upload):
    evidence = work / phase
    evidence.mkdir()
    subprocess.run(
        [
            bazel, "--batch", "--output_base=" + str(evidence / "output"),
            "build", TARGET,
            "--disk_cache=", "--remote_executor=",
            "--experimental_remote_downloader=",
            "--remote_accept_cached=true", "--remote_download_outputs=all",
            "--remote_upload_local_results=" + str(upload).lower(),
            "--execution_log_json_file=" + str(evidence / "execution.json"),
            "--build_event_json_file=" + str(evidence / "events.jsonl"),
        ],
        cwd=workspace, check=True,
    )
    output = archive_output(evidence / "events.jsonl")
    return {
        "actions": action_evidence(evidence / "execution.json"),
        "archive_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
    }


def verify(bazel, workspace, upload):
    work = Path(tempfile.mkdtemp(prefix="sonic-remote-cache-"))
    try:
        seed = run_build(bazel, workspace, work, "seed", upload)
        reader = run_build(bazel, workspace, work, "reader", False)
        for mnemonic, action in reader["actions"].items():
            if not action["cache_hit"] or "remote" not in str(action["runner"]).lower():
                raise ValueError(
                    f"{mnemonic} was not restored from the remote cache: {action}. "
                    "Check the remote-cache connection and seed this fixture with "
                    "an upload-enabled trusted build before running read-only CI."
                )
        if seed["archive_sha256"] != reader["archive_sha256"]:
            raise ValueError("remote-cache reuse changed the fixture archive")
        return {
            "status": "passed", "target": TARGET,
            "upload_local_results": upload, "seed": seed, "reader": reader,
        }
    finally:
        remove_temporary_work(work)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--upload-local-results", choices=("true", "false"), default="false")
    parser.add_argument("--artifacts", type=Path, default=Path("target/bazel-remote-cache-check"))
    args = parser.parse_args()
    args.artifacts.mkdir(parents=True, exist_ok=True)
    summary = {"status": "failed", "target": TARGET}
    try:
        summary = verify(args.bazel, Path.cwd(), args.upload_local_results == "true")
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        summary["error"] = str(exc)
        print(f"ERROR: Bazel remote cache check failed: {exc}", file=sys.stderr)
    finally:
        (args.artifacts / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    if summary["status"] != "passed":
        return 1
    print("Bazel remote cache check passed: all fixture actions were remote cache hits.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
