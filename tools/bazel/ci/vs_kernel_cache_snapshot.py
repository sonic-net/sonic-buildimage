"""Find a retained kernel index produced by this repository's master push workflow."""

from http.client import HTTPException
import os
from pathlib import Path

from tools.bazel.ci.vs_host_base_snapshot import github_api


ARTIFACT = "vs-kernel-cache-index-amd64-v1"
WORKFLOW = ".github/workflows/bazel-oci.yml"


def select(api, repository, current_run_number):
    # Bound optional discovery to three GitHub requests, regardless of PR traffic.
    workflow = api("workflows/bazel-oci.yml")
    workflow_id = workflow["id"]
    if workflow["path"] != WORKFLOW or type(workflow_id) is not int or workflow_id <= 0:
        return None
    runs = api(f"workflows/{workflow_id}/runs?event=push&branch=master&per_page=100")
    trusted = {
        run["id"] for run in runs["workflow_runs"]
        if type(run["id"]) is int and run["id"] > 0 and run["run_number"] < current_run_number
        and run["workflow_id"] == workflow_id and run["path"] == WORKFLOW
        and run["event"] == "push" and run["head_branch"] == "master"
        and (run["repository"] or {}).get("full_name") == repository
        and (run["head_repository"] or {}).get("full_name") == repository
    }
    if not trusted:
        return None
    artifacts = api(f"artifacts?name={ARTIFACT}&per_page=100")["artifacts"]
    eligible = (artifact for artifact in artifacts
                if artifact["name"] == ARTIFACT and artifact["expired"] is False
                and type(artifact["id"]) is int and artifact["id"] > 0
                and (artifact["workflow_run"] or {}).get("id") in trusted)
    return max(eligible, key=lambda artifact: (artifact["created_at"], artifact["id"]), default=None)


def main():
    try:
        artifact = select(github_api, os.environ["GITHUB_REPOSITORY"], int(os.environ["GITHUB_RUN_NUMBER"]))
    except (HTTPException, OSError, RuntimeError) as error:
        print(f"VS kernel index lookup unavailable ({error}). Make will prepare the kernel normally.")
        return
    if artifact is None:
        print("No retained VS kernel index from a previous master push.")
        return
    with Path(os.environ["GITHUB_OUTPUT"]).open("a") as output:
        print(f"run_id={artifact['workflow_run']['id']}", file=output)
        print(f"artifact_id={artifact['id']}", file=output)


if __name__ == "__main__":
    main()
